// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/briefing_data.h"
#include "engines/newworlds/mission_ui.h"
#include "engines/newworlds/mission_surface.h"
#include "engines/newworlds/viewers.h"

#include "common/fs.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
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
        {"mining-station", "fms1", "kms1", "rms1"},        {"accommodation", "fca", "kcho", "rab"},
        {"sickbay", "fsb", "ksb", "ruh"},                  {"science-station", "fss", "kss", "rsb"},
        {"security-center", "fsc", "ka", "risb"},        {"farm", "fhf", "klsf", "rf"},
        {"storage-facility", "fsfr", "ksf", "rsf"},        {"transporter-pad", "flp", "kief", "rlp"},
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

constexpr int kJumpRows = 2; // JUMP TO: an icon's height, in lines
// The build and vehicle menus' portraits: at their own size, cut to the
// game's 64 x 60 (its cells are 90 x 60).
constexpr int kMenuIconW = 64, kMenuIcon = 60;

// The play screen's layout (gui.md#the-in-game-interface), on the 640 x
// 480 screen; a panel's lines of text add to the heights.
// The right-hand panels: 100 pixels wide in the recordings; here as wide
// as their longest lines in the font there is, and 22 more.
constexpr int kSideMinW = 100, kSideSpare = 22;
constexpr int kTextLeft = 10; // a panel's lines and buttons from its left edge
// NEW WORLDS: as high as its lines and 24 + 8 more, the first line 16
// under its top.
constexpr int kColonyTop = 24, kColonyFoot = 8, kColonyFirstLine = 16;
// TRICORDER: 102 high over its row of buttons; its map 14 under the
// panel's top, as high as the panel less 34 and its two lines (the
// readout and the buttons).
constexpr int kTricorderH = 102, kTricorderMapTop = 14, kTricorderMapCut = 34;
// COMMUNICATIONS: 220 wide, 70 high over its row of buttons.
constexpr int kCommsW = 220, kCommsH = 70;
constexpr int kCommandW = 260; // COMMAND
// The playbook: icons 32 across, in a column 4 apart.
constexpr int kPlaybookIcon = 32, kPlaybookGap = 4;

} // namespace

const char* MissionViewer::pendingOrderName() const {
    switch (play_.pendingOrder) {
    case 's': return "> SCIENCE SCAN";
    case 'm': return "> MEDICAL SCAN";
    case 'e': return "> ENGINEERING SCAN";
    case 'c': return "> CAPTURE BUILDING";
    case 'a': return "> ATTACK EN ROUTE / ASSAULT";
    case 'p': return "> PROTECT";
    default: return "";
    }
}

// The Tricorder's relief (exe.md#the-tricorder): blocks the side hasn't
// surveyed, its surveyed ground, and the deposits it has surveyed by ore,
// in the map's palette shifted for the race; the main camera's place
// marked, white outlined in black.
void MissionViewer::drawTricorderMap(const MapViewer::Relief& relief) {
    map_.drawTricorderRelief(screen_, relief, side(), [&](int bx, int by) {
        if (fog_ && !fog_->has(bx, by, Fog::Surveyed, playerSlot_))
            return 1;
        const int kind = map_.depositAt(bx, by);
        return kind >= 1 && kind <= 6 && map_.depositAmount(bx, by) > 0 ? 1 + kind : 0;
    }, [&](int bx, int by) { return !fog_ || fog_->has(bx, by, Fog::Seen, playerSlot_); });
    float cx = 0, cy = 0;
    int px = 0, py = 0;
    map_.cameraTarget(cx, cy);
    if (map_.reliefPoint(relief, cx, cy, px, py)) {
        const int arm = relief.w > 200 ? 5 : 3;
        auto cross = [&](int a, int t, Color c) {
            fillRect(screen_, px - a - t, py - t, 2 * (a + t) + 1, 2 * t + 1, c);
            fillRect(screen_, px - t, py - a - t, 2 * t + 1, 2 * (a + t) + 1, c);
        };
        cross(arm, 1, Color{0, 0, 0});
        cross(arm, 0, Color{255, 255, 255});
    }
}

// The playbook: its icons in a column left of the Tricorder; the one
// under the pointer named in a bar beside it.
void MissionViewer::drawPlaybook() {
    if (play_.playbook.empty())
        return;
    const int n = int(play_.playbook.size());
    play_.playbookAt = {screen_.w - sideW_ - kPlaybookIcon - 8, screen_.h - n * (kPlaybookIcon + kPlaybookGap) - 4};
    for (int i = 0; i < n; ++i) {
        const Rect r = playbookIcon(i);
        if (const Texture* t = ui_.icon(play_.playbook[size_t(i)].icon))
            ui_.drawPicture(screen_, *t, r.x, r.y, r.w, r.h);
        else
            ui_.drawSmall(screen_, r.x, r.y + r.h / 2, std::string(1, play_.playbook[size_t(i)].order),
                          ui_.brightColor());
        if (r.contains(mouseX_, mouseY_)) {
            const std::string name = play_.playbook[size_t(i)].name;
            const int w = ui_.smallWidth(name) + 10, bx = r.x - w - 4, by = r.y + r.h / 2 - ui_.smallHeight() / 2 - 3;
            fillRect(screen_, bx, by, w, ui_.smallHeight() + 6, Color{30, 120, 40});
            ui_.drawSmall(screen_, bx + 5, by + 3, name, Color{220, 255, 220});
        }
    }
}

// Where drawPlaybook() put its icons, and where a click gives their orders.
MissionViewer::Rect MissionViewer::playbookIcon(int i) const {
    const int step = kPlaybookIcon + kPlaybookGap;
    return {play_.playbookAt[0], play_.playbookAt[1] + i * step, kPlaybookIcon, kPlaybookIcon};
}

// The STAFF panel: a card for each of the side's officers: rank and name,
// section and skill, where they are.
std::vector<std::string> MissionViewer::staffCards() const {
    std::vector<std::string> out;
    if (!fieldWork_)
        return out;
    const char race = side();
    for (const FieldWork::OfficerAt& o : fieldWork_->officers()) {
        out.push_back(upperCase(std::string(rankTitle(race, o.officer->rank)) + " " + o.officer->name));
        out.push_back(std::string(specialityName(o.officer->speciality)) + ", " + skillWord(o.skill));
        std::string where = o.in < 0 ? "DEAD" : "IN ORBIT"; // (-1: died with what they were in)
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
        const std::string name = upperCase(o.name);
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
    auto pct = [](double v) { return std::to_string(int(std::clamp(v, 0.0, 9.99) * 100)) + "%"; };
    const BuildingType* bt = u.building ? buildingType(u.name) : nullptr;
    out.push_back(bt && u.slot == playerSlot_ ? bt->panel : upperName(u.building ? u.name : u.kind.name));
    const int owner = u.heldBy >= 0 ? u.heldBy : u.slot;
    const char race = owner == playerSlot_ ? side()
                      : ai_                ? ai_->raceOf(owner)
                                           : u.side;
    out.push_back(upperName(raceName(race == '?' ? u.side : race)));
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
    if (play_.colonyDetails && c->type == "colony-hub") {
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
            out.push_back("RAW " + upperName(oreName(k)) + " " + std::to_string(int(ore[size_t(k - 1)])));
    }
    if (c->type == "resource-processor") {
        const auto ore = economy_->oreStock();
        for (int k = 1; k <= kOres; ++k)
            out.push_back("RAW " + upperName(oreName(k)) + " " + std::to_string(int(ore[size_t(k - 1)])));
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

// Where the full-screen Tricorder (Tab) draws its relief, and its zoom:
// the main view's place, left of the right-hand panels (the small
// Tricorder's panel showing the main view meanwhile).
MissionViewer::Rect MissionViewer::fullTricorderRect(int& tiles) const {
    // (Twice as many tiles across as the small one.)
    tiles = kTricorderTiles << (tricorderZoom_ + 1);
    return {0, 0, std::max(screen_.w - sideW_, screen_.w / 2), screen_.h};
}

MapViewer::Relief MissionViewer::tricorderRelief(const Rect& map, int tiles) const {
    // Relief orbit turns as the briefings' relief does.
    const float orbit = float(nowMs_ % 3600000) * kBriefingTurn;
    return map_.tricorderRelief(map.x, map.y, map.w, map.h, tiles, tricorderView_, orbit);
}

MapViewer::Relief MissionViewer::tricorderRelief() const {
    int tiles = 0;
    if (play_.tricorderFull && hasMap_)
        return tricorderRelief(fullTricorderRect(tiles), tiles);
    if (tricorder_.w <= 0)
        return {};
    return tricorderRelief(tricorder_, kTricorderTiles << tricorderZoom_);
}

// What play adds over the map: the full-screen Tricorder (Tab), with the
// units on it, and the box a left drag draws.
void MissionViewer::drawPlay() {
    if (play_.tricorderFull && hasMap_) {
        int tiles = 0;
        const Rect map = fullTricorderRect(tiles);
        const MapViewer::Relief relief = tricorderRelief(map, tiles);
        dimRect(screen_, 0, 0, screen_.w, screen_.h);
        // The marked place the readout at the top reads under: the camera's
        // target (the game keeps it at its map's centre).
        drawTricorderMap(relief);
        // The small Tricorder's panel shows the main camera's view
        // (play-fed-lp.md): the scene under the overlay, made small.
        drawSceneInset(tricorder_);
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
                if (!map_.reliefPoint(relief, u.x, u.y, px, py))
                    continue;
                const bool own = u.slot == playerSlot_, enemy = world_->stance(u.slot, playerSlot_) == 3;
                const uint8_t r = u.selected ? 255 : own ? 120 : 255;
                const uint8_t g = u.selected ? 150 : own ? 255 : enemy ? 70 : 230;
                const uint8_t b = u.selected ? 30 : own ? 120 : 60;
                fillRect(screen_, px - 2, py - 2, 5, 5, Color{r, g, b});
            }
    }
    if (play_.boxing && (std::abs(mouseX_ - boxX_) > 6 || std::abs(mouseY_ - boxY_) > 6)) {
        const int x0 = std::min(boxX_, mouseX_), x1 = std::max(boxX_, mouseX_);
        const int y0 = std::min(boxY_, mouseY_), y1 = std::max(boxY_, mouseY_);
        frameRect(screen_, x0, y0, x1 - x0 + 1, y1 - y0 + 1, Color{120, 255, 120});
    }
    // Escape's box (Reluva's wording).
    if (play_.quitAsked) {
        const int lh = ui_.smallHeight() + 2, w = std::max(180, ui_.smallWidth("LEAVE THIS MISSION?") + 40),
                  h = kPanelTitleH + lh * 3;
        const int x = (screen_.w - w) / 2, y = (screen_.h - h) / 2;
        ui_.drawPanel(screen_, x, y, w, h, "QUIT");
        const std::string q = "LEAVE THIS MISSION?";
        ui_.drawSmall(screen_, x + (w - ui_.smallWidth(q)) / 2, y + 18, q, ui_.brightColor());
        const int by = y + 18 + lh * 2, yw = ui_.smallWidth("YES") + 12, nw = ui_.smallWidth("NO") + 12;
        quitYes_ = {x + w / 4 - yw / 2, by - 2, yw, lh + 4};
        quitNo_ = {x + 3 * w / 4 - nw / 2, by - 2, nw, lh + 4};
        ui_.drawSmall(screen_, quitYes_.x + 6, by, "YES", ui_.brightColor());
        ui_.drawSmall(screen_, quitNo_.x + 6, by, "NO", ui_.brightColor());
    }
}

// The in-game interface, laid out as the recordings show it on the 640 x
// 480 screen; see docs/formats/newworlds/gui.md#the-in-game-interface.
void MissionViewer::drawInterface() {
    const Level& level = missions_[size_t(current_)];
    const char side = this->side();
    play_.buttons.clear();
    play_.panels.clear();
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
    const int sideW = std::max({kSideMinW, ui_.smallWidth("COLONY SIZE = 20/50"), ui_.smallWidth("TRANSALUMIN = 5000"),
                                ui_.smallWidth("TERRAIN AT <000:000>"), ui_.smallWidth("ENGINEERING, HIGHLY SKILLED") + 24}) +
                      kSideSpare;
    sideW_ = sideW;

    // The strip along the top: the compass, or with the full-screen
    // Tricorder up, its readout (the compass moves to the
    // small Tricorder's line; play-14east.md, play-fed-lp.md).
    if (hasMap_) {
        const Color pale{220, 220, 225}; // the game's strip is a pale grey throughout
        if (!play_.tricorderFull) { // (with it up, drawPlay() puts its readout here)
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
        const auto menu = play_.buildMenu ? buildMenu() : std::vector<const BuildingType*>{};
        const auto offered = play_.researchMenu && research_ ? research_->offered() : std::vector<size_t>{};
        const auto vehicles = play_.vehicleMenu && economy_ ? vehicleMenu() : std::vector<const VehicleType*>{};
        const size_t jobs = play_.vehicleMenu && economy_ ? economy_->vehicleJobs().size() : 0;
        const Unit* picked = play_.selectedBuilding && world_ ? world_->unit(play_.selectedBuilding) : nullptr;
        if (!picked && world_ && world_->selection().size() == 1 && !play_.buildMenu && !play_.researchMenu &&
            !play_.vehicleMenu)
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
            // DEMOLISH asks first; so does cancelling a site (while it's
            // still going up).
            if (play_.razeAsked == picked->id)
                actions = {{"DEMOLISH? YES", "raze-yes"}, {"NO", "raze-no"}};
            if (const std::vector<int> pending = play_.cancelAsked ? pendingBuilds() : std::vector<int>{};
                play_.cancelFrom == picked->id && play_.colonyDetails &&
                std::find(pending.begin(), pending.end(), play_.cancelAsked) != pending.end())
                actions = {{"CANCEL IT? YES", "cancel-yes"}, {"NO", "cancel-no"}};
        }
        const std::vector<std::string> cards = play_.staffView ? staffCards() : std::vector<std::string>{};
        const std::vector<std::string> training =
            play_.staffView && play_.trainingView ? trainingLines() : std::vector<std::string>{};
        const size_t selected = world_ ? world_->selection().size() : 0;
        const int multiLines = (kMultiCellH * int((std::min(selected, kMultiPage) + 2) / 3) + lh - 1) / lh; // its grid
        const int lines = play_.buildMenu                         ? menuLines(lh)
                          : play_.researchMenu                    ? std::max(2, int(offered.size()) + 1)
                          : play_.vehicleMenu                     ? menuLines(lh) + int(std::min<size_t>(jobs, 4))
                          : play_.trainingView && play_.staffView ? std::max(2, int(training.size()) + 1)
                          : play_.staffView                       ? int(cards.size()) + 2
                          : !info.empty() ? int(info.size()) + 1 + (play_.pendingOrder ? 1 : 0) + (actions.empty() ? 0 : 2)
                          : selected > 1                          ? 3 + (play_.pendingOrder ? 1 : 0) + multiLines
                          : economy_                              ? 9 + 5 * kJumpRows
                                                                  : 7;
        const int x = screen_.w - sideW, y = 0, w = sideW, h = kColonyTop + lines * lh + kColonyFoot;
        ui_.drawPanel(screen_, x, y, w, h, play_.vehicleMenu && !play_.buildMenu ? "ENGINEERING" : "NEW WORLDS");
        play_.panels.push_back({x, y, w, h, play_.buttons.size()});
        // The Romulans start with a little less (most of their recordings).
        static const char* const kNames[] = {"DILITHIUM", "POLYCOMP", "THERMOCONC", "TRANSALUMIN", "NITRIUM", "DURANIUM"};
        static constexpr int kStart[] = {500, 1000, 2300, 500, 4000, 5000}, kRomulan[] = {500, 988, 2211, 497, 3500, 5000};
        std::pair<const char*, int> kStock[6];
        for (int i = 0; i < 6; ++i)
            kStock[i] = {kNames[i], side == 'R' ? kRomulan[i] : kStart[i]};
        int ty = y + kColonyFirstLine;
        const std::vector<int> sel = world_ ? world_->selection() : std::vector<int>{};
        const BuildingType* placing = play_.placing.empty() ? nullptr : buildingType(play_.placing);
        if ((play_.vehicleMenu && economy_) || play_.buildMenu) {
            drawMenu(x, ty, w, play_.buildMenu ? std::vector<const VehicleType*>{} : vehicles, menu, jobs);
        } else if (play_.researchMenu && research_) {
            // Research: the topics open, each with its key, and how far
            // it has got (as the game's topic buttons: not started, in
            // progress, finished).
            ui_.drawSmall(screen_, x + kTextLeft, ty, "RESEARCH", bright);
            if (offered.empty())
                ui_.drawSmall(screen_, x + kTextLeft, ty + lh, "NO TOPICS", text);
            for (size_t i = 0; i < offered.size(); ++i) {
                const Research::Topic& t = research_->topics()[offered[i]];
                const std::string name = upperName(t.name);
                const std::string state =
                    t.running ? std::to_string(t.done * 100 / std::max(1, t.points)) + "%" : "READY";
                ui_.drawSmall(screen_, x + kTextLeft, ty + lh * int(i + 1),
                              std::to_string(i + 1) + " " + name + " " + state, t.running ? bright : text);
            }
        } else if (placing) {
            // Placing: the building and what it costs.
            ui_.drawSmall(screen_, x + kTextLeft, ty, placing->panel, bright);
            int line = 1;
            for (int m = 0; m < kMaterials; ++m)
                if (placing->costFor(side)[size_t(m)] > 0)
                    ui_.drawSmall(screen_, x + kTextLeft, ty + lh * line++,
                                  std::string(materialName(m)) + " = " + std::to_string(placing->costFor(side)[size_t(m)]),
                                  economy_->stock()[size_t(m)] >= placing->costFor(side)[size_t(m)] ? text : Color{255, 80, 60});
        } else if (play_.staffView && play_.trainingView) {
            // Training: each officer's course; a click starts it.
            ui_.drawSmall(screen_, x + kTextLeft, ty, "TRAINING", bright);
            if (training.empty())
                ui_.drawSmall(screen_, x + kTextLeft, ty + lh, "NO ONE TO TRAIN", text);
            for (size_t i = 0; i < training.size(); ++i) {
                const FieldWork::Course& c = fieldWork_->courses()[i];
                const int cy = ty + lh * int(i + 1);
                ui_.drawSmall(screen_, x + kTextLeft, cy, training[i], c.running ? bright : text);
                if (!c.finished && !c.running)
                    play_.buttons.push_back({x + 6, cy, w - 12, lh, "course" + std::to_string(i)});
            }
        } else if (play_.staffView && fieldWork_) {
            // The officers' cards: each one's own face, then rank
            // and name, section and skill, and where they are. A click picks
            // one: the next of the colony's buildings or vehicles clicked
            // takes them.
            ui_.drawSmall(screen_, x + kTextLeft, ty, play_.pickedOfficer >= 0 ? "STAFF: CLICK WHERE TO" : "STAFF",
                          bright);
            // TRAINING, lit while a course is still to be had.
            const auto& courses = fieldWork_->courses();
            if (std::any_of(courses.begin(), courses.end(), [](const FieldWork::Course& c) { return !c.finished; })) {
                const int bw = ui_.smallWidth("TRAINING") + 8, bx = x + w - bw - 8;
                ui_.drawSmall(screen_, bx + 4, ty, "TRAINING", ui_.titleColor());
                play_.buttons.push_back({bx, ty - 2, bw, lh + 4, "training"});
            }
            for (size_t i = 0; i * 3 < cards.size(); ++i) {
                const int cy = ty + lh * int(1 + i * 3);
                const auto& o = fieldWork_->officers()[i];
                if (const Texture* face = ui_.officerFace(o.officer->face))
                    ui_.drawPicture(screen_, *face, x + 8, cy, lh * 3 - 2, lh * 3 - 2);
                for (int k = 0; k < 3; ++k)
                    ui_.drawSmall(screen_, x + 12 + lh * 3, cy + lh * k, cards[i * 3 + size_t(k)],
                                  int(i) == play_.pickedOfficer ? bright : k == 0 ? ui_.titleColor() : text);
                play_.buttons.push_back({x + 6, cy, w - 12, lh * 3, "officer" + std::to_string(i)});
            }
        } else if (picked) {
            // One object, a unit or a building: the game's panel, with its
            // hit points as a bar under it.
            // (On the colony's details, an entry going up is a button: a
            // click asks to cancel it.)
            const std::vector<int> pending = play_.colonyDetails ? pendingBuilds() : std::vector<int>{};
            size_t entry = 0;
            for (size_t i = 0; i < info.size(); ++i) {
                ui_.drawSmall(screen_, x + kTextLeft, ty + lh * int(i), info[i], i == 0 ? bright : text);
                if (entry < pending.size() &&
                    (info[i].rfind("BUILDING ", 0) == 0 || info[i].rfind("ENGINEERING ", 0) == 0))
                    play_.buttons.push_back(
                        {x + 6, ty + lh * int(i), w - 12, lh, "cancel" + std::to_string(pending[entry++])});
            }
            const int by = ty + lh * int(info.size()) + 2, bw = w - 20;
            const float share = std::clamp(picked->hp / std::max(1.0f, picked->maxHp), 0.0f, 1.0f);
            const int full = int(float(bw) * share);
            fillRect(screen_, x + kTextLeft, by, full, 4, share < 0.3f ? Color{230, 60, 70} : Color{60, 210, 70});
            fillRect(screen_, x + kTextLeft + full, by, bw - full, 4, Color{40, 40, 50});
            if (play_.pendingOrder)
                ui_.drawSmall(screen_, x + kTextLeft, by + 6, pendingOrderName(), bright);
            // Its buttons, in a row (wrapping) under the bar.
            int bx = x + kTextLeft, byy = by + 8;
            for (const auto& [label, action] : actions) {
                const int bw = ui_.smallWidth(label) + 8;
                if (bx + bw > x + w - 6)
                    bx = x + kTextLeft, byy += lh + 4;
                fillRect(screen_, bx, byy - 2, bw, 1, text);
                fillRect(screen_, bx, byy + lh + 1, bw, 1, text);
                ui_.drawSmall(screen_, bx + 4, byy, label, bright);
                play_.buttons.push_back({bx, byy - 2, bw, lh + 4, action});
                bx += bw + 4;
            }
        } else if (sel.size() > 1) {
            // MULTISELECTION (exe.md#the-multiselection): pages of 15, three
            // across: each object's portrait with its health under it, a
            // frame round those ticked; DELETE drops the ticked from the
            // selection, INVERT flips the ticks.
            const int pages = int((sel.size() + kMultiPage - 1) / kMultiPage);
            play_.multiPage = std::clamp(play_.multiPage, 0, pages - 1);
            ui_.drawSmall(screen_, x + kTextLeft, ty,
                          pages > 1
                              ? "MULTISELECTION " + std::to_string(play_.multiPage + 1) + "/" + std::to_string(pages)
                              : std::string("MULTISELECTION"),
                          bright);
            const int gx = x + std::max(4, (w - 3 * kMultiCellW) / 2), gy = ty + lh;
            const size_t first = size_t(play_.multiPage) * kMultiPage;
            for (size_t i = first; i < sel.size() && i < first + kMultiPage; ++i) {
                const Unit* m = world_->unit(sel[i]);
                if (!m)
                    continue;
                const int cx = gx + int((i - first) % 3) * kMultiCellW, cy = gy + int((i - first) / 3) * kMultiCellH;
                if (play_.multiTicked.count(m->id)) {
                    constexpr Color kTick{0, 128, 255};
                    fillRect(screen_, cx + 5, cy + 1, 36, 2, kTick), fillRect(screen_, cx + 5, cy + 41, 36, 2, kTick);
                    fillRect(screen_, cx + 5, cy + 1, 2, 42, kTick), fillRect(screen_, cx + 39, cy + 1, 2, 42, kTick);
                }
                if (const Texture* t = m->type ? ui_.objectPortrait(portraitOf(m->type)) : nullptr)
                    ui_.drawPicture(screen_, *t, cx + 7, cy + 3, 32, 32);
                else
                    ui_.drawSmall(screen_, cx + 7, cy + 12, m->kind.name.substr(0, 4), text);
                const int green = int(32 * std::clamp(m->hp / std::max(1.0f, m->maxHp), 0.0f, 1.0f));
                fillRect(screen_, cx + 7, cy + 37, green, 3, Color{0, 255, 0});
                fillRect(screen_, cx + 7 + green, cy + 37, 32 - green, 3, Color{255, 0, 0});
                play_.buttons.push_back({cx, cy, kMultiCellW, kMultiCellH, "msel" + std::to_string(m->id)});
            }
            int by = gy + kMultiCellH * int((std::min(sel.size() - first, kMultiPage) + 2) / 3) + 2;
            if (play_.pendingOrder)
                ui_.drawSmall(screen_, x + kTextLeft, by, pendingOrderName(), bright), by += lh;
            int bx = x + kTextLeft;
            for (const auto& [label, action] : std::vector<std::pair<std::string, std::string>>{
                     {"<", "msel-prev"}, {"DELETE", "msel-delete"}, {"INVERT", "msel-invert"}, {">", "msel-next"}}) {
                const bool off = (action == "msel-prev" && play_.multiPage == 0) ||
                                 (action == "msel-next" && play_.multiPage + 1 >= pages);
                const int bw = ui_.smallWidth(label) + 8;
                ui_.drawSmall(screen_, bx + 4, by + 2, label, off ? text : bright);
                if (!off)
                    play_.buttons.push_back({bx, by, bw, lh + 4, action});
                bx += bw + 4;
            }
        } else {
            if (economy_) {
                for (int i = 0; i < 6; ++i)
                    kStock[i].second = economy_->stock()[size_t(i)];
                ui_.drawSmall(screen_, x + kTextLeft, ty,
                              "COLONY SIZE = " + std::to_string(economy_->colonySize()) + "/" +
                                  std::to_string(economy_->capacity()),
                              bright);
            } else {
                ui_.drawSmall(screen_, x + kTextLeft, ty, "COLONY SIZE = 20/50", bright);
            }
            for (const auto& [name, n] : kStock) {
                ty += lh;
                ui_.drawSmall(screen_, x + kTextLeft, ty, std::string(name) + " = " + std::to_string(n), text);
            }
            // The colony's people: at work, and waiting in the pools.
            if (economy_)
                ui_.drawSmall(screen_, x + kTextLeft, ty + lh,
                              "CREW " + std::to_string(economy_->crew()) + " POOL " + std::to_string(economy_->pool()),
                              text);
            // JUMP TO: on the left a slot for each class of the colony's
            // objects (a click: the camera to the next of them), on the
            // right battle groups 1-5 (their first member's portrait; a
            // click selects the group and goes to it).
            if (economy_ && world_) {
                ty += lh * 2;
                ui_.drawSmall(screen_, x + kTextLeft, ty, "JUMP TO", bright);
                ty += lh;
                const int race = side == 'K' ? 1 : side == 'R' ? 2 : 0, icon = lh * kJumpRows - 2;
                for (int i = 0; i < 10; ++i) {
                    const int ix = x + kTextLeft + (i / 5) * (icon + 8), iy = ty + (i % 5) * (icon + 2);
                    int type = i < 5 ? kJumpIcons[race][i] : 0;
                    if (i >= 5)
                        if (const auto& g = world_->group(i - 4); !g.empty())
                            if (const Unit* m = world_->unit(g[0]))
                                type = m->type;
                    const Texture* t = type ? ui_.objectPortrait(portraitOf(type)) : nullptr;
                    if (t)
                        ui_.drawPicture(screen_, *t, ix, iy, icon, icon);
                    frameRect(screen_, ix - 1, iy - 1, icon + 2, icon + 2,
                              Color{uint8_t(text.r / 2), uint8_t(text.g / 2), uint8_t(text.b / 2)});
                    if (i >= 5 && !t)
                        ui_.drawSmall(screen_, ix + icon / 2 - 3, iy + icon / 2 - lh / 2, std::to_string(i - 4), text);
                    play_.buttons.push_back({ix, iy, icon, icon, "jump" + std::to_string(i)});
                }
            }
        }
    }

    // TRICORDER (exe.md#the-tricorder): the relief round the camera at
    // its zoom, the side's units on it (with the full-screen one up, the
    // main view instead: drawPlay()); a line naming what's under the
    // pointer (else the view and zoom); COMMS, ALLY, ZOOM under it.
    {
        const int x = screen_.w - sideW, w = sideW, h = kTricorderH + lh + 4, y = screen_.h - h;
        ui_.drawPanel(screen_, x, y, w, h, "TRICORDER");
        play_.panels.push_back({x, y, w, h, play_.buttons.size()});
        const int mx = x + kPanelInset, my = y + kTricorderMapTop, mw = w - 2 * kPanelInset,
                  mh = h - kTricorderMapCut - 2 * lh;
        const int tiles = kTricorderTiles << tricorderZoom_;
        if (hasMap_) {
            tricorder_ = {mx, my, mw, mh};
            const MapViewer::Relief relief = tricorderRelief(tricorder_, tiles);
            if (!play_.tricorderFull)
                drawTricorderMap(relief);
            // Each side's own colour, whoever it is to the player
            // (exe.md#the-tricorder): the Federation blue, the Klingons
            // orange, the Romulans green, the Taubat and Metar yellow, the
            // Hubrin magenta, the Orions red; an alerted object red. A blip
            // is outlined in black, the outline blinking white on the
            // selected and alerted.
            static constexpr Color kSideColours[] = {{0, 0, 255},   {255, 128, 0}, {0, 255, 0},  {255, 255, 0},
                                                     {255, 0, 255}, {255, 255, 0}, {255, 0, 0}};
            const bool blink = (nowMs_ / 500) % 2 == 1;
            if (world_ && !play_.tricorderFull)
                for (const Unit& u : world_->units()) {
                    if (!visible(u))
                        continue;
                    int px = 0, py = 0;
                    if (!map_.reliefPoint(relief, u.x, u.y, px, py))
                        continue;
                    const bool alerted = u.slot == playerSlot_ && sim_ && sim_->alerted(u.id);
                    const Color fill = alerted                                     ? Color{255, 0, 0}
                                       : u.slot >= 0 && u.slot < int(std::size(kSideColours)) ? kSideColours[u.slot]
                                                                                   : Color{0, 0, 255};
                    const Color edge = blink && (u.selected || alerted) ? Color{255, 255, 255} : Color{0, 0, 0};
                    fillRect(screen_, px - 1, py - 1, 4, 4, edge);
                    fillRect(screen_, px, py, 2, 2, fill);
                }
            // With the full-screen Tricorder up its line carries the compass.
            ui_.drawSmall(screen_, x + kTextLeft, y + h - 14 - 2 * lh,
                          play_.tricorderFull ? compassStrip() : tricorderReadout(relief), text);
        }
        int bx = x + kTextLeft;
        for (const auto& [label, action] : {std::pair{"COMMS", "tri-comms"}, std::pair{"ALLY", "tri-ally"},
                                            std::pair{"ZOOM", "tri-zoom"}}) {
            const int bw = ui_.smallWidth(label) + 8;
            ui_.drawSmall(screen_, bx + 4, y + h - 11 - lh, label,
                          play_.allyOpen && std::string_view(action) == "tri-ally" ? bright : text);
            play_.buttons.push_back({bx, y + h - 13 - lh, bw, lh + 4, action});
            bx += bw + 4;
        }
        // ALLY: a row for each other race in the mission, the player's
        // posture towards it; a click on one offers FRIENDLY, NEUTRAL,
        // HOSTILE.
        if (play_.allyOpen && world_) {
            const auto others = allyRaces();
            const int ah = kPanelTitleH + int(others.size() + (play_.allyPick >= 0 ? 1 : 0)) * (lh + 2);
            const int ay = y - ah;
            ui_.drawPanel(screen_, x, ay, w, ah, "ALLY");
            play_.panels.push_back({x, ay, w, ah, play_.buttons.size()});
            int ry = ay + kPanelFirstLine;
            for (int slot : others) {
                const int st = world_->stance(playerSlot_, slot);
                const std::string name = upperCase(raceName(ai_ ? ai_->raceOf(slot) : '?'));
                const std::string word = st == 1 ? "FRIENDLY" : st == 2 ? "NEUTRAL" : "HOSTILE";
                ui_.drawSmall(screen_, x + kTextLeft, ry, name, slot == play_.allyPick ? bright : text);
                ui_.drawSmall(screen_, x + w - kPanelInset - ui_.smallWidth(word), ry, word,
                              st == 3 ? Color{255, 90, 70} : text);
                play_.buttons.push_back({x + 6, ry - 1, w - 12, lh + 2, "ally-race" + std::to_string(slot)});
                ry += lh + 2;
                if (slot == play_.allyPick) {
                    int ox = x + 14;
                    for (const auto& [label, v] : {std::pair{"FRIENDLY", 1}, std::pair{"NEUTRAL", 2}, std::pair{"HOSTILE", 3}}) {
                        const int bw = ui_.smallWidth(label) + 6;
                        ui_.drawSmall(screen_, ox + 3, ry, label, bright);
                        play_.buttons.push_back({ox, ry - 1, bw, lh + 2, "ally-set" + std::to_string(v)});
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
    if (!play_.commsClosed) {
        const int w = kCommsW, h = kCommsH + lh + 4, x = (screen_.w - sideW - w) / 2 + 10, y = screen_.h - h;
        ui_.drawPanel(screen_, x, y, w, h, "COMMUNICATIONS");
        play_.panels.push_back({x, y, w, h, play_.buttons.size()});
        const int n = int(log_.size());
        const int shown = play_.commsShown < 0 || play_.commsShown >= n ? n - 1 : play_.commsShown;
        if (log_.empty()) {
            ui_.drawSmall(screen_, x + 60, y + 16, "NO MESSAGES ON FILE.", text);
        } else {
            const auto& [sender, message] = log_[size_t(shown)];
            const std::string natives = level.shortName == "C9" ? "orion" : "taubat";
            // The face the message's officer number gives (the mission's
            // mail for that section), else the section's.
            const std::string section = fs::toLower(sender.substr(0, sender.find(' ')));
            const Texture* face = ui_.messageFace(messageOfficer(level.shortName, side, section));
            if (const Texture* p = face ? face : ui_.portraitFor(sender, natives))
                ui_.drawPicture(screen_, *p, x + kTextLeft, y + 13, 44, 41);
            int ty = y + 14;
            ui_.drawSmall(screen_, x + 58, ty, upperCase(sender + ":"), text);
            for (const std::string& l : ui_.wrapSmall(message, w - 70)) {
                ty += lh;
                if (ty > y + h - 14 - 2 * lh)
                    break;
                ui_.drawSmall(screen_, x + 58, ty, l, text);
            }
            const std::string counter = std::to_string(shown + 1) + "/" + std::to_string(n);
            ui_.drawSmall(screen_, x + w - kPanelInset - ui_.smallWidth(counter), y + 14, counter, text);
        }
        int bx = x + kTextLeft;
        const int by = y + h - 11 - lh;
        for (const auto& [label, action] : {std::pair{"DELETE", "comms-delete"}, std::pair{"|<", "comms-first"},
                                            std::pair{"<", "comms-prev"}, std::pair{">", "comms-next"},
                                            std::pair{">|", "comms-last"}, std::pair{"MISSION", "comms-mission"},
                                            std::pair{"X", "comms-close"}}) {
            const int bw = ui_.smallWidth(label) + 6;
            ui_.drawSmall(screen_, bx + 3, by, label, bright);
            play_.buttons.push_back({bx, by - 2, bw, lh + 4, action});
            bx += bw + 3;
        }
    }

    // COMMAND: the mission, and its objectives as the recordings' panel
    // lists them (MISSION on the communications panel shows or hides it).
    if (run_ && play_.commandShown) {
        const MissionScript* script = findMissionScript(level.shortName, side);
        std::vector<std::pair<std::string, Color>> lines;
        const int w = kCommandW;
        for (const std::string& l : ui_.wrapSmall(
                 upperCase(levelLabel() + (level.title.empty() ? "" : ": " + level.title)), w - 2 * kPanelInset))
            lines.push_back({l, bright});
        for (const MissionScript::Objective& o : script->objectives) {
            const MissionRun::State st = run_->objectiveState(o.id);
            if (st == MissionRun::State::Hidden)
                continue;
            const std::string* t = table_.find(o.id);
            const std::string s = "MISSION OBJECTIVE: " + (t ? *t : std::to_string(o.id)) +
                                  (st == MissionRun::State::Complete ? " - COMPLETE" : " - INCOMPLETE");
            for (const std::string& l : ui_.wrapSmall(s, w - 2 * kPanelInset))
                lines.push_back({l, st == MissionRun::State::Complete ? bright : text});
        }
        const int h = kPanelTitleH + int(lines.size()) * lh;
        ui_.drawPanel(screen_, 0, 0, w, h, "COMMAND");
        play_.panels.push_back({0, 0, w, h, play_.buttons.size()});
        for (size_t i = 0; i < lines.size(); ++i)
            ui_.drawSmall(screen_, kPanelInset, kPanelFirstLine + int(i) * lh, lines[i].first, lines[i].second);
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
    const char side = this->side();
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
    play_.menuPage = std::clamp(play_.menuPage, 0, pages - 1);
    ui_.drawSmall(screen_, x + kTextLeft, ty, buildMenuTitle(), bright);
    if (items.empty())
        ui_.drawSmall(screen_, x + kTextLeft, ty + lh, play_.vehicleMenu ? "NO VEHICLE YARD" : "NOTHING TO BUILD",
                      text);
    int iy = ty + lh + 2;
    int shown = -1; // the item at the foot
    for (int k = 0; k < 3; ++k) {
        const int i = play_.menuPage * 3 + k;
        if (i >= int(items.size()))
            break;
        const Item& it = items[size_t(i)];
        const int ix = x + kTextLeft;
        if (const Texture* t = ui_.objectPortrait(it.portrait))
            ui_.drawPicture(screen_, *t, ix, iy, kMenuIconW, kMenuIcon, true);
        const bool flash = play_.menuFlash.first == i && nowMs_ - play_.menuFlash.second < 300;
        const Rect cell{ix, iy, w - 16, kMenuIcon}; // drawn, hovered and clicked
        const bool over = cell.contains(mouseX_, mouseY_);
        if (over)
            shown = i;
        const Color frame = flash ? Color{255, 60, 255} : over ? bright : dim;
        frameRect(screen_, ix - 1, iy - 1, kMenuIconW + 2, kMenuIcon + 2, frame);
        ui_.drawSmall(screen_, ix + 2, iy + 1, std::to_string((i + 1) % 10), bright);
        int ny = iy;
        for (const std::string& l : ui_.wrapSmall(it.name, w - kMenuIconW - 26)) {
            ui_.drawSmall(screen_, ix + kMenuIconW + 6, ny, l, !it.open ? dim : it.pays ? bright : text);
            ny += lh;
        }
        play_.buttons.push_back({cell.x, cell.y, cell.w, cell.h, "menu-item" + std::to_string(i)});
        iy += kMenuIcon + 4;
    }
    // The foot: what's under the pointer, else the page's first.
    int fy = ty + lh + 2 + 3 * (kMenuIcon + 4);
    if (shown < 0 && play_.menuPage * 3 < int(items.size()))
        shown = play_.menuPage * 3;
    if (shown >= 0) {
        const Item& it = items[size_t(shown)];
        ui_.drawSmall(screen_, x + kTextLeft, fy, it.name, bright);
        ui_.drawSmall(screen_, x + kTextLeft, fy + lh, it.role, text);
        int cy = fy + 2 * lh;
        for (const std::string& c : costLines(it.cost)) {
            ui_.drawSmall(screen_, x + kTextLeft, cy, c, text);
            cy += lh;
        }
    }
    fy += (2 + kMaterials) * lh;
    // The arrows, the page, X.
    int bx = x + kTextLeft;
    for (const auto& [label, action] : {std::pair{"<", "menu-prev"}, std::pair{">", "menu-next"}, std::pair{"X", "menu-close"}}) {
        const int bw = ui_.smallWidth(label) + 10;
        ui_.drawSmall(screen_, bx + 5, fy, label, bright);
        play_.buttons.push_back({bx, fy - 2, bw, lh + 4, action});
        bx += bw + 6;
    }
    ui_.drawSmall(screen_, bx + 4, fy, std::to_string(play_.menuPage + 1) + "/" + std::to_string(pages), text);
    // The yards' work.
    for (size_t i = 0; play_.vehicleMenu && economy_ && i < jobs && i < 4; ++i) {
        const Economy::VehicleJob& j = economy_->vehicleJobs()[i];
        const VehicleType* v = vehicleType(j.type);
        const int pct = j.total > 0 ? int(100 - std::max(0.0, j.work) * 100 / j.total) : 100;
        ui_.drawSmall(screen_, x + kTextLeft, fy + lh * int(i + 1),
                      std::string("> ") + (v ? v->panel : "?") + " " + std::to_string(pct) + "%", text);
    }
}

const char* MissionViewer::buildMenuTitle() const {
    return play_.vehicleMenu && !play_.buildMenu ? "VEHICLES" : "BUILD";
}

// The Tricorder's line (exe.md#the-tricorder): with the pointer over
// it, what's there: one of the side's objects in sight (FRIENDLY,
// NEUTRAL or HOSTILE, by the player's posture to its side, then DEFENSES,
// STRUCTURE or UNIT), ground no unit can cross (IMPASSABLE), a surveyed
// deposit (its ore AT <a:b>) or the ground (TERRAIN AT <a:b>); else the
// view and its zoom (RELIEF X2).
std::string MissionViewer::tricorderReadout(const MapViewer::Relief& relief) const {
    static const char* const kView[] = {"STARSHIP ", "RELIEF ", "RELIEF ORBIT ", "RELIEF FIXED "};
    static const char* const kZoom[] = {"X4", "X2", "X1"};
    float wx = 0, wy = 0;
    if (!Rect{relief.x, relief.y, relief.w, relief.h}.contains(mouseX_, mouseY_) ||
        !map_.reliefToWorld(relief, mouseX_, mouseY_, wx, wy))
        return std::string(kView[std::clamp(tricorderView_, 0, 3)]) + kZoom[tricorderZoom_];
    // An object within two pixels' worth of the pointer.
    float x1 = wx, y1 = wy;
    map_.reliefToWorld(relief, mouseX_ + 2, mouseY_, x1, y1);
    return tricorderReadoutAt(wx, wy, std::max(std::hypot(x1 - wx, y1 - wy), 1500.0f));
}

// The main camera's view made small in a panel's place (the scene as last
// drawn, under the overlay): its middle, as wide as the place allows.
void MissionViewer::drawSceneInset(const Rect& at) {
    const std::vector<uint8_t>& scene = map_.lastScene();
    if (at.w <= 0 || at.h <= 0 || scene.size() != size_t(screen_.w) * size_t(screen_.h) * 4)
        return;
    // The scene's part with the place's shape, centred.
    int sw = screen_.w, sh = screen_.w * at.h / at.w;
    if (sh > screen_.h)
        sh = screen_.h, sw = screen_.h * at.w / at.h;
    const int sx = (screen_.w - sw) / 2, sy = (screen_.h - sh) / 2;
    for (int y = 0; y < at.h; ++y) {
        if (at.y + y < 0 || at.y + y >= screen_.h)
            continue;
        const uint8_t* src = &scene[size_t(sy + y * sh / at.h) * size_t(screen_.w) * 4];
        uint8_t* row = screen_.row(at.y + y);
        for (int x = 0; x < at.w; ++x)
            if (at.x + x >= 0 && at.x + x < screen_.w)
                std::copy_n(src + size_t(sx + x * sw / at.w) * 4, 4, row + (at.x + x) * 4);
    }
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
        if (const int kind = map_.depositAt(b / 8, a / 8))
            what = upperCase(kind == 5 ? "MAGNESITE ORE" : oreName(kind));
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

} // namespace reluva::newworlds
