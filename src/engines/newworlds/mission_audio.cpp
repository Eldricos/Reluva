// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/mission_ui.h"
#include "engines/newworlds/viewers.h"

#include "common/audio/decode.h"
#include "common/fs.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>

namespace reluva::newworlds {

namespace {

constexpr int kMusicVolume = 160; // under the speech

} // namespace

std::shared_ptr<const audio::Sound> MissionViewer::loadSound(const std::string& stem) {
    // The game folder's (Media/), else the archives' (the actors' voices:
    // stnwsfx/fapc003a.wav).
    std::vector<uint8_t> data;
    if (const auto it = mediaFiles_.find(upperCase(stem)); it != mediaFiles_.end()) {
        std::ifstream in(it->second, std::ios::binary);
        data.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    } else {
        const CatEntry* entry = nullptr;
        for (const auto& a : archives_)
            if ((entry = a->find("stnwsfx/" + fs::toLower(stem) + ".wav")) && !a->read(*entry, data))
                break;
        if (data.empty()) {
            soundStatus_ = "no " + stem + " in the game folder";
            return nullptr;
        }
    }
    auto sound = std::make_shared<audio::Sound>();
    if (Error err = audio::decodeSound(data, *sound)) {
        soundStatus_ = stem + ": " + err.message;
        return nullptr;
    }
    return sound;
}

// The colony's voices (exe.md#sounds): an officer's report by number,
// in the player's race's recording (FED002c, KLI002a, ...); "attack" its
// "under attack" clip. Each at most once in 600 ms, and none over a clip
// already playing.
void MissionViewer::speak(const std::string& what) {
    static const std::map<std::string, std::array<char, 3>> kLetters = {
        {"001", {'c', 'a', 'b'}}, {"002", {'c', 'a', 'a'}}, {"003", {'b', 'a', 'a'}}, {"004", {'b', 'b', 'a'}},
        {"005", {'b', 'a', 'a'}}, {"006", {'c', 'd', 'd'}}, {"007", {'a', 'b', 'a'}}, {"008", {'a', 'b', 'b'}}};
    if (!mixer_ || (clipPlaying() && what != "attack"))
        return;
    const char side = this->side();
    const int race = side == 'K' ? 1 : side == 'R' ? 2 : 0;
    std::string clip;
    if (what == "attack") {
        // The colony's danger sting (Media/Music) with it, over the map's
        // music.
        const char* sting = side == 'K' ? "klinColonydan" : side == 'R' ? "RomColonyInDangr" : "fedcoldanger";
        if (!mixer_->isPlaying(stingHandle_))
            if (auto sound = loadSound(sting))
                stingHandle_ = mixer_->play(std::move(sound), audio::SoundType::Music, kMusicVolume);
        clip = std::string(side == 'K' ? "k" : side == 'R' ? "r" : "f") + "atckvoc";
    } else if (auto it = kLetters.find(what); it != kLetters.end()) {
        clip = std::string(race == 1 ? "KLI" : race == 2 ? "ROM" : "FED") + what + it->second[size_t(race)];
    }
    if (clip.empty() || clipPlaying() || nowMs_ - spokenAt_[clip] < 600)
        return;
    spokenAt_[clip] = nowMs_;
    if (auto sound = loadSound(clip))
        clipHandle_ = mixer_->play(std::move(sound), audio::SoundType::Speech);
}

// One of the player's vehicles speaking (exe.md#sounds): group 0
// selected, 1 ordered, 2 refusing; in its own voice (one of its type's
// two, picked at random the first time), over no other clip.
void MissionViewer::unitSpeaks(int unit, int group) {
    const Unit* u = world_ ? world_->unit(unit) : nullptr;
    if (!mixer_ || !u || u->slot != playerSlot_ || u->building || clipPlaying())
        return;
    const auto [it, fresh] = play_.unitVoices.try_emplace(unit, 0);
    if (fresh)
        it->second = int(rng_() & 1);
    const std::string clip = unitLine(u->type, group, it->second, int(rng_() & 1));
    if (clip.empty())
        return;
    if (auto sound = loadSound(clip))
        clipHandle_ = mixer_->play(std::move(sound), audio::SoundType::Speech);
}

// A sound from the game's archives played from a spot on the map, as the
// game's positional sounds (exe.md#sounds): its volume (65536 - distance /
// gain) x 127 / 65536 of 127, heard from the camera's spot on the ground,
// panned by how far to the side it lies; none if it would start below 1.
audio::SoundHandle MissionViewer::playAt(const std::string& archive, const std::string& stem, float x, float y,
                                         float gain) {
    if (!mixer_)
        return audio::kNoSound;
    float cx = 0, cy = 0;
    map_.cameraTarget(cx, cy);
    const float dx = x - cx, dy = y - cy, d = std::hypot(dx, dy);
    const int volume = std::clamp(int((65536 - d / gain) * 127 / 65536), 0, 127);
    if (volume < 1)
        return audio::kNoSound;
    auto& cache = archiveSounds_[archive + "/" + stem];
    if (!cache) {
        std::vector<uint8_t> data;
        for (const auto& a : archives_)
            if (fs::toLower(fs::toUtf8(a->path().filename())) == archive)
                if (const CatEntry* e = a->find("stnwsfx/" + stem + ".wav"); e && !a->read(*e, data))
                    break;
        auto sound = std::make_shared<audio::Sound>();
        if (data.empty() || audio::decodeSound(data, *sound))
            return audio::kNoSound;
        cache = std::move(sound);
    }
    const float side = d > 0 ? (dx * std::cos(map_.yaw()) + dy * std::sin(map_.yaw())) / d : 0;
    return mixer_->play(cache, audio::SoundType::Effects, volume * 2, int(std::lround(side * audio::kMaxPan)));
}

// An order given to the selection: its first vehicle takes it, or, if it
// can't get there, refuses.
void MissionViewer::orderSpoken(bool moving) {
    const std::vector<int>& chosen = world_->selection();
    if (chosen.empty())
        return;
    const Unit* u = world_->unit(chosen[0]);
    unitSpeaks(chosen[0], moving && u && !u->moving() ? 2 : 1);
}

bool MissionViewer::clipPlaying() const { return mixer_ && mixer_->isPlaying(clipHandle_); }

void MissionViewer::playClip(int index) {
    stopClip();
    clip_ = index;
    soundStatus_.clear();
    if (clips_.empty())
        return;
    if (!mixer_) {
        soundStatus_ = "no audio output";
        return;
    }
    if (auto sound = loadSound(clips_[size_t(clip_)]))
        clipHandle_ = mixer_->play(std::move(sound), audio::SoundType::Speech);
}

void MissionViewer::stopClip() {
    if (mixer_)
        mixer_->stop(clipHandle_);
    clipHandle_ = audio::kNoSound;
}

void MissionViewer::startMusic() {
    const std::string theme = hasMap_ ? planetTheme(map_.mapArchive()) : std::string();
    if (mixer_ && musicHandle_ != audio::kNoSound && mixer_->isPlaying(musicHandle_) && theme == musicName_)
        return; // already playing it
    stopMusic();
    musicName_.clear();
    if (theme.empty()) {
        soundStatus_ = "no music known for this map";
        return;
    }
    if (!mixer_) {
        soundStatus_ = "no audio output";
        return;
    }
    if (auto sound = loadSound(theme)) {
        musicName_ = theme;
        musicHandle_ = mixer_->play(std::move(sound), audio::SoundType::Music, kMusicVolume, 0, true);
    }
}

void MissionViewer::stopMusic() {
    if (mixer_)
        mixer_->stop(musicHandle_);
    musicHandle_ = audio::kNoSound;
}

} // namespace reluva::newworlds
