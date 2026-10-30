// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// Reluva's mission scripts: what each mission reacts to and how. They
// follow the game's own logic, as studied in its executable
// (docs/formats/newworlds/exe.md#the-script-engine): the same objectives
// and fail criteria (the mission's strings, by id), triggers, conditions
// and delays, written in Reluva's own format and words - the messages
// paraphrase the game's. Each is a file in scripts/ (listed in
// scripts/scripts.cmake), built into the program (cmake/EmbedFiles.cmake):
// the rules every mission shares (generic.nws), then a file for each
// mission and side. Each script ends with what it can't say yet ("Not
// expressible yet").

#include "engines/newworlds/mission_script.h"

#include <cstddef>
#include <cstdio>
#include <iterator>
#include <string>

namespace reluva::newworlds {

namespace {

struct EmbeddedFile {
    const char* name; // scripts/c4.nws
    const unsigned char* data;
    std::size_t size;
};

#include "mission_scripts.inc" // kEmbeddedFiles: scripts/*.nws

} // namespace

const std::vector<MissionScript>& missionScripts() {
    static const std::vector<MissionScript> scripts = [] {
        std::vector<MissionScript> s, file;
        for (const EmbeddedFile& f : kEmbeddedFiles) {
            const std::string_view text(reinterpret_cast<const char*>(f.data), f.size);
            // (The tests parse them, so this shouldn't happen; if it does, play
            // goes on unscripted, and says why.)
            if (Error err = parseMissionScripts(text, file, f.name)) {
                std::fprintf(stderr, "mission scripts: %s\n", err.message.c_str());
                return std::vector<MissionScript>();
            }
            s.insert(s.end(), std::make_move_iterator(file.begin()), std::make_move_iterator(file.end()));
        }
        return s;
    }();
    return scripts;
}

} // namespace reluva::newworlds
