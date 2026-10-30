// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// A sound file (common/audio): as the mixer's loader takes it (by its
// contents), and as WAV and as MP3 regardless.

#include "fuzz.h"

#include "common/audio/decode.h"
#include "common/audio/mp3.h"
#include "common/audio/wav.h"

RELUVA_FUZZ_TARGET(sound)(const uint8_t* data, size_t size) {
    using namespace reluva::audio;
    const std::vector<uint8_t> in = reluva::fuzz::bytes(data, size);
    Sound s;
    if (!decodeSound(in, s))
        encodePcmWav(s);
    Sound w, m;
    decodeWav(in, w);
    decodeMp3(in, m);
    return 0;
}
