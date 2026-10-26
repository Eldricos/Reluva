// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "engines/newworlds/string_table.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace reluva::newworlds {

// What a briefing step shows behind its page (see
// docs/formats/newworlds/play.md).
enum class BriefingView {
    Galaxy,     // the Tabula Rasa galaxy
    StarSystem, // the mission's star system
    Ground,     // the map, from the ground
    Relief,     // the map as the relief grid
    Model,      // a building or ship turning on a grid
    Emblem,     // the side's emblem
};

// One step of a mission's briefing: a voice clip (its stem, e.g. "K1005";
// empty for a silent step), the string table's page shown (its id; 0 for
// none) and the view behind it.
struct BriefingStep {
    std::string clip;
    uint32_t page = 0;
    BriefingView view = BriefingView::Emblem;
    // For a map view, the place it looks at: a map position as the game
    // gives them (eighths of a block, row first), or -1 for the whole map.
    int siteA = -1, siteB = -1;
    // For a model view, the model shown ("geo/bad/tt.geo"); empty if unknown.
    std::string model;
    // How long it lasts at least, in seconds (it lasts the longer of this
    // and its clip).
    double minimum = 0;
    // The briefing scene's unit the camera follows (briefingScene; -1 none).
    int scene = -1;
};

// The briefing of a mission played as a side ('F', 'K', 'R'): the
// executable's own step table (briefing_data.h; every mission has one);
// for one without, built by the rule the recordings showed: the clips in
// number order, one per step, given to the table's heading pages in order
// (leaving out the transmission, pass and fail pages and empty ones), the
// first over the galaxy, the second over the star system, the rest over
// the map; clips left over play over the emblem.
// clips: the mission's voice clips (stems), sorted.
std::vector<BriefingStep> briefingScript(std::string_view shortName, char side, const StringTable& table,
                                         const std::vector<std::string>& clips);

// Whether the executable has a mission's briefing table.
bool hasRecordedBriefing(std::string_view shortName, char side);

// The order clips play in: by number, a lettered clip right after its
// number (K12009 before K12009A before K12010).
bool clipBefore(const std::string& a, const std::string& b);

} // namespace reluva::newworlds
