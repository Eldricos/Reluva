// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// A New Worlds texture (.lbp, a mipmap level, a font's .fbp).

#include "fuzz.h"

#include "engines/newworlds/texture.h"

RELUVA_FUZZ_TARGET(texture)(const uint8_t* data, size_t size) {
    reluva::newworlds::Texture t;
    reluva::newworlds::decodeTexture(reluva::fuzz::bytes(data, size), t);
    return 0;
}
