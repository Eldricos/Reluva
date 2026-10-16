// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/texture.h"

#include <string>

namespace reluva::newworlds {

namespace {

constexpr uint32_t kPalettedWithAlpha = 6;
constexpr uint32_t kMaxSide = 4096;

uint32_t rd32(const std::vector<uint8_t>& d, size_t o) {
    return uint32_t(d[o]) | uint32_t(d[o + 1]) << 8 | uint32_t(d[o + 2]) << 16 | uint32_t(d[o + 3]) << 24;
}

} // namespace

Error decodeTexture(const std::vector<uint8_t>& data, Texture& out) {
    out = Texture{};
    if (data.size() < 8)
        return Error::fail("texture too small");
    out.format = rd32(data, 0);
    if (out.format != kPalettedWithAlpha)
        return Error::fail("unsupported texture format " + std::to_string(out.format));

    const uint32_t paletteCount = rd32(data, 4);
    if (paletteCount > 256)
        return Error::fail("palette has " + std::to_string(paletteCount) + " entries");
    const size_t palette = 8, header = palette + size_t(paletteCount) * 4;
    if (header + 12 > data.size())
        return Error::fail("texture ends inside its header");
    const uint32_t w = rd32(data, header + 4), h = rd32(data, header + 8);
    if (w == 0 || h == 0 || w > kMaxSide || h > kMaxSide)
        return Error::fail("texture size " + std::to_string(w) + "x" + std::to_string(h) + " out of range");
    const size_t pixels = size_t(w) * h;
    const size_t indices = header + 12, alpha = indices + pixels;
    if (alpha + pixels != data.size())
        return Error::fail("texture data is " + std::to_string(data.size()) + " bytes, expected " +
                           std::to_string(alpha + pixels));

    out.width = int(w);
    out.height = int(h);
    out.rgba.resize(pixels * 4);
    for (size_t i = 0; i < pixels; ++i) {
        const uint8_t index = data[indices + i];
        uint8_t* px = &out.rgba[i * 4];
        if (index < paletteCount) {
            const uint8_t* entry = &data[palette + size_t(index) * 4]; // blue, green, red, 0
            px[0] = entry[2];
            px[1] = entry[1];
            px[2] = entry[0];
        } else {
            px[0] = px[1] = px[2] = 0;
        }
        px[3] = data[alpha + i];
    }
    return Error::success();
}

} // namespace reluva::newworlds
