// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/types.h"

#include <algorithm>
#include <cctype>
#include <string>

// What the missions page's files (mission_*.cpp, MissionViewer's parts)
// share: the page's colours, the panels' layout, the side's bees.

namespace reluva::newworlds {

constexpr Color kHeadingColor{255, 220, 90};
constexpr Color kTextColor{230, 230, 230};
constexpr Color kNoteColor{150, 160, 190};
constexpr int kPanelMargin = 12; // round the page's panel, and inside it

// The game's panels (GameUi::drawPanel) round lines of text: the title
// bar and frame 30 pixels high besides the lines, the first line 15 under
// the panel's top, the lines 12 in from its sides.
constexpr int kPanelTitleH = 30;
constexpr int kPanelFirstLine = 15;
constexpr int kPanelInset = 12;

constexpr int kTricorderTiles = 16; // tiles across the small Tricorder at its closest (X4)

// JUMP TO's left column (exe.md#the-colony-overview): per race, the type
// whose portrait each slot shows: the hub, the mining stations, the
// defensive weapons, the defensive artillery, the fighting vehicles.
constexpr int kJumpIcons[3][5] = {{23, 45, 80, 83, 56}, {28, 52, 54, 82, 40}, {43, 65, 81, 84, 64}};

inline std::string upperCase(std::string s) {
    for (char& c : s)
        c = char(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

// A name as the panels give it: upper case, a dash a space
// ("mining-station": "MINING STATION").
inline std::string upperName(std::string s) {
    std::replace(s.begin(), s.end(), '-', ' ');
    return upperCase(std::move(s));
}

// A side's bees and cargo pods (the Federation's crate, the Klingons' and
// Romulans' own), the Federation's for anyone else.
struct BeeModels {
    const char *construction, *cargo, *worker, *pod;
};

inline const BeeModels& beeModels(char side) {
    static constexpr BeeModels kFed{"geo/fed/fconbee.geo", "geo/fed/fcarb.geo", "geo/fed/workbee.geo",
                                    "geo/fed/crate_l.geo"};
    static constexpr BeeModels kKln{"geo/kln/conbee.geo", "geo/kln/kwb.geo", "geo/kln/kwb.geo", "geo/kln/kwbc.geo"};
    static constexpr BeeModels kRom{"geo/rom/rcb.geo", "geo/rom/rwbc.geo", "geo/rom/rwb.geo", "geo/rom/rc.geo"};
    return side == 'K' ? kKln : side == 'R' ? kRom : kFed;
}

} // namespace reluva::newworlds
