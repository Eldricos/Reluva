// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// New Worlds' level.dat, both ways (failing on a bad line, and skipping it).

#include "fuzz.h"

#include "engines/newworlds/levels.h"

#include <string_view>

RELUVA_FUZZ_TARGET(levels)(const uint8_t* data, size_t size) {
    using namespace reluva::newworlds;
    const std::string_view text(reinterpret_cast<const char*>(data), size);
    std::vector<Level> levels;
    std::vector<std::string> skipped;
    parseLevelList(text, levels);
    parseLevelList(text, levels, &skipped);
    for (const Level& l : levels)
        levelForMap(levels, "map/" + l.shortName + ".gmf");
    return 0;
}
