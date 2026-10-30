// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/game_ui.h"

#include "common/text.h"

#include <algorithm>
#include <cctype>

namespace reluva::newworlds {

namespace {

bool loadPicture(const Archives& archives, const std::string& name, Texture& out) {
    for (const auto& a : archives)
        if (const CatEntry* e = a->find(name)) {
            std::vector<uint8_t> data;
            return !a->read(*e, data) && !decodeTexture(data, out);
        }
    return false;
}

// The Romulan recordings' faces by section, matched to the portraits
// (gui.md#the-in-game-interface): a section's officer, usually this one.
constexpr std::pair<const char*, const char*> kRomulanFaces[] = {
    {"Operations", "r_c17"}, {"Engineering", "r_c19"}, {"Intelligence", "r_c18"},
    {"Science", "r_c21"},    {"Security", "r_c20"},
};

// Each race's 40 officer faces, the small portraits (ptrt/crew/sm) in the
// order the game lists them: the mission gives each officer an index here.
// Some are shared with the natives or another race's folder.
constexpr const char* kFaces[3][40] = {
    {"f_c9_s",  "f_c33_s", "f_c7_s",  "f_c20_s", "f_c21_s",  "f_c32_s",  "f_c23_s", "f_c1_s",
     "k_plyr_s", "r_plyr_s", "taubat", "hubrin", "metar",   "orion",    "f_c31_s",  "f_c3_s",
     "f_c4_s",  "f_c5_s",  "f_c8_s",  "f_c11_s", "f_c12_s",  "f_c13_s",  "f_c14_s", "f_c34_s",
     "f_c16_s", "f_c19_s", "fairbk_s", "f_sarek_", "f_fox_s", "hubrin",  "f_c9_s",  "f_c33_s",
     "f_c1_s",  "f_c20_s", "f_c21_s", "f_c32_s", "f_c23_s",  "taubat",   "orion",   "hubrin"},
    {"k_c1_s",  "k_c2_s",  "k_c3_s",  "k_c4_s",  "k_c5_s",   "k_c6_s",   "k_c27_s", "f_c7_s",
     "k_plyr_s", "r_plyr_s", "taubat", "hubrin", "metar",   "orion",    "k_c7_s",   "k_c8_s",
     "k_c9_s",  "k_c10_s", "k_c11_s", "k_c12_s", "k_c13_s",  "k_c14_s",  "k_c15_s", "k_c16_s",
     "k_c17_s", "k_c18_s", "k_c19_s", "k_c20_s", "k_c21_s",  "kirgat_s", "k_c1_s",  "k_c2_s",
     "k_c3_s",  "k_c4_s",  "k_c5_s",  "k_c6_s",  "k_c27_s",  "taubat",   "orion",   "hubrin"},
    {"r_c17_s", "r_c18_s", "r_c19_s", "r_c20_s", "r_c21_s",  "r_c24_s",  "r_c25_s", "f_c7_s",
     "k_plyr_s", "r_plyr_s", "taubat", "hubrin", "metar",   "orion",    "r_c1_s",   "r_c2_s",
     "r_c3_s",  "r_c4_s",  "r_c5_s",  "r_c6_s",  "r_c7_s",   "r_c8_s",   "r_c9_s",  "r_c10_s",
     "r_c11_s", "r_c12_s", "r_c13_s", "r_c14_s", "r_c15_s",  "jathek_s", "r_nark_s", "r_c18_s",
     "r_c19_s", "r_c20_s", "r_c21_s", "r_c24_s", "r_c25_s",  "taubat",   "orion",   "hubrin"},
};

// Sections in the order the side's crew portraits are handed out.
constexpr const char* kSections[] = {"Command",      "Operations",   "Engineering",   "Science",
                                     "Medical",      "Security",     "Intelligence",  "Communications"};

// Puts a texture's pixel down, blended by its alpha layer when it has one
// (the frame pieces' edges are smoothed that way).
void putPixel(uint8_t* o, const Texture& t, size_t i) {
    const uint8_t* p = &t.rgba[i * 4];
    if (p[3] == 0)
        return;
    const int a = t.alpha.empty() ? 255 : t.alpha[i];
    for (int c = 0; c < 3; ++c)
        o[c] = uint8_t((p[c] * a + o[c] * (255 - a)) / 255);
}

} // namespace

bool GameUi::load(const Archives& archives, char side) {
    side_ = side;
    const std::string folder = side == 'K' ? "kln" : side == 'R' ? "rom" : "fed";
    const std::string base = "lbp/gui/" + folder + "/";
    loaded_ = loadPicture(archives, base + "corner.lbp", corner_) && loadPicture(archives, base + "border.lbp", border_);
    if (!loaded_)
        return false;
    loadPicture(archives, base + "wallpapr.lbp", wallpaper_);
    // The panels' icons, and the playbook's (its orders: pbattack, ...;
    // the scans' from the shared folder).
    for (const char* n : {"logo32", "shldicon", "pwricon", "popnicon", "closebut", "vcr", "pbattack", "pbbombrd",
                          "pbcaptre", "pbdefend", "pbmove", "pbprohre", "pbprotct", "pbreturn"}) {
        Texture t;
        if (loadPicture(archives, base + n + ".lbp", t))
            icons_[n] = std::move(t);
    }
    for (const char* n : {"science", "medical", "engine"}) {
        Texture t;
        if (loadPicture(archives, std::string("lbp/gui/") + n + ".lbp", t))
            icons_[n] = std::move(t);
    }
    // Crew portraits: f_c1, k_c1, r_c1 ... (the large ones, 64 x 60).
    const char prefix = side == 'K' ? 'k' : side == 'R' ? 'r' : 'f';
    for (int i = 1; i <= 40 && crew_.size() < std::size(kSections); ++i) {
        Texture t;
        if (loadPicture(archives, base + "ptrt/crew/lg/" + prefix + "_c" + std::to_string(i) + ".lbp", t))
            crew_.push_back(std::move(t));
    }
    if (side == 'R')
        for (const auto& [section, stem] : kRomulanFaces) {
            Texture t;
            if (loadPicture(archives, base + "ptrt/crew/lg/" + stem + ".lbp", t))
                faces_[section] = std::move(t);
        }
    // The officers' faces: from the side's own folder, else whichever
    // race's has the picture (f_c7 in the Klingon and Romulan lists, the
    // other races' player faces).
    for (const char* stem : kFaces[side == 'K' ? 1 : side == 'R' ? 2 : 0]) {
        Texture t;
        for (const char* f : {folder.c_str(), "fed", "kln", "rom"})
            if (loadPicture(archives, std::string("lbp/gui/") + f + "/ptrt/crew/sm/" + stem + ".lbp", t))
                break;
        officerFaces_.push_back(std::move(t));
        // The large one: the same name without "_s" (three are spelt
        // otherwise).
        std::string large = stem;
        if (large == "f_sarek_")
            large = "f_sarek";
        else if (large == "kirgat_s")
            large = "k_kirgat";
        else if (large == "r_nark_s")
            large = "r_narok";
        else if (large.size() > 2 && large.compare(large.size() - 2, 2, "_s") == 0)
            large.resize(large.size() - 2);
        Texture l;
        for (const char* f : {folder.c_str(), "fed", "kln", "rom"})
            if (loadPicture(archives, std::string("lbp/gui/") + f + "/ptrt/crew/lg/" + large + ".lbp", l))
                break;
        largeFaces_.push_back(std::move(l));
    }
    // The portraits of the side's buildings and craft, small ("fhub_s") and large ("fhub").
    for (const char* portraitDir : {"ptrt/build/sm/", "ptrt/craft/sm/", "ptrt/build/lg/", "ptrt/craft/lg/"}) {
        const std::string under = base + portraitDir;
        for (const auto& a : archives)
            for (const CatEntry& e : a->entries())
                if (e.name.rfind(under, 0) == 0 && e.name.size() > under.size() + 4 &&
                    e.name.compare(e.name.size() - 4, 4, ".lbp") == 0) {
                    const std::string stem = e.name.substr(under.size(), e.name.size() - under.size() - 4);
                    Texture t;
                    if (!objects_.count(stem) && loadPicture(archives, e.name, t))
                        objects_[stem] = std::move(t);
                }
    }
    for (const char* n : {"taubat", "metar", "hubrin", "orion"}) {
        Texture t;
        if (loadPicture(archives, base + "ptrt/crew/lg/" + n + ".lbp", t))
            natives_[n] = std::move(t);
    }
    // Fonts: fe8pt/kl8pt/rm8pt and fe5pt/kl5pt/rm5pt.
    fonts_ = loadGameFonts(archives);
    const std::string f = side == 'K' ? "kl" : side == 'R' ? "rm" : "fe";
    for (const GameFont& g : fonts_) {
        if (g.name == "fnt/" + f + "5pt")
            small_ = &g;
        if (g.name == "fnt/" + f + "8pt")
            large_ = &g;
    }
    if (side == 'K')
        title_ = {240, 150, 40}, text_ = {90, 170, 255};
    return true;
}

void GameUi::blit(Surface& s, const Texture& t, int dx, int dy, bool flipX, bool flipY, bool transpose) const {
    const int w = transpose ? t.height : t.width, h = transpose ? t.width : t.height;
    for (int y = 0; y < h; ++y) {
        if (dy + y < 0 || dy + y >= s.h)
            continue;
        uint8_t* row = s.row(dy + y);
        for (int x = 0; x < w; ++x) {
            if (dx + x < 0 || dx + x >= s.w)
                continue;
            int sx = transpose ? y : x, sy = transpose ? x : y;
            if (flipX)
                sx = t.width - 1 - sx;
            if (flipY)
                sy = t.height - 1 - sy;
            putPixel(row + (dx + x) * 4, t, size_t(sy) * size_t(t.width) + size_t(sx));
        }
    }
}

void GameUi::drawPicture(Surface& s, const Texture& t, int x0, int y0, int w, int h, bool clipped) const {
    if (clipped)
        w = std::min(w, t.width), h = std::min(h, t.height);
    if (t.rgba.empty() || w <= 0 || h <= 0)
        return;
    for (int y = 0; y < h; ++y) {
        if (y0 + y < 0 || y0 + y >= s.h)
            continue;
        uint8_t* row = s.row(y0 + y);
        const int sy = clipped ? y : y * t.height / h;
        for (int x = 0; x < w; ++x) {
            if (x0 + x < 0 || x0 + x >= s.w)
                continue;
            putPixel(row + (x0 + x) * 4, t, size_t(sy) * size_t(t.width) + size_t(clipped ? x : x * t.width / w));
        }
    }
}

void GameUi::drawPanel(Surface& s, int x, int y, int w, int h, std::string_view title) const {
    const int c = corner_.width, b = border_.height; // 12 and 12
    // The inside: the side's wallpaper tile, darkened.
    for (int yy = y + b; yy < y + h - b; ++yy) {
        if (yy < 0 || yy >= s.h)
            continue;
        uint8_t* row = s.row(yy);
        for (int xx = x + b; xx < x + w - b; ++xx) {
            if (xx < 0 || xx >= s.w)
                continue;
            uint8_t* o = row + xx * 4;
            if (!wallpaper_.rgba.empty()) {
                const uint8_t* p = &wallpaper_.rgba[(size_t((yy - y) % wallpaper_.height) * size_t(wallpaper_.width) +
                                                     size_t((xx - x) % wallpaper_.width)) *
                                                    4];
                o[0] = uint8_t(p[0] / 2), o[1] = uint8_t(p[1] / 2), o[2] = uint8_t(p[2] / 2);
            } else {
                o[0] = o[1] = o[2] = 0;
            }
        }
    }
    // The edges: the border tile along the top, flipped along the bottom,
    // turned along the sides; the corner piece in each corner.
    for (int xx = x + c; xx < x + w - c; xx += border_.width) {
        blit(s, border_, xx, y, false, false, false);
        blit(s, border_, xx, y + h - b, false, true, false);
    }
    for (int yy = y + c; yy < y + h - c; yy += border_.width) {
        blit(s, border_, x, yy, false, false, true);
        blit(s, border_, x + w - b, yy, false, true, true);
    }
    blit(s, corner_, x, y, false, false, false);
    blit(s, corner_, x + w - c, y, true, false, false);
    blit(s, corner_, x, y + h - c, false, true, false);
    blit(s, corner_, x + w - c, y + h - c, true, true, false);
    // The title, centred in the top bar.
    if (!title.empty()) {
        const int tw = large_ ? large_->textWidth(title) : 0;
        drawLarge(s, x + (w - tw) / 2, y + 1, title, title_);
    }
}

int GameUi::drawSmall(Surface& s, int x, int y, std::string_view text, Color c) const {
    return small_ ? small_->drawText(s, x, y, text, c) : x;
}

int GameUi::drawLarge(Surface& s, int x, int y, std::string_view text, Color c) const {
    return large_ ? large_->drawText(s, x, y, text, c) : drawSmall(s, x, y, text, c);
}

int GameUi::smallWidth(std::string_view text) const { return small_ ? small_->textWidth(text) : 0; }

int GameUi::smallHeight() const { return small_ ? small_->lineHeight : 9; }

std::vector<std::string> GameUi::wrapSmall(std::string_view text, int width) const {
    std::vector<std::string> lines;
    std::string line;
    size_t i = 0;
    while (i < text.size()) {
        const size_t space = std::min(text.find(' ', i), text.size());
        const std::string word(text.substr(i, space - i));
        const std::string next = line.empty() ? word : line + " " + word;
        if (!line.empty() && smallWidth(next) > width) {
            lines.push_back(line);
            line = word;
        } else {
            line = next;
        }
        i = space + 1;
    }
    if (!line.empty())
        lines.push_back(line);
    return lines;
}

const Texture* GameUi::portraitFor(std::string_view sender, std::string_view natives) const {
    for (const auto& [section, face] : faces_)
        if (sender.rfind(section, 0) == 0)
            return &face;
    for (size_t i = 0; i < std::size(kSections); ++i)
        if (sender.rfind(kSections[i], 0) == 0)
            return i < crew_.size() ? &crew_[i] : nullptr;
    if (sender.rfind("Incoming", 0) == 0) {
        const auto it = natives_.find(std::string(natives));
        if (it != natives_.end())
            return &it->second;
    }
    return icon("logo32");
}

const Texture* GameUi::officerFace(int index) const {
    if (index < 0 || size_t(index) >= officerFaces_.size() || officerFaces_[size_t(index)].rgba.empty())
        return nullptr;
    return &officerFaces_[size_t(index)];
}

const Texture* GameUi::messageFace(int number) const {
    if (number < 1 || size_t(number) > largeFaces_.size() || largeFaces_[size_t(number - 1)].rgba.empty())
        return nullptr;
    return &largeFaces_[size_t(number - 1)];
}

const Texture* GameUi::objectPortrait(std::string_view name) const {
    const auto it = objects_.find(name);
    return it == objects_.end() ? nullptr : &it->second;
}

const Texture* GameUi::icon(std::string_view name) const {
    const auto it = icons_.find(std::string(name));
    return it == icons_.end() ? nullptr : &it->second;
}

} // namespace reluva::newworlds
