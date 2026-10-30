// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// A New Worlds .gmf map, and every tile of it looked up.

#include "fuzz.h"

#include "engines/newworlds/gmf.h"

RELUVA_FUZZ_TARGET(gmf)(const uint8_t* data, size_t size) {
    using namespace reluva::newworlds;
    GmfMap map;
    if (parseGmf(reluva::fuzz::bytes(data, size), map))
        return 0;
    for (int y = -1; y <= 2 * map.height && y < 256; ++y)
        for (int x = -1; x <= 2 * map.width && x < 256; ++x)
            tileAt(map, x, y);
    return 0;
}
