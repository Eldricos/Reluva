// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/briefing.h"

#include <algorithm>
#include <cctype>

namespace reluva::newworlds {

namespace {

using V = BriefingView;

struct Site {
    int a = -1, b = -1; // a map position (eighths of a block, row first); -1 the whole map
};

struct Step {
    const char* clip; // the clip's number after the mission (and side) prefix; "" for none
    uint32_t page;
    V view;
    Site site = {};
    const char* model = ""; // for a model view
};

// The models the recordings' model views showed, matched by shape and
// colour against the archives' models (forbuild, foractor, fedbuild,
// kliactor).
constexpr const char* kTemple = "geo/bad/tt.geo";       // the Taubat temple
constexpr const char* kHubrinCraft = "geo/bad/h.geo";   // a red and white winged craft
constexpr const char* kMetarDevice = "geo/bad/mad.geo"; // a blue faceted crystal, J8's anomaly device
constexpr const char* kPillar = "geo/bad/hcp.geo";      // the containment pillar
constexpr const char* kMetarCore = "geo/bad/mch.geo";   // a Metar spire
constexpr const char* kShieldGen = "geo/fed/fsg.geo";   // the Federation shield generator
constexpr const char* kDisruptorTank = "geo/kln/kdt1.geo";

struct Script {
    const char* mission;
    char side;
    std::vector<Step> steps;
};

// Places the recordings' messages give positions for (map positions,
// eighths of a block, row first). The recordings' map views can't be
// placed exactly (their position readout follows the mouse), so each map
// step looks at the place its page or view is about: the colony (taken
// from the mines it reported exhausted early, which lie beside it), or a
// site a message named. Steps about places no message located, and the
// turning overviews, show the whole map.
constexpr Site kF1Colony{407, 209}, kF1Temple{332, 152};        // the start's readout; the energy source
constexpr Site kK1Colony{244, 92}, kK1Temple{372, 107};         // mine; the Taubat temple
constexpr Site kK2Colony{300, 284}, kK2Ally{279, 26};           // between the two mines; the allied base
constexpr Site kJ1Colony{52, 148}, kJ2Colony{92, 404}, kJ2Alien{112, 260};
constexpr Site kJ3Colony{204, 172}, kJ4Colony{340, 180}, kK12Colony{84, 252}, kK14Colony{100, 292};

// The briefings as recorded play showed them (docs/formats/newworlds/play-*.md):
// which page and view went with each clip. Where the recording didn't show
// a step (a clip not heard), the step is a guess from its neighbours.
const std::vector<Script>& recordedScripts() {
    static const std::vector<Script> scripts = {
        {"F1", 'F', {{"", 3, V::Emblem}, {"005", 2, V::Galaxy}, {"006", 4, V::StarSystem}, {"007", 0, V::Ground, kF1Temple},
                     {"008", 0, V::Ground, kF1Colony}, {"009", 6, V::Ground, kF1Colony}, {"010", 7, V::Relief, kF1Temple},
                     {"011", 8, V::Model, {}, kTemple}, {"012", 9, V::Relief, kF1Colony}, {"013", 10, V::Ground, kF1Temple},
                     {"014", 11, V::Ground, kF1Temple}, {"015", 0, V::Emblem}}},
        {"F2", 'F', {{"", 3, V::Emblem}, {"005", 2, V::Galaxy}, {"006", 4, V::StarSystem}, {"007", 6, V::Ground},
                     {"008", 7, V::Relief}, {"009", 8, V::Relief}, {"010", 9, V::Model}, {"011", 10, V::Emblem},
                     {"012", 11, V::Emblem}}},
        {"K1", 'K', {{"005", 2, V::Galaxy}, {"006", 3, V::StarSystem}, {"007", 4, V::Relief}, {"008", 5, V::Relief, kK1Colony},
                     {"009", 6, V::Ground, kK1Colony}, {"010", 7, V::Ground, kK1Temple}, {"011", 8, V::Ground, kK1Colony},
                     {"012", 9, V::Ground, kK1Temple},
                     {"013", 10, V::Emblem}}},
        {"K2", 'K', {{"005", 4, V::Galaxy}, {"006", 5, V::StarSystem}, {"007", 6, V::Ground, kK2Colony}, {"008", 7, V::Model, {}, kTemple},
                     {"009", 8, V::Relief}, {"010", 9, V::Ground, kK2Ally}, {"011", 10, V::Ground, kK2Colony}, {"012", 0, V::Emblem}}},
        {"J1", 'K', {{"005", 3, V::Galaxy}, {"006", 4, V::StarSystem}, {"007", 5, V::Relief}, {"008", 6, V::Model, {}, kTemple},
                     {"009", 7, V::Relief}, {"010", 8, V::Ground, kJ1Colony}, {"011", 9, V::Ground}, {"012", 10, V::Relief},
                     {"013", 11, V::Relief}, {"014", 12, V::Emblem}}},
        {"J2", 'K', {{"005", 7, V::Galaxy}, {"006", 8, V::StarSystem}, {"007", 5, V::Ground, kJ2Colony}, {"008", 0, V::Relief, kJ2Alien},
                     {"009", 0, V::Ground, kJ2Colony}, {"010", 0, V::Emblem}, {"011", 0, V::Emblem}}},
        {"J3", 'K', {{"005", 10, V::Galaxy}, {"006", 9, V::Galaxy}, {"007", 3, V::StarSystem}, {"008", 0, V::Model, {}, kTemple},
                     {"009", 0, V::Model, {}, kTemple}, {"010", 5, V::Ground}, {"011", 5, V::Ground}, {"012", 0, V::Relief},
                     {"013", 6, V::Ground, kJ3Colony}, {"014", 7, V::Emblem}}},
        {"K7", 'K', {{"005", 2, V::Galaxy}, {"006", 3, V::StarSystem}, {"007", 4, V::Ground}, {"008", 0, V::Emblem},
                     {"009", 0, V::Emblem}, {"010", 0, V::Emblem}, {"011", 0, V::Emblem}, {"012", 0, V::Emblem},
                     {"013", 0, V::Emblem}}},
        {"J4", 'K', {{"005", 5, V::Galaxy}, {"006", 6, V::StarSystem}, {"007", 0, V::Model, {}, kMetarDevice}, {"008", 0, V::Relief, kJ4Colony},
                     {"009", 0, V::Ground, kJ4Colony}, {"010", 7, V::Ground, kJ4Colony}, {"011", 8, V::Ground, kJ4Colony},
                     {"012", 9, V::Ground, kJ4Colony}, {"013", 10, V::Ground, kJ4Colony}, {"014", 11, V::Ground}, {"015", 12, V::Emblem}}},
        // 006-009 weren't heard in the recording (skipped); guessed.
        {"K8", 'K', {{"005", 2, V::Galaxy}, {"006", 3, V::StarSystem}, {"007", 0, V::StarSystem},
                     {"008", 0, V::StarSystem}, {"009", 0, V::StarSystem}, {"010", 4, V::Ground},
                     {"011", 5, V::Ground}, {"012", 0, V::Ground}, {"013", 0, V::Model, {}, kDisruptorTank}, {"014", 0, V::Emblem},
                     {"015", 0, V::Emblem}}},
        {"J6", 'K', {{"005", 3, V::Galaxy}, {"006", 4, V::StarSystem}, {"007", 5, V::Ground}, {"008", 6, V::Model, {}, kShieldGen},
                     {"009", 7, V::Model, {}, kMetarCore}, {"010", 0, V::Emblem}, {"011", 0, V::Emblem}, {"012", 0, V::Emblem},
                     {"013", 0, V::Emblem}, {"014", 0, V::Emblem}}},
        {"J7", 'K', {{"005", 3, V::Galaxy}, {"006", 4, V::StarSystem}, {"007", 5, V::Ground}, {"008", 6, V::Model, {}, kHubrinCraft},
                     {"009", 0, V::Emblem}, {"010", 0, V::Emblem}, {"011", 0, V::Emblem}, {"012", 0, V::Emblem},
                     {"013", 0, V::Emblem}, {"014", 0, V::Emblem}}},
        {"K12", 'K', {{"005", 2, V::Galaxy}, {"006", 3, V::Galaxy}, {"007", 4, V::StarSystem}, {"008", 5, V::Ground, kK12Colony},
                      {"009", 6, V::Model, {}, kPillar}, {"009A", 0, V::Ground}, {"010", 0, V::Ground, kK12Colony},
                      {"011", 0, V::Relief, kK12Colony},
                      {"012", 0, V::Emblem}, {"013", 0, V::Emblem}}},
        {"J8", 'K', {{"", 7, V::Emblem}, {"005", 3, V::Galaxy}, {"006", 4, V::StarSystem}, {"007", 5, V::Model, {}, kMetarDevice},
                     {"008", 6, V::Ground}, {"009", 0, V::Model, {}, kHubrinCraft}, {"010", 0, V::Ground}, {"011", 0, V::Ground}}},
        // 007 and 009 weren't heard (commentary); their pages are guessed.
        {"K14", 'K', {{"005", 2, V::Galaxy}, {"006", 3, V::StarSystem}, {"007", 4, V::Ground}, {"008", 5, V::Model, {}, kHubrinCraft},
                      {"009", 6, V::Ground}, {"010", 0, V::Relief, kK14Colony}, {"011", 0, V::Emblem}}},
        {"J9", 'K', {{"005", 3, V::Galaxy}, {"006", 4, V::Relief}, {"007", 5, V::Relief}, {"008", 6, V::Relief},
                     {"009", 0, V::Relief}}},
        {"C10", 'K', {{"", 74, V::Emblem}}},
    };
    return scripts;
}

bool sameName(std::string_view a, std::string_view b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
               return std::toupper(static_cast<unsigned char>(x)) == std::toupper(static_cast<unsigned char>(y));
           });
}

const Script* findScript(std::string_view shortName, char side) {
    for (const Script& s : recordedScripts())
        if (sameName(s.mission, shortName) && std::toupper(static_cast<unsigned char>(side)) == s.side)
            return &s;
    return nullptr;
}

// A clip's number and letter: "K12009A" -> (9, 'A'); the number is the
// last three digits before an optional letter.
std::pair<int, char> clipOrder(const std::string& stem) {
    size_t end = stem.size();
    char letter = 0;
    if (end && std::isalpha(static_cast<unsigned char>(stem[end - 1])))
        letter = char(std::toupper(static_cast<unsigned char>(stem[--end])));
    int n = 0;
    for (size_t i = end >= 3 ? end - 3 : 0; i < end; ++i)
        n = n * 10 + (std::isdigit(static_cast<unsigned char>(stem[i])) ? stem[i] - '0' : 0);
    return {n, letter};
}

// "~HEADING~text" -> "HEADING"; empty if the string isn't a page.
std::string headingOf(const std::string& text) {
    if (text.size() < 2 || text[0] != '~')
        return {};
    const size_t close = text.find('~', 1);
    return close == std::string::npos ? std::string() : text.substr(1, close - 1);
}

// Pages the generic rule shows: headed, with text, not a transmission or
// the pass and fail pages.
bool briefingPage(const std::string& text) {
    std::string heading = headingOf(text);
    if (heading.empty())
        return false;
    for (char& c : heading)
        c = char(std::toupper(static_cast<unsigned char>(c)));
    if (heading.find("TRANSMISSION") != std::string::npos || heading.find("RECEIVING") != std::string::npos ||
        heading.rfind("MISSION PASS", 0) == 0 || heading.rfind("MISSION FAIL", 0) == 0)
        return false;
    const std::string body = text.substr(heading.size() + 2);
    return body.find_first_not_of(" \t\r\n") != std::string::npos;
}

} // namespace

bool clipBefore(const std::string& a, const std::string& b) { return clipOrder(a) < clipOrder(b); }

bool hasRecordedBriefing(std::string_view shortName, char side) { return findScript(shortName, side) != nullptr; }

std::vector<BriefingStep> briefingScript(std::string_view shortName, char side, const StringTable& table,
                                         const std::vector<std::string>& clips) {
    std::vector<BriefingStep> out;
    std::string prefix(shortName);
    if (!prefix.empty() && std::toupper(static_cast<unsigned char>(prefix[0])) == 'J')
        prefix += char(std::toupper(static_cast<unsigned char>(side)));
    for (char& c : prefix)
        c = char(std::toupper(static_cast<unsigned char>(c)));
    if (const Script* s = findScript(shortName, side)) {
        for (const Step& st : s->steps)
            out.push_back({*st.clip ? prefix + st.clip : std::string(), st.page, st.view, st.site.a, st.site.b, st.model});
        return out;
    }
    std::vector<std::string> sorted = clips;
    std::sort(sorted.begin(), sorted.end(), clipBefore);
    std::vector<uint32_t> pages;
    for (const StringTable::String& s : table.strings())
        if (briefingPage(s.text))
            pages.push_back(s.id);
    const size_t n = std::max(pages.size(), sorted.size());
    for (size_t i = 0; i < n; ++i) {
        BriefingStep step;
        step.clip = i < sorted.size() ? sorted[i] : std::string();
        step.page = i < pages.size() ? pages[i] : 0;
        step.view = i >= pages.size() ? V::Emblem : i == 0 ? V::Galaxy : i == 1 ? V::StarSystem : V::Ground;
        out.push_back(std::move(step));
    }
    return out;
}

} // namespace reluva::newworlds
