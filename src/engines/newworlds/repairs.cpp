// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/repairs.h"

#include "engines/newworlds/ai_data.h"

#include <algorithm>
#include <cmath>
#include <set>

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
    // The yards fill their sides' pools.
    for (const Unit& u : world_.units())
        if (u.hp > 0 && u.built >= 1 && u.slot >= 0 && u.slot < 7)
            if (const CombatType* t = combatType(u.type); t && t->repair == 4)
                pool_[size_t(u.slot)] = std::min(kPoolCap, pool_[size_t(u.slot)] + kYardFill);
    // The buildings draw on them, class 1 first.
    for (int cls = 1; cls <= 3; ++cls)
        for (const Unit& u : world_.units()) {
            const CombatType* t = combatType(u.type);
            if (!t || t->repair != cls || u.hp <= 0 || u.built < 1 || u.slot < 0 || u.slot >= 7)
                continue;
            State& s = states_[u.id];
            double& pool = pool_[size_t(u.slot)];
            const double take = std::min({kStep - s.stored, kDraw, pool});
            if (take > 0)
                s.stored += take, pool -= take;
        }
    std::set<int> alive;
    for (const Unit& seen : world_.units())
        alive.insert(seen.id);
    for (int id : alive) {
        Unit& u = *world_.unit(id);
        const CombatType* t = combatType(u.type);
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
    for (auto it = states_.begin(); it != states_.end();)
        it = alive.count(it->first) ? std::next(it) : states_.erase(it);
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
