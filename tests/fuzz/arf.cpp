// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// A New Worlds .arf animation frame, and a transform read at each offset.

#include "fuzz.h"

#include "engines/newworlds/arf.h"

RELUVA_FUZZ_TARGET(arf)(const uint8_t* data, size_t size) {
    using namespace reluva::newworlds;
    const std::vector<uint8_t> in = reluva::fuzz::bytes(data, size);
    std::vector<PartTransform> frame;
    parseArf(in, frame);
    PartTransform t;
    for (size_t at = 0; at < in.size() + 2 && at < 256; ++at)
        readTransform(in, at, t);
    return 0;
}
