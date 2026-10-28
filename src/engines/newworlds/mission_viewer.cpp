// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/briefing_data.h"
#include "engines/newworlds/model_render.h"
#include "engines/newworlds/star_system.h"
#include "engines/newworlds/start_data.h"
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

// A type's small portrait (gui.md#pictures), by the game's type id.
const char* portraitOf(int type) {
    static const std::pair<int, const char*> kPortraits[] = {
        // Hubs, mining stations, defensive weapons and artillery (JUMP TO).
        {23, "fhub_s"}, {28, "kchub_s"}, {43, "rchub_s"}, {45, "fms1_s"}, {52, "kms1_s"}, {65, "rms1_s"},
        {80, "fpdt_s"}, {54, "kdws_s"}, {81, "rdsb_s"}, {83, "fpds_s"}, {82, "kdpt_s"}, {84, "rddt_s"},
        // Vehicles.
        {8, "fpv_s"}, {55, "fapcwt_s"}, {56, "fapc_s"}, {12, "fwb_s"}, {13, "fcrgb_s"}, {14, "fconb_s"},
        {97, "fgs_s"}, {47, "fptl_s"}, {9, "fptla_s"}, {62, "fsv_s"}, {154, "fmsg_s"},
        {88, "kapcwt_s"}, {10, "kdt_s"}, {229, "kcdt_s"}, {228, "kmdb_s"}, {11, "kwb_s"}, {36, "kpa_s"},
        {37, "kfgs_s"}, {40, "kapc_s"}, {46, "kcb_s"}, {95, "ksv_s"},
        {89, "rctp_s"}, {35, "rct_s"}, {98, "rfgs_s"}, {87, "rptl_s"}, {64, "rapc_s"}, {230, "rcapc_s"},
        {90, "rcrgb_s"}, {91, "rconb_s"}, {92, "rwb_s"}, {63, "rsv_s"}, {148, "rt_s"}};
    for (const auto& [id, name] : kPortraits)
        if (id == type)
            return name;
    return "";
}

// A building's large portrait, per side (gui.md#pictures).
std::string buildingPortrait(std::string_view id, char side) {
    static const struct {
        const char* id;
        const char *fed, *kln, *rom;
    } kPortraits[] = {
        {"colony-hub", "fhub", "kchub", "rchub"},          {"construction-yard", "fcy", "kcy", "rcy"},
        {"vehicle-yard", "fvpy", "kvy", "rvy"},            {"resource-processor", "frps", "krps", "rrpp"},
        {"power-generator", "fpgs1", "kpg1", "rps1"},      {"advanced-generator", "fpgs2", "kpg2", "rps2"},
        {"mining-station", "fms1", "kms1", "rms1"},        {"accommodation", "fca", "ka", "rab"},
        {"sickbay", "fsb", "ksb", "ruh"},                  {"science-station", "fss", "kss", "rsb"},
        {"security-center", "fsc", "kief", "risb"},        {"farm", "fhf", "klsf", "rf"},
        {"storage-facility", "fsfr", "ksf", "rsf"},        {"transporter-pad", "flp", "", "rlp"},
        {"landing-pad", "flp", "", ""},                    {"shield-generator", "fsg", "ksg", "rsg"},
        {"defensive-weapon", "fpdt", "kdws", "rdsb"},      {"defensive-artillery", "fpds", "kdpt", "rddt"}};
    for (const auto& p : kPortraits)
        if (id == p.id)
            return side == 'K' ? p.kln : side == 'R' ? p.rom : p.fed;
    return {};
}

// The menus' one-line roles (Reluva's own words; the game's hover text
// isn't copied).
const char* roleOf(std::string_view id) {
    static const std::pair<const char*, const char*> kRoles[] = {
        {"storage-facility", "KEEPS THE COLONY'S MATERIALS"}, {"power-generator", "POWERS THE COLONY"},
        {"advanced-generator", "POWERS THE COLONY, MORE"},   {"construction-yard", "SUPPLIES THE BUILDING SITES"},
        {"vehicle-yard", "BUILDS VEHICLES"},                 {"resource-processor", "TURNS ORE INTO MATERIALS"},
        {"mining-station", "DIGS ORE FROM A DEPOSIT"},       {"transporter-pad", "BEAMS PEOPLE DOWN"},
        {"landing-pad", "BRINGS PEOPLE DOWN"},               {"science-station", "RESEARCH AND LABS"},
        {"sickbay", "LOOKS AFTER THE SICK"},                 {"security-center", "THE COLONY'S SECURITY"},
        {"accommodation", "HOUSES MORE PEOPLE"},             {"farm", "FEEDS LIFE SUPPORT"},
        {"shield-generator", "SHIELDS WHAT'S ROUND IT"},     {"defensive-weapon", "A GUN THAT GUARDS THE COLONY"},
        {"defensive-artillery", "SHELLS ATTACKERS FROM AFAR"}, {"cargo-bee", "CARRIES ORE AND GOODS"},
        {"construction-bee", "PUTS UP BUILDINGS"},           {"worker-bee", "MOVES GOODS IN AND OUT"},
        {"ground-scout", "FAST, UNARMED, SCANS"},            {"armed-apc", "CARRIES CREW, CAPTURES"},
        {"unarmed-apc", "CARRIES CREW"},                     {"science-vehicle", "SCANS CLOSE UP"},
        {"mobile-shield-generator", "SHIELDS UNITS ROUND IT"}};
    for (const auto& [n, role] : kRoles)
        if (id == n)
            return role;
    return "";
}

// A cost as the menus give it: dilithium in G, the next three in T,
// nitrium and duranium in KG.
std::vector<std::string> costLines(const Stock& c) {
    static const char* const kUnits[] = {"G", "T", "T", "T", "KG", "KG"};
    std::vector<std::string> out;
    for (int m = 0; m < kMaterials; ++m)
        if (c[size_t(m)] > 0)
            out.push_back(std::string(materialName(m)) + " " + std::to_string(c[size_t(m)]) + kUnits[m]);
    return out;
}

// The menus' order, as the training missions' pages show it; what they
// didn't show after.
int menuRank(std::string_view id) {
    static const char* const kOrder[] = {"storage-facility", "power-generator", "construction-yard", "vehicle-yard",
                                         "resource-processor", "mining-station", "transporter-pad", "landing-pad",
                                         "science-station", "sickbay", "security-center",
                                         "cargo-bee", "construction-bee", "worker-bee", "ground-scout", "armed-apc",
                                         "photon-artillery", "advanced-photon-artillery"};
    for (size_t i = 0; i < std::size(kOrder); ++i)
        if (id == kOrder[i])
            return int(i);
    return int(std::size(kOrder));
}

// JUMP TO's left column (exe.md#the-colony-overview): per race, the type
// whose portrait each slot shows: the hub, the mining stations, the
// defensive weapons, the defensive artillery, the fighting vehicles.
constexpr int kJumpRows = 2; // an icon's height, in lines
// The build and vehicle menus' portraits: at their own size, cut to the
// game's 64 x 60 (its cells are 90 x 60).
constexpr int kMenuIconW = 64, kMenuIcon = 60;
constexpr int kTricorderTiles = 16; // tiles across the small Tricorder at its closest (X4)
constexpr float kEdgeRamp = 80, kEdgeTop = 6400; // edge scrolling, units a tick
// How long a hit object stays among Space's alerts (Reluva's: the game's 41
// Tricorder redraws).
constexpr int kAlertTicks = 41;
constexpr int kJumpIcons[3][5] = {{23, 45, 80, 83, 56}, {28, 52, 54, 82, 40}, {43, 65, 81, 84, 64}};

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
        const char side = missionSides(level.shortName)[size_t(side_)];
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
    if (economy_) {
        const char side = missionSides(missions_[size_t(current_)].shortName)[size_t(side_)];
        bee = side == 'K' ? "geo/kln/conbee.geo" : side == 'R' ? "geo/rom/rcb.geo" : "geo/fed/fconbee.geo";
    }
    if (colony_ && colonyShown_)
        for (size_t i = 0; i < colony_->buildings().size(); ++i) {
            const ColonyBuilding& c = colony_->buildings()[i];
            // A destroyed one is left as its flattened wreck.
            const bool razed = i < colonyObjects().size() && colonyObjects()[i] < 0;
            buildings.push_back({c.model, c.a, c.b, 0, razed ? 0.0f : 1.0f, razed ? std::string() : bee, 0});
        }
    // The other sides' buildings.
    // The other sides' buildings (an AI colony's sites going up with its
    // construction bee).
    shownBuildings_ = 0;
    otherBuildings_.clear();
    if (world_)
        for (const Unit& u : world_->units())
            if (u.building && u.slot != playerSlot_ && !u.kind.model.empty() && visible(u)) {
                const std::string otherBee = u.built >= 1   ? std::string()
                                             : u.side == 'K' ? "geo/kln/conbee.geo"
                                             : u.side == 'R' ? "geo/rom/rcb.geo"
                                                             : "geo/fed/fconbee.geo";
                buildings.push_back({u.kind.model, int(u.y / MapViewer::kBlockUnits * 8),
                                     int(u.x / MapViewer::kBlockUnits * 8), 0, u.built, otherBee});
                otherBuildings_.push_back({u.id, u.built});
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
    map_.setTricorderScheme(0); // play draws the ring hatch (the game's setup)
    if (!hasMap_ || running_)
        return;
    const Level& level = missions_[size_t(current_)];
    const char side = missionSides(level.shortName)[size_t(side_)];
    // The colony as it starts (the mission's own, else round its site),
    // and the mission played on the shared simulation (mission_sim.h).
    colonyShown_ = true;
    startColony();
    ColonyMap ground{[this](int bx, int by) { return map_.depositAt(bx, by); },
                     [this](int bx, int by) { return map_.depositAmount(bx, by); }};
    sim_ = std::make_unique<MissionSim>(level.shortName, side, map_.terrain(), ground, map_.blocksWide(),
                                        map_.blocksHigh(),
                                        colony_ ? colony_->buildings() : std::vector<ColonyBuilding>{});
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
    flashes_.clear();
    blasts_.clear();
    pendingOrder_ = 0;
    researchMenu_ = vehicleMenu_ = buildMenu_ = false;
    placing_.clear();
    walk_.clear();
    log_.clear();
    walkNote_.clear();
    selectedBuilding_ = 0;
    razeAsked_ = 0;
    colonyDetails_ = false;
    jumpedTo_ = {};
    commsShown_ = -1, commsClosed_ = false, commandShown_ = true;
    allyOpen_ = false, allyPick_ = -1;
    shownOthers_.clear();
    // What the script said as the mission began.
    for (const MissionSim::Said& said : sim_->takeSaid())
        deliver(said.fired, said.out);
    float hx = map_.worldWidth() / 2, hy = map_.worldHeight() / 2;
    hubPosition(hx, hy);
    playing_ = true;
    interface_ = true;
    briefing_ = false;
    boxing_ = tricorderFull_ = false;
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

void MissionViewer::stopPlay() {
    if (!playing_)
        return;
    playing_ = false;
    quitAsked_ = paused_ = false;
    map_.stopFollowing();
    if (mixer_)
        mixer_->stop(stingHandle_);
    world_ = nullptr, economy_ = nullptr, ai_ = nullptr, fieldWork_ = nullptr;
    research_ = nullptr, effects_ = nullptr, fog_ = nullptr, run_ = nullptr;
    sim_.reset();
    log_.clear();
    walkNote_.clear();
    buildMenu_ = false;
    placing_.clear();
    map_.setGhost({}, 0, 0, false);
    map_.setUnits({});
    map_.unlock();
    redraw();
    updateTitle();
}

const char* MissionViewer::pendingOrderName() const {
    switch (pendingOrder_) {
    case 's': return "> SCIENCE SCAN";
    case 'm': return "> MEDICAL SCAN";
    case 'e': return "> ENGINEERING SCAN";
    case 'c': return "> CAPTURE BUILDING";
    case 'a': return "> ATTACK EN ROUTE / ASSAULT";
    case 'p': return "> PROTECT";
    default: return "";
    }
}

// The colony's officers into its buildings' efficiency (each building's
// best at its speciality), and the vehicles the yards have finished out
// on the map beside them (bees into the hub).
// The selection's order on another side's unit or building: the field
// work chosen (S, M, E, C), else a scout or science vehicle scans it (the
// game's default for them: a science scan) and the rest attack it.
void MissionViewer::orderOn(int target) {
    std::vector<int> attackers;
    for (int id : world_->selection()) {
        const Unit* u = world_->unit(id);
        if (!u)
            continue;
        if (fieldWork_)
            fieldWork_->cancel(id);
        const FieldWork::Kind kind = pendingOrder_ == 'm'   ? FieldWork::Medical
                                     : pendingOrder_ == 'e' ? FieldWork::Engineering
                                                            : FieldWork::Science;
        if (fieldWork_ && pendingOrder_ == 'c' && FieldWork::canCapture(*u) && fieldWork_->capture(id, target))
            continue;
        if (fieldWork_ && pendingOrder_ != 'c' && FieldWork::canScan(*u) && fieldWork_->scan(id, target, kind))
            continue;
        attackers.push_back(id);
    }
    // Attacks only on what the player is hostile to.
    const Unit* t = world_->unit(target);
    if (t && world_->stance(playerSlot_, t->slot) != 3)
        attackers.clear();
    if (!attackers.empty() && pendingOrder_ == 'a')
        world_->assault(attackers, target);
    else if (!attackers.empty() && pendingOrder_ != 's' && pendingOrder_ != 'm' && pendingOrder_ != 'e')
        world_->attack(attackers, target);
    pendingOrder_ = 0;
}

// What combat did since the last step, to draw: sparks where shots
// struck, blasts where something was destroyed, the colony's lost
// buildings flattened.
void MissionViewer::combatStep() {
    if (!sim_)
        return;
    const auto events = sim_->takeEvents();
    bool rebuild = false;
    for (const WorldEvent& e : events) {
        // A vehicle setting off: its start-up sound from where it set out
        // (any side's; not over its own still playing).
        if (e.kind == WorldEvent::Moved) {
            if (const auto [archive, stem] = driveSound(e.what.type); !archive.empty() &&
                                                                      !(mixer_ && mixer_->isPlaying(driveHandles_[e.from])))
                driveHandles_[e.from] = playAt(archive, stem, e.x0, e.y0, 1);
            continue;
        }
        if (e.kind == WorldEvent::Hit)
            flashes_.push_back({e.x1, e.y1, e.x1, e.y1, e.weapon, nowMs_ + 200});
        if (e.kind != WorldEvent::Destroyed)
            continue;
        blasts_.push_back({e.x0, e.y0, e.what.building ? 5000.0f : 2500.0f, nowMs_});
        rebuild = rebuild || e.what.building;
    }
    size_t others = 0;
    for (const Unit& u : world_->units())
        others += u.building && u.slot != playerSlot_ && !u.kind.model.empty() && visible(u);
    if (rebuild || others != shownBuildings_)
        showColony();
    if (!events.empty())
        showUnits();
    flashes_.erase(std::remove_if(flashes_.begin(), flashes_.end(), [&](const Flash& f) { return f.until <= nowMs_; }),
                   flashes_.end());
    blasts_.erase(std::remove_if(blasts_.begin(), blasts_.end(), [&](const Blast& b) { return b.start + 800 <= nowMs_; }),
                  blasts_.end());
    if (!flashes_.empty() || !blasts_.empty() || !events.empty() || !world_->projectiles().empty())
        redraw();
}

namespace {

void plot(Surface& s, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x >= 0 && y >= 0 && x < s.w && y < s.h) {
        uint8_t* p = s.row(y) + x * 4;
        p[0] = r, p[1] = g, p[2] = b;
    }
}

void line(Surface& s, int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b) {
    const int n = std::max({std::abs(x1 - x0), std::abs(y1 - y0), 1});
    if (n > 4000)
        return;
    for (int i = 0; i <= n; ++i)
        plot(s, x0 + (x1 - x0) * i / n, y0 + (y1 - y0) * i / n, r, g, b);
}

} // namespace

// Shots as beams (phasers orange, disruptors green) or a shell flying,
// blasts where something was destroyed, and a hit point bar over what's
// damaged or selected.
void MissionViewer::drawCombat() {
    if (!world_ || tricorderFull_)
        return;
    auto toScreen = [&](float x, float y, float up, int& sx, int& sy) {
        return map_.worldToScreen(x, y, map_.groundAtWorld(x, y) + up, sx, sy);
    };
    // The projectiles in flight: a phaser pulse an orange streak, a
    // disruptor bolt a green one, a shell a bright dot up its arc.
    for (const Projectile& p : world_->projectiles()) {
        int ax = 0, ay = 0, bx = 0, by = 0;
        if (p.kind == WeaponKind::Artillery) {
            if (!map_.worldToScreen(p.x, p.y, p.z + 300, ax, ay))
                continue;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    plot(screen_, ax + dx, ay + dy, 255, 240, 160);
            continue;
        }
        if (!toScreen(p.px, p.py, 600, ax, ay) || !toScreen(p.x, p.y, 600, bx, by))
            continue;
        if (p.kind == WeaponKind::Single) {
            line(screen_, ax, ay, bx, by, 90, 255, 90);
            line(screen_, ax, ay + 1, bx, by + 1, 40, 200, 40);
        } else {
            line(screen_, ax, ay, bx, by, 255, 170, 60);
        }
    }
    // Where they struck: a spark.
    for (const Flash& f : flashes_) {
        int cx = 0, cy = 0;
        if (!toScreen(f.x1, f.y1, 500, cx, cy))
            continue;
        const bool green = f.weapon == WeaponKind::Single;
        for (int d = -3; d <= 3; ++d) {
            plot(screen_, cx + d, cy, 255, green ? 255 : 230, green ? 140 : 120);
            plot(screen_, cx, cy + d, 255, green ? 255 : 230, green ? 140 : 120);
        }
    }
    for (const Blast& b : blasts_) {
        int cx = 0, cy = 0, ex = 0, ey = 0;
        if (!toScreen(b.x, b.y, 300, cx, cy) || !toScreen(b.x + b.size, b.y, 300, ex, ey))
            continue;
        const float t = float(nowMs_ - b.start) / 800.0f;
        const int r = std::max(2, int(std::hypot(float(ex - cx), float(ey - cy)) * (0.3f + t)));
        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx)
                if (dx * dx + dy * dy <= r * r && (dx * dx + dy * dy) * 3 >= r * r * int(t * 3))
                    plot(screen_, cx + dx, cy + dy, 255, uint8_t(220 - 160 * t), uint8_t(80 - 70 * t));
    }
    // The script's effects: meteors (a falling streak, then a blast),
    // storm flashes, beams (a column of light).
    if (effects_)
        for (const ScriptEffects::Spark& sp : effects_->sparks()) {
            int gx = 0, gy = 0, tx = 0, ty = 0;
            if (!toScreen(sp.x, sp.y, 0, gx, gy))
                continue;
            const float t = float(sp.age);
            if (sp.kind == 0 && toScreen(sp.x, sp.y, 6000 * std::max(0.0f, 0.3f - t) / 0.3f, tx, ty)) {
                line(screen_, tx, ty - 20, tx, ty, 255, 200, 120);
                if (t > 0.3f)
                    for (int dy = -4; dy <= 4; ++dy)
                        for (int dx = -4; dx <= 4; ++dx)
                            if (dx * dx + dy * dy <= int(16 * (1 - t)))
                                plot(screen_, gx + dx, gy + dy, 255, 160, 60);
            } else if (sp.kind == 1 && t < 0.15f && toScreen(sp.x, sp.y, 8000, tx, ty)) {
                line(screen_, tx, ty, gx + 6, (ty + gy) / 2, 200, 220, 255);
                line(screen_, gx + 6, (ty + gy) / 2, gx, gy, 200, 220, 255);
            } else if (sp.kind == 2 && toScreen(sp.x, sp.y, 4000, tx, ty)) {
                for (int dx = -2; dx <= 2; ++dx)
                    if ((int(nowMs_ / 40) + dx) % 2)
                        line(screen_, gx + dx, gy, tx + dx, ty, 180, 220, 255);
            }
        }
    // Scans as a dotted pale blue line; held buildings marked.
    if (fieldWork_) {
        for (const auto& [from, to] : fieldWork_->scanning()) {
            const Unit* a = world_->unit(from);
            const Unit* b = world_->unit(to);
            int ax = 0, ay = 0, bx = 0, by = 0;
            if (!a || !b || !toScreen(a->x, a->y, 700, ax, ay) || !toScreen(b->x, b->y, 800, bx, by))
                continue;
            const int n = std::max({std::abs(bx - ax), std::abs(by - ay), 1});
            const int phase = int(nowMs_ / 60 % 6);
            for (int i = 0; i <= n && n < 4000; ++i)
                if ((i + phase) % 6 < 3)
                    plot(screen_, ax + (bx - ax) * i / n, ay + (by - ay) * i / n, 140, 210, 255);
        }
        for (const Unit& u : world_->units())
            if (const int holder = fieldWork_->heldBy(u.id); holder >= 0) {
                int sx = 0, sy = 0;
                if (toScreen(u.x, u.y, 3500, sx, sy))
                    ui_.drawSmall(screen_, sx - ui_.smallWidth("CAPTURED") / 2, sy - 12, "CAPTURED",
                                  holder == playerSlot_ ? Color{120, 255, 120} : Color{255, 90, 60});
            }
    }
    for (const Unit& u : world_->units()) {
        if (!(u.selected || u.hp < u.maxHp) || u.hp <= 0)
            continue;
        int sx = 0, sy = 0;
        if (!toScreen(u.x, u.y, u.building ? 3000 : 1500, sx, sy))
            continue;
        const int w = u.building ? 24 : 14, filled = int(float(w) * u.hp / u.maxHp);
        for (int x = 0; x < w; ++x)
            for (int y = 0; y < 2; ++y) {
                const bool on = x < filled;
                const float share = u.hp / u.maxHp;
                plot(screen_, sx - w / 2 + x, sy + y, on ? uint8_t(share < 0.5f ? 255 : 80) : 40,
                     on ? uint8_t(share < 0.25f ? 60 : 220) : 40, on ? 60 : 40);
            }
    }
}

// Whether the player sees a thing: its own always, another side's on a
// block its fog marks as seen (the game draws nothing else).
bool MissionViewer::visible(const Unit& u) const {
    return u.slot == playerSlot_ || ((!fog_ || fog_->seenAt(u.x, u.y, playerSlot_)) && !u.cloaked);
}

void MissionViewer::showUnits() {
    std::vector<MapViewer::UnitDraw> draw;
    if (world_)
        for (const Unit& u : world_->units())
            if (!u.building && !u.kind.model.empty() && visible(u)) {
                // A cloaking unit shimmers over its 63-tick fades, then is
                // drawn as it is (the side's own; others' are gone): its
                // own side's most at the middle of a fade, another side's
                // most as it nears cloaked (exe.md#cloaking).
                float shimmer = 0;
                if (u.fading > 0) {
                    const float towards = float(u.cloaking ? 63 - u.fading : u.fading) / 63;
                    shimmer = u.slot == playerSlot_ ? 97 * std::sin(3.14159265f * towards) : 98 * towards;
                }
                MapViewer::UnitDraw d{u.kind.model, u.x, u.y, u.heading, u.selected, u.turretYaw, 0, shimmer};
                d.health = u.maxHp > 0 ? u.hp / u.maxHp : 1;
                draw.push_back(d);
            }
    // The colony's bees out of the hub: flying (rising off and settling
    // back down), or docked at a mine or the processor; those at work on a
    // site the map draws at its corners. Construction and cargo bees cruise
    // in eight lanes 9000-12500 up (exe.md#docking), worker bees 500 up
    // between their building and its yard; a pod aboard hangs under its bee.
    if (economy_) {
        const char side = missionSides(missions_[size_t(current_)].shortName)[size_t(side_)];
        const char* conBee = side == 'K' ? "geo/kln/conbee.geo" : side == 'R' ? "geo/rom/rcb.geo" : "geo/fed/fconbee.geo";
        const char* cargoBee = side == 'K' ? "geo/kln/kwb.geo" : side == 'R' ? "geo/rom/rwbc.geo" : "geo/fed/fcarb.geo";
        const char* workBee = side == 'K' ? "geo/kln/kwb.geo" : side == 'R' ? "geo/rom/rwb.geo" : "geo/fed/workbee.geo";
        // The pods: the Federation's crate, the Klingons' and Romulans' own.
        const char* pod = side == 'K' ? "geo/kln/kwbc.geo" : side == 'R' ? "geo/rom/rc.geo" : "geo/fed/crate_l.geo";
        for (const Economy::Bee& b : economy_->bees()) {
            // (A worker bee waiting at its building is docked inside it.)
            if (b.home || b.working || (b.worker && b.flight == 0 && b.pods.empty()))
                continue;
            const float f = b.flight > 0 ? float(b.flown) / float(b.flight) : 0;
            const float height = b.worker ? 500.0f : 9000.0f + 500.0f * float(b.id % 8);
            const float lift = b.flight > 0 ? height * std::min(1.0f, 4 * std::min(f, 1 - f)) : 0;
            MapViewer::UnitDraw d{b.worker ? workBee : b.cargo ? cargoBee : conBee, b.x, b.y, b.heading};
            d.lift = lift;
            draw.push_back(d);
            for (size_t k = 0; k < b.pods.size(); ++k) {
                MapViewer::UnitDraw c{pod, b.x + float(k) * 600, b.y, b.heading};
                c.lift = std::max(0.0f, lift - 400);
                draw.push_back(c);
            }
        }
        // The cargo pods standing on the yards.
        for (const Economy::Pod& p : economy_->pods())
            if (p.state == Economy::Pod::Parked)
                draw.push_back({pod, p.x, p.y, 0});
    }
    map_.setUnits(std::move(draw));
}

std::vector<const BuildingType*> MissionViewer::buildMenu() const {
    std::vector<const BuildingType*> out;
    if (!economy_)
        return out;
    const char side = missionSides(missions_[size_t(current_)].shortName)[size_t(side_)];
    // What can be built now, as the game's menu offers it: buildings the
    // side has a model for, and upgrades.
    for (const BuildingType& t : buildingTypes())
        if (economy_->available(t) && (!t.upgradeOf.empty() || !buildingModel(t.id, side).empty()))
            out.push_back(&t);
    // (Upgrades, which the game offers on their building's own page, last.)
    std::stable_sort(out.begin(), out.end(), [](const BuildingType* a, const BuildingType* b) {
        return std::pair{!a->upgradeOf.empty(), menuRank(a->id)} < std::pair{!b->upgradeOf.empty(), menuRank(b->id)};
    });
    return out;
}

std::vector<const VehicleType*> MissionViewer::vehicleMenu() const {
    std::vector<const VehicleType*> out = economy_ ? economy_->vehicleMenu() : std::vector<const VehicleType*>{};
    std::stable_sort(out.begin(), out.end(), [](const VehicleType* a, const VehicleType* b) {
        const CombatType *ta = combatType(a->type), *tb = combatType(b->type);
        return menuRank(ta ? ta->name : "") < menuRank(tb ? tb->name : "");
    });
    return out;
}

void MissionViewer::choose(const BuildingType& t) {
    if (!economy_ || !economy_->available(t))
        return;
    if (!t.upgradeOf.empty()) {
        // An upgrade goes on its building at once.
        for (const ColonyStructure& s : economy_->structures())
            if (s.type == t.upgradeOf && s.progress >= 1) {
                if (economy_->affordable(t) && colony_ && colony_->add(t.id)) {
                    const ColonyBuilding& c = colony_->buildings().back();
                    sim_->placeUpgrade(t.id, c.model, c.a, c.b, s.x, s.y);
                    showColony();
                }
                break;
            }
        buildMenu_ = false;
        return;
    }
    placing_ = t.id;
    buildMenu_ = false;
    ghostA_ = ghostB_ = -1;
    updateGhost();
}

// The hologram follows the pointer, on the tile grid, green where the
// building can go.
void MissionViewer::updateGhost() {
    if (placing_.empty() || !colony_)
        return;
    float x = 0, y = 0;
    int a = -1, b = -1;
    bool ok = false;
    if (map_.screenToGround(mouseX_, mouseY_, x, y)) {
        const int pa = int(y / MapViewer::kBlockUnits * 8), pb = int(x / MapViewer::kBlockUnits * 8);
        // In play, the game's own test (exe.md#building-placement).
        if (sim_ ? !sim_->placementSpot(placing_, x, y, a, b, ok) : !colony_->spotAt(placing_, pa, pb, a, b, ok))
            a = b = -1;
    }
    if (a == ghostA_ && b == ghostB_ && ok == ghostOk_)
        return;
    ghostA_ = a, ghostB_ = b, ghostOk_ = ok;
    const char side = missionSides(missions_[size_t(current_)].shortName)[size_t(side_)];
    map_.setGhost(a < 0 ? std::string() : buildingModel(placing_, side), a, b, ok);
}

void MissionViewer::placeBuilding() {
    if (placing_.empty() || !colony_ || !economy_ || !ghostOk_ || ghostA_ < 0)
        return;
    const char side = missionSides(missions_[size_t(current_)].shortName)[size_t(side_)];
    if (!sim_ || sim_->placeBuilding(placing_, buildingModel(placing_, side), ghostA_, ghostB_) < 0)
        return;
    colony_->put(placing_, ghostA_, ghostB_);
    placing_.clear();
    map_.setGhost({}, 0, 0, false);
    showColony();
}

void MissionViewer::showProgress() {
    // The other sides' sites; one finished loses its bee.
    const size_t own = buildingsShown_ - otherBuildings_.size();
    bool finished = false;
    for (size_t i = 0; world_ && i < otherBuildings_.size(); ++i)
        if (const Unit* u = world_->unit(otherBuildings_[i].first); u && u->built != otherBuildings_[i].second) {
            finished = finished || u->built >= 1;
            otherBuildings_[i].second = u->built;
            map_.setBuildingProgress(own + i, u->built);
        }
    if (finished) {
        showColony();
        return;
    }
    if (!economy_)
        return;
    const auto& all = economy_->structures();
    for (size_t i = 0; i < structureIds().size(); ++i)
        for (const ColonyStructure& c : all)
            if (c.id == structureIds()[i]) {
                map_.setBuildingProgress(i, c.progress, economy_->beesAt(c.id));
                if (Unit* u = world_ && i < colonyObjects().size() && colonyObjects()[i] > 0 ? world_->unit(colonyObjects()[i]) : nullptr) {
                    u->built = c.progress;
                    u->efficiency = economy_->efficiency(c);
                }
            }
}

bool MissionViewer::playEvent(const Event& ev) {
    if (ev.type == EventType::MouseMove || ev.type == EventType::MouseDown || ev.type == EventType::MouseUp)
        mouseX_ = ev.mouseX, mouseY_ = ev.mouseY, mouseSeen_ = true;
    const bool ctrl = (ev.mods & keymod::Ctrl) != 0, shift = (ev.mods & keymod::Shift) != 0,
               alt = (ev.mods & keymod::Alt) != 0;
    // Pause (and the window losing the focus) freezes the mission: no
    // time passes and nothing on the map can be touched; nothing says so
    // (exe.md#keys).
    if ((ev.type == EventType::KeyDown && ev.key == key::Pause) || ev.type == EventType::FocusLost ||
        ev.type == EventType::FocusGained) {
        if (ev.type == EventType::KeyDown)
            paused_ = !paused_;
        else
            unfocused_ = ev.type == EventType::FocusLost;
        boxing_ = false;
        return true;
    }
    if (paused_ || unfocused_)
        return true;
    // Escape's box: quit the mission, yes or no; the mission goes on
    // meanwhile.
    if (quitAsked_) {
        bool yes = false, no = false;
        if (ev.type == EventType::KeyDown)
            yes = ev.key == 'y' || ev.key == key::Return, no = ev.key == 'n' || ev.key == key::Escape;
        else if (ev.type == EventType::MouseDown && ev.button == MouseButton::Left) {
            auto in = [&](const int* r) {
                return ev.mouseX >= r[0] && ev.mouseY >= r[1] && ev.mouseX < r[0] + r[2] && ev.mouseY < r[1] + r[3];
            };
            yes = in(quitYes_), no = in(quitNo_);
        }
        if (yes) {
            quitAsked_ = false;
            if (flow_ == Flow::Playing)
                endFlow();
            else
                stopPlay();
            return true;
        }
        if (no)
            quitAsked_ = false, redraw();
        if (yes || no || ev.type == EventType::KeyDown || ev.type == EventType::MouseDown)
            return true;
    }
    // The tutorials wait for keys, by the game's (Windows) key codes.
    if (ev.type == EventType::KeyDown && run_ && !run_->finished()) {
        int code = 0;
        if (ev.key == key::Tab)
            code = 9;
        else if (ev.key == key::Up || ev.key == key::Down || ev.key == key::Left || ev.key == key::Right)
            code = ev.key == key::Left ? 37 : ev.key == key::Up ? 38 : ev.key == key::Right ? 39 : 40;
        else if (ev.key >= key::F1 && ev.key < key::F1 + 12)
            code = 112 + int(ev.key - key::F1);
        else if (ev.key >= key::Kp0 && ev.key <= key::Kp0 + 9)
            code = 96 + int(ev.key - key::Kp0);
        else if (ev.key == key::LCtrl || ev.key == key::RCtrl)
            code = 17;
        else if (ev.key >= '0' && ev.key <= '9')
            code = int(ev.key);
        else if (ev.key == key::Home)
            code = 36;
        if (code) {
            const ScriptEvent k{"key", {std::to_string(code)}};
            deliver(k, run_->fire(k));
        }
    }
    switch (ev.type) {
    case EventType::KeyDown:
        if (ev.key == key::Escape && trainingView_) {
            trainingView_ = false;
            break;
        }
        if (ev.key == key::Escape && (!playbook_.empty() || staffView_ || pickedOfficer_ >= 0)) {
            playbook_.clear();
            staffView_ = false;
            pickedOfficer_ = -1;
            break;
        }
        if (ev.key == key::Escape && (researchMenu_ || vehicleMenu_)) {
            researchMenu_ = vehicleMenu_ = false;
            break;
        }
        if (ev.key == key::Escape && (buildMenu_ || !placing_.empty())) {
            buildMenu_ = false;
            placing_.clear();
            map_.setGhost({}, 0, 0, false);
            break;
        }
        if (ev.key == key::Escape) {
            quitAsked_ = true;
            break;
        }
        // R: the research topics open to the colony; a number starts one.
        if (ev.key == 'r' && research_) {
            researchMenu_ = !researchMenu_;
            buildMenu_ = vehicleMenu_ = false;
            break;
        }
        // V: the vehicles the yards can make (the game's vehicle menu); a
        // number orders one.
        if (ev.key == 'v' && !shift && !ctrl && !alt && economy_) {
            vehicleMenu_ = !vehicleMenu_;
            menuPage_ = 0;
            buildMenu_ = researchMenu_ = false;
            break;
        }
        if (vehicleMenu_ && economy_ && ev.key >= '0' && ev.key <= '9') {
            const size_t n = ev.key == '0' ? 9 : size_t(ev.key - '1');
            const auto menu = vehicleMenu();
            if (n < menu.size())
                economy_->orderVehicle(*menu[n]);
            break;
        }
        if (researchMenu_ && research_ && ev.key >= '1' && ev.key <= '9') {
            const auto offered = research_->offered();
            if (size_t(ev.key - '1') < offered.size())
                research_->start(offered[size_t(ev.key - '1')]);
            break;
        }
        // Shift or Ctrl + B / V: the camera to the next of the colony's
        // buildings / fighting vehicles (exe.md#keys).
        if ((ev.key == 'b' || ev.key == 'v') && (shift || ctrl)) {
            cycleCamera(ev.key == 'b');
            break;
        }
        // C: the colony's details (the hub's INFO page).
        if (ev.key == 'c' && !alt && !ctrl) {
            for (size_t i = 0; economy_ && i < colonyObjects().size() && i < structureIds().size(); ++i)
                if (const ColonyStructure* c = structureOf(colonyObjects()[i]); c && c->type == "colony-hub") {
                    world_->clearSelection();
                    selectedBuilding_ = colonyObjects()[i];
                    colonyDetails_ = true;
                    buildMenu_ = vehicleMenu_ = researchMenu_ = false;
                    break;
                }
            break;
        }
        // A, G, S, T, P: all the colony's APCs, ground scouts, science
        // vehicles, tanks, photon artillery selected.
        if (!alt && !ctrl && (ev.key == 'a' || ev.key == 'g' || ev.key == 's' || ev.key == 't' || ev.key == 'p')) {
            selectAllOfKind(char(ev.key));
            break;
        }
        if (ev.key == 'b') {
            researchMenu_ = vehicleMenu_ = false;
            buildMenu_ = !buildMenu_;
            menuPage_ = 0;
            placing_.clear();
            map_.setGhost({}, 0, 0, false);
            break;
        }
        if (buildMenu_ && ev.key >= '0' && ev.key <= '9') {
            // 1-9, then 0 for the tenth.
            const size_t n = ev.key == '0' ? 9 : size_t(ev.key - '1');
            const auto menu = buildMenu();
            if (n < menu.size())
                choose(*menu[n]);
            break;
        }
        if (ev.key == key::Tab) {
            tricorderFull_ = !tricorderFull_;
            break;
        }
        // Space: the camera to the next of the colony's objects lately hit
        // (the game's alerts), in their order, round again.
        if (ev.key == ' ' && world_) {
            std::vector<const Unit*> alerted;
            for (const Unit& u : world_->units())
                if (u.slot == playerSlot_ && u.hp > 0 && sim_->alerted(u.id))
                    alerted.push_back(&u);
            if (!alerted.empty()) {
                size_t next = 0;
                for (size_t i = 0; i < alerted.size(); ++i)
                    if (alerted[i]->id == alertedLast_)
                        next = (i + 1) % alerted.size();
                alertedLast_ = alerted[next]->id;
                cameraTo(*alerted[next]);
            }
            break;
        }
        if (ev.key == key::Home) {
            float x = 0, y = 0;
            if (hubPosition(x, y))
                map_.centreOn(x, y);
            break;
        }
        // Battle groups 1-5, as the game's (exe.md#keys).
        if (ev.key >= '1' && ev.key <= '5') {
            const int n = int(ev.key - '0');
            float x = 0, y = 0;
            if (ctrl)
                world_->setGroup(n);
            else if (shift && world_->groupCentre(n, x, y))
                map_.centreOn(x, y);
            else if (!shift)
                world_->selectGroup(n);
            showUnits();
            break;
        }
        if (ev.key == 'i') {
            interface_ = !interface_;
            break;
        }
        // Field work, for the selection (Reluva's keys for the playbook's
        // orders, under Alt, the game's own letters being taken): Alt+S,
        // M, E a science, medical or engineering scan and Alt+C a capture
        // (on the next unit or building clicked); O takes aboard the
        // officers of the building beside it (Shift+O puts them down).
        // Alt+A attack en route (on the ground) or assault (on a target),
        // Alt+P protect a spot (or, mobile shield generators, the building
        // clicked), Y back to the rally point.
        if (alt && (ev.key == 's' || ev.key == 'm' || ev.key == 'e' || ev.key == 'c' || ev.key == 'a' || ev.key == 'p') &&
            !world_->selection().empty()) {
            pendingOrder_ = pendingOrder_ == char(ev.key) ? 0 : char(ev.key);
            break;
        }
        // Ctrl+X: the script's walkthrough a step on (a testing aid, as X
        // is on the missions page).
        if (ev.key == 'x' && ctrl && run_) {
            if (walk_.empty())
                walk_ = run_->walkthrough(), walkPos_ = 0;
            stepScript();
            break;
        }
        if (ev.key == 'y' && !world_->selection().empty()) {
            rallySelected();
            break;
        }
        if (ev.key == 'o' && fieldWork_) {
            int moved = 0;
            for (int id : world_->selection())
                moved += shift ? fieldWork_->disembark(id) : fieldWork_->board(id);
            walkNote_ = std::to_string(moved) + (shift ? " officers put down" : " officers aboard");
            break;
        }
        if (map_.gameKey(ev.key, mouseX_, mouseY_))
            break;
        return true; // the data viewer's keys don't apply in play
    case EventType::MouseDown:
        if (!placing_.empty() && !ctrl) {
            // Left click builds where the hologram is green; right click
            // puts it away.
            if (ev.button == MouseButton::Left)
                placeBuilding();
            else if (ev.button == MouseButton::Right) {
                placing_.clear();
                map_.setGhost({}, 0, 0, false);
            }
            break;
        }
        // The playbook open: a click on one of its icons gives that order;
        // any other click (or the right button) puts it away.
        if (!playbook_.empty()) {
            constexpr int kIcon = 32, kGap = 4;
            const int i = (ev.mouseY - playbookAt_[1]) / (kIcon + kGap);
            if (ev.button == MouseButton::Left && ev.mouseX >= playbookAt_[0] && ev.mouseX < playbookAt_[0] + kIcon &&
                ev.mouseY >= playbookAt_[1] && i < int(playbook_.size()) && (ev.mouseY - playbookAt_[1]) % (kIcon + kGap) < kIcon)
                playbookOrder(playbook_[size_t(i)].order);
            playbook_.clear();
            redraw();
            break;
        }
        if ((ev.button == MouseButton::Left || (ev.button == MouseButton::Right && !ctrl)) && interface_ &&
            pressButton(ev.mouseX, ev.mouseY, ev.button == MouseButton::Right, ev.clicks))
            break;
        if (ev.button == MouseButton::Left && interface_ && !ctrl && tricorder_[2] > 0 &&
            ev.mouseX >= tricorder_[0] && ev.mouseY >= tricorder_[1] && ev.mouseX < tricorder_[0] + tricorder_[2] &&
            ev.mouseY < tricorder_[1] + tricorder_[3]) {
            // A click on the tricorder takes the camera there.
            float wx = 0, wy = 0;
            map_.minimapToWorld(ev.mouseX, ev.mouseY, tricorder_[0], tricorder_[1], tricorder_[2], tricorder_[3],
                                kTricorderTiles << tricorderZoom_, wx, wy);
            map_.stopFollowing();
            map_.centreOn(wx, wy);
            break;
        }
        if (ev.button == MouseButton::Left && ctrl) {
            map_.zoomAt(ev.mouseX, ev.mouseY, true);
        } else if (ev.button == MouseButton::Right && ctrl) {
            map_.zoomAt(ev.mouseX, ev.mouseY, false);
        } else if (ev.button == MouseButton::Right && ev.clicks >= 2) {
            // A double right click takes the camera to the ground there, on
            // the map or the Tricorder (exe.md#cameras).
            float wx = 0, wy = 0;
            const bool onTricorder = interface_ && tricorder_[2] > 0 && ev.mouseX >= tricorder_[0] &&
                                     ev.mouseY >= tricorder_[1] && ev.mouseX < tricorder_[0] + tricorder_[2] &&
                                     ev.mouseY < tricorder_[1] + tricorder_[3];
            if (onTricorder)
                map_.minimapToWorld(ev.mouseX, ev.mouseY, tricorder_[0], tricorder_[1], tricorder_[2], tricorder_[3],
                                    kTricorderTiles << tricorderZoom_, wx, wy);
            if (onTricorder || map_.screenToGround(ev.mouseX, ev.mouseY, wx, wy)) {
                map_.stopFollowing();
                followed_ = 0;
                map_.centreOn(wx, wy);
            }
        } else if (ev.button == MouseButton::Right) {
            // Right-click drops the selection, or the camera's lock.
            if (map_.locked())
                map_.unlock();
            else
                world_->clearSelection(), selectedBuilding_ = 0;
            showUnits();
        } else if (ev.button == MouseButton::Left) {
            boxing_ = true;
            boxX_ = ev.mouseX, boxY_ = ev.mouseY;
            return true;
        }
        break;
    case EventType::MouseMove:
        if (!placing_.empty()) {
            updateGhost();
            return true;
        }
        if (!boxing_)
            return true;
        break;
    case EventType::MouseUp: {
        if (ev.button != MouseButton::Left || !boxing_)
            return true;
        boxing_ = false;
        const int x0 = std::min(boxX_, ev.mouseX), x1 = std::max(boxX_, ev.mouseX);
        const int y0 = std::min(boxY_, ev.mouseY), y1 = std::max(boxY_, ev.mouseY);
        std::vector<int> picked;
        if (x1 - x0 > 6 || y1 - y0 > 6) {
            // A box: the side's units inside it.
            for (const Unit& u : world_->units()) {
                int sx = 0, sy = 0;
                if (u.slot == playerSlot_ && !u.building &&
                    map_.worldToScreen(u.x, u.y, map_.groundAtWorld(u.x, u.y), sx, sy) &&
                    sx >= x0 && sx <= x1 && sy >= y0 && sy <= y1)
                    picked.push_back(u.id);
            }
            world_->select(picked);
        } else {
            // A click: the nearest unit to it, if close; else, with units
            // selected, they go there; a double click on the ground locks
            // the camera on it.
            // With units selected, a click on another side's unit or
            // building sends them to attack it.
            int best = -1, bestD = 18 * 18, other = -1, otherD = 18 * 18;
            for (const Unit& u : world_->units()) {
                int sx = 0, sy = 0;
                if (!map_.worldToScreen(u.x, u.y, map_.groundAtWorld(u.x, u.y) + (u.building ? 1500 : 300), sx, sy))
                    continue;
                const int d = (sx - ev.mouseX) * (sx - ev.mouseX) + (sy - ev.mouseY) * (sy - ev.mouseY);
                if (u.slot == playerSlot_ && !u.building && d < bestD)
                    best = u.id, bestD = d;
                else if (u.slot != playerSlot_ && visible(u) && d < (u.building ? 2 : 1) * otherD)
                    other = u.id, otherD = u.building ? d / 2 : d;
            }
            float gx = 0, gy = 0;
            // Protecting one of the colony's buildings: mobile shield
            // generators guard it (the game's "protect area"), the rest
            // hold the spot.
            int ownBuilding = -1, ownD = 30 * 30;
            if (pendingOrder_ == 'p')
                for (const Unit& u : world_->units()) {
                    int sx = 0, sy = 0;
                    if (u.slot != playerSlot_ || !u.building ||
                        !map_.worldToScreen(u.x, u.y, map_.groundAtWorld(u.x, u.y) + 1500, sx, sy))
                        continue;
                    const int d = (sx - ev.mouseX) * (sx - ev.mouseX) + (sy - ev.mouseY) * (sy - ev.mouseY);
                    if (d < ownD)
                        ownBuilding = u.id, ownD = d;
                }
            // A building clicked with nothing to order: selected, its
            // details in the panel (the colony's, or another side's seen).
            int building = -1, buildingD = 30 * 30;
            if (!pendingOrder_)
                for (const Unit& u : world_->units()) {
                    int sx = 0, sy = 0;
                    if (!u.building || u.kind.model.empty() || (u.slot != playerSlot_ && !visible(u)) ||
                        !map_.worldToScreen(u.x, u.y, map_.groundAtWorld(u.x, u.y) + 1500, sx, sy))
                        continue;
                    const int d = (sx - ev.mouseX) * (sx - ev.mouseX) + (sy - ev.mouseY) * (sy - ev.mouseY);
                    if (d < buildingD)
                        building = u.id, buildingD = d;
                }
            // An officer picked on the STAFF panel: the building or vehicle
            // clicked takes them.
            if (pickedOfficer_ >= 0 && fieldWork_) {
                const int into = best >= 0 ? best
                                 : building >= 0 && world_->unit(building)->slot == playerSlot_ ? building
                                                                                                 : -1;
                if (into >= 0) {
                    walkNote_ = fieldWork_->moveOfficer(size_t(pickedOfficer_), into) ? "officer moved"
                                                                                        : "can't take an officer";
                    pickedOfficer_ = -1;
                    showUnits();
                    break;
                }
            }
            // A double click with units selected: the playbook, for what
            // was clicked (another side's object, one of the colony's
            // buildings, or the ground).
            if (ev.clicks >= 2 && !world_->selection().empty() && best < 0) {
                float px = 0, py = 0;
                map_.screenToGround(ev.mouseX, ev.mouseY, px, py);
                const int target = other >= 0 ? other
                                   : building >= 0 && world_->unit(building)->slot == playerSlot_ ? building
                                                                                                   : 0;
                openPlaybook(target, px, py);
                break;
            }
            const bool ordering = !world_->selection().empty() && other >= 0 && building == other;
            if (building >= 0 && best < 0 && !ordering && world_->selection().empty()) {
                world_->clearSelection();
                selectedBuilding_ = building;
                if (ev.clicks >= 2)
                    doubleClicked(building);
            } else if (best >= 0 && ev.clicks >= 2 && (other < 0 || otherD >= bestD)) {
                world_->select({best}), selectedBuilding_ = 0;
                doubleClicked(best);
            } else if (ownBuilding >= 0 && !world_->selection().empty()) {
                const Unit& b = *world_->unit(ownBuilding);
                const std::vector<int> chosen = world_->selection();
                std::vector<int> shields, others;
                for (int id : chosen)
                    if (const Unit* u = world_->unit(id))
                        (u->name == "mobile-shield-generator" ? shields : others).push_back(id);
                world_->protect(shields, {ownBuilding});
                world_->select(others);
                world_->moveSelected(b.x, b.y - 1.2f * kTile, UnitOrder::Protect);
                world_->protect(others);
                world_->select(chosen);
                pendingOrder_ = 0;
                orderSpoken(false);
            } else if (other >= 0 && !world_->selection().empty() && (best < 0 || otherD < bestD)) {
                orderOn(other);
                orderSpoken(false);
            }
            else if (best >= 0)
                world_->select({best}), selectedBuilding_ = 0;
            else if (map_.screenToGround(ev.mouseX, ev.mouseY, gx, gy)) {
                if (!world_->selection().empty()) {
                    for (int id : world_->selection())
                        if (fieldWork_)
                            fieldWork_->cancel(id);
                    // A move fires on the way (cloaking units hold fire);
                    // attack en route fires, cloaked or not; protect holds
                    // the spot.
                    world_->moveSelected(gx, gy, pendingOrder_ == 'a'   ? UnitOrder::EnRoute
                                                 : pendingOrder_ == 'p' ? UnitOrder::Protect
                                                                        : UnitOrder::Move);
                    if (pendingOrder_ == 'p')
                        world_->protect(world_->selection());
                    pendingOrder_ = 0;
                    orderSpoken(true);
                }
                else if (ev.clicks >= 2)
                    map_.lockOn(gx, gy);
            }
        }
        showUnits();
        break;
    }
    case EventType::MouseWheel: map_.handleEvent(ev); return true;
    default: return true;
    }
    redraw();
    updateTitle();
    return true;
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
                                    ev.mouseX >= continue_[0] && ev.mouseY >= continue_[1] &&
                                    ev.mouseX < continue_[0] + continue_[2] && ev.mouseY < continue_[1] + continue_[3];
            if (onContinue || (key && ev.key == key::Return))
                endFlow();
            return;
        }
        if (flowScreen())
            return;
    }
    if (playing_ && playEvent(ev))
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
        const double dt = paused_ || unfocused_ ? 0.0 : std::min(double(nowMs - lastTickMs_) / 1000.0, 0.25);
        lastTickMs_ = nowMs;
        // The mission moves on (mission_sim.h); the screen shows what it
        // did and what its script said.
        sim_->advance(dt);
        // The pointer on the screen's outermost pixels scrolls the
        // isometric cameras that way, speeding up 80 a tick to 6400.
        edgeClock_ += dt * 20;
        for (; edgeClock_ >= 1; edgeClock_ -= 1) {
            const int ex = mouseX_ <= 0 ? -1 : mouseX_ >= screen_.w - 1 ? 1 : 0;
            const int ey = mouseY_ <= 0 ? 1 : mouseY_ >= screen_.h - 1 ? -1 : 0;
            if ((ex || ey) && map_.view() != MapViewer::View::FirstPerson && mouseSeen_) {
                edgeSpeed_ = std::min(kEdgeTop, edgeSpeed_ + kEdgeRamp);
                map_.pan(float(ex) * edgeSpeed_, float(ey) * edgeSpeed_);
            } else if ((ex || ey) && mouseSeen_ && followed_ == 0) {
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
        if (others != shownOthers_) {
            shownOthers_ = std::move(others);
            showColony();
        }
        showUnits();
        combatStep();
        showProgress();
        for (const MissionSim::Said& said : sim_->takeSaid())
            deliver(said.fired, said.out);
        for (const std::string& v : sim_->takeVoices())
            speak(v);
        // F1 following an object: the camera on it, till it's gone.
        if (map_.following()) {
            const Unit* f = world_->unit(followed_);
            if (!f || f->hp <= 0 || map_.view() != MapViewer::View::FirstPerson)
                map_.stopFollowing();
            else
                map_.follow(f->x, f->y, f->heading, f->kind.model);
        }
        // A new selection: its first vehicle answers; MULTISELECTION starts
        // at its first page, nothing ticked.
        if (world_->selection() != spokenSelection_) {
            spokenSelection_ = world_->selection();
            multiTicked_.clear();
            multiPage_ = 0;
            if (!spokenSelection_.empty())
                unitSpeaks(spokenSelection_[0], 0);
        }
        updateGhost(); // the camera may have moved under the pointer
        if (economy_ && nowMs / 1000 != (nowMs - uint64_t(dt * 1000)) / 1000)
            redraw(); // the panels' figures, once a second
    }
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
    if (playing_) {
        drawCombat();
        if (interface_)
            drawInterface();
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
    t += cadetMode() ? "; Cadet mode (K)" : "";
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
        t += "; playing: " + std::to_string(world_->selection().size()) + " selected (F3 or Escape: stop)";
    if (playing_ && economy_)
        t += "; power " + std::to_string(int(economy_->powerSupply())) + "/" +
             std::to_string(int(economy_->powerDemand())) + " (B: build)";
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

// The Tricorder's map (exe.md#the-tricorder): blocks the side hasn't
// surveyed, its surveyed ground, and the deposits it has surveyed by ore,
// in the map's palette shifted for the race.
void MissionViewer::drawTricorderMap(int x, int y, int w, int h, int tiles) {
    const char race = missionSides(missions_[size_t(current_)].shortName)[size_t(side_)];
    map_.drawTricorder(screen_, x, y, w, h, tiles, race, [&](int bx, int by) {
        if (fog_ && !fog_->has(bx, by, Fog::Surveyed, playerSlot_))
            return 1;
        const int kind = map_.depositAt(bx, by);
        return kind >= 1 && kind <= 6 && map_.depositAmount(bx, by) > 0 ? 1 + kind : 0;
    }, [&](int bx, int by) { return !fog_ || fog_->has(bx, by, Fog::Seen, playerSlot_); });
}

// The playbook a double click opens (exe.md#orders): for another side's
// object, the attack orders (and capture, for armed APCs on a building),
// or the scans when the selection is all scouts and science vehicles; for
// one of the colony's buildings with mobile shield generators selected,
// protect area; otherwise, on the ground, the move orders.
void MissionViewer::openPlaybook(int target, float gx, float gy) {
    playbook_.clear();
    const std::vector<int> sel = world_->selection();
    if (sel.empty())
        return;
    bool scanners = true, apcs = false, shields = false;
    for (int id : sel)
        if (const Unit* u = world_->unit(id)) {
            scanners = scanners && FieldWork::canScan(*u);
            apcs = apcs || FieldWork::canCapture(*u);
            shields = shields || u->name == "mobile-shield-generator";
        }
    const Unit* t = target ? world_->unit(target) : nullptr;
    // By the player's posture towards it (exe.md#scans-and-captures): on a
    // hostile one, scans (scouts and science vehicles) or attacks, and a
    // building's capture (armed APCs); on a neutral one only the scans; on
    // a friendly one nothing (as the ground). Props offer nothing.
    const int posture = t && t->slot != playerSlot_ ? world_->stance(playerSlot_, t->slot) : 1;
    const CombatType* ct = t ? combatType(t->type) : nullptr;
    if (t && ct && (ct->mask & 0x800000))
        t = nullptr;
    if (t && t->slot != playerSlot_ && (posture == 3 || (posture == 2 && scanners))) {
        if (scanners)
            playbook_ = {{"science", "SCIENCE SCAN", 's'}, {"medical", "MEDICAL SCAN", 'm'}, {"engine", "ENGINEERING SCAN", 'e'}};
        else
            playbook_ = {{"pbattack", "FOLLOW AND ATTACK ENEMY", 'f'}, {"pbbombrd", "ASSAULT ENEMY/LOCATION", 'b'}};
        if (!scanners && apcs && t->building && posture == 3)
            playbook_.push_back({"pbcaptre", "CAPTURE BUILDING", 'c'});
    } else if (t && t->building && t->slot == playerSlot_ && shields) {
        playbook_ = {{"pbprohre", "PROTECT AREA", 'a'}};
    } else {
        t = nullptr;
        playbook_ = {{"pbdefend", "ATTACK EN ROUTE", 'r'},
                     {"pbprotct", "PROTECT FROM LOCATION", 'l'},
                     {"pbmove", "MOVE WITHOUT ATTACKING", 'v'},
                     {"pbreturn", "RETURN TO VEHICLE RALLY POINT", 'y'}};
    }
    playbookTarget_ = t ? t->id : 0;
    playbookX_ = gx, playbookY_ = gy;
}

void MissionViewer::playbookOrder(char order) {
    const int target = playbookTarget_;
    playbook_.clear();
    if (!world_ || world_->selection().empty())
        return;
    switch (order) {
    case 'f':
    case 'b':
    case 'c':
    case 's':
    case 'm':
    case 'e':
        if (world_->unit(target)) {
            pendingOrder_ = order == 'f' ? 0 : order == 'b' ? 'a' : order;
            orderOn(target);
        }
        break;
    case 'a': {
        std::vector<int> shields;
        for (int id : world_->selection())
            if (const Unit* u = world_->unit(id); u && u->name == "mobile-shield-generator")
                shields.push_back(id);
        world_->protect(shields, {target});
        break;
    }
    case 'y': rallySelected(); break;
    default:
        for (int id : world_->selection())
            if (fieldWork_)
                fieldWork_->cancel(id);
        world_->moveSelected(playbookX_, playbookY_, order == 'r'   ? UnitOrder::EnRoute
                                                     : order == 'l' ? UnitOrder::Protect
                                                                    : UnitOrder::Move);
        if (order == 'l')
            world_->protect(world_->selection());
    }
    showUnits();
}

// The selected armed vehicles back to the rally point.
void MissionViewer::rallySelected() {
    float hx = 0, hy = 0;
    if (!hubPosition(hx, hy))
        return;
    for (int id : world_->selection())
        if (const Unit* u = world_->unit(id); u && u->weapon.kind != WeaponKind::None) {
            const auto [rx, ry] = rallySpot(hx, hy, rallied_++);
            const auto [px, py] = world_->openSpot(rx, ry);
            world_->rally(id, px, py);
        }
}

// The playbook: its icons in a column left of the Tricorder; the one
// under the pointer named in a bar beside it.
void MissionViewer::drawPlaybook() {
    if (playbook_.empty())
        return;
    constexpr int kIcon = 32, kGap = 4;
    const int n = int(playbook_.size());
    const int x = screen_.w - sideW_ - kIcon - 8, y0 = screen_.h - n * (kIcon + kGap) - 4;
    playbookAt_ = {x, y0};
    for (int i = 0; i < n; ++i) {
        const int y = y0 + i * (kIcon + kGap);
        if (const Texture* t = ui_.icon(playbook_[size_t(i)].icon))
            ui_.drawPicture(screen_, *t, x, y, kIcon, kIcon);
        else
            ui_.drawSmall(screen_, x, y + kIcon / 2, std::string(1, playbook_[size_t(i)].order), ui_.brightColor());
        if (mouseX_ >= x && mouseX_ < x + kIcon && mouseY_ >= y && mouseY_ < y + kIcon) {
            const std::string name = playbook_[size_t(i)].name;
            const int w = ui_.smallWidth(name) + 10, bx = x - w - 4, by = y + kIcon / 2 - ui_.smallHeight() / 2 - 3;
            for (int yy = by; yy < by + ui_.smallHeight() + 6; ++yy)
                for (int xx = std::max(0, bx); xx < bx + w && xx < screen_.w; ++xx) {
                    uint8_t* p = screen_.row(yy) + xx * 4;
                    p[0] = 30, p[1] = 120, p[2] = 40;
                }
            ui_.drawSmall(screen_, bx + 5, by + 3, name, Color{220, 255, 220});
        }
    }
}

// A click on a panel's button: true if one was hit.
bool MissionViewer::pressButton(int x, int y, bool right, int clicks) {
    for (const PanelButton& b : buttons_) {
        if (x < b.x || y < b.y || x >= b.x + b.w || y >= b.y + b.h)
            continue;
        const bool entry = b.action.rfind("msel", 0) == 0 && b.action.size() > 4 && std::isdigit(uint8_t(b.action[4]));
        if (right && !entry)
            return false;
        const Unit* u = selectedBuilding_ && world_ ? world_->unit(selectedBuilding_) : nullptr;
        if (entry && world_) {
            // A MULTISELECTION entry: right click ticks it; a click selects
            // it alone; a double click is a double click on it.
            const int id = std::atoi(b.action.c_str() + 4);
            if (right) {
                if (!multiTicked_.erase(id))
                    multiTicked_.insert(id);
            } else if (clicks >= 2) {
                doubleClicked(id);
            } else {
                world_->select({id});
                selectedBuilding_ = 0;
            }
        } else if (b.action == "msel-delete" && world_) {
            std::vector<int> kept;
            for (int id : world_->selection())
                if (!multiTicked_.count(id))
                    kept.push_back(id);
            world_->select(kept);
            multiTicked_.clear();
        } else if (b.action == "msel-invert" && world_) {
            std::set<int> flipped;
            for (int id : world_->selection())
                if (!multiTicked_.count(id))
                    flipped.insert(id);
            multiTicked_ = std::move(flipped);
        } else if (b.action == "msel-prev" || b.action == "msel-next") {
            multiPage_ += b.action == "msel-next" ? 1 : -1;
        } else if (b.action == "staff") {
            staffView_ = !staffView_;
            trainingView_ = false;
            pickedOfficer_ = -1;
        } else if (b.action == "training") {
            trainingView_ = true;
        } else if (b.action.rfind("course", 0) == 0 && fieldWork_) {
            fieldWork_->train(size_t(std::atoi(b.action.c_str() + 6)));
        } else if (b.action.rfind("menu-item", 0) == 0) {
            const size_t n = size_t(std::atoi(b.action.c_str() + 9));
            menuFlash_ = {int(n), nowMs_};
            if (buildMenu_) {
                const auto menu = buildMenu();
                if (n < menu.size())
                    choose(*menu[n]);
            } else if (vehicleMenu_ && economy_) {
                const auto menu = vehicleMenu();
                if (n < menu.size())
                    economy_->orderVehicle(*menu[n]);
            }
        } else if (b.action == "menu-prev" || b.action == "menu-next") {
            menuPage_ += b.action == "menu-next" ? 1 : -1;
        } else if (b.action == "menu-close") {
            buildMenu_ = vehicleMenu_ = false;
        } else if (b.action.rfind("comms-", 0) == 0) {
            const int n = int(log_.size());
            const int shown = commsShown_ < 0 || commsShown_ >= n ? n - 1 : commsShown_;
            if (b.action == "comms-delete" && n > 0) {
                log_.erase(log_.begin() + shown);
                commsShown_ = std::min(shown, int(log_.size()) - 1);
            } else if (b.action == "comms-first" && n > 0) {
                commsShown_ = 0;
            } else if (b.action == "comms-prev" && n > 0) {
                commsShown_ = std::max(0, shown - 1);
            } else if (b.action == "comms-next" && n > 0) {
                commsShown_ = std::min(n - 1, shown + 1);
            } else if (b.action == "comms-last") {
                commsShown_ = -1;
            } else if (b.action == "comms-mission") {
                commandShown_ = !commandShown_;
            } else if (b.action == "comms-close") {
                commsClosed_ = true;
            }
        } else if (b.action == "tri-comms") {
            commsClosed_ = false;
        } else if (b.action == "tri-zoom") {
            tricorderZoom_ = (tricorderZoom_ + 1) % 3;
        } else if (b.action == "tri-ally") {
            allyOpen_ = !allyOpen_, allyPick_ = -1;
        } else if (b.action.rfind("ally-race", 0) == 0) {
            const int slot = std::atoi(b.action.c_str() + 9);
            allyPick_ = allyPick_ == slot ? -1 : slot;
        } else if (b.action.rfind("ally-set", 0) == 0 && world_ && allyPick_ >= 0) {
            // The player's posture towards that race (theirs unchanged);
            // set neutral, it turns hostile at their first shot.
            sim_->setPosture(allyPick_, std::atoi(b.action.c_str() + 8));
            allyPick_ = -1;
        } else if (b.action == "info") {
            colonyDetails_ = !colonyDetails_;
        } else if (b.action.rfind("jump", 0) == 0) {
            jumpTo(std::atoi(b.action.c_str() + 4));
        } else if (b.action == "raze" && u) {
            razeAsked_ = u->id;
        } else if (b.action == "cancel-yes") {
            if (sim_)
                sim_->cancelBuild(cancelAsked_);
            cancelAsked_ = 0;
        } else if (b.action == "cancel-no") {
            cancelAsked_ = 0;
        } else if (b.action.rfind("cancel", 0) == 0) {
            cancelAsked_ = std::atoi(b.action.c_str() + 6);
        } else if (b.action == "raze-no") {
            razeAsked_ = 0;
        } else if (b.action == "raze-yes" && u && economy_) {
            razeAsked_ = 0;
            if (sim_)
                sim_->raze(u->id);
        } else if (b.action == "build") {
            buildMenu_ = !(u && u->name == "vehicle-yard");
            vehicleMenu_ = u && u->name == "vehicle-yard";
            menuPage_ = 0;
        } else if (b.action == "research") {
            researchMenu_ = true;
        } else if ((b.action == "crew-" || b.action == "crew+") && u && economy_) {
            for (size_t i = 0; i < colonyObjects().size() && i < structureIds().size(); ++i)
                if (colonyObjects()[i] == u->id)
                    for (const ColonyStructure& c : economy_->structures())
                        if (c.id == structureIds()[i]) {
                            const BuildingType* t = buildingType(c.type);
                            const int now = c.target < 0 ? (t ? t->crewSlots : 0) : c.target;
                            economy_->setCrewTarget(c.id, now + (b.action == "crew+" ? 1 : -1));
                        }
        } else if (b.action.rfind("officer", 0) == 0) {
            // A click picks the officer; a second on the same card brings
            // the camera to whatever they're in (exe.md#crew-and-efficiency).
            const int i = std::atoi(b.action.c_str() + 7);
            if (i == pickedOfficer_ && nowMs_ - officerClickMs_ < 400 && fieldWork_ &&
                size_t(i) < fieldWork_->officers().size())
                if (const Unit* in = world_ ? world_->unit(fieldWork_->officers()[size_t(i)].in) : nullptr)
                    cameraTo(*in);
            pickedOfficer_ = i;
            officerClickMs_ = nowMs_;
        }
        redraw();
        return true;
    }
    return false;
}

// The STAFF panel: a card for each of the side's officers: rank and name,
// section and skill, where they are.
std::vector<std::string> MissionViewer::staffCards() const {
    std::vector<std::string> out;
    if (!fieldWork_)
        return out;
    const char race = missionSides(missions_[size_t(current_)].shortName)[size_t(side_)];
    auto upper = [](std::string v) {
        for (char& c : v)
            c = char(std::toupper(static_cast<unsigned char>(c)));
        return v;
    };
    for (const FieldWork::OfficerAt& o : fieldWork_->officers()) {
        out.push_back(upper(std::string(rankTitle(race, o.officer->rank)) + " " + o.officer->name));
        out.push_back(std::string(specialityName(o.officer->speciality)) + ", " + skillWord(o.skill));
        std::string where = "IN ORBIT";
        if (const Unit* u = o.in > 0 && world_ ? world_->unit(o.in) : nullptr) {
            const BuildingType* t = u->building ? buildingType(u->name) : nullptr;
            where = t ? t->panel : u->kind.name;
        }
        out.push_back(where);
    }
    return out;
}

const ColonyStructure* MissionViewer::structureOf(int unit) const {
    for (size_t i = 0; economy_ && i < colonyObjects().size() && i < structureIds().size(); ++i)
        if (colonyObjects()[i] == unit)
            for (const ColonyStructure& s : economy_->structures())
                if (s.id == structureIds()[i])
                    return &s;
    return nullptr;
}

// The TRAINING list: a line for each officer's course, as the game's
// buttons read: who, and train in / training in / trained in their
// speciality (with how far it has got).
std::vector<std::string> MissionViewer::trainingLines() const {
    std::vector<std::string> out;
    if (!fieldWork_)
        return out;
    for (const FieldWork::Course& c : fieldWork_->courses()) {
        const Officer& o = *fieldWork_->officers()[c.officer].officer;
        std::string name = o.name;
        for (char& ch : name)
            ch = char(std::toupper(static_cast<unsigned char>(ch)));
        const std::string what = c.finished  ? "TRAINED IN "
                                 : c.running ? "TRAINING IN "
                                             : "TRAIN IN ";
        out.push_back(name + ": " + what + specialityName(o.speciality) +
                      (c.running ? " " + std::to_string(c.done * 100 / FieldWork::kCoursePoints) + "%" : ""));
    }
    return out;
}

// A selected object's panel, as the game's: its name; its side (the
// holder's, when captured); its state (OPERATIONAL or INCOMPLETE, CAPTURED,
// DESTROYED; a vehicle READY or EN ROUTE); its shield cover (HIGH inside a
// shield field of its side, else BASIC). For the colony's own buildings
// also its efficiency, power, people and what the building keeps: the
// hub and generators show the colony's power (their GENERATOR level by
// type); consumers their own supply and reserve.
std::vector<std::string> MissionViewer::objectPanel(const Unit& u) const {
    std::vector<std::string> out;
    auto upper = [](std::string v) {
        for (char& c : v)
            c = c == '-' ? ' ' : char(std::toupper(static_cast<unsigned char>(c)));
        return v;
    };
    auto pct = [](double v) { return std::to_string(int(std::clamp(v, 0.0, 9.99) * 100)) + "%"; };
    const BuildingType* bt = u.building ? buildingType(u.name) : nullptr;
    out.push_back(bt && u.slot == playerSlot_ ? bt->panel : upper(u.building ? u.name : u.kind.name));
    const int owner = u.heldBy >= 0 ? u.heldBy : u.slot;
    const char race = owner == playerSlot_ ? missionSides(missions_[size_t(current_)].shortName)[size_t(side_)]
                      : ai_                ? ai_->raceOf(owner)
                                           : u.side;
    out.push_back(upper(raceName(race == '?' ? u.side : race)));
    const ColonyStructure* own = structureOf(u.id);
    if (u.hp < 1)
        out.push_back("DESTROYED");
    else if (own && own->evacuating)
        out.push_back("EVACUATING");
    else if (u.heldBy >= 0)
        out.push_back("CAPTURED");
    else if (u.building)
        out.push_back(u.built >= 1 ? "OPERATIONAL" : "INCOMPLETE");
    else
        out.push_back(u.moving() ? "EN ROUTE" : "READY");
    const bool high = fog_ && fog_->has(int(u.x / MapViewer::kBlockUnits), int(u.y / MapViewer::kBlockUnits), Fog::Shielded,
                                       u.slot);
    out.push_back(std::string("SHIELD COVER ") + (u.hp < 1 ? "NONE" : high ? "HIGH" : "BASIC"));
    if (!u.building) {
        if (fieldWork_) {
            if (const std::string work = fieldWork_->status(u.id); !work.empty())
                out.push_back(work);
            for (const Officer* o : fieldWork_->aboard(u.id))
                out.push_back(std::string("OFFICER ") + o->name);
        }
        return out;
    }
    // The colony's own: its structure in the economy.
    const ColonyStructure* c = nullptr;
    for (size_t i = 0; economy_ && i < colonyObjects().size() && i < structureIds().size(); ++i)
        if (colonyObjects()[i] == u.id)
            for (const ColonyStructure& s : economy_->structures())
                if (s.id == structureIds()[i])
                    c = &s;
    if (!c || !bt || u.slot != playerSlot_)
        return out;
    // The hub's INFO: the colony's details in place of its own.
    if (colonyDetails_ && c->type == "colony-hub") {
        const std::vector<std::string> details = colonyDetails();
        out.insert(out.end(), details.begin(), details.end());
        return out;
    }
    out.push_back("EFFICIENCY " + std::to_string(int(economy_->efficiency(*c) * 100)) + "%");
    const bool generator = c->type == "colony-hub" || c->type == "power-generator" || c->type == "advanced-generator";
    if (generator) {
        out.push_back(std::string("GENERATOR ") + (c->type == "colony-hub"           ? "LOW"
                                                  : c->type == "power-generator" ? "MED."
                                                                                  : "HIGH"));
        out.push_back("CONSUMPTION " + pct(economy_->consumption()));
        out.push_back("DEMAND MET " + pct(economy_->demandMet()));
        out.push_back("RESERVE " + pct(economy_->reserve()));
    } else if (bt->bufferCap > 0) {
        out.push_back("CONSUMER");
        out.push_back("SUPPLY " + pct(c->supply > 0.99f ? 1.0 : c->supply));
        const double reserve = (4 * c->fill + c->stored / bt->bufferCap) / 4;
        out.push_back("RESERVE " + pct(reserve > 0.99 ? 1.0 : reserve));
    }
    if (bt->crewSlots > 0 || bt->poolSlots > 0 || c->type == "colony-hub") {
        int officers = 0;
        if (fieldWork_)
            for (const FieldWork::OfficerAt& o : fieldWork_->officers())
                officers += o.in == u.id;
        out.push_back("OFFICERS " + std::to_string(officers));
        out.push_back("CREW " + std::to_string(c->crew));
        if (bt->poolSlots > 0)
            out.push_back("POOL " + std::to_string(c->pool));
    }
    auto bees = [&](const char* kind) {
        auto it = beesHoused().find(kind);
        return std::to_string(it == beesHoused().end() ? 0 : it->second);
    };
    if (c->type == "colony-hub" || c->type == "construction-yard") {
        const int transports = sim_ ? sim_->transports() : 0;
        out.push_back("WORKBEE " + bees("worker-bee"));
        out.push_back("CARGOBEE " + bees("cargo-bee"));
        out.push_back("CNSTRCTNBEE " + bees("construction-bee"));
        out.push_back("TRANSPORT " + std::to_string(transports));
    }
    if (c->type == "vehicle-yard") {
        const Economy::VehicleJob* job = nullptr;
        for (const Economy::VehicleJob& j : economy_->vehicleJobs())
            if (j.yard == c->id && !job)
                job = &j;
        if (job && !economy_->yardRunning(c->id))
            out.push_back("SUSPENDED");
        else if (job)
            if (const VehicleType* v = vehicleType(job->type))
                out.push_back(std::string("MANUFACTURING ") + v->panel + " " +
                              std::to_string(job->total > 0 ? int(100 - std::max(0.0, job->work) * 100 / job->total) : 100) +
                              "%");
    }
    if (c->type == "shield-generator")
        out.push_back(std::string("COVERAGE ") + (u.efficiency > 0.6f ? "HIGH" : u.efficiency > 0.2f ? "MED." : "LOW"));
    // A storage facility its own store; the construction yard the colony's.
    if (c->type == "storage-facility" || c->type == "construction-yard") {
        const Stock st = c->type == "storage-facility" ? economy_->storeOf(c->id) : economy_->stock();
        for (int m = 0; m < kMaterials; ++m)
            out.push_back(std::string(materialName(m)) + " " + std::to_string(st[size_t(m)]));
    }
    if (c->type == "mining-station") {
        const auto ore = economy_->mineStock(c->id);
        for (int k = 1; k <= kOres; ++k)
            out.push_back("RAW " + upper(oreName(k)) + " " + std::to_string(int(ore[size_t(k - 1)])));
    }
    if (c->type == "resource-processor") {
        const auto ore = economy_->oreStock();
        for (int k = 1; k <= kOres; ++k)
            out.push_back("RAW " + upper(oreName(k)) + " " + std::to_string(int(ore[size_t(k - 1)])));
    }
    // The sickbay: the colony's crew (not officers), the dead among them:
    // healthy, sick (infected and alive), dead, and their average health
    // (the dead counting none).
    if (c->type == "sickbay") {
        const int dead = economy_->dead(), assigned = economy_->crew() + economy_->pool() + dead;
        const int infected = effects_ ? effects_->infected() : 0;
        out.push_back("HEALTHY " + std::to_string(std::max(0, assigned - infected)));
        out.push_back("SICK " + std::to_string(std::max(0, infected - dead)));
        out.push_back("DEAD " + std::to_string(dead));
        // (Each out of 256; the dead count nothing.)
        const int64_t health = effects_ ? effects_->healthSum() : int64_t(assigned - dead) * 256;
        out.push_back("AVG. HEALTH " + std::string(assigned ? std::to_string(health * 100 / (int64_t(assigned) * 256)) : "999") + "%");
    }
    // A farm: the colony's life support, as a producer.
    if (c->type == "farm") {
        out.push_back("PRODUCER");
        out.push_back("CONSUMPTION " + std::to_string(int(std::lround(economy_->lifeConsumption() * 100))) + "%");
        out.push_back("DEMAND MET " + std::to_string(int(std::lround(economy_->lifeDemandMet() * 100))) + "%");
        out.push_back("RESERVE " + std::to_string(int(std::lround(economy_->lifeSupport() * 100))) + "%");
    }
    return out;
}

// The buildings and upgrades going up, in the colony details' order
// (buildings, then upgrades).
std::vector<int> MissionViewer::pendingBuilds() const {
    std::vector<int> builds, upgrades;
    if (economy_)
        for (const ColonyStructure& s : economy_->structures())
            if (s.progress < 1) {
                const BuildingType* t = buildingType(s.type);
                (t && !t->upgradeOf.empty() ? upgrades : builds).push_back(s.id);
            }
    builds.insert(builds.end(), upgrades.begin(), upgrades.end());
    return builds;
}

// The hub's INFO page (the game's colony details): the colony's people,
// then each building and upgrade going up.
std::vector<std::string> MissionViewer::colonyDetails() const {
    std::vector<std::string> out{"COLONY DETAILS"};
    if (!economy_)
        return out;
    out.push_back("PEOPLE " + std::to_string(economy_->people()) + " IN ORBIT " + std::to_string(economy_->inOrbit()));
    out.push_back("WORKING " + std::to_string(economy_->crew()) + " POOL " + std::to_string(economy_->pool()));
    out.push_back("DEAD " + std::to_string(economy_->dead()));
    std::vector<const ColonyStructure*> builds, upgrades;
    for (const ColonyStructure& s : economy_->structures())
        if (s.progress < 1) {
            const BuildingType* t = buildingType(s.type);
            (t && !t->upgradeOf.empty() ? upgrades : builds).push_back(&s);
        }
    auto list = [&](const std::vector<const ColonyStructure*>& v, const char* what) {
        for (size_t i = 0; i < v.size(); ++i) {
            const BuildingType* t = buildingType(v[i]->type);
            out.push_back(std::string(what) + std::to_string(i + 1) + " OF " + std::to_string(v.size()) + " " +
                          (t ? t->panel : v[i]->type) + " " + std::to_string(int(v[i]->progress * 100)) + "%");
        }
    };
    list(builds, "BUILDING ");
    list(upgrades, "ENGINEERING ");
    return out;
}

// JUMP TO, a left slot: the camera to the next of the colony's objects of
// its class (round again after the last), those being razed or destroyed
// skipped; a right slot: battle group n selected, and the camera on it.
void MissionViewer::jumpTo(int slot) {
    if (!world_)
        return;
    if (slot >= 5) {
        float x = 0, y = 0;
        if (world_->selectGroup(slot - 4) && world_->groupCentre(slot - 4, x, y))
            map_.centreOn(x, y);
        showUnits();
        return;
    }
    const char side = missionSides(missions_[size_t(current_)].shortName)[size_t(side_)];
    const int race = side == 'K' ? 1 : side == 'R' ? 2 : 0;
    auto matches = [&](const Unit& u) {
        if (u.slot != playerSlot_ || u.hp <= 0 || (sim_ && sim_->razing(u.id)))
            return false;
        const CombatType* t = combatType(u.type);
        if (slot == 1)
            return t && (t->mask & 0x8) != 0; // the mines
        if (slot == 4)
            return t && (t->mask & 0x4440100) != 0; // fighting, science and scouting vehicles
        return u.type == kJumpIcons[race][slot];
    };
    std::vector<const Unit*> all;
    for (const Unit& u : world_->units())
        if (matches(u))
            all.push_back(&u);
    if (all.empty())
        return;
    size_t next = 0;
    for (size_t i = 0; i < all.size(); ++i)
        if (all[i]->id == jumpedTo_[size_t(slot)])
            next = (i + 1) % all.size();
    jumpedTo_[size_t(slot)] = all[next]->id;
    cameraTo(*all[next]);
}

// The compass strip, as the game's: a character a degree round the
// camera's heading (about 13 each way), "+" every 5, a three-digit label
// every 10 ("350", "010"), the quarters bracketed ("[ N ]"), dashes
// between.
std::string MissionViewer::compassStrip() const {
    constexpr int kHalf = 14;
    const int heading = int(std::lround(map_.heading()));
    std::string strip(2 * kHalf + 1, '-');
    auto put = [&](int at, const std::string& label) {
        for (size_t i = 0; i < label.size(); ++i)
            if (const int k = at - int(label.size() / 2) + int(i); k >= 0 && k < int(strip.size()))
                strip[size_t(k)] = label[i];
    };
    for (int d = -kHalf - 2; d <= kHalf + 2; ++d)
        if (((heading + d) % 360 + 360) % 360 % 10 == 5)
            put(d + kHalf, "+");
    for (int d = -kHalf - 2; d <= kHalf + 2; ++d) {
        const int deg = ((heading + d) % 360 + 360) % 360;
        static const char* kPoints[] = {"N", "E", "S", "W"};
        if (deg % 90 == 0)
            put(d + kHalf, std::string("[ ") + kPoints[deg / 90] + " ]");
        else if (deg % 10 == 0) {
            char label[4];
            std::snprintf(label, sizeof label, "%03d", deg);
            put(d + kHalf, label);
        }
    }
    return strip;
}

// Where the full-screen Tricorder (Tab) draws its map, and its zoom.
void MissionViewer::fullTricorderRect(int& x0, int& y0, int& size, int& tiles) const {
    size = std::min(screen_.w, screen_.h) - 40;
    x0 = (screen_.w - size) / 2, y0 = (screen_.h - size) / 2;
    // (Twice as many tiles across as the small one.)
    tiles = kTricorderTiles << (tricorderZoom_ + 1);
}

// What play adds over the map: the full-screen Tricorder (Tab), with the
// units on it, and the box a left drag draws.
void MissionViewer::drawPlay() {
    if (tricorderFull_ && hasMap_) {
        int x0 = 0, y0 = 0, size = 0, tiles = 0;
        fullTricorderRect(x0, y0, size, tiles);
        for (int y = 0; y < screen_.h; ++y) {
            uint8_t* row = screen_.row(y);
            for (int x = 0; x < screen_.w; ++x, row += 4)
                row[0] = uint8_t(row[0] / 4), row[1] = uint8_t(row[1] / 4), row[2] = uint8_t(row[2] / 4);
        }
        drawTricorderMap(x0, y0, size, size, tiles);
        // The cross the readout at the top reads under: the camera's target
        // (the game keeps it at its map's centre).
        {
            float cx = 0, cy = 0;
            int px = 0, py = 0;
            map_.cameraTarget(cx, cy);
            if (map_.minimapPoint(cx, cy, x0, y0, size, size, px, py, tiles))
                for (int d = -5; d <= 5; ++d)
                    for (const auto& [qx, qy] : {std::pair{px + d, py}, std::pair{px, py + d}})
                        if (qx >= 0 && qy >= 0 && qx < screen_.w && qy < screen_.h) {
                            uint8_t* q = screen_.row(qy) + qx * 4;
                            q[0] = q[1] = q[2] = 255;
                        }
        }
        // Its readout along the top, in the compass's place: the map under
        // the cross, the camera's target, not the pointer (play-fed-lp.md).
        {
            float cx = 0, cy = 0;
            map_.cameraTarget(cx, cy);
            const std::string line = tricorderReadoutAt(cx, cy, 1500.0f);
            ui_.drawSmall(screen_, (screen_.w - ui_.smallWidth(line)) / 2, 2, line, Color{220, 220, 225});
        }
        // What the side sees: its own green (selected orange), those
        // hostile to it red, the rest yellow.
        if (world_)
            for (const Unit& u : world_->units()) {
                if (!visible(u))
                    continue;
                int px = 0, py = 0;
                if (!map_.minimapPoint(u.x, u.y, x0, y0, size, size, px, py, tiles))
                    continue;
                const bool own = u.slot == playerSlot_, enemy = world_->stance(u.slot, playerSlot_) == 3;
                const uint8_t r = u.selected ? 255 : own ? 120 : 255;
                const uint8_t g = u.selected ? 150 : own ? 255 : enemy ? 70 : 230;
                const uint8_t b = u.selected ? 30 : own ? 120 : 60;
                for (int dy = -2; dy <= 2; ++dy)
                    for (int dx = -2; dx <= 2; ++dx)
                        if (px + dx >= 0 && py + dy >= 0 && px + dx < screen_.w && py + dy < screen_.h) {
                            uint8_t* p = screen_.row(py + dy) + (px + dx) * 4;
                            p[0] = r, p[1] = g, p[2] = b;
                        }
            }
    }
    if (boxing_ && (std::abs(mouseX_ - boxX_) > 6 || std::abs(mouseY_ - boxY_) > 6)) {
        const int x0 = std::min(boxX_, mouseX_), x1 = std::max(boxX_, mouseX_);
        const int y0 = std::min(boxY_, mouseY_), y1 = std::max(boxY_, mouseY_);
        auto put = [&](int x, int y) {
            if (x >= 0 && y >= 0 && x < screen_.w && y < screen_.h) {
                uint8_t* p = screen_.row(y) + x * 4;
                p[0] = 120, p[1] = 255, p[2] = 120;
            }
        };
        for (int x = x0; x <= x1; ++x)
            put(x, y0), put(x, y1);
        for (int y = y0; y <= y1; ++y)
            put(x0, y), put(x1, y);
    }
    // Escape's box (Reluva's wording).
    if (quitAsked_) {
        const int lh = ui_.smallHeight() + 2, w = std::max(180, ui_.smallWidth("LEAVE THIS MISSION?") + 40), h = 30 + lh * 3;
        const int x = (screen_.w - w) / 2, y = (screen_.h - h) / 2;
        ui_.drawPanel(screen_, x, y, w, h, "QUIT");
        const std::string q = "LEAVE THIS MISSION?";
        ui_.drawSmall(screen_, x + (w - ui_.smallWidth(q)) / 2, y + 18, q, ui_.brightColor());
        const int by = y + 18 + lh * 2, yw = ui_.smallWidth("YES") + 12, nw = ui_.smallWidth("NO") + 12;
        quitYes_[0] = x + w / 4 - yw / 2, quitYes_[1] = by - 2, quitYes_[2] = yw, quitYes_[3] = lh + 4;
        quitNo_[0] = x + 3 * w / 4 - nw / 2, quitNo_[1] = by - 2, quitNo_[2] = nw, quitNo_[3] = lh + 4;
        ui_.drawSmall(screen_, quitYes_[0] + 6, by, "YES", ui_.brightColor());
        ui_.drawSmall(screen_, quitNo_[0] + 6, by, "NO", ui_.brightColor());
    }
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
                                ui_.smallWidth("TERRAIN AT <000:000>"), ui_.smallWidth("ENGINEERING, HIGHLY SKILLED") + 24}) +
                      22;
    sideW_ = sideW;
    buttons_.clear();

    // The strip along the top: the compass, or with the full-screen
    // Tricorder up, its readout (the compass moves to the
    // small Tricorder's line; play-14east.md, play-fed-lp.md).
    if (hasMap_) {
        const Color pale{220, 220, 225}; // the game's strip is a pale grey throughout
        if (!tricorderFull_) { // (with it up, drawPlay() puts its readout here)
            const std::string strip = compassStrip();
            const int adv = ui_.smallWidth("-");
            int x = screen_.w / 2 - adv * int(strip.size() / 2) - adv / 2;
            for (size_t i = 0; i < strip.size(); ++i, x += adv)
                ui_.drawSmall(screen_, x, 2, std::string(1, strip[i]), pale);
        }
    }

    // NEW WORLDS: the colony panel: the colony's size and stock (in play
    // the economy's; else what every recording started with), the
    // selection, or the build menu.
    {
        const auto menu = buildMenu_ ? buildMenu() : std::vector<const BuildingType*>{};
        const auto offered = researchMenu_ && research_ ? research_->offered() : std::vector<size_t>{};
        const auto vehicles = vehicleMenu_ && economy_ ? vehicleMenu() : std::vector<const VehicleType*>{};
        const size_t jobs = vehicleMenu_ && economy_ ? economy_->vehicleJobs().size() : 0;
        const Unit* picked = selectedBuilding_ && world_ ? world_->unit(selectedBuilding_) : nullptr;
        if (!picked && world_ && world_->selection().size() == 1 && !buildMenu_ && !researchMenu_ && !vehicleMenu_)
            picked = world_->unit(world_->selection()[0]);
        const std::vector<std::string> info = picked ? objectPanel(*picked) : std::vector<std::string>{};
        // The building's buttons, as the game's (BUILD, STAFF on the hub,
        // the labs' research, the crew assigned).
        std::vector<std::pair<std::string, std::string>> actions; // label, action
        if (picked && picked->building && picked->slot == playerSlot_ && picked->built >= 1) {
            const BuildingType* bt = buildingType(picked->name);
            if (picked->name == "colony-hub")
                actions.push_back({"STAFF", "staff"}), actions.push_back({"INFO", "info"});
            if (picked->name == "colony-hub" || picked->name == "construction-yard" || picked->name == "vehicle-yard")
                actions.push_back({"BUILD", "build"});
            if (picked->name == "science-station" || picked->name == "sickbay")
                actions.push_back({"RESEARCH", "research"});
            if (bt && bt->crewSlots > 0) {
                actions.push_back({"- CREW", "crew-"});
                actions.push_back({"+ CREW", "crew+"});
            }
            if (const ColonyStructure* c = structureOf(picked->id); c && economy_->razable(c->id))
                actions.push_back({"DEMOLISH", "raze"});
            // DEMOLISH asks first; so does cancelling a site.
            if (razeAsked_ == picked->id)
                actions = {{"DEMOLISH? YES", "raze-yes"}, {"NO", "raze-no"}};
            if (cancelAsked_ && colonyDetails_)
                actions = {{"CANCEL IT? YES", "cancel-yes"}, {"NO", "cancel-no"}};
        }
        const std::vector<std::string> cards = staffView_ ? staffCards() : std::vector<std::string>{};
        const std::vector<std::string> training = staffView_ && trainingView_ ? trainingLines() : std::vector<std::string>{};
        const int lines = buildMenu_      ? menuLines(lh)
                          : researchMenu_ ? std::max(2, int(offered.size()) + 1)
                          : vehicleMenu_  ? menuLines(lh) + int(std::min<size_t>(jobs, 4))
                          : trainingView_ && staffView_ ? std::max(2, int(training.size()) + 1)
                          : staffView_    ? int(cards.size()) + 2
                          : !info.empty() ? int(info.size()) + 1 + (pendingOrder_ ? 1 : 0) + (actions.empty() ? 0 : 2)
                          : world_ && world_->selection().size() > 1
                              ? 3 + (pendingOrder_ ? 1 : 0) +
                                    (kMultiCellH * int((std::min<size_t>(world_->selection().size(), kMultiPage) + 2) / 3) + lh - 1) / lh
                          : economy_ ? 9 + 5 * kJumpRows : 7;
        const int x = screen_.w - sideW, y = 0, w = sideW, h = 24 + lines * lh + 8;
        ui_.drawPanel(screen_, x, y, w, h, vehicleMenu_ && !buildMenu_ ? "ENGINEERING" : "NEW WORLDS");
        // The Romulans start with a little less (most of their recordings).
        static const char* const kNames[] = {"DILITHIUM", "POLYCOMP", "THERMOCONC", "TRANSALUMIN", "NITRIUM", "DURANIUM"};
        static constexpr int kStart[] = {500, 1000, 2300, 500, 4000, 5000}, kRomulan[] = {500, 988, 2211, 497, 3500, 5000};
        std::pair<const char*, int> kStock[6];
        for (int i = 0; i < 6; ++i)
            kStock[i] = {kNames[i], side == 'R' ? kRomulan[i] : kStart[i]};
        int ty = y + 16;
        const std::vector<int> sel = world_ ? world_->selection() : std::vector<int>{};
        const BuildingType* placing = placing_.empty() ? nullptr : buildingType(placing_);
        if ((vehicleMenu_ && economy_) || buildMenu_) {
            drawMenu(x, ty, w, buildMenu_ ? std::vector<const VehicleType*>{} : vehicles, menu, jobs);
        } else if (researchMenu_ && research_) {
            // Research: the topics open, each with its key, and how far
            // it has got (as the game's topic buttons: not started, in
            // progress, finished).
            ui_.drawSmall(screen_, x + 10, ty, "RESEARCH", bright);
            if (offered.empty())
                ui_.drawSmall(screen_, x + 10, ty + lh, "NO TOPICS", text);
            for (size_t i = 0; i < offered.size(); ++i) {
                const Research::Topic& t = research_->topics()[offered[i]];
                std::string name = t.name;
                for (char& c : name)
                    c = c == '-' ? ' ' : char(std::toupper(static_cast<unsigned char>(c)));
                const std::string state =
                    t.running ? std::to_string(t.done * 100 / std::max(1, t.points)) + "%" : "READY";
                ui_.drawSmall(screen_, x + 10, ty + lh * int(i + 1), std::to_string(i + 1) + " " + name + " " + state,
                              t.running ? bright : text);
            }
        } else if (placing) {
            // Placing: the building and what it costs.
            ui_.drawSmall(screen_, x + 10, ty, placing->panel, bright);
            int line = 1;
            for (int m = 0; m < kMaterials; ++m)
                if (placing->costFor(side)[size_t(m)] > 0)
                    ui_.drawSmall(screen_, x + 10, ty + lh * line++,
                                  std::string(materialName(m)) + " = " + std::to_string(placing->costFor(side)[size_t(m)]),
                                  economy_->stock()[size_t(m)] >= placing->costFor(side)[size_t(m)] ? text : Color{255, 80, 60});
        } else if (staffView_ && trainingView_) {
            // Training: each officer's course; a click starts it.
            ui_.drawSmall(screen_, x + 10, ty, "TRAINING", bright);
            if (training.empty())
                ui_.drawSmall(screen_, x + 10, ty + lh, "NO ONE TO TRAIN", text);
            for (size_t i = 0; i < training.size(); ++i) {
                const FieldWork::Course& c = fieldWork_->courses()[i];
                const int cy = ty + lh * int(i + 1);
                ui_.drawSmall(screen_, x + 10, cy, training[i], c.running ? bright : text);
                if (!c.finished && !c.running)
                    buttons_.push_back({x + 6, cy, w - 12, lh, "course" + std::to_string(i)});
            }
        } else if (staffView_) {
            // The officers' cards: each one's own face, then rank
            // and name, section and skill, and where they are. A click picks
            // one: the next of the colony's buildings or vehicles clicked
            // takes them.
            ui_.drawSmall(screen_, x + 10, ty, pickedOfficer_ >= 0 ? "STAFF: CLICK WHERE TO" : "STAFF", bright);
            // TRAINING, lit while a course is still to be had.
            const auto& courses = fieldWork_->courses();
            if (std::any_of(courses.begin(), courses.end(), [](const FieldWork::Course& c) { return !c.finished; })) {
                const int bw = ui_.smallWidth("TRAINING") + 8, bx = x + w - bw - 8;
                ui_.drawSmall(screen_, bx + 4, ty, "TRAINING", ui_.titleColor());
                buttons_.push_back({bx, ty - 2, bw, lh + 4, "training"});
            }
            for (size_t i = 0; i * 3 < cards.size(); ++i) {
                const int cy = ty + lh * int(1 + i * 3);
                const auto& o = fieldWork_->officers()[i];
                if (const Texture* face = ui_.officerFace(o.officer->face))
                    ui_.drawPicture(screen_, *face, x + 8, cy, lh * 3 - 2, lh * 3 - 2);
                for (int k = 0; k < 3; ++k)
                    ui_.drawSmall(screen_, x + 12 + lh * 3, cy + lh * k, cards[i * 3 + size_t(k)],
                                  int(i) == pickedOfficer_ ? bright : k == 0 ? ui_.titleColor() : text);
                buttons_.push_back({x + 6, cy, w - 12, lh * 3, "officer" + std::to_string(i)});
            }
        } else if (picked) {
            // One object, a unit or a building: the game's panel, with its
            // hit points as a bar under it.
            // (On the colony's details, an entry going up is a button: a
            // click asks to cancel it.)
            const std::vector<int> pending = colonyDetails_ ? pendingBuilds() : std::vector<int>{};
            size_t entry = 0;
            for (size_t i = 0; i < info.size(); ++i) {
                ui_.drawSmall(screen_, x + 10, ty + lh * int(i), info[i], i == 0 ? bright : text);
                if (entry < pending.size() &&
                    (info[i].rfind("BUILDING ", 0) == 0 || info[i].rfind("ENGINEERING ", 0) == 0))
                    buttons_.push_back({x + 6, ty + lh * int(i), w - 12, lh, "cancel" + std::to_string(pending[entry++])});
            }
            const int by = ty + lh * int(info.size()) + 2, bw = w - 20;
            const float share = std::clamp(picked->hp / std::max(1.0f, picked->maxHp), 0.0f, 1.0f);
            for (int yy = by; yy < by + 4 && yy < screen_.h; ++yy)
                for (int xx = 0; xx < bw && x + 10 + xx < screen_.w; ++xx) {
                    uint8_t* p = screen_.row(yy) + (x + 10 + xx) * 4;
                    const bool full = xx < int(float(bw) * share);
                    p[0] = full ? uint8_t(share < 0.3f ? 230 : 60) : 40;
                    p[1] = full ? uint8_t(share < 0.3f ? 60 : 210) : 40;
                    p[2] = full ? 70 : 50;
                }
            if (pendingOrder_)
                ui_.drawSmall(screen_, x + 10, by + 6, pendingOrderName(), bright);
            // Its buttons, in a row (wrapping) under the bar.
            int bx = x + 10, byy = by + 8;
            for (const auto& [label, action] : actions) {
                const int bw = ui_.smallWidth(label) + 8;
                if (bx + bw > x + w - 6)
                    bx = x + 10, byy += lh + 4;
                for (int xx = bx; xx < bx + bw; ++xx)
                    for (int yy : {byy - 2, byy + lh + 1})
                        if (yy >= 0 && yy < screen_.h && xx < screen_.w) {
                            uint8_t* p = screen_.row(yy) + xx * 4;
                            p[0] = text.r, p[1] = text.g, p[2] = text.b;
                        }
                ui_.drawSmall(screen_, bx + 4, byy, label, bright);
                buttons_.push_back({bx, byy - 2, bw, lh + 4, action});
                bx += bw + 4;
            }
        } else if (sel.size() > 1) {
            // MULTISELECTION (exe.md#the-multiselection): pages of 15, three
            // across: each object's portrait with its health under it, a
            // frame round those ticked; DELETE drops the ticked from the
            // selection, INVERT flips the ticks.
            const int pages = int((sel.size() + kMultiPage - 1) / kMultiPage);
            multiPage_ = std::clamp(multiPage_, 0, pages - 1);
            ui_.drawSmall(screen_, x + 10, ty,
                          pages > 1 ? "MULTISELECTION " + std::to_string(multiPage_ + 1) + "/" + std::to_string(pages)
                                    : std::string("MULTISELECTION"),
                          bright);
            auto rect = [&](int rx, int ry, int rw, int rh, uint8_t r, uint8_t g, uint8_t b) {
                for (int yy = std::max(0, ry); yy < ry + rh && yy < screen_.h; ++yy)
                    for (int xx = std::max(0, rx); xx < rx + rw && xx < screen_.w; ++xx) {
                        uint8_t* p = screen_.row(yy) + xx * 4;
                        p[0] = r, p[1] = g, p[2] = b;
                    }
            };
            const int gx = x + std::max(4, (w - 3 * kMultiCellW) / 2), gy = ty + lh;
            const size_t first = size_t(multiPage_) * kMultiPage;
            for (size_t i = first; i < sel.size() && i < first + kMultiPage; ++i) {
                const Unit* m = world_->unit(sel[i]);
                if (!m)
                    continue;
                const int cx = gx + int((i - first) % 3) * kMultiCellW, cy = gy + int((i - first) / 3) * kMultiCellH;
                if (multiTicked_.count(m->id)) {
                    rect(cx + 5, cy + 1, 36, 2, 0, 128, 255), rect(cx + 5, cy + 41, 36, 2, 0, 128, 255);
                    rect(cx + 5, cy + 1, 2, 42, 0, 128, 255), rect(cx + 39, cy + 1, 2, 42, 0, 128, 255);
                }
                if (const Texture* t = m->type ? ui_.objectPortrait(portraitOf(m->type)) : nullptr)
                    ui_.drawPicture(screen_, *t, cx + 7, cy + 3, 32, 32);
                else
                    ui_.drawSmall(screen_, cx + 7, cy + 12, m->kind.name.substr(0, 4), text);
                const int green = int(32 * std::clamp(m->hp / std::max(1.0f, m->maxHp), 0.0f, 1.0f));
                rect(cx + 7, cy + 37, green, 3, 0, 255, 0);
                rect(cx + 7 + green, cy + 37, 32 - green, 3, 255, 0, 0);
                buttons_.push_back({cx, cy, kMultiCellW, kMultiCellH, "msel" + std::to_string(m->id)});
            }
            int by = gy + kMultiCellH * int((std::min(sel.size() - first, kMultiPage) + 2) / 3) + 2;
            if (pendingOrder_)
                ui_.drawSmall(screen_, x + 10, by, pendingOrderName(), bright), by += lh;
            int bx = x + 10;
            for (const auto& [label, action] : std::vector<std::pair<std::string, std::string>>{
                     {"<", "msel-prev"}, {"DELETE", "msel-delete"}, {"INVERT", "msel-invert"}, {">", "msel-next"}}) {
                const bool off = (action == "msel-prev" && multiPage_ == 0) || (action == "msel-next" && multiPage_ + 1 >= pages);
                const int bw = ui_.smallWidth(label) + 8;
                ui_.drawSmall(screen_, bx + 4, by + 2, label, off ? text : bright);
                if (!off)
                    buttons_.push_back({bx, by, bw, lh + 4, action});
                bx += bw + 4;
            }
        } else {
            if (economy_) {
                for (int i = 0; i < 6; ++i)
                    kStock[i].second = economy_->stock()[size_t(i)];
                ui_.drawSmall(screen_, x + 10, ty,
                              "COLONY SIZE = " + std::to_string(economy_->colonySize()) + "/" +
                                  std::to_string(economy_->capacity()),
                              bright);
            } else {
                ui_.drawSmall(screen_, x + 10, ty, "COLONY SIZE = 20/50", bright);
            }
            for (const auto& [name, n] : kStock) {
                ty += lh;
                ui_.drawSmall(screen_, x + 10, ty, std::string(name) + " = " + std::to_string(n), text);
            }
            // The colony's people: at work, and waiting in the pools.
            if (economy_)
                ui_.drawSmall(screen_, x + 10, ty + lh,
                              "CREW " + std::to_string(economy_->crew()) + " POOL " + std::to_string(economy_->pool()),
                              text);
            // JUMP TO: on the left a slot for each class of the colony's
            // objects (a click: the camera to the next of them), on the
            // right battle groups 1-5 (their first member's portrait; a
            // click selects the group and goes to it).
            if (economy_ && world_) {
                ty += lh * 2;
                ui_.drawSmall(screen_, x + 10, ty, "JUMP TO", bright);
                ty += lh;
                const int race = side == 'K' ? 1 : side == 'R' ? 2 : 0, icon = lh * kJumpRows - 2;
                for (int i = 0; i < 10; ++i) {
                    const int ix = x + 10 + (i / 5) * (icon + 8), iy = ty + (i % 5) * (icon + 2);
                    int type = i < 5 ? kJumpIcons[race][i] : 0;
                    if (i >= 5)
                        if (const auto& g = world_->group(i - 4); !g.empty())
                            if (const Unit* m = world_->unit(g[0]))
                                type = m->type;
                    const Texture* t = type ? ui_.objectPortrait(portraitOf(type)) : nullptr;
                    if (t)
                        ui_.drawPicture(screen_, *t, ix, iy, icon, icon);
                    for (int xx = ix - 1; xx <= ix + icon; ++xx)
                        for (int yy : {iy - 1, iy + icon})
                            if (xx >= 0 && yy >= 0 && xx < screen_.w && yy < screen_.h) {
                                uint8_t* px = screen_.row(yy) + xx * 4;
                                px[0] = text.r / 2, px[1] = text.g / 2, px[2] = text.b / 2;
                            }
                    for (int yy = iy - 1; yy <= iy + icon; ++yy)
                        for (int xx : {ix - 1, ix + icon})
                            if (xx >= 0 && yy >= 0 && xx < screen_.w && yy < screen_.h) {
                                uint8_t* px = screen_.row(yy) + xx * 4;
                                px[0] = text.r / 2, px[1] = text.g / 2, px[2] = text.b / 2;
                            }
                    if (i >= 5 && !t)
                        ui_.drawSmall(screen_, ix + icon / 2 - 3, iy + icon / 2 - lh / 2, std::to_string(i - 4), text);
                    buttons_.push_back({ix, iy, icon, icon, "jump" + std::to_string(i)});
                }
            }
        }
    }

    // TRICORDER (exe.md#the-tricorder): the map round the camera at its
    // zoom, the side's units on it; a line naming what's under the pointer
    // (else the view and zoom); COMMS, ALLY, ZOOM under it.
    {
        const int x = screen_.w - sideW, w = sideW, h = 102 + lh + 4, y = screen_.h - h;
        ui_.drawPanel(screen_, x, y, w, h, "TRICORDER");
        const int mx = x + 12, my = y + 14, mw = w - 24, mh = h - 34 - 2 * lh;
        const int tiles = kTricorderTiles << tricorderZoom_;
        if (hasMap_) {
            drawTricorderMap(mx, my, mw, mh, tiles);
            tricorder_ = {mx, my, mw, mh};
            // Each side's own colour, whoever it is to the player
            // (exe.md#the-tricorder): the Federation blue, the Klingons
            // orange, the Romulans green, the Taubat and Metar yellow, the
            // Hubrin magenta, the Orions red; an alerted object red. A blip
            // is outlined in black, the outline blinking white on the
            // selected and alerted.
            static constexpr Color kSideColours[] = {{0, 0, 255},   {255, 128, 0}, {0, 255, 0},  {255, 255, 0},
                                                     {255, 0, 255}, {255, 255, 0}, {255, 0, 0}};
            const bool blink = (nowMs_ / 500) % 2 == 1;
            if (world_)
                for (const Unit& u : world_->units()) {
                    if (!visible(u))
                        continue;
                    int px = 0, py = 0;
                    if (!map_.minimapPoint(u.x, u.y, mx, my, mw, mh, px, py, tiles))
                        continue;
                    const bool alerted = u.slot == playerSlot_ && sim_ && sim_->alerted(u.id);
                    const Color fill = alerted                                     ? Color{255, 0, 0}
                                       : u.slot >= 0 && u.slot < int(std::size(kSideColours)) ? kSideColours[u.slot]
                                                                                   : Color{0, 0, 255};
                    const Color edge = blink && (u.selected || alerted) ? Color{255, 255, 255} : Color{0, 0, 0};
                    for (int dy = -1; dy < 3; ++dy)
                        for (int dx = -1; dx < 3; ++dx)
                            if (px + dx >= 0 && py + dy >= 0 && px + dx < screen_.w && py + dy < screen_.h) {
                                const Color c = dx < 0 || dy < 0 || dx > 1 || dy > 1 ? edge : fill;
                                uint8_t* p = screen_.row(py + dy) + (px + dx) * 4;
                                p[0] = c.r, p[1] = c.g, p[2] = c.b;
                            }
                }
            // With the full-screen Tricorder up its line carries the compass.
            ui_.drawSmall(screen_, x + 10, y + h - 14 - 2 * lh,
                          tricorderFull_ ? compassStrip() : tricorderReadout(mx, my, mw, mh, tiles), text);
        }
        int bx = x + 10;
        for (const auto& [label, action] : {std::pair{"COMMS", "tri-comms"}, std::pair{"ALLY", "tri-ally"},
                                            std::pair{"ZOOM", "tri-zoom"}}) {
            const int bw = ui_.smallWidth(label) + 8;
            ui_.drawSmall(screen_, bx + 4, y + h - 11 - lh, label, allyOpen_ && std::string_view(action) == "tri-ally" ? bright : text);
            buttons_.push_back({bx, y + h - 13 - lh, bw, lh + 4, action});
            bx += bw + 4;
        }
        // ALLY: a row for each other race in the mission, the player's
        // posture towards it; a click on one offers FRIENDLY, NEUTRAL,
        // HOSTILE.
        if (allyOpen_ && world_) {
            std::vector<std::string> rows;
            const auto others = allyRaces();
            const int ah = 30 + int(others.size() + (allyPick_ >= 0 ? 1 : 0)) * (lh + 2);
            const int ay = y - ah;
            ui_.drawPanel(screen_, x, ay, w, ah, "ALLY");
            int ry = ay + 15;
            for (int slot : others) {
                const int st = world_->stance(playerSlot_, slot);
                std::string name = raceName(ai_ ? ai_->raceOf(slot) : '?');
                std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return char(std::toupper(c)); });
                const std::string word = st == 1 ? "FRIENDLY" : st == 2 ? "NEUTRAL" : "HOSTILE";
                ui_.drawSmall(screen_, x + 10, ry, name, slot == allyPick_ ? bright : text);
                ui_.drawSmall(screen_, x + w - 12 - ui_.smallWidth(word), ry, word, st == 3 ? Color{255, 90, 70} : text);
                buttons_.push_back({x + 6, ry - 1, w - 12, lh + 2, "ally-race" + std::to_string(slot)});
                ry += lh + 2;
                if (slot == allyPick_) {
                    int ox = x + 14;
                    for (const auto& [label, v] : {std::pair{"FRIENDLY", 1}, std::pair{"NEUTRAL", 2}, std::pair{"HOSTILE", 3}}) {
                        const int bw = ui_.smallWidth(label) + 6;
                        ui_.drawSmall(screen_, ox + 3, ry, label, bright);
                        buttons_.push_back({ox, ry - 1, bw, lh + 2, "ally-set" + std::to_string(v)});
                        ox += bw + 4;
                    }
                    ry += lh + 2;
                }
            }
        }
    }

    // COMMUNICATIONS: a message (the latest, unless the arrows have gone
    // back), with its sender's portrait and n/N; under it DELETE, the
    // first / previous / next / last arrows, MISSION (the objectives) and
    // X (the panel away until the next message).
    if (!commsClosed_) {
        const int w = 220, h = 70 + lh + 4, x = (screen_.w - sideW - w) / 2 + 10, y = screen_.h - h;
        ui_.drawPanel(screen_, x, y, w, h, "COMMUNICATIONS");
        const int n = int(log_.size());
        const int shown = commsShown_ < 0 || commsShown_ >= n ? n - 1 : commsShown_;
        if (log_.empty()) {
            ui_.drawSmall(screen_, x + 60, y + 16, "NO MESSAGES ON FILE.", text);
        } else {
            const auto& [sender, message] = log_[size_t(shown)];
            const std::string natives = level.shortName == "C9" ? "orion" : "taubat";
            // The face the message's officer number gives (the mission's
            // mail for that section), else the section's.
            std::string section = sender.substr(0, sender.find(' '));
            for (char& c : section)
                c = char(std::tolower(static_cast<unsigned char>(c)));
            const char race = missionSides(level.shortName)[size_t(side_)];
            const Texture* face = ui_.messageFace(messageOfficer(level.shortName, race, section));
            if (const Texture* p = face ? face : ui_.portraitFor(sender, natives))
                ui_.drawPicture(screen_, *p, x + 10, y + 13, 44, 41);
            std::string up = sender + ":";
            std::transform(up.begin(), up.end(), up.begin(), [](unsigned char c) { return char(std::toupper(c)); });
            int ty = y + 14;
            ui_.drawSmall(screen_, x + 58, ty, up, text);
            for (const std::string& l : ui_.wrapSmall(message, w - 70)) {
                ty += lh;
                if (ty > y + h - 14 - 2 * lh)
                    break;
                ui_.drawSmall(screen_, x + 58, ty, l, text);
            }
            const std::string counter = std::to_string(shown + 1) + "/" + std::to_string(n);
            ui_.drawSmall(screen_, x + w - 12 - ui_.smallWidth(counter), y + 14, counter, text);
        }
        int bx = x + 10;
        const int by = y + h - 11 - lh;
        for (const auto& [label, action] : {std::pair{"DELETE", "comms-delete"}, std::pair{"|<", "comms-first"},
                                            std::pair{"<", "comms-prev"}, std::pair{">", "comms-next"},
                                            std::pair{">|", "comms-last"}, std::pair{"MISSION", "comms-mission"},
                                            std::pair{"X", "comms-close"}}) {
            const int bw = ui_.smallWidth(label) + 6;
            ui_.drawSmall(screen_, bx + 3, by, label, bright);
            buttons_.push_back({bx, by - 2, bw, lh + 4, action});
            bx += bw + 3;
        }
    }

    // COMMAND: the mission, and its objectives as the recordings' panel
    // lists them (MISSION on the communications panel shows or hides it).
    if (run_ && commandShown_) {
        const MissionScript* script = findMissionScript(level.shortName, side);
        std::vector<std::pair<std::string, Color>> lines;
        const int w = 260;
        {
            std::string title = levelLabel() + (level.title.empty() ? "" : ": " + level.title);
            std::transform(title.begin(), title.end(), title.begin(), [](unsigned char c) { return char(std::toupper(c)); });
            for (const std::string& l : ui_.wrapSmall(title, w - 24))
                lines.push_back({l, bright});
        }
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
    drawPlaybook();
}

int MissionViewer::menuLines(int lh) const {
    // The title, three icons, the hovered item's name, role and cost, the
    // arrows.
    return 1 + (3 * (kMenuIcon + 4) + lh - 1) / lh + 2 + kMaterials + 1;
}

// The build and vehicle menus as the game's: three portraits a page (the
// key that picks each in its corner, its name beside it, plain if the
// stock can't pay), the item under the pointer (else the page's first)
// at the foot with its role and cost, then the arrows, the page and X; a
// clicked icon flashes magenta. The vehicle menu adds the yards' work.
void MissionViewer::drawMenu(int x, int ty, int w, const std::vector<const VehicleType*>& vehicles,
                             const std::vector<const BuildingType*>& buildings, size_t jobs) {
    const int lh = ui_.smallHeight();
    const Color text = ui_.textColor(), bright = ui_.brightColor();
    const Color dim{uint8_t(text.r / 2), uint8_t(text.g / 2), uint8_t(text.b / 2)};
    const char side = missionSides(missions_[size_t(current_)].shortName)[size_t(side_)];
    struct Item {
        std::string portrait, name, role;
        Stock cost;
        bool pays, open;
    };
    std::vector<Item> items;
    const Stock st = economy_ ? economy_->stock() : Stock{};
    auto pays = [&](const Stock& c) {
        for (int m = 0; m < kMaterials; ++m)
            if (st[size_t(m)] < c[size_t(m)])
                return false;
        return true;
    };
    for (const BuildingType* t : buildings) {
        const bool upgrade = !t->upgradeOf.empty();
        items.push_back({buildingPortrait(upgrade ? t->upgradeOf : t->id, side), t->panel,
                         upgrade ? "AN UPGRADE" : roleOf(t->id), t->costFor(side), pays(t->costFor(side)),
                         economy_->available(*t)});
    }
    for (const VehicleType* v : vehicles) {
        const CombatType* ct = combatType(v->type);
        std::string pic = portraitOf(v->type);
        if (pic.size() > 2)
            pic.resize(pic.size() - 2); // the large one
        items.push_back({pic, v->panel, ct ? roleOf(ct->name) : "", v->cost, pays(v->cost), true});
    }
    const int pages = std::max(1, int((items.size() + 2) / 3));
    menuPage_ = std::clamp(menuPage_, 0, pages - 1);
    ui_.drawSmall(screen_, x + 10, ty, buildMenuTitle(), bright);
    if (items.empty())
        ui_.drawSmall(screen_, x + 10, ty + lh, vehicleMenu_ ? "NO VEHICLE YARD" : "NOTHING TO BUILD", text);
    int iy = ty + lh + 2;
    int shown = -1; // the item at the foot
    for (int k = 0; k < 3; ++k) {
        const int i = menuPage_ * 3 + k;
        if (i >= int(items.size()))
            break;
        const Item& it = items[size_t(i)];
        const int ix = x + 10;
        if (const Texture* t = ui_.objectPortrait(it.portrait))
            ui_.drawPicture(screen_, *t, ix, iy, kMenuIconW, kMenuIcon, true);
        const bool flash = menuFlash_.first == i && nowMs_ - menuFlash_.second < 300;
        const bool over = mouseX_ >= ix && mouseX_ < x + w - 6 && mouseY_ >= iy && mouseY_ < iy + kMenuIcon;
        if (over)
            shown = i;
        const Color frame = flash ? Color{255, 60, 255} : over ? bright : dim;
        for (int xx = ix - 1; xx <= ix + kMenuIconW; ++xx)
            for (int yy : {iy - 1, iy + kMenuIcon})
                if (xx >= 0 && yy >= 0 && xx < screen_.w && yy < screen_.h) {
                    uint8_t* px = screen_.row(yy) + xx * 4;
                    px[0] = frame.r, px[1] = frame.g, px[2] = frame.b;
                }
        for (int yy = iy - 1; yy <= iy + kMenuIcon; ++yy)
            for (int xx : {ix - 1, ix + kMenuIconW})
                if (xx >= 0 && yy >= 0 && xx < screen_.w && yy < screen_.h) {
                    uint8_t* px = screen_.row(yy) + xx * 4;
                    px[0] = frame.r, px[1] = frame.g, px[2] = frame.b;
                }
        ui_.drawSmall(screen_, ix + 2, iy + 1, std::to_string((i + 1) % 10), bright);
        int ny = iy;
        for (const std::string& l : ui_.wrapSmall(it.name, w - kMenuIconW - 26)) {
            ui_.drawSmall(screen_, ix + kMenuIconW + 6, ny, l, !it.open ? dim : it.pays ? bright : text);
            ny += lh;
        }
        buttons_.push_back({ix, iy, w - 16, kMenuIcon, "menu-item" + std::to_string(i)});
        iy += kMenuIcon + 4;
    }
    // The foot: what's under the pointer, else the page's first.
    int fy = ty + lh + 2 + 3 * (kMenuIcon + 4);
    if (shown < 0 && menuPage_ * 3 < int(items.size()))
        shown = menuPage_ * 3;
    if (shown >= 0) {
        const Item& it = items[size_t(shown)];
        ui_.drawSmall(screen_, x + 10, fy, it.name, bright);
        ui_.drawSmall(screen_, x + 10, fy + lh, it.role, text);
        int cy = fy + 2 * lh;
        for (const std::string& c : costLines(it.cost)) {
            ui_.drawSmall(screen_, x + 10, cy, c, text);
            cy += lh;
        }
    }
    fy += (2 + kMaterials) * lh;
    // The arrows, the page, X.
    int bx = x + 10;
    for (const auto& [label, action] : {std::pair{"<", "menu-prev"}, std::pair{">", "menu-next"}, std::pair{"X", "menu-close"}}) {
        const int bw = ui_.smallWidth(label) + 10;
        ui_.drawSmall(screen_, bx + 5, fy, label, bright);
        buttons_.push_back({bx, fy - 2, bw, lh + 4, action});
        bx += bw + 6;
    }
    ui_.drawSmall(screen_, bx + 4, fy, std::to_string(menuPage_ + 1) + "/" + std::to_string(pages), text);
    // The yards' work.
    for (size_t i = 0; vehicleMenu_ && economy_ && i < jobs && i < 4; ++i) {
        const Economy::VehicleJob& j = economy_->vehicleJobs()[i];
        const VehicleType* v = vehicleType(j.type);
        const int pct = j.total > 0 ? int(100 - std::max(0.0, j.work) * 100 / j.total) : 100;
        ui_.drawSmall(screen_, x + 10, fy + lh * int(i + 1), std::string("> ") + (v ? v->panel : "?") + " " + std::to_string(pct) + "%",
                      text);
    }
}

const char* MissionViewer::buildMenuTitle() const { return vehicleMenu_ && !buildMenu_ ? "VEHICLES" : "BUILD"; }

// The Tricorder's line (exe.md#the-tricorder): with the pointer over
// it, what's there: one of the side's objects in sight (FRIENDLY,
// NEUTRAL or HOSTILE, by the player's posture to its side, then DEFENSES,
// STRUCTURE or UNIT), ground no unit can cross (IMPASSABLE), a surveyed
// deposit (its ore AT <a:b>) or the ground (TERRAIN AT <a:b>); else the
// view and its zoom (RELIEF X2).
std::string MissionViewer::tricorderReadout(int mx, int my, int mw, int mh, int tiles) const {
    static const char* const kZoom[] = {"X4", "X2", "X1"};
    if (mouseX_ < mx || mouseY_ < my || mouseX_ >= mx + mw || mouseY_ >= my + mh)
        return std::string("RELIEF ") + kZoom[tricorderZoom_];
    float wx = 0, wy = 0;
    map_.minimapToWorld(mouseX_, mouseY_, mx, my, mw, mh, tiles, wx, wy);
    // An object within two pixels' worth of the pointer.
    float x1 = 0, y1 = 0;
    map_.minimapToWorld(mouseX_ + 2, mouseY_, mx, my, mw, mh, tiles, x1, y1);
    return tricorderReadoutAt(wx, wy, std::max(std::fabs(x1 - wx), 1500.0f));
}

// What the Tricorder's line says of a world point (objects within reach).
std::string MissionViewer::tricorderReadoutAt(float wx, float wy, float reach) const {
    if (world_)
        for (const Unit& u : world_->units())
            if (visible(u) && std::hypot(u.x - wx, u.y - wy) < reach) {
                const int st = u.slot == playerSlot_ ? 1 : world_->stance(playerSlot_, u.slot);
                const CombatType* t = combatType(u.type);
                const char* kind = t && (t->mask & 0x10) ? " DEFENSES" : u.building ? " STRUCTURE" : " UNIT";
                return std::string(st == 1 ? "FRIENDLY" : st == 2 ? "NEUTRAL" : "HOSTILE") + kind;
            }
    const int a = int(wy / MapViewer::kBlockUnits * 8), b = int(wx / MapViewer::kBlockUnits * 8);
    auto pad = [](int v) {
        std::string s = std::to_string(v);
        return std::string(s.size() < 3 ? 3 - s.size() : 0, '0') + s;
    };
    if (world_ && !world_->passable(int(wx / kTile), int(wy / kTile)))
        return "IMPASSABLE";
    std::string what = "TERRAIN";
    if (fog_ && fog_->has(b / 8, a / 8, Fog::Surveyed, playerSlot_) && map_.depositAmount(b / 8, a / 8) > 0)
        if (const int kind = map_.depositAt(b / 8, a / 8)) {
            what = kind == 5 ? "MAGNESITE ORE" : oreName(kind);
            std::transform(what.begin(), what.end(), what.begin(), [](unsigned char c) { return char(std::toupper(c)); });
        }
    return what + " AT <" + pad(a) + ":" + pad(b) + ">";
}

// The other races in the mission, one slot each (not the player's).
std::vector<int> MissionViewer::allyRaces() const {
    std::vector<int> out;
    std::set<char> seen;
    if (!world_ || !ai_)
        return out;
    for (const Unit& u : world_->units()) {
        const char race = ai_->raceOf(u.slot);
        if (u.slot != playerSlot_ && race != '?' && seen.insert(race).second)
            out.push_back(u.slot);
    }
    std::sort(out.begin(), out.end());
    return out;
}

// A, G, S, T, P (exe.md#keys): the colony's vehicles of a kind selected.
void MissionViewer::selectAllOfKind(char k) {
    if (!world_)
        return;
    std::vector<int> ids;
    for (const Unit& u : world_->units()) {
        if (u.slot != playerSlot_ || u.building || u.hp <= 0)
            continue;
        const std::string& n = u.name;
        const bool artillery = n.find("artillery") != std::string::npos && n.find("defensive") == std::string::npos;
        const bool is = k == 'a'   ? n.find("apc") != std::string::npos
                        : k == 'g' ? n == "ground-scout"
                        : k == 's' ? n == "science-vehicle"
                        : k == 'p' ? artillery
                                   : (n.find("tank") != std::string::npos || n.find("battery") != std::string::npos);
        if (is)
            ids.push_back(u.id);
    }
    world_->select(ids);
    selectedBuilding_ = 0;
    showUnits();
}

// Shift or Ctrl + B / V: the camera on to the next of the colony's
// buildings, or of its fighting, science and scouting vehicles.
void MissionViewer::cycleCamera(bool buildings) {
    if (!world_)
        return;
    std::vector<const Unit*> all;
    for (const Unit& u : world_->units()) {
        if (u.slot != playerSlot_ || u.hp <= 0 || u.building != buildings)
            continue;
        const CombatType* t = combatType(u.type);
        if (!buildings && !(t && (t->mask & 0x4440100)))
            continue;
        all.push_back(&u);
    }
    if (all.empty())
        return;
    int& last = buildings ? cycledBuilding_ : cycledVehicle_;
    size_t next = 0;
    for (size_t i = 0; i < all.size(); ++i)
        if (all[i]->id == last)
            next = (i + 1) % all.size();
    last = all[next]->id;
    cameraTo(*all[next]);
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
        // the game's own building is the player's).
        if (e.kind == "built" && !e.args.empty() && colony_) {
            if (colony_->add(e.args[0]))
                showColony();
            else
                walkNote_ += " (not placed)";
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
void MissionViewer::deliver(const ScriptEvent& fired, const std::vector<MissionRun::Output>& out) {
    const char side = missionSides(missions_[size_t(current_)].shortName)[size_t(side_)];
    if (playing_ && !out.empty())
        walkNote_ = fired.kind; // in play, the latest that said anything
    // A message's own voice clip first; else the game's announcement.
    std::string voice;
    for (const MissionRun::Output& o : out)
        if (o.kind == MissionRun::Output::Message && !o.clip.empty())
            voice = o.clip;
    if (voice.empty())
        voice = announcementFor(fired, out, side);
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
            commsShown_ = -1, commsClosed_ = false;
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
    const char side = missionSides(missions_[size_t(current_)].shortName)[size_t(side_)];
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
    const auto [it, fresh] = unitVoices_.try_emplace(unit, 0);
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
            if (fs::toLower(a->path().filename().string()) == archive)
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

// A double click on one of the player's objects (exe.md#cameras): a
// finished building plays its own sound (exe.md#sounds), from where it
// stands: full by the camera's spot, fading out 8 blocks off, panned to
// its side; only one at a time. In F1 the camera then follows it; in F2
// and F3 it stays put.
void MissionViewer::doubleClicked(int unit) {
    const Unit* u = world_ ? world_->unit(unit) : nullptr;
    if (!u || u->slot != playerSlot_)
        return;
    if (const auto [archive, stem] = buildingSound(u->type); !archive.empty() && u->built >= 1) {
        if (mixer_)
            mixer_->stop(objectSoundHandle_);
        objectSoundHandle_ = playAt(archive, stem, u->x, u->y, 2);
    }
    followInF1(unit);
}

// F1's camera on to one of the player's objects, following it.
void MissionViewer::followInF1(int unit) {
    const Unit* u = world_ ? world_->unit(unit) : nullptr;
    if (!u || u->slot != playerSlot_ || map_.view() != MapViewer::View::FirstPerson)
        return;
    followed_ = unit;
    map_.startFollowing();
    map_.follow(u->x, u->y, u->heading, u->kind.model);
}

// The camera to one of the player's objects (JUMP TO, Space): in F1 it
// follows it, as after a double click (but without the sound); else it
// centres on it.
void MissionViewer::cameraTo(const Unit& u) {
    if (map_.view() == MapViewer::View::FirstPerson && u.slot == playerSlot_)
        followInF1(u.id);
    else
        map_.centreOn(u.x, u.y);
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

void MissionViewer::stopScript() {
    if (!run_ || sim_)
        return;
    pageRun_.reset();
    run_ = nullptr;
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
    scene_ = -1; // its scenes start afresh
    startStep(0);
}

void MissionViewer::stopBriefing() {
    if (!running_)
        return;
    running_ = false;
    // In a mission's run, play follows (after its loading box).
    if (flow_ == Flow::Briefing) {
        flow_ = Flow::Starting;
        flowStartMs_ = nowMs_;
    }
    stopClip();
    if (!playing_)
        map_.setUnits({}); // (a scene's unit gone with it)
    scene_ = -1;
    map_.setColouring(savedColouring_);
    map_.resetCamera();
    redraw();
    updateTitle();
}

// A briefing scene's unit (briefing_data.h): the mission's briefing script
// flies it from its start along its waypoints, at its height, 100 a tick
// (the shuttles' flight speed). Its model by type: the races' shuttles
// (the Federation's 6, the Romulans' 93, the Klingons' 94 and 38: matched
// by race and speed), else the type's own.
void MissionViewer::flyScene(int index, uint64_t nowMs) {
    const BriefingScene* sc = briefingScene(index);
    if (!sc)
        return;
    auto at = [](int a, int b) { return std::pair{float(b) / 8 * MapViewer::kBlockUnits, float(a) / 8 * MapViewer::kBlockUnits}; };
    if (scene_ != index) {
        scene_ = index, sceneLeg_ = 0, sceneMs_ = nowMs;
        std::tie(sceneX_, sceneY_) = at(sc->a, sc->b);
    }
    double left = double(nowMs - sceneMs_) / 1000 * 2000; // units to fly
    sceneMs_ = nowMs;
    float heading = 0;
    while (left > 0 && sceneLeg_ < sc->ways) {
        const auto [wx, wy] = at(sc->way[sceneLeg_][0], sc->way[sceneLeg_][1]);
        const float dx = wx - sceneX_, dy = wy - sceneY_, d = std::hypot(dx, dy);
        if (d > 1)
            heading = std::atan2(dy, dx);
        if (d <= left) {
            sceneX_ = wx, sceneY_ = wy, left -= d, ++sceneLeg_;
        } else {
            sceneX_ += dx / d * float(left), sceneY_ += dy / d * float(left), left = 0;
        }
    }
    if (heading != 0)
        sceneHeading_ = heading;
    std::string model = sc->type == 6 ? "geo/fed/shuttle.geo"
                        : sc->type == 93 ? "geo/rom/rs.geo"
                        : sc->type == 94 || sc->type == 38 ? "geo/kln/kcs.geo"
                                                           : "";
    if (model.empty())
        if (const CombatType* t = combatType(sc->type))
            model = t->model;
    MapViewer::UnitDraw d{model, sceneX_, sceneY_, sceneHeading_};
    d.lift = sc->height;
    map_.setUnits({d});
    map_.centreOn(sceneX_, sceneY_);
    dirty_ = true;
}

void MissionViewer::startStep(int step) {
    stopClip();
    if (step >= int(script_.size())) {
        stopBriefing();
        return;
    }
    step_ = step;
    const BriefingStep& s = script_[size_t(step)];
    if (s.scene < 0 && !playing_)
        map_.setUnits({});
    // The relief and the Tricorder drawn as the step's scene says, the
    // ground's classes from the map's deposits (nobody's view yet).
    map_.setTricorder(missionSides(missions_[size_t(current_)].shortName)[size_t(side_)],
                      [this](int bx, int by) {
                          const int kind = map_.depositAt(bx, by);
                          return kind >= 1 && kind <= 6 && map_.depositAmount(bx, by) > 0 ? 1 + kind : 0;
                      },
                      {});
    map_.setTricorderScheme(s.scheme);
    stepOnMap_ = hasMap_ && (s.view == BriefingView::Ground || s.view == BriefingView::Relief);
    if (stepOnMap_) {
        map_.setColouring(s.view == BriefingView::Relief ? 1 : 0);
        map_.lookAt(s.siteA, s.siteB, s.view == BriefingView::Ground);
        lastTurnMs_ = nowMs_;
    }
    // Its clip, if the game folder has it; it lasts the longer of the
    // clip and its minimum (5 s if it has neither).
    stepEndMs_ = nowMs_ + uint64_t(1000 * (s.minimum > 0 ? s.minimum : 5.0));
    stepMinimumMs_ = stepEndMs_;
    const auto it = std::find(clips_.begin(), clips_.end(), s.clip);
    if (!s.clip.empty() && it != clips_.end()) {
        playClip(int(it - clips_.begin()));
        if (clipHandle_ != audio::kNoSound)
            stepEndMs_ = 0;
    }
    redraw();
    updateTitle();
}

bool MissionViewer::mapInView() const { return hasMap_ && !flowScreen() && (!running_ || stepOnMap_); }

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

void MissionViewer::startFlow() {
    stopBriefing();
    stopPlay();
    flow_ = Flow::Loading;
    flowStartMs_ = kFlowUnstarted; // (timed from the next update: the clock may be stale)
    dirty_ = true;
}

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
    s(mission)(sideIndex)(cadet)(flowed);
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
        startPlay();
        stopClip(); // the opening's voice, already heard
        if (!sim_)
            return Error::fail("Couldn't start " + mission + ".");
    }
    sim_->syncState(s);
    s(log_)(commsShown_)(commsClosed_)(commandShown_)(shownOthers_)(otherBuildings_)(unitVoices_);
    map_.syncCamera(s);
    if (s.loading()) {
        if (!s.ok())
            return Error::fail("The save is damaged: " + s.error() + ". The mission starts over.");
        flow_ = flowed ? Flow::Playing : Flow::None;
        flowWon_ = false;
        lastTickMs_ = nowMs_;
        quitAsked_ = paused_ = false;
        selectedBuilding_ = 0;
        placing_.clear();
        showColony();
        showUnits();
        dirty_ = true;
        redraw();
        updateTitle();
    }
    return Error::success();
}

void MissionViewer::endFlow() {
    const Flow was = flow_;
    flow_ = Flow::None;
    if (was == Flow::Report && flowWon_ && isCampaignFinale(missions_[size_t(current_)].shortName))
        campaignWon_ = side();
    if (was == Flow::Briefing)
        stopBriefing();
    stopPlay();
    dirty_ = true;
    redraw();
    updateTitle();
}

// The loading boxes' heading: the mission as the game labels it
// ("FEDERATION LEVEL 01", "JOINT LEVEL 03", the tutorials, the bonus
// missions).
std::string MissionViewer::levelLabel() const {
    const std::string& m = missions_[size_t(current_)].shortName;
    const int n = std::atoi(m.c_str() + 1);
    auto two = [](int v) { return (v < 10 ? "0" : "") + std::to_string(v); };
    switch (m.empty() ? ' ' : m[0]) {
    case 'F': return "FEDERATION LEVEL " + two(n);
    case 'K': return "KLINGON LEVEL " + two(n);
    case 'R': return "ROMULAN LEVEL " + two(n);
    case 'J': return "JOINT LEVEL " + two(n);
    case 'C':
        if (n >= 9 && n <= 11)
            return std::string("BONUS ") + (n == 9 ? "FEDERATION" : n == 10 ? "KLINGON" : "ROMULAN") + " MISSION";
        return "TUTORIAL";
    default: return missions_[size_t(current_)].title;
    }
}

// The run's screens: the loading boxes over the three emblems (with the
// mission's label, what's being readied and how far it has got), and the
// end: the side's won or lost screen with the MISSION REPORT (each
// objective complete or not) and CONTINUE.
void MissionViewer::drawFlow() {
    const Level& level = missions_[size_t(current_)];
    const char side = missionSides(level.shortName)[size_t(side_)];
    if (uiSide_ != side) {
        uiSide_ = side;
        ui_ = GameUi{};
        ui_.load(archives_, side);
    }
    for (int y = 0; y < screen_.h; ++y) {
        uint8_t* row = screen_.row(y);
        for (int x = 0; x < screen_.w; ++x, row += 4)
            row[0] = row[1] = row[2] = 0, row[3] = 255;
    }
    auto blit = [&](const Texture& t, int x0, int y0, int w, int h) {
        for (int y = 0; y < h; ++y) {
            if (y0 + y < 0 || y0 + y >= screen_.h)
                continue;
            uint8_t* row = screen_.row(y0 + y);
            const int sy = y * t.height / h;
            for (int x = 0; x < w; ++x) {
                if (x0 + x < 0 || x0 + x >= screen_.w)
                    continue;
                const uint8_t* p = &t.rgba[(size_t(sy) * size_t(t.width) + size_t(x * t.width / w)) * 4];
                uint8_t* o = row + (x0 + x) * 4;
                o[0] = p[0], o[1] = p[1], o[2] = p[2];
            }
        }
    };
    if (!ui_.loaded()) {
        drawText(kPanelMargin, kPanelMargin, "no GUI archive for this side", kNoteColor);
        return;
    }
    const int lh = ui_.smallHeight();
    if (flow_ == Flow::Report) {
        // Won: the race's emblem (the game never shows its f_win tiles);
        // lost: the lose tiles (exe.md#the-selection-panel).
        if (auto it = losses_.find(side); !flowWon_ && it != losses_.end())
            blit(it->second, 0, 0, screen_.w, screen_.h);
        else if (auto e = emblems_.find(side); e != emblems_.end())
            blit(e->second, 0, 0, screen_.w, screen_.h);
        // MISSION REPORT, right of centre, under the MISSION LOG heading:
        // won, each objective listed, complete or incomplete; lost, only the
        // fail criterion that fired.
        std::vector<std::pair<std::string, Color>> lines;
        lines.push_back({"MISSION LOG", ui_.brightColor()});
        const int w = std::min(300, screen_.w / 2 - 20);
        if (!flowWon_ && run_ && run_->failedCriterion()) {
            const std::string* t = table_.find(run_->failedCriterion());
            for (const std::string& l :
                 ui_.wrapSmall((t ? *t : std::to_string(run_->failedCriterion())) + " - FAILED", w - 24))
                lines.push_back({l, ui_.textColor()});
        } else if (const MissionScript* script = findMissionScript(level.shortName, side); flowWon_ && script && run_)
            for (const MissionScript::Objective& o : script->objectives) {
                const MissionRun::State st = run_->objectiveState(o.id);
                if (st == MissionRun::State::Hidden)
                    continue;
                const std::string* t = table_.find(o.id);
                const std::string line = "MISSION OBJECTIVE: " + (t ? *t : std::to_string(o.id)) +
                                         (st == MissionRun::State::Complete ? " - COMPLETE" : " - INCOMPLETE");
                for (const std::string& l : ui_.wrapSmall(line, w - 24))
                    lines.push_back({l, st == MissionRun::State::Complete ? ui_.brightColor() : ui_.textColor()});
            }
        const int h = 30 + int(lines.size()) * lh + lh + 16;
        const int x = std::min(screen_.w / 2 + 10, screen_.w - w - 8), y = (screen_.h - h) / 2;
        ui_.drawPanel(screen_, x, y, w, h, "MISSION REPORT");
        for (size_t i = 0; i < lines.size(); ++i)
            ui_.drawSmall(screen_, x + 12, y + 15 + int(i) * lh, lines[i].first, lines[i].second);
        // CONTINUE, bottom right.
        const std::string label = "CONTINUE";
        const int bw = ui_.smallWidth(label) + 16, bh = lh + 8;
        const int bx = screen_.w - bw - 16, by = screen_.h - bh - 12;
        for (int yy = by; yy < by + bh; ++yy) {
            uint8_t* row = screen_.row(yy);
            for (int xx = bx; xx < bx + bw; ++xx) {
                uint8_t* o = row + xx * 4;
                const bool edge = yy == by || yy == by + bh - 1 || xx == bx || xx == bx + bw - 1;
                const Color c = edge ? ui_.textColor() : Color{10, 20, 40};
                o[0] = c.r, o[1] = c.g, o[2] = c.b;
            }
        }
        ui_.drawSmall(screen_, bx + 8, by + 4, label, ui_.brightColor());
        continue_ = {bx, by, bw, bh};
        return;
    }
    // The loading boxes: the three emblems, and a box with the lines.
    const int ew = screen_.w / 4, eh = ew * 3 / 4, gap = screen_.w / 16;
    const int ex = (screen_.w - 3 * ew - 2 * gap) / 2, ey = screen_.h / 6;
    int k = 0;
    for (char e : {'F', 'K', 'R'}) {
        if (auto it = emblems_.find(e); it != emblems_.end())
            blit(it->second, ex + k * (ew + gap), ey, ew, eh);
        ++k;
    }
    const uint64_t length = flow_ == Flow::Starting ? kStartingMs : kLoadingMs;
    const uint64_t since = flowStartMs_ == kFlowUnstarted || nowMs_ < flowStartMs_ ? 0 : nowMs_ - flowStartMs_;
    const int done = int(std::min<uint64_t>(100, since * 100 / std::max<uint64_t>(1, length)));
    std::vector<std::string> lines;
    if (flow_ == Flow::Debriefing)
        lines = {"PREPARING THE DEBRIEFING", levelLabel(), "COLLECTING THE OFFICERS' REPORTS"};
    else
        lines = {"STARTING COLONY SYSTEMS", levelLabel(), flow_ == Flow::Loading ? "READYING THE BRIEFING" : "READYING THE MAP",
                 "LOADING COMMAND ROUTINES", "BRINGING THE OFFICERS ON STATION"};
    lines.push_back("DONE " + std::to_string(done) + "%");
    const int w = std::max(240, screen_.w / 3), h = 30 + int(lines.size()) * lh;
    const int x = (screen_.w - w) / 2, y = ey + eh + screen_.h / 12;
    ui_.drawPanel(screen_, x, y, w, h, flow_ == Flow::Debriefing ? "DEBRIEFING" : "LOADING");
    for (size_t i = 0; i < lines.size(); ++i)
        ui_.drawSmall(screen_, x + 12, y + 15 + int(i) * lh, lines[i], i == 1 ? ui_.brightColor() : ui_.textColor());
}

bool MissionViewer::loadScreen(const std::string& prefix, Texture& screen) const {
    screen = Texture{};
    screen.width = 640;
    screen.height = 480;
    screen.rgba.assign(size_t(640) * 480 * 4, 0);
    for (int t = 0; t < 6; ++t) {
        Texture tile;
        if (!loadTexture(archives_, prefix + std::to_string(t + 1) + ".lbp", tile))
            return false;
        const int x0 = (t % 3) * 256, y0 = (t / 3) * 256;
        for (int y = 0; y < tile.height && y0 + y < 480; ++y)
            for (int x = 0; x < tile.width && x0 + x < 640; ++x)
                std::copy_n(&tile.rgba[(size_t(y) * size_t(tile.width) + size_t(x)) * 4], 4,
                            &screen.rgba[(size_t(y0 + y) * 640 + size_t(x0 + x)) * 4]);
    }
    return true;
}

void MissionViewer::loadBriefingArt() {
    // The emblem screens are six tiles, three across and two down:
    // 256 + 256 + 128 by 256 + 224, the 640 x 480 screen.
    // The lost screens are cut the same way (f_lose01-06, ...).
    const std::pair<char, const char*> sets[] = {{'F', "lbp/gui/ufp00"}, {'K', "lbp/gui/kliemp0"}, {'R', "lbp/gui/romemp0"}};
    for (const auto& [side, prefix] : sets) {
        const char s = char(std::tolower(static_cast<unsigned char>(side)));
        Texture screen;
        if (loadScreen(prefix, screen))
            emblems_[side] = std::move(screen);
        if (loadScreen(std::string("lbp/gui/") + s + "_lose0", screen))
            losses_[side] = std::move(screen);
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
    // The star systems' models, by name (the galaxy isn't one).
    for (const auto& a : archives_)
        for (const CatEntry& e : a->entries()) {
            std::string folder, stem, ext;
            splitName(e.name, folder, stem, ext);
            if (folder == "solsys" && ext == "geo" && stem != "tabras" &&
                std::find(systemNames_.begin(), systemNames_.end(), stem) == systemNames_.end())
                systemNames_.push_back(stem);
        }
}

// The star system the mission's table names, with its pictures.
void MissionViewer::loadStarSystem() {
    system_ = GeoModel{};
    systemLayout_ = {};
    systemTextures_.clear();
    systemName_ = starSystemName(table_, systemNames_);
    if (systemName_.empty())
        return;
    for (const auto& a : archives_)
        if (const CatEntry* e = a->find("geo/solsys/" + systemName_ + ".geo")) {
            std::vector<uint8_t> data;
            if (a->read(*e, data) || parseGeo(data, system_))
                system_ = GeoModel{};
            break;
        }
    if (system_.faces.empty())
        return;
    systemLayout_ = readStarSystem(system_);
    for (const std::string& stem : starSystemTextures(systemLayout_, hasMap_ ? planetPicture(map_.mapArchive()) : "",
                                                          systemName_)) {
        Texture tex;
        if (!stem.empty())
            loadTexture(archives_, "lbp/txr/solsys/" + stem + ".mp0", tex);
        systemTextures_.push_back(std::move(tex));
    }
}

// The system from near the mission's planet, turning about it: the
// starfield adding its light behind (its alpha layer is its brightness,
// dimmed as in the recordings), the star and planets as stacks of round
// pictures, each turned to face the camera (so its slices stand one behind
// another, a ball from any side; seen edge-on they'd be a pile of discs).
bool MissionViewer::drawStarSystem() {
    if (system_.faces.empty())
        return false;
    for (int y = 0; y < screen_.h; ++y) {
        uint8_t* row = screen_.row(y);
        for (int x = 0; x < screen_.w; ++x, row += 4)
            row[0] = row[1] = row[2] = 0, row[3] = 255;
    }
    const int body = missionBody(systemLayout_);
    GeoModel::Vec3 target{0, 0, 0};
    if (body >= 0)
        target = systemLayout_.bodies[size_t(body)].centre;
    const float yaw = float(nowMs_ % 120000) / 120000.0f * 6.2832f;
    const OrbitCamera cam({target.x, target.y, target.z}, yaw, 0.25f, 2200.0f, screen_.w, screen_.h);
    raster_.begin(screen_);
    for (int pass = 0; pass < 2; ++pass) // the starfield first
        for (size_t f = 0; f < system_.faces.size(); ++f) {
            const GeoModel::Face& face = system_.faces[f];
            const int slot = systemLayout_.faceSlot[f];
            const bool backdrop = std::find(systemLayout_.backdropSlots.begin(), systemLayout_.backdropSlots.end(),
                                            slot) != systemLayout_.backdropSlots.end();
            if (slot < 0 || size_t(slot) >= systemTextures_.size() || systemTextures_[size_t(slot)].rgba.empty() ||
                backdrop != (pass == 0) || face.corners.size() < 3 || face.part >= system_.parts.size())
                continue;
            const Texture& tex = systemTextures_[size_t(slot)];
            const auto& verts = system_.parts[face.part].vertices;
            const int body = systemLayout_.faceBody[f];
            std::vector<ViewVertex> poly;
            for (const GeoModel::Corner& c : face.corners) {
                if (c.vertex >= verts.size())
                    break;
                const GeoModel::Vec3& p = verts[c.vertex];
                Vec at{p.x, p.y, p.z};
                if (body >= 0) {
                    const GeoModel::Vec3& o = systemLayout_.bodies[size_t(body)].centre;
                    const Vec d{p.x - o.x, p.y - o.y, p.z - o.z};
                    at = Vec{o.x, o.y, o.z} + cam.right * d.x + cam.up * d.y - cam.forward * d.z;
                }
                poly.push_back(cam.toView(at, c.u, c.v));
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
                if (backdrop) {
                    for (int k = 0; k < 3; ++k)
                        out[k] = uint8_t(std::min(255, out[k] + tex.rgba[i * 4 + size_t(k)] * a / 512));
                    return false;
                }
                if (a < 128)
                    return false; // outside the disc
                for (int k = 0; k < 3; ++k)
                    out[k] = tex.rgba[i * 4 + size_t(k)];
                return true;
            };
            for (size_t i = 1; i + 1 < s.size(); ++i)
                raster_.drawTriangle(s[0], s[i], s[i + 1], shade);
        }
    return true;
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
    if (added)
        m.ok = loadTexturedModel(archives_, textureFiles_, name, m.model);
    if (!m.ok)
        return false;

    // The grid, as behind the recordings' model views: teal lines for the
    // Federation, dark red for the Klingons, green crosses for the Romulans.
    const Level& level = missions_[size_t(current_)];
    const char side = missionSides(level.shortName)[size_t(side_)];
    const uint8_t grid[3] = {uint8_t(side == 'F' ? 20 : side == 'R' ? 20 : 70),
                             uint8_t(side == 'F' ? 45 : side == 'R' ? 80 : 14),
                             uint8_t(side == 'F' ? 60 : side == 'R' ? 25 : 14)};
    const int w = screen_.w, h = screen_.h;
    for (int y = 0; y < h; ++y) {
        uint8_t* row = screen_.row(y);
        for (int x = 0; x < w; ++x, row += 4) {
            const bool line = side == 'R' ? (y % 24 == 0 && (x + 2) % 24 < 5) || (x % 24 == 0 && (y + 2) % 24 < 5)
                                          : (y % 24 == 0 && x % 4 == 0) || (x % 24 == 0 && y % 4 == 0);
            row[0] = line ? grid[0] : 0, row[1] = line ? grid[1] : 0, row[2] = line ? grid[2] : 0, row[3] = 255;
        }
    }
    // Turning once a minute, in the side's style.
    return drawModelView(screen_, raster_, m.model, float(nowMs_ % 60000) / 60000.0f * 6.2832f, 0.35f,
                         briefingModelStyle(side));
}

// The briefing's views without a map: the game's galaxy, star system,
// models and emblem where the archives have them; else Reluva's own
// stand-ins.
void MissionViewer::drawBackdrop(BriefingView view) {
    const char side = missionSides(missions_[size_t(current_)].shortName)[size_t(side_)];
    if ((view == BriefingView::Galaxy && drawGalaxy()) || (view == BriefingView::Emblem && drawEmblem(side)) ||
        (view == BriefingView::StarSystem && drawStarSystem()) ||
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
