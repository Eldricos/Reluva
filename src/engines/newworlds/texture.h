// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/types.h"

#include <cstdint>
#include <vector>

namespace reluva::newworlds {

// A decoded texture (chunk tag 0x16). See docs/formats/newworlds/lbp.md.
struct Texture {
    uint32_t format = 0;
    uint32_t sizeFlag = 0;     // the u32 after the palette: 0 = size stored, 1 = not stored
    bool sizeGuessed = false;  // true when the size was inferred from the data length
    int width = 0, height = 0;
    std::vector<uint8_t> rgba; // width * height * 4, bytes R, G, B, A
};

Error decodeTexture(const std::vector<uint8_t>& data, Texture& out);

} // namespace reluva::newworlds
