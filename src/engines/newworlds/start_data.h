// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "engines/newworlds/economy.h"

#include <string_view>
#include <vector>

namespace reluva::newworlds {

// What the game's executable says each mission starts with
// (docs/formats/newworlds/exe.md#starting-colonies), for the three player
// sides: the buildings and vehicles it places, and the starting stock.

// An object type, by the game's id: a building (by Reluva's building id,
// "construction-yard") or a vehicle (by its panel name, "GROUND SCOUT"),
// its model ("geo/fed/fcy.geo"; empty if none is known), and a vehicle's
// pace (the game's path steps a tick; how far a step goes isn't known).
struct ObjectType {
    int id;
    bool building;
    const char* name;
    const char* model;
    float pace;
};
const ObjectType* objectType(int id);

struct StartPlacement {
    const char* mission; // "F1", "J3", "C4"
    char side;           // 'F', 'K', 'R'
    int type;            // an ObjectType id
    int a, b;            // map position: eighths of a block, row first
};
// A mission's placements for a side (empty if it has none).
std::vector<StartPlacement> startPlacements(std::string_view mission, char side);
// A mission's starting stock for a side; false if it isn't given.
bool startStock(std::string_view mission, char side, Stock& out);

} // namespace reluva::newworlds
