// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include <cstdint>

namespace reluva::video {

// Bink video's constant tables (bink_tables.cpp).
extern const uint8_t kBinkScan[64];             // the DCT and residue coefficient order
extern const uint8_t kBinkTreeCodes[16][16];    // the 16 trees' codes (read low bit first)
extern const uint8_t kBinkTreeLengths[16][16];  // and their lengths
extern const uint8_t kBinkPatterns[16][64];     // run blocks' pixel orders
extern const int32_t kBinkIntraQuant[16][64];   // quantizers (11 fraction bits), in scan order
extern const int32_t kBinkInterQuant[16][64];

} // namespace reluva::video
