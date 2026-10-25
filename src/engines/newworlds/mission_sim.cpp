// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/mission_sim.h"

#include "engines/newworlds/colony.h"
#include "engines/newworlds/start_data.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

namespace reluva::newworlds {

namespace {

constexpr float kBlock = 16384;
constexpr double kStep = 0.1;
// How long before something counts as stuck: a unit with a path that
// hasn't got nearer, a group still gathering, one idle on a leg, a build
// whose spot hasn't cleared, a unit firing at something it doesn't harm.
constexpr double kUnitStuck = 30, kGathering = 180, kOnLeg = 240, kBuilding = 120, kHarmless = 120;

std::pair<float, float> worldAt(int a, int b) { return {float(b) / 8 * kBlock, float(a) / 8 * kBlock}; }

std::string orderName(const NpcOrder& o) {
    std::string task = o.task == 4                    ? "guard"
                       : o.task == 1 || o.task == 2   ? "patrol"
                       : o.task == 3 || o.task == 9   ? "build"
                                                      : "assault";
    const CombatType* t = combatType(o.type);
    return std::string(*o.name ? o.name : "(unnamed)") + " [slot " + std::to_string(o.slot) + " " + task + " " +
           std::to_string(o.count) + " x " + (t ? t->name : std::to_string(o.type)) + "]";
}

} // namespace

std::string mapPosition(float x, float y) {
    return std::to_string(int(y / kBlock * 8)) + ":" + std::to_string(int(x / kBlock * 8));
}

MissionSim::MissionSim(std::string_view mission, char side, const Ground& ground)
    : mission_(mission), side_(side), player_(side == 'K' ? 1 : side == 'R' ? 2 : 0) {
    world_ = std::make_unique<World>(ground.terrain());
    ColonyMap map{[&ground](int bx, int by) { return ground.depositAt(bx, by); },
                  [&ground](int bx, int by) { return ground.depositAmount(bx, by); }};
    Stock stock{500, 1000, 2300, 500, 4000, 5000};
    startStock(mission_, side_, stock);
    economy_ = std::make_unique<Economy>(side_, stock, 50, map);
    economy_->setOfficers(int(officers(mission_, player_).size()));

    // The player's colony as the mission places it, its hub first (else a
    // hub where the recordings put the colony); its vehicles.
    std::vector<StartPlacement> placed = startPlacements(mission_, side_);
    std::stable_sort(placed.begin(), placed.end(), [](const StartPlacement& a, const StartPlacement& b) {
        const ObjectType* ta = objectType(a.type);
        const ObjectType* tb = objectType(b.type);
        return (ta && std::string_view(ta->name) == "colony-hub") > (tb && std::string_view(tb->name) == "colony-hub");
    });
    int hubA = -1, hubB = -1;
    for (const StartPlacement& p : placed)
        if (const ObjectType* t = objectType(p.type); t && t->building && *t->model)
            if (const CombatType* ct = combatType(p.type)) {
                const auto [x, y] = worldAt(p.a, p.b);
                const int id = spawnObject(*world_, player_, side_, *ct, x, y, {}, t->model, true);
                if (buildingType(t->name))
                    structureOf_[id] = economy_->addBuilt(t->name, x, y);
                if (hubA < 0 && std::string_view(t->name) == "colony-hub")
                    hubA = p.a, hubB = p.b;
            }
    if (hubA < 0) {
        const ColonySite site = colonySite(mission_, side_);
        if (site.a >= 0)
            for (int id = 1; id < 330; ++id)
                if (const CombatType* t = combatType(id); t && t->building && t->race == side_ &&
                                                          std::string_view(t->name) == "colony-hub") {
                    const auto [x, y] = worldAt(site.a, site.b);
                    structureOf_[spawnObject(*world_, player_, side_, *t, x, y, {}, {}, true)] =
                        economy_->addBuilt("colony-hub", x, y);
                    hubA = site.a, hubB = site.b;
                    break;
                }
    }
    // (Bees on the hub's spot stay inside; other vehicles there park round it.)
    std::map<std::pair<int, int>, int> onSpot;
    for (const StartPlacement& p : placed)
        if (const ObjectType* t = objectType(p.type); t && !t->building && *t->model)
            if (const CombatType* ct = combatType(p.type); ct && !(isBee(*ct) && p.a == hubA && p.b == hubB)) {
                const auto [x, y] = worldAt(p.a, p.b);
                const auto [sx, sy] = parkingSpot(onSpot[{p.a, p.b}]++, x, y, p.a == hubA && p.b == hubB);
                const auto [px, py] = world_->openSpot(sx, sy);
                spawnObject(*world_, player_, side_, *ct, px, py, {}, t->model, true);
            }
    for (const SidePlacement* p : sidePlacements(mission_, player_))
        if (const CombatType* t = combatType(p->type); t && *p->label && t->building && !buildingType(t->name)) {
            const auto [x, y] = worldAt(p->a, p->b);
            spawnObject(*world_, player_, side_, *t, x, y, p->label, p->model, true);
        }

    ai_ = std::make_unique<AiSides>(mission_, player_, *world_);
    ai_->setGround(map);
    ai_->start();
    script_ = findMissionScript(mission_, side_);
    fieldWork_ = std::make_unique<FieldWork>(mission_, player_, *world_,
                                             script_ ? script_->sites : std::map<std::string, std::pair<int, int>>{});
    fieldWork_->placeOfficers();
    research_ = std::make_unique<Research>(mission_, player_);
    effects_ = std::make_unique<ScriptEffects>(player_, *world_, ai_.get(), economy_.get(), script_);
    fog_ = std::make_unique<Fog>(ground.blocksWide(), ground.blocksHigh());
    fog_->refresh(*world_);
    fieldWork_->setFog(fog_.get());
    repairs_ = std::make_unique<Repairs>(*world_);
    repairs_->setEngineering([this](int id) { return fieldWork_->skill(id, 1); });
    world_->setDamage([this](const Unit& target, float damage) {
        return shieldedDamage(target, damage, fog_.get(), target.slot == player_ ? fieldWork_->skill(target.id, 2) : 0);
    });
    world_->setMarksmanship(
        [this](const Unit& shooter) { return shooter.slot == player_ ? fieldWork_->skill(shooter.id, 2) : 0.0f; });

    // Units placed where they can't move.
    for (const Unit& u : world_->units())
        if (!u.building && !u.flies && !world_->passable(int(u.x / kTile), int(u.y / kTile))) {
            placedBadly_.insert(u.id);
            note("placed on ground it can't move on: slot " + std::to_string(u.slot) + " " + u.name + " at " +
                 mapPosition(u.x, u.y));
        }
    if (script_) {
        run_ = std::make_unique<MissionRun>(*script_, findMissionScript("generic", 'F'));
        const ScriptEvent start{"start", {}};
        deliver(run_->fire(start));
    }
}

void MissionSim::note(const std::string& line) {
    char at[32];
    std::snprintf(at, sizeof at, "%5.0f s  ", clock_);
    log_.push_back(at + line);
}

void MissionSim::advance(double seconds) {
    for (double left = seconds; left > 1e-9; left -= kStep)
        step(std::min(left, kStep));
}

void MissionSim::step(double dt) {
    const double before = clock_;
    clock_ += dt;
    const bool second = std::floor(clock_) != std::floor(before);
    std::vector<ScriptEvent> happened;
    ai_->advance(dt);
    world_->advance(dt);
    for (fogClock_ += dt * 20; fogClock_ >= 1; fogClock_ -= 1)
        fog_->tick(*world_);
    repairs_->advance(dt);
    const auto events = world_->takeEvents();
    for (const WorldEvent& e : events)
        if (e.kind == WorldEvent::Destroyed) {
            if (auto it = structureOf_.find(e.what.id); it != structureOf_.end()) {
                economy_->remove(it->second);
                structureOf_.erase(it);
            }
            note("destroyed: slot " + std::to_string(e.what.slot) + " " + e.what.name + " at " +
                 mapPosition(e.what.x, e.what.y));
        }
    auto said = ai_->observe(events);
    happened.insert(happened.end(), said.begin(), said.end());
    auto done = effects_->advance(dt);
    happened.insert(happened.end(), done.begin(), done.end());
    auto found = research_->advance(dt);
    happened.insert(happened.end(), found.begin(), found.end());
    auto worked = fieldWork_->advance(dt, [this](const ScriptEvent& e) { return run_ && run_->handles(e); });
    happened.insert(happened.end(), worked.begin(), worked.end());
    // The player's buildings: health in, progress and efficiency out.
    for (const auto& [id, structure] : structureOf_)
        if (const Unit* u = world_->unit(id))
            economy_->setHealth(structure, u->hp / u->maxHp);
    economy_->setVehicles(int(std::count_if(world_->units().begin(), world_->units().end(),
                                            [&](const Unit& u) { return u.slot == player_ && !u.building; })));
    auto built = economy_->advance(dt);
    happened.insert(happened.end(), built.begin(), built.end());
    for (const ColonyStructure& c : economy_->structures())
        for (const auto& [id, structure] : structureOf_)
            if (structure == c.id)
                if (Unit* u = world_->unit(id))
                    u->built = c.progress, u->efficiency = economy_->efficiency(c);
    if (second) {
        // The sites the script knows: seen, scanned (the fog's marks).
        if (script_)
            for (const auto& [site, at] : script_->sites) {
                const int bx = at.second / 8, by = at.first / 8;
                if (fog_->has(bx, by, Fog::Seen, player_) && seenSites_.insert(site).second)
                    happened.push_back({"seen", {site}, at.first, at.second});
                if (fog_->has(bx, by, Fog::Scanned, player_) && scannedSites_.insert(site).second)
                    happened.push_back({"cell-scanned", {site}, at.first, at.second});
            }
        const Stock st = economy_->stock();
        for (int m = 0; m < kMaterials; ++m) {
            std::string name = materialName(m);
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return char(std::tolower(c)); });
            happened.push_back({"stock", {name, std::to_string(st[size_t(m)])}});
        }
        std::map<std::string, int> counts;
        for (const ColonyStructure& c : economy_->structures())
            counts[c.type] += c.progress >= 1;
        for (const auto& [type, n] : counts)
            if (auto it = lastCounts_.find(type); it == lastCounts_.end() || it->second != n)
                happened.push_back({"count", {type, std::to_string(n)}});
        lastCounts_ = std::move(counts);
        watchUnits();
    }
    if (run_ && !run_->finished()) {
        for (const ScriptEvent& e : happened)
            deliver(run_->fire(e));
        deliver(run_->advance(dt));
    }
}

void MissionSim::deliver(const std::vector<MissionRun::Output>& out) {
    for (const MissionRun::Output& o : out) {
        switch (o.kind) {
        case MissionRun::Output::Won: note("mission won"); break;
        case MissionRun::Output::Lost:
            lost_ = true;
            note("mission lost");
            break;
        case MissionRun::Output::Failing: note("mission failing"); break;
        case MissionRun::Output::Ai:
            if (!ai_->order(o.sender, o.text))
                note("script's ai order not found: " + o.sender + " " + o.text);
            else
                note("ai " + o.sender + " " + o.text);
            break;
        case MissionRun::Output::Stance: ai_->stance(o.sender, o.text); break;
        case MissionRun::Output::Research: research_->unlock(o.text); break;
        case MissionRun::Output::Effect:
        case MissionRun::Output::Stock: effects_->apply(o.sender, o.text); break;
        default: break;
        }
    }
}

// Each second: a unit still more than half a block from where it's going
// that hasn't got a block nearer in half a minute is stuck (one milling
// round its group's spot isn't).
void MissionSim::watchUnits() {
    // Firing at one target for two minutes without denting it.
    std::map<int, Shooting> shooting;
    for (const Unit& u : world_->units()) {
        const Unit* t = u.firing && u.target ? world_->unit(u.target) : nullptr;
        if (!t)
            continue;
        auto it = shooting_.find(u.id);
        if (it == shooting_.end() || it->second.target != t->id || t->hp < it->second.hp - 0.5f) {
            shooting[u.id] = {t->id, t->hp, clock_};
            continue;
        }
        shooting[u.id] = it->second;
        if (clock_ - it->second.since >= kHarmless && harmless_.insert(u.id).second)
            note("firing without effect: slot " + std::to_string(u.slot) + " " + u.name + " at " + mapPosition(u.x, u.y) +
                 " on slot " + std::to_string(t->slot) + "'s " + t->name);
    }
    shooting_ = std::move(shooting);
    std::map<int, Track> next;
    for (const Unit& u : world_->units()) {
        if (u.building || !u.moving() || u.firing)
            continue;
        const auto [ex, ey] = u.path.back();
        const float left = std::hypot(ex - u.x, ey - u.y);
        if (left < kBlock / 2)
            continue;
        auto it = tracks_.find(u.id);
        // Kept: the same destination, and no nearer than a block closer.
        if (it == tracks_.end() || std::hypot(ex - it->second.x, ey - it->second.y) > kBlock / 2 ||
            left < it->second.left - kBlock) {
            next[u.id] = {ex, ey, left, clock_};
            continue;
        }
        next[u.id] = it->second;
        if (clock_ - it->second.since >= kUnitStuck && stuck_.insert(u.id).second)
            note("stuck: slot " + std::to_string(u.slot) + " " + u.name + " at " + mapPosition(u.x, u.y) +
                 " on its way to " + mapPosition(ex, ey));
    }
    tracks_ = std::move(next);
}

std::vector<std::string> MissionSim::problems() const {
    std::vector<std::string> out;
    for (int id : stuck_)
        if (const Unit* u = world_->unit(id); u && u->moving() && tracks_.count(id))
            out.push_back("stuck: unit, slot " + std::to_string(u->slot) + " " + u->name + " at " + mapPosition(u->x, u->y));
    for (int id : placedBadly_)
        if (const Unit* u = world_->unit(id))
            out.push_back("stuck: unit placed on ground it can't move on, slot " + std::to_string(u->slot) + " " + u->name +
                          " at " + mapPosition(u->x, u->y));
    for (int id : harmless_)
        if (const Unit* u = world_->unit(id); u && shooting_.count(id))
            if (const Unit* t = world_->unit(shooting_.at(id).target))
                out.push_back("stuck: firing without effect, slot " + std::to_string(u->slot) + " " + u->name + " at " +
                              mapPosition(u->x, u->y) + " on slot " + std::to_string(t->slot) + "'s " + t->name);
    for (const AiSides::OrderStatus& s : ai_->status()) {
        const NpcOrder& o = *s.order;
        if (!s.started)
            continue;
        if (o.task == 3 || o.task == 9) {
            if (!s.site && s.legSeconds >= kBuilding) {
                const Unit* b = s.blocker ? world_->unit(s.blocker) : nullptr;
                out.push_back("stuck: build never placed (its spot " + std::to_string(o.way[0][0]) + ":" +
                              std::to_string(o.way[0][1]) + " " +
                              (b ? "has slot " + std::to_string(b->slot) + "'s " + b->name + " at " + mapPosition(b->x, b->y)
                                 : std::string("refused by the economy")) +
                              "): " + orderName(o));
            }
            continue;
        }
        if (!s.gathered && s.legSeconds >= kGathering) {
            // A group waits as the game's would: for a yard its side doesn't
            // have, for its yards' queue, or for yards standing idle (no crew
            // once the hub is gone, too little power).
            const Economy* colony = ai_->colony(o.slot);
            int yards = 0, working = 0;
            if (colony)
                for (const ColonyStructure& c : colony->structures())
                    if (c.type == "vehicle-yard" && c.progress >= 1) {
                        ++yards;
                        working += c.crew > 0 && colony->efficiency(c) > 0.35f;
                    }
            const std::string why = !yards     ? "no vehicle yard"
                                    : !working ? "its vehicle yards idle (no crew or power)"
                                               : std::to_string(colony->vehicleJobs().size()) + " vehicles queued";
            out.push_back("waits: group gathering for " +
                          std::to_string(int(s.legSeconds)) + " s (has " + std::to_string(s.units) + ", " +
                          std::to_string(s.awaited) + " being made; " + why + "): " + orderName(o));
        } else if (s.gathered && !s.hunting && o.task != 4 && s.units > 0 && s.legSeconds >= kOnLeg &&
                   std::none_of(s.ids.begin(), s.ids.end(), [&](int id) {
                       const Unit* u = world_->unit(id);
                       return u && (u->moving() || u->firing);
                   })) {
            const auto [wx, wy] = worldAt(o.way[s.leg][0], o.way[s.leg][1]);
            out.push_back("stuck: group idle on leg " + std::to_string(s.leg + 1) + " for " +
                          std::to_string(int(s.legSeconds)) + " s, bound for " + mapPosition(wx, wy) + ": " + orderName(o));
        }
    }
    return out;
}

} // namespace reluva::newworlds
