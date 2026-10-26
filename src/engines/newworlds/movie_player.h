// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/audio/mixer.h"
#include "common/events.h"
#include "common/system.h"
#include "common/types.h"
#include "common/video/bink.h"

#include <cstdint>
#include <deque>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace reluva::newworlds {

// The game's movies (Bink): played over the whole screen with their sound,
// one after another, until they end or are skipped (install.md#movies).
class MoviePlayer {
public:
    MoviePlayer(System& sys, Surface& screen, audio::Mixer* mixer, std::filesystem::path gameDir);
    ~MoviePlayer();

    // Where a movie is: the game folder's MOVIES folder (where the CD keeps
    // them), else the game folder itself.
    static std::optional<std::filesystem::path> find(const std::filesystem::path& dir, std::string_view name);

    // Queues movies to play in turn (missing ones left out); returns how
    // many were found.
    int play(std::initializer_list<std::string_view> names);
    bool playing() const { return active_; }
    // Escape, Space, Return or a click skips the movie playing.
    void handleEvent(const Event& ev);
    // Shows the frame due (playing() turns false after the last movie).
    void update(uint64_t nowMs);
    void stop();

private:
    bool startNext(uint64_t nowMs);
    void endMovie();
    double clock(uint64_t nowMs);
    void feedAudio(double seconds);
    void draw();

    System& sys_;
    Surface& screen_;
    audio::Mixer* mixer_;
    std::filesystem::path dir_;
    std::deque<std::filesystem::path> queue_;
    video::BinkMovie movie_;
    bool active_ = false, open_ = false;
    audio::SoundHandle sound_ = audio::kNoSound;
    uint64_t audioFrames_ = 0;     // stereo frames given to the stream
    uint32_t audioPacket_ = 0;     // the next movie frame whose audio is due
    uint64_t startMs_ = 0;         // when the movie started (its clock without sound)
    double lastClock_ = 0;         // the clock at lastClockMs_
    uint64_t lastClockMs_ = 0;
    std::vector<int16_t> samples_;
    std::vector<uint8_t> rgba_;
};

} // namespace reluva::newworlds
