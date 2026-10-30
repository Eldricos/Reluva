// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// The New Worlds file-format readers' byte helpers: the little-endian
// readers (common/bytes.h), and offsets and names for their messages.

#pragma once

#include "common/bytes.h"

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace reluva::newworlds::bytes {

using reluva::bytes::asFloat;
using reluva::bytes::f32;
using reluva::bytes::fits;
using reluva::bytes::Reader;
using reluva::bytes::s32;
using reluva::bytes::u16;
using reluva::bytes::u16Or0;
using reluva::bytes::u32;
using reluva::bytes::u32Or0;

// "0x1a2b", for offsets in error messages.
inline std::string hex(uint64_t v) {
    char buf[24];
    std::snprintf(buf, sizeof buf, "0x%llx", static_cast<unsigned long long>(v));
    return buf;
}

// ASCII case-insensitive equality, for mission and archive names.
inline bool sameName(std::string_view a, std::string_view b) {
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    return true;
}

} // namespace reluva::newworlds::bytes
