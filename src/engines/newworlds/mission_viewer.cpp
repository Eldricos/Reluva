// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/viewers.h"

#include "common/audio/decode.h"
#include "common/font.h"
#include "common/fs.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <fstream>
#include <iterator>

namespace reluva::newworlds {

namespace {

constexpr Color kHeadingColor{255, 220, 90};
constexpr Color kTextColor{230, 230, 230};
constexpr Color kNoteColor{150, 160, 190};
constexpr int kPanelMargin = 12; // round the panel, and inside it
constexpr int kMusicVolume = 160; // under the speech
constexpr float kBriefingTurn = 0.05f / 1000; // radians per millisecond (about 3 degrees a second)

std::string upperCase(std::string s) {
    for (char& c : s)
        c = char(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

const char* sideName(char side) {
    switch (side) {
    case 'K': return "the Klingons";
    case 'R': return "the Romulans";
    default: return "the Federation";
    }
}

} // namespace

MissionViewer::MissionViewer(System& sys, Surface& screen, const Archives& archives, std::vector<Level> levels,
                             audio::Mixer* mixer, const std::filesystem::path& gameDir)
    : sys_(sys), screen_(screen), archives_(archives), missions_(std::move(levels)), mixer_(mixer),
      map_(sys, screen, archives) {
    map_.setLevels(missions_);
    map_.setOverlay([this] { drawBriefing(); });
    map_.setTitleFilter([this](const std::string& mapTitle) {
        mapTitle_ = mapTitle;
        return title(mapTitle);
    });
    fonts_ = loadGameFonts(archives_);
    for (const GameFont& f : fonts_)
        if (f.name == "fnt/fe8pt")
            font_ = &f;

    std::error_code ec;
    if (!gameDir.empty())
        for (std::filesystem::recursive_directory_iterator it(gameDir, ec), end; !ec && it != end; it.increment(ec)) {
            const std::string ext = fs::toLower(it->path().extension().string());
            if (it->is_regular_file(ec) && (ext == ".mp3" || ext == ".wav"))
                mediaFiles_.emplace(upperCase(it->path().stem().string()), it->path());
        }

    // Without any of the missions' maps or text there's nothing to show.
    const bool any = std::any_of(missions_.begin(), missions_.end(), [&](const Level& l) {
        for (const auto& a : archives_) {
            for (const std::string& name : missionMapNames(l.shortName))
                if (a->find(name))
                    return true;
            for (char side : missionSides(l.shortName))
                if (const std::string table = missionTableName(l.shortName, side); !table.empty() && a->find(table))
                    return true;
        }
        return false;
    });
    if (!any)
        missions_.clear();
}

void MissionViewer::activate() {
    if (!shown_) {
        shown_ = true;
        show(0);
        return;
    }
    if (hasMap_)
        map_.activate();
    if (music_)
        startMusic();
    redraw();
    updateTitle();
}

void MissionViewer::deactivate() {
    stopClip();
    if (mixer_)
        mixer_->stop(musicHandle_);
    musicHandle_ = audio::kNoSound;
}

void MissionViewer::show(int index) {
    stopClip();
    soundStatus_.clear();
    current_ = index;
    side_ = 0;
    const Level& level = missions_[size_t(current_)];
    hasMap_ = false;
    mapName_.clear();
    mapNote_.clear();
    mapTitle_.clear();
    const std::vector<std::string> names = missionMapNames(level.shortName);
    for (size_t i = 0; i < names.size() && !hasMap_; ++i)
        if (map_.showMap(names[i])) {
            hasMap_ = true;
            mapName_ = names[i];
            if (i > 0)
                mapNote_ = "mission 12's map, whose world the briefing names";
        }
    if (!hasMap_)
        mapNote_ = "no " + names[0] + " in these archives";
    loadSide();
    if (music_)
        startMusic();
    redraw();
    updateTitle();
}

void MissionViewer::loadSide() {
    stopBriefing();
    stopClip();
    page_ = clip_ = 0;
    pages_.clear();
    clips_.clear();
    status_.clear();
    const Level& level = missions_[size_t(current_)];
    const char side = missionSides(level.shortName)[size_t(side_)];
    tableName_ = missionTableName(level.shortName, side);

    table_ = StringTable{};
    StringTable& table = table_;
    if (tableName_.empty())
        status_ = "no briefing known for this mission";
    else {
        status_ = "no " + tableName_ + " in these archives";
        for (const auto& a : archives_)
            if (const CatEntry* e = a->find(tableName_)) {
                std::vector<uint8_t> data;
                if (Error err = a->read(*e, data))
                    status_ = err.message;
                else if (Error err2 = table.parse(data))
                    status_ = err2.message;
                else
                    status_.clear();
                break;
            }
    }
    // Pages in the table's order: a "~HEADING~" string each, and the plain
    // strings between them (the title and stardate first, the objectives
    // as listed in game last) run together.
    for (const StringTable::String& s : table.strings()) {
        const size_t close = s.text.size() > 1 && s.text[0] == '~' ? s.text.find('~', 1) : std::string::npos;
        if (close != std::string::npos) {
            std::string heading = s.text.substr(1, close - 1);
            std::replace(heading.begin(), heading.end(), '\t', ' ');
            pages_.push_back({std::move(heading), s.text.substr(close + 1), s.id});
            continue;
        }
        if (pages_.empty() || !pages_.back().heading.empty())
            pages_.push_back({{}, {}, s.id});
        Page& page = pages_.back();
        page.text += (page.text.empty() ? "" : "\n") + s.text;
    }

    for (const auto& [stem, path] : mediaFiles_)
        if (fs::toLower(path.extension().string()) == ".mp3" && isMissionClip(stem, level.shortName, side))
            clips_.push_back(stem);
    std::sort(clips_.begin(), clips_.end(), clipBefore);
    script_ = briefingScript(level.shortName, side, table_, clips_);
}

void MissionViewer::handleEvent(const Event& ev) {
    if (missions_.empty())
        return;
    if (ev.type != EventType::KeyDown) {
        if (hasMap_)
            map_.handleEvent(ev);
        return;
    }
    const int n = int(missions_.size());
    const int sides = int(missionSides(missions_[size_t(current_)].shortName).size());
    switch (ev.key) {
    case key::Right: show((current_ + 1) % n); return;
    case key::Left: show((current_ + n - 1) % n); return;
    case key::Down:
    case key::PageDown:
        if (page_ + 1 < int(pages_.size()))
            ++page_, briefing_ = true;
        break;
    case key::Up:
    case key::PageUp:
        if (page_ > 0)
            --page_, briefing_ = true;
        break;
    case 'b': briefing_ = !briefing_; break;
    case key::Return:
        if (running_)
            startStep(step_ + 1);
        else
            startBriefing();
        break;
    case key::Backspace: stopBriefing(); break;
    case 'j':
        if (sides < 2)
            return;
        side_ = (side_ + 1) % sides;
        loadSide();
        break;
    case key::Space:
        if (clipPlaying())
            stopClip();
        else
            playClip(clip_);
        break;
    case 'n':
        if (!clips_.empty())
            playClip((clip_ + 1) % int(clips_.size()));
        break;
    case 'p':
        if (!clips_.empty())
            playClip((clip_ + int(clips_.size()) - 1) % int(clips_.size()));
        break;
    case 'm':
        music_ = !music_;
        soundStatus_.clear();
        if (music_)
            startMusic();
        else
            stopMusic();
        break;
    default:
        if (hasMap_)
            map_.handleEvent(ev);
        return;
    }
    redraw();
    updateTitle();
}

void MissionViewer::update(uint64_t nowMs) {
    if (missions_.empty())
        return;
    nowMs_ = nowMs;
    // The briefing moves on when its step's clip ends, or its time is up.
    if (running_ && (stepEndMs_ ? nowMs >= stepEndMs_ : !clipPlaying()))
        startStep(step_ + 1);
    // The briefing's map views turn slowly about their target.
    if (running_ && stepOnMap_ && nowMs - lastTurnMs_ >= 100) {
        map_.turn(float(nowMs - lastTurnMs_) * kBriefingTurn);
        lastTurnMs_ = nowMs;
    }
    if (mapInView())
        map_.update(nowMs);
    else if (dirty_) {
        if (running_)
            drawBackdrop(script_[size_t(step_)].view);
        else
            drawNoMap();
        drawBriefing();
        dirty_ = false;
    }
    // A clip ending changes the title.
    const bool playing = clipPlaying();
    if (playing != wasPlaying_)
        updateTitle();
    wasPlaying_ = playing;
}

void MissionViewer::redraw() {
    if (mapInView())
        map_.redraw();
    else
        dirty_ = true;
}

int MissionViewer::measure(std::string_view text) const {
    return font_ ? font_->textWidth(text) : int(font::length(text)) * font::kAdvance;
}

int MissionViewer::lineHeight() const { return font_ ? font_->lineHeight : font::kLineHeight; }

void MissionViewer::drawText(int x, int y, std::string_view text, Color color) {
    if (font_)
        font_->drawText(screen_, x, y, text, color);
    else
        font::drawText(screen_, x, y, text, color);
}

void MissionViewer::drawNoMap() {
    for (int y = 0; y < screen_.h; ++y) {
        uint8_t* row = screen_.row(y);
        for (int x = 0; x < screen_.w; ++x, row += 4)
            row[0] = 22, row[1] = 26, row[2] = 38, row[3] = 255;
    }
    drawText(kPanelMargin, kPanelMargin, mapNote_, kNoteColor);
}

void MissionViewer::drawBriefing() {
    if (!briefing_)
        return;
    // Lay the page out in lines, word-wrapped to the panel.
    struct Line {
        std::string text;
        Color color;
    };
    std::vector<Line> lines;
    const int width = screen_.w - 4 * kPanelMargin;
    auto wrap = [&](const std::string& text, Color color) {
        size_t start = 0;
        while (start <= text.size()) {
            const size_t end = std::min(text.find('\n', start), text.size());
            const std::string para = text.substr(start, end - start);
            std::string line;
            size_t i = 0;
            while (i < para.size()) {
                const size_t space = std::min(para.find(' ', i), para.size());
                const std::string word = para.substr(i, space - i);
                const std::string next = line.empty() ? word : line + " " + word;
                if (!line.empty() && measure(next) > width) {
                    lines.push_back({line, color});
                    line = word;
                } else
                    line = next;
                i = space + 1;
            }
            lines.push_back({line, color});
            start = end + 1;
        }
    };
    if (running_) {
        // The step's page, if it has one.
        const uint32_t id = script_[size_t(step_)].page;
        for (const Page& page : pages_)
            if (id && page.id == id) {
                if (!page.heading.empty())
                    wrap(page.heading, kHeadingColor);
                wrap(page.text, kTextColor);
                break;
            }
        if (!lines.empty())
            lines.push_back({"briefing " + std::to_string(step_ + 1) + "/" + std::to_string(script_.size()) +
                                 " (Enter: next; Backspace: stop)",
                             kNoteColor});
    } else if (!status_.empty())
        wrap(status_, kNoteColor);
    else if (!pages_.empty()) {
        const Page& page = pages_[size_t(page_)];
        if (!page.heading.empty())
            wrap(page.heading, kHeadingColor);
        wrap(page.text, kTextColor);
        lines.push_back({"page " + std::to_string(page_ + 1) + "/" + std::to_string(pages_.size()) +
                             " (Up/Down; B hides)",
                         kNoteColor});
    }
    if (lines.empty())
        return;

    // A dark panel along the bottom, as tall as the page (or the screen).
    const int lh = lineHeight();
    const int maxLines = std::max(1, (screen_.h - 4 * kPanelMargin) / lh);
    if (int(lines.size()) > maxLines) {
        lines.resize(size_t(maxLines));
        lines.back() = {"...", kNoteColor};
    }
    const int height = int(lines.size()) * lh + 2 * kPanelMargin;
    const int top = screen_.h - kPanelMargin - height;
    for (int y = std::max(top, 0); y < screen_.h - kPanelMargin; ++y) {
        uint8_t* px = screen_.row(y) + kPanelMargin * 4;
        for (int x = kPanelMargin; x < screen_.w - kPanelMargin; ++x, px += 4) {
            px[0] = uint8_t(px[0] / 4 + 8);
            px[1] = uint8_t(px[1] / 4 + 12);
            px[2] = uint8_t(px[2] / 4 + 28);
        }
    }
    for (size_t i = 0; i < lines.size(); ++i)
        drawText(2 * kPanelMargin, top + kPanelMargin + int(i) * lh, lines[i].text, lines[i].color);
}

std::string MissionViewer::title(const std::string& mapTitle) const {
    if (missions_.empty())
        return {};
    const Level& level = missions_[size_t(current_)];
    const std::vector<char> sides = missionSides(level.shortName);
    std::string t = "New Worlds missions " + std::to_string(current_ + 1) + "/" + std::to_string(missions_.size()) +
                    ": " + level.shortName + " \"" + level.title + "\", " + std::to_string(level.maxPlayers) +
                    (level.maxPlayers == 1 ? " player" : " players") + (level.commentedOut ? " (not listed)" : "");
    if (sides.size() > 1)
        t += std::string(", as ") + sideName(sides[size_t(side_)]) + " (J: side)";
    t += hasMap_ ? "; " + mapName_ : "";
    if (!mapNote_.empty())
        t += " (" + mapNote_ + ")";
    if (clips_.empty())
        t += "; no voice clips";
    else {
        t += "; voice " + std::to_string(clip_ + 1) + "/" + std::to_string(clips_.size()) + " " + clips_[size_t(clip_)];
        t += clipPlaying() ? " [playing]" : " (Space: play)";
    }
    if (running_)
        t += "; briefing step " + std::to_string(step_ + 1) + "/" + std::to_string(script_.size()) +
             (hasRecordedBriefing(level.shortName, sides[size_t(side_)]) ? "" : " (by the general rule)");
    if (music_ && !musicName_.empty())
        t += " [music " + musicName_ + "]";
    if (!soundStatus_.empty())
        t += " [" + soundStatus_ + "]";
    // Then the map's own details: its archive, size and view settings.
    if (const size_t p = mapTitle.find(" ("); hasMap_ && p != std::string::npos)
        t += mapTitle.substr(p);
    return t;
}

void MissionViewer::updateTitle() {
    if (!missions_.empty())
        sys_.setWindowTitle(title(mapTitle_));
}

std::shared_ptr<const audio::Sound> MissionViewer::loadSound(const std::string& stem) {
    const auto it = mediaFiles_.find(upperCase(stem));
    if (it == mediaFiles_.end()) {
        soundStatus_ = "no " + stem + " in the game folder";
        return nullptr;
    }
    std::ifstream in(it->second, std::ios::binary);
    const std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    auto sound = std::make_shared<audio::Sound>();
    if (Error err = audio::decodeSound(data, *sound)) {
        soundStatus_ = stem + ": " + err.message;
        return nullptr;
    }
    return sound;
}

void MissionViewer::startBriefing() {
    if (script_.empty())
        return;
    savedColouring_ = map_.colouring();
    running_ = true;
    briefing_ = true;
    startStep(0);
}

void MissionViewer::stopBriefing() {
    if (!running_)
        return;
    running_ = false;
    stopClip();
    map_.setColouring(savedColouring_);
    map_.resetCamera();
    redraw();
    updateTitle();
}

void MissionViewer::startStep(int step) {
    stopClip();
    if (step >= int(script_.size())) {
        stopBriefing();
        return;
    }
    step_ = step;
    const BriefingStep& s = script_[size_t(step)];
    stepOnMap_ = hasMap_ && (s.view == BriefingView::Ground || s.view == BriefingView::Relief);
    if (stepOnMap_) {
        map_.setColouring(s.view == BriefingView::Relief ? 1 : 0);
        map_.lookAt(s.siteA, s.siteB, s.view == BriefingView::Ground);
        lastTurnMs_ = nowMs_;
    }
    // Its clip, if the game folder has it; else the step is shown a while.
    stepEndMs_ = nowMs_ + 5000;
    const auto it = std::find(clips_.begin(), clips_.end(), s.clip);
    if (!s.clip.empty() && it != clips_.end()) {
        playClip(int(it - clips_.begin()));
        if (clipHandle_ != audio::kNoSound)
            stepEndMs_ = 0;
    }
    redraw();
    updateTitle();
}

bool MissionViewer::mapInView() const { return hasMap_ && (!running_ || stepOnMap_); }

// Stand-ins for the briefing's other views, drawn by Reluva itself: the
// game's galaxy, star system and emblem scenes aren't known from the data.
void MissionViewer::drawBackdrop(BriefingView view) {
    const int w = screen_.w, h = screen_.h;
    for (int y = 0; y < h; ++y) {
        uint8_t* row = screen_.row(y);
        for (int x = 0; x < w; ++x, row += 4)
            row[0] = row[1] = row[2] = 0, row[3] = 255;
    }
    auto plot = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (x >= 0 && y >= 0 && x < w && y < h) {
            uint8_t* p = screen_.row(y) + x * 4;
            p[0] = r, p[1] = g, p[2] = b;
        }
    };
    uint32_t seed = 12345;
    auto rnd = [&] { return (seed = seed * 1664525u + 1013904223u) >> 8; };
    const char side = missionSides(missions_[size_t(current_)].shortName)[size_t(side_)];
    switch (view) {
    case BriefingView::Galaxy:
        for (int i = 0; i < 900; ++i) {
            // A flattened disc of stars with a red and a blue arm.
            const float t = float(rnd() % 1000) / 1000.0f * 6.283f, rr = float(rnd() % 1000) / 1000.0f;
            const int x = w / 2 + int(std::cos(t) * rr * float(w) * 0.32f);
            const int y = h / 2 + int(std::sin(t) * rr * float(h) * 0.16f);
            const uint8_t v = uint8_t(120 + rnd() % 136);
            plot(x, y, v, v, v);
        }
        for (int i = 0; i < 300; ++i) {
            const int x = int(rnd() % unsigned(w / 3)), d = int(rnd() % 9) - 4;
            plot(w / 3 + x, h / 2 - 30 + x / 4 + d, 200, 40, 40);
            plot(w / 3 + x, h / 2 + 20 - x / 5 + d, 60, 80, 220);
        }
        break;
    case BriefingView::StarSystem:
        for (int i = 0; i < 400; ++i)
            plot(int(rnd() % unsigned(w)), int(rnd() % unsigned(h)), 90, 90, 110);
        for (int i = 0; i < 7; ++i) {
            const int cx = int(rnd() % unsigned(w)), cy = int(rnd() % unsigned(h)), r = 2 + int(rnd() % 5);
            const uint8_t cr = uint8_t(rnd() % 256), cg = uint8_t(rnd() % 256), cb = uint8_t(rnd() % 256);
            for (int y = -r; y <= r; ++y)
                for (int x = -r; x <= r; ++x)
                    if (x * x + y * y <= r * r)
                        plot(cx + x, cy + y, cr, cg, cb);
        }
        break;
    case BriefingView::Model:
    case BriefingView::Ground:
    case BriefingView::Relief:
        // A dim red grid, as the model views have (and the map views when
        // there's no map).
        for (int y = 0; y < h; y += 24)
            for (int x = 0; x < w; x += 4)
                plot(x, y, 70, 14, 14);
        for (int x = 0; x < w; x += 24)
            for (int y = 0; y < h; y += 4)
                plot(x, y, 70, 14, 14);
        drawText(kPanelMargin, kPanelMargin, view == BriefingView::Model ? "(a model turning)" : mapNote_, kNoteColor);
        break;
    case BriefingView::Emblem: {
        const char* name = side == 'K' ? "KLINGON EMPIRE" : side == 'R' ? "ROMULAN STAR EMPIRE" : "UNITED FEDERATION OF PLANETS";
        const Color c = side == 'K' ? Color{200, 60, 40} : side == 'R' ? Color{60, 170, 90} : Color{90, 140, 230};
        const int scale = 3, tw = int(font::length(name)) * font::kAdvance * scale;
        font::drawText(screen_, (w - tw) / 2, h / 3, name, c, scale);
        break;
    }
    }
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
