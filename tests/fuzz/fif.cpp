// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// A New Worlds .fif font, with its image: a u16 (little-endian) giving the
// font's length, the font, then the .fbp texture.

#include "fuzz.h"

#include "engines/newworlds/fif.h"

#include <algorithm>

RELUVA_FUZZ_TARGET(fif)(const uint8_t* data, size_t size) {
    using namespace reluva::newworlds;
    if (size < 2)
        return 0;
    const size_t cut = std::min(size, 2 + size_t(data[0] | data[1] << 8));
    GameFont font;
    if (parseFif(std::vector<uint8_t>(data + 2, data + cut), font))
        return 0;
    setFontImage(std::vector<uint8_t>(data + cut, data + size), font);
    return 0;
}
