// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace reluva::newworlds {

// A step of a mission's briefing as the executable's table gives it
// (exe.md#briefings): it lasts the longer of its minimum and its clip.
struct BriefingRow {
    const char* mission;
    char side;          // 'F', 'K', 'R'
    int minimum;        // script seconds (kScriptSecond)
    const char* clip;   // the voice clip's stem ("F1005"; "" silent)
    uint32_t page;      // the string shown (0 none)
    int view;           // 0 relief, 1 ground, 2 model, 3 galaxy, 4 star system, 5 emblem
    int a, b;           // where a map view looks (map position; -1 the whole map)
    int type;           // a model view's object type (0 unknown)
    bool hold;          // waits for the mission script's show (a scene played out)
    int scene;          // the briefing scene's unit it follows (briefingScene; -1 none)
    int scheme;         // the Tricorder's and relief's drawing (0 ring hatch, 1 dot, 2 grid lines)
    int zoom;           // the Tricorder's zoom (0 X4, 1 X2, 2 X1)
    int tricorderView;  // the Tricorder's view (0 Starship, 1 Relief, 2 Relief orbit, 3 Relief fixed)
};

// A briefing scene's unit: what the mission's briefing script flies over
// the map while the briefing plays (F1's shuttle): its type, where it
// starts (map position), its height, and the waypoints it flies to.
struct BriefingScene {
    int type;
    int a, b;
    float height;
    int ways;
    int way[8][2];
};
const BriefingScene* briefingScene(int index);

// A mission's steps for a side, in order (empty if the table has none).
std::vector<const BriefingRow*> briefingRows(std::string_view mission, char side);

// The officer number a section's messages carry in a mission ("science",
// "intelligence", ...; 0 none known): their face is the large portrait
// at that number less one in the race's list (exe.md#the-tricorder).
struct MessageFace {
    const char* mission;
    char side;
    const char* section;
    int number;
};
int messageOfficer(std::string_view mission, char side, std::string_view section);

} // namespace reluva::newworlds
