// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/viewers.h"

#include <algorithm>
#include <cstdio>
#include <map>

namespace reluva::newworlds {

SoundViewer::SoundViewer(System& sys, Surface& screen, const Archives& archives)
    : sys_(sys), screen_(screen), archives_(archives) {
    for (size_t a = 0; a < archives_.size(); ++a) {
        const auto& entries = archives_[a]->entries();
        for (size_t i = 0; i < entries.size(); ++i)
            if (entries[i].tag == cattag::Wave)
                sounds_.push_back({a, i});
    }
    // Find the copies by their size and a hash of their bytes (FNV-1a).
    std::map<std::pair<size_t, uint64_t>, size_t> seen;
    for (size_t s = 0; s < sounds_.size(); ++s) {
        CatArchive& cat = *archives_[sounds_[s].archive];
        std::vector<uint8_t> data;
        if (cat.read(cat.entries()[sounds_[s].entry], data))
            firstCopy_.push_back(s); // unreadable: shown on its own, with the error
        else {
            uint64_t hash = 14695981039346656037ull;
            for (uint8_t b : data)
                hash = (hash ^ b) * 1099511628211ull;
            firstCopy_.push_back(seen.emplace(std::make_pair(data.size(), hash), s).first->second);
        }
    }
    for (size_t s = 0; s < sounds_.size(); ++s)
        if (showCopies_ || firstCopy_[s] == s)
            shownSounds_.push_back(s);
}

const std::string& SoundViewer::nameOf(size_t sound) const {
    return archives_[sounds_[sound].archive]->entries()[sounds_[sound].entry].name;
}

std::string SoundViewer::archiveOf(size_t sound) const {
    return archives_[sounds_[sound].archive]->path().filename().string();
}

SoundViewer::~SoundViewer() {
    if (audioStarted_)
        sys_.stopAudio();
}

void SoundViewer::activate() {
    if (!shown_) {
        shown_ = true;
        show(0);
        return;
    }
    draw();
    updateTitle();
}

void SoundViewer::deactivate() { stop(); }

void SoundViewer::handleKey(uint32_t k) {
    const int n = int(shownSounds_.size());
    if (n == 0)
        return;
    switch (k) {
    case 'f': {
        // List every copy, or only the first of each.
        const size_t sound = shownSounds_[size_t(current_)];
        showCopies_ = !showCopies_;
        shownSounds_.clear();
        for (size_t s = 0; s < sounds_.size(); ++s)
            if (showCopies_ || firstCopy_[s] == s)
                shownSounds_.push_back(s);
        const auto it = std::find(shownSounds_.begin(), shownSounds_.end(), firstCopy_[sound]);
        const auto at = std::find(shownSounds_.begin(), shownSounds_.end(), sound);
        current_ = int((at != shownSounds_.end() ? at : it) - shownSounds_.begin());
        updateTitle();
        break;
    }
    case key::Right: show((current_ + 1) % n); break;
    case key::Left: show((current_ + n - 1) % n); break;
    case key::PageDown: show(std::min(current_ + 10, n - 1)); break;
    case key::PageUp: show(std::max(current_ - 10, 0)); break;
    case key::Home: show(0); break;
    case key::End: show(n - 1); break;
    case key::Space:
    case key::Return:
        if (playing())
            stop();
        else
            play();
        break;
    case 'a': autoPlay_ = !autoPlay_; updateTitle(); break;
    default: break;
    }
}

void SoundViewer::update(uint64_t) {
    // Redraw while playing, to move the play position, and once after.
    const bool now = playing();
    if (now || wasPlaying_) {
        draw();
        updateTitle();
    }
    wasPlaying_ = now;
}

void SoundViewer::show(int index) {
    stop();
    current_ = index;
    status_.clear();
    const EntryRef& ref = sounds_[shownSounds_[size_t(index)]];
    CatArchive& cat = *archives_[ref.archive];
    std::vector<uint8_t> data;
    auto sound = std::make_shared<Sound>();
    if (Error err = cat.read(cat.entries()[ref.entry], data))
        status_ = err.message;
    else if (Error err2 = decodeWav(data, *sound))
        status_ = err2.message;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sound_ = status_.empty() ? sound : nullptr;
    }
    draw();
    updateTitle();
    if (autoPlay_)
        play();
}

bool SoundViewer::playing() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return playing_;
}

void SoundViewer::play() {
    if (!audioStarted_) {
        // Mix the current sound into the output, resampled linearly.
        Error err = sys_.startAudio([this](int16_t* out, int frames) {
            std::lock_guard<std::mutex> lock(mutex_);
            const int rate = sys_.audioRate();
            for (int i = 0; i < frames; ++i) {
                int16_t left = 0, right = 0;
                if (playing_ && sound_ && sound_->frames()) {
                    const size_t n = sound_->frames(), ch = size_t(sound_->channels);
                    const size_t f = size_t(position_);
                    if (f + 1 >= n) {
                        playing_ = false;
                    } else {
                        const double t = position_ - double(f);
                        auto at = [&](size_t frame, size_t c) { return double(sound_->samples[frame * ch + c]); };
                        left = int16_t(at(f, 0) * (1 - t) + at(f + 1, 0) * t);
                        right = ch > 1 ? int16_t(at(f, 1) * (1 - t) + at(f + 1, 1) * t) : left;
                        position_ += double(sound_->rate) / double(rate);
                    }
                }
                out[2 * i] = left;
                out[2 * i + 1] = right;
            }
        });
        if (err) {
            status_ = err.message;
            updateTitle();
            return;
        }
        audioStarted_ = true;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (sound_) {
        position_ = 0;
        playing_ = true;
    }
}

void SoundViewer::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    playing_ = false;
}

void SoundViewer::draw() {
    const int w = screen_.w, h = screen_.h;
    for (int y = 0; y < h; ++y) {
        uint8_t* row = screen_.row(y);
        for (int x = 0; x < w; ++x, row += 4)
            row[0] = 22, row[1] = 26, row[2] = 38, row[3] = 255;
    }
    std::shared_ptr<const Sound> sound;
    double position = 0;
    bool playing = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sound = sound_;
        position = position_;
        playing = playing_;
    }
    auto plot = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (x < 0 || y < 0 || x >= w || y >= h)
            return;
        uint8_t* p = screen_.row(y) + x * 4;
        p[0] = r, p[1] = g, p[2] = b;
    };
    const int mid = h / 2;
    for (int x = 0; x < w; ++x)
        plot(x, mid, 60, 66, 84);
    if (!sound || !sound->frames())
        return;
    // The waveform: each column's lowest and highest sample (first channel).
    const size_t n = sound->frames(), ch = size_t(sound->channels);
    const size_t playFrame = playing ? size_t(position) : n;
    for (int x = 0; x < w; ++x) {
        const size_t from = n * size_t(x) / size_t(w), to = std::max(from + 1, n * size_t(x + 1) / size_t(w));
        int lo = 32767, hi = -32768;
        for (size_t f = from; f < to && f < n; ++f) {
            lo = std::min(lo, int(sound->samples[f * ch]));
            hi = std::max(hi, int(sound->samples[f * ch]));
        }
        const int y0 = mid - hi * (h / 2 - 20) / 32768, y1 = mid - lo * (h / 2 - 20) / 32768;
        const bool played = from < playFrame;
        for (int y = y0; y <= y1; ++y)
            plot(x, y, played ? 255 : 120, played ? 200 : 170, played ? 80 : 230);
    }
    if (playing) {
        const int x = int(double(w) * position / double(n));
        for (int y = 0; y < h; ++y)
            plot(x, y, 255, 255, 255);
    }
}

void SoundViewer::updateTitle() {
    if (shownSounds_.empty())
        return;
    const size_t current = shownSounds_[size_t(current_)];
    std::string title = std::string("New Worlds sounds ") + (showCopies_ ? "[all] " : "") +
                        std::to_string(current_ + 1) + "/" + std::to_string(shownSounds_.size()) + ": " +
                        nameOf(current) + " in " + archiveOf(current) + " (";
    std::shared_ptr<const Sound> sound;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sound = sound_;
    }
    if (!status_.empty() || !sound) {
        title += status_ + ")";
    } else {
        char info[96];
        std::snprintf(info, sizeof info, "%d Hz, %s, %.2f s", sound->rate, sound->channels == 1 ? "mono" : "stereo",
                      double(sound->frames()) / sound->rate);
        title += std::string(info) + (sound->format == 0x11 ? ", IMA ADPCM" : ", PCM") + ")";
    }
    // Its copies: how many, and any other names or archives they're in.
    size_t copies = 0;
    std::vector<std::string> names;
    for (size_t s = 0; s < sounds_.size(); ++s) {
        if (firstCopy_[s] != firstCopy_[current])
            continue;
        ++copies;
        std::string where;
        if (nameOf(s) != nameOf(current))
            where = nameOf(s);
        if (archiveOf(s) != archiveOf(current))
            where += (where.empty() ? "in " : " in ") + archiveOf(s);
        if (!where.empty() && std::find(names.begin(), names.end(), where) == names.end())
            names.push_back(where);
    }
    if (copies > 1) {
        title += " [" + std::to_string(copies) + " copies";
        for (size_t i = 0; i < names.size(); ++i)
            title += (i ? ", " : "; also ") + names[i];
        title += "]";
    }
    title += playing() ? " [playing]" : " (Space: play)";
    if (autoPlay_)
        title += " [auto-play]";
    sys_.setWindowTitle(title);
}

} // namespace reluva::newworlds
