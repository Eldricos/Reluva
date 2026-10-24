// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/ground.h"

#include <cmath>

namespace reluva::newworlds {

float geoGroundHeight(const GeoModel& geo, float lx, float ly) {
    bool found = false;
    float best = 0;
    for (const auto& face : geo.faces) {
        const auto& verts = geo.parts[face.part].vertices;
        const auto& a = verts[face.corners[0].vertex];
        for (size_t i = 1; i + 1 < face.corners.size(); ++i) {
            const auto& b = verts[face.corners[i].vertex];
            const auto& c = verts[face.corners[i + 1].vertex];
            const float d = (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
            if (std::fabs(d) < 1e-3f)
                continue;
            const float u = ((lx - a.x) * (c.y - a.y) - (c.x - a.x) * (ly - a.y)) / d;
            const float v = ((b.x - a.x) * (ly - a.y) - (lx - a.x) * (b.y - a.y)) / d;
            if (u < -1e-4f || v < -1e-4f || u + v > 1 + 1e-4f)
                continue;
            const float z = a.z + u * (b.z - a.z) + v * (c.z - a.z);
            if (!found || z > best)
                best = z, found = true;
        }
    }
    return best;
}

Terrain terrainOf(const GmfMap& map, std::function<float(float x, float y)> heightAt) {
    Terrain t;
    if (map.width <= 0)
        return t;
    t.width = map.width * 2;
    t.height = map.height * 2;
    t.moveClass.resize(size_t(t.width) * size_t(t.height));
    for (int ty = 0; ty < t.height; ++ty)
        for (int tx = 0; tx < t.width; ++tx) {
            const int r = tileAt(map, tx, ty);
            t.moveClass[size_t(ty) * size_t(t.width) + size_t(tx)] = r < 0 ? 0 : int((map.tiles[size_t(r)].flags >> 16) & 3);
        }
    t.heightAt = std::move(heightAt);
    return t;
}

Error Ground::load(const std::vector<std::unique_ptr<CatArchive>>& archives, std::string_view mapName) {
    const std::string wanted = CatArchive::normalizeName(mapName);
    for (const auto& cat : archives)
        for (const auto& entry : cat->entries()) {
            if (entry.tag != cattag::Gmf || CatArchive::normalizeName(entry.name) != wanted)
                continue;
            std::vector<uint8_t> data;
            if (Error err = cat->read(entry, data))
                return err;
            if (Error err = parseGmf(data, map_))
                return err;
            // A tile's model number counts the archive's .geo files in the
            // order tileModelOrder gives.
            std::vector<size_t> geoEntries;
            std::vector<std::string> geoNames;
            for (size_t i = 0; i < cat->entries().size(); ++i)
                if (cat->entries()[i].tag == cattag::Geometry) {
                    geoEntries.push_back(i);
                    geoNames.push_back(cat->entries()[i].name);
                }
            const std::vector<size_t> order = tileModelOrder(geoNames);
            models_.assign(order.size(), GeoModel{});
            loaded_.assign(order.size(), false);
            for (size_t n = 0; n < order.size(); ++n) {
                if (order[n] == kNoModel)
                    continue;
                if (!cat->read(cat->entries()[geoEntries[order[n]]], data) && !parseGeo(data, models_[n]))
                    loaded_[n] = true;
            }
            return {};
        }
    return Error::fail("no map " + std::string(mapName));
}

float Ground::height(float x, float y) const {
    constexpr float kTileSize = 8192;
    const int tx = int(std::floor(x / kTileSize)), ty = int(std::floor(y / kTileSize));
    const int t = tileAt(map_, tx, ty);
    if (t < 0)
        return 0;
    const GmfMap::Tile& tile = map_.tiles[size_t(t)];
    if (tile.model >= loaded_.size() || !loaded_[tile.model])
        return 0;
    // Into the model's own frame: undo the tile's turn.
    float lx = x - (float(tx) + 0.5f) * kTileSize, ly = y - (float(ty) + 0.5f) * kTileSize;
    for (int k = 0; k < tile.rotation; ++k) {
        const float t0 = lx;
        lx = ly;
        ly = -t0;
    }
    return geoGroundHeight(models_[tile.model], lx, ly);
}

int Ground::depositAt(int bx, int by) const {
    if (bx < 0 || by < 0 || bx >= map_.width || by >= map_.height)
        return 0;
    return map_.cells[size_t(by) * size_t(map_.width) + size_t(bx)].deposit();
}

int Ground::depositAmount(int bx, int by) const {
    if (bx < 0 || by < 0 || bx >= map_.width || by >= map_.height)
        return 0;
    return map_.cells[size_t(by) * size_t(map_.width) + size_t(bx)].value & 0xfff;
}

Terrain Ground::terrain() const {
    return terrainOf(map_, [this](float x, float y) { return height(x, y); });
}

} // namespace reluva::newworlds
