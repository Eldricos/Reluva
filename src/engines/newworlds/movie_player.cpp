// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/movie_player.h"

#include "common/fs.h"

#include <algorithm>
#include <cstring>

namespace reluva::newworlds {

namespace {

// How far ahead of the picture the sound is decoded.
constexpr double kAudioLead = 1.0;

} // namespace

MoviePlayer::MoviePlayer(System& sys, Surface& screen, audio::Mixer* mixer, std::filesystem::path gameDir)
    : sys_(sys), screen_(screen), mixer_(mixer), dir_(std::move(gameDir)) {}

MoviePlayer::~MoviePlayer() { stop(); }

std::optional<std::filesystem::path> MoviePlayer::find(const std::filesystem::path& dir, std::string_view name) {
    if (auto p = fs::findFile(dir, "movies/" + std::string(name)))
        return p;
    return fs::findFile(dir, name);
}

int MoviePlayer::play(std::initializer_list<std::string_view> names) {
    int found = 0;
    for (std::string_view n : names)
        if (auto p = find(dir_, n)) {
            queue_.push_back(*p);
            ++found;
        }
    if (found && !open_)
        active_ = startNext(sys_.getMillis());
    return found;
}

void MoviePlayer::stop() {
    queue_.clear();
    endMovie();
    active_ = false;
}

void MoviePlayer::endMovie() {
    if (mixer_ && sound_ != audio::kNoSound)
        mixer_->stop(sound_);
    sound_ = audio::kNoSound;
    open_ = false;
}

bool MoviePlayer::startNext(uint64_t nowMs) {
    endMovie();
    while (!queue_.empty()) {
        const std::filesystem::path p = queue_.front();
        queue_.pop_front();
        if (movie_.open(p))
            continue; // unreadable: on to the next
        open_ = true;
        startMs_ = lastClockMs_ = nowMs;
        lastClock_ = 0;
        audioFrames_ = 0;
        audioPacket_ = 0;
        rgba_.assign(size_t(movie_.width()) * movie_.height() * 4, 0);
        if (mixer_ && movie_.hasAudio()) {
            mixer_->stopType(audio::SoundType::Music);
            sound_ = mixer_->playStream(movie_.audioRate(), movie_.audioChannels(), audio::SoundType::Music);
            feedAudio(kAudioLead);
        }
        for (int y = 0; y < screen_.h; ++y)
            std::memset(screen_.row(y), 0, size_t(screen_.pitch));
        return true;
    }
    return false;
}

// Seconds into the movie: its sound's position while it plays, else the
// time since it started (or since the sound's last reading).
double MoviePlayer::clock(uint64_t nowMs) {
    if (mixer_ && sound_ != audio::kNoSound && mixer_->isPlaying(sound_)) {
        lastClock_ = mixer_->position(sound_) / movie_.audioRate();
        lastClockMs_ = nowMs;
        return lastClock_;
    }
    return lastClock_ + double(nowMs - lastClockMs_) / 1000.0;
}

void MoviePlayer::feedAudio(double seconds) {
    if (sound_ == audio::kNoSound)
        return;
    const uint64_t want = uint64_t(seconds * movie_.audioRate());
    while (audioFrames_ < want && audioPacket_ < movie_.frames()) {
        samples_.clear();
        if (movie_.decodeAudio(audioPacket_++, samples_)) {
            audioPacket_ = movie_.frames(); // bad audio: the rest silent
            break;
        }
        mixer_->append(sound_, samples_.data(), samples_.size());
        audioFrames_ += samples_.size() / size_t(movie_.audioChannels());
    }
    if (audioPacket_ >= movie_.frames())
        mixer_->finishStream(sound_);
}

void MoviePlayer::handleEvent(const Event& ev) {
    if (!active_)
        return;
    const bool skip = (ev.type == EventType::KeyDown && !ev.repeat &&
                       (ev.key == key::Escape || ev.key == key::Space || ev.key == key::Return)) ||
                      ev.type == EventType::MouseDown;
    if (skip)
        active_ = startNext(sys_.getMillis());
}

void MoviePlayer::update(uint64_t nowMs) {
    if (!active_)
        return;
    const double t = clock(nowMs);
    feedAudio(t + kAudioLead);
    // Every frame up to the one due is decoded (each builds on the last);
    // only the last is shown.
    const uint32_t due = std::min<uint32_t>(movie_.frames(), uint32_t(t * movie_.fps()) + 1);
    bool fresh = false;
    while (movie_.frame() < due) {
        if (movie_.nextFrame()) {
            active_ = startNext(nowMs); // bad data: on to the next movie
            return;
        }
        fresh = true;
    }
    if (fresh)
        draw();
    const bool soundDone = !mixer_ || sound_ == audio::kNoSound || !mixer_->isPlaying(sound_);
    if (movie_.frame() >= movie_.frames() && t * movie_.fps() >= movie_.frames() && soundDone)
        active_ = startNext(nowMs);
}

void MoviePlayer::draw() {
    movie_.toRgba(rgba_.data(), movie_.width() * 4);
    // Centred, cropped to the screen.
    const int w = std::min(movie_.width(), screen_.w), h = std::min(movie_.height(), screen_.h);
    const int sx = (movie_.width() - w) / 2, sy = (movie_.height() - h) / 2;
    const int dx = (screen_.w - w) / 2, dy = (screen_.h - h) / 2;
    for (int y = 0; y < h; ++y)
        std::memcpy(screen_.row(dy + y) + dx * 4, rgba_.data() + (size_t(sy + y) * movie_.width() + sx) * 4,
                    size_t(w) * 4);
}

} // namespace reluva::newworlds
