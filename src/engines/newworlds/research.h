// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "engines/newworlds/ai_data.h"
#include "engines/newworlds/mission_script.h"

#include <string>
#include <string_view>
#include <vector>

namespace reluva::newworlds {

// A side's research, as the game's executable runs it
// (docs/formats/newworlds/exe.md#research): the mission's topics, each
// locked until the script has sent its name a set number of times (0, 1
// or 2); the player starts an open one (the Operations panel's Research
// tab), and every tick the side's 1000 research points are shared out
// between the topics running. No building, officer or crew enters into
// it. A finished topic sends researched <topic> to the scripts.
class Research {
public:
    static constexpr int kPointsPerTick = 1000;

    Research(std::string_view mission, int slot);

    struct Topic {
        std::string name;    // as the scripts name it ("taubat-colony")
        std::vector<std::string> aliases; // other names the script used for it
        int field = 0;       // 1 medical, 3 science (2, 4 also occur)
        int points = 0;      // needed
        int locks = 0;       // the script's unlocks still wanted
        int done = 0;        // points so far
        bool running = false, finished = false;
        bool open() const { return locks < 1; }
    };
    const std::vector<Topic>& topics() const { return topics_; }

    // A script's research action: one unlock of the topic it names (a
    // topic the mission doesn't have is added, open, at 100000 points).
    void unlock(const std::string& name);
    // The player starts an open topic (by its index); false if it can't.
    bool start(size_t index);
    // The topics open and not finished, by index, for the panel.
    std::vector<size_t> offered() const;

    std::vector<ScriptEvent> advance(double seconds);

private:
    Topic* find(const std::string& name);

    std::vector<Topic> topics_;
    std::vector<size_t> queue_;
    double clock_ = 0;
};

} // namespace reluva::newworlds
