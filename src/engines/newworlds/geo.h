// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace reluva::newworlds {

// A .geo model (chunk tag 0x17). See docs/formats/newworlds/geo.md; parts of
// the format are still unknown and are skipped.
struct GeoModel {
    struct Vec3 {
        float x, y, z;
    };
    struct Corner {
        uint32_t vertex; // index into vertices
        float u, v;      // texture coordinates, 0..1
    };
    struct Face {
        uint32_t groupA = 0, groupB = 0; // unknown header values (0 for face 0)
        uint32_t flag = 0;               // unknown, 0 or 1
        std::vector<Corner> corners;
    };
    struct Edge {
        uint32_t v0, v1;
        uint32_t faceA, faceB; // the faces on either side (0-based)
    };

    float centreX = 0, centreY = 0, radius = 0;
    std::vector<Vec3> vertices;
    std::vector<std::vector<Vec3>> normalLists;
    std::vector<Face> faces;
    std::vector<Edge> edges;
};

Error parseGeo(const std::vector<uint8_t>& data, GeoModel& out);

} // namespace reluva::newworlds
