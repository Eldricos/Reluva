// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/events.h"
#include "common/system.h"
#include "common/audio/mixer.h"
#include "common/audio/sound.h"
#include "common/types.h"
#include "engines/newworlds/briefing.h"
#include "engines/newworlds/cat_archive.h"
#include "engines/newworlds/fif.h"
#include "engines/newworlds/geo.h"
#include "engines/newworlds/gmf.h"
#include "engines/newworlds/levels.h"
#include "engines/newworlds/mdl.h"
#include "engines/newworlds/mission_script.h"
#include "engines/newworlds/model_textures.h"
#include "engines/newworlds/rasterizer.h"
#include "engines/newworlds/string_table.h"
#include "engines/newworlds/texture.h"

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
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
    void loadAnimations(const std::string& stem, const std::string& modelFolder);
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
        std::string state;                             // the .mdl state it plays: walk, manipulate
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
    void setColouring(int colouring) {
        colouring_ = colouring;
        dirty_ = true;
    }

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
    Texture underlay_; // ground showing through tiles with GmfMap::Tile::underlay

    float targetX_ = 0, targetY_ = 0, yaw_ = 0, pitch_ = 0, distance_ = 1;
    bool textured_ = true, outlines_ = false, walls_ = true, scenery_ = true, smooth_ = true;
    int colouring_ = 0; // see kColouringNames
    bool dragging_ = false;
    int dragX_ = 0, dragY_ = 0;
    bool dirty_ = true;
    unsigned drawnTiles_ = 0, drawnFaces_ = 0, drawnObjects_ = 0;
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
class MissionViewer {
public:
    MissionViewer(System& sys, Surface& screen, const Archives& archives, std::vector<Level> levels,
                  audio::Mixer* mixer, const std::filesystem::path& gameDir);
    size_t count() const { return missions_.size(); }
    void activate();
    void deactivate(); // stops the sound when switching to another viewer
    void handleEvent(const Event& ev);
    void update(uint64_t nowMs);

private:
    struct Page {
        std::string heading; // without the ~s; empty for the title page
        std::string text;
        uint32_t id = 0; // the string's (the first one's, for plain strings run together)
    };

    void show(int index);
    void loadSide(); // the briefing and clips for the mission's current side
    void drawBriefing();
    void drawNoMap();
    std::string title(const std::string& mapTitle) const;
    void updateTitle();
    void redraw();
    // The briefing player: steps run one after another, each as long as its
    // clip (a few seconds when it has none, or its clip is missing).
    void stepScript(); // the next scripted event (X)
    void stopScript();
    void startBriefing();
    void stopBriefing();
    void startStep(int step);
    bool mapInView() const; // the map is drawn (not a briefing backdrop)
    void drawBackdrop(BriefingView view);
    // The game's own art for the briefing's scenes, when the archives have
    // it (glblgui.cat, glblgame.cat): each side's emblem screen, and the
    // Tabula Rasa galaxy model with its textures.
    void loadBriefingArt();
    bool drawEmblem(char side);
    bool drawGalaxy();
    // A model turning on the grid, with its textures (loaded once).
    bool drawModel(const std::string& name);
    struct BriefingModel {
        GeoModel geo;
        std::vector<Texture> textures;
        bool ok = false;
    };
    std::map<std::string, BriefingModel> models_;
    std::vector<TextureFile> textureFiles_;
    void playClip(int index);
    void stopClip();
    void startMusic(); // the shown map's planet theme, looped
    void stopMusic();
    bool clipPlaying() const;
    // Reads and decodes one of the game folder's sound files.
    std::shared_ptr<const audio::Sound> loadSound(const std::string& stem);
    // Text in the briefing's font: the game's fe8pt if there, else Reluva's.
    int measure(std::string_view text) const;
    int lineHeight() const;
    void drawText(int x, int y, std::string_view text, Color color);

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
    GeoModel galaxy_;
    std::vector<Texture> galaxyTextures_; // in the order its faces number them
    Rasterizer raster_;
    bool running_ = false, stepOnMap_ = false;
    // The mission's script, being walked through.
    std::unique_ptr<MissionRun> run_;
    std::vector<ScriptEvent> walk_;
    size_t walkPos_ = 0;
    std::vector<std::pair<std::string, std::string>> log_; // sender, text
    std::string walkNote_;
    int step_ = 0, savedColouring_ = 0;
    uint64_t nowMs_ = 0, stepEndMs_ = 0, lastTurnMs_ = 0; // stepEndMs_: when a step without a playing clip ends
    audio::SoundHandle clipHandle_ = audio::kNoSound, musicHandle_ = audio::kNoSound;
    std::string musicName_;
};

} // namespace reluva::newworlds
