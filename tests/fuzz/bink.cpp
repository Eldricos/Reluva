// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// A Bink movie (common/video/bink): opened, every frame decoded and turned
// to RGBA, and its audio, frame by frame and whole.

#include "fuzz.h"

#include "common/video/bink.h"

RELUVA_FUZZ_TARGET(bink)(const uint8_t* data, size_t size) {
    using namespace reluva;
    const auto path = fuzz::asFile("bink", data, size);
    video::BinkMovie movie;
    if (!movie.open(path).ok)
        return 0;
    std::vector<uint8_t> rgba(size_t(movie.width()) * size_t(movie.height()) * 4);
    std::vector<int16_t> samples;
    for (uint32_t f = 0; f < movie.frames() && f < 64; ++f) {
        if (!movie.nextFrame().ok)
            break;
        movie.toRgba(rgba.data(), movie.width() * 4);
        if (movie.hasAudio())
            movie.decodeAudio(f, samples);
    }
    if (movie.open(path).ok && movie.hasAudio()) {
        audio::Sound whole;
        movie.decodeAudio(whole);
    }
    return 0;
}
