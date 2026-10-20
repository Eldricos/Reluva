// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/view3d.h"
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
    loadBriefingArt();
    textureFiles_ = listTextureFiles(archives_);
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
    stopScript();
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
    startColony();
}

void MissionViewer::startColony() {
    colony_.reset();
    colonySite_ = {};
    if (hasMap_) {
        const Level& level = missions_[size_t(current_)];
        const char side = missionSides(level.shortName)[size_t(side_)];
        colonySite_ = colonySite(level.shortName, side);
        if (colonySite_.a >= 0) {
            ColonyGround ground{map_.blocksWide(), map_.blocksHigh(),
                                [this](int bx, int by) { return map_.depositAt(bx, by); },
                                [this](int a, int b) { return map_.groundAt(a, b); },
                                [this](const std::string& model) { return map_.modelSize(model); }};
            colony_ = std::make_unique<ColonyLayout>(side, std::move(ground));
            if (!colony_->start(colonySite_.a, colonySite_.b))
                colony_.reset();
        }
    }
    showColony();
}

void MissionViewer::showColony() {
    std::vector<MapViewer::Building> buildings;
    if (colony_ && colonyShown_)
        for (const ColonyBuilding& c : colony_->buildings())
            buildings.push_back({c.model, c.a, c.b});
    map_.setBuildings(buildings);
    map_.setPaved(colony_ && colonyShown_ ? colony_->pad() : std::vector<std::pair<int, int>>{});
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
    case 'x': stepScript(); break;
    case 'z': stopScript(); break;
    case 'i': interface_ = !interface_; break;
    case 'c':
        colonyShown_ = !colonyShown_;
        showColony();
        break;
    case 'v':
        if (colony_ && !colony_->buildings().empty())
            map_.lookAt(colony_->buildings()[0].a, colony_->buildings()[0].b, true);
        break;
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
    // The galaxy and models turn slowly too.
    if (running_ && !stepOnMap_ &&
        (script_[size_t(step_)].view == BriefingView::Galaxy || script_[size_t(step_)].view == BriefingView::Model) &&
        nowMs - lastTurnMs_ >= 100) {
        lastTurnMs_ = nowMs;
        dirty_ = true;
    }
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
    if (interface_ && !running_) {
        drawInterface();
        return;
    }
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
    if (run_ && !running_) {
        // The mission's objectives, then its latest messages.
        wrap("OBJECTIVES", kHeadingColor);
        const MissionScript* script = findMissionScript(
            missions_[size_t(current_)].shortName, missionSides(missions_[size_t(current_)].shortName)[size_t(side_)]);
        for (const MissionScript::Objective& o : script->objectives) {
            const MissionRun::State st = run_->objectiveState(o.id);
            if (st == MissionRun::State::Hidden)
                continue;
            const std::string* text = table_.find(o.id);
            wrap(std::string(st == MissionRun::State::Complete ? "[done] " : "[    ] ") +
                     (text ? *text : "objective " + std::to_string(o.id)),
                 st == MissionRun::State::Complete ? kNoteColor : kTextColor);
        }
        if (!log_.empty())
            wrap("COMMUNICATIONS " + std::to_string(log_.size()), kHeadingColor);
        for (size_t i = log_.size() > 4 ? log_.size() - 4 : 0; i < log_.size(); ++i)
            wrap(log_[i].first + ": " + log_[i].second, kTextColor);
        lines.push_back({walkNote_ + " (X: next; Z: stop)", kNoteColor});
    } else if (running_) {
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
    if (colony_)
        t += "; colony at " + std::to_string(colonySite_.a) + ":" + std::to_string(colonySite_.b) + " (" +
             colonySite_.source + "), " + std::to_string(colony_->buildings().size()) + " building" +
             (colony_->buildings().size() == 1 ? "" : "s") + (colonyShown_ ? "" : ", hidden (C)");
    else if (hasMap_)
        t += "; no colony site known";
    if (run_)
        t += "; script: " + walkNote_;
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

// The in-game interface, laid out as the recordings show it on the 640 x
// 480 screen; see docs/formats/newworlds/gui.md#the-in-game-interface.
void MissionViewer::drawInterface() {
    const Level& level = missions_[size_t(current_)];
    const char side = missionSides(level.shortName)[size_t(side_)];
    if (uiSide_ != side) {
        uiSide_ = side;
        ui_ = GameUi{};
        ui_.load(archives_, side);
    }
    if (!ui_.loaded()) {
        drawText(kPanelMargin, kPanelMargin, "no GUI archive for this side", kNoteColor);
        return;
    }
    const int lh = ui_.smallHeight();
    const Color text = ui_.textColor(), bright = ui_.brightColor();
    // The right-hand panels are about 100 pixels wide in the recordings;
    // wide enough here for their longest lines in the font we have.
    const int sideW = std::max({100, ui_.smallWidth("COLONY SIZE = 20/50"), ui_.smallWidth("TRANSALUMIN = 5000"),
                                ui_.smallWidth("TERRAIN AT <000:000>")}) +
                      22;

    // The compass strip along the top: a tick every 5 degrees, a label
    // every 10, N E S W at the quarters, centred on the camera's heading.
    if (hasMap_) {
        const float heading = map_.heading();
        const int cx = screen_.w / 2, span = 120, pxPerDeg = 2;
        for (int d = -span / 2; d <= span / 2; ++d) {
            int deg = int(std::lround(heading)) + d;
            deg = ((deg % 360) + 360) % 360;
            const int x = cx + d * pxPerDeg;
            if (deg % 10 == 0) {
                static const char* kPoints[] = {"N", "E", "S", "W"};
                const std::string label = deg % 90 == 0 ? kPoints[deg / 90]
                                                        : (deg < 100 ? "0" : "") + std::to_string(deg);
                ui_.drawSmall(screen_, x - ui_.smallWidth(label) / 2, 2, label, deg % 90 == 0 ? bright : text);
            } else if (deg % 5 == 0) {
                ui_.drawSmall(screen_, x - 1, 2, "+", text);
            }
        }
    }

    // NEW WORLDS: the colony panel, with the stock every recording started
    // with (the economy isn't simulated).
    {
        const int x = screen_.w - sideW, y = 0, w = sideW, h = 24 + 7 * lh + 8;
        ui_.drawPanel(screen_, x, y, w, h, "NEW WORLDS");
        static const std::pair<const char*, int> kStock[] = {{"DILITHIUM", 500},   {"POLYCOMP", 1000},
                                                             {"THERMOCONC", 2300}, {"TRANSALUMIN", 500},
                                                             {"NITRIUM", 4000},    {"DURANIUM", 5000}};
        int ty = y + 16;
        ui_.drawSmall(screen_, x + 10, ty, "COLONY SIZE = 20/50", bright);
        for (const auto& [name, n] : kStock) {
            ty += lh;
            ui_.drawSmall(screen_, x + 10, ty, std::string(name) + " = " + std::to_string(n), text);
        }
    }

    // TRICORDER: the map small, and the camera's position.
    {
        const int x = screen_.w - sideW, w = sideW, h = 102, y = screen_.h - h;
        ui_.drawPanel(screen_, x, y, w, h, "TRICORDER");
        if (hasMap_) {
            map_.drawMinimap(screen_, x + 12, y + 14, w - 24, h - 30 - lh);
            int a = 0, b = 0;
            map_.cameraPosition(a, b);
            auto pad = [](int v) {
                std::string s = std::to_string(v);
                return std::string(s.size() < 3 ? 3 - s.size() : 0, '0') + s;
            };
            ui_.drawSmall(screen_, x + 10, y + h - 12 - lh, "TERRAIN AT <" + pad(a) + ":" + pad(b) + ">", text);
        }
    }

    // COMMUNICATIONS: the script's latest message, with its sender's portrait.
    {
        const int w = 220, h = 70, x = (screen_.w - sideW - w) / 2 + 10, y = screen_.h - h;
        ui_.drawPanel(screen_, x, y, w, h, "COMMUNICATIONS");
        if (log_.empty()) {
            ui_.drawSmall(screen_, x + 60, y + 16, "NO MESSAGES ON FILE.", text);
        } else {
            const auto& [sender, message] = log_.back();
            const std::string natives = level.shortName == "C9" ? "orion" : "taubat";
            if (const Texture* p = ui_.portraitFor(sender, natives))
                ui_.drawPicture(screen_, *p, x + 10, y + 13, 44, 41);
            std::string up = sender + ":";
            std::transform(up.begin(), up.end(), up.begin(), [](unsigned char c) { return char(std::toupper(c)); });
            int ty = y + 14;
            ui_.drawSmall(screen_, x + 58, ty, up, text);
            for (const std::string& l : ui_.wrapSmall(message, w - 70)) {
                ty += lh;
                if (ty > y + h - 12 - lh)
                    break;
                ui_.drawSmall(screen_, x + 58, ty, l, text);
            }
            const std::string counter = std::to_string(log_.size()) + "/" + std::to_string(log_.size());
            ui_.drawSmall(screen_, x + w - 12 - ui_.smallWidth(counter), y + h - 11 - lh, counter, text);
        }
    }

    // COMMAND: the objectives, as the recordings' panel lists them.
    if (run_) {
        const MissionScript* script = findMissionScript(level.shortName, side);
        std::vector<std::pair<std::string, Color>> lines;
        const int w = 260;
        for (const MissionScript::Objective& o : script->objectives) {
            const MissionRun::State st = run_->objectiveState(o.id);
            if (st == MissionRun::State::Hidden)
                continue;
            const std::string* t = table_.find(o.id);
            const std::string s = "MISSION OBJECTIVE: " + (t ? *t : std::to_string(o.id)) +
                                  (st == MissionRun::State::Complete ? " - COMPLETE" : " - INCOMPLETE");
            for (const std::string& l : ui_.wrapSmall(s, w - 24))
                lines.push_back({l, st == MissionRun::State::Complete ? bright : text});
        }
        const int h = 30 + int(lines.size()) * lh;
        ui_.drawPanel(screen_, 0, 0, w, h, "COMMAND");
        for (size_t i = 0; i < lines.size(); ++i)
            ui_.drawSmall(screen_, 12, 15 + int(i) * lh, lines[i].first, lines[i].second);
    }
}

void MissionViewer::stepScript() {
    const Level& level = missions_[size_t(current_)];
    const char side = missionSides(level.shortName)[size_t(side_)];
    if (!run_) {
        const MissionScript* script = findMissionScript(level.shortName, side);
        if (!script) {
            soundStatus_ = "no script for this mission yet";
            updateTitle();
            return;
        }
        stopBriefing();
        run_ = std::make_unique<MissionRun>(*script, findMissionScript("generic", 'F'));
        walk_ = run_->walkthrough();
        walkPos_ = 0;
        log_.clear();
        walkNote_ = "script loaded";
        briefing_ = true;
        redraw();
        updateTitle();
        return;
    }
    if (run_->finished())
        return;
    std::vector<MissionRun::Output> out;
    if (walkPos_ < walk_.size()) {
        const ScriptEvent& e = walk_[walkPos_++];
        walkNote_ = "event " + std::to_string(walkPos_) + "/" + std::to_string(walk_.size()) + ": " + e.kind;
        for (const std::string& a : e.args)
            walkNote_ += " " + a;
        out = run_->fire(e);
        // The colony grows as the script builds (on the missions page only:
        // the game's own building is the player's).
        if (e.kind == "built" && !e.args.empty() && colony_) {
            if (colony_->add(e.args[0]))
                showColony();
            else
                walkNote_ += " (not placed)";
        }
    } else if (run_->nextTimer() >= 0) {
        walkNote_ = "time passes (" + std::to_string(int(run_->nextTimer())) + " s)";
        out = run_->advance(run_->nextTimer());
    } else {
        walkNote_ = "nothing more scripted";
    }
    for (const MissionRun::Output& o : out) {
        if (o.kind == MissionRun::Output::Message)
            log_.push_back({o.sender, o.text});
        else if (o.kind == MissionRun::Output::Won)
            walkNote_ += "; mission won";
        else if (o.kind == MissionRun::Output::Lost)
            walkNote_ += "; mission lost";
    }
    redraw();
    updateTitle();
}

void MissionViewer::stopScript() {
    if (!run_)
        return;
    run_.reset();
    walk_.clear();
    log_.clear();
    startColony();
    redraw();
    updateTitle();
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

namespace {

// A texture from whichever archive has it; false if none does or it doesn't decode.
bool loadTexture(const Archives& archives, const std::string& name, Texture& out) {
    for (const auto& a : archives)
        if (const CatEntry* e = a->find(name)) {
            std::vector<uint8_t> data;
            return !a->read(*e, data) && !decodeTexture(data, out);
        }
    return false;
}

} // namespace

void MissionViewer::loadBriefingArt() {
    // The emblem screens are six tiles, three across and two down:
    // 256 + 256 + 128 by 256 + 224, the 640 x 480 screen.
    const std::pair<char, const char*> sets[] = {{'F', "lbp/gui/ufp00"}, {'K', "lbp/gui/kliemp0"}, {'R', "lbp/gui/romemp0"}};
    for (const auto& [side, prefix] : sets) {
        Texture screen;
        screen.width = 640;
        screen.height = 480;
        screen.rgba.assign(size_t(640) * 480 * 4, 0);
        bool ok = true;
        for (int t = 0; t < 6 && ok; ++t) {
            Texture tile;
            ok = loadTexture(archives_, prefix + std::to_string(t + 1) + ".lbp", tile);
            const int x0 = (t % 3) * 256, y0 = (t / 3) * 256;
            for (int y = 0; ok && y < tile.height && y0 + y < 480; ++y)
                for (int x = 0; x < tile.width && x0 + x < 640; ++x)
                    std::copy_n(&tile.rgba[(size_t(y) * size_t(tile.width) + size_t(x)) * 4], 4,
                                &screen.rgba[(size_t(y0 + y) * 640 + size_t(x0 + x)) * 4]);
        }
        if (ok)
            emblems_[side] = std::move(screen);
    }
    // The galaxy: a slab of star field round three flat layers (blue arm,
    // red ribbon, stars), numbering tabras01-04 in name order.
    for (const auto& a : archives_)
        if (const CatEntry* e = a->find("geo/solsys/tabras.geo")) {
            std::vector<uint8_t> data;
            if (a->read(*e, data) || parseGeo(data, galaxy_))
                galaxy_ = GeoModel{};
            break;
        }
    for (int t = 1; t <= 4 && !galaxy_.faces.empty(); ++t) {
        Texture tex;
        loadTexture(archives_, "lbp/txr/solsys/tabras0" + std::to_string(t) + ".mp0", tex);
        galaxyTextures_.push_back(std::move(tex));
    }
}

bool MissionViewer::drawEmblem(char side) {
    const auto it = emblems_.find(side);
    if (it == emblems_.end())
        return false;
    const Texture& e = it->second;
    for (int y = 0; y < screen_.h; ++y) {
        uint8_t* row = screen_.row(y);
        const int sy = y * e.height / screen_.h;
        for (int x = 0; x < screen_.w; ++x, row += 4) {
            const uint8_t* p = &e.rgba[(size_t(sy) * size_t(e.width) + size_t(x * e.width / screen_.w)) * 4];
            row[0] = p[0], row[1] = p[1], row[2] = p[2], row[3] = 255;
        }
    }
    return true;
}

bool MissionViewer::drawGalaxy() {
    if (galaxy_.faces.empty())
        return false;
    for (int y = 0; y < screen_.h; ++y) {
        uint8_t* row = screen_.row(y);
        for (int x = 0; x < screen_.w; ++x, row += 4)
            row[0] = row[1] = row[2] = 0, row[3] = 255;
    }
    // Seen from above at an angle, turning; each layer adds its light (its
    // alpha layer is its brightness), so there's no depth to sort.
    const float yaw = float(nowMs_ % 120000) / 120000.0f * 6.2832f;
    const OrbitCamera cam({0, 0, 0}, yaw, 0.45f, 16000.0f, screen_.w, screen_.h);
    raster_.begin(screen_);
    for (const GeoModel::Face& face : galaxy_.faces) {
        if (face.corners.size() < 3 || face.part >= galaxy_.parts.size())
            continue;
        const uint32_t ti = face.corners.back().extra[4];
        if (ti >= galaxyTextures_.size() || galaxyTextures_[ti].rgba.empty())
            continue;
        const Texture& tex = galaxyTextures_[ti];
        const auto& verts = galaxy_.parts[face.part].vertices;
        std::vector<ViewVertex> poly;
        for (const GeoModel::Corner& c : face.corners) {
            if (c.vertex >= verts.size())
                break;
            const GeoModel::Vec3& p = verts[c.vertex];
            poly.push_back(cam.toView({p.x, p.y, p.z}, c.u, c.v));
        }
        if (poly.size() != face.corners.size())
            continue;
        poly = clipNear(poly, cam.nearZ);
        if (poly.size() < 3)
            continue;
        std::vector<ScreenVertex> s;
        for (const ViewVertex& v : poly)
            s.push_back(cam.project(v));
        auto shade = [&](float u, float v, uint8_t* out) {
            const int tx = std::clamp(int(u * float(tex.width)), 0, tex.width - 1);
            const int ty = std::clamp(int(v * float(tex.height)), 0, tex.height - 1);
            const size_t i = size_t(ty) * size_t(tex.width) + size_t(tx);
            const int a = tex.alpha.empty() ? 255 : tex.alpha[i];
            for (int k = 0; k < 3; ++k)
                out[k] = uint8_t(std::min(255, out[k] + tex.rgba[i * 4 + size_t(k)] * a / 255));
            return false;
        };
        for (size_t i = 1; i + 1 < s.size(); ++i)
            raster_.drawTriangle(s[0], s[i], s[i + 1], shade);
    }
    return true;
}

bool MissionViewer::drawModel(const std::string& name) {
    if (name.empty())
        return false;
    auto [it, added] = models_.try_emplace(name);
    BriefingModel& m = it->second;
    if (added) {
        for (const auto& a : archives_)
            if (const CatEntry* e = a->find(name)) {
                std::vector<uint8_t> data;
                m.ok = !a->read(*e, data) && !parseGeo(data, m.geo);
                break;
            }
        if (m.ok) {
            // Its textures as the model viewer finds them (model_textures.h).
            size_t needed = 0;
            for (const auto& face : m.geo.faces)
                needed = std::max<size_t>(needed, face.corners.back().extra[4] + 1);
            std::vector<const TextureFile*> slots = findModelTextures(name, textureFiles_, needed);
            addDoorTextures(m.geo, name, textureFiles_, slots);
            addPadTextures(m.geo, name, textureFiles_, slots);
            for (const TextureFile* t : slots) {
                Texture tex;
                std::vector<uint8_t> data;
                if (t) {
                    CatArchive& cat = *archives_[t->ref.archive];
                    if (cat.read(cat.entries()[t->ref.entry], data) || decodeTexture(data, tex))
                        tex = Texture{};
                }
                m.textures.push_back(std::move(tex));
            }
        }
    }
    if (!m.ok)
        return false;

    // The grid, as behind the recordings' model views.
    const int w = screen_.w, h = screen_.h;
    for (int y = 0; y < h; ++y) {
        uint8_t* row = screen_.row(y);
        for (int x = 0; x < w; ++x, row += 4) {
            const bool line = (y % 24 == 0 && x % 4 == 0) || (x % 24 == 0 && y % 4 == 0);
            row[0] = line ? 70 : 0, row[1] = line ? 14 : 0, row[2] = line ? 14 : 0, row[3] = 255;
        }
    }
    // Framed by its size, turning once a minute, lit from above.
    Vec lo{1e30f, 1e30f, 1e30f}, hi{-1e30f, -1e30f, -1e30f};
    for (const auto& part : m.geo.parts)
        for (const auto& v : part.vertices) {
            lo = {std::min(lo.x, v.x), std::min(lo.y, v.y), std::min(lo.z, v.z)};
            hi = {std::max(hi.x, v.x), std::max(hi.y, v.y), std::max(hi.z, v.z)};
        }
    if (lo.x > hi.x)
        return false;
    const Vec centre = (lo + hi) * 0.5f;
    const float radius = std::max(1.0f, std::sqrt(dot(hi - lo, hi - lo)) * 0.5f);
    const float yaw = float(nowMs_ % 60000) / 60000.0f * 6.2832f;
    const OrbitCamera cam(centre, yaw, 0.35f, radius * 3.0f, w, h);
    const Vec light = normalize({0.4f, -0.3f, 1.0f});
    raster_.begin(screen_);
    for (const GeoModel::Face& face : m.geo.faces) {
        if (face.corners.size() < 3 || face.part >= m.geo.parts.size())
            continue;
        const auto& verts = m.geo.parts[face.part].vertices;
        std::vector<Vec> world;
        for (const GeoModel::Corner& c : face.corners)
            if (c.vertex < verts.size())
                world.push_back({verts[c.vertex].x, verts[c.vertex].y, verts[c.vertex].z});
        if (world.size() != face.corners.size())
            continue;
        const Vec n = normalize(cross(world[1] - world[0], world[2] - world[0]));
        const float lit = 0.45f + 0.55f * std::fabs(dot(n, light));
        std::vector<ViewVertex> poly;
        for (size_t i = 0; i < world.size(); ++i)
            poly.push_back(cam.toView(world[i], face.corners[i].u, face.corners[i].v));
        poly = clipNear(poly, cam.nearZ);
        if (poly.size() < 3)
            continue;
        std::vector<ScreenVertex> s;
        for (const ViewVertex& v : poly)
            s.push_back(cam.project(v));
        const uint32_t ti = face.corners.back().extra[4];
        const Texture* tex = ti < m.textures.size() && !m.textures[ti].rgba.empty() ? &m.textures[ti] : nullptr;
        auto shade = [&](float u, float v, uint8_t* out) {
            uint8_t c[3] = {150, 150, 150};
            if (tex) {
                const int tx = std::clamp(int(u * float(tex->width)), 0, tex->width - 1);
                const int ty = std::clamp(int(v * float(tex->height)), 0, tex->height - 1);
                const uint8_t* p = &tex->rgba[(size_t(ty) * size_t(tex->width) + size_t(tx)) * 4];
                if (p[3] == 0)
                    return false; // colour-keyed: see-through
                c[0] = p[0], c[1] = p[1], c[2] = p[2];
            }
            for (int k = 0; k < 3; ++k)
                out[k] = uint8_t(float(c[k]) * lit);
            return true;
        };
        for (size_t i = 1; i + 1 < s.size(); ++i)
            raster_.drawTriangle(s[0], s[i], s[i + 1], shade);
    }
    return true;
}

// The briefing's views without a map: the game's galaxy and emblem where
// the archives have them; else (and for the star system and model views,
// whose scenes aren't known) Reluva's own stand-ins.
void MissionViewer::drawBackdrop(BriefingView view) {
    const char side = missionSides(missions_[size_t(current_)].shortName)[size_t(side_)];
    if ((view == BriefingView::Galaxy && drawGalaxy()) || (view == BriefingView::Emblem && drawEmblem(side)) ||
        (view == BriefingView::Model && drawModel(script_[size_t(step_)].model)))
        return;
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
