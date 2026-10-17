// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/types.h"

#include <array>
#include <cstdint>
#include <vector>

namespace reluva::newworlds {

// A .gmf map (chunk tag 0x11). See docs/formats/newworlds/gmf.md; much of the
// format is still unknown and is skipped.
//
// A map is a grid of blocks; a block is 2 x 2 tiles; a tile is a ground model
// from the planet's archive (8192 x 8192 units).
struct GmfMap {
    struct Tile {
        uint32_t model = 0;  // index of its ground model among the archive's .geo files
        uint32_t flags = 0;  // unknown (0x8000, 0x18000, 0x1018000 seen)
        std::array<uint8_t, 64> cells{}; // the first 8 x 8 grid; unknown meaning
    };
    struct Cell {
        uint16_t block = 0; // which block
        uint16_t value = 0; // unknown (50 in every kurt64 cell)
    };

    int width = 0, height = 0;                 // in blocks
    std::vector<Tile> tiles;
    std::vector<std::array<uint32_t, 4>> blocks; // four tile numbers each
    std::vector<Cell> cells;                   // width x height, row by row
};

Error parseGmf(const std::vector<uint8_t>& data, GmfMap& out);

} // namespace reluva::newworlds
