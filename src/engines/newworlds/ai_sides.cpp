// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/ai_sides.h"

#include "engines/newworlds/start_data.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace reluva::newworlds {

namespace {

constexpr float kBlock = 16384;
// Placeholders (not found in the executable): how far guards go after
// what comes near their spot, and how near a waypoint counts as reached.
constexpr float kGuardReach = 2 * kBlock;
constexpr float kArrived = 0.6f * kBlock;
// Units placed on one spot stand this far apart, and this far out from a
// building there.
constexpr float kSpacing = 2500;
constexpr float kHubClear = 12000;
// Placeholders: how far from another building a build order's spot must
// be, centre to centre (the game uses its placement test), and how long
// a building the economy doesn't know (defences, shield generators) takes to go up.
constexpr float kBuildClear = 0.3f * kBlock;
constexpr double kOtherBuildSeconds = 40;

std::pair<float, float> worldAt(int a, int b) { return {float(b) / 8 * kBlock, float(a) / 8 * kBlock}; }

} // namespace

bool isBee(const CombatType& t) { return std::string_view(t.name).find("bee") != std::string_view::npos; }

std::pair<float, float> rallySpot(float hubX, float hubY, int n) {
    const float cx = std::floor(hubX / kBlock) * kBlock, cy = std::floor(hubY / kBlock) * kBlock;
    n %= 16;
    return {cx + 2560 + 4096 * float(n % 4), cy + 2560 + 4096 * float(n / 4)};
}

std::pair<float, float> parkingSpot(int n, float x, float y, bool building) {
    const float clear = building ? kHubClear : 0;
    if (n == 0 && clear == 0)
        return {x, y};
    const float ring = float(n / 8), angle = float(n % 8) * 0.785f + ring * 0.4f;
    const float r = clear + (ring + (clear > 0 ? 0 : 1)) * kSpacing;
    return {x + std::cos(angle) * r, y + std::sin(angle) * r};
}

int spawnObject(World& world, int slot, char race, const CombatType& type, float x, float y, const std::string& label,
                const std::string& model, bool player) {
    UnitKind kind{type.name, model.empty() ? type.model : model,
                  type.building ? 0 : groundScout('F').speed * type.pace / 0.125f};
    const int id = world.addUnit(race, kind, x, y, 3.1416f / 2);
    Unit& u = *world.unit(id);
    u.slot = slot;
    u.type = type.id;
    u.name = type.name;
    u.label = label;
    u.building = type.building;
    const float tough = player && race == 'F' ? 1.2f : player && race == 'R' ? 1.3f : 1.0f;
    u.hp = u.maxHp = float(std::max(1, type.hp)) * tough;
    u.weapon = type.weapon;
    u.ai = !player;
    u.flies = std::string_view(type.name) == "bird-of-prey";
    u.cloaks = type.cloaks;
    return id;
}

std::vector<std::string> scriptNames(const Unit& u, int playerSlot) {
    std::vector<std::string> out;
    const bool colony = u.side == 'F' || u.side == 'K' || u.side == 'R' || u.side == 'V';
    if (u.slot == playerSlot || !colony) {
        out.push_back(u.name);
    } else {
        const std::string race = raceName(u.side);
        out.push_back(race + "-" + u.name);
        if (u.name == "colony-hub")
            out.push_back(race + "-hub");
    }
    if (!u.label.empty() && u.label != out[0])
        out.push_back(u.label);
    return out;
}

AiSides::AiSides(std::string_view mission, int playerSlot, World& world)
    : mission_(mission), player_(playerSlot), world_(world) {
    for (const SideInfo* s : slotsOf(mission))
        races_[s->slot] = s->race;
}

char AiSides::raceOf(int slot) const {
    auto it = races_.find(slot);
    return it == races_.end() ? '?' : it->second;
}

std::vector<int> AiSides::slotsNamed(const std::string& side) const {
    std::vector<int> out;
    for (const auto& [slot, race] : races_)
        if (side == raceName(race))
            out.push_back(slot);
    if (out.empty())
        for (const auto& [slot, race] : races_)
            for (const SidePlacement* p : sidePlacements(mission_, slot))
                if (const CombatType* t = combatType(p->type); t && side == raceName(t->race)) {
                    out.push_back(slot);
                    break;
                }
    return out;
}

std::pair<float, float> AiSides::openSpot(float x, float y) const { return world_.openSpot(x, y); }

void AiSides::start() {
    for (const SideInfo* s : slotsOf(mission_))
        for (int to = 0; to < 7; ++to)
            world_.setStance(s->slot, to, s->posture[size_t(to)]);
    // The other colonies' economies, with the mission's stock (else the
    // game's default) and their officers.
    for (const auto& [slot, race] : races_)
        if (slot != player_ && slot <= 2 && (race == 'F' || race == 'K' || race == 'R')) {
            Stock stock{200, 1000, 1000, 1000, 5000, 5000};
            startStock(mission_, race, stock);
            Colony& c = colonies_[slot];
            c.economy = std::make_unique<Economy>(race, stock, 50, ground_);
            c.economy->setFree(true);
            c.economy->setOfficers(int(officers(mission_, slot).size()));
        }
    for (const auto& [slot, race] : races_) {
        if (slot == player_)
            continue;
        std::map<std::pair<int, int>, int> onSpot;
        std::set<std::pair<int, int>> built; // spots a building stands on
        for (const SidePlacement* p : sidePlacements(mission_, slot))
            if (const CombatType* t = combatType(p->type); t && t->building)
                built.insert({p->a, p->b});
        auto colony = colonies_.find(slot);
        for (const SidePlacement* p : sidePlacements(mission_, slot)) {
            const CombatType* t = combatType(p->type);
            // Without a model only what the scripts name, and shield
            // generators (their fields count), are placed (unseen).
            if (!t || p->forWhom == 1 || isBee(*t) ||
                (!*t->model && !*p->model && !*p->label && std::string_view(t->name) != "shield-generator"))
                continue;
            auto [x, y] = worldAt(p->a, p->b);
            if (!t->building) {
                // Several on a spot stand round it, ring by ring (round
                // the building, where one stands there: the game parks
                // them by the hub).
                std::tie(x, y) = parkingSpot(onSpot[{p->a, p->b}]++, x, y, built.count({p->a, p->b}) > 0);
                std::tie(x, y) = openSpot(x, y);
            }
            const int id = spawnObject(world_, slot, race, *t, x, y, p->label, p->model);
            if (colony != colonies_.end() && t->building && buildingType(t->name))
                colony->second.structureOf[id] = colony->second.economy->addBuilt(t->name, x, y);
        }
        // Each building's best officer at its speciality.
        if (colony != colonies_.end()) {
            std::map<int, float> best;
            for (const Officer* o : officers(mission_, slot))
                for (const auto& [id, structure] : colony->second.structureOf) {
                    const Unit* u = world_.unit(id);
                    const BuildingType* b = u ? buildingType(u->name) : nullptr;
                    if (u && b && u->type == o->building && b->speciality == o->speciality)
                        best[structure] = std::max(best[structure], o->skill);
                }
            for (const auto& [structure, skill] : best)
                colony->second.economy->setOfficer(structure, skill);
        }
    }
    // Unnamed orders, and named builds (which start armed).
    for (const NpcOrder* o : npcOrders(mission_))
        if ((!*o->name || o->task == 9) && (o->slot != player_ || o->human))
            queue(*o);
}

const Economy* AiSides::colony(int slot) const {
    auto it = colonies_.find(slot);
    return it == colonies_.end() ? nullptr : it->second.economy.get();
}

std::pair<float, float> AiSides::wayPoint(const NpcOrder& o, int leg) const {
    if (o.ways == 0)
        return {-1, -1};
    const int i = std::clamp(leg, 0, o.ways - 1);
    return worldAt(o.way[i][0], o.way[i][1]);
}

void AiSides::queue(const NpcOrder& o) {
    running_.push_back(Running{&o, {}});
    if (clock_ >= o.delay)
        fire(running_.back());
}

// An order's time has come: a build tries its spot; a group gathers its
// units (beamed in, the side's own, or asked of its yards).
void AiSides::fire(Running& r) {
    r.started = true;
    if (r.order->task == 9 || r.order->task == 3) {
        build(r);
        return;
    }
    const NpcOrder& o = *r.order;
    const CombatType* t = combatType(o.type);
    if (!t || t->building) {
        r.done = true;
        return;
    }
    if (o.spawn[0] >= 0) {
        const auto [x, y] = worldAt(o.spawn[0], o.spawn[1]);
        const char race = raceOf(o.slot);
        for (int n = 0; n < o.count; ++n) {
            const float angle = float(n) * 1.1f;
            const auto [sx, sy] = openSpot(x + std::cos(angle) * kSpacing, y + std::sin(angle) * kSpacing);
            r.units.push_back(spawnObject(world_, o.slot, race == '?' ? t->race : race, *t, sx, sy, {}));
        }
    }
    gather(r);
}

// A group short of units: the side's idle ones of the type, nearest its
// first waypoint first; the rest asked of its vehicle yards (an AI
// colony's, free; never the player's). Once it has them all it sets off.
void AiSides::gather(Running& r) {
    const NpcOrder& o = *r.order;
    r.units.erase(std::remove_if(r.units.begin(), r.units.end(), [&](int id) { return !world_.unit(id); }),
                  r.units.end());
    if (r.gathered)
        return;
    std::set<int> busy;
    for (const Running& other : running_)
        if (!other.done)
            busy.insert(other.units.begin(), other.units.end());
    const auto [wx, wy] = wayPoint(o, 0);
    std::vector<std::pair<float, int>> idle;
    for (const Unit& u : world_.units())
        if (u.slot == o.slot && u.type == o.type && !u.building && !busy.count(u.id))
            idle.push_back({wx < 0 ? 0 : std::hypot(u.x - wx, u.y - wy), u.id});
    std::sort(idle.begin(), idle.end());
    for (size_t i = 0; i < idle.size() && int(r.units.size()) + r.awaited < o.count; ++i)
        r.units.push_back(idle[i].second);
    auto colony = colonies_.find(o.slot);
    const VehicleType* v = vehicleType(o.type);
    while (colony != colonies_.end() && v && int(r.units.size()) + r.awaited < o.count &&
           colony->second.economy->orderVehicle(*v))
        ++r.awaited;
    if (int(r.units.size()) >= o.count) {
        r.gathered = true;
        steer(r);
    }
}

// A build order on its turn: once its spot is clear, the building goes up
// there as a site (an AI colony's in its economy). A named one keeps
// building it again whenever it's gone; the other builds once.
void AiSides::build(Running& r) {
    const NpcOrder& o = *r.order;
    const CombatType* t = combatType(o.type);
    if (!t || !t->building || o.ways == 0) {
        r.done = true;
        return;
    }
    if (r.site && world_.unit(r.site))
        return;
    const auto [x, y] = wayPoint(o, 0);
    for (const Unit& u : world_.units())
        if (u.building && u.hp > 0 && std::hypot(u.x - x, u.y - y) < kBuildClear) {
            // Its own building already there (placed at the start): kept.
            if (u.slot == o.slot && u.type == o.type) {
                r.site = u.id;
                if (o.task == 3)
                    r.done = true;
            }
            return;
        }
    int structure = 0;
    auto colony = colonies_.find(o.slot);
    const BuildingType* b = buildingType(t->name);
    if (colony != colonies_.end() && b) {
        structure = colony->second.economy->order(t->name, x, y, int(x / kTile), int(y / kTile));
        if (!structure)
            return; // (a mining station off its deposit) tried again on its next turn
    }
    const char race = raceOf(o.slot);
    r.site = spawnObject(world_, o.slot, race == '?' ? t->race : race, *t, x, y, {});
    world_.unit(r.site)->built = 0;
    if (structure)
        colony->second.structureOf[r.site] = structure;
    else
        nativeSites_[r.site] = 0;
    if (o.task == 3)
        r.done = true;
}

// The AI colonies' economies moved on: sites going up, their buildings'
// health, the vehicles their yards finish (each to the order that asked
// for one of its type, oldest first).
void AiSides::colonies(double seconds) {
    for (auto& [id, put] : nativeSites_) {
        put += seconds;
        if (Unit* u = world_.unit(id))
            u->built = float(std::min(1.0, put / kOtherBuildSeconds));
    }
    for (auto& [slot, c] : colonies_) {
        for (const auto& [id, structure] : c.structureOf)
            if (const Unit* u = world_.unit(id))
                c.economy->setHealth(structure, u->hp / u->maxHp);
        for (double left = seconds; left > 1e-9; left -= 0.25)
            c.economy->advance(std::min(left, 0.25));
        for (const ColonyStructure& s : c.economy->structures())
            for (const auto& [id, structure] : c.structureOf)
                if (structure == s.id)
                    if (Unit* u = world_.unit(id)) {
                        u->built = s.progress;
                        u->efficiency = c.economy->efficiency(s);
                    }
        for (const auto& [type, yard] : c.economy->takeVehicles()) {
            const CombatType* t = combatType(type);
            if (!t)
                continue;
            float x = 0, y = 0;
            for (const ColonyStructure& s : c.economy->structures())
                if (s.id == yard)
                    x = s.x, y = s.y - 1.2f * kTile;
            const auto [px, py] = openSpot(x, y);
            const int id = spawnObject(world_, slot, raceOf(slot), *t, px, py, {});
            bool asked = false;
            for (Running& r : running_)
                if (!r.done && r.awaited > 0 && r.order->slot == slot && r.order->type == type) {
                    --r.awaited;
                    r.units.push_back(id);
                    asked = true;
                    break;
                }
            // One no order asked for (a group gone meanwhile) parks at the
            // side's rally point, armed.
            if (!asked && t->weapon.kind != WeaponKind::None)
                for (const ColonyStructure& s : c.economy->structures())
                    if (s.type == "colony-hub") {
                        const auto [rx, ry] = rallySpot(s.x, s.y, rallied_[slot]++);
                        world_.rally(id, rx, ry);
                        break;
                    }
        }
    }
}

bool AiSides::order(const std::string& side, const std::string& text) {
    std::string name = text;
    const bool abort = name.rfind("abort ", 0) == 0;
    if (abort)
        name = name.substr(6);
    const std::vector<int> slots = slotsNamed(side);
    const NpcOrder* found = nullptr;
    for (const NpcOrder* o : npcOrders(mission_))
        if (name == o->name && (!found || std::count(slots.begin(), slots.end(), o->slot)))
            found = o;
    if (!found)
        return false;
    if (abort) {
        for (Running& r : running_)
            if (r.order == found)
                r.done = true;
        return true;
    }
    // A named build already armed stays as it is.
    if (found->task == 9 && std::any_of(running_.begin(), running_.end(),
                                        [&](const Running& r) { return r.order == found && !r.done; }))
        return true;
    queue(*found);
    return true;
}

void AiSides::stance(const std::string& side, const std::string& verb) {
    const int p = verb == "hostile" ? 3 : verb == "allied" ? 1 : 2;
    for (int slot : slotsNamed(side))
        world_.setStance(slot, player_, p);
}

// An order's units each second: each to its leg's waypoint, unless it's
// fighting; when they're all there, the next leg (a patrol round again).
// Assaults past their last leg go for the nearest enemy; guards for what
// comes near.
void AiSides::steer(Running& r) {
    const NpcOrder& o = *r.order;
    r.units.erase(std::remove_if(r.units.begin(), r.units.end(), [&](int id) { return !world_.unit(id); }),
                  r.units.end());
    // A group wiped out is over.
    if (r.units.empty())
        r.done = true;
    if (r.units.empty() || o.ways == 0)
        return;
    const bool guard = o.task == 4, patrol = o.task == 1 || o.task == 2;
    auto [wx, wy] = wayPoint(o, r.leg);
    float cx = 0, cy = 0;
    bool allThere = true;
    for (int id : r.units) {
        const Unit& u = *world_.unit(id);
        cx += u.x / float(r.units.size()), cy += u.y / float(r.units.size());
        // There, or stopped short (no way on): only those still driving
        // or fighting hold the group up.
        if (std::hypot(u.x - wx, u.y - wy) > kArrived * (1 + float(r.units.size()) / 4) && (u.moving() || u.firing))
            allThere = false;
    }
    // (A leg's first seconds don't count: its units are only setting off.)
    if (!guard && allThere && !r.hunting && r.legClock >= 2) {
        r.legClock = 0;
        if (r.leg + 1 < o.ways) {
            ++r.leg;
        } else if (o.slot == player_) {
            r.done = true; // the player's units, placed, are theirs again
            return;
        } else if (patrol)
            r.leg = 0;
        else
            r.hunting = true;
        std::tie(wx, wy) = wayPoint(o, r.leg);
    }
    // What to go for: an assault's nearest enemy at the end of its route;
    // a guard's, near its spot.
    const Unit* prey = nullptr;
    if (r.hunting || guard) {
        float best = r.hunting ? 1e30f : kGuardReach;
        for (const Unit& v : world_.units())
            if (world_.prey(o.slot, v.slot) && v.hp > 0 && !v.cloaked && v.built >= 1) {
                const float d = r.hunting ? std::hypot(v.x - cx, v.y - cy) : std::hypot(v.x - wx, v.y - wy);
                if (d < best)
                    best = d, prey = &v;
            }
    }
    int n = 0;
    for (int id : r.units) {
        Unit& u = *world_.unit(id);
        ++n;
        // A guard drawn too far from its spot goes back.
        if (guard && std::hypot(u.x - wx, u.y - wy) > 2 * kGuardReach) {
            world_.moveTo(id, wx, wy);
            continue;
        }
        if (u.firing || (u.ordered && u.target))
            continue;
        if (prey) {
            world_.attack({id}, prey->id);
            continue;
        }
        if (u.moving())
            continue;
        const float angle = float(n) * 2.4f, off = float(n > 1) * kSpacing;
        const float tx = wx + std::cos(angle) * off, ty = wy + std::sin(angle) * off;
        if (std::hypot(u.x - tx, u.y - ty) > kSpacing)
            world_.moveTo(id, tx, ty);
    }
}

void AiSides::advance(double seconds) {
    clock_ += seconds;
    colonies(seconds);
    for (Running& r : running_)
        if (r.started && !r.done)
            r.clock += seconds, r.legClock += seconds;
    // Each second: orders whose time has come fire; builds try their spot
    // again; groups still short gather; the rest are steered.
    if (std::floor(clock_) != std::floor(clock_ - seconds))
        for (size_t i = 0; i < running_.size(); ++i) {
            Running& r = running_[i];
            if (r.done)
                continue;
            if (!r.started) {
                if (clock_ >= r.order->delay)
                    fire(r);
            } else if (r.order->task == 9 || r.order->task == 3) {
                build(r);
            } else if (!r.gathered) {
                gather(r);
            } else {
                steer(r);
            }
        }
    running_.erase(std::remove_if(running_.begin(), running_.end(), [](const Running& r) { return r.done; }),
                   running_.end());
}

std::vector<ScriptEvent> AiSides::observe(const std::vector<WorldEvent>& events) {
    std::vector<ScriptEvent> out;
    for (const WorldEvent& e : events) {
        if (e.kind != WorldEvent::Destroyed)
            continue;
        nativeSites_.erase(e.what.id);
        if (auto c = colonies_.find(e.what.slot); c != colonies_.end())
            if (auto it = c->second.structureOf.find(e.what.id); it != c->second.structureOf.end()) {
                c->second.economy->remove(it->second);
                c->second.structureOf.erase(it);
            }
        const int a = int(e.what.y / kBlock * 8), b = int(e.what.x / kBlock * 8);
        for (const std::string& n : scriptNames(e.what, player_))
            out.push_back({"destroyed", {n}, a, b});
        if (e.what.name == "colony-hub" && e.what.slot != player_)
            out.push_back({"hub-destroyed", {raceName(e.what.side)}, a, b});
    }
    std::map<std::string, int> counts;
    for (const Unit& u : world_.units())
        if (!(u.slot == player_ && u.building) && u.built >= 1) // (sites aren't counted)
            for (const std::string& n : scriptNames(u, player_))
                ++counts[n];
    for (const auto& [n, c] : counts)
        if (auto it = counts_.find(n); it == counts_.end() || it->second != c)
            out.push_back({"count", {n, std::to_string(c)}});
    for (const auto& [n, c] : counts_)
        if (!counts.count(n))
            out.push_back({"count", {n, "0"}});
    counts_ = std::move(counts);
    return out;
}

std::vector<AiSides::OrderStatus> AiSides::status() const {
    std::vector<OrderStatus> out;
    for (const Running& r : running_) {
        if (r.done)
            continue;
        int blocker = 0;
        if ((r.order->task == 3 || r.order->task == 9) && r.order->ways > 0 && !r.site) {
            const auto [x, y] = wayPoint(*r.order, 0);
            for (const Unit& u : world_.units())
                if (u.building && u.hp > 0 && std::hypot(u.x - x, u.y - y) < kBuildClear)
                    blocker = u.id;
        }
        out.push_back({r.order, r.started, r.gathered, r.hunting, int(r.units.size()), r.awaited, r.leg,
                       r.gathered ? r.legClock : r.clock, r.site, blocker, r.units});
    }
    return out;
}

std::vector<std::string> AiSides::running() const {
    std::vector<std::string> out;
    for (const Running& r : running_)
        if (!r.done)
            out.push_back(r.order->name);
    return out;
}

} // namespace reluva::newworlds
