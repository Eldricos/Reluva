// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/mission_ui.h"
#include "engines/newworlds/mission_surface.h"
#include "engines/newworlds/start_data.h"
#include "engines/newworlds/viewers.h"

#include "common/font.h"
#include "common/fs.h"

#include <algorithm>
#include <cstdio>

namespace reluva::newworlds {

namespace {

constexpr float kEdgeRamp = 80, kEdgeTop = 6400; // edge scrolling, units a tick

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
            const std::string ext = fs::toLower(fs::toUtf8(it->path().extension()));
            if (it->is_regular_file(ec) && (ext == ".mp3" || ext == ".wav"))
                mediaFiles_.emplace(upperCase(fs::toUtf8(it->path().stem())), it->path());
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
    map_.forgetTitle(); // (another page's was up)
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
        mixer_->stop(musicHandle_), mixer_->stop(buildingClipHandle_);
    musicHandle_ = audio::kNoSound;
}

void MissionViewer::show(int index) {
    stopPlay();
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
            if (!level.shortName.empty() && (level.shortName[0] == 'C' || level.shortName[0] == 'c'))
                mapNote_ = "the map its recording fits";
            else if (i > 0)
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
    stopPlay();
    stopScript();
    stopBriefing();
    stopClip();
    page_ = clip_ = 0;
    pages_.clear();
    clips_.clear();
    status_.clear();
    const Level& level = missions_[size_t(current_)];
    const char side = this->side();
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
        if (fs::toLower(fs::toUtf8(path.extension())) == ".mp3" && isMissionClip(stem, level.shortName, side))
            clips_.push_back(stem);
    std::sort(clips_.begin(), clips_.end(), clipBefore);
    script_ = briefingScript(level.shortName, side, table_, clips_);
    // Clips a step names from elsewhere in the game folder (the tutorials'
    // own, in MISC) join the mission's.
    for (const BriefingStep& st : script_)
        if (!st.clip.empty() && mediaFiles_.count(st.clip) &&
            std::find(clips_.begin(), clips_.end(), st.clip) == clips_.end())
            clips_.push_back(st.clip);
    loadStarSystem();
    startColony();
}

void MissionViewer::startColony() {
    colony_.reset();
    colonySite_ = {};
    if (hasMap_) {
        const Level& level = missions_[size_t(current_)];
        const char side = this->side();
        ColonyGround ground{map_.blocksWide(), map_.blocksHigh(),
                            [this](int bx, int by) { return map_.depositAt(bx, by); },
                            [this](int a, int b) { return map_.groundAt(a, b); },
                            [this](const std::string& model) { return map_.modelSize(model); }};
        // The colony the mission places (the executable's), its hub first;
        // else one round the site the recordings give.
        std::vector<ColonyBuilding> placed;
        for (const StartPlacement& p : startPlacements(level.shortName, side))
            if (const ObjectType* t = objectType(p.type); t && t->building && *t->model) {
                const ColonyBuilding c{t->name, t->model, p.a, p.b};
                if (std::string_view(t->name) == "colony-hub")
                    placed.insert(placed.begin(), c);
                else
                    placed.push_back(c);
            }
        if (!placed.empty() && placed[0].name == "colony-hub") {
            colonySite_ = {placed[0].a, placed[0].b, "the mission's own placements (exe.md)"};
            colony_ = std::make_unique<ColonyLayout>(side, std::move(ground));
            colony_->startWith(placed);
        } else {
            colonySite_ = colonySite(level.shortName, side);
            if (colonySite_.a >= 0) {
                colony_ = std::make_unique<ColonyLayout>(side, std::move(ground));
                if (!colony_->start(colonySite_.a, colonySite_.b))
                    colony_.reset();
            }
        }
    }
    showColony();
}

void MissionViewer::showColony() {
    std::vector<MapViewer::Building> buildings;
    // In play, each has the side's construction bee to work on it while
    // it goes up.
    std::string bee;
    if (economy_)
        bee = beeModels(side()).construction;
    if (colony_ && colonyShown_)
        for (size_t i = 0; i < colony_->buildings().size(); ++i) {
            const ColonyBuilding& c = colony_->buildings()[i];
            // A destroyed one is left as its flattened wreck; a site
            // cancelled is gone (kept in the list, the progress shown by
            // its place in it).
            const int object = i < colonyObjects().size() ? colonyObjects()[i] : 0;
            buildings.push_back({c.model, c.a, c.b, 0, object == -2 ? -1.0f : object < 0 ? 0.0f : 1.0f,
                                 object < 0 ? std::string() : bee, 0});
        }
    // The other sides' buildings (an AI colony's sites going up with its
    // construction bee).
    shownBuildings_ = 0;
    play_.otherBuildings.clear();
    if (world_)
        for (const Unit& u : world_->units())
            if (u.building && u.slot != playerSlot_ && !u.kind.model.empty() && visible(u)) {
                const std::string otherBee = u.built >= 1 ? std::string() : beeModels(u.side).construction;
                buildings.push_back({u.kind.model, int(u.y / MapViewer::kBlockUnits * 8),
                                     int(u.x / MapViewer::kBlockUnits * 8), 0, u.built, otherBee});
                play_.otherBuildings.push_back({u.id, u.built});
                ++shownBuildings_;
            }
    buildingsShown_ = buildings.size();
    map_.setBuildings(buildings);
    showProgress();
    map_.setPaved(colony_ && colonyShown_ ? colony_->pad() : std::vector<std::pair<int, int>>{});
}

bool MissionViewer::hubPosition(float& x, float& y) const {
    if (!colony_ || colony_->buildings().empty())
        return false;
    const ColonyBuilding& hub = colony_->buildings()[0];
    x = float(hub.b) / 8 * MapViewer::kBlockUnits;
    y = float(hub.a) / 8 * MapViewer::kBlockUnits;
    return true;
}

// Play: the colony's starting units by the hub (four of the side's ground
// scouts, as F1 and R1 start with four vehicles of one kind), the game's
// camera over it, the interface up.
void MissionViewer::startPlay() {
    // Play's Tricorders are new ones (the game makes the mission's world
    // afresh, its Tricorders with it): whatever the briefing's steps set,
    // the ring hatch, Relief, X2.
    map_.setTricorderScheme(0);
    tricorderZoom_ = 1;
    tricorderView_ = 1;
    if (!hasMap_ || running_)
        return;
    const Level& level = missions_[size_t(current_)];
    // The colony as it starts (the mission's own, else round its site),
    // and the mission played on the shared simulation (mission_sim.h).
    colonyShown_ = true;
    startColony();
    ColonyMap ground{[this](int bx, int by) { return map_.depositAt(bx, by); },
                     [this](int bx, int by) { return map_.depositAmount(bx, by); }};
    carryIn_ = carried_;
    sim_ = std::make_unique<MissionSim>(level.shortName, side(), map_.terrain(), ground, map_.blocksWide(),
                                        map_.blocksHigh(),
                                        colony_ ? colony_->buildings() : std::vector<ColonyBuilding>{}, carryIn_);
    sim_->useModels(archives_);
    world_ = &sim_->world();
    economy_ = &sim_->economy();
    ai_ = &sim_->ai();
    fieldWork_ = &sim_->fieldWork();
    research_ = &sim_->research();
    effects_ = &sim_->effects();
    fog_ = &sim_->fog();
    pageRun_.reset();
    run_ = sim_->run();
    playerSlot_ = sim_->playerSlot();
    resetPlayUi();
    walk_.clear();
    // What the script said as the mission began.
    for (const MissionSim::Said& said : sim_->takeSaid())
        deliver(said.fired, said.out);
    float hx = map_.worldWidth() / 2, hy = map_.worldHeight() / 2;
    hubPosition(hx, hy);
    playing_ = true;
    interface_ = true;
    briefing_ = false;
    lastTickMs_ = nowMs_;
    map_.centreOn(hx, hy);
    map_.setView(MapViewer::View::BirdsEye);
    showColony();
    showUnits();
    redraw();
    updateTitle();
}

const std::vector<int>& MissionViewer::colonyObjects() const {
    static const std::vector<int> kNone;
    return sim_ ? sim_->colonyObjects() : kNone;
}

const std::vector<int>& MissionViewer::structureIds() const {
    static const std::vector<int> kNone;
    return sim_ ? sim_->structureIds() : kNone;
}

const std::map<std::string, int>& MissionViewer::beesHoused() const {
    static const std::map<std::string, int> kNone;
    return sim_ ? sim_->beesHoused() : kNone;
}

// What play's panels show and what they're waiting on, as a mission
// starts: nothing open, picked or asked, so none of it outlives its play;
// the script's messages and note with it.
void MissionViewer::resetPlayUi() {
    play_ = PlayUi{};
    log_.clear();
    walkNote_.clear();
}

void MissionViewer::stopPlay() {
    if (!playing_)
        return;
    playing_ = false;
    // The capacity the player's colony has as the mission ends, won, lost
    // or left, is the next one's (the game never resets it), with what the
    // debriefing's scene adds: the mission's starting housing once more.
    if (economy_)
        carried_ = economy_->debriefed();
    map_.stopFollowing();
    if (mixer_)
        mixer_->stop(stingHandle_), mixer_->stop(buildingClipHandle_);
    world_ = nullptr, economy_ = nullptr, ai_ = nullptr, fieldWork_ = nullptr;
    research_ = nullptr, effects_ = nullptr, fog_ = nullptr, run_ = nullptr;
    sim_.reset();
    resetPlayUi();
    map_.setGhost({}, 0, 0, false);
    map_.setUnits({});
    map_.unlock();
    // The mission's colony as it starts again, not the one played (the
    // player's buildings and the other sides', with no world behind them).
    startColony();
    redraw();
    updateTitle();
}

void MissionViewer::handleEvent(const Event& ev) {
    if (missions_.empty())
        return;
    // A mission's run: CONTINUE (or Return) after the report ends it;
    // Escape leaves it at any stage.
    if (flow_ != Flow::None) {
        const bool key = ev.type == EventType::KeyDown;
        if (key && ev.key == key::Escape && !(flow_ == Flow::Playing && playing_)) {
            endFlow();
            return;
        }
        if (flow_ == Flow::Report) {
            const bool onContinue = ev.type == EventType::MouseDown && ev.button == MouseButton::Left &&
                                    continue_.contains(ev.mouseX, ev.mouseY);
            if (onContinue || (key && ev.key == key::Return))
                endFlow();
            return;
        }
        if (flowScreen())
            return;
    }
    if (playing_ && playEvent(ev))
        return;
    // A run's briefing (or play) is its mission's, as whom it started:
    // the keys that pick another mission or side, or start another play,
    // wait until it's over.
    if (flow_ != Flow::None && ev.type == EventType::KeyDown &&
        (ev.key == key::Left || ev.key == key::Right || ev.key == 'j' || ev.key == 'x' || ev.key == 'z' ||
         ev.key == 'c' || ev.key == 'g' || ev.key == key::F1 + 2))
        return;
    if (ev.type == EventType::KeyDown && ev.key == 'g') {
        startFlow();
        return;
    }
    if (ev.type == EventType::KeyDown && ev.key == key::F1 + 2) {
        startPlay();
        return;
    }
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
    case 'k': setCadetMode(!cadetMode()); break; // the next mission played
    case 'u': // the carried capacity back to a fresh session's
        if (!playing_)
            carried_ = {};
        break;
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
    if (running_ && (stepEndMs_ ? nowMs >= stepEndMs_ : !clipPlaying() && nowMs >= stepMinimumMs_))
        startStep(step_ + 1);
    // A briefing scene's unit flies its path (from when the camera first
    // follows it), the camera with it.
    if (running_ && stepOnMap_ && script_[size_t(step_)].scene >= 0)
        flyScene(script_[size_t(step_)].scene, nowMs);
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
    // The mission's run (G): each stage after its time.
    {
        if (flowStartMs_ == kFlowUnstarted)
            flowStartMs_ = nowMs;
        const uint64_t since = nowMs - flowStartMs_;
        if (flow_ == Flow::Loading && since >= kLoadingMs) {
            if (!script_.empty()) {
                flow_ = Flow::Briefing;
                startBriefing();
            } else {
                flow_ = Flow::Starting;
                flowStartMs_ = nowMs;
            }
        } else if (flow_ == Flow::Starting && since >= kStartingMs) {
            flow_ = Flow::Playing;
            startPlay();
        } else if (flow_ == Flow::Debriefing && since >= kLoadingMs) {
            flow_ = Flow::Report;
        }
        if (flowScreen())
            dirty_ = true;
    }
    if (playing_ && sim_ && flow_ != Flow::Debriefing && flow_ != Flow::Report) {
        const double dt = play_.paused || unfocused_ ? 0.0 : std::min(double(nowMs - lastTickMs_) / 1000.0, 0.25);
        lastTickMs_ = nowMs;
        // The mission moves on (mission_sim.h); the screen shows what it
        // did and what its script said. (Only a whole tick moves anything:
        // the map is drawn anew when what it shows changed, the panels
        // when the mission moved on.)
        const double clock = sim_->clock();
        sim_->advance(dt, [this] { noteBees(); });
        const bool advanced = sim_->clock() != clock;
        // The pointer on the screen's outermost pixels scrolls the
        // isometric cameras that way, speeding up 80 a tick to 6400.
        edgeClock_ += dt * 20;
        for (; edgeClock_ >= 1; edgeClock_ -= 1) {
            const int ex = mouseX_ <= 0 ? -1 : mouseX_ >= screen_.w - 1 ? 1 : 0;
            const int ey = mouseY_ <= 0 ? 1 : mouseY_ >= screen_.h - 1 ? -1 : 0;
            if ((ex || ey) && map_.view() != MapViewer::View::FirstPerson && mouseSeen_) {
                edgeSpeed_ = std::min(kEdgeTop, edgeSpeed_ + kEdgeRamp);
                map_.pan(float(ex) * edgeSpeed_, float(ey) * edgeSpeed_);
            } else if ((ex || ey) && mouseSeen_ && play_.followed == 0) {
                // F1's free camera (exe.md#cameras): the side edges turn
                // it, the top and bottom raise and lower it.
                if (ex)
                    map_.gameKey(ex < 0 ? key::Kp0 + 4 : key::Kp0 + 6, mouseX_, mouseY_);
                if (ey && edgeTicks_++ % 4 == 0)
                    map_.gameKey(ey > 0 ? key::Kp0 + 7 : key::Kp0 + 1, mouseX_, mouseY_);
            } else {
                edgeSpeed_ = 0;
            }
        }
        // What the player sees of the other sides follows the fog.
        std::set<int> others;
        for (const Unit& u : world_->units())
            if (u.building && u.slot != playerSlot_ && visible(u))
                others.insert(u.id);
        if (others != play_.shownOthers) {
            play_.shownOthers = std::move(others);
            showColony();
        }
        // What moves is drawn on its way between ticks (drawnAt; the bees
        // from where noteBees found them as the last tick began), so as
        // time passes; under the full-screen Tricorder only as ticks run.
        if (advanced || (dt > 0 && !play_.tricorderFull))
            showUnits();
        if (advanced)
            redraw();
        combatStep();
        showProgress();
        for (const MissionSim::Said& said : sim_->takeSaid())
            deliver(said.fired, said.out);
        for (const std::string& v : sim_->takeVoices())
            speak(v);
        // F1 following an object: the camera on it, till it's gone.
        if (map_.following()) {
            const Unit* f = world_->unit(play_.followed);
            if (!f || f->hp <= 0 || map_.view() != MapViewer::View::FirstPerson) {
                map_.stopFollowing();
            } else {
                const Pose at = drawnAt(*f);
                map_.follow(at.x, at.y, at.heading, f->kind.model);
            }
        }
        // A new selection: its first vehicle answers; MULTISELECTION starts
        // at its first page, nothing ticked.
        if (world_->selection() != play_.spokenSelection) {
            play_.spokenSelection = world_->selection();
            play_.multiTicked.clear();
            play_.multiPage = 0;
            if (!play_.spokenSelection.empty())
                unitSpeaks(play_.spokenSelection[0], 0);
        }
        updateGhost(); // the camera may have moved under the pointer
        if (economy_ && nowMs / 1000 != (nowMs - uint64_t(dt * 1000)) / 1000)
            redraw(), updateTitle(); // the panels' figures (and the title's), once a second
        // The Tricorder's blips blink; a menu item clicked flashes.
        if (nowMs / 500 != blinkPhase_ || nowMs - play_.menuFlash.second < 400)
            blinkPhase_ = nowMs / 500, redraw();
    }
    // The full-screen Tricorder covers the map: it isn't drawn under it.
    map_.holdScene(playing_ && play_.tricorderFull);
    if (mapInView())
        map_.update(nowMs);
    else if (dirty_ && flowScreen()) {
        drawFlow();
        dirty_ = false;
    } else if (dirty_) {
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

// In play the map's scene changes mark it themselves (the units, the
// camera, the buildings): what play draws over it is all that's redrawn.
void MissionViewer::redraw() {
    if (mapInView() && playing_)
        map_.redrawOverlay();
    else if (mapInView())
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
    fillScreen(screen_, Color{22, 26, 38});
    drawText(kPanelMargin, kPanelMargin, mapNote_, kNoteColor);
}

void MissionViewer::drawBriefing() {
    if (playing_) {
        drawCombat();
        if (interface_)
            drawInterface();
        else
            play_.buttons.clear(), play_.panels.clear(); // (nothing of the interface to click)
        drawPlay();
        return;
    }
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
        const MissionScript* script = findMissionScript(missions_[size_t(current_)].shortName, side());
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
    dimRect(screen_, kPanelMargin, top, screen_.w - 2 * kPanelMargin, screen_.h - kPanelMargin - top, Color{8, 12, 28});
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
        t += std::string(", as ") + sideName(side()) + " (J: side)";
    t += cadetMode() ? "; Cadet mode (K)" : "";
    if (!(carried_ == CarriedCapacity{}))
        t += "; capacity carried " + std::to_string(carried_.capacity) +
             (carried_.bank ? " (+" + std::to_string(carried_.bank) + " banked)" : "") + " (U: 50)";
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
    if (playing_ && world_)
        t += "; playing: " + std::to_string(world_->selection().size()) + " selected (F1-F3: cameras, Escape: leave)";
    if (playing_ && economy_)
        t += "; power " + std::to_string(int(economy_->powerSupply())) + "/" +
             std::to_string(int(economy_->powerDemand())) + " (B: build)";
    if (run_)
        t += "; script: " + walkNote_;
    if (running_)
        t += "; briefing step " + std::to_string(step_ + 1) + "/" + std::to_string(script_.size()) +
             (hasRecordedBriefing(level.shortName, side()) ? "" : " (by the general rule)");
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
        map_.setTitle(title(mapTitle_)); // (only when it changed)
}

void MissionViewer::stepScript() {
    if (!run_) {
        const MissionScript* script = findMissionScript(missions_[size_t(current_)].shortName, side());
        if (!script) {
            soundStatus_ = "no script for this mission yet";
            updateTitle();
            return;
        }
        stopBriefing();
        pageRun_ = std::make_unique<MissionRun>(*script, findMissionScript("generic", 'F'));
        run_ = pageRun_.get();
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
    ScriptEvent fired{"timer", {}};
    // The next event first lets time pass while it waits on the script's
    // timers (a handler for it a timer will arm).
    const bool waits = walkPos_ < walk_.size() && run_->waitsOnTimers(walk_[walkPos_]) && run_->nextTimer() >= 0;
    if (walkPos_ < walk_.size() && !waits) {
        const ScriptEvent& e = walk_[walkPos_++];
        walkNote_ = "event " + std::to_string(walkPos_) + "/" + std::to_string(walk_.size()) + ": " + e.kind;
        for (const std::string& a : e.args)
            walkNote_ += " " + a;
        fired = e;
        if (sim_) {
            // In play the simulation hears it, and does its part.
            sim_->fireScript(e);
            for (const MissionSim::Said& said : sim_->takeSaid())
                deliver(said.fired, said.out);
            redraw();
            updateTitle();
            return;
        }
        out = run_->fire(e);
        // The colony grows as the script builds (on the missions page only:
        // the game's own building is the player's), each building with its
        // clip.
        if (e.kind == "built" && !e.args.empty() && colony_) {
            if (colony_->add(e.args[0]))
                showColony();
            else
                walkNote_ += " (not placed)";
        }
        if (e.kind == "built" && !e.args.empty())
            if (const std::string clip = finishedClip(e.args[0], side()); !clip.empty()) {
                speak(clip);
                walkNote_ += " [" + clip + "]";
            }
    } else if (run_->nextTimer() >= 0) {
        walkNote_ = "time passes (" + std::to_string(int(run_->nextTimer())) + " s)";
        if (sim_) {
            sim_->runTimers(run_->nextTimer());
            for (const MissionSim::Said& said : sim_->takeSaid())
                deliver(said.fired, said.out);
            redraw();
            updateTitle();
            return;
        }
        out = run_->advance(run_->nextTimer());
    } else {
        walkNote_ = "nothing more scripted";
    }
    deliver(fired, out);
    redraw();
    updateTitle();
}

// What the script said: its messages to the log, the game's spoken
// announcement for it (Media/Misc), if any, and the outcome in the note.
// The voice is on the officers' channel, cutting in on what plays there
// (where the game plays a message's clip isn't known); a building's clip
// is apart (speak).
void MissionViewer::deliver(const ScriptEvent& fired, const std::vector<MissionRun::Output>& out) {
    if (playing_ && !out.empty())
        walkNote_ = fired.kind; // in play, the latest that said anything
    // A message's own voice clip first; else the game's announcement.
    std::string voice;
    for (const MissionRun::Output& o : out)
        if (o.kind == MissionRun::Output::Message && !o.clip.empty())
            voice = o.clip;
    if (voice.empty())
        voice = announcementFor(fired, out, side());
    if (!voice.empty() && mixer_) {
        stopClip();
        if (auto sound = loadSound(voice))
            clipHandle_ = mixer_->play(std::move(sound), audio::SoundType::Speech);
        else
            soundStatus_.clear(); // the game folder may not have Misc
        walkNote_ += " [" + voice + "]";
    }
    for (const MissionRun::Output& o : out) {
        if (o.kind == MissionRun::Output::Message) {
            log_.push_back({o.sender, o.text});
            // A new message: the panel opens on it.
            play_.commsShown = -1, play_.commsClosed = false;
        }
        else if (o.kind == MissionRun::Output::Won || o.kind == MissionRun::Output::Lost) {
            walkNote_ += o.kind == MissionRun::Output::Won ? "; mission won" : "; mission lost";
            // In a mission's run, the debriefing follows.
            if (flow_ == Flow::Playing) {
                flow_ = Flow::Debriefing;
                flowWon_ = o.kind == MissionRun::Output::Won;
                flowStartMs_ = nowMs_;
                dirty_ = true;
            }
        }
        else if (o.kind == MissionRun::Output::Failing)
            walkNote_ += "; mission failing";
        // (The AI's orders, postures, research and effects the simulation
        // has seen to.)
    }
    if (!out.empty())
        redraw();
}

void MissionViewer::stopScript() {
    if (!run_ || sim_)
        return;
    pageRun_.reset();
    run_ = nullptr;
    walk_.clear();
    log_.clear();
    if (mixer_)
        mixer_->stop(buildingClipHandle_);
    startColony();
    redraw();
    updateTitle();
}

bool MissionViewer::mapInView() const { return hasMap_ && !flowScreen() && (!running_ || stepOnMap_); }

char MissionViewer::side() const {
    if (missions_.empty())
        return 0;
    const std::vector<char> sides = missionSides(missions_[size_t(current_)].shortName);
    return size_t(side_) < sides.size() ? sides[size_t(side_)] : 0;
}

bool MissionViewer::canSave(std::string& why) const {
    if (!playing_ || !sim_) {
        why = "Only a mission being played can be saved (G or F3 on the missions page).";
        return false;
    }
    if (flow_ != Flow::None && flow_ != Flow::Playing) {
        why = "The mission is over.";
        return false;
    }
    return true;
}

std::string MissionViewer::stateDescription() const {
    if (missions_.empty() || !sim_)
        return {};
    const Level& level = missions_[size_t(current_)];
    const int seconds = int(sim_->clock());
    char time[16];
    std::snprintf(time, sizeof time, "%d:%02d", seconds / 60, seconds % 60);
    const char s = side();
    const char* race = s == 'K' ? "Klingon" : s == 'R' ? "Romulan" : "Federation";
    return level.shortName + ": " + level.title + " (" + race + ", " + time + ")";
}

Error MissionViewer::syncState(save::Serializer& s) {
    // Which mission, as whom.
    std::string mission = missions_.empty() ? "" : missions_[size_t(current_)].shortName;
    int sideIndex = side_;
    bool cadet = cadetMode(), flowed = flow_ == Flow::Playing;
    // (The capacity it started from: a load restarts from it, as the
    // session's carried value then.)
    CarriedCapacity carryIn = carryIn_;
    s(mission)(sideIndex)(cadet)(flowed)(carryIn);
    if (!s.ok())
        return Error::fail("The save is damaged: " + s.error());
    if (s.loading()) {
        const auto it = std::find_if(missions_.begin(), missions_.end(),
                                     [&](const Level& l) { return l.shortName == mission; });
        if (it == missions_.end())
            return Error::fail("This game's mission list has no " + mission + ".");
        const int index = int(it - missions_.begin());
        if (sideIndex < 0 || size_t(sideIndex) >= missionSides(mission).size())
            return Error::fail("The save is damaged: no such side.");
        // The mission started afresh, as play starts it, then put back.
        if (flow_ != Flow::None) {
            if (flow_ == Flow::Briefing)
                stopBriefing();
            flow_ = Flow::None;
        }
        stopBriefing();
        shown_ = true; // the page is up (activate() leaves it as it is)
        show(index);
        if (sideIndex != side_) {
            side_ = sideIndex;
            loadSide();
        }
        if (!hasMap_)
            return Error::fail("The archives have no map for " + mission + ".");
        setCadetMode(cadet);
        carried_ = carryIn;
        startPlay();
        stopClip(); // the opening's voice, already heard
        if (!sim_)
            return Error::fail("Couldn't start " + mission + ".");
    }
    sim_->syncState(s);
    s(log_)(play_.commsShown)(play_.commsClosed)(play_.commandShown);
    s(play_.shownOthers)(play_.otherBuildings)(play_.unitVoices);
    map_.syncCamera(s);
    if (s.loading()) {
        if (!s.ok()) {
            // Whatever was read is dropped: the mission from its start.
            flow_ = flowed ? Flow::Playing : Flow::None;
            restartPlay();
            return Error::fail("The save is damaged: " + s.error() + ". The mission starts over.");
        }
        flow_ = flowed ? Flow::Playing : Flow::None;
        flowWon_ = false;
        lastTickMs_ = nowMs_;
        play_.quitAsked = play_.paused = false;
        play_.selectedBuilding = 0;
        play_.placing.clear();
        showColony();
        showUnits();
        dirty_ = true;
        redraw();
        updateTitle();
    }
    return Error::success();
}

bool MissionViewer::restartPlay() {
    if (!playing_)
        return false;
    // From the capacity it started from (not what this play made of it).
    const CarriedCapacity in = carryIn_;
    stopPlay();
    carried_ = in;
    startPlay();
    flowWon_ = false;
    dirty_ = true;
    return playing_;
}

} // namespace reluva::newworlds
