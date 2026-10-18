// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/types.h"

#include <string>
#include <string_view>
#include <vector>

namespace reluva::newworlds {

// A mission listed in level.dat, the main menu's list. See
// docs/formats/newworlds/level.md.
struct Level {
    std::string shortName; // "F1": also the map's name (map/f1.gmf)
    int maxPlayers = 0;
    std::string title;      // "Power Corrupts"
    bool commentedOut = false; // a "//" line (C9-C11, an early J1)
};

// Reads level.dat's text: "short name,max players,title" per line, "//"
// starting a comment. Commented-out lines that still look like a level are
// kept, marked.
Error parseLevelList(std::string_view text, std::vector<Level>& out);

// The level whose short name is the map's file stem ("map/f1.gmf" -> F1),
// ignoring case; listed levels before commented-out ones; nullptr if none.
const Level* levelForMap(const std::vector<Level>& levels, std::string_view mapName);

} // namespace reluva::newworlds
