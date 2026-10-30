// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// A New Worlds .geo model, and what's read from one: a ground tile's
// sides, a star system's layout.

#include "fuzz.h"

#include "engines/newworlds/geo.h"
#include "engines/newworlds/star_system.h"

RELUVA_FUZZ_TARGET(geo)(const uint8_t* data, size_t size) {
    using namespace reluva::newworlds;
    GeoModel geo;
    if (parseGeo(reluva::fuzz::bytes(data, size), geo))
        return 0;
    for (int r = 0; r < 4; ++r)
        tileSides(geo, r);
    const StarSystemLayout layout = readStarSystem(geo);
    starSystemTextures(layout, "ice");
    missionBody(layout);
    return 0;
}
