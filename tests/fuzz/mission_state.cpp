// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// A New Worlds mission's savestate (the state a save file carries, after
// its header): loaded into F1 as the game loads one, and if it's taken,
// played on a second.

#include "fuzz.h"

#include "engines/newworlds/mission_sim.h"

RELUVA_FUZZ_TARGET(mission_state)(const uint8_t* data, size_t size) {
    using namespace reluva;
    using namespace reluva::newworlds;
    newworlds::Terrain t;
    t.width = t.height = 128;
    t.moveClass.assign(size_t(t.width) * size_t(t.height), 1);
    t.heightAt = [](float, float) { return 0.0f; };
    ColonyMap map;
    map.depositKind = [](int bx, int by) { return (bx + by) % 9 == 0 ? 1 : 0; };
    map.depositAmount = [](int, int) { return 500; };
    MissionSim sim("F1", 'F', t, map, 64, 64, MissionSim::placedColony("F1", 'F'));
    save::Serializer in = save::Serializer::loader(fuzz::bytes(data, size));
    sim.syncState(in);
    if (in.ok()) // (the page's own state follows it in a save)
        sim.advance(1);
    return 0;
}
