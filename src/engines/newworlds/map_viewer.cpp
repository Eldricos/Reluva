// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/ground_textures.h"
#include "engines/newworlds/view3d.h"
#include "engines/newworlds/viewers.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace reluva::newworlds {

namespace {

constexpr float kTileSize = 8192;          // ground models span -4096..4096 in x and y
constexpr float kBlockSize = 2 * kTileSize;
static_assert(kBlockSize == MapViewer::kBlockUnits);
constexpr float kPi = 3.14159265f;
constexpr float kMaxPitch = 89.0f * kPi / 180.0f;
constexpr float kKeyTurn = 10.0f * kPi / 180.0f;
// Where a block's four tiles go, in its record's order: round the square
// (block 0 of kurt64 numbers its tiles 0, 1, 3, 2). Not confirmed.
constexpr int kSlotX[4] = {0, 1, 1, 0};
constexpr int kSlotY[4] = {0, 0, 1, 1};
// How the ground is coloured: with the models' own textures, with the colour
// of each cell's terrain code (from the tile's layers), or textured and tinted
// where the grid marks a resource deposit.
constexpr int kColourings = 3;
constexpr const char* kColouringNames[kColourings] = {"models' own textures", "terrain codes",
                                                      "resource deposits"};
// Tints for the six deposit kinds (which resource each is isn't known).
constexpr uint8_t kDepositColours[6][3] = {{255, 60, 60}, {255, 230, 40}, {60, 230, 60},
                                           {40, 230, 255}, {255, 60, 255}, {255, 255, 255}};

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
    tileEdges_.clear();
    archiveModels_.clear();
    const EntryRef& ref = maps_[size_t(index)];
    CatArchive& cat = *archives_[ref.archive];
    // A tile's model number counts the .geo files of the map's archive, in
    // the order tileModelOrder gives.
    std::vector<size_t> geoEntries;
    std::vector<std::string> geoNames;
    for (size_t i = 0; i < cat.entries().size(); ++i)
        if (cat.entries()[i].tag == cattag::Geometry) {
            geoEntries.push_back(i);
            geoNames.push_back(cat.entries()[i].name);
        }
    for (size_t k : tileModelOrder(geoNames))
        archiveModels_.push_back(k == kNoModel ? kNoModel : geoEntries[k]);
    std::vector<uint8_t> data;
    // Walls get the archive's last full-size cliff texture (icewrld's
    // cliff07, which no ground model's name matches). A guess.
    wallTexture_ = Texture{};
    for (auto e = cat.entries().rbegin(); e != cat.entries().rend(); ++e) {
        std::string folder, stem, ext;
        splitName(e->name, folder, stem, ext);
        if (e->tag != cattag::Texture || ext != "mp0" || stem.rfind("cliff", 0) != 0)
            continue;
        if (cat.read(*e, data) || decodeTexture(data, wallTexture_))
            wallTexture_ = Texture{};
        break;
    }
    if (Error err = cat.read(cat.entries()[ref.entry], data))
        status_ = err.message;
    else if (Error err2 = parseGmf(data, map_))
        status_ = err2.message;
    if (!status_.empty())
        map_ = GmfMap{};
    placeScenery();
    buildings_.clear();
    buildingAreas_.clear();
    hidden_.clear();
    paved_.clear();
    pavedModels_.clear();
    makeUnderlay();
    resetView();
    updateTitle();
}

bool MapViewer::showMap(std::string_view name) {
    const std::string wanted = CatArchive::normalizeName(name);
    for (size_t i = 0; i < maps_.size(); ++i)
        if (CatArchive::normalizeName(archives_[maps_[i].archive]->entries()[maps_[i].entry].name) == wanted) {
            shown_ = true;
            show(int(i));
            return true;
        }
    return false;
}

void MapViewer::lookAt(int a, int b, bool ground) {
    resetView();
    if (a >= 0 && b >= 0) {
        // Row first: the second number is the column, along x.
        targetX_ = float(b) / 8 * kBlockSize;
        targetY_ = float(a) / 8 * kBlockSize;
        targetZ_ = groundHeight(targetX_, targetY_); // the place, not the ground under a hill
        distance_ = (ground ? 3.0f : 16.0f) * kBlockSize;
    }
    pitch_ = (ground ? 22.0f : 60.0f) * kPi / 180.0f;
    dirty_ = true;
}

void MapViewer::drawMinimap(Surface& s, int x0, int y0, int w, int h) const {
    if (map_.width <= 0 || map_.height <= 0)
        return;
    const int tilesW = map_.width * 2, tilesH = map_.height * 2;
    for (int y = 0; y < h; ++y) {
        if (y0 + y < 0 || y0 + y >= s.h)
            continue;
        // Row 0 of the grid at the bottom, as the map is drawn.
        const float fy = (float(h - 1 - y) + 0.5f) / float(h) * float(tilesH);
        uint8_t* row = s.row(y0 + y);
        for (int x = 0; x < w; ++x) {
            if (x0 + x < 0 || x0 + x >= s.w)
                continue;
            const float fx = (float(x) + 0.5f) / float(w) * float(tilesW);
            const int t = tileAt(int(fx), int(fy));
            unsigned code = 0;
            if (t >= 0 && size_t(t) < map_.tiles.size())
                code = terrainCode(map_.tiles[size_t(t)], std::clamp(int((fx - std::floor(fx)) * 8), 0, 7),
                                   std::clamp(int((fy - std::floor(fy)) * 8), 0, 7));
            uint8_t* p = row + (x0 + x) * 4;
            if (code && code < map_.palette.size())
                p[0] = map_.palette[code][0], p[1] = map_.palette[code][1], p[2] = map_.palette[code][2];
            else
                p[0] = p[1] = p[2] = 20;
        }
    }
    // The camera's target, a small cross.
    const int cx = x0 + int(targetX_ / (float(map_.width) * kBlockSize) * float(w));
    const int cy = y0 + h - 1 - int(targetY_ / (float(map_.height) * kBlockSize) * float(h));
    for (int d = -3; d <= 3; ++d)
        for (const auto& [px, py] : {std::pair{cx + d, cy}, std::pair{cx, cy + d}})
            if (px >= x0 && px < x0 + w && py >= y0 && py < y0 + h && px < s.w && py < s.h) {
                uint8_t* p = s.row(py) + px * 4;
                p[0] = 255, p[1] = 255, p[2] = 255;
            }
}

void MapViewer::cameraPosition(int& a, int& b) const {
    a = std::clamp(int(targetY_ / kBlockSize * 8), 0, std::max(0, map_.height * 8 - 1));
    b = std::clamp(int(targetX_ / kBlockSize * 8), 0, std::max(0, map_.width * 8 - 1));
}

float MapViewer::heading() const {
    float d = std::fmod(yaw_ * 180.0f / kPi, 360.0f);
    return d < 0 ? d + 360.0f : d;
}

std::string MapViewer::mapArchive() const {
    return maps_.empty() ? std::string() : archives_[maps_[size_t(current_)].archive]->path().filename().string();
}

void MapViewer::resetView() {
    targetX_ = float(map_.width) * kBlockSize / 2;
    targetY_ = float(map_.height) * kBlockSize / 2;
    targetZ_ = 0;
    yaw_ = 0;
    pitch_ = 50.0f * kPi / 180.0f;
    distance_ = std::max(1.0f, float(std::max(map_.width, map_.height)) * kBlockSize * 0.9f);
    dirty_ = true;
}

// A tile's ground model, decoded once per map, with its textures. Which
// textures the game gives ground tiles isn't known (see
// docs/formats/newworlds/gmf.md); chooseGroundTextures picks them by role.
const MapViewer::TileModel& MapViewer::tileModel(uint32_t index) {
    auto found = tileModels_.find(index);
    if (found != tileModels_.end())
        return found->second;
    TileModel& m = tileModels_[index];
    if (index >= archiveModels_.size() || archiveModels_[index] == kNoModel)
        m.error = "no model numbered " + std::to_string(index);
    else
        loadModel(archiveModels_[index], m, true);
    return m;
}

void MapViewer::loadModel(size_t entryIndex, TileModel& m, bool ground) {
    CatArchive& cat = *archives_[maps_[size_t(current_)].archive];
    const auto& entry = cat.entries()[entryIndex];
    m.name = entry.name;
    std::vector<uint8_t> data;
    if (Error err = cat.read(entry, data)) {
        m.error = err.message;
        return;
    }
    prepareModel(data, m, ground, false);
}

void MapViewer::prepareModel(const std::vector<uint8_t>& geoData, TileModel& m, bool ground, bool building) {
    std::vector<uint8_t> data;
    if (Error err = parseGeo(geoData, m.geo)) {
        m.error = err.message;
        return;
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
    std::string folder, stem, ext;
    splitName(m.name, folder, stem, ext);
    std::vector<const TextureFile*> slots;
    if (ground) {
        // Ground tiles: by the role of each slot's faces (ground_textures.h).
        std::vector<std::string> planet;
        for (const TextureFile& t : textureFiles_)
            if (t.folder == folder)
                planet.push_back(t.stem);
        for (const std::string& want : chooseGroundTextures(stem, m.geo, planet)) {
            const TextureFile* found = nullptr;
            for (const TextureFile& t : textureFiles_)
                if (t.folder == folder && t.stem == want)
                    found = &t;
            slots.push_back(found);
        }
    } else {
        slots = findModelTextures(m.name, textureFiles_, needed);
        if (building) {
            addDoorTextures(m.geo, m.name, textureFiles_, slots);
            addPadTextures(m.geo, m.name, textureFiles_, slots);
        }
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
}

// The ground's height at a point: the highest face of the tile there.
float MapViewer::groundHeight(float x, float y) {
    const int tx = int(std::floor(x / kTileSize)), ty = int(std::floor(y / kTileSize));
    const int t = tileAt(tx, ty);
    if (t < 0)
        return 0;
    const GmfMap::Tile& tile = map_.tiles[size_t(t)];
    const TileModel& m = tileModel(tile.model);
    if (!m.error.empty())
        return 0;
    // Into the model's own frame: undo the tile's turn.
    float lx = x - (float(tx) + 0.5f) * kTileSize, ly = y - (float(ty) + 0.5f) * kTileSize;
    for (int k = 0; k < tile.rotation; ++k) {
        const float t0 = lx;
        lx = ly;
        ly = -t0;
    }
    bool found = false;
    float best = 0;
    for (const auto& face : m.geo.faces) {
        const auto& verts = m.geo.parts[face.part].vertices;
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

// The ground under tiles drawn over something (hole_02's texture is
// transparent around the hole): the texture of the map's most common flat,
// low tile whose faces all use one texture (icewrld: ice_07). Where such a
// tile's own texture is transparent, this shows through at the same texture
// coordinates. The game's choice isn't known.
void MapViewer::makeUnderlay() {
    underlay_ = Texture{};
    std::map<uint32_t, size_t> uses; // tile model: grid squares using it
    for (const auto& cell : map_.cells)
        for (uint32_t t : map_.blocks[cell.block])
            ++uses[map_.tiles[t].model];
    size_t bestUses = 0;
    for (const auto& [model, n] : uses) {
        const TileModel& m = tileModel(model);
        if (!m.error.empty() || m.textures.empty() || m.textures[0].rgba.empty() || n <= bestUses)
            continue;
        bool fits = true;
        for (const auto& face : m.geo.faces)
            fits = fits && face.corners.back().extra[4] == 0;
        for (const auto& part : m.geo.parts)
            for (const auto& v : part.vertices)
                fits = fits && std::fabs(v.z) < 1000;
        if (fits) {
            underlay_ = m.textures[0];
            bestUses = n;
        }
    }
}

// The map's placed objects, each at the centre of its square on the block's
// 16 x 16 grid (bits 8-13 of its entry, perhaps a finer position, aren't
// used), on the ground. Which model a type is comes from icewrld, where the
// sizes fit: trees are planted two squares apart, and the rest are small.
void MapViewer::placeScenery() {
    placed_.clear();
    sceneryModels_.clear();
    static constexpr std::pair<int, const char*> kModels[] = {{0, "rock"}, {1, "tree"}, {2, "bush"}, {4, "frn_spec"}};
    CatArchive& cat = *archives_[maps_[size_t(current_)].archive];
    for (const auto& [type, name] : kModels)
        for (size_t entry : archiveModels_) {
            if (entry == kNoModel)
                continue;
            const std::string& full = cat.entries()[entry].name;
            std::string folder, stem, ext;
            splitName(full, folder, stem, ext);
            if (stem != name || full.find("/lod/") != std::string::npos)
                continue;
            loadModel(entry, sceneryModels_[type], false);
            break;
        }
    const float square = kBlockSize / 16;
    for (const auto& o : map_.scenery) {
        const auto found = sceneryModels_.find(o.type);
        if (found == sceneryModels_.end() || !found->second.error.empty())
            continue;
        const float x = (float(o.bx * 16 + o.x) + 0.5f) * square, y = (float(o.by * 16 + o.y) + 0.5f) * square;
        placed_.push_back({{x, y, groundHeight(x, y)}, float(o.turn) * kPi / 8, &found->second});
    }
}

void MapViewer::setPaved(const std::vector<std::pair<int, int>>& tiles) {
    paved_ = {tiles.begin(), tiles.end()};
    updateHidden();
    dirty_ = true;
}

// No trees or rocks under a building or on its colony's concrete.
void MapViewer::updateHidden() {
    hidden_.assign(placed_.size(), false);
    for (size_t i = 0; i < placed_.size(); ++i) {
        const Vec3& p = placed_[i].position;
        if (paved_.count({int(std::floor(p.x / kTileSize)), int(std::floor(p.y / kTileSize))}))
            hidden_[i] = true;
        for (const auto& [x, y, hw, hh] : buildingAreas_)
            if (std::fabs(p.x - x) < hw && std::fabs(p.y - y) < hh)
                hidden_[i] = true;
    }
}

// The planet's concrete texture (lbp/txr/map/<planet>/concrete; the ice
// world's is in glblgame) on every face: the recordings' colony pads.
const MapViewer::TileModel& MapViewer::pavedModel(uint32_t index) {
    auto [it, added] = pavedModels_.try_emplace(index);
    if (added) {
        it->second = tileModel(index);
        std::string folder, stem, ext;
        splitName(it->second.name, folder, stem, ext);
        Texture concrete;
        std::vector<uint8_t> data;
        for (const TextureFile& t : textureFiles_)
            if (t.folder == folder && t.stem == "concrete") {
                CatArchive& cat = *archives_[t.ref.archive];
                if (cat.read(cat.entries()[t.ref.entry], data) || decodeTexture(data, concrete))
                    concrete = Texture{};
                break;
            }
        if (!concrete.rgba.empty())
            for (Texture& t : it->second.textures)
                t = concrete;
    }
    return it->second;
}

const MapViewer::TileModel& MapViewer::namedModel(const std::string& name) {
    auto [it, added] = buildingModels_.try_emplace(name);
    TileModel& m = it->second;
    if (!added)
        return m;
    m.name = name;
    m.error = "not in the archives";
    for (const auto& a : archives_)
        if (const CatEntry* e = a->find(name)) {
            std::vector<uint8_t> data;
            m.error.clear();
            if (Error err = a->read(*e, data))
                m.error = err.message;
            else
                prepareModel(data, m, false, true);
            break;
        }
    return m;
}

// A model's extent across x and y, and the middle of it.
static void modelBounds(const GeoModel& geo, float& w, float& h, float& cx, float& cy) {
    float x0 = 0, x1 = 0, y0 = 0, y1 = 0;
    bool any = false;
    for (const auto& part : geo.parts)
        for (const auto& v : part.vertices) {
            if (!any)
                x0 = x1 = v.x, y0 = y1 = v.y, any = true;
            x0 = std::min(x0, v.x), x1 = std::max(x1, v.x), y0 = std::min(y0, v.y), y1 = std::max(y1, v.y);
        }
    w = x1 - x0, h = y1 - y0, cx = (x0 + x1) / 2, cy = (y0 + y1) / 2;
}

std::pair<float, float> MapViewer::modelSize(const std::string& name) {
    const TileModel& m = namedModel(name);
    float w = 0, h = 0, cx = 0, cy = 0;
    if (m.error.empty())
        modelBounds(m.geo, w, h, cx, cy);
    return {w, h};
}

void MapViewer::setBuildings(const std::vector<Building>& buildings) {
    buildings_.clear();
    buildingAreas_.clear();
    // Upgrades stand where the building they upgrade does, in its frame:
    // centred by the first model placed at that spot.
    std::map<std::pair<int, int>, std::pair<float, float>> centres;
    for (const Building& b : buildings) {
        const TileModel& m = namedModel(b.model);
        if (!m.error.empty())
            continue;
        float w = 0, h = 0, cx = 0, cy = 0;
        modelBounds(m.geo, w, h, cx, cy);
        const auto [c, first] = centres.try_emplace({b.a, b.b}, std::pair{cx, cy});
        cx = c->second.first, cy = c->second.second;
        // Row first, as lookAt; the model's middle there (some models' own
        // origin is at a corner: rch, rms), standing on the ground.
        const float angle = float(b.turn) * kPi / 2, cs = std::cos(angle), sn = std::sin(angle);
        const float x = float(b.b) / 8 * kBlockSize - (cx * cs - cy * sn);
        const float y = float(b.a) / 8 * kBlockSize - (cx * sn + cy * cs);
        const float ground = groundHeight(float(b.b) / 8 * kBlockSize, float(b.a) / 8 * kBlockSize);
        buildings_.push_back({{x, y, ground}, angle, &m});
        const bool across = b.turn % 2 != 0;
        buildingAreas_.push_back({float(b.b) / 8 * kBlockSize, float(b.a) / 8 * kBlockSize,
                                  (across ? h : w) / 2 + 512, (across ? w : h) / 2 + 512});
    }
    updateHidden();
    dirty_ = true;
}

float MapViewer::groundAt(int a, int b) {
    const float x = float(b) / 8 * kBlockSize, y = float(a) / 8 * kBlockSize;
    if (tileAt(int(std::floor(x / kTileSize)), int(std::floor(y / kTileSize))) < 0)
        return std::numeric_limits<float>::quiet_NaN();
    return groundHeight(x, y);
}

int MapViewer::depositAt(int bx, int by) const {
    if (bx < 0 || by < 0 || bx >= map_.width || by >= map_.height)
        return 0;
    return map_.cells[size_t(by) * size_t(map_.width) + size_t(bx)].deposit();
}

int MapViewer::tileAt(int tx, int ty) const { return newworlds::tileAt(map_, tx, ty); }

const MapViewer::TileEdges& MapViewer::tileEdges(uint32_t tileIndex) {
    auto found = tileEdges_.find(tileIndex);
    if (found != tileEdges_.end())
        return found->second;
    TileEdges& e = tileEdges_[tileIndex];
    const GmfMap::Tile& tile = map_.tiles[tileIndex];
    const TileModel& m = tileModel(tile.model);
    if (m.error.empty())
        e = tileSides(m.geo, tile.rotation);
    return e;
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
    case 'g': colouring_ = (colouring_ + 1) % kColourings; break;
    case 'l': outlines_ = !outlines_; break;
    case 'k': walls_ = !walls_; break;
    case 'o': scenery_ = !scenery_; break;
    case 'h': smooth_ = !smooth_; break;
    case key::Right: show((current_ + 1) % n); return;
    case key::Left: show((current_ + n - 1) % n); return;
    default: return;
    }
    dirty_ = true;
}

void MapViewer::update(uint64_t) {
    if (dirty_) {
        render();
        if (overlay_)
            overlay_();
        dirty_ = false;
        updateTitle();
    }
}

void MapViewer::updateTitle() {
    if (maps_.empty())
        return;
    const EntryRef& ref = maps_[size_t(current_)];
    std::string title = "New Worlds maps " + std::to_string(current_ + 1) + "/" + std::to_string(maps_.size()) +
                        ": " + archives_[ref.archive]->entries()[ref.entry].name;
    if (const Level* level = levelForMap(levels_, archives_[ref.archive]->entries()[ref.entry].name))
        title += " [" + level->shortName + " \"" + level->title + "\", " + std::to_string(level->maxPlayers) +
                 (level->maxPlayers == 1 ? " player" : " players") + (level->commentedOut ? ", not listed" : "") + "]";
    title += " (" + archives_[ref.archive]->path().filename().string();
    if (!status_.empty()) {
        title += "; " + status_ + ")";
        sys_.setWindowTitle(titleFilter_ ? titleFilter_(title) : title);
        return;
    }
    title += "; " + std::to_string(map_.width) + " x " + std::to_string(map_.height) + " blocks, " +
             std::to_string(map_.tiles.size()) + " tiles, " + std::to_string(map_.blocks.size()) + " blocks; drew " +
             std::to_string(drawnTiles_) + " tiles, " + std::to_string(drawnObjects_) + " objects, " +
             std::to_string(drawnFaces_) + " faces)";
    if (!walls_)
        title += " [no walls]";
    if (!scenery_)
        title += " [no objects]";
    if (!smooth_)
        title += " [flat shading]";
    if (!textured_)
        title += " [untextured]";
    else
        title += std::string(" [") + kColouringNames[colouring_] + "]";
    sys_.setWindowTitle(titleFilter_ ? titleFilter_(title) : title);
}

// The terrain code of a tile's cell (column cx from the west, row cy from the
// south, in map orientation): the topmost layer's that isn't 0, or 0.
unsigned MapViewer::terrainCode(const GmfMap::Tile& tile, int cx, int cy) {
    for (int layer = GmfMap::kLayers - 1; layer >= 0; --layer)
        if (const unsigned code = tile.layers[size_t(layer * 64 + cy * 8 + cx)])
            return code;
    return 0;
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
    drawnTiles_ = drawnFaces_ = drawnObjects_ = 0;
    if (!status_.empty())
        return;

    const OrbitCamera cam(Vec{targetX_, targetY_, targetZ_}, yaw_, pitch_, distance_, w, h);
    const Vec light = normalize(Vec{-0.4f, 0.5f, -0.75f}); // towards the light, in view space
    const float tileRadius = kTileSize * 0.75f;
    raster_.begin(screen_);
    std::vector<ViewVertex> poly;
    std::vector<ScreenVertex> projected;
    std::vector<std::vector<ScreenVertex>> outlines;
    // How lit a surface facing direction (x, y, z) is, both sides alike.
    auto lightLevel = [&](float x, float y, float z) {
        return 0.45f + 0.55f * std::fabs(dot(cam.directionToView(Vec{x, y, z}), light));
    };
    // Draw a model turned by an angle with cosine c and sine s (anticlockwise)
    // about origin; ground is the tile it is, for terrain codes, or null.
    auto drawModel = [&](const TileModel& m, const Vec& origin, float c, float s, const GmfMap::Tile* ground,
                         const uint8_t* tint = nullptr) {
        for (size_t fi = 0; fi < m.geo.faces.size(); ++fi) {
            const auto& face = m.geo.faces[fi];
            const auto& verts = m.geo.parts[face.part].vertices;
            const auto& normalLists = m.geo.parts[face.part].normalLists;
            const auto& fn = m.faceNormals[fi];
            poly.clear();
            for (const auto& corner : face.corners) {
                const auto& v = verts[corner.vertex];
                const float x = c * v.x - s * v.y, y = s * v.x + c * v.y;
                // For terrain codes, (u, v) is the position across the
                // tile, in map orientation.
                const bool planar = ground && colouring_ == 1;
                poly.push_back(cam.toView(Vec{origin.x + x, origin.y + y, origin.z + v.z},
                                          planar ? (x + kTileSize / 2) / kTileSize : corner.u,
                                          planar ? (y + kTileSize / 2) / kTileSize : corner.v));
                // Smooth shading: the corner's own normal (its first extra
                // value numbers the part's second normal list), else the face's.
                const bool own = smooth_ && normalLists.size() > 1 && corner.extra[0] < normalLists[1].size();
                const GeoModel::Vec3 n = own ? normalLists[1][corner.extra[0]] : fn;
                poly.back().s = lightLevel(c * n.x - s * n.y, s * n.x + c * n.y, n.z);
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
            const uint32_t textureIndex = face.corners.back().extra[4];
            const Texture* tex = textured_ && textureIndex < m.textures.size() &&
                                         !m.textures[textureIndex].rgba.empty()
                                     ? &m.textures[textureIndex]
                                     : nullptr;
            const bool codes = textured_ && ground && colouring_ == 1;
            auto shade = [&](float u, float v, float intensity, uint8_t* out) {
                static const uint8_t plain[3] = {170, 180, 195};
                const uint8_t* col = plain;
                if (codes) {
                    const unsigned code =
                        terrainCode(*ground, std::clamp(int(u * 8), 0, 7), std::clamp(int(v * 8), 0, 7));
                    if (code && code < map_.palette.size())
                        col = map_.palette[code].data();
                } else if (const Texture* tt = tex) {
                    int tx = int(std::floor(u * float(tt->width))) % tt->width;
                    int ty = int(std::floor(v * float(tt->height))) % tt->height;
                    tx += tx < 0 ? tt->width : 0;
                    ty += ty < 0 ? tt->height : 0;
                    const size_t texel = size_t(ty) * size_t(tt->width) + size_t(tx);
                    col = &tt->rgba[texel * 4];
                    // On the planets the alpha layer is transparency: the
                    // trees' and bushes' outlines, the ground round a hole.
                    // Other ground tiles are opaque (water02-04 have alpha
                    // layers, presumably for the game's own water effect).
                    const bool opaqueGround = ground && !ground->underlay;
                    const float a = col[3] == 0                         ? 0.0f
                                    : tt->alpha.empty() || opaqueGround ? 1.0f
                                                                        : float(tt->alpha[texel]) / 255;
                    if (a < 1 && ground && ground->underlay && !underlay_.rgba.empty()) {
                        // Ground shows through, at the same texture coordinates.
                        const Texture& g = underlay_;
                        const int gx = std::clamp(int(u * float(g.width)), 0, g.width - 1);
                        const int gy = std::clamp(int(v * float(g.height)), 0, g.height - 1);
                        const uint8_t* gc = &g.rgba[(size_t(gy) * size_t(g.width) + size_t(gx)) * 4];
                        for (int i = 0; i < 3; ++i)
                            out[i] = uint8_t(std::min(255.0f, (float(col[i]) * a + float(gc[i]) * (1 - a)) * intensity));
                        out[3] = 255;
                        return true;
                    }
                    if (a == 0)
                        return false;
                    if (a < 1) { // blend; only mostly opaque texels hide what's drawn later
                        for (int i = 0; i < 3; ++i)
                            out[i] = uint8_t(std::min(255.0f, float(col[i]) * intensity * a + float(out[i]) * (1 - a)));
                        return a >= 0.5f;
                    }
                }
                for (int i = 0; i < 3; ++i) {
                    const float base = tint ? (float(col[i]) + float(tint[i])) / 2 : float(col[i]);
                    out[i] = uint8_t(std::min(255.0f, base * intensity));
                }
                out[3] = 255;
                return true;
            };
            for (size_t i = 1; i + 1 < projected.size(); ++i)
                raster_.drawTriangle(projected[0], projected[i], projected[i + 1], shade);
            if (outlines_)
                outlines.push_back(projected);
        }
    };

    for (int by = 0; by < map_.height; ++by)
        for (int bx = 0; bx < map_.width; ++bx) {
            const auto& block = map_.blocks[map_.cells[size_t(by) * size_t(map_.width) + size_t(bx)].block];
            for (int slot = 0; slot < 4; ++slot) {
                const GmfMap::Tile& tile = map_.tiles[block[size_t(slot)]];
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
                const bool paved = colouring_ == 0 && !paved_.empty() &&
                                   paved_.count({bx * 2 + kSlotX[slot], by * 2 + kSlotY[slot]}) != 0;
                const TileModel& m = paved ? pavedModel(tile.model) : tileModel(tile.model);
                if (!m.error.empty())
                    continue;
                ++drawnTiles_;
                static constexpr float kCos[4] = {1, 0, -1, 0}, kSin[4] = {0, 1, 0, -1};
                const int deposit = map_.cells[size_t(by) * size_t(map_.width) + size_t(bx)].deposit();
                const uint8_t* tint =
                    textured_ && colouring_ == 2 && deposit >= 1 && deposit <= 6 ? kDepositColours[deposit - 1] : nullptr;
                drawModel(m, origin, kCos[tile.rotation], kSin[tile.rotation], &tile, tint);
            }
        }
    // Placed objects big enough on the screen to see.
    if (scenery_)
        for (size_t i = 0; i < placed_.size(); ++i) {
            if (i < hidden_.size() && hidden_[i])
                continue;
            const Placed& o = placed_[i];
            const Vec at{o.position.x, o.position.y, o.position.z};
            const ViewVertex c = cam.toView(at);
            if (c.z < cam.nearZ)
                continue;
            const float r = std::max(o.model->geo.radius, 256.0f) * cam.focal / c.z;
            const ScreenVertex sv = cam.project(c);
            if (r < 1.5f || sv.x < -r || sv.x > float(w) + r || sv.y < -r || sv.y > float(h) + r)
                continue;
            ++drawnObjects_;
            drawModel(*o.model, at, std::cos(o.angle), std::sin(o.angle), nullptr);
        }
    // The buildings, whatever their size on the screen.
    for (const Placed& o : buildings_) {
        const Vec at{o.position.x, o.position.y, o.position.z};
        if (cam.toView(at).z < cam.nearZ)
            continue;
        ++drawnObjects_;
        drawModel(*o.model, at, std::cos(o.angle), std::sin(o.angle), nullptr);
    }
    // Walls: where a tile's east or north edge and its neighbour's don't meet,
    // close the gap with flat, unlit quads. The game draws something there
    // (cliff walls in screenshots); how isn't known, so this is the viewer's
    // own approximation.
    if (walls_)
        for (int ty = 0; ty < 2 * map_.height; ++ty)
            for (int tx = 0; tx < 2 * map_.width; ++tx) {
                const int here = tileAt(tx, ty);
                for (int s = 0; s < 2; ++s) {
                    const int there = tileAt(tx + (s == 0), ty + (s == 1));
                    if (there < 0)
                        continue;
                    const TileEdges& a = tileEdges(uint32_t(here));
                    const TileEdges& b = tileEdges(uint32_t(there));
                    const int other = s == 0 ? 2 : 3; // the neighbour's west or south side
                    if (!a.present[s] || !b.present[other])
                        continue;
                    // The shared edge, in world coordinates.
                    const float ex = float(tx + 1) * kTileSize, ey = float(ty + 1) * kTileSize;
                    const float cx = (float(tx) + 0.5f) * kTileSize, cy = (float(ty) + 0.5f) * kTileSize;
                    const ViewVertex mid = cam.toView(Vec{s == 0 ? ex : cx, s == 0 ? cy : ey, 0});
                    if (mid.z < -tileRadius)
                        continue;
                    for (int k = 0; k < 4; ++k) {
                        const float z0a = a.z[s][k], z1a = a.z[s][k + 1];
                        const float z0b = b.z[other][k], z1b = b.z[other][k + 1];
                        if (std::fabs(z0a - z0b) < 80 && std::fabs(z1a - z1b) < 80)
                            continue;
                        const float t0 = -kTileSize / 2 + float(k) * kTileSize / 4, t1 = t0 + kTileSize / 4;
                        auto at = [&](float t, float z) {
                            return s == 0 ? Vec{ex, cy + t, z} : Vec{cx + t, ey, z};
                        };
                        poly.clear();
                        // (u, v): across the quarter edge, and height above the
                        // wall's lowest point.
                        const float zl = std::min({z0a, z1a, z0b, z1b});
                        poly.push_back(cam.toView(at(t0, z0a), 0, z0a - zl));
                        poly.push_back(cam.toView(at(t1, z1a), 1, z1a - zl));
                        poly.push_back(cam.toView(at(t1, z1b), 1, z1b - zl));
                        poly.push_back(cam.toView(at(t0, z0b), 0, z0b - zl));
                        poly = clipNear(poly, cam.nearZ);
                        if (poly.size() < 3)
                            continue;
                        projected.clear();
                        for (const auto& p : poly)
                            projected.push_back(cam.project(p));
                        // Textured with the cliff texture, projected sideways
                        // (or coloured like the higher tile's edge cells);
                        // east- and north-facing walls are shaded differently.
                        const bool aHigher = z0a + z1a > z0b + z1b;
                        const GmfMap::Tile& high = map_.tiles[size_t(aHigher ? here : there)];
                        const int edgeCell = aHigher ? 7 : 0;
                        const float shadeLevel = s == 0 ? 0.6f : 0.75f;
                        const float zLow = std::min({z0a, z1a, z0b, z1b});
                        auto wall = [&](float u, float v, uint8_t* out) {
                            static const uint8_t plain[3] = {150, 160, 175};
                            const uint8_t* col = plain;
                            const int along = std::clamp(k * 2 + int(u * 2), 0, 7);
                            const Texture* tt = textured_ && colouring_ == 0 && !wallTexture_.rgba.empty()
                                                    ? &wallTexture_
                                                    : nullptr;
                            if (textured_ && colouring_ == 1) {
                                const unsigned code =
                                    terrainCode(high, s == 0 ? edgeCell : along, s == 0 ? along : edgeCell);
                                if (code && code < map_.palette.size())
                                    col = map_.palette[code].data();
                            } else if (tt) {
                                // The image upright: its top row at the top of the wall.
                                const float tu = (float(k) + u) / 4, tv = -(zLow + v) / kTileSize;
                                int tx = int(std::floor(tu * float(tt->width))) % tt->width;
                                int ty = int(std::floor(tv * float(tt->height))) % tt->height;
                                tx += tx < 0 ? tt->width : 0;
                                ty += ty < 0 ? tt->height : 0;
                                col = &tt->rgba[(size_t(ty) * size_t(tt->width) + size_t(tx)) * 4];
                            }
                            for (int i = 0; i < 3; ++i)
                                out[i] = uint8_t(float(col[i]) * shadeLevel);
                            out[3] = 255;
                            return true;
                        };
                        for (size_t i = 1; i + 1 < projected.size(); ++i)
                            raster_.drawTriangle(projected[0], projected[i], projected[i + 1], wall);
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
