// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/events.h"
#include "common/system.h"
#include "common/types.h"
#include "engines/newworlds/cat_archive.h"
#include "engines/newworlds/geo.h"
#include "engines/newworlds/gmf.h"
#include "engines/newworlds/mdl.h"
#include "engines/newworlds/model_textures.h"
#include "engines/newworlds/rasterizer.h"
#include "engines/newworlds/texture.h"
#include "engines/newworlds/wav.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Data viewers used by the New Worlds engine until the game itself runs.
// Each draws into the engine's 640x480 RGBA32 screen and reports its state in
// the window title.

namespace reluva::newworlds {

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
//   T: textures on/off   G: alpha layers (as glow, ignored, as transparency)
//   Left/Right: previous/next   PageUp/PageDown: jump 10   Home/End: first/last
//   Mouse drag or W/A/S/D: rotate   wheel or +/-: zoom   R: reset the view
//   Space: spin   P: show one part at a time   U: texture-coordinate checks
//   C: colour by texture number instead of texturing   H: smooth or flat shading
//   M: the model's points (from its .mdl) on/off   N: select the next point
//   K: play the next animation that fits the model (.arf frames), then none
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
    void drawLine(float x0, float y0, float x1, float y1, const uint8_t* colour = nullptr);
    void loadTextures(const std::string& modelName);
    void loadMetamodel(const std::string& modelName);
    void loadAnimations(const std::string& stem);
    int channelOfNode(uint32_t node) const;

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
    std::map<std::string, EntryRef> metamodels_; // .mdl files by "folder/stem"
    MdlModel mdl_;
    bool hasMdl_ = false, markers_ = true;
    int selectedPoint_ = -1; // -1: none
    struct Animation {
        std::string name;                              // conbee, fcmwalk
        std::vector<std::vector<PartTransform>> frames; // per frame, per animated part
    };
    std::vector<Animation> animations_;
    int animation_ = -1, animFrame_ = 0; // -1: the stored pose
    uint64_t animMs_ = 0;
    static constexpr uint64_t kFrameMs = 100;
    float centre_[3] = {0, 0, 0};
    float size_ = 1; // distance from the centre to the farthest vertex

    float yaw_ = 0, pitch_ = 0, zoom_ = 1;
    int soloPart_ = -1; // -1: all parts
    bool spin_ = false, uvCheck_ = false, colourByTail_ = false, textured_ = true, smooth_ = true;
    int outlines_ = 0, faces_ = 0; // faces_: 0 front only, 1 both sides, 2 back only
    int alphaMode_ = 0;            // see kAlphaNames
    bool dragging_ = false;
    int dragX_ = 0, dragY_ = 0;
    bool dirty_ = true;
    uint64_t lastMs_ = 0;
};

// Shows the planet maps (.gmf), built from their ground tiles: a map is a
// grid of blocks, a block 2 x 2 tiles, and a tile one of the planet's ground
// models. See docs/formats/newworlds/gmf.md.
//   Left/Right: previous/next map   Mouse drag: rotate   wheel or +/-: zoom
//   W/A/S/D: move   Q/E: turn   R: reset the view   L: outlines   K: walls
//   O: placed objects (trees, rocks) on/off   H: smooth or flat shading
//   T: textures on/off   G: colouring (the models' own textures; the map's
//   terrain codes in its colour table; textures tinted by resource deposit)
class MapViewer {
public:
    MapViewer(System& sys, Surface& screen, const Archives& archives);
    size_t count() const { return maps_.size(); }
    void activate();
    void handleEvent(const Event& ev);
    void update(uint64_t nowMs); // redraws when something changed

private:
    using Vec3 = GeoModel::Vec3;
    struct TileModel {
        std::string name;              // e.g. "geo/map/ice/ice_07.geo"
        std::string error;             // when it doesn't decode
        GeoModel geo;
        std::vector<Vec3> faceNormals; // per face, unit length
        std::vector<Texture> textures; // in the order the faces number them
        std::vector<std::string> textureNames;
    };

    // A tile's edge heights after its rotation, at 5 points along each side
    // (east, north, west, south); a side without vertices on it is missing.
    struct TileEdges {
        bool present[4] = {};
        float z[4][5] = {};
    };

    // A placed object, ready to draw.
    struct Placed {
        Vec3 position;
        float angle; // anticlockwise, in radians
        const TileModel* model;
    };

    void show(int index);
    void resetView();
    void render();
    void updateTitle();
    // An entry of the map's archive; ground tiles get textures by role, other
    // models those named after them.
    void loadModel(size_t entry, TileModel& m, bool ground);
    const TileModel& tileModel(uint32_t index);
    float groundHeight(float x, float y);
    void placeScenery();
    void makeUnderlay();
    const TileEdges& tileEdges(uint32_t tile);
    static TileEdges edgesOf(const GeoModel& geo, int rotation);
    int tileAt(int tx, int ty) const; // tile record at a tile position, or -1
    static unsigned terrainCode(const GmfMap::Tile& tile, int cx, int cy);

    System& sys_;
    Surface& screen_;
    const Archives& archives_;
    Rasterizer raster_;
    std::vector<EntryRef> maps_;
    std::vector<TextureFile> textureFiles_;
    bool shown_ = false;
    int current_ = 0;

    GmfMap map_;
    std::string status_;                 // decode error, if any
    std::vector<size_t> archiveModels_;  // entry indices of the map archive's .geo files, by model number
    std::map<uint32_t, TileModel> tileModels_;
    std::map<uint32_t, TileEdges> tileEdges_;
    Texture wallTexture_; // the map archive's last cliff texture, if any
    std::map<int, TileModel> sceneryModels_; // by object type
    std::vector<Placed> placed_;
    Texture underlay_; // ground showing through tiles with GmfMap::Tile::underlay

    float targetX_ = 0, targetY_ = 0, yaw_ = 0, pitch_ = 0, distance_ = 1;
    bool textured_ = true, outlines_ = false, walls_ = true, scenery_ = true, smooth_ = true;
    int colouring_ = 0; // see kColouringNames
    bool dragging_ = false;
    int dragX_ = 0, dragY_ = 0;
    bool dirty_ = true;
    unsigned drawnTiles_ = 0, drawnFaces_ = 0, drawnObjects_ = 0;
};

// Plays the sounds (.wav) and shows their waveforms.
//   Left/Right: previous/next   PageUp/PageDown: jump 10   Home/End: first/last
//   Space or Enter: play/stop   A: play each sound on selecting it
class SoundViewer {
public:
    SoundViewer(System& sys, Surface& screen, const Archives& archives);
    ~SoundViewer();
    size_t count() const { return sounds_.size(); }
    void activate();
    void deactivate(); // stops playing when switching to another viewer
    void handleKey(uint32_t key);
    void update(uint64_t nowMs);

private:
    void show(int index);
    void play();
    void stop();
    bool playing() const;
    void draw();
    void updateTitle();
    const std::string& nameOf(size_t sound) const;

    System& sys_;
    Surface& screen_;
    const Archives& archives_;
    std::vector<EntryRef> sounds_;
    // The archives store some sounds many times over (kliactor has kneg1.wav
    // 24 times, in 4 slots of each vehicle's set), sometimes under other
    // names: each sound's first copy, and the sounds listed (all, or only
    // the first of each set of copies).
    std::vector<size_t> firstCopy_;
    std::vector<size_t> shownSounds_;
    bool showCopies_ = false;
    bool shown_ = false, autoPlay_ = false, audioStarted_ = false, wasPlaying_ = false;
    int current_ = 0;
    std::string status_;
    // Shared with the audio thread.
    mutable std::mutex mutex_;
    std::shared_ptr<const Sound> sound_;
    double position_ = 0; // in the sound's frames
    bool playing_ = false;
};

} // namespace reluva::newworlds
