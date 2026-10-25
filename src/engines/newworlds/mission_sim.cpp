// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/mission_sim.h"

#include "engines/newworlds/colony.h"
#include "engines/newworlds/start_data.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <tuple>

namespace reluva::newworlds {

namespace {

constexpr float kBlock = 16384;
constexpr double kStep = 0.1;
// How long before something counts as stuck: a unit with a path that
// hasn't got nearer, a group still gathering, one idle on a leg, a build
// whose spot hasn't cleared, a unit firing at something it doesn't harm.
// How long an object's alert lasts (the game's 41 Tricorder redraws,
// taken as ticks), and the least time between two "under attack" voices.
constexpr int kAlertTicks = 41;
constexpr double kAttackVoiceEvery = 30;
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

std::string unitLine(int type, int group, int voice, int line) {
    struct Set {
        const char* stem;
        int lines[4]; // selected, selected, ordered, ordered
        char voices[2];
    };
    static const std::map<int, Set> kSets = [] {
        const Set fapc{"fapc", {3, 4, 7, 12}, {'a', 'b'}}, ffgs{"ffgs", {2, 3, 9, 10}, {'c', 'd'}},
            fmsg{"fmsg", {3, 4, 8, 12}, {'b', 'e'}}, fpa{"fpa", {1, 3, 7, 9}, {'a', 'c'}},
            fpv{"fpv", {4, 5, 7, 8}, {'b', 'd'}}, fsv{"fsv", {1, 5, 10, 11}, {'c', 'e'}};
        const Set kapc{"kapc", {1, 2, 4, 5}, {'a', 'b'}}, kdt{"kdt", {1, 2, 4, 5}, {'b', 'd'}},
            kfgs{"kfgs", {1, 2, 4, 5}, {'c', 'd'}}, kmdb{"kmdb", {1, 2, 4, 5}, {'b', 'e'}},
            kpa{"kpa", {1, 2, 4, 5}, {'a', 'c'}}, ksv{"ksv", {1, 2, 4, 5}, {'c', 'e'}};
        const Set rapc{"rapc", {3, 4, 7, 12}, {'a', 'b'}}, rct{"rct", {4, 5, 7, 8}, {'b', 'd'}},
            rfgs{"rfgs", {2, 3, 9, 10}, {'c', 'd'}}, rpa{"rpa", {1, 3, 7, 9}, {'a', 'c'}},
            rsv{"rsv", {1, 5, 10, 11}, {'c', 'e'}};
        return std::map<int, Set>{{56, fapc}, {55, fapc}, {97, ffgs}, {154, fmsg}, {47, fpa}, {9, fpa},
                                  {8, fpv},   {62, fsv},  {88, kapc}, {40, kapc}, {10, kdt},  {229, kdt},
                                  {228, kmdb}, {36, kpa}, {37, kfgs}, {95, ksv},  {89, rapc}, {64, rapc},
                                  {230, rapc}, {35, rct}, {148, rct}, {98, rfgs}, {87, rpa},  {63, rsv}};
    }();
    const auto it = kSets.find(type);
    if (it == kSets.end() || group < 0 || group > 2)
        return {};
    const Set& set = it->second;
    if (group == 2)
        return std::string(1, set.stem[0]) + "neg1";
    char number[4];
    std::snprintf(number, sizeof number, "%03d", set.lines[group * 2 + (line & 1)]);
    return set.stem + std::string(number) + set.voices[voice & 1];
}

std::string mapPosition(float x, float y) {
    return std::to_string(int(y / kBlock * 8)) + ":" + std::to_string(int(x / kBlock * 8));
}

std::vector<ColonyBuilding> MissionSim::placedColony(std::string_view mission, char side) {
    std::vector<ColonyBuilding> out;
    for (const StartPlacement& p : startPlacements(mission, side))
        if (const ObjectType* t = objectType(p.type); t && t->building && *t->model) {
            const ColonyBuilding c{t->name, t->model, p.a, p.b};
            if (std::string_view(t->name) == "colony-hub")
                out.insert(out.begin(), c);
            else
                out.push_back(c);
        }
    if (!out.empty() && out[0].name != "colony-hub")
        out.clear();
    return out;
}

namespace {

// The headless colony: the mission's own, else its hub where the
// recordings put the colony.
std::vector<ColonyBuilding> headlessColony(std::string_view mission, char side) {
    std::vector<ColonyBuilding> out = MissionSim::placedColony(mission, side);
    if (out.empty())
        if (const ColonySite site = colonySite(mission, side); site.a >= 0)
            out.push_back({"colony-hub", buildingModel("colony-hub", side), site.a, site.b});
    return out;
}

} // namespace

MissionSim::MissionSim(std::string_view mission, char side, const Ground& ground)
    : MissionSim(mission, side, ground.terrain(),
                 ColonyMap{[&ground](int bx, int by) { return ground.depositAt(bx, by); },
                           [&ground](int bx, int by) { return ground.depositAmount(bx, by); }},
                 ground.blocksWide(), ground.blocksHigh(), headlessColony(mission, side)) {}

MissionSim::MissionSim(std::string_view mission, char side, Terrain terrain, ColonyMap map, int blocksWide,
                       int blocksHigh, std::vector<ColonyBuilding> colony)
    : mission_(mission), side_(side), player_(side == 'K' ? 1 : side == 'R' ? 2 : 0), blocksWide_(blocksWide),
      blocksHigh_(blocksHigh), colony_(std::move(colony)) {
    world_ = std::make_unique<World>(std::move(terrain));
    // The game's default stock (exe.md#the-stock), or the mission's own;
    // the Romulan recordings start a construction yard's cost short.
    Stock stock = side_ == 'R' ? Stock{500, 988, 2211, 497, 3500, 5000} : Stock{500, 1000, 2300, 500, 4000, 5000};
    startStock(mission_, side_, stock);
    economy_ = std::make_unique<Economy>(side_, stock, 50, map);
    economy_->setOfficers(int(officers(mission_, player_).size()));
    for (const ColonyBuilding& c : colony_) {
        const auto [x, y] = worldAt(c.a, c.b);
        structureIds_.push_back(economy_->addBuilt(c.name, x, y));
    }
    // The other sides as the mission places them, and the colony's own
    // buildings as things they can shoot at.
    ai_ = std::make_unique<AiSides>(mission_, player_, *world_);
    ai_->setGround(map);
    ai_->start();
    for (size_t i = 0; i < colony_.size(); ++i)
        spawnColonyObject(i);
    script_ = findMissionScript(mission_, side_);
    fieldWork_ = std::make_unique<FieldWork>(mission_, player_, *world_,
                                             script_ ? script_->sites : std::map<std::string, std::pair<int, int>>{});
    fieldWork_->placeOfficers();
    research_ = std::make_unique<Research>(mission_, player_);
    effects_ = std::make_unique<ScriptEffects>(player_, *world_, ai_.get(), economy_.get(), script_);

    // The mission's own vehicles where it places them (several on one spot
    // spread a little; bees on the hub's spot inside it, the construction
    // and cargo bees flying out to work); else four ground scouts in a
    // row south of the hub. Each on the nearest open tile.
    float hx = float(blocksWide_) * kBlock / 2, hy = float(blocksHigh_) * kBlock / 2;
    if (!colony_.empty())
        std::tie(hx, hy) = worldAt(colony_[0].a, colony_[0].b);
    auto onHub = [&](const StartPlacement& p) { return !colony_.empty() && p.a == colony_[0].a && p.b == colony_[0].b; };
    auto bee = [](int type) {
        const CombatType* ct = combatType(type);
        return ct && isBee(*ct);
    };
    struct Start {
        UnitKind kind;
        float x, y;
        int type;
    };
    std::vector<Start> start;
    std::map<std::pair<int, int>, int> onSpot;
    for (const StartPlacement& p : startPlacements(mission_, side_)) {
        const ObjectType* t = objectType(p.type);
        if (!t || t->building)
            continue;
        if (onHub(p) && bee(p.type)) {
            ++housed_;
            ++beesHoused_[combatType(p.type)->name];
        } else if (*t->model) {
            const auto [x, y] = worldAt(p.a, p.b);
            const auto [sx, sy] = parkingSpot(onSpot[{p.a, p.b}]++, x, y, onHub(p));
            start.push_back({UnitKind{t->name, t->model, groundScout(side_).speed * t->pace / 0.125f}, sx, sy, p.type});
        }
    }
    economy_->setBees(beesHoused_["construction-bee"], beesHoused_["cargo-bee"], beesHoused_["worker-bee"]);
    if (start.empty())
        for (int i = 0; i < 4; ++i)
            start.push_back({groundScout(side_), hx + (float(i) - 1.5f) * 2500, hy - 1.4f * kTile, 0});
    for (const Start& at : start) {
        const auto [px, py] = world_->openSpot(at.x, at.y);
        if (const CombatType* ct = combatType(at.type)) {
            const int id = spawnObject(*world_, player_, side_, *ct, px, py, {}, at.kind.model, true);
            world_->unit(id)->kind = at.kind;
        } else {
            world_->unit(world_->addUnit(side_, at.kind, px, py, 3.1416f / 2))->slot = player_;
        }
    }
    // The player's own named objects the colony doesn't cover (K7's escape
    // pods): placed for the scripts.
    for (const SidePlacement* p : sidePlacements(mission_, player_))
        if (const CombatType* t = combatType(p->type); t && *p->label && t->building && !buildingType(t->name)) {
            const auto [x, y] = worldAt(p->a, p->b);
            spawnObject(*world_, player_, side_, *t, x, y, p->label, p->model, true);
        }

    // The fog, marked for everyone placed; repairs; hits lessened by
    // shield fields and security officers, made stronger by them; the
    // defensive turrets' power.
    fog_ = std::make_unique<Fog>(blocksWide_, blocksHigh_);
    fog_->refresh(*world_);
    fieldWork_->setFog(fog_.get());
    repairs_ = std::make_unique<Repairs>(*world_);
    repairs_->setEngineering([this](int id) { return fieldWork_->skill(id, 1); });
    world_->setDamage([this](const Unit& target, float damage) {
        return shieldedDamage(target, damage, fog_.get(), target.slot == player_ ? fieldWork_->skill(target.id, 2) : 0);
    });
    world_->setShotPower([this](const Unit& turret, float power) {
        if (turret.slot == player_) {
            for (size_t i = 0; i < colonyObjects_.size() && i < structureIds_.size(); ++i)
                if (colonyObjects_[i] == turret.id)
                    return economy_->spend(structureIds_[i], power);
            return true;
        }
        return ai_->payShot(turret, power);
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
        deliver(start, run_->fire(start));
    }
}

// A colony building as something the other sides can shoot at (its
// side's type of that name).
void MissionSim::spawnColonyObject(size_t index) {
    if (colonyObjects_.size() <= index)
        colonyObjects_.resize(index + 1, 0);
    const ColonyBuilding& c = colony_[index];
    for (int id = 1; id < 330; ++id)
        if (const CombatType* t = combatType(id); t && t->building && t->race == side_ && c.name == t->name) {
            const auto [x, y] = worldAt(c.a, c.b);
            colonyObjects_[index] = spawnObject(*world_, player_, side_, *t, x, y, {}, c.model, true);
            return;
        }
}

int MissionSim::placeBuilding(const std::string& name, const std::string& model, int a, int b) {
    const auto [x, y] = worldAt(a, b);
    return placeUpgrade(name, model, a, b, x, y);
}

int MissionSim::placeUpgrade(const std::string& name, const std::string& model, int a, int b, float x, float y) {
    const int id = economy_->order(name, x, y, int(x / kTile), int(y / kTile));
    if (id == 0)
        return -1;
    colony_.push_back({name, model, a, b});
    structureIds_.push_back(id);
    spawnColonyObject(colony_.size() - 1);
    return int(colony_.size()) - 1;
}

bool MissionSim::raze(int unit) {
    for (size_t i = 0; i < colonyObjects_.size() && i < structureIds_.size(); ++i) {
        if (colonyObjects_[i] != unit || !economy_->raze(structureIds_[i]))
            continue;
        // Its officers go to the hub; then it stands empty and is razed.
        const int hub = colonyObjects_.empty() ? 0 : colonyObjects_[0];
        for (size_t o = 0; hub > 0 && o < fieldWork_->officers().size(); ++o)
            if (fieldWork_->officers()[o].in == unit)
                fieldWork_->moveOfficer(o, hub);
        razing_.push_back(unit);
        return true;
    }
    return false;
}

bool MissionSim::razing(int unit) const { return std::find(razing_.begin(), razing_.end(), unit) != razing_.end(); }

void MissionSim::setPosture(int slot, int posture) {
    const char race = ai_->raceOf(slot);
    for (int s = 0; s < 7; ++s)
        if (s != player_ && (s == slot || (race != '?' && ai_->raceOf(s) == race))) {
            world_->setStance(player_, s, posture);
            if (posture == 2)
                touchyNeutral_.insert(s);
            else
                touchyNeutral_.erase(s);
        }
}

void MissionSim::fireScript(const ScriptEvent& e) {
    if (run_ && !run_->finished())
        deliver(e, run_->fire(e));
}

void MissionSim::runTimers(double seconds) {
    if (run_ && !run_->finished())
        deliver({"timer", {}}, run_->advance(seconds));
}

std::vector<MissionSim::Said> MissionSim::takeSaid() {
    std::vector<Said> out;
    out.swap(said_);
    return out;
}

std::vector<std::string> MissionSim::takeVoices() {
    std::vector<std::string> out;
    out.swap(voices_);
    return out;
}

bool MissionSim::alerted(int unit) const {
    auto it = alerts_.find(unit);
    return it != alerts_.end() && world_->tick() - it->second < kAlertTicks;
}

std::vector<WorldEvent> MissionSim::takeEvents() {
    std::vector<WorldEvent> out;
    out.swap(events_);
    return out;
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
    repairs_->advance(dt);
    world_->advance(dt);
    for (fogClock_ += dt * 20; fogClock_ >= 1; fogClock_ -= 1)
        fog_->tick(*world_);
    // What combat did: a race set neutral turning hostile at its first
    // shot at the player; the colony's buildings lost.
    const auto events = world_->takeEvents();
    for (const WorldEvent& e : events) {
        if (e.kind == WorldEvent::Shot && e.toSlot == player_ && touchyNeutral_.count(e.fromSlot) &&
            world_->stance(player_, e.fromSlot) == 2)
            setPosture(e.fromSlot, 3);
        // A hit: an alert on what it struck; on one of the colony's
        // finished buildings, the "under attack" voice.
        if (e.kind == WorldEvent::Hit && e.toSlot == player_) {
            alerts_[e.to] = world_->tick();
            if (const Unit* t = world_->unit(e.to); t && t->building && t->built >= 1 &&
                                                     clock_ - lastAttackVoice_ >= kAttackVoiceEvery) {
                voices_.push_back("attack");
                lastAttackVoice_ = clock_;
            }
        }
        if (e.kind != WorldEvent::Destroyed)
            continue;
        for (size_t i = 0; i < colonyObjects_.size(); ++i)
            if (colonyObjects_[i] == e.what.id) {
                colonyObjects_[i] = -1;
                if (i < structureIds_.size())
                    economy_->remove(structureIds_[i]);
            }
        note("destroyed: slot " + std::to_string(e.what.slot) + " " + e.what.name + " at " +
             mapPosition(e.what.x, e.what.y));
    }
    events_.insert(events_.end(), events.begin(), events.end());
    auto said = ai_->observe(events);
    happened.insert(happened.end(), said.begin(), said.end());
    rollOut();
    // Buildings being razed: once empty, gone (the game's own blow,
    // through the usual destruction).
    for (auto it = razing_.begin(); it != razing_.end();) {
        int structure = 0;
        for (size_t i = 0; i < colonyObjects_.size() && i < structureIds_.size(); ++i)
            if (colonyObjects_[i] == *it)
                structure = structureIds_[i];
        if (!structure) {
            it = razing_.erase(it);
        } else if (economy_->evacuated(structure)) {
            if (Unit* u = world_->unit(*it))
                u->hp = 0;
            it = razing_.erase(it);
        } else {
            ++it;
        }
    }
    auto done = effects_->advance(dt);
    happened.insert(happened.end(), done.begin(), done.end());
    auto found = research_->advance(dt);
    happened.insert(happened.end(), found.begin(), found.end());
    auto worked = fieldWork_->advance(dt, [this](const ScriptEvent& e) { return run_ && run_->handles(e); });
    happened.insert(happened.end(), worked.begin(), worked.end());
    if (second)
        sightings(happened);
    // The economy: its vehicles (those out and the bees in the hub), then
    // construction, power, mining; the buildings' progress and efficiency
    // back on their objects.
    economy_->setVehicles(int(std::count_if(world_->units().begin(), world_->units().end(),
                                            [&](const Unit& u) { return u.slot == player_ && !u.building; })) +
                          housed_);
    // What the economy reports: its officers' lines (for the screen), the
    // reports that raise an alert (a mine exhausted, a crew beamed down),
    // the rest for the script.
    for (ScriptEvent& e : economy_->advance(dt)) {
        if (e.kind == "officer") {
            voices_.push_back(e.args.empty() ? std::string() : e.args[0]);
            continue;
        }
        auto alertOn = [&](int structure) {
            for (size_t i = 0; i < structureIds_.size() && i < colonyObjects_.size(); ++i)
                if (structureIds_[i] == structure && colonyObjects_[i] > 0)
                    alerts_[colonyObjects_[i]] = world_->tick();
        };
        if (e.kind == "beamed-down" && !e.args.empty()) {
            alertOn(std::atoi(e.args[0].c_str()));
            continue;
        }
        if (e.kind == "mine-exhausted")
            for (const ColonyStructure& c : economy_->structures())
                if (c.type == "mining-station" && c.blockY * 8 + 4 == e.a && c.blockX * 8 + 4 == e.b)
                    alertOn(c.id);
        happened.push_back(std::move(e));
    }
    for (size_t i = 0; i < colonyObjects_.size() && i < structureIds_.size(); ++i)
        if (Unit* u = colonyObjects_[i] > 0 ? world_->unit(colonyObjects_[i]) : nullptr)
            for (const ColonyStructure& c : economy_->structures())
                if (c.id == structureIds_[i])
                    u->built = c.progress, u->efficiency = economy_->efficiency(c);
    // The colony's state for the scripts' monitors: the stock once a
    // second, its building counts as they change.
    if (second) {
        const Stock st = economy_->stock();
        for (int m = 0; m < kMaterials; ++m) {
            std::string name = materialName(m);
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return char(std::tolower(c)); });
            happened.push_back({"stock", {name, std::to_string(st[size_t(m)])}});
        }
    }
    std::map<std::string, int> counts;
    for (const ColonyStructure& c : economy_->structures())
        counts[c.type] += c.progress >= 1;
    for (const auto& [type, n] : counts)
        if (auto it = lastCounts_.find(type); it == lastCounts_.end() || it->second != n)
            happened.push_back({"count", {type, std::to_string(n)}});
    for (const auto& [type, n] : lastCounts_)
        if (!counts.count(type))
            happened.push_back({"count", {type, "0"}});
    lastCounts_ = std::move(counts);
    if (second)
        watchUnits();
    // The script hears what happened, and its timers run.
    if (run_ && !run_->finished()) {
        for (const ScriptEvent& e : happened)
            deliver(e, run_->fire(e));
        deliver({"timer", {}}, run_->advance(dt));
    }
}

// The colony's buildings' officer terms (the best officer posted of its
// speciality) and health; vehicles out of the yards: bees into the hub,
// the rest beside their yard, armed ones on to the rally point.
void MissionSim::rollOut() {
    for (size_t i = 0; i < colonyObjects_.size() && i < structureIds_.size(); ++i) {
        const std::string& type = colony_[i].name;
        const int wanted = type == "construction-yard" || type == "vehicle-yard"                 ? 1
                           : type == "security-center"                                          ? 2
                           : type == "sickbay"                                                  ? 3
                           : type == "science-station"                                          ? 4
                           : type == "mining-station" || type == "resource-processor"           ? 6
                           : type == "colony-hub"                                               ? 7
                           : type == "storage-facility"                                         ? 8
                           : type == "accommodation"                                            ? 9
                                                                                                : 0;
        float best = 0;
        for (const FieldWork::OfficerAt& o : fieldWork_->officers())
            if (o.in == colonyObjects_[i] && o.in > 0 && o.officer->speciality == wanted)
                best = std::max(best, o.skill);
        economy_->setOfficer(structureIds_[i], best);
        if (const Unit* u = colonyObjects_[i] > 0 ? world_->unit(colonyObjects_[i]) : nullptr)
            economy_->setHealth(structureIds_[i], u->hp / u->maxHp);
    }
    for (const auto& [type, yard] : economy_->takeVehicles()) {
        const VehicleType* v = vehicleType(type);
        if (v && v->bee()) {
            ++housed_;
            if (const CombatType* ct = combatType(type))
                ++beesHoused_[ct->name];
            economy_->setBees(beesHoused_["construction-bee"], beesHoused_["cargo-bee"], beesHoused_["worker-bee"]);
            continue;
        }
        float x = float(blocksWide_) * kBlock / 2, y = float(blocksHigh_) * kBlock / 2;
        for (const ColonyStructure& c : economy_->structures())
            if (c.id == yard)
                x = c.x, y = c.y - 1.2f * kTile;
        const ObjectType* t = objectType(type);
        const CombatType* ct = combatType(type);
        if (!t || !ct)
            continue;
        const auto [px, py] = world_->openSpot(x, y);
        const int id = spawnObject(*world_, player_, side_, *ct, px, py, {}, t->model, true);
        world_->unit(id)->kind = UnitKind{t->name, t->model, groundScout(side_).speed * t->pace / 0.125f};
        // An armed one drives on to the rally point.
        if (ct->weapon.kind != WeaponKind::None && !colony_.empty()) {
            const auto [hx, hy] = worldAt(colony_[0].a, colony_[0].b);
            const auto [rx, ry] = rallySpot(hx, hy, rallied_++);
            const auto [qx, qy] = world_->openSpot(rx, ry);
            world_->rally(id, qx, qy);
        }
    }
}

// The scripts' sightings, once each: a site seen or scanned (the fog's
// marks on its block: the game's cell monitors), and a side's units (by
// race), or one of the player's units (by kind), near a site (how near is
// near is Reluva's).
void MissionSim::sightings(std::vector<ScriptEvent>& happened) {
    if (!script_)
        return;
    constexpr float kNear = 1.5f * kBlock;
    for (const auto& [site, at] : script_->sites) {
        const auto [sx, sy] = worldAt(at.first, at.second);
        const int bx = at.second / 8, by = at.first / 8;
        if (fog_->has(bx, by, Fog::Seen, player_) && seenSites_.insert(site).second)
            happened.push_back({"seen", {site}, at.first, at.second});
        if (fog_->has(bx, by, Fog::Scanned, player_) && scannedSites_.insert(site).second)
            happened.push_back({"cell-scanned", {site}, at.first, at.second});
        for (const Unit& u : world_->units()) {
            if (u.building || std::hypot(u.x - sx, u.y - sy) >= kNear)
                continue;
            const std::string who = u.slot == player_ ? u.name : raceName(u.side);
            if (!who.empty() && approached_.insert({who, site}).second)
                happened.push_back({"approach", {who, site}, at.first, at.second});
        }
    }
}

// What the script said: the rules' part done here (the AI's orders and
// postures, research opened, effects, the outcome); everything kept for
// the screen.
void MissionSim::deliver(const ScriptEvent& fired, const std::vector<MissionRun::Output>& out) {
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
    if (!out.empty())
        said_.push_back({fired, out});
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
