// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/geo.h"

#include <cmath>
#include <cstring>
#include <string>

namespace reluva::newworlds {

namespace {

constexpr size_t kTexVertexSize = 32;
constexpr size_t kCornerSize = 32;
constexpr float kUvScale = 1.0f / 65536.0f;
constexpr uint32_t kMaxCount = 1u << 20;

struct Reader {
    const std::vector<uint8_t>& d;
    bool has(size_t at, size_t bytes) const { return at <= d.size() && bytes <= d.size() - at; }
    uint32_t u32(size_t o) const {
        return uint32_t(d[o]) | uint32_t(d[o + 1]) << 8 | uint32_t(d[o + 2]) << 16 | uint32_t(d[o + 3]) << 24;
    }
    float f32(size_t o) const {
        const uint32_t v = u32(o);
        float f;
        std::memcpy(&f, &v, 4);
        return f;
    }
    GeoModel::Vec3 vec3(size_t o) const { return {f32(o), f32(o + 4), f32(o + 8)}; }
};

std::string hex(size_t v) {
    char buf[24];
    std::snprintf(buf, sizeof buf, "0x%zx", v);
    return buf;
}

bool isUnit(const GeoModel::Vec3& v) {
    const float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return std::fabs(len - 1.0f) < 0.01f;
}

} // namespace

Error parseGeo(const std::vector<uint8_t>& data, GeoModel& out) {
    out = GeoModel{};
    const Reader r{data};
    if (!r.has(0, 16))
        return Error::fail("model too small");
    const uint32_t faceCount = r.u32(4) ? r.u32(4) - 1 : 0; // the header counts one more than there are
    const uint32_t texVertexCount = r.u32(8);
    if (faceCount > kMaxCount || texVertexCount > kMaxCount)
        return Error::fail("model header counts out of range");

    // Texture vertices: u32 vertex, f32 u, f32 v, then 20 bytes not yet understood.
    size_t p = 16;
    if (!r.has(p, size_t(texVertexCount) * kTexVertexSize))
        return Error::fail("model ends inside its texture vertices");
    p += size_t(texVertexCount) * kTexVertexSize;

    // Faces: u32, u32, u32 corner count, u32 0, then 32-byte corners laid out
    // like texture vertices.
    for (uint32_t i = 0; i < faceCount; ++i) {
        if (!r.has(p, 16))
            return Error::fail("model ends inside face " + std::to_string(i));
        GeoModel::Face face;
        face.groupA = r.u32(p);
        face.groupB = r.u32(p + 4);
        const uint32_t n = r.u32(p + 8);
        if (n < 3 || n > 64 || r.u32(p + 12) != 0 || !r.has(p + 16, size_t(n) * kCornerSize))
            return Error::fail("face " + std::to_string(i) + " at " + hex(p) + " has a bad header");
        p += 16;
        for (uint32_t k = 0; k < n; ++k, p += kCornerSize)
            face.corners.push_back({r.u32(p), r.f32(p + 4) * kUvScale, r.f32(p + 8) * kUvScale});
        out.faces.push_back(std::move(face));
    }

    // Bounds: u32 0, 1, 1, attachment count, then centre x, y and radius.
    if (!r.has(p, 28) || r.u32(p) != 0 || r.u32(p + 4) != 1 || r.u32(p + 8) != 1)
        return Error::fail("no bounds block after the faces at " + hex(p));
    out.centreX = r.f32(p + 16);
    out.centreY = r.f32(p + 20);
    out.radius = r.f32(p + 24);
    p += 28;

    // Attachment data of unknown layout follows; find the vertex block, which
    // starts u32 size, 1, 1, 0, count and is followed by a list of unit normals.
    size_t vertexBlock = 0;
    for (size_t q = p; r.has(q, 20); q += 4) {
        const uint32_t n = r.u32(q + 16);
        if (r.u32(q + 4) != 1 || r.u32(q + 8) != 1 || r.u32(q + 12) != 0 || n == 0 || n > kMaxCount)
            continue;
        const size_t after = q + 20 + size_t(n) * 12;
        if (r.has(after, 20) && r.u32(after) == 1 && r.u32(after + 4) > 0 && isUnit(r.vec3(after + 8))) {
            vertexBlock = q;
            break;
        }
    }
    if (!vertexBlock)
        return Error::fail("vertex block not found after " + hex(p));
    p = vertexBlock + 16;
    const uint32_t vertexCount = r.u32(p);
    p += 4;
    for (uint32_t i = 0; i < vertexCount; ++i, p += 12)
        out.vertices.push_back(r.vec3(p));

    // Normal lists: u32 1, u32 count, count unit vectors; repeated.
    while (r.has(p, 20) && r.u32(p) == 1 && r.u32(p + 4) > 0 && r.u32(p + 4) <= kMaxCount && isUnit(r.vec3(p + 8))) {
        const uint32_t n = r.u32(p + 4);
        if (!r.has(p + 8, size_t(n) * 12))
            return Error::fail("model ends inside a normal list");
        std::vector<GeoModel::Vec3> list;
        for (uint32_t i = 0; i < n; ++i)
            list.push_back(r.vec3(p + 8 + size_t(i) * 12));
        out.normalLists.push_back(std::move(list));
        p += 8 + size_t(n) * 12;
    }

    // Edges: u32 1, 1, count, 0, then count x (v0, v1, faceA, faceB, 0).
    if (!r.has(p, 16) || r.u32(p) != 1 || r.u32(p + 4) != 1 || r.u32(p + 12) != 0)
        return Error::fail("edge list not found at " + hex(p));
    const uint32_t edgeCount = r.u32(p + 8);
    p += 16;
    if (edgeCount > kMaxCount || !r.has(p, size_t(edgeCount) * 20))
        return Error::fail("model ends inside its edge list");
    for (uint32_t i = 0; i < edgeCount; ++i, p += 20)
        out.edges.push_back({r.u32(p), r.u32(p + 4), r.u32(p + 8), r.u32(p + 12)});

    // Consistency: every index in range, every edge's vertices on its faces.
    for (const auto& face : out.faces)
        for (const auto& c : face.corners)
            if (c.vertex >= out.vertices.size())
                return Error::fail("face corner uses vertex " + std::to_string(c.vertex) + " of " +
                                   std::to_string(out.vertices.size()));
    auto onFace = [&](uint32_t vertex, uint32_t face) {
        if (face == 0)
            return true;
        for (const auto& c : out.faces[face - 1].corners)
            if (c.vertex == vertex)
                return true;
        return false;
    };
    for (size_t i = 0; i < out.edges.size(); ++i) {
        const auto& e = out.edges[i];
        if (e.v0 >= out.vertices.size() || e.v1 >= out.vertices.size() || e.faceA > out.faces.size() ||
            e.faceB > out.faces.size())
            return Error::fail("edge " + std::to_string(i) + " is out of range");
        if (!onFace(e.v0, e.faceA) || !onFace(e.v1, e.faceA) || !onFace(e.v0, e.faceB) || !onFace(e.v1, e.faceB))
            return Error::fail("edge " + std::to_string(i) + " doesn't lie on its faces");
    }
    return Error::success();
}

} // namespace reluva::newworlds
