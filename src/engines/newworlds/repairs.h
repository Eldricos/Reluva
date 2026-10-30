// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/savestate.h"
#include "engines/newworlds/fog.h"
#include "engines/newworlds/world.h"

#include <array>
#include <functional>
#include <map>
#include <vector>

namespace reluva::newworlds {

struct CombatType;

// Repairs, as the game's executable has them (exe.md#repairs), every tick
// and in battle too; a building still going up isn't repaired.
//
// A vehicle (and a native's building) mends itself a point at a time:
// after each point it waits 6400 / its full hit points ticks, a third of
// that with a skilled engineer aboard (divided by 2 x skill + 1), and uses
// no power. A colony's building draws on its side's repair pool, which
// only construction yards fill (8192 a tick each): it takes up to 2048 a
// tick until it holds 16384, then spends that on 2 x skill + 1 points. The
// pool serves its buildings by priority class (1 first). A building that
// holds nothing mends at the vehicles' rate.
class Repairs {
public:
    // A savestate's part (docs/savestates.md): what play changes, saved or
    // loaded; the rest is made again as the mission starts.
    void syncState(save::Serializer& s);
    explicit Repairs(World& world);

    // The best engineering skill in an object (its officers'; 0 none).
    void setEngineering(std::function<float(int unit)> skill) { skill_ = std::move(skill); }

    // Moves time on, at the game's 20 ticks a second.
    void advance(double seconds);
    void tick();

    double pool(int slot) const { return slot >= 0 && slot < 7 ? pool_[size_t(slot)] : 0; }
    static constexpr double kPoolCap = 2097152;

private:
    struct State {
        int countdown = 0;
        double stored = 0; // repair energy held
        friend void syncState(save::Serializer& s, State& v) { s(v.countdown)(v.stored); }
    };
    World& world_;
    std::function<float(int)> skill_;
    std::map<int, State> states_;
    std::array<double, 7> pool_{};
    double clock_ = 0;
    // Scratch for tick(), kept to save allocating (not saved).
    std::vector<const CombatType*> types_;
    std::vector<int> alive_;
};

// The hit points a hit takes from its target (exe.md#shields): a building,
// armed vehicle, armed APC, science vehicle or scout inside a shield field
// of its own side takes half (rounded down), and its best security
// officer divides what's left by 4 x skill + 1. Other kinds take it all.
float shieldedDamage(const Unit& target, float damage, const Fog* fog, float security);

} // namespace reluva::newworlds
