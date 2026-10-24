// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/ai_sides.h"

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

std::pair<float, float> worldAt(int a, int b) { return {float(b) / 8 * kBlock, float(a) / 8 * kBlock}; }

bool isBee(const CombatType& t) { return std::string_view(t.name).find("bee") != std::string_view::npos; }

} // namespace

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
    for (const auto& [slot, race] : races_) {
        if (slot == player_)
            continue;
        std::map<std::pair<int, int>, int> onSpot;
        std::set<std::pair<int, int>> built; // spots a building stands on
        for (const SidePlacement* p : sidePlacements(mission_, slot))
            if (const CombatType* t = combatType(p->type); t && t->building)
                built.insert({p->a, p->b});
        for (const SidePlacement* p : sidePlacements(mission_, slot)) {
            const CombatType* t = combatType(p->type);
            if (!t || p->forWhom == 1 || isBee(*t) || (!*t->model && !*p->model))
                continue;
            auto [x, y] = worldAt(p->a, p->b);
            if (!t->building) {
                // Several on a spot stand round it, ring by ring (round
                // the building, where one stands there: the game parks
                // them by the hub).
                const int n = onSpot[{p->a, p->b}]++;
                const float clear = built.count({p->a, p->b}) ? kHubClear : 0;
                if (n > 0 || clear > 0) {
                    const float ring = float(n / 8), angle = float(n % 8) * 0.785f + ring * 0.4f;
                    x += std::cos(angle) * (clear + (ring + (clear > 0 ? 0 : 1)) * kSpacing);
                    y += std::sin(angle) * (clear + (ring + (clear > 0 ? 0 : 1)) * kSpacing);
                }
                std::tie(x, y) = openSpot(x, y);
            }
            spawnObject(world_, slot, race, *t, x, y, p->label, p->model);
        }
    }
    for (const NpcOrder* o : npcOrders(mission_))
        if (!*o->name && o->slot != player_)
            begin(*o);
}

bool AiSides::home(int slot, float& x, float& y) const {
    for (const Unit& u : world_.units())
        if (u.slot == slot && u.building) {
            x = u.x, y = u.y;
            return true;
        }
    return false;
}

std::pair<float, float> AiSides::wayPoint(const NpcOrder& o, int leg) const {
    if (o.ways == 0)
        return {-1, -1};
    const int i = std::clamp(leg, 0, o.ways - 1);
    return worldAt(o.way[i][0], o.way[i][1]);
}

void AiSides::begin(const NpcOrder& o) {
    Running r{&o, {}};
    const char race = raceOf(o.slot);
    const CombatType* t = combatType(o.type);
    if (o.task == 9 || o.task == 3) {
        running_.push_back(std::move(r)); // built after its delay
        return;
    }
    if (!t || t->building)
        return;
    std::set<int> busy;
    for (const Running& other : running_)
        if (!other.done)
            busy.insert(other.units.begin(), other.units.end());
    const auto [wx, wy] = wayPoint(o, 0);
    if (o.spawn[0] < 0) {
        // The side's own, nearest the first waypoint first.
        std::vector<std::pair<float, int>> idle;
        for (const Unit& u : world_.units())
            if (u.slot == o.slot && u.type == o.type && !busy.count(u.id))
                idle.push_back({wx < 0 ? 0 : std::hypot(u.x - wx, u.y - wy), u.id});
        std::sort(idle.begin(), idle.end());
        for (size_t i = 0; i < idle.size() && int(r.units.size()) < o.count; ++i)
            r.units.push_back(idle[i].second);
    }
    if (int(r.units.size()) < o.count && (*t->model)) {
        // The rest beamed in, or out of the side's first building.
        float x = wx, y = wy;
        if (o.spawn[0] >= 0)
            std::tie(x, y) = worldAt(o.spawn[0], o.spawn[1]);
        else
            home(o.slot, x, y);
        if (x < 0)
            return;
        for (int n = int(r.units.size()); n < o.count; ++n) {
            const float angle = float(n) * 1.1f;
            const auto [sx, sy] = openSpot(x + std::cos(angle) * kSpacing, y + std::sin(angle) * kSpacing);
            r.units.push_back(spawnObject(world_, o.slot, race == '?' ? t->race : race, *t, sx, sy, {}));
        }
    }
    running_.push_back(std::move(r));
    steer(running_.back());
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
    begin(*found);
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
        if (r.leg + 1 < o.ways)
            ++r.leg;
        else if (patrol)
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
            if (world_.prey(o.slot, v.slot) && v.hp > 0) {
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
    for (Running& r : running_) {
        if (r.done)
            continue;
        r.clock += seconds;
        r.legClock += seconds;
        if (r.order->task == 9 || r.order->task == 3) {
            // A build order: the building stands once its delay is up (how
            // the AI's bees put it up isn't modelled).
            if (r.clock >= r.order->delay && r.order->ways > 0) {
                if (const CombatType* t = combatType(r.order->type); t && *t->model) {
                    const auto [x, y] = wayPoint(*r.order, 0);
                    spawnObject(world_, r.order->slot, raceOf(r.order->slot), *t, x, y, {});
                }
                r.done = true;
            }
            continue;
        }
    }
    if (std::floor(clock_) != std::floor(clock_ - seconds))
        for (Running& r : running_)
            if (!r.done && r.order->task != 9 && r.order->task != 3)
                steer(r);
    running_.erase(std::remove_if(running_.begin(), running_.end(),
                                  [](const Running& r) { return r.done && r.order->task != 9 && r.order->task != 3; }),
                   running_.end());
}

std::vector<ScriptEvent> AiSides::observe(const std::vector<WorldEvent>& events) {
    std::vector<ScriptEvent> out;
    for (const WorldEvent& e : events) {
        if (e.kind != WorldEvent::Destroyed)
            continue;
        const int a = int(e.what.y / kBlock * 8), b = int(e.what.x / kBlock * 8);
        for (const std::string& n : scriptNames(e.what, player_))
            out.push_back({"destroyed", {n}, a, b});
        if (e.what.name == "colony-hub" && e.what.slot != player_)
            out.push_back({"hub-destroyed", {raceName(e.what.side)}, a, b});
    }
    std::map<std::string, int> counts;
    for (const Unit& u : world_.units())
        if (!(u.slot == player_ && u.building))
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

std::vector<std::string> AiSides::running() const {
    std::vector<std::string> out;
    for (const Running& r : running_)
        if (!r.done)
            out.push_back(r.order->name);
    return out;
}

} // namespace reluva::newworlds
