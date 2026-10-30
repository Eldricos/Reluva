// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "engines/newworlds/fif.h"
#include "engines/newworlds/model_textures.h"
#include "engines/newworlds/texture.h"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace reluva::newworlds {

// The in-game interface's look, from a side's GUI archive (fedgui, kligui,
// romgui): its frame pieces, fonts, icons and portraits, and panels drawn
// with them where recordings of play show the game's (see
// docs/formats/newworlds/gui.md#the-in-game-interface). Not playable: the
// panels show what they're given.
class GameUi {
public:
    // small_ and large_ point into fonts_: a move keeps them (the vector's
    // buffer moves with it), a copy wouldn't, so there's none.
    GameUi() = default;
    GameUi(const GameUi&) = delete;
    GameUi& operator=(const GameUi&) = delete;
    GameUi(GameUi&&) = default;
    GameUi& operator=(GameUi&&) = default;

    // side: 'F', 'K' or 'R'. False if the side's GUI archive isn't there.
    bool load(const Archives& archives, char side);
    bool loaded() const { return loaded_; }

    // A framed panel with a title in its top bar; returns the inside's
    // top-left corner.
    void drawPanel(Surface& s, int x, int y, int w, int h, std::string_view title) const;
    // Text in the side's small or larger font; returns the x after it.
    int drawSmall(Surface& s, int x, int y, std::string_view text, Color c) const;
    int drawLarge(Surface& s, int x, int y, std::string_view text, Color c) const;
    int smallWidth(std::string_view text) const;
    int smallHeight() const;
    // Word-wraps text into lines of at most width pixels in the small font.
    std::vector<std::string> wrapSmall(std::string_view text, int width) const;
    // A picture, scaled to w x h, see-through where colour-keyed.
    // Scaled to w x h; clipped: at its own size, cut to w x h.
    void drawPicture(Surface& s, const Texture& t, int x, int y, int w, int h, bool clipped = false) const;

    // A message sender's portrait: a crew member for each section (the
    // Romulans' as their recordings show; the others' isn't known: the
    // side's first portraits in turn), the natives'
    // for an incoming message, the side's emblem otherwise.
    const Texture* portraitFor(std::string_view sender, std::string_view natives) const;
    // An officer's face (its small portrait), by the index the mission
    // gives it in the race's list of 40 (exe.md#crew-and-efficiency).
    const Texture* officerFace(int index) const;
    // A message's face: the large portrait for its officer number (the
    // list's entry number - 1; exe.md#the-tricorder), or null.
    const Texture* messageFace(int number) const;
    const Texture* icon(std::string_view name) const; // "logo32", "shldicon", "pbattack", "science", ...
    // A building's or vehicle's small portrait by its picture's name
    // ("fhub", "fcy", "fpv"; gui.md#pictures), or none.
    const Texture* objectPortrait(std::string_view name) const;

    // Colours: the panel titles, ordinary text, highlighted text.
    Color titleColor() const { return title_; }
    Color textColor() const { return text_; }
    Color brightColor() const { return bright_; }

private:
    void blit(Surface& s, const Texture& t, int dx, int dy, bool flipX, bool flipY, bool transpose) const;
    bool loaded_ = false;
    char side_ = 'F';
    Texture corner_, border_, wallpaper_;
    std::map<std::string, Texture> icons_;
    std::vector<Texture> crew_;
    std::vector<Texture> officerFaces_; // the race's 40 (empty where one is missing)
    std::vector<Texture> largeFaces_;   // the same 40, large (the messages')
    std::map<std::string, Texture> faces_; // by section, where known
    std::map<std::string, Texture, std::less<>> objects_; // the small building and craft portraits
    std::map<std::string, Texture> natives_;
    std::vector<GameFont> fonts_;
    const GameFont* small_ = nullptr;
    const GameFont* large_ = nullptr;
    Color title_{232, 170, 40}, text_{80, 160, 255}, bright_{100, 230, 230};
};

} // namespace reluva::newworlds
