// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/savestate.h"
#include "common/events.h"
#include "common/system.h"
#include "common/audio/mixer.h"
#include "common/audio/sound.h"
#include "common/types.h"
#include "engines/newworlds/briefing.h"
#include "engines/newworlds/cat_archive.h"
#include "engines/newworlds/fif.h"
#include "engines/newworlds/game_ui.h"
#include "engines/newworlds/colony.h"
#include "engines/newworlds/economy.h"
#include "engines/newworlds/model_pose.h"
#include "engines/newworlds/model_render.h"
#include "engines/newworlds/star_system.h"
#include "engines/newworlds/world.h"
#include "engines/newworlds/ai_sides.h"
#include "engines/newworlds/field_work.h"
#include "engines/newworlds/research.h"
#include "engines/newworlds/script_effects.h"
#include "engines/newworlds/fog.h"
#include "engines/newworlds/repairs.h"
#include "engines/newworlds/geo.h"
#include "engines/newworlds/gmf.h"
#include "engines/newworlds/levels.h"
#include "engines/newworlds/mdl.h"
#include "engines/newworlds/mission_script.h"
#include "engines/newworlds/mission_sim.h"
#include "engines/newworlds/model_textures.h"
#include "engines/newworlds/rasterizer.h"
#include "engines/newworlds/string_table.h"
#include "engines/newworlds/texture.h"

#include <filesystem>
#include <functional>
#include <array>
#include <map>
#include <random>
#include <set>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
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
    Metamodels metamodels_;
    MdlModel mdl_;
    bool hasMdl_ = false, markers_ = true, turret_ = false;
    int selectedPoint_ = -1; // -1: none
    std::vector<Animation> animations_;
    int animation_ = -1, animFrame_ = 0; // -1: the stored pose; animations_.size(): the turret sweep
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
    // level.dat's missions, to name each map's mission in the title.
    void setLevels(std::vector<Level> levels) { levels_ = std::move(levels); }
    size_t count() const { return maps_.size(); }
    void activate();
    void handleEvent(const Event& ev);
    void update(uint64_t nowMs); // redraws when something changed

    // For the missions page, which shows its maps through this viewer:
    // shows the map with that name ("map/f1.gmf", in any archive, ignoring
    // case); false if there's none.
    bool showMap(std::string_view name);
    // The archive the shown map is in ("icewrld.cat").
    std::string mapArchive() const;
    // Called after each redraw, to draw over the map.
    void setOverlay(std::function<void()> overlay) { overlay_ = std::move(overlay); }
    // Given the title this viewer would set, returns the one to set instead.
    void setTitleFilter(std::function<std::string(const std::string&)> filter) { titleFilter_ = std::move(filter); }
    void redraw() { dirty_ = true; }
    // Only what the overlay draws changed: the last scene again under it,
    // not drawn anew (the scene's own changes mark it themselves).
    void redrawOverlay() { overlayDirty_ = true; }
    // While held, the scene is drawn only when the camera moves, and at
    // most every kHeldMs when what's on it changed: else the last one
    // stays under the overlay (the full-screen Tricorder dims it); let
    // go, it's drawn as it now is.
    void holdScene(bool hold) {
        if (hold != sceneHeld_)
            sceneHeld_ = hold, dirty_ = true;
    }
    // The window's title, set only when it changes; forgetTitle() after
    // something else (another page) has set it.
    void setTitle(const std::string& title);
    void forgetTitle() { shownTitle_.clear(); }
    // How the ground is coloured (0 textures, 1 terrain codes, 2 deposits).
    int colouring() const { return colouring_; }
    // Points the camera at a map position as the game gives them (eighths
    // of a block, row first; see docs/formats/newworlds/gmf.md), close and
    // low for a ground view or high above for an overview; a < 0 for the
    // whole map. turn() turns it about its target.
    void lookAt(int a, int b, bool ground);
    void turn(float radians) {
        yaw_ += radians;
        dirty_ = true;
    }
    void resetCamera() { resetView(); }
    // The Tricorder's relief as the game draws it (exe.md#the-tricorder):
    // the map's five terrain-code layers as stacked sheets, each drawn
    // where the ground reaches it, seen by the Tricorder's own small
    // camera centred on the main camera's target. Each point's colour:
    // its block's class (0 surveyed ground, 1 unsurveyed, 2-7 ores 1-6)
    // gives a hue, saturation and brightness shift, for the player's
    // race, of the map's own colour for the terrain code there; each block
    // drawn from its class's tile: a hatch (a ring; solid for unsurveyed
    // blocks and the border), cut at its edges by which of the blocks
    // round each corner share its class (a block out of view counts as
    // sharing it).
    // Where it's drawn and its camera (tricorderRelief()).
    struct Relief {
        int x = 0, y = 0, w = 0, h = 0; // on the screen
        float cx = 0, cy = 0;           // the point it looks at (world units)
        float yaw = 0, pitch = 0;       // radians: the heading (0 north up), the tilt above the ground
        float distance = 1;             // the camera's distance from that point
        float step = 0;                 // the layers' spacing, in world units
    };
    // The view (0 Starship, 1 Relief, 2 Relief orbit, 3 Relief fixed) at
    // a zoom (tiles across the width); orbit the Relief orbit view's
    // heading (radians).
    Relief tricorderRelief(int x, int y, int w, int h, int tiles, int view, float orbit) const;
    void drawTricorderRelief(Surface& s, const Relief& relief, char race,
                             const std::function<int(int bx, int by)>& cellClass,
                             const std::function<bool(int bx, int by)>& inView);
    // A world point's pixel on the relief (standing on the ground there;
    // false if it's outside it), and the ground under a pixel (false if
    // there's no map there).
    bool reliefPoint(const Relief& relief, float wx, float wy, int& px, int& py) const;
    bool reliefToWorld(const Relief& relief, int px, int py, float& wx, float& wy) const;
    // The last scene drawn (the screen's pixels under the overlay; empty
    // before the first).
    const std::vector<uint8_t>& lastScene() const { return scene_; }
    // The class and view sources the Tricorder's colours use (the relief
    // view too, once set), and its scheme (0 the ring hatch, 1 the dot, 2
    // grid lines: a briefing step's).
    void setTricorder(char race, std::function<int(int bx, int by)> cellClass, std::function<bool(int bx, int by)> inView) {
        tricorderRace_ = race, tricorderClass_ = std::move(cellClass), tricorderInView_ = std::move(inView);
    }
    void setTricorderScheme(int scheme) {
        if (tricorderScheme_ != scheme)
            tricorderScheme_ = scheme, dirty_ = true;
    }
    // A point's Tricorder colour, at (fx, fy) in tiles; false where there's
    // no ground (the background). layer: the terrain code of that layer
    // (0-4), else the highest there.
    bool tricorderColour(float fx, float fy, uint8_t* out, int layer = -1) const;
    // Moves the camera along the ground, by world units right and forward.
    void pan(float right, float forward);
    void cameraTarget(float& x, float& y) const { x = targetX_, y = targetY_; }
    float yaw() const { return yaw_; } // radians; the view's right along the ground is (cos, sin)
    void cameraPosition(int& a, int& b) const;
    float heading() const;
    void setColouring(int colouring) {
        colouring_ = colouring;
        dirty_ = true;
    }
    // Buildings to stand on the shown map (the missions page's starting
    // colony): a model by name ("geo/fed/fch.geo", in any archive) at a map
    // position, turned anticlockwise by quarter turns. Cleared by showMap.
    struct Building {
        std::string model;
        int a = 0, b = 0;
        int turn = 0;
        float progress = 1; // under construction: 0 just begun, 1 built; below 0 not drawn (a site cancelled)
        std::string builder; // the construction bee working on it ("geo/fed/fconbee.geo"), or none
        int builders = 2;    // how many of them are at work (0-4)
    };
    void setBuildings(const std::vector<Building>& buildings);
    // A building's construction moved on (by its index in setBuildings').
    // The recordings show an apron first, then the building rising out of
    // it (play-tutorials.md): here its model flat, then growing upwards.
    // builders: how many bees are at work on it now (-1: as they were).
    void setBuildingProgress(size_t i, float progress, int builders = -1);
    // The hologram of a building being placed (play): its model, tinted
    // green where it can go and red where it can't; model empty for none.
    void setGhost(const std::string& model, int a, int b, bool valid);
    // Tiles (x east, y north) drawn with the planet's concrete texture
    // instead of their own, as under a starting colony. Cleared by showMap.
    void setPaved(const std::vector<std::pair<int, int>>& tiles);
    // The shown map's size in blocks, a block's resource deposit (0 for
    // none), and the ground's height at a map position (NaN off the map
    // or where no tile is).
    int blocksWide() const { return map_.width; }
    int blocksHigh() const { return map_.height; }
    int depositAt(int bx, int by) const;
    // The cell value's low 12 bits at a block (50-300; how much a deposit
    // holds, presumably; gmf.md).
    int depositAmount(int bx, int by) const;
    float groundAt(int a, int b);
    // A building model's size across x and y (0 if it isn't there).
    std::pair<float, float> modelSize(const std::string& name);

    // For play (the missions page): units to draw this frame, by model, at
    // world positions (x east, y north), standing on the ground; the
    // selected ones with the recordings' row of orange marks above them.
    struct UnitDraw {
        std::string model;
        float x = 0, y = 0, heading = 0; // heading: radians anticlockwise from east
        bool selected = false;
        // Its turret turned from straight ahead (anticlockwise) and its gun
        // raised, in radians, for a model with one.
        float turretYaw = 0, gunPitch = 0;
        // A cloaking unit's shimmer (exe.md#cloaking): each corner of its
        // model is thrown up to this far (world units) each frame (a new
        // throw each game tick).
        float shimmer = 0;
        float health = 1; // hit points left, a share (its selection marks)
        float lift = 0;    // above the ground (a bee in flight)
        bool operator==(const UnitDraw&) const = default;
    };
    // Redraws only if they changed.
    void setUnits(std::vector<UnitDraw> units) {
        if (units != units_)
            units_ = std::move(units), dirty_ = sceneChanged_ = true;
    }
    // The ground's height at a world point (0 off the map); the map's size
    // in world units.
    float groundAtWorld(float x, float y) { return groundHeight(x, y); }
    float worldWidth() const { return float(map_.width) * kBlockUnits; }
    // The ground for units: each tile's movement class (its flags' bits
    // 16-17) and the ground's height (gmf.md); empty with no map shown.
    Terrain terrain();
    float worldHeight() const { return float(map_.height) * kBlockUnits; }
    // The world point under a screen pixel (false if the ray misses the
    // map), and a world point's pixel (false if behind the camera).
    bool screenToGround(int sx, int sy, float& x, float& y);
    bool worldToScreen(float x, float y, float z, int& sx, int& sy) const;

    // The game's camera (play-tutorials.md): the bird's-eye view, F2's sky
    // view, F1's first-person view; arrows move, numpad 4/6 turn, 8/2 zoom
    // towards the pointer, 7/1 raise and lower, 9/3 tilt. A key it doesn't
    // use returns false. mouseX/Y: where the pointer is, for the zoom.
    // The game's cameras (exe.md#cameras): F1 low over the ground, F2
    // isometric, F3 close-up isometric.
    enum class View { BirdsEye, Close, FirstPerson };
    bool gameKey(uint32_t key, int mouseX, int mouseY);
    void setView(View v);
    View view() const { return view_; }
    // The camera, for a savestate: its mode, where it looks from and Z's
    // mark (a lock or a followed object let go).
    void syncCamera(save::Serializer& s) {
        s(view_)(markX_)(markY_)(marked_)(targetX_)(targetY_)(targetZ_)(yaw_)(pitch_)(distance_);
        if (s.loading())
            locked_ = following_ = false, dirty_ = true;
    }
    // Ctrl+click: zoom in on the pointed spot, or out.
    void zoomAt(int sx, int sy, bool in);
    // Centres the view on a world point, keeping the view's height.
    void centreOn(float x, float y);
    // Double-click on a building: the camera closes in and orbits it (4/6
    // turn about it) until unlocked.
    void lockOn(float x, float y);
    void unlock() { locked_ = false; }
    bool locked() const { return locked_; }
    // F1's camera following an object (exe.md#cameras): behind it, turning
    // with it, at 1000 (Up and Down: 800 to 2000); numpad 4/6 orbit it, 7/9
    // raise and 1/3 lower the camera. F1, F2, F3 or Q (or panning) end it.
    // follow() puts the camera on the object as it now is.
    void startFollowing();
    void stopFollowing() { following_ = false; }
    bool following() const { return following_; }
    void follow(float x, float y, float heading, const std::string& model);
    static constexpr float kBlockUnits = 16384; // a block's size in model units
    size_t buildingsShown() const { return buildings_.size(); }

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
    using TileEdges = TileSides;

    // A placed object, ready to draw.
    struct Placed {
        Vec3 position;
        float angle; // anticlockwise, in radians
        const TileModel* model;
        float progress = 1;
        std::string builder;
        int builders = 2;
    };
    // A model's moving parts: its metamodel, animations and turret.
    struct Motion {
        MdlModel mdl;
        std::vector<Animation> animations;
        bool turret = false;
    };
    const Motion& motionOf(const std::string& model);

    void show(int index);
    void resetView();
    void render();
    void updateTitle();
    // An entry of the map's archive; ground tiles get textures by role, other
    // models those named after them.
    void loadModel(size_t entry, TileModel& m, bool ground);
    // The rest of loading a model, once read; buildings get their doors'
    // and aprons' textures too.
    void prepareModel(const std::vector<uint8_t>& data, TileModel& m, bool ground, bool building);
    // A model by name, loaded once; buildings get their doors' and aprons'
    // textures.
    const TileModel& namedModel(const std::string& name, bool building = true);
    const TileModel& tileModel(uint32_t index);
    float groundHeight(float x, float y);
    void placeScenery();
    void makeUnderlay();
    const TileEdges& tileEdges(uint32_t tile);
    // Walls: the quads closing the gaps between tiles' edges, found once
    // for the shown map (makeWalls), each frame only drawn.
    struct Wall {
        Vec3 corner[4];     // round the quad: this tile's edge, then the neighbour's
        float rise[4];      // each corner's height above the wall's foot
        Vec3 middle;        // the shared edge's middle, at height 0 (culling)
        uint32_t high;      // the higher tile's record (its edge cells' codes)
        int side, quarter;  // 0 the tile's east edge, 1 its north; which quarter of it
        bool higherHere;    // this tile is the higher one
        float foot;         // the lowest corner's height
    };
    std::vector<Wall> wallQuads_;
    void makeWalls();
    // The Tricorder's blocks for a frame (prepareTricorder): each block's
    // class and whether it's in view, from the sources, and the palette's
    // colours shifted for each class's band, for the race.
    void prepareTricorder();
    std::vector<int8_t> tricorderClasses_;
    std::vector<uint8_t> tricorderViews_;
    std::vector<std::array<uint8_t, 3>> tricorderShifted_; // band * palette size + code
    char tricorderShiftedRace_ = 0;
    // The relief's layer code at a point (in tiles), 0 for none or off
    // the map; how many layers the ground has there.
    unsigned layerCode(float fx, float fy, int layer) const;
    int groundLayers(float fx, float fy) const;
    Surface reliefSurface_; // the relief drawn, before it's put on the screen
    Rasterizer reliefRaster_;
    int tileAt(int tx, int ty) const; // tile record at a tile position, or -1
    static unsigned terrainCode(const GmfMap::Tile& tile, int cx, int cy);

    System& sys_;
    Surface& screen_;
    const Archives& archives_;
    Rasterizer raster_;
    std::vector<EntryRef> maps_;
    std::vector<Level> levels_;
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
    std::map<std::string, TileModel> buildingModels_; // by name, across maps
    std::vector<Placed> buildings_;
    std::vector<bool> hidden_; // placed_ objects under a building or on paved_
    std::vector<UnitDraw> units_;
    Metamodels metamodels_;
    std::map<std::string, Motion> motions_;
    uint64_t animClockMs_ = 0;
    std::optional<Placed> ghost_;
    bool ghostValid_ = false;
    View view_ = View::BirdsEye;
    float markX_ = 0, markY_ = 0; // Z's camera place
    bool marked_ = false;
    bool locked_ = false;
    bool following_ = false;
    float followDistance_ = 1000, followOrbit_ = 0, followRise_ = 0;
    std::vector<std::array<float, 4>> buildingAreas_; // middle x, y and half sizes, in model units
    void updateHidden();
    std::set<std::pair<int, int>> paved_;
    std::map<uint32_t, TileModel> pavedModels_; // tile models with the concrete texture throughout
    const TileModel& pavedModel(uint32_t index);
    Texture underlay_; // ground showing through tiles with GmfMap::Tile::underlay

    float targetX_ = 0, targetY_ = 0, targetZ_ = 0, yaw_ = 0, pitch_ = 0, distance_ = 1;
    bool textured_ = true, outlines_ = false, walls_ = true, scenery_ = true, smooth_ = true;
    int colouring_ = 0; // see kColouringNames
    bool dragging_ = false;
    int dragX_ = 0, dragY_ = 0;
    bool dirty_ = true, overlayDirty_ = false, sceneHeld_ = false;
    std::vector<uint8_t> scene_; // the last scene drawn (the screen's pixels before the overlay)
    // The camera it was drawn from (mode, target, yaw, pitch, distance)
    // and when; whether the units or buildings changed since.
    std::array<float, 7> sceneCamera_{};
    uint64_t sceneMs_ = 0;
    bool sceneChanged_ = false;
    std::string shownTitle_;
    unsigned drawnTiles_ = 0, drawnFaces_ = 0, drawnObjects_ = 0;
    char tricorderRace_ = 'F';
    int tricorderScheme_ = 0;
    std::function<int(int, int)> tricorderClass_;
    std::function<bool(int, int)> tricorderInView_;
    float reliefTileX_ = 0, reliefTileY_ = 0; // the ground tile being drawn (for the relief's colours)
    uint32_t shimmerFrame_ = 0; // a new throw each game tick (50 ms)
    std::function<void()> overlay_;
    std::function<std::string(const std::string&)> titleFilter_;
};

// Plays the sounds (.wav) and shows their waveforms.
//   Left/Right: previous/next   PageUp/PageDown: jump 10   Home/End: first/last
//   Space or Enter: play/stop   A: play each sound on selecting it
class SoundViewer {
public:
    // gameDir: the game folder, whose loose .mp3 and .wav files (Media/) are
    // listed after the archives' sounds; empty for none. mixer: the engine's,
    // playing into the System's output; nullptr when there's no output.
    SoundViewer(System& sys, Surface& screen, const Archives& archives, audio::Mixer* mixer,
                const std::filesystem::path& gameDir = {});
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
    std::string archiveOf(size_t sound) const; // its archive's file name

    System& sys_;
    Surface& screen_;
    const Archives& archives_;
    // A sound in an archive, or a loose file in the game folder.
    struct Item {
        EntryRef ref{0, 0};
        std::filesystem::path file; // empty for an archive's
        std::string name;           // the archive's name for it, or the path in the game folder
    };
    Error read(size_t sound, std::vector<uint8_t>& data) const;
    std::vector<Item> sounds_;
    // The archives store some sounds many times over (kliactor has kneg1.wav
    // 24 times, in 4 slots of each vehicle's set), sometimes under other
    // names: each sound's first copy, and the sounds listed (all, or only
    // the first of each set of copies).
    std::vector<size_t> firstCopy_;
    std::vector<size_t> shownSounds_;
    bool showCopies_ = false;
    bool shown_ = false, autoPlay_ = false, wasPlaying_ = false;
    int current_ = 0;
    std::string status_;
    std::shared_ptr<const audio::Sound> sound_; // the one shown
    audio::Mixer* mixer_;                       // one sound at a time here
    audio::SoundHandle playing_ = audio::kNoSound;
};

// Shows the string tables (.stb), word-wrapped, with their ids.
class StringViewer {
public:
    StringViewer(System& sys, Surface& screen, const Archives& archives);
    size_t count() const { return tables_.size(); }
    void activate();
    void handleKey(uint32_t key);

private:
    struct Span {
        std::string text;
        Color color;
    };
    struct Line {
        std::string id; // on a string's first line
        std::vector<Span> spans;
    };

    void show(int index);
    void layout(); // wraps the table into lines_ for the screen's width
    void draw();
    void updateTitle();
    int rows() const; // lines that fit on the screen
    // Text in the current font: Reluva's own, or one of the game's.
    int measure(std::string_view text) const;
    int lineHeight() const;
    int drawSpan(int x, int y, std::string_view text, Color color);

    System& sys_;
    Surface& screen_;
    const Archives& archives_;
    std::vector<EntryRef> tables_;
    std::vector<GameFont> fonts_;
    int font_ = -1; // index in fonts_, -1 for the built-in font
    bool shown_ = false;
    int current_ = 0, top_ = 0, scale_ = 1;
    StringTable table_;
    std::string status_;
    std::vector<Line> lines_;
};

// Shows the game's fonts: each one's characters and some sample text.
class FontViewer {
public:
    FontViewer(System& sys, Surface& screen, const Archives& archives);
    size_t count() const { return fonts_.size(); }
    void activate();
    void handleKey(uint32_t key);

private:
    void draw();
    void updateTitle();

    System& sys_;
    Surface& screen_;
    std::vector<GameFont> fonts_;
    int current_ = 0, scale_ = 2;
    bool boxes_ = false; // outline each glyph's box
};

// Shows the missions level.dat lists: each one's map (through a MapViewer,
// so its keys move the view), its briefing pages from the side's string
// table, its voice clips (Media/FedVO, KlingVO, RomVO) and its planet's
// music. The maps hold no units, buildings or colonies (see
// docs/formats/newworlds/gmf.md); where the missions put them isn't known.
//   Left/Right: previous/next mission   Up/Down: briefing page
//   B: briefing on/off   J: side (joint missions)
//   Space: play/stop the voice clip   N/P: next/previous clip (and play it)
//   M: the planet's music on/off   and the map viewer's keys
//   Enter: play the briefing (its pages, views and clips in turn, as
//   recordings of play showed them), or skip to its next step
//   Backspace: stop the briefing
//   X: play the mission's script a step at a time, in the order the
//   recording showed its events (the messages and objectives as they
//   change); Z: stop it
//   I: the game's interface over the map (its panels in the side's style,
//   with the script's messages and objectives; not playable)
//   C: the colony on/off (its hub where the recordings put it, and the
//   buildings the script builds; see colony.h)   V: look at the colony
//   F3: play on the map (the game's camera and controls, the colony's
//   starting units to select and move; world.h): Escape asks whether to
//   leave (in play F1, F2 and F3 are the game's cameras, F3 its close-up)
//   G: the mission as the game runs it (loading, briefing, play, report)
// Its parts: mission_viewer.cpp the page, play's start and its savestates;
// mission_play.cpp play's world drawn (units, combat, buildings going up);
// mission_input.cpp play's controls; mission_hud.cpp play's panels;
// mission_briefing.cpp the briefing; mission_flow.cpp the run (G);
// mission_audio.cpp the voices, sounds and music.
class MissionViewer {
public:
    MissionViewer(System& sys, Surface& screen, const Archives& archives, std::vector<Level> levels,
                  audio::Mixer* mixer, const std::filesystem::path& gameDir);
    size_t count() const { return missions_.size(); }
    void activate();
    void deactivate(); // stops the sound when switching to another viewer
    void handleEvent(const Event& ev);
    void update(uint64_t nowMs);
    // Playing: Tab and Escape are the game's, not the data viewer's.
    bool playing() const { return playing_; }
    // A run (G) under way, at any stage: the keys are the run's too.
    bool inRun() const { return flow_ != Flow::None; }
    // The side the page shows ('F', 'K', 'R'; 0 with no missions).
    char side() const;
    // Savestates (docs/savestates.md): a mission being played can be
    // saved (`why` says why not); loading one starts its mission and puts
    // the play and the page back as they were.
    bool canSave(std::string& why) const;
    std::string stateDescription() const;
    Error syncState(save::Serializer& s);
    // The mission being played from its start again (a load that failed
    // after it began putting the play back); false, doing nothing, if none
    // is.
    bool restartPlay();
    // A race's campaign just won (its last mission's report closed): the
    // side, once ('F', 'K', 'R'; 0 none), for its movie.
    char takeCampaignWon() {
        const char s = campaignWon_;
        campaignWon_ = 0;
        return s;
    }
    // For the page's regression test (tests/test_newworlds_page.cpp): the
    // missions in the page's order, the one shown, play's pause and quit
    // box, and its random picks (the units' voices) seeded.
    const Level& mission(size_t i) const { return missions_[i]; }
    int shown() const { return current_; }
    bool paused() const { return play_.paused; }
    bool quitAsked() const { return play_.quitAsked; }
    void seed(uint32_t s) { rng_.seed(s); }

private:
    char campaignWon_ = 0;
    struct Page {
        std::string heading; // without the ~s; empty for the title page
        std::string text;
        uint32_t id = 0; // the string's (the first one's, for plain strings run together)
    };
    // A rectangle on the screen, drawn and clicked.
    struct Rect {
        int x = 0, y = 0, w = 0, h = 0;
        bool contains(int px, int py) const { return px >= x && py >= y && px < x + w && py < y + h; }
    };

    // --- The page (mission_viewer.cpp) ---------------------------------------
    void show(int index);
    void loadSide(); // the briefing and clips for the mission's current side
    void drawBriefing();
    void drawNoMap();
    std::string title(const std::string& mapTitle) const;
    void updateTitle();
    void redraw();
    void stepScript(); // the next scripted event (X)
    void startColony(); // the hub alone, for the mission's side
    // Play: the game's controls over the map.
    void startPlay();
    void stopPlay();
    void resetPlayUi(); // play's panels closed and its picks dropped (both of the above)
    bool hubPosition(float& x, float& y) const;
    void deliver(const ScriptEvent& fired, const std::vector<MissionRun::Output>& out);
    void showColony();  // hands the colony's buildings to the map
    void stopScript();
    bool mapInView() const; // the map is drawn (not a briefing backdrop)
    // Text in the briefing's font: the game's fe8pt if there, else Reluva's.
    int measure(std::string_view text) const;
    int lineHeight() const;
    void drawText(int x, int y, std::string_view text, Color color);

    // --- Play's world drawn (mission_play.cpp) -------------------------------
    void showUnits();
    // Building in play (B): the menu's types for the side, the hologram
    // under the pointer, placing it, and its construction drawn.
    std::vector<const BuildingType*> buildMenu() const;
    std::vector<const VehicleType*> vehicleMenu() const; // the yards' offer, in the game's order
    void choose(const BuildingType& t);
    void updateGhost();
    void placeBuilding();
    void showProgress();
    void combatStep(); // what combat did, to draw
    void drawCombat();
    bool visible(const Unit& u) const;
    // What moves is drawn between ticks (mission_sim.h's tickShare): a
    // unit as far from where the last tick found it to where it is (its
    // place, heading and turret); a bee from where it was as the last
    // tick began (by id: x, y, heading, lift), noted as each tick begins
    // (MissionSim::advance's beforeTick).
    struct Pose {
        float x, y, heading, turretYaw;
    };
    Pose drawnAt(const Unit& u) const;
    void noteBees();

    // --- Play's controls (mission_input.cpp) ---------------------------------
    bool playEvent(const Event& ev); // true if play used it
    bool pressButton(int x, int y, bool right = false, int clicks = 1);
    int panelAt(int x, int y) const; // the topmost panel there (its index in panels), or -1
    // The selection replaced by these units: no building picked, nothing
    // asked about one, the hub's details put away (as a click selects).
    void selectUnits(const std::vector<int>& ids);
    void selectAllOfKind(char k);
    void orderOn(int target);
    void openPlaybook(int target, float gx, float gy);
    void playbookOrder(char order);
    void rallySelected();
    void jumpTo(int slot);
    void cycleCamera(bool buildings);
    void doubleClicked(int unit);
    void followInF1(int unit);
    void cameraTo(const Unit& u);

    // --- Play's panels (mission_hud.cpp) -------------------------------------
    void drawInterface();
    void drawPlay();
    void drawPlaybook();
    Rect playbookIcon(int i) const; // the playbook's i-th icon, in its column as last drawn
    std::string compassStrip() const;
    Rect fullTricorderRect(int& tiles) const; // the full-screen Tricorder's map, and its zoom
    // The Tricorder's relief a click is on: the full-screen one's, else
    // the small one's as last drawn (its w 0 if there's none); and a
    // relief at a place, its view's camera as it is now.
    MapViewer::Relief tricorderRelief() const;
    MapViewer::Relief tricorderRelief(const Rect& map, int tiles) const;
    void drawTricorderMap(const MapViewer::Relief& relief);
    std::string tricorderReadout(const MapViewer::Relief& relief) const;
    void drawSceneInset(const Rect& at);
    std::string tricorderReadoutAt(float wx, float wy, float reach) const; // the selection box, the full-screen Tricorder
    std::vector<int> allyRaces() const;
    // A selected object's panel lines, as the game's (exe.md#the-selection-panel),
    // and its hit points' share for the bar under them.
    std::vector<std::string> objectPanel(const Unit& u) const;
    std::vector<std::string> staffCards() const;
    std::vector<std::string> trainingLines() const;
    const ColonyStructure* structureOf(int unit) const; // the colony's building's, or none
    std::vector<std::string> colonyDetails() const;
    std::vector<int> pendingBuilds() const;
    int menuLines(int lh) const;
    void drawMenu(int x, int ty, int w, const std::vector<const VehicleType*>& vehicles,
                  const std::vector<const BuildingType*>& buildings, size_t jobs);
    const char* buildMenuTitle() const;
    const char* pendingOrderName() const;

    // --- The briefing (mission_briefing.cpp) ---------------------------------
    // The briefing player: steps run one after another, each as long as its
    // clip (a few seconds when it has none, or its clip is missing).
    void startBriefing();
    void stopBriefing();
    void startStep(int step);
    void flyScene(int index, uint64_t nowMs);
    void drawBackdrop(BriefingView view);
    // The game's own art for the briefing's scenes, when the archives have
    // it (glblgui.cat, glblgame.cat): each side's emblem screen, and the
    // Tabula Rasa galaxy model with its textures.
    void loadBriefingArt();
    bool loadScreen(const std::string& prefix, Texture& out) const;
    bool drawEmblem(char side);
    bool drawGalaxy();
    void loadStarSystem(); // the one the mission's table names
    bool drawStarSystem();
    // A model turning on the grid, with its textures (loaded once).
    bool drawModel(const std::string& name);

    // --- The run, G (mission_flow.cpp) ---------------------------------------
    void startFlow();
    void endFlow();
    void drawFlow();
    std::string levelLabel() const;

    // --- Voices, sounds and music (mission_audio.cpp) ------------------------
    void speak(const std::string& what);
    void unitSpeaks(int unit, int group);
    void orderSpoken(bool moving);
    audio::SoundHandle playAt(const std::string& archive, const std::string& stem, float x, float y, float gain);
    void playClip(int index);
    void stopClip();
    void startMusic(); // the shown map's planet theme, looped
    void stopMusic();
    bool clipPlaying() const;
    // Reads and decodes one of the game folder's sound files.
    std::shared_ptr<const audio::Sound> loadSound(const std::string& stem);

    audio::SoundHandle objectSoundHandle_ = audio::kNoSound; // the last double-clicked building's
    std::map<std::string, std::shared_ptr<const audio::Sound>> archiveSounds_; // decoded, by archive/stem
    std::map<int, audio::SoundHandle> driveHandles_;                           // each unit's start-up sound
    Rect quitYes_, quitNo_;             // Escape's box's buttons, as last drawn
    bool unfocused_ = false;
    static constexpr size_t kMultiPage = 15;            // MULTISELECTION entries a page
    static constexpr int kMultiCellW = 46, kMultiCellH = 48;
    std::minstd_rand rng_{std::random_device{}()};
    std::map<std::string, uint64_t> spokenAt_; // each clip, when it last played
    audio::SoundHandle stingHandle_ = audio::kNoSound; // the colony's danger music
    struct BriefingModel {
        TexturedModel model;
        bool ok = false;
    };
    std::map<std::string, BriefingModel> models_;
    std::vector<TextureFile> textureFiles_;

    System& sys_;
    Surface& screen_;
    const Archives& archives_;
    std::vector<Level> missions_;
    audio::Mixer* mixer_;
    MapViewer map_;
    std::vector<GameFont> fonts_;
    const GameFont* font_ = nullptr;
    // The game folder's loose MP3s, by upper-case stem (F1005 -> its path).
    std::map<std::string, std::filesystem::path> mediaFiles_;

    bool shown_ = false, hasMap_ = false, dirty_ = true, briefing_ = true, music_ = false, wasPlaying_ = false;
    int current_ = 0, side_ = 0, page_ = 0, clip_ = 0;
    std::string mapName_, mapNote_, mapTitle_, tableName_, status_, soundStatus_;
    std::vector<Page> pages_;
    std::vector<std::string> clips_; // stems, in order
    StringTable table_;
    std::vector<BriefingStep> script_;
    std::map<char, Texture> emblems_; // 640 x 480, by side
    std::map<char, Texture> losses_; // the mission lost screens, by side
    // A mission played through as the game runs it (G): a loading box, the
    // briefing, a loading box, play; once won or lost, the debriefing box,
    // then the won or lost screen with the MISSION REPORT and CONTINUE.
    enum class Flow { None, Loading, Briefing, Starting, Playing, Debriefing, Report };
    Flow flow_ = Flow::None;
    // The loading boxes stay only while the loading lasts, as the game's
    // do (the recordings' 3-10 s were the player's clicks): here a frame.
    static constexpr uint64_t kLoadingMs = 1, kStartingMs = 1;
    static constexpr uint64_t kFlowUnstarted = ~uint64_t(0);
    uint64_t flowStartMs_ = 0;
    bool flowWon_ = false;
    Rect continue_; // the CONTINUE button
    bool flowScreen() const {
        return flow_ == Flow::Loading || flow_ == Flow::Starting || flow_ == Flow::Debriefing || flow_ == Flow::Report;
    }
    GeoModel galaxy_;
    std::vector<Texture> galaxyTextures_; // in the order its faces number them
    std::vector<std::string> systemNames_; // geo/solsys/'s systems
    std::string systemName_;
    GeoModel system_;
    StarSystemLayout systemLayout_;
    std::vector<Texture> systemTextures_; // per texture number
    Rasterizer raster_;
    bool running_ = false, stepOnMap_ = false;
    // The mission's script, being walked through: in play the
    // simulation's, on the missions page its own.
    MissionRun* run_ = nullptr;
    std::unique_ptr<MissionRun> pageRun_;
    std::vector<ScriptEvent> walk_;
    size_t walkPos_ = 0;
    std::vector<std::pair<std::string, std::string>> log_; // sender, text
    std::string walkNote_;
    GameUi ui_;
    char uiSide_ = 0;
    std::unique_ptr<ColonyLayout> colony_;
    // In play: the mission being played (mission_sim.h), and its parts.
    std::unique_ptr<MissionSim> sim_;
    // The colony's capacity carried from mission to mission in this
    // session, as the game's (exe.md#the-economy): the next mission played
    // starts from it (U, between missions, puts it back to 50); and the
    // one the mission being played started from (saved with it).
    CarriedCapacity carried_, carryIn_;
    World* world_ = nullptr;
    Economy* economy_ = nullptr;
    // The colony's buildings' world objects (-1 once destroyed) and
    // economy structures, the bees in the hub (by kind) and all of them.
    const std::vector<int>& colonyObjects() const;
    const std::vector<int>& structureIds() const;
    const std::map<std::string, int>& beesHoused() const;
    int housed() const { return sim_ ? sim_->housed() : 0; }
    // The playbook (exe.md#orders): the orders a double click offers for
    // the selection and what was clicked, as a column of icons beside the
    // Tricorder. Each entry: its icon, its name, and the order (f follow
    // and attack, b assault, c capture, s/m/e scans, r attack en route,
    // l protect from location, v move without attacking, y back to the
    // rally point, a protect area).
    struct PlaybookEntry {
        const char* icon;
        const char* name;
        char order;
    };
    // Buttons drawn on the panels this frame: where, and what they do.
    struct PanelButton {
        int x, y, w, h;
        std::string action;
    };
    // The panels drawn this frame, in order (each over those before): where,
    // and its first button in buttons (those after it, up to the next
    // panel's, are its own). A click inside one is the interface's.
    struct PanelArea {
        int x, y, w, h;
        size_t firstButton;
    };
    struct Flash {
        float x0, y0, x1, y1;
        WeaponKind weapon;
        uint64_t until;
    };
    struct Blast {
        float x, y, size;
        uint64_t start;
    };
    // What play's panels show and wait on, and what it draws between
    // ticks: nothing open, picked or asked as a mission starts (and once
    // it stops), so none of it outlives its play (resetPlayUi()).
    struct PlayUi {
        bool quitAsked = false;            // Escape's box up
        bool paused = false;
        bool boxing = false;               // a left drag's box being drawn
        bool tricorderFull = false;        // the full-screen Tricorder (Tab)
        std::vector<Flash> flashes;
        std::vector<Blast> blasts;
        char pendingOrder = 0;
        bool researchMenu = false;
        bool vehicleMenu = false;          // the vehicle yards' list shown (V)
        bool buildMenu = false, ghostOk = false;
        std::string placing;               // the type being placed, or empty
        int ghostA = -1, ghostB = -1;      // where its hologram stands
        int selectedBuilding = 0;          // a building selected (its unit id), its details in NEW WORLDS
        int razeAsked = 0;                 // DEMOLISH pressed: the building asked about (yes / no)
        int cancelAsked = 0;               // a site or upgrade picked on the colony's details (yes / no)
        int orderAsked = 0;                // a vehicle order picked on the yards' list (a second click cancels it)
        int cancelFrom = 0;                // the building whose details it was picked on (the hub)
        bool colonyDetails = false;        // the hub's INFO: the colony's details instead of the hub's
        bool staffView = false;            // the STAFF panel: the officers' cards
        bool trainingView = false;         // its TRAINING list: the officers' courses
        int pickedOfficer = -1;            // a card picked: the next building or vehicle clicked takes them
        uint64_t officerClickMs = 0;       // when a card was last clicked (a double click: the camera to them)
        std::vector<PlaybookEntry> playbook;
        int playbookTarget = 0;            // what it was opened on (0: the ground)
        float playbookX = 0, playbookY = 0;
        std::array<int, 2> playbookAt{};   // its column's top-left
        int multiPage = 0;
        std::set<int> multiTicked;
        std::map<int, std::array<float, 4>> beesBefore;
        std::array<int, 5> jumpedTo{};     // JUMP TO: each class's object the camera went to last
        int cycledBuilding = 0, cycledVehicle = 0; // Shift+B / Shift+V: where the camera went last
        int followed = 0;                  // the object F1's camera follows
        int alertedLast = 0;               // Space: the alerted object the camera went to last
        int rallied = 0;                   // vehicles sent back to the rally point (Y): its parking places in turn
        int menuPage = 0;                  // the build or vehicle menu's page
        std::pair<int, uint64_t> menuFlash{-1, 0}; // the menu item last clicked, and when
        int commsShown = -1;               // the message COMMUNICATIONS shows (-1: the latest)
        bool commsClosed = false;          // put away with X, until the next message
        bool commandShown = true;          // the COMMAND panel (MISSION shows or hides it)
        bool allyOpen = false;             // the ALLY page
        int allyPick = -1;                 // its race clicked (a slot): the postures offered
        std::set<int> shownOthers;         // the other sides' buildings in sight
        std::vector<std::pair<int, float>> otherBuildings; // the other sides' shown: unit id, progress shown
        std::map<int, int> unitVoices;     // each vehicle's voice, once it's spoken
        std::vector<int> spokenSelection;  // the selection its first vehicle last answered
        std::vector<PanelButton> buttons;
        std::vector<PanelArea> panels;
    };
    PlayUi play_;
    uint64_t blinkPhase_ = 0;           // the Tricorder's blink, as last drawn (nowMs / 500)
    double edgeClock_ = 0;
    float edgeSpeed_ = 0;               // edge scrolling's speed, units a tick
    int edgeTicks_ = 0; // F1's edges raise and lower the camera every fourth tick
    bool mouseSeen_ = false;            // the pointer has moved (it starts at 0, 0)
    int tricorderZoom_ = 1;             // 0 X4 (16 tiles across), 1 X2 (the start), 2 X1
    // The Tricorder's view: 0 Starship, 1 Relief (play's), 2 Relief orbit,
    // 3 Relief fixed (exe.md#the-tricorder). Only a briefing step sets
    // another in the game; play keeps Relief.
    int tricorderView_ = 1;
    int sideW_ = 120;                   // the right-hand panels' width, as last drawn
    // The other sides, and combat: each colony building's object in the
    // world (0 once destroyed), shots and blasts being drawn, and the
    // sites the scripts have heard seen or approached.
    AiSides* ai_ = nullptr;
    // Scans, captures and officers; a field order waiting for its target
    // (s, m, e: a scan; c: a capture; 0 none).
    FieldWork* fieldWork_ = nullptr;
    Research* research_ = nullptr;     // the side's research, and its list shown (R)
    ScriptEffects* effects_ = nullptr; // the script's effects in play
    Fog* fog_ = nullptr;               // the fog of war
    int playerSlot_ = 0;
    size_t shownBuildings_ = 0;
    size_t buildingsShown_ = 0; // all the map's, the colony's first
    Rect tricorder_; // the TRICORDER panel's map, as last drawn
    bool playing_ = false;
    int mouseX_ = 0, mouseY_ = 0, boxX_ = 0, boxY_ = 0;
    uint64_t lastTickMs_ = 0;
    ColonySite colonySite_;
    bool colonyShown_ = true;
    bool interface_ = false;
    int step_ = 0, savedColouring_ = 0;
    uint64_t nowMs_ = 0, stepEndMs_ = 0, lastTurnMs_ = 0; // stepEndMs_: when a step without a playing clip ends
    uint64_t stepMinimumMs_ = 0; // a step with a clip lasts at least until then
    // The briefing scene playing (-1 none): its unit's place, heading, next
    // waypoint, and when it last moved.
    int scene_ = -1, sceneLeg_ = 0;
    float sceneX_ = 0, sceneY_ = 0, sceneHeading_ = 0;
    uint64_t sceneMs_ = 0;
    // The speech channels (exe.md#sounds): clipHandle_ the officers' (1),
    // which the briefings, the script's messages and announcements and the
    // vehicles' voices use too; buildingClipHandle_ the finished
    // buildings' (0).
    audio::SoundHandle clipHandle_ = audio::kNoSound, buildingClipHandle_ = audio::kNoSound;
    audio::SoundHandle musicHandle_ = audio::kNoSound;
    std::string musicName_;
};

} // namespace reluva::newworlds
