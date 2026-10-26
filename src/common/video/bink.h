// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/audio/sound.h"
#include "common/types.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace reluva::video {

// A Sound's format when it comes from a Bink movie's audio.
constexpr int kFormatBink = 0x10042;

// A Bink 1 movie (.bik, versions c to k): its frames decoded one after
// another, and its first audio track (Bink audio of the RDFT kind).
// The format follows FFmpeg's description of it (libavformat/bink.c,
// libavcodec/bink.c and binkaudio.c).
class BinkMovie {
public:
    Error open(const std::filesystem::path& file);

    int width() const { return width_; }
    int height() const { return height_; }
    uint32_t frames() const { return uint32_t(index_.size()); }
    double fps() const { return fpsDen_ ? double(fpsNum_) / fpsDen_ : 0; }
    bool hasAudio() const { return !tracks_.empty(); }

    // Decodes the next frame (false and an error past the end or on bad
    // data). The first call gives frame 0.
    Error nextFrame();
    uint32_t frame() const { return next_; } // frames decoded so far

    // The last decoded frame as RGBA (width() * height() * 4 bytes, rows
    // `pitch` bytes apart).
    void toRgba(uint8_t* out, int pitch) const;

    // The first audio track decoded whole: interleaved 16-bit samples at
    // its rate (empty if the movie has none). Reads every frame's audio.
    Error decodeAudio(audio::Sound& out);

    // A plane of the last frame (0 Y, 1 U, 2 V): its samples, and the
    // distance between its rows.
    const std::vector<uint8_t>& plane(int p) const { return planes_[p]; }
    int stride(int p) const { return p ? cstride_ : ystride_; }

private:
    struct Track {
        int rate = 0;
        bool stereo = false, dct = false;
    };
    Error readPacket(uint32_t frame, std::vector<uint8_t>& out);

    std::ifstream file_;
    uint32_t fileSize_ = 0;
    char version_ = 0;
    uint32_t flags_ = 0;
    int width_ = 0, height_ = 0;
    uint32_t fpsNum_ = 0, fpsDen_ = 0;
    std::vector<Track> tracks_;
    std::vector<uint32_t> index_; // each frame's offset (the last entry's end is fileSize_)

    uint32_t next_ = 0;
    int ystride_ = 0, cstride_ = 0, yrows_ = 0, crows_ = 0;
    std::vector<uint8_t> planes_[4], last_[4];
    std::vector<uint8_t> packet_;
};

} // namespace reluva::video
