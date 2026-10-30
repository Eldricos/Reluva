// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "common/audio/mixer.h"

#include <algorithm>
#include <cmath>

namespace reluva::audio {

Mixer::Mixer(int outputRate) : outputRate_(std::max(outputRate, 1)) {}

Mixer::Channel* Mixer::find(SoundHandle handle) {
    for (Channel& c : channels_)
        if (c.handle == handle && !c.done)
            return &c;
    return nullptr;
}

const Mixer::Channel* Mixer::find(SoundHandle handle) const {
    for (const Channel& c : channels_)
        if (c.handle == handle && !c.done)
            return &c;
    return nullptr;
}

SoundHandle Mixer::play(std::shared_ptr<const Sound> sound, SoundType type, int volume, int pan, bool loop) {
    if (!sound || sound->frames() == 0 || sound->rate <= 0 || sound->channels < 1 || sound->channels > 2)
        return kNoSound;
    std::lock_guard<std::mutex> lock(mutex_);
    const SoundHandle handle = nextHandle_++;
    if (nextHandle_ == kNoSound)
        nextHandle_ = 1;
    const int rate = sound->rate, channels = sound->channels;
    channels_.push_back({handle, std::move(sound), type, std::clamp(volume, 0, kMaxVolume),
                         std::clamp(pan, -kMaxPan, kMaxPan), loop, rate, channels, 0, false, nullptr, false});
    return handle;
}

SoundHandle Mixer::playStream(int rate, int channels, SoundType type, int volume) {
    if (rate <= 0 || channels < 1 || channels > 2)
        return kNoSound;
    auto stream = std::make_unique<Stream>();
    stream->chunks.reserve(16);
    stream->spent.reserve(16);
    std::lock_guard<std::mutex> lock(mutex_);
    const SoundHandle handle = nextHandle_++;
    if (nextHandle_ == kNoSound)
        nextHandle_ = 1;
    channels_.push_back({handle, nullptr, type, std::clamp(volume, 0, kMaxVolume), 0, false, rate, channels, 0, false,
                         std::move(stream), true});
    return handle;
}

void Mixer::append(SoundHandle stream, const int16_t* samples, size_t count) {
    if (!samples || !count)
        return;
    // Allocated and freed outside the lock: the audio thread waits on it.
    std::vector<int16_t> chunk(samples, samples + count);
    std::vector<std::vector<int16_t>> spent;
    spent.reserve(16);
    std::lock_guard<std::mutex> lock(mutex_);
    Channel* c = find(stream);
    if (!c || !c->open)
        return;
    Stream& st = *c->stream;
    chunk.resize(chunk.size() - chunk.size() % size_t(c->channels));
    if (chunk.empty())
        return;
    spent.swap(st.spent);
    // The chunks mix() has finished with (moved to `spent`) make room.
    st.chunks.erase(st.chunks.begin(), st.chunks.begin() + long(st.head));
    st.head = 0;
    st.frames += chunk.size() / size_t(c->channels);
    st.chunks.push_back(std::move(chunk));
}

void Mixer::finishStream(SoundHandle stream) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (Channel* c = find(stream))
        c->open = false;
}

void Mixer::stop(SoundHandle handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (Channel* c = find(handle))
        c->done = true;
}

void Mixer::stopType(SoundType type) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (Channel& c : channels_)
        if (c.type == type)
            c.done = true;
}

void Mixer::stopAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (Channel& c : channels_)
        c.done = true;
}

bool Mixer::isPlaying(SoundHandle handle) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return find(handle) != nullptr;
}

bool Mixer::isTypePlaying(SoundType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::any_of(channels_.begin(), channels_.end(), [&](const Channel& c) { return c.type == type && !c.done; });
}

double Mixer::position(SoundHandle handle) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const Channel* c = find(handle);
    return c ? c->position : 0;
}

void Mixer::setVolume(SoundHandle handle, int volume) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (Channel* c = find(handle))
        c->volume = std::clamp(volume, 0, kMaxVolume);
}

void Mixer::setPan(SoundHandle handle, int pan) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (Channel* c = find(handle))
        c->pan = std::clamp(pan, -kMaxPan, kMaxPan);
}

void Mixer::setTypeVolume(SoundType type, int volume) {
    std::lock_guard<std::mutex> lock(mutex_);
    typeVolume_[int(type)] = std::clamp(volume, 0, kMaxVolume);
}

int Mixer::typeVolume(SoundType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return typeVolume_[int(type)];
}

void Mixer::mix(int16_t* out, int frames) {
    if (frames <= 0)
        return;
    std::lock_guard<std::mutex> lock(mutex_);
    accumulator_.assign(size_t(frames) * 2, 0);
    for (Channel& c : channels_) {
        if (c.done)
            continue;
        Stream* st = c.stream.get();
        const size_t ch = size_t(c.channels);
        const size_t n = st ? st->frames : c.sound->frames();
        const double step = double(c.rate) / double(outputRate_);
        // Gains in 1/65536ths: volume x type volume, and a linear pan that
        // keeps the louder side at full level.
        const int64_t gain = int64_t(c.volume) * typeVolume_[int(c.type)];
        const int64_t gainL = c.pan > 0 ? gain * (kMaxPan - c.pan) / kMaxPan : gain;
        const int64_t gainR = c.pan < 0 ? gain * (kMaxPan + c.pan) / kMaxPan : gain;
        auto sample = [&](size_t frame, size_t channel) {
            if (!st)
                return double(c.sound->samples[frame * ch + channel]);
            // A stream's: in the chunk that holds it (one of the first few).
            frame -= st->first;
            for (size_t k = st->head; k < st->chunks.size(); ++k) {
                const size_t kf = st->chunks[k].size() / ch;
                if (frame < kf)
                    return double(st->chunks[k][frame * ch + channel]);
                frame -= kf;
            }
            return 0.0;
        };
        for (int i = 0; i < frames; ++i) {
            if (c.position >= double(n)) {
                if (c.open)
                    break; // a stream waiting for more: silence until it comes
                if (!c.loop) {
                    c.done = true;
                    break;
                }
                c.position = std::fmod(c.position, double(n));
            }
            // Linear interpolation towards the next frame (the first, when
            // looping past the end; else the last holds).
            const size_t f = size_t(c.position);
            const size_t g = f + 1 < n ? f + 1 : (c.loop ? 0 : f);
            const double t = c.position - double(f);
            const double l = sample(f, 0) * (1 - t) + sample(g, 0) * t;
            const double r = ch > 1 ? sample(f, 1) * (1 - t) + sample(g, 1) * t : l;
            accumulator_[size_t(i) * 2] += int32_t(std::lround(l * double(gainL) / 65536.0));
            accumulator_[size_t(i) * 2 + 1] += int32_t(std::lround(r * double(gainR) / 65536.0));
            c.position += step;
        }
        // A stream lets go of the chunks it has played past.
        while (st && st->head < st->chunks.size() &&
               size_t(c.position) - st->first >= st->chunks[st->head].size() / ch) {
            st->first += st->chunks[st->head].size() / ch;
            st->spent.push_back(std::move(st->chunks[st->head++]));
        }
    }
    for (size_t i = 0; i < accumulator_.size(); ++i)
        out[i] = int16_t(std::clamp<int32_t>(accumulator_[i], -32768, 32767));
    channels_.erase(std::remove_if(channels_.begin(), channels_.end(), [](const Channel& c) { return c.done; }),
                    channels_.end());
}

Error startMixerOutput(System& sys, Mixer& mixer) {
    return sys.startAudio([&mixer](int16_t* out, int frames) { mixer.mix(out, frames); });
}

} // namespace reluva::audio
