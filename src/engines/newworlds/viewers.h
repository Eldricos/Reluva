// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/events.h"
#include "common/system.h"
#include "common/types.h"
#include "engines/newworlds/cat_archive.h"
#include "engines/newworlds/geo.h"
#include "engines/newworlds/rasterizer.h"
#include "engines/newworlds/texture.h"

#include <memory>
#include <string>
#include <vector>

// Data viewers used by the New Worlds engine until the game itself runs.
// Each draws into the engine's 640x480 RGBA32 screen and reports its state in
// the window title.

namespace reluva::newworlds {

using Archives = std::vector<std::unique_ptr<CatArchive>>;

// Where an item lives: archive index and entry index.
struct EntryRef {
    size_t archive, entry;
};

// Browses every texture.
//   Left/Right: previous/next   PageUp/PageDown: jump 10   Home/End: first/last
//   F: filter (all, failed, colour-keyed, with alpha)
class TextureViewer {
public:
    TextureViewer(System& sys, Surface& screen, const Archives& archives);
    size_t count() const { return textures_.size(); }
    void activate(); // redraw and retitle after switching to this viewer
    void handleKey(uint32_t key);

private:
    struct TextureRef {
        EntryRef ref;
        bool ok = false, colourKeyed = false, alpha = false;
    };

    void classify();
    void applyFilter(int filter);
    void show(int index);
    void fill(int x0, int y0, int w, int h, uint8_t r, uint8_t g, uint8_t b);
    void drawCheckerboard();
    void drawTexture(const Texture& tex);

    System& sys_;
    Surface& screen_;
    const Archives& archives_;
    std::vector<TextureRef> textures_;
    std::vector<size_t> visible_; // indices into textures_ that pass the filter
    bool classified_ = false;
    int filter_ = 0;
    int current_ = 0;
};

// Shows every model with its textures. A model's textures are the ones named
// after it (k_bop: k_bop_a, k_bop_b, k_bop_g), in name order; each face picks
// one with the last value of its last corner. See docs/formats/newworlds/geo.md.
//   T: textures on/off
//   Left/Right: previous/next   PageUp/PageDown: jump 10   Home/End: first/last
//   Mouse drag or W/A/S/D: rotate   wheel or +/-: zoom   R: reset the view
//   Space: spin   P: show one part at a time   U: texture-coordinate checks
//   C: colour by texture number instead of texturing
//   L: outlines (solid, solid + outlines, outlines only)
//   B: back faces (hidden, shown, only back faces)   F: filter (all, multi-part, failed)
class ModelViewer {
public:
    ModelViewer(System& sys, Surface& screen, const Archives& archives);
    size_t count() const { return models_.size(); }
    void activate();
    void handleEvent(const Event& ev);
    void update(uint64_t nowMs); // redraws when something changed

private:
    void classify();
    void applyFilter(int filter);
    void show(int index);
    void resetView();
    void render();
    void updateTitle();
    void drawLine(float x0, float y0, float x1, float y1);
    void loadTextures(const std::string& modelName);

    struct TextureFile {
        std::string stem, folder; // "k_bop_a", "kln" for lbp/txr/kln/k_bop_a.mp0
        EntryRef ref;
    };

    System& sys_;
    Surface& screen_;
    const Archives& archives_;
    Rasterizer raster_;
    std::vector<EntryRef> models_;
    std::vector<uint32_t> partCounts_; // per model; 0 when it doesn't decode
    std::vector<size_t> visible_;
    std::vector<TextureFile> textureFiles_; // every full-size texture
    std::vector<Texture> modelTextures_;    // the current model's, in face order
    std::vector<std::string> modelTextureNames_;
    bool classified_ = false;
    int filter_ = 0;
    int current_ = 0;

    GeoModel model_;
    std::string status_; // decode error, if any
    float centre_[3] = {0, 0, 0};
    float size_ = 1; // distance from the centre to the farthest vertex

    float yaw_ = 0, pitch_ = 0, zoom_ = 1;
    int soloPart_ = -1; // -1: all parts
    bool spin_ = false, uvCheck_ = false, colourByTail_ = false, textured_ = true;
    int outlines_ = 0, faces_ = 0; // faces_: 0 front only, 1 both sides, 2 back only
    bool dragging_ = false;
    int dragX_ = 0, dragY_ = 0;
    bool dirty_ = true;
    uint64_t lastMs_ = 0;
};

} // namespace reluva::newworlds
