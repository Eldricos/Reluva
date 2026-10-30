// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/repairs.h"

#include "engines/newworlds/ai_data.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace reluva::newworlds {

namespace {

constexpr double kTick = 0.05;
constexpr double kStep = 16384;     // repair energy a building spends on a step
constexpr double kDraw = 2048;      // the most it takes from the pool a tick
constexpr double kYardFill = 8192;  // what a construction yard adds to the pool a tick
constexpr float kBlock = 16384;

} // namespace

Repairs::Repairs(World& world) : world_(world) {}

void Repairs::advance(double seconds) {
    clock_ += seconds;
    while (clock_ >= kTick) {
        clock_ -= kTick;
        tick();
    }
}

void Repairs::tick() {
    // The world is ours to change (world_ isn't const); units() only hands
    // out a const view, and looking each unit up by id would be O(n) apiece.
    auto& units = const_cast<std::vector<Unit>&>(world_.units());
    types_.resize(units.size());
    // The yards fill their sides' pools.
    for (size_t i = 0; i < units.size(); ++i) {
        const Unit& u = units[i];
        const CombatType* t = types_[i] = combatType(u.type);
        if (u.hp > 0 && u.built >= 1 && u.slot >= 0 && u.slot < 7)
            if (t && t->repair == 4)
                pool_[size_t(u.slot)] = std::min(kPoolCap, pool_[size_t(u.slot)] + kYardFill);
    }
    // The buildings draw on them, class 1 first.
    for (int cls = 1; cls <= 3; ++cls)
        for (size_t i = 0; i < units.size(); ++i) {
            const Unit& u = units[i];
            const CombatType* t = types_[i];
            if (!t || t->repair != cls || u.hp <= 0 || u.built < 1 || u.slot < 0 || u.slot >= 7)
                continue;
            State& s = states_[u.id];
            double& pool = pool_[size_t(u.slot)];
            const double take = std::min({kStep - s.stored, kDraw, pool});
            if (take > 0)
                s.stored += take, pool -= take;
        }
    // Each unit mends on its own (nothing here reads another's), so the
    // world's order serves as well as the ids' order.
    alive_.clear();
    for (size_t i = 0; i < units.size(); ++i) {
        Unit& u = units[i];
        alive_.push_back(u.id);
        const CombatType* t = types_[i];
        if (!t || t->repair == 4 || u.hp <= 0 || u.built < 1 || u.hp >= u.maxHp)
            continue;
        State& s = states_[u.id];
        const float skill = skill_ ? skill_(u.id) : 0;
        if (t->repair >= 1 && s.stored > 0) {
            // A building with energy in hand: a step once it holds enough.
            if (s.stored >= kStep && u.maxHp - u.hp >= 1) {
                s.stored -= kStep;
                u.hp = std::min(u.maxHp, u.hp + std::trunc(2 * skill + 1));
            }
            continue;
        }
        if (s.countdown > 0) {
            --s.countdown;
            continue;
        }
        u.hp = std::min(u.maxHp, u.hp + 1);
        s.countdown = std::max(1, int(std::trunc(std::trunc(6400 / u.maxHp) / (2 * skill + 1))));
    }
    // Forget the units that are gone (the world keeps them in id order, so
    // this rarely sorts).
    if (!std::is_sorted(alive_.begin(), alive_.end()))
        std::sort(alive_.begin(), alive_.end());
    for (auto it = states_.begin(); it != states_.end();)
        it = std::binary_search(alive_.begin(), alive_.end(), it->first) ? std::next(it) : states_.erase(it);
}

float shieldedDamage(const Unit& target, float damage, const Fog* fog, float security) {
    const CombatType* t = combatType(target.type);
    if (!t || !t->shielded)
        return damage;
    if (fog && fog->has(int(target.x / kBlock), int(target.y / kBlock), Fog::Shielded, target.slot))
        damage = std::floor(damage / 2);
    return std::trunc(damage / (4 * security + 1));
}

void Repairs::syncState(save::Serializer& s) {
    s(states_)(pool_)(clock_);
}

} // namespace reluva::newworlds
