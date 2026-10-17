// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// Star Trek: New Worlds engine (clean-room reimplementation).
//
// Everything in this engine must come from the game's data files, observed
// behaviour of the original, or documented reverse engineering of a legally
// owned copy. Never from leaked source code. See CONTRIBUTING.md.
//
// For now the engine is a data viewer: it opens every .cat archive in the
// game folder and lets you browse the textures.
//   Left/Right: previous/next texture   PageUp/PageDown: jump 10
//   Home/End: first/last                F: filter (all, failed, colour-keyed, with alpha)
//   Esc: back to the launcher

#include "engines/engine.h"
#include "engines/newworlds/cat_archive.h"
#include "engines/newworlds/texture.h"
#include "common/fs.h"

#include <algorithm>
#include <cstring>
#include <memory>

namespace reluva {

namespace {

using newworlds::CatArchive;

// Detection table. Built with `reluva --hash`: each entry needs the size and
// the MD5 of the first kDetectionHashBytes bytes.
//
// Detection keys on glblgame.cat: patches tend to replace the executable but
// not the large archives, so the executable is only required to exist.
const std::vector<GameVariant> kVariants = {
    {"newworlds", "Star Trek: New Worlds", "en", "windows",
     {{"glblgame.cat", "9b5c322aef388f8e58e72b81bd9a91ad", 6883984},
      {"STNW.exe", "", -1}}},
};

constexpr int kScreenW = 640;
constexpr int kScreenH = 480;

bool endsWith(const std::string& s, const char* suffix) {
    const size_t n = std::strlen(suffix);
    return s.size() >= n && s.compare(s.size() - n, n, suffix) == 0;
}

class NewWorldsEngine : public Engine {
public:
    NewWorldsEngine(System& system, GameTarget target) : sys_(system), target_(std::move(target)) {}

    Error run() override {
        if (Error err = openArchives())
            return err;
        if (textures_.empty())
            return Error::fail("No textures found in " + target_.path.string());
        if (Error err = sys_.initGraphics(kScreenW, kScreenH, PixelFormat::RGBA32))
            return err;
        screen_.create(kScreenW, kScreenH, PixelFormat::RGBA32);
        classify();
        applyFilter(0); // draws the first texture, so the screen must exist


        while (true) {
            Event ev;
            while (sys_.pollEvent(ev)) {
                if (ev.type == EventType::Quit || ev.type == EventType::ReturnToLauncher)
                    return Error::success();
                if (ev.type != EventType::KeyDown)
                    continue;
                const int n = int(visible_.size());
                if (ev.key == 'f') {
                    applyFilter((filter_ + 1) % kFilterCount);
                    continue;
                }
                if (n == 0)
                    continue;
                switch (ev.key) {
                case key::Escape: return Error::success();
                case key::Right: show((current_ + 1) % n); break;
                case key::Left: show((current_ + n - 1) % n); break;
                case key::PageDown: show(std::min(current_ + 10, n - 1)); break;
                case key::PageUp: show(std::max(current_ - 10, 0)); break;
                case key::Home: show(0); break;
                case key::End: show(n - 1); break;
                default: break;
                }
            }
            sys_.updateScreen(screen_);
            sys_.delayMillis(10);
        }
    }

private:
    struct TextureRef {
        size_t archive, entry;
        bool ok = false, colourKeyed = false, alpha = false;
    };

    static constexpr int kFilterCount = 4;
    static constexpr const char* kFilterNames[kFilterCount] = {"all", "failed", "colour-keyed", "with alpha"};

    // Decode every texture once so the filters know which is which.
    void classify() {
        std::vector<uint8_t> data;
        newworlds::Texture tex;
        for (TextureRef& ref : textures_) {
            CatArchive& cat = *archives_[ref.archive];
            ref.ok = !cat.read(cat.entries()[ref.entry], data) && !newworlds::decodeTexture(data, tex);
            ref.colourKeyed = ref.ok && tex.colourKeyed;
            ref.alpha = ref.ok && tex.format == 6;
        }
    }

    void applyFilter(int filter) {
        filter_ = filter;
        visible_.clear();
        for (size_t i = 0; i < textures_.size(); ++i) {
            const TextureRef& r = textures_[i];
            const bool keep = filter == 0 || (filter == 1 && !r.ok) || (filter == 2 && r.colourKeyed) ||
                              (filter == 3 && r.alpha);
            if (keep)
                visible_.push_back(i);
        }
        if (visible_.empty()) {
            drawCheckerboard();
            sys_.setWindowTitle(std::string("New Worlds textures [") + kFilterNames[filter_] +
                                "]: none (press F for the next filter)");
            return;
        }
        show(0);
    }

    Error openArchives() {
        std::vector<std::filesystem::path> paths;
        std::error_code ec;
        for (std::filesystem::directory_iterator it(target_.path, ec), end; !ec && it != end; it.increment(ec))
            if (fs::toLower(it->path().extension().string()) == ".cat")
                paths.push_back(it->path());
        if (paths.empty())
            return Error::fail("No .cat archives in " + target_.path.string());
        std::sort(paths.begin(), paths.end());

        for (const auto& p : paths) {
            auto cat = std::make_unique<CatArchive>();
            if (Error err = cat->open(p))
                return err;
            const auto& entries = cat->entries();
            for (size_t i = 0; i < entries.size(); ++i)
                if (entries[i].tag == newworlds::cattag::Texture && !endsWith(entries[i].name, ".stb"))
                    textures_.push_back({archives_.size(), i});
            archives_.push_back(std::move(cat));
        }
        return Error::success();
    }

    void show(int index) {
        current_ = index;
        const TextureRef& ref = textures_[visible_[size_t(index)]];
        CatArchive& cat = *archives_[ref.archive];
        const auto& entry = cat.entries()[ref.entry];

        drawCheckerboard();
        std::string status;
        std::vector<uint8_t> data;
        newworlds::Texture tex;
        if (Error err = cat.read(entry, data)) {
            status = err.message;
        } else if (Error err2 = newworlds::decodeTexture(data, tex)) {
            status = err2.message;
            fill(0, 0, kScreenW, kScreenH, 80, 0, 0); // dark red: couldn't decode
        } else {
            drawTexture(tex);
            status = "format " + std::to_string(tex.format) + ", " + std::to_string(tex.width) + "x" +
                     std::to_string(tex.height) +
                     (tex.colourKeyed ? ", colour key " + std::to_string(tex.keyIndex) : std::string());
        }
        sys_.setWindowTitle(std::string("New Worlds textures [") + kFilterNames[filter_] + "] " +
                            std::to_string(index + 1) + "/" + std::to_string(visible_.size()) + ": " +
                            cat.entries()[ref.entry].name + " (" + status + ")");
    }

    void fill(int x0, int y0, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
        for (int y = y0; y < y0 + h; ++y) {
            uint8_t* row = screen_.row(y) + x0 * 4;
            for (int x = 0; x < w; ++x, row += 4) {
                row[0] = r;
                row[1] = g;
                row[2] = b;
                row[3] = 255;
            }
        }
    }

    void drawCheckerboard() {
        for (int y = 0; y < kScreenH; ++y) {
            uint8_t* row = screen_.row(y);
            for (int x = 0; x < kScreenW; ++x, row += 4) {
                const uint8_t v = ((x / 16 + y / 16) & 1) ? 70 : 50;
                row[0] = row[1] = row[2] = v;
                row[3] = 255;
            }
        }
    }

    // Draw centred, scaled up by the largest whole factor that fits (or down
    // by halves), blended over the checkerboard using the alpha layer.
    void drawTexture(const newworlds::Texture& tex) {
        int up = 1, down = 1;
        while (tex.width * (up + 1) <= kScreenW - 20 && tex.height * (up + 1) <= kScreenH - 20)
            ++up;
        while (tex.width / down > kScreenW || tex.height / down > kScreenH)
            down *= 2;
        const int w = tex.width * up / down, h = tex.height * up / down;
        const int x0 = (kScreenW - w) / 2, y0 = (kScreenH - h) / 2;
        for (int y = 0; y < h; ++y) {
            const int sy = y * down / up;
            uint8_t* dst = screen_.row(y0 + y) + x0 * 4;
            for (int x = 0; x < w; ++x, dst += 4) {
                const uint8_t* src = &tex.rgba[(size_t(sy) * size_t(tex.width) + size_t(x * down / up)) * 4];
                const int a = src[3];
                for (int c = 0; c < 3; ++c)
                    dst[c] = uint8_t((src[c] * a + dst[c] * (255 - a)) / 255);
            }
        }
    }

    System& sys_;
    GameTarget target_;
    Surface screen_;
    std::vector<std::unique_ptr<CatArchive>> archives_;
    std::vector<TextureRef> textures_;
    std::vector<size_t> visible_; // indices into textures_ that pass the filter
    int filter_ = 0;
    int current_ = 0;
};

class NewWorldsMetaEngine : public MetaEngine {
public:
    const char* id() const override { return "newworlds"; }
    const char* name() const override { return "New Worlds"; }

    std::vector<DetectedGame> detect(const std::filesystem::path& dir) const override {
        return detectFromTable(id(), kVariants, dir);
    }

    std::unique_ptr<Engine> create(System& system, const GameTarget& target) const override {
        return std::make_unique<NewWorldsEngine>(system, target);
    }
};

} // namespace

const MetaEngine& newWorldsMetaEngine() {
    static const NewWorldsMetaEngine instance;
    return instance;
}

} // namespace reluva
