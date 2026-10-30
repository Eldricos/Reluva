// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// A New Worlds .cat archive: opened, every entry read.

#include "fuzz.h"

#include "engines/newworlds/cat_archive.h"

RELUVA_FUZZ_TARGET(cat_archive)(const uint8_t* data, size_t size) {
    using namespace reluva::newworlds;
    CatArchive cat;
    if (cat.open(reluva::fuzz::asFile("cat_archive", data, size)))
        return 0;
    std::vector<uint8_t> out;
    for (const CatEntry& e : cat.entries()) {
        cat.read(e, out);
        cat.find(e.name);
    }
    return 0;
}
