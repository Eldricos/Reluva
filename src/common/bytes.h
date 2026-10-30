// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// Little-endian byte reading, shared by the file-format readers (WAV, Bink,
// save states, the engines' own formats).
//
// The plain readers (u16, u32, s32, f32) don't check bounds: a caller checks
// with fits() (or Reader::has()) first, as each format's own error messages
// depend on where the check fails. The "...Or0" readers check and give 0 past
// the end, for tools that look at arbitrary bytes.

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace reluva::bytes {

inline uint16_t u16(const uint8_t* p) { return uint16_t(p[0] | p[1] << 8); }
inline uint32_t u32(const uint8_t* p) {
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}

// Whether `bytes` bytes starting at `at` lie inside d (without overflowing).
inline bool fits(const std::vector<uint8_t>& d, size_t at, size_t bytes) {
    return at <= d.size() && bytes <= d.size() - at;
}

inline uint16_t u16(const std::vector<uint8_t>& d, size_t o) { return uint16_t(d[o] | d[o + 1] << 8); }
inline uint32_t u32(const std::vector<uint8_t>& d, size_t o) {
    return uint32_t(d[o]) | uint32_t(d[o + 1]) << 8 | uint32_t(d[o + 2]) << 16 | uint32_t(d[o + 3]) << 24;
}
inline int32_t s32(const std::vector<uint8_t>& d, size_t o) { return int32_t(u32(d, o)); }
inline float asFloat(uint32_t v) {
    float f;
    std::memcpy(&f, &v, 4);
    return f;
}
inline float f32(const std::vector<uint8_t>& d, size_t o) { return asFloat(u32(d, o)); }

inline uint16_t u16Or0(const std::vector<uint8_t>& d, size_t o) { return o + 2 <= d.size() ? u16(d, o) : 0; }
inline uint32_t u32Or0(const std::vector<uint8_t>& d, size_t o) { return o + 4 <= d.size() ? u32(d, o) : 0; }

// A view of a file's bytes; check has() before reading.
struct Reader {
    const std::vector<uint8_t>& d;
    bool has(size_t at, size_t bytes) const { return fits(d, at, bytes); }
    uint16_t u16(size_t o) const { return bytes::u16(d, o); }
    uint32_t u32(size_t o) const { return bytes::u32(d, o); }
    float f32(size_t o) const { return bytes::f32(d, o); }
};

} // namespace reluva::bytes
