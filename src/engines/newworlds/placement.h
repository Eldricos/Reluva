// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "engines/newworlds/world.h"

#include <functional>
#include <utility>

namespace reluva::newworlds {

// Where a building may go (exe.md#building-placement): the game's one test,
// for the player's hologram and the AI's build orders alike. Each type has
// a footprint: a whole block, one of its four tiles, or a quarter of a
// tile (turrets, shield and power generators); mines take a block of open
// ground. The spot snaps to its footprint's middle.
enum class Footprint { Block, Tile, Quarter, Mine };
Footprint footprintOf(int type);
std::pair<float, float> snapped(Footprint f, float x, float y);

enum class PlaceFail { None, Edge, Ground, Unseen, Taken, Units, Deposit };

// Whether a building of the type can go at (x, y) for a side (slot):
// inside the map, not in its outermost ring of blocks; a block or tile on
// building ground (all four of its block's tiles), a mine's on open
// ground; its block seen by the side now; its part of the block free (and
// the block no other side's); no unit standing in the way. seen(bx, by):
// the side sees the block; deposit(bx, by), for the player's mines: the
// block has ore (null: not asked).
PlaceFail placementTest(const World& world, int type, float x, float y, int slot,
                        const std::function<bool(int bx, int by)>& seen,
                        const std::function<bool(int bx, int by)>& deposit = {});

// The building standing (or going up) in the way of one at (x, y), or 0.
int placementBlocker(const World& world, int type, float x, float y);

} // namespace reluva::newworlds
