// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// A savestate (common/savestate): its file read (header alone, and with
// the state), and a state loaded into every kind of value sync() knows.

#include "fuzz.h"

#include "common/savestate.h"

#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <cstdlib>
#include <string>

namespace {
enum class Kind : uint8_t { A, B, C };
struct Thing {
    int id = 0;
    float x = 0;
    Kind kind = Kind::A, checked = Kind::A;
    std::vector<std::string> names;
    std::optional<double> maybe;
    const int* entry = nullptr;
};
const std::vector<int> kTable{1, 2, 3};
void syncState(reluva::save::Serializer& s, Thing& t) {
    s(t.id)(t.x)(t.kind)(t.names)(t.maybe);
    reluva::save::syncEnum(s, t.checked, Kind::C);
    reluva::save::syncIndex(s, t.entry, kTable);
}
struct State {
    std::vector<Thing> things;
    std::map<int, std::vector<bool>> flags;
    std::map<std::string, std::deque<int64_t>> queues;
    std::set<uint16_t> ids;
    std::unique_ptr<Thing> owned;
    std::array<std::pair<int, float>, 3> pairs{};
    std::vector<uint8_t> blob;
    uint8_t grid[2][3] = {};
    bool flag = false;
    double clock = 0;
};
void syncState(reluva::save::Serializer& s, State& v) {
    s(v.things)(v.flags)(v.queues)(v.ids)(v.owned)(v.pairs)(v.blob)(v.grid)(v.flag)(v.clock);
}
} // namespace

RELUVA_FUZZ_TARGET(savestate)(const uint8_t* data, size_t size) {
    using namespace reluva;
    // The state alone, and wrapped in a file.
    save::Serializer in = save::Serializer::loader(fuzz::bytes(data, size));
    State st;
    in(st);
    if (in.ok()) { // saved again, it loads the same
        save::Serializer out = save::Serializer::saver();
        out(st);
        State again;
        save::Serializer back = save::Serializer::loader(out.take());
        back(again);
        if (!back.ok() || !back.atEnd())
            std::abort();
    }
    const auto path = fuzz::asFile("savestate", data, size);
    save::Header h;
    std::vector<uint8_t> state;
    save::readHeader(path, h);
    save::readFile(path, h, state);
    return 0;
}
