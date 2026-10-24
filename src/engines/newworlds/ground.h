// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/types.h"
#include "engines/newworlds/cat_archive.h"
#include "engines/newworlds/geo.h"
#include "engines/newworlds/gmf.h"
#include "engines/newworlds/world.h"

#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace reluva::newworlds {

// The height of a ground tile's model at a point in the tile's own frame
// (its middle at 0, 0, before its turn): the highest face over it; 0 if
// none.
float geoGroundHeight(const GeoModel& geo, float lx, float ly);

// The terrain units move on, from a map's tiles (their movement class,
// bits 16-17 of the flags) and the ground's height.
Terrain terrainOf(const GmfMap& map, std::function<float(float x, float y)> heightAt);

// A mission map's ground without drawing it: the map and its tiles'
// models, for heights, deposits and the terrain (headless runs).
class Ground {
public:
    // The map named ("map/f1.gmf") from whichever archive holds it.
    Error load(const std::vector<std::unique_ptr<CatArchive>>& archives, std::string_view mapName);

    const GmfMap& map() const { return map_; }
    int blocksWide() const { return map_.width; }
    int blocksHigh() const { return map_.height; }
    float height(float x, float y) const;
    int depositAt(int bx, int by) const;
    int depositAmount(int bx, int by) const;
    // Holds on to this ground: it must outlive the terrain.
    Terrain terrain() const;

private:
    GmfMap map_;
    std::vector<GeoModel> models_; // by the tiles' model numbers
    std::vector<bool> loaded_;
};

} // namespace reluva::newworlds
