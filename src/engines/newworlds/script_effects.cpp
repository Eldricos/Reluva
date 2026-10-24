// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/script_effects.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace reluva::newworlds {

namespace {

constexpr float kBlock = 16384;
// The script objects' tick: 16 a second (exe.md#script-effects), and
// their figures: the top-up every 63 ticks; a meteor's 30 points of damage
// to the one thing it hits; an area storm's 2 % chance a tick of a flash;
// the disease's rolls (in a hundred): 10 to catch it, 60 to recover.
constexpr double kTick = 1.0 / 16;
constexpr int kTopUpTicks = 63;
constexpr float kMeteorDamage = 30;
constexpr float kFlashChance = 0.02f;
constexpr float kCatch = 0.10f, kRecover = 0.60f;
constexpr double kSparkLife = 1.0;
// A thing's size, for a meteor hitting it (a placeholder for the model's
// bounding sphere).
constexpr float kBuildingSize = 6000, kVehicleSize = 1000;

std::vector<std::string> words(const std::string& text) {
    std::istringstream in(text);
    std::vector<std::string> out;
    for (std::string w; in >> w;)
        out.push_back(w);
    return out;
}

std::pair<float, float> worldAt(int a, int b) { return {float(b) / 8 * kBlock, float(a) / 8 * kBlock}; }

} // namespace

ScriptEffects::ScriptEffects(int playerSlot, World& world, AiSides* ai, Economy* economy, const MissionScript* script)
    : player_(playerSlot), world_(world), ai_(ai), economy_(economy), script_(script) {}

std::array<float, 4> ScriptEffects::zoneRect(const std::string& zone) const {
    if (!script_)
        return {0, 0, 0, 0};
    auto it = script_->zones.find(zone);
    if (it == script_->zones.end())
        return {0, 0, 0, 0};
    const auto [x0, y0] = worldAt(it->second[0], it->second[1]);
    const auto [x1, y1] = worldAt(it->second[2], it->second[3]);
    return {x0, y0, x1, y1};
}

int ScriptEffects::slotNamed(const std::string& side) const {
    if (side == "us" || (ai_ && raceName(ai_->raceOf(player_)) == side))
        return player_;
    if (ai_)
        for (int slot : ai_->slotsNamed(side))
            return slot;
    return -1;
}

void ScriptEffects::apply(const std::string& verb, const std::string& args) {
    const std::vector<std::string> a = words(args);
    if ((verb == "give" || verb == "take") && a.size() == 2 && economy_) {
        const int amount = std::atoi(a[1].c_str());
        economy_->give(materialNamed(a[0]), verb == "give" ? amount : -amount);
    } else if (verb == "top-up") {
        toppingUp_ = !(a.size() == 1 && a[0] == "off");
        topUp_ = {};
        for (size_t i = 0; toppingUp_ && i + 1 < a.size(); i += 2)
            if (const int m = materialNamed(a[i]); m >= 0)
                topUp_[size_t(m)] = std::atoi(a[i + 1].c_str());
    } else if (verb == "damage" && (a.size() == 2 || a.size() == 3)) {
        // A share of each unit's full strength (all: destroyed), of the
        // side named if there is one. A group is its label, or a type
        // name.
        const int only = a.size() == 3 ? slotNamed(a[2]) : -2;
        for (const Unit& u : world_.units())
            if ((u.label == a[0] || u.name == a[0]) && (only == -2 || u.slot == only))
                if (Unit* v = world_.unit(u.id))
                    v->hp = a[1] == "all" ? 0 : v->hp - v->maxHp * float(std::atoi(a[1].c_str())) / 100;
    } else if (verb == "beam-up" && a.size() == 1) {
        std::vector<int> gone;
        for (const Unit& u : world_.units())
            if (u.label == a[0]) {
                gone.push_back(u.id);
                sparks_.push_back({u.x, u.y, 2});
            }
        for (int id : gone)
            world_.remove(id);
        beaming_.push_back(a[0]);
    } else if (verb == "spawn" && a.size() >= 5) {
        const int slot = slotNamed(a[0]);
        if (slot < 0)
            return;
        const char race = ai_ ? ai_->raceOf(slot) : 'F';
        const CombatType* type = nullptr;
        for (int id = 1; id < 330 && !type; ++id)
            if (const CombatType* t = combatType(id); t && a[1] == t->name && (t->race == race || !type))
                type = t;
        if (!type)
            return;
        const auto [x, y] = worldAt(std::atoi(a[3].c_str()), std::atoi(a[4].c_str()));
        const int count = std::atoi(a[2].c_str());
        for (int n = 0; n < count; ++n) {
            const float angle = float(n) * 1.3f, r = n ? 2500.0f : 0.0f;
            const auto [sx, sy] = type->building ? std::pair{x, y} : world_.openSpot(x + std::cos(angle) * r, y + std::sin(angle) * r);
            spawnObject(world_, slot, race == '?' ? type->race : race, *type, sx, sy, a.size() > 5 ? a[5] : "", {},
                        slot == player_);
            sparks_.push_back({sx, sy, 2});
        }
    } else if (verb == "move" && a.size() >= 3) {
        Route r;
        r.loop = a.back() == "loop";
        for (size_t i = 1; i + 1 < a.size(); i += 2)
            r.points.push_back(worldAt(std::atoi(a[i].c_str()), std::atoi(a[i + 1].c_str())));
        for (const Unit& u : world_.units())
            if (u.label == a[0] && !u.building) {
                routes_[u.id] = r;
                world_.moveTo(u.id, r.points[0].first, r.points[0].second);
            }
    } else if ((verb == "meteors" || verb == "storm") && !a.empty()) {
        storms_.erase(std::remove_if(storms_.begin(), storms_.end(),
                                     [&](const Storm& s) { return s.zone == a[0] && s.meteors == (verb == "meteors"); }),
                      storms_.end());
        if (verb == "meteors" && a.size() > 1 && a[1] == "off")
            return;
        Storm s;
        s.zone = a[0];
        s.meteors = verb == "meteors";
        s.every = s.meteors ? std::max(kTick, std::atof(a[1].c_str())) : 0;
        storms_.push_back(s);
    } else if (verb == "infect") {
        infecting_ = 1;
    } else if (verb == "cure") {
        infecting_ = -1;
    }
}

std::vector<ScriptEvent> ScriptEffects::advance(double seconds) {
    std::vector<ScriptEvent> out;
    for (const std::string& g : beaming_)
        out.push_back({"beamed-up", {g}});
    beaming_.clear();
    for (Spark& s : sparks_)
        s.age += seconds;
    sparks_.erase(std::remove_if(sparks_.begin(), sparks_.end(), [](const Spark& s) { return s.age > kSparkLife; }),
                  sparks_.end());
    clock_ += seconds;
    while (clock_ >= kTick) {
        clock_ -= kTick;
        tick(out);
    }
    return out;
}

void ScriptEffects::tick(std::vector<ScriptEvent>& out) {
    auto random01 = [&] {
        random_ = random_ * 1664525u + 1013904223u;
        return float(random_ >> 8) / float(1u << 24);
    };
    // Top-up.
    if (toppingUp_ && economy_ && (topUpClock_ += 1) >= kTopUpTicks) {
        topUpClock_ = 0;
        const Stock st = economy_->stock();
        for (int m = 0; m < kMaterials; ++m)
            if (st[size_t(m)] < topUp_[size_t(m)])
                economy_->give(m, topUp_[size_t(m)] - st[size_t(m)]);
    }
    // Storms: a meteor (damage round where it falls) or a flash at a
    // random point of the zone.
    for (Storm& s : storms_) {
        if (s.meteors ? (s.clock += kTick) < s.every - 1e-9 : random01() >= kFlashChance)
            continue;
        s.clock = 0;
        const auto r = zoneRect(s.zone);
        const float x = r[0] + (r[2] - r[0]) * random01(), y = r[1] + (r[3] - r[1]) * random01();
        sparks_.push_back({x, y, s.meteors ? 0 : 1});
        if (!s.meteors)
            continue;
        // The one thing it falls on.
        const Unit* hit = nullptr;
        float best = 1e30f;
        for (const Unit& u : world_.units()) {
            const float d = std::hypot(u.x - x, u.y - y);
            if (d < (u.building ? kBuildingSize : kVehicleSize) && d < best)
                best = d, hit = &u;
        }
        if (hit)
            world_.unit(hit->id)->hp -= kMeteorDamage;
    }
    // Routes: on to the next point when one is reached.
    for (auto it = routes_.begin(); it != routes_.end();) {
        Unit* u = world_.unit(it->first);
        if (!u) {
            it = routes_.erase(it);
            continue;
        }
        Route& r = it->second;
        if (!u->moving() && !u->firing) {
            const auto [px, py] = r.points[r.next];
            if (std::hypot(u->x - px, u->y - py) < kBlock) {
                if (++r.next >= r.points.size()) {
                    if (!r.loop) {
                        it = routes_.erase(it);
                        continue;
                    }
                    r.next = 0;
                }
            }
            world_.moveTo(u->id, r.points[r.next].first, r.points[r.next].second);
        }
        ++it;
    }
    // The disease: each tick one of the crew, in turn, rolls: one who's
    // well may catch it while it spreads; one who's ill may recover once
    // the cure has begun. The percentage is of the crew when it began.
    if (infecting_ != 0) {
        if (crewAtStart_ == 0)
            crewAtStart_ = std::max(1, economy_ ? economy_->crew() : 1);
        const int before = infected_ * 100 / crewAtStart_;
        const bool ill = random01() * float(crewAtStart_) < float(infected_);
        const float roll = random01();
        if (infecting_ > 0 && !ill && roll < kCatch)
            infected_ = std::min(crewAtStart_, infected_ + 1);
        else if (infecting_ < 0 && ill && roll < kRecover)
            --infected_;
        infection_ = double(infected_ * 100 / crewAtStart_);
        if (int(infection_) != before)
            out.push_back({"infection", {std::to_string(int(infection_))}});
        if (infecting_ < 0 && infected_ <= 0) {
            infecting_ = 0;
            out.push_back({"cured", {}});
        }
    }
    // Once a second: zones, and what has stood how long.
    if ((second_ += kTick) >= 1) {
        second_ -= 1;
        zones(out);
        std::set<std::string> now;
        if (economy_)
            for (const ColonyStructure& c : economy_->structures())
                if (c.progress >= 1)
                    now.insert(c.type);
        for (const Unit& u : world_.units())
            if (u.slot == player_ && u.building && !buildingType(u.name)) {
                now.insert(u.name);
                if (!u.label.empty())
                    now.insert(u.label);
            }
        for (auto it = standing_.begin(); it != standing_.end();)
            it = now.count(it->first) ? std::next(it) : standing_.erase(it);
        for (const std::string& w : now) {
            const int s = int(standing_[w] += 1);
            out.push_back({"standing", {w, std::to_string(s)}});
        }
    }
}

// The area monitors: who is in each zone (a side's units by race, ours by
// kind and as "ours", a group when all of it is there, and "any"), and
// the changes: inside when they come, outside when they've all gone (and,
// the first time, for those present elsewhere).
void ScriptEffects::zones(std::vector<ScriptEvent>& out) {
    if (!script_ || script_->zones.empty())
        return;
    std::map<std::string, std::pair<int, int>> groups; // label: inside, all
    std::set<std::string> everywhere;
    for (const Unit& u : world_.units()) {
        if (u.building)
            continue;
        if (!u.label.empty())
            ++groups[u.label].second;
        everywhere.insert(u.slot == player_ ? "ours" : raceName(u.side));
        if (u.slot == player_)
            everywhere.insert(u.name);
    }
    for (const auto& [zone, at] : script_->zones) {
        const auto r = zoneRect(zone);
        std::set<std::string> now;
        std::map<std::string, int> groupInside;
        for (const Unit& u : world_.units()) {
            if (u.building || u.x < r[0] || u.x > r[2] || u.y < r[1] || u.y > r[3])
                continue;
            now.insert("any");
            now.insert(u.slot == player_ ? "ours" : raceName(u.side));
            if (u.slot == player_)
                now.insert(u.name);
            if (!u.label.empty())
                ++groupInside[u.label];
        }
        for (const auto& [label, n] : groupInside)
            if (n == groups[label].second)
                now.insert(label);
        std::set<std::string>& before = inside_[zone];
        for (const std::string& w : now)
            if (!before.count(w))
                out.push_back({"inside", {w, zone}});
        for (const std::string& w : before)
            if (!now.count(w))
                out.push_back({"outside", {w, zone}});
        if (!zonesSeen_) {
            for (const std::string& w : everywhere)
                if (!now.count(w))
                    out.push_back({"outside", {w, zone}});
            for (const auto& [label, n] : groups)
                if (!groupInside.count(label))
                    out.push_back({"outside", {label, zone}});
        }
        before = std::move(now);
    }
    zonesSeen_ = true;
}

} // namespace reluva::newworlds
