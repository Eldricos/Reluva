// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// A New Worlds .stb string table.

#include "fuzz.h"

#include "engines/newworlds/string_table.h"

RELUVA_FUZZ_TARGET(string_table)(const uint8_t* data, size_t size) {
    reluva::newworlds::StringTable t;
    if (!t.parse(reluva::fuzz::bytes(data, size)))
        for (const auto& s : t.strings())
            t.find(s.id);
    return 0;
}
