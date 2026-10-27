// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/savestate.h"
#include "engines/newworlds/world.h"

#include <array>
#include <cstdint>
#include <vector>

namespace reluva::newworlds {

// The fog of war, as the game's executable keeps it (exe.md#fog-of-war):
// one cell a block, each with six channels of a bit per side (slots 0-5).
// Every object marks the cells round it for its own side, by its type's
// fog class (a disc 7 or 5 blocks across, a 3 x 3 square, ...). The marks
// mean "covered now": each tick one cell in 64 (one of the 8 x 8 residues)
// is cleared and marked again, so the whole map every 64 ticks.
//
// Channels: 0 seen (the scripts' "cell seen"; other sides' objects are
// shown only on cells their viewer sees), 1 scanned (science vehicles and
// most buildings, the 3 x 3 round them: the scripts' "cell scanned"),
// 2 surveyed (minerals: the Tricorder's ore names), 3 and 4 two more
// science channels, 5 inside a shield generator's field.
class Fog {
public:
    // A savestate's part (docs/savestates.md): what play changes, saved or
    // loaded; the rest is made again as the mission starts.
    void syncState(save::Serializer& s);
    static constexpr int kChannels = 6;
    enum Channel { Seen, Scanned, Surveyed, Deep, Deeper, Shielded };

    Fog(int blocksWide, int blocksHigh);

    // One of the game's ticks (20 a second): a residue cleared and marked.
    void tick(const World& world);
    // The whole map marked at once (the start).
    void refresh(const World& world);

    bool has(int bx, int by, Channel c, int slot) const;
    bool seenAt(float x, float y, int slot) const;
    int width() const { return w_; }
    int height() const { return h_; }

private:
    void stamp(const World& world, int rx, int ry); // residue rx, ry; -1 for every cell
    std::array<uint8_t, kChannels>* cell(int bx, int by);

    int w_, h_;
    std::vector<std::array<uint8_t, kChannels>> cells_;
    int step_ = 0;
};

} // namespace reluva::newworlds
