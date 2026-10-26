// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/placement.h"

#include <cmath>
#include <map>

namespace reluva::newworlds {

namespace {

constexpr float kBlockSize = 16384, kQuarter = 4096;
// A unit's reserved box reaches this far past its middle.
constexpr float kUnitBox = 512;

// A block's occupancy: four bits a tile (its quarters), and the side
// whose building first stands in it (-1 none).
struct Cells {
    uint16_t bits = 0;
    int owner = -1;
};

// The bits a footprint at a snapped spot takes in its block.
uint16_t bitsOf(Footprint f, float x, float y) {
    const int tx = int(std::floor(x / kTile)) & 1, ty = int(std::floor(y / kTile)) & 1;
    const int tile = tx + 2 * ty;
    switch (f) {
    case Footprint::Block:
    case Footprint::Mine: return 0xffff;
    case Footprint::Tile: return uint16_t(0xf << (4 * tile));
    case Footprint::Quarter: {
        const int qx = std::fmod(x, kTile) >= kQuarter ? 1 : 0, qy = std::fmod(y, kTile) >= kQuarter ? 1 : 0;
        return uint16_t(1 << (4 * tile + qx + 2 * qy));
    }
    }
    return 0;
}

std::pair<int, int> blockOf(float x, float y) {
    return {int(std::floor(x / kBlockSize)), int(std::floor(y / kBlockSize))};
}

} // namespace

Footprint footprintOf(int type) {
    static const std::map<int, Footprint> kTypes = [] {
        std::map<int, Footprint> m;
        for (int t : {53, 26, 122, 51, 32, 66, 217, 27, 73, 72, 126, 127, 158, 172, 178, 50, 182, 222, 74})
            m[t] = Footprint::Tile;
        for (int t : {80, 83, 54, 82, 81, 84, 316, 317, 21, 29, 69, 22, 30, 70, 198, 219, 220})
            m[t] = Footprint::Quarter;
        for (int t = 276; t <= 288; ++t)
            m[t] = Footprint::Quarter;
        for (int t : {45, 163, 52, 189, 65, 173})
            m[t] = Footprint::Mine;
        return m;
    }();
    const auto it = kTypes.find(type);
    return it == kTypes.end() ? Footprint::Block : it->second;
}

std::pair<float, float> snapped(Footprint f, float x, float y) {
    auto centre = [](float v, float size) { return (std::floor(v / size) + 0.5f) * size; };
    switch (f) {
    case Footprint::Block:
    case Footprint::Mine: return {centre(x, kBlockSize), centre(y, kBlockSize)};
    case Footprint::Tile: return {centre(x, kTile), centre(y, kTile)};
    case Footprint::Quarter: return {centre(x, kQuarter), centre(y, kQuarter)};
    }
    return {x, y};
}

PlaceFail placementTest(const World& world, int type, float x, float y, int slot,
                        const std::function<bool(int bx, int by)>& seen,
                        const std::function<bool(int bx, int by)>& deposit) {
    const Footprint f = footprintOf(type);
    const auto [sx, sy] = snapped(f, x, y);
    const auto [bx, by] = blockOf(sx, sy);
    const Terrain& t = world.terrain();
    const int blocksWide = t.width / 2, blocksHigh = t.height / 2;
    if (bx <= 0 || by <= 0 || bx >= blocksWide - 1 || by >= blocksHigh - 1)
        return PlaceFail::Edge;
    // The ground: a block's four tiles all building ground (a mine's all
    // open ground, none of it building ground); a quarter anywhere.
    // (A map without its tiles' flags takes anything.)
    if (f != Footprint::Quarter && !t.ground.empty()) {
        for (int i = 0; i < 4; ++i) {
            const int tx = 2 * bx + (i & 1), ty = 2 * by + (i >> 1);
            const uint8_t g = t.ground[size_t(ty) * size_t(t.width) + size_t(tx)];
            const bool build = (g & (kPadGround | kBuildGround)) != 0, open = (g & kOpenGround) != 0;
            if (f == Footprint::Mine ? build || !open : !build)
                return PlaceFail::Ground;
        }
    }
    if (seen && !seen(bx, by))
        return PlaceFail::Unseen;
    // What already stands in the block (sites too).
    Cells cells;
    for (const Unit& u : world.units()) {
        if (!u.building || u.hp <= 0 || blockOf(u.x, u.y) != std::make_pair(bx, by))
            continue;
        const Footprint uf = footprintOf(u.type);
        const auto [ux, uy] = snapped(uf, u.x, u.y);
        cells.bits |= bitsOf(uf, ux, uy);
        if (cells.owner < 0)
            cells.owner = u.slot;
    }
    const uint16_t want = bitsOf(f, sx, sy);
    if ((cells.bits & want) != 0 || (f != Footprint::Block && f != Footprint::Mine && cells.owner >= 0 && cells.owner != slot))
        return PlaceFail::Taken;
    // No unit in the way.
    const float half = (f == Footprint::Block ? 8192.0f : f == Footprint::Quarter ? 2048.0f : 4096.0f) + kUnitBox;
    for (const Unit& u : world.units())
        if (!u.building && !u.flies && u.hp > 0 && std::abs(u.x - sx) <= half && std::abs(u.y - sy) <= half)
            return PlaceFail::Units;
    if (f == Footprint::Mine && deposit && !deposit(bx, by))
        return PlaceFail::Deposit;
    return PlaceFail::None;
}

int placementBlocker(const World& world, int type, float x, float y) {
    const Footprint f = footprintOf(type);
    const auto [sx, sy] = snapped(f, x, y);
    const uint16_t want = bitsOf(f, sx, sy);
    for (const Unit& u : world.units()) {
        if (!u.building || u.hp <= 0 || blockOf(u.x, u.y) != blockOf(sx, sy))
            continue;
        const Footprint uf = footprintOf(u.type);
        const auto [ux, uy] = snapped(uf, u.x, u.y);
        if (bitsOf(uf, ux, uy) & want)
            return u.id;
    }
    return 0;
}

} // namespace reluva::newworlds
