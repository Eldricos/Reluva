// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/texture.h"

#include <string>

namespace reluva::newworlds {

namespace {

constexpr uint32_t kPaletted = 1;          // one layer: palette indices
constexpr uint32_t kPalettedWithAlpha = 6; // two layers: palette indices, then alpha
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
    int layers;
    if (out.format == kPaletted)
        layers = 1;
    else if (out.format == kPalettedWithAlpha)
        layers = 2;
    else
        return Error::fail("unsupported texture format " + std::to_string(out.format));

    const uint32_t paletteCount = rd32(data, 4);
    if (paletteCount > 256)
        return Error::fail("palette has " + std::to_string(paletteCount) + " entries");
    const size_t palette = 8, header = palette + size_t(paletteCount) * 4;
    if (header + 4 > data.size())
        return Error::fail("texture ends inside its header");
    out.sizeFlag = rd32(data, header);

    uint32_t w = 0, h = 0;
    size_t indices = 0;
    if (out.sizeFlag == 0) {
        if (header + 12 > data.size())
            return Error::fail("texture ends inside its header");
        w = rd32(data, header + 4);
        h = rd32(data, header + 8);
        indices = header + 12;
    } else if (out.sizeFlag == 1) {
        // The size isn't stored (it may be given elsewhere). Infer it: square
        // if possible, otherwise twice as wide as high.
        indices = header + 4;
        const size_t rest = data.size() - indices;
        const size_t n = rest / size_t(layers);
        if (rest % size_t(layers) != 0 || n == 0)
            return Error::fail("size not stored and data length doesn't divide into layers");
        uint32_t side = 1;
        while (size_t(side) * side < n)
            ++side;
        if (size_t(side) * side == n) {
            w = h = side;
        } else {
            uint32_t half = 1;
            while (size_t(half) * half * 2 < n)
                ++half;
            if (size_t(half) * half * 2 != n)
                return Error::fail("size not stored; " + std::to_string(n) + " pixels isn't a square or 2:1");
            w = half * 2;
            h = half;
        }
        out.sizeGuessed = true;
    } else {
        return Error::fail("unknown value " + std::to_string(out.sizeFlag) + " after the palette");
    }

    if (w == 0 || h == 0 || w > kMaxSide || h > kMaxSide)
        return Error::fail("texture size " + std::to_string(w) + "x" + std::to_string(h) + " out of range");
    const size_t pixels = size_t(w) * h;
    const size_t alpha = indices + pixels;
    if (indices + pixels * size_t(layers) != data.size())
        return Error::fail("texture data is " + std::to_string(data.size()) + " bytes, expected " +
                           std::to_string(indices + pixels * size_t(layers)));

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
        px[3] = layers == 2 ? data[alpha + i] : 255;
    }
    return Error::success();
}

} // namespace reluva::newworlds
