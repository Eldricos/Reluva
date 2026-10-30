// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/types.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>

// Drawing on an RGBA32 surface (the missions page's and play's): points,
// lines and rectangles, clipped to the surface, its alpha left as it is
// (but fillScreen's, opaque).

namespace reluva::newworlds {

inline void plot(Surface& s, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x >= 0 && y >= 0 && x < s.w && y < s.h) {
        uint8_t* p = s.row(y) + x * 4;
        p[0] = r, p[1] = g, p[2] = b;
    }
}

inline void plot(Surface& s, int x, int y, Color c) { plot(s, x, y, c.r, c.g, c.b); }

// A point at every pixel along it (none for one over 4000 long).
inline void line(Surface& s, int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b) {
    const int n = std::max({std::abs(x1 - x0), std::abs(y1 - y0), 1});
    if (n > 4000)
        return;
    for (int i = 0; i <= n; ++i)
        plot(s, x0 + (x1 - x0) * i / n, y0 + (y1 - y0) * i / n, r, g, b);
}

inline void fillRect(Surface& s, int x, int y, int w, int h, Color c) {
    for (int yy = std::max(0, y); yy < y + h && yy < s.h; ++yy) {
        uint8_t* p = s.row(yy) + std::max(0, x) * 4;
        for (int xx = std::max(0, x); xx < x + w && xx < s.w; ++xx, p += 4)
            p[0] = c.r, p[1] = c.g, p[2] = c.b;
    }
}

// A frame a pixel wide: the rectangle's outermost pixels.
inline void frameRect(Surface& s, int x, int y, int w, int h, Color c) {
    fillRect(s, x, y, w, 1, c);
    fillRect(s, x, y + h - 1, w, 1, c);
    fillRect(s, x, y, 1, h, c);
    fillRect(s, x + w - 1, y, 1, h, c);
}

// Each pixel a quarter as bright, then `tint` added.
inline void dimRect(Surface& s, int x, int y, int w, int h, Color tint = {}) {
    for (int yy = std::max(0, y); yy < y + h && yy < s.h; ++yy) {
        uint8_t* p = s.row(yy) + std::max(0, x) * 4;
        for (int xx = std::max(0, x); xx < x + w && xx < s.w; ++xx, p += 4)
            p[0] = uint8_t(p[0] / 4 + tint.r), p[1] = uint8_t(p[1] / 4 + tint.g), p[2] = uint8_t(p[2] / 4 + tint.b);
    }
}

// The whole surface one colour, opaque.
inline void fillScreen(Surface& s, Color c) {
    for (int y = 0; y < s.h; ++y) {
        uint8_t* row = s.row(y);
        for (int x = 0; x < s.w; ++x, row += 4)
            row[0] = c.r, row[1] = c.g, row[2] = c.b, row[3] = 255;
    }
}

} // namespace reluva::newworlds
