// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/fog.h"

#include "engines/newworlds/ai_data.h"

#include <cstdlib>

namespace reluva::newworlds {

namespace {

constexpr float kBlock = 16384;

// The masks' shapes (exe.md#fog-of-war), centred on the object's block:
// whether an offset is inside.
enum Shape { None, Disc7, Disc5, Square3, Plus, Centre };

bool inside(Shape s, int dx, int dy) {
    const int ax = std::abs(dx), ay = std::abs(dy);
    switch (s) {
    case Disc7: // rows 3, 5, 7, 7, 7, 5, 3 wide
        return ay <= 3 && ax <= (ay == 3 ? 1 : ay == 2 ? 2 : 3);
    case Disc5: // rows 3, 5, 5, 5, 3
        return ay <= 2 && ax <= (ay == 2 ? 1 : 2);
    case Square3: return ax <= 1 && ay <= 1;
    case Plus: return ax + ay <= 1;
    case Centre: return ax == 0 && ay == 0;
    default: return false;
    }
}

// Each fog class's shape per channel (seen, scanned, surveyed, deep,
// deeper, shielded). Class 8 marks nothing (it hides; see stamp).
constexpr Shape kClasses[9][Fog::kChannels] = {
    {None, None, None, None, None, None},
    {Disc7, None, None, None, None, None},
    {Disc7, None, None, None, None, Square3},
    {Disc5, None, None, None, None, None},
    {Disc7, Square3, Disc7, Disc7, Disc7, None},
    {Disc7, None, Disc7, None, None, None},
    {Disc7, None, None, None, None, Plus},
    {Disc7, None, None, None, None, Centre},
    {None, None, None, None, None, None},
};

int fogClass(const Unit& u) {
    const CombatType* t = combatType(u.type);
    return t && t->fog >= 0 && t->fog <= 8 ? t->fog : 0;
}

} // namespace

Fog::Fog(int blocksWide, int blocksHigh)
    : w_(std::max(1, blocksWide)), h_(std::max(1, blocksHigh)), cells_(size_t(w_) * size_t(h_)) {
    for (auto& c : cells_)
        c.fill(0);
}

std::array<uint8_t, Fog::kChannels>* Fog::cell(int bx, int by) {
    if (bx < 0 || by < 0 || bx >= w_ || by >= h_)
        return nullptr;
    return &cells_[size_t(by) * size_t(w_) + size_t(bx)];
}

bool Fog::has(int bx, int by, Channel c, int slot) const {
    if (bx < 0 || by < 0 || bx >= w_ || by >= h_ || slot < 0 || slot > 5)
        return false;
    return (cells_[size_t(by) * size_t(w_) + size_t(bx)][size_t(c)] >> slot) & 1;
}

bool Fog::seenAt(float x, float y, int slot) const { return has(int(x / kBlock), int(y / kBlock), Seen, slot); }

// Marks round every object for its side, on the cells of one residue (the
// cells whose x and y modulo 8 are rx, ry), or on every cell (rx < 0).
void Fog::stamp(const World& world, int rx, int ry) {
    auto mine = [&](int bx, int by) { return rx < 0 || ((bx & 7) == rx && (by & 7) == ry); };
    for (const Unit& u : world.units()) {
        if (u.hp <= 0 || u.slot < 0 || u.slot > 5)
            continue;
        const int cls = fogClass(u);
        const int cx = int(u.x / kBlock), cy = int(u.y / kBlock);
        const uint8_t bit = uint8_t(1 << u.slot);
        if (cls == 8) {
            // A jammer: on its own block, only its own side's marks stay.
            if (auto* c = cell(cx, cy); c && mine(cx, cy))
                for (int ch = 0; ch < 5; ++ch)
                    (*c)[size_t(ch)] &= bit;
            continue;
        }
        if (cls == 0)
            continue;
        for (int dy = -4; dy < 4; ++dy)
            for (int dx = -4; dx < 4; ++dx) {
                if (!mine(cx + dx, cy + dy))
                    continue;
                auto* c = cell(cx + dx, cy + dy);
                if (!c)
                    continue;
                for (int ch = 0; ch < kChannels; ++ch)
                    if (inside(kClasses[cls][ch], dx, dy))
                        (*c)[size_t(ch)] |= bit;
            }
    }
}

void Fog::tick(const World& world) {
    // This tick's residue: cleared, then marked again.
    const int rx = step_ % 8, ry = step_ / 8;
    step_ = (step_ + 1) % 64;
    for (int by = ry; by < h_; by += 8)
        for (int bx = rx; bx < w_; bx += 8)
            cell(bx, by)->fill(0);
    stamp(world, rx, ry);
}

void Fog::refresh(const World& world) {
    for (auto& c : cells_)
        c.fill(0);
    stamp(world, -1, -1);
}

} // namespace reluva::newworlds
