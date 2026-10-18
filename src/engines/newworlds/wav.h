// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/types.h"

#include <cstdint>
#include <vector>

namespace reluva::newworlds {

// A decoded sound: signed 16-bit samples, interleaved when stereo.
struct Sound {
    int rate = 0;     // samples per second
    int channels = 0; // 1 or 2
    int format = 0;   // the WAV format tag: 1 PCM, 0x11 IMA ADPCM
    std::vector<int16_t> samples;
    size_t frames() const { return channels ? samples.size() / size_t(channels) : 0; }
};

// A RIFF WAVE file (chunk tag 0x33 in the archives; the game's are IMA ADPCM,
// mono, 22050 or 11025 Hz, in 512-byte blocks). Reads 8- and 16-bit PCM and
// IMA ADPCM (format 0x11). See docs/formats/newworlds/wav.md.
Error decodeWav(const std::vector<uint8_t>& data, Sound& out);

// A 16-bit PCM WAV file of the sound, for checking it elsewhere.
std::vector<uint8_t> encodePcmWav(const Sound& sound);

} // namespace reluva::newworlds
