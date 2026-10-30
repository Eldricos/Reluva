// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/briefing_data.h"
#include "engines/newworlds/mission_ui.h"
#include "engines/newworlds/mission_surface.h"
#include "engines/newworlds/model_render.h"
#include "engines/newworlds/star_system.h"
#include "engines/newworlds/view3d.h"
#include "engines/newworlds/viewers.h"

#include "common/font.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace reluva::newworlds {

void MissionViewer::startBriefing() {
    if (script_.empty())
        return;
    savedColouring_ = map_.colouring();
    running_ = true;
    briefing_ = true;
    scene_ = -1; // its scenes start afresh
    startStep(0);
}

void MissionViewer::stopBriefing() {
    if (!running_)
        return;
    running_ = false;
    // In a mission's run, play follows (after its loading box).
    if (flow_ == Flow::Briefing) {
        flow_ = Flow::Starting;
        flowStartMs_ = nowMs_;
    }
    stopClip();
    if (!playing_)
        map_.setUnits({}); // (a scene's unit gone with it)
    scene_ = -1;
    map_.setColouring(savedColouring_);
    map_.resetCamera();
    redraw();
    updateTitle();
}

// A briefing scene's unit (briefing_data.h): the mission's briefing script
// flies it from its start along its waypoints, at its height, 100 a tick
// (the shuttles' flight speed). Its model by type: the races' shuttles
// (the Federation's 6, the Romulans' 93, the Klingons' 94 and 38: matched
// by race and speed), else the type's own.
void MissionViewer::flyScene(int index, uint64_t nowMs) {
    const BriefingScene* sc = briefingScene(index);
    if (!sc)
        return;
    auto at = [](int a, int b) { return std::pair{float(b) / 8 * MapViewer::kBlockUnits, float(a) / 8 * MapViewer::kBlockUnits}; };
    if (scene_ != index) {
        scene_ = index, sceneLeg_ = 0, sceneMs_ = nowMs;
        std::tie(sceneX_, sceneY_) = at(sc->a, sc->b);
    }
    double left = double(nowMs - sceneMs_) / 1000 * 2000; // units to fly
    sceneMs_ = nowMs;
    float heading = 0;
    while (left > 0 && sceneLeg_ < sc->ways) {
        const auto [wx, wy] = at(sc->way[sceneLeg_][0], sc->way[sceneLeg_][1]);
        const float dx = wx - sceneX_, dy = wy - sceneY_, d = std::hypot(dx, dy);
        if (d > 1)
            heading = std::atan2(dy, dx);
        if (d <= left) {
            sceneX_ = wx, sceneY_ = wy, left -= d, ++sceneLeg_;
        } else {
            sceneX_ += dx / d * float(left), sceneY_ += dy / d * float(left), left = 0;
        }
    }
    if (heading != 0)
        sceneHeading_ = heading;
    std::string model = sc->type == 6 ? "geo/fed/shuttle.geo"
                        : sc->type == 93 ? "geo/rom/rs.geo"
                        : sc->type == 94 || sc->type == 38 ? "geo/kln/kcs.geo"
                                                           : "";
    if (model.empty())
        if (const CombatType* t = combatType(sc->type))
            model = t->model;
    MapViewer::UnitDraw d{model, sceneX_, sceneY_, sceneHeading_};
    d.lift = sc->height;
    map_.setUnits({d});
    map_.centreOn(sceneX_, sceneY_);
    dirty_ = true;
}

void MissionViewer::startStep(int step) {
    stopClip();
    if (step >= int(script_.size())) {
        stopBriefing();
        return;
    }
    step_ = step;
    const BriefingStep& s = script_[size_t(step)];
    if (s.scene < 0 && !playing_)
        map_.setUnits({});
    // The relief and the Tricorder drawn as the step's scene says, the
    // ground's classes from the map's deposits (nobody's view yet).
    map_.setTricorder(side(),
                      [this](int bx, int by) {
                          const int kind = map_.depositAt(bx, by);
                          return kind >= 1 && kind <= 6 && map_.depositAmount(bx, by) > 0 ? 1 + kind : 0;
                      },
                      {});
    // (Each step sets the scheme, zoom and view of both Tricorders.)
    map_.setTricorderScheme(s.scheme);
    tricorderZoom_ = std::clamp(s.zoom, 0, 2);
    tricorderView_ = std::clamp(s.tricorderView, 0, 3);
    stepOnMap_ = hasMap_ && (s.view == BriefingView::Ground || s.view == BriefingView::Relief);
    if (stepOnMap_) {
        map_.setColouring(s.view == BriefingView::Relief ? 1 : 0);
        map_.lookAt(s.siteA, s.siteB, s.view == BriefingView::Ground);
        lastTurnMs_ = nowMs_;
    }
    // Its clip, if the game folder has it; it lasts the longer of the
    // clip and its minimum (5 s if it has neither).
    stepEndMs_ = nowMs_ + uint64_t(1000 * (s.minimum > 0 ? s.minimum : 5.0));
    stepMinimumMs_ = stepEndMs_;
    const auto it = std::find(clips_.begin(), clips_.end(), s.clip);
    if (!s.clip.empty() && it != clips_.end()) {
        playClip(int(it - clips_.begin()));
        if (clipHandle_ != audio::kNoSound)
            stepEndMs_ = 0;
    }
    redraw();
    updateTitle();
}

namespace {

// A texture from whichever archive has it; false if none does or it doesn't decode.
bool loadTexture(const Archives& archives, const std::string& name, Texture& out) {
    for (const auto& a : archives)
        if (const CatEntry* e = a->find(name)) {
            std::vector<uint8_t> data;
            return !a->read(*e, data) && !decodeTexture(data, out);
        }
    return false;
}

} // namespace

bool MissionViewer::loadScreen(const std::string& prefix, Texture& screen) const {
    screen = Texture{};
    screen.width = 640;
    screen.height = 480;
    screen.rgba.assign(size_t(640) * 480 * 4, 0);
    for (int t = 0; t < 6; ++t) {
        Texture tile;
        if (!loadTexture(archives_, prefix + std::to_string(t + 1) + ".lbp", tile))
            return false;
        const int x0 = (t % 3) * 256, y0 = (t / 3) * 256;
        for (int y = 0; y < tile.height && y0 + y < 480; ++y)
            for (int x = 0; x < tile.width && x0 + x < 640; ++x)
                std::copy_n(&tile.rgba[(size_t(y) * size_t(tile.width) + size_t(x)) * 4], 4,
                            &screen.rgba[(size_t(y0 + y) * 640 + size_t(x0 + x)) * 4]);
    }
    return true;
}

void MissionViewer::loadBriefingArt() {
    // The emblem screens are six tiles, three across and two down:
    // 256 + 256 + 128 by 256 + 224, the 640 x 480 screen.
    // The lost screens are cut the same way (f_lose01-06, ...).
    const std::pair<char, const char*> sets[] = {{'F', "lbp/gui/ufp00"}, {'K', "lbp/gui/kliemp0"}, {'R', "lbp/gui/romemp0"}};
    for (const auto& [side, prefix] : sets) {
        const char s = char(std::tolower(static_cast<unsigned char>(side)));
        Texture screen;
        if (loadScreen(prefix, screen))
            emblems_[side] = std::move(screen);
        if (loadScreen(std::string("lbp/gui/") + s + "_lose0", screen))
            losses_[side] = std::move(screen);
    }
    // The galaxy: a slab of star field round three flat layers (blue arm,
    // red ribbon, stars), numbering tabras01-04 in name order.
    for (const auto& a : archives_)
        if (const CatEntry* e = a->find("geo/solsys/tabras.geo")) {
            std::vector<uint8_t> data;
            if (a->read(*e, data) || parseGeo(data, galaxy_))
                galaxy_ = GeoModel{};
            break;
        }
    for (int t = 1; t <= 4 && !galaxy_.faces.empty(); ++t) {
        Texture tex;
        loadTexture(archives_, "lbp/txr/solsys/tabras0" + std::to_string(t) + ".mp0", tex);
        galaxyTextures_.push_back(std::move(tex));
    }
    // The star systems' models, by name (the galaxy isn't one).
    for (const auto& a : archives_)
        for (const CatEntry& e : a->entries()) {
            std::string folder, stem, ext;
            splitName(e.name, folder, stem, ext);
            if (folder == "solsys" && ext == "geo" && stem != "tabras" &&
                std::find(systemNames_.begin(), systemNames_.end(), stem) == systemNames_.end())
                systemNames_.push_back(stem);
        }
}

// The star system the mission's table names, with its pictures.
void MissionViewer::loadStarSystem() {
    system_ = GeoModel{};
    systemLayout_ = {};
    systemTextures_.clear();
    systemName_ = starSystemName(table_, systemNames_);
    if (systemName_.empty())
        return;
    for (const auto& a : archives_)
        if (const CatEntry* e = a->find("geo/solsys/" + systemName_ + ".geo")) {
            std::vector<uint8_t> data;
            if (a->read(*e, data) || parseGeo(data, system_))
                system_ = GeoModel{};
            break;
        }
    if (system_.faces.empty())
        return;
    systemLayout_ = readStarSystem(system_);
    if (systemLayout_.bodies.empty() && systemLayout_.backdropSlots.empty()) {
        system_ = GeoModel{}; // nothing in it to draw (a corrupt model)
        return;
    }
    for (const std::string& stem : starSystemTextures(systemLayout_, hasMap_ ? planetPicture(map_.mapArchive()) : "",
                                                          systemName_)) {
        Texture tex;
        if (!stem.empty())
            loadTexture(archives_, "lbp/txr/solsys/" + stem + ".mp0", tex);
        systemTextures_.push_back(std::move(tex));
    }
}

// The system from near the mission's planet, turning about it: the
// starfield adding its light behind (its alpha layer is its brightness,
// dimmed as in the recordings), the star and planets as stacks of round
// pictures, each turned to face the camera (so its slices stand one behind
// another, a ball from any side; seen edge-on they'd be a pile of discs).
bool MissionViewer::drawStarSystem() {
    if (system_.faces.empty())
        return false;
    fillScreen(screen_, Color{});
    const int body = missionBody(systemLayout_);
    GeoModel::Vec3 target{0, 0, 0};
    if (body >= 0)
        target = systemLayout_.bodies[size_t(body)].centre;
    const float yaw = float(nowMs_ % 120000) / 120000.0f * 6.2832f;
    const OrbitCamera cam({target.x, target.y, target.z}, yaw, 0.25f, 2200.0f, screen_.w, screen_.h);
    raster_.begin(screen_);
    for (int pass = 0; pass < 2; ++pass) // the starfield first
        for (size_t f = 0; f < system_.faces.size(); ++f) {
            const GeoModel::Face& face = system_.faces[f];
            const int slot = systemLayout_.faceSlot[f];
            const bool backdrop = std::find(systemLayout_.backdropSlots.begin(), systemLayout_.backdropSlots.end(),
                                            slot) != systemLayout_.backdropSlots.end();
            if (slot < 0 || size_t(slot) >= systemTextures_.size() || systemTextures_[size_t(slot)].rgba.empty() ||
                backdrop != (pass == 0) || face.corners.size() < 3 || face.part >= system_.parts.size())
                continue;
            const Texture& tex = systemTextures_[size_t(slot)];
            const auto& verts = system_.parts[face.part].vertices;
            const int faceBody = systemLayout_.faceBody[f];
            std::vector<ViewVertex> poly;
            for (const GeoModel::Corner& c : face.corners) {
                if (c.vertex >= verts.size())
                    break;
                const GeoModel::Vec3& p = verts[c.vertex];
                Vec at{p.x, p.y, p.z};
                if (faceBody >= 0) {
                    const GeoModel::Vec3& o = systemLayout_.bodies[size_t(faceBody)].centre;
                    const Vec d{p.x - o.x, p.y - o.y, p.z - o.z};
                    at = Vec{o.x, o.y, o.z} + cam.right * d.x + cam.up * d.y - cam.forward * d.z;
                }
                poly.push_back(cam.toView(at, c.u, c.v));
            }
            if (poly.size() != face.corners.size())
                continue;
            poly = clipNear(poly, cam.nearZ);
            if (poly.size() < 3)
                continue;
            std::vector<ScreenVertex> s;
            for (const ViewVertex& v : poly)
                s.push_back(cam.project(v));
            auto shade = [&](float u, float v, uint8_t* out) {
                const int tx = std::clamp(int(u * float(tex.width)), 0, tex.width - 1);
                const int ty = std::clamp(int(v * float(tex.height)), 0, tex.height - 1);
                const size_t i = size_t(ty) * size_t(tex.width) + size_t(tx);
                const int a = tex.alpha.empty() ? 255 : tex.alpha[i];
                if (backdrop) {
                    for (int k = 0; k < 3; ++k)
                        out[k] = uint8_t(std::min(255, out[k] + tex.rgba[i * 4 + size_t(k)] * a / 512));
                    return false;
                }
                if (a < 128)
                    return false; // outside the disc
                for (int k = 0; k < 3; ++k)
                    out[k] = tex.rgba[i * 4 + size_t(k)];
                return true;
            };
            for (size_t i = 1; i + 1 < s.size(); ++i)
                raster_.drawTriangle(s[0], s[i], s[i + 1], shade);
        }
    return true;
}

bool MissionViewer::drawEmblem(char side) {
    const auto it = emblems_.find(side);
    if (it == emblems_.end())
        return false;
    const Texture& e = it->second;
    for (int y = 0; y < screen_.h; ++y) {
        uint8_t* row = screen_.row(y);
        const int sy = y * e.height / screen_.h;
        for (int x = 0; x < screen_.w; ++x, row += 4) {
            const uint8_t* p = &e.rgba[(size_t(sy) * size_t(e.width) + size_t(x * e.width / screen_.w)) * 4];
            row[0] = p[0], row[1] = p[1], row[2] = p[2], row[3] = 255;
        }
    }
    return true;
}

bool MissionViewer::drawGalaxy() {
    if (galaxy_.faces.empty())
        return false;
    fillScreen(screen_, Color{});
    // Seen from above at an angle, turning; each layer adds its light (its
    // alpha layer is its brightness), so there's no depth to sort.
    const float yaw = float(nowMs_ % 120000) / 120000.0f * 6.2832f;
    const OrbitCamera cam({0, 0, 0}, yaw, 0.45f, 16000.0f, screen_.w, screen_.h);
    raster_.begin(screen_);
    for (const GeoModel::Face& face : galaxy_.faces) {
        if (face.corners.size() < 3 || face.part >= galaxy_.parts.size())
            continue;
        const uint32_t ti = face.corners.back().extra[4];
        if (ti >= galaxyTextures_.size() || galaxyTextures_[ti].rgba.empty())
            continue;
        const Texture& tex = galaxyTextures_[ti];
        const auto& verts = galaxy_.parts[face.part].vertices;
        std::vector<ViewVertex> poly;
        for (const GeoModel::Corner& c : face.corners) {
            if (c.vertex >= verts.size())
                break;
            const GeoModel::Vec3& p = verts[c.vertex];
            poly.push_back(cam.toView({p.x, p.y, p.z}, c.u, c.v));
        }
        if (poly.size() != face.corners.size())
            continue;
        poly = clipNear(poly, cam.nearZ);
        if (poly.size() < 3)
            continue;
        std::vector<ScreenVertex> s;
        for (const ViewVertex& v : poly)
            s.push_back(cam.project(v));
        auto shade = [&](float u, float v, uint8_t* out) {
            const int tx = std::clamp(int(u * float(tex.width)), 0, tex.width - 1);
            const int ty = std::clamp(int(v * float(tex.height)), 0, tex.height - 1);
            const size_t i = size_t(ty) * size_t(tex.width) + size_t(tx);
            const int a = tex.alpha.empty() ? 255 : tex.alpha[i];
            for (int k = 0; k < 3; ++k)
                out[k] = uint8_t(std::min(255, out[k] + tex.rgba[i * 4 + size_t(k)] * a / 255));
            return false;
        };
        for (size_t i = 1; i + 1 < s.size(); ++i)
            raster_.drawTriangle(s[0], s[i], s[i + 1], shade);
    }
    return true;
}

bool MissionViewer::drawModel(const std::string& name) {
    if (name.empty())
        return false;
    auto [it, added] = models_.try_emplace(name);
    BriefingModel& m = it->second;
    if (added)
        m.ok = loadTexturedModel(archives_, textureFiles_, name, m.model);
    if (!m.ok)
        return false;

    // The grid, as behind the recordings' model views: teal lines for the
    // Federation, dark red for the Klingons, green crosses for the Romulans.
    const char side = this->side();
    const uint8_t grid[3] = {uint8_t(side == 'F' ? 20 : side == 'R' ? 20 : 70),
                             uint8_t(side == 'F' ? 45 : side == 'R' ? 80 : 14),
                             uint8_t(side == 'F' ? 60 : side == 'R' ? 25 : 14)};
    const int w = screen_.w, h = screen_.h;
    for (int y = 0; y < h; ++y) {
        uint8_t* row = screen_.row(y);
        for (int x = 0; x < w; ++x, row += 4) {
            const bool line = side == 'R' ? (y % 24 == 0 && (x + 2) % 24 < 5) || (x % 24 == 0 && (y + 2) % 24 < 5)
                                          : (y % 24 == 0 && x % 4 == 0) || (x % 24 == 0 && y % 4 == 0);
            row[0] = line ? grid[0] : 0, row[1] = line ? grid[1] : 0, row[2] = line ? grid[2] : 0, row[3] = 255;
        }
    }
    // Turning once a minute, in the side's style.
    return drawModelView(screen_, raster_, m.model, float(nowMs_ % 60000) / 60000.0f * 6.2832f, 0.35f,
                         briefingModelStyle(side));
}

// The briefing's views without a map: the game's galaxy, star system,
// models and emblem where the archives have them; else Reluva's own
// stand-ins.
void MissionViewer::drawBackdrop(BriefingView view) {
    const char side = this->side();
    if ((view == BriefingView::Galaxy && drawGalaxy()) || (view == BriefingView::Emblem && drawEmblem(side)) ||
        (view == BriefingView::StarSystem && drawStarSystem()) ||
        (view == BriefingView::Model && drawModel(script_[size_t(step_)].model)))
        return;
    const int w = screen_.w, h = screen_.h;
    fillScreen(screen_, Color{});
    uint32_t seed = 12345;
    auto rnd = [&] { return (seed = seed * 1664525u + 1013904223u) >> 8; };
    switch (view) {
    case BriefingView::Galaxy:
        for (int i = 0; i < 900; ++i) {
            // A flattened disc of stars with a red and a blue arm.
            const float t = float(rnd() % 1000) / 1000.0f * 6.283f, rr = float(rnd() % 1000) / 1000.0f;
            const int x = w / 2 + int(std::cos(t) * rr * float(w) * 0.32f);
            const int y = h / 2 + int(std::sin(t) * rr * float(h) * 0.16f);
            const uint8_t v = uint8_t(120 + rnd() % 136);
            plot(screen_, x, y, v, v, v);
        }
        for (int i = 0; i < 300; ++i) {
            const int x = int(rnd() % unsigned(w / 3)), d = int(rnd() % 9) - 4;
            plot(screen_, w / 3 + x, h / 2 - 30 + x / 4 + d, 200, 40, 40);
            plot(screen_, w / 3 + x, h / 2 + 20 - x / 5 + d, 60, 80, 220);
        }
        break;
    case BriefingView::StarSystem:
        for (int i = 0; i < 400; ++i) {
            // Drawn into locals so every compiler gets the same stars
            // (the order of a call's arguments is unspecified): y first,
            // as GCC drew them.
            const int y = int(rnd() % unsigned(h)), x = int(rnd() % unsigned(w));
            plot(screen_, x, y, 90, 90, 110);
        }
        for (int i = 0; i < 7; ++i) {
            const int cx = int(rnd() % unsigned(w)), cy = int(rnd() % unsigned(h)), r = 2 + int(rnd() % 5);
            const uint8_t cr = uint8_t(rnd() % 256), cg = uint8_t(rnd() % 256), cb = uint8_t(rnd() % 256);
            for (int y = -r; y <= r; ++y)
                for (int x = -r; x <= r; ++x)
                    if (x * x + y * y <= r * r)
                        plot(screen_, cx + x, cy + y, cr, cg, cb);
        }
        break;
    case BriefingView::Model:
    case BriefingView::Ground:
    case BriefingView::Relief:
        // A dim red grid, as the model views have (and the map views when
        // there's no map).
        for (int y = 0; y < h; y += 24)
            for (int x = 0; x < w; x += 4)
                plot(screen_, x, y, 70, 14, 14);
        for (int x = 0; x < w; x += 24)
            for (int y = 0; y < h; y += 4)
                plot(screen_, x, y, 70, 14, 14);
        drawText(kPanelMargin, kPanelMargin, view == BriefingView::Model ? "(a model turning)" : mapNote_, kNoteColor);
        break;
    case BriefingView::Emblem: {
        const char* name = side == 'K' ? "KLINGON EMPIRE" : side == 'R' ? "ROMULAN STAR EMPIRE" : "UNITED FEDERATION OF PLANETS";
        const Color c = side == 'K' ? Color{200, 60, 40} : side == 'R' ? Color{60, 170, 90} : Color{90, 140, 230};
        const int scale = 3, tw = int(font::length(name)) * font::kAdvance * scale;
        font::drawText(screen_, (w - tw) / 2, h / 3, name, c, scale);
        break;
    }
    }
}

} // namespace reluva::newworlds
