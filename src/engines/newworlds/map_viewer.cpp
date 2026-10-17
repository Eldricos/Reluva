// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/view3d.h"
#include "engines/newworlds/viewers.h"

#include <algorithm>
#include <cmath>

namespace reluva::newworlds {

namespace {

constexpr float kTileSize = 8192;          // ground models span -4096..4096 in x and y
constexpr float kBlockSize = 2 * kTileSize;
constexpr float kPi = 3.14159265f;
constexpr float kMaxPitch = 89.0f * kPi / 180.0f;
constexpr float kKeyTurn = 10.0f * kPi / 180.0f;
// Where a block's four tiles go, in its record's order: round the square
// (block 0 of kurt64 numbers its tiles 0, 1, 3, 2). Not confirmed.
constexpr int kSlotX[4] = {0, 1, 1, 0};
constexpr int kSlotY[4] = {0, 0, 1, 1};

} // namespace

MapViewer::MapViewer(System& sys, Surface& screen, const Archives& archives)
    : sys_(sys), screen_(screen), archives_(archives) {
    for (size_t a = 0; a < archives_.size(); ++a) {
        const auto& entries = archives_[a]->entries();
        for (size_t i = 0; i < entries.size(); ++i)
            if (entries[i].tag == cattag::Gmf)
                maps_.push_back({a, i});
    }
    textureFiles_ = listTextureFiles(archives_);
}

void MapViewer::activate() {
    if (!shown_) {
        shown_ = true;
        show(0);
        return;
    }
    dirty_ = true;
    updateTitle();
}

void MapViewer::show(int index) {
    current_ = index;
    status_.clear();
    tileModels_.clear();
    archiveModels_.clear();
    const EntryRef& ref = maps_[size_t(index)];
    CatArchive& cat = *archives_[ref.archive];
    // A tile's model number counts the .geo files of the map's archive.
    for (size_t i = 0; i < cat.entries().size(); ++i)
        if (cat.entries()[i].tag == cattag::Geometry)
            archiveModels_.push_back(i);
    std::vector<uint8_t> data;
    // The tiles' texture layers number the archive's full-size textures in
    // order (.mp0 and .lbp: icewrld's 15 is ice01).
    planetTextures_.clear();
    for (const auto& e : cat.entries()) {
        if (e.tag != cattag::Texture)
            continue;
        std::string folder, stem, ext;
        splitName(e.name, folder, stem, ext);
        if (ext != "mp0" && ext != "lbp")
            continue;
        Texture tex;
        if (cat.read(e, data) || decodeTexture(data, tex))
            tex = Texture{};
        planetTextures_.push_back(std::move(tex));
    }
    if (Error err = cat.read(cat.entries()[ref.entry], data))
        status_ = err.message;
    else if (Error err2 = parseGmf(data, map_))
        status_ = err2.message;
    if (!status_.empty())
        map_ = GmfMap{};
    resetView();
    updateTitle();
}

void MapViewer::resetView() {
    targetX_ = float(map_.width) * kBlockSize / 2;
    targetY_ = float(map_.height) * kBlockSize / 2;
    yaw_ = 0;
    pitch_ = 50.0f * kPi / 180.0f;
    distance_ = std::max(1.0f, float(std::max(map_.width, map_.height)) * kBlockSize * 0.9f);
    dirty_ = true;
}

// A tile's ground model, decoded once per map, with its own textures (used
// where the map's layers give none, or with G): the one named like the model
// without its underscore (ice_07: ice07) first, then those the usual rule
// finds. Faces numbering other textures are untextured.
const MapViewer::TileModel& MapViewer::tileModel(uint32_t index) {
    auto found = tileModels_.find(index);
    if (found != tileModels_.end())
        return found->second;
    TileModel& m = tileModels_[index];
    if (index >= archiveModels_.size()) {
        m.error = "model " + std::to_string(index) + " of " + std::to_string(archiveModels_.size());
        return m;
    }
    CatArchive& cat = *archives_[maps_[size_t(current_)].archive];
    const auto& entry = cat.entries()[archiveModels_[index]];
    m.name = entry.name;
    std::vector<uint8_t> data;
    if (Error err = cat.read(entry, data)) {
        m.error = err.message;
        return m;
    }
    if (Error err = parseGeo(data, m.geo)) {
        m.error = err.message;
        return m;
    }
    for (const auto& face : m.geo.faces) {
        const auto& verts = m.geo.parts[face.part].vertices;
        Vec n{0, 0, 0}; // Newell's method
        for (size_t i = 0; i < face.corners.size(); ++i) {
            const auto& a = verts[face.corners[i].vertex];
            const auto& b = verts[face.corners[(i + 1) % face.corners.size()].vertex];
            n.x += (a.y - b.y) * (a.z + b.z);
            n.y += (a.z - b.z) * (a.x + b.x);
            n.z += (a.x - b.x) * (a.y + b.y);
        }
        n = normalize(n);
        m.faceNormals.push_back({n.x, n.y, n.z});
    }

    size_t needed = 0;
    for (const auto& face : m.geo.faces)
        needed = std::max<size_t>(needed, face.corners.back().extra[4] + 1);
    std::vector<const TextureFile*> slots = findModelTextures(m.name, textureFiles_, needed);
    std::string folder, stem, ext;
    splitName(m.name, folder, stem, ext);
    std::string joined;
    for (char c : stem)
        if (c != '_')
            joined += c;
    for (const TextureFile& t : textureFiles_)
        if (t.folder == folder && t.stem == joined) {
            if (slots.empty())
                slots.push_back(&t);
            else
                slots[0] = &t;
            break;
        }
    for (const TextureFile* t : slots) {
        Texture tex;
        if (t) {
            CatArchive& tcat = *archives_[t->ref.archive];
            if (tcat.read(tcat.entries()[t->ref.entry], data) || decodeTexture(data, tex))
                tex = Texture{};
        }
        m.textures.push_back(std::move(tex));
        m.textureNames.push_back(t ? t->stem : "?");
    }
    return m;
}

void MapViewer::handleEvent(const Event& ev) {
    switch (ev.type) {
    case EventType::MouseDown:
        if (ev.button == MouseButton::Left) {
            dragging_ = true;
            dragX_ = ev.mouseX;
            dragY_ = ev.mouseY;
        }
        return;
    case EventType::MouseUp:
        if (ev.button == MouseButton::Left)
            dragging_ = false;
        return;
    case EventType::MouseMove:
        if (dragging_) {
            yaw_ += float(ev.mouseX - dragX_) * 0.01f;
            pitch_ = std::clamp(pitch_ + float(ev.mouseY - dragY_) * 0.01f, 0.05f, kMaxPitch);
            dragX_ = ev.mouseX;
            dragY_ = ev.mouseY;
            dirty_ = true;
        }
        return;
    case EventType::MouseWheel:
        distance_ = std::clamp(distance_ * std::pow(0.85f, float(ev.wheelY)), kTileSize * 0.5f, 1e8f);
        dirty_ = true;
        return;
    case EventType::KeyDown: break;
    default: return;
    }

    const int n = int(maps_.size());
    // Moving: a tenth of the view distance, relative to the way the camera faces.
    const float step = distance_ * 0.1f;
    const float fx = -std::sin(yaw_), fy = std::cos(yaw_); // forward along the ground
    switch (ev.key) {
    case 'w': targetX_ += fx * step, targetY_ += fy * step; break;
    case 's': targetX_ -= fx * step, targetY_ -= fy * step; break;
    case 'a': targetX_ -= fy * step, targetY_ += fx * step; break;
    case 'd': targetX_ += fy * step, targetY_ -= fx * step; break;
    case 'q': yaw_ -= kKeyTurn; break;
    case 'e': yaw_ += kKeyTurn; break;
    case '=':
    case '+': distance_ = std::max(distance_ * 0.8f, kTileSize * 0.5f); break;
    case '-': distance_ = std::min(distance_ * 1.25f, 1e8f); break;
    case 'r': resetView(); break;
    case 't': textured_ = !textured_; break;
    case 'g': layered_ = !layered_; break;
    case 'l': outlines_ = !outlines_; break;
    case key::Right: show((current_ + 1) % n); return;
    case key::Left: show((current_ + n - 1) % n); return;
    default: return;
    }
    dirty_ = true;
}

void MapViewer::update(uint64_t) {
    if (dirty_) {
        render();
        dirty_ = false;
        updateTitle();
    }
}

void MapViewer::updateTitle() {
    if (maps_.empty())
        return;
    const EntryRef& ref = maps_[size_t(current_)];
    std::string title = "New Worlds maps " + std::to_string(current_ + 1) + "/" + std::to_string(maps_.size()) +
                        ": " + archives_[ref.archive]->entries()[ref.entry].name + " (" +
                        archives_[ref.archive]->path().filename().string();
    if (!status_.empty()) {
        sys_.setWindowTitle(title + "; " + status_ + ")");
        return;
    }
    title += "; " + std::to_string(map_.width) + " x " + std::to_string(map_.height) + " blocks, " +
             std::to_string(map_.tiles.size()) + " tiles, " + std::to_string(map_.blocks.size()) + " blocks; drew " +
             std::to_string(drawnTiles_) + " tiles, " + std::to_string(drawnFaces_) + " faces)";
    if (!textured_)
        title += " [untextured]";
    else if (!layered_)
        title += " [models' own textures]";
    sys_.setWindowTitle(title);
}

void MapViewer::render() {
    const int w = screen_.w, h = screen_.h;
    for (int y = 0; y < h; ++y) { // sky: dark blue at the top
        uint8_t* row = screen_.row(y);
        const uint8_t v = uint8_t(20 + 30 * y / h);
        for (int x = 0; x < w; ++x, row += 4) {
            row[0] = v;
            row[1] = uint8_t(v + 6);
            row[2] = uint8_t(v + 18);
            row[3] = 255;
        }
    }
    drawnTiles_ = drawnFaces_ = 0;
    if (!status_.empty())
        return;

    const OrbitCamera cam(Vec{targetX_, targetY_, 0}, yaw_, pitch_, distance_, w, h);
    const Vec light = normalize(Vec{-0.4f, 0.5f, -0.75f}); // towards the light, in view space
    const float tileRadius = kTileSize * 0.75f;
    raster_.begin(screen_);
    std::vector<ViewVertex> poly;
    std::vector<ScreenVertex> projected;
    std::vector<std::vector<ScreenVertex>> outlines;

    for (int by = 0; by < map_.height; ++by)
        for (int bx = 0; bx < map_.width; ++bx) {
            const auto& block = map_.blocks[map_.cells[size_t(by) * size_t(map_.width) + size_t(bx)].block];
            for (int slot = 0; slot < 4; ++slot) {
                const GmfMap::Tile& tile = map_.tiles[block[size_t(slot)]];
                // Quarter turns anticlockwise about the tile's centre.
                auto turn = [&](float x, float y) {
                    for (int k = 0; k < tile.rotation; ++k) {
                        const float t = x;
                        x = -y;
                        y = t;
                    }
                    return std::pair<float, float>(x, y);
                };
                const Vec origin{(float(bx * 2 + kSlotX[slot]) + 0.5f) * kTileSize,
                                 (float(by * 2 + kSlotY[slot]) + 0.5f) * kTileSize, 0};
                // Skip tiles behind the camera or well off the screen.
                const ViewVertex c = cam.toView(origin);
                if (c.z < -tileRadius)
                    continue;
                if (c.z > cam.nearZ) {
                    const float margin = tileRadius * cam.focal / c.z;
                    const ScreenVertex s = cam.project(c);
                    if (s.x < -margin || s.x > float(w) + margin || s.y < -margin || s.y > float(h) + margin)
                        continue;
                }
                const TileModel& m = tileModel(tile.model);
                if (!m.error.empty())
                    continue;
                ++drawnTiles_;
                for (size_t fi = 0; fi < m.geo.faces.size(); ++fi) {
                    const auto& face = m.geo.faces[fi];
                    const auto& verts = m.geo.parts[face.part].vertices;
                    poly.clear();
                    for (const auto& corner : face.corners) {
                        const auto& v = verts[corner.vertex];
                        const auto [x, y] = turn(v.x, v.y);
                        poly.push_back(cam.toView(Vec{origin.x + x, origin.y + y, origin.z + v.z}, corner.u, corner.v));
                    }
                    poly = clipNear(poly, cam.nearZ);
                    if (poly.size() < 3)
                        continue;
                    projected.clear();
                    for (const auto& p : poly)
                        projected.push_back(cam.project(p));
                    float area = 0; // positive when clockwise on the screen: a front face
                    for (size_t i = 0; i < projected.size(); ++i) {
                        const auto& a = projected[i];
                        const auto& b = projected[(i + 1) % projected.size()];
                        area += a.x * b.y - b.x * a.y;
                    }
                    if (area <= 0)
                        continue;
                    ++drawnFaces_;
                    const auto& fn = m.faceNormals[fi];
                    const auto [nx, ny] = turn(fn.x, fn.y);
                    const float intensity =
                        0.45f + 0.55f * std::fabs(dot(cam.directionToView(Vec{nx, ny, fn.z}), light));
                    const uint32_t textureIndex = face.corners.back().extra[4];
                    const Texture* tex = textured_ && textureIndex < m.textures.size() &&
                                                 !m.textures[textureIndex].rgba.empty()
                                             ? &m.textures[textureIndex]
                                             : nullptr;
                    const bool layers = textured_ && layered_ && !planetTextures_.empty();
                    auto shade = [&](float u, float v, uint8_t* out) {
                        static const uint8_t plain[3] = {170, 180, 195};
                        const uint8_t* col = plain;
                        // The map's layers: the topmost texture in the cell
                        // under (u, v), the tile's own texture coordinates.
                        // The first row and column hold other values, so they
                        // take the next cell's.
                        const Texture* cellTex = nullptr;
                        if (layers) {
                            const int cx = std::clamp(int(u * 8), 1, 7), cy = std::clamp(int(v * 8), 1, 7);
                            for (int layer = GmfMap::kLayers - 1; layer >= 0 && !cellTex; --layer) {
                                const unsigned n = tile.layers[size_t(layer * 64 + cy * 8 + cx)];
                                if (n && n < planetTextures_.size() && !planetTextures_[n].rgba.empty())
                                    cellTex = &planetTextures_[n];
                            }
                        }
                        if (const Texture* tt = cellTex ? cellTex : tex) {
                            int tx = int(std::floor(u * float(tt->width))) % tt->width;
                            int ty = int(std::floor(v * float(tt->height))) % tt->height;
                            tx += tx < 0 ? tt->width : 0;
                            ty += ty < 0 ? tt->height : 0;
                            col = &tt->rgba[(size_t(ty) * size_t(tt->width) + size_t(tx)) * 4];
                            if (col[3] == 0)
                                return false;
                        }
                        for (int i = 0; i < 3; ++i)
                            out[i] = uint8_t(std::min(255.0f, float(col[i]) * intensity));
                        out[3] = 255;
                        return true;
                    };
                    for (size_t i = 1; i + 1 < projected.size(); ++i)
                        raster_.drawTriangle(projected[0], projected[i], projected[i + 1], shade);
                    if (outlines_)
                        outlines.push_back(projected);
                }
            }
        }
    for (const auto& o : outlines)
        for (size_t i = 0; i < o.size(); ++i) {
            const auto& a = o[i];
            const auto& b = o[(i + 1) % o.size()];
            const int steps = int(std::ceil(std::max(std::fabs(b.x - a.x), std::fabs(b.y - a.y))));
            if (steps > 4 * (w + h))
                continue;
            for (int k = 0; k <= steps; ++k) {
                const float t = steps ? float(k) / float(steps) : 0;
                const int x = int(a.x + (b.x - a.x) * t), y = int(a.y + (b.y - a.y) * t);
                if (x >= 0 && y >= 0 && x < w && y < h) {
                    uint8_t* px = screen_.row(y) + x * 4;
                    px[0] = 240, px[1] = 240, px[2] = 200;
                }
            }
        }
}

} // namespace reluva::newworlds
