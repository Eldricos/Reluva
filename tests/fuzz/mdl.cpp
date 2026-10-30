// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// A New Worlds .mdl model description.

#include "fuzz.h"

#include "engines/newworlds/mdl.h"

RELUVA_FUZZ_TARGET(mdl)(const uint8_t* data, size_t size) {
    reluva::newworlds::MdlModel m;
    reluva::newworlds::parseMdl(reluva::fuzz::bytes(data, size), m);
    return 0;
}
