// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// Tests for the New Worlds .cat reader against archives built here, following
// docs/formats/newworlds/cat.md, so no game data is needed.

#include "engines/newworlds/cat_archive.h"
#include "engines/newworlds/string_table.h"
#include "engines/newworlds/texture.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace reluva;
using namespace reluva::newworlds;

static int failures = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, \
                         __LINE__, #cond);                              \
            ++failures;                                                 \
        }                                                               \
    } while (0)

namespace {

struct Item {
    std::string path; // empty for containers
    uint16_t tag;
    std::string payload;
    std::vector<Item> children;
};

Item file(std::string path, uint16_t tag, std::string payload) { return {std::move(path), tag, std::move(payload), {}}; }
Item container(uint16_t tag, std::vector<Item> children) { return {"", tag, "", std::move(children)}; }

void put16(std::vector<uint8_t>& b, uint16_t v) {
    b.push_back(uint8_t(v));
    b.push_back(uint8_t(v >> 8));
}
void put32(std::vector<uint8_t>& b, uint32_t v) {
    for (int i = 0; i < 4; ++i)
        b.push_back(uint8_t(v >> (8 * i)));
}
void set32(std::vector<uint8_t>& b, size_t at, uint32_t v) {
    for (int i = 0; i < 4; ++i)
        b[at + size_t(i)] = uint8_t(v >> (8 * i));
}

uint32_t chunkLength(const Item& it) {
    if (it.children.empty() && !isContainerTag(it.tag))
        return 6 + uint32_t(it.payload.size());
    uint32_t n = 6;
    for (const Item& c : it.children)
        n += chunkLength(c);
    return n;
}

struct Builder {
    std::vector<uint8_t> table, data;
    uint32_t count = 0, index = 0;

    void add(const std::vector<Item>& items) {
        for (const Item& it : items) {
            const bool isContainer = isContainerTag(it.tag);
            // The real offset is data position + 12 (two chunk headers).
            put32(table, uint32_t(data.size()) + 12);
            put16(table, uint16_t(index++));
            table.push_back(0);
            table.push_back(0x15);
            put16(data, it.tag);
            if (isContainer) {
                put32(data, chunkLength(it) - 6);
                table.resize(table.size() + 256, 0xcd); // junk, as in real files
                ++count;
                add(it.children);
                ++index; // a folder uses an extra index after its children
            } else {
                put32(data, uint32_t(it.payload.size()));
                data.insert(data.end(), it.payload.begin(), it.payload.end());
                const size_t base = table.size();
                table.resize(base + 326, 0);
                std::memcpy(&table[base], it.path.data(), it.path.size());
                set32(table, base + 0x116, uint32_t(it.payload.size()));
                set32(table, base + 0x11e, 966294866); // 2000-08-14 23:14:26 UTC
                ++count;
            }
        }
    }

    std::vector<uint8_t> build() const {
        std::vector<uint8_t> body;
        put16(body, 5);
        put32(body, uint32_t(table.size() + 4));
        put32(body, count);
        body.insert(body.end(), table.begin(), table.end());
        body.insert(body.end(), data.begin(), data.end());
        std::vector<uint8_t> out;
        put16(out, 1);
        put32(out, uint32_t(body.size()));
        out.insert(out.end(), body.begin(), body.end());
        return out;
    }
};

std::filesystem::path writeArchive(const std::filesystem::path& dir, const std::vector<uint8_t>& bytes) {
    std::filesystem::create_directories(dir);
    auto p = dir / "test.cat";
    std::ofstream(p, std::ios::binary).write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
    return p;
}

const char* kPrefix = "c:\\prjs\\game\\stc\\bin\\rsc\\dat\\pc_win32\\";

std::vector<Item> sampleItems() {
    return {
        file(std::string(kPrefix) + "str\\eng\\miss\\rom01.stb", cattag::Strings, "strings!"),
        container(cattag::TextureSet,
                  {file(std::string(kPrefix) + "lbp\\txr\\rom\\rb_muzzl.mmb", cattag::Mmb, "8 bytes."),
                   file(std::string(kPrefix) + "lbp\\txr\\rom\\rb_muzzl.mp0", cattag::Texture, "pixels")}),
        container(cattag::FontSet,
                  {container(cattag::FontPart, {file(std::string(kPrefix) + "gui\\font.fif", cattag::Fif, "font")})}),
        file("c:\\prjs\\STNWSFX\\alert003.wav", cattag::Wave, "RIFF...."),
    };
}

void testRead(const std::filesystem::path& tmp) {
    Builder b;
    b.add(sampleItems());
    CatArchive cat;
    Error err = cat.open(writeArchive(tmp, b.build()));
    CHECK(!err);
    if (err) {
        std::fprintf(stderr, "  %s\n", err.message.c_str());
        return;
    }

    const auto& e = cat.entries();
    CHECK(e.size() == 8);
    CHECK(e[0].name == "str/eng/miss/rom01.stb" && e[0].tag == cattag::Strings && e[0].parent == -1);
    CHECK(e[0].modified == 966294866);
    CHECK(e[1].container && e[1].parent == -1);
    CHECK(e[2].name == "lbp/txr/rom/rb_muzzl.mmb" && e[2].parent == 1);
    CHECK(e[3].parent == 1);
    CHECK(e[4].container && e[5].container && e[5].parent == 4);
    CHECK(e[6].name == "gui/font.fif" && e[6].parent == 5);
    CHECK(e[7].name == "stnwsfx/alert003.wav" && e[7].parent == -1);

    const CatEntry* wav = cat.find("STNWSFX\\Alert003.WAV");
    CHECK(wav == &e[7]);
    std::vector<uint8_t> data;
    CHECK(wav && !cat.read(*wav, data));
    CHECK(std::string(data.begin(), data.end()) == "RIFF....");
    const CatEntry* mp0 = cat.find("lbp/txr/rom/rb_muzzl.mp0");
    CHECK(mp0 && !cat.read(*mp0, data) && std::string(data.begin(), data.end()) == "pixels");
    CHECK(cat.find("missing.stb") == nullptr);
}

void testRejectsCorruption(const std::filesystem::path& tmp) {
    Builder b;
    b.add(sampleItems());
    const std::vector<uint8_t> good = b.build();
    CatArchive cat;

    std::vector<uint8_t> bad = good;
    bad[0] = 2; // wrong root tag
    CHECK(cat.open(writeArchive(tmp, bad)));

    bad = good;
    bad.push_back(0); // root size no longer matches the file
    CHECK(cat.open(writeArchive(tmp, bad)));

    bad = good;
    bad[0x10] += 1; // first entry's offset off by one: breaks the chunk chain
    CHECK(cat.open(writeArchive(tmp, bad)));

    CHECK(!cat.open(writeArchive(tmp, good)));
}

void testNormalizeName() {
    CHECK(CatArchive::normalizeName("c:\\prjs\\game\\stc\\bin\\rsc\\dat\\pc_win32\\geo\\rom\\rb_mid.geo") ==
          "geo/rom/rb_mid.geo");
    CHECK(CatArchive::normalizeName("c:\\prjs\\STNWSFX\\alert003.wav") == "stnwsfx/alert003.wav");
    CHECK(CatArchive::normalizeName("") == "");
}

// Laid out like the start of rom01.stb: count, then (id, UTF-16 text, 0).
std::vector<uint8_t> stringTableBytes(const std::vector<std::pair<uint32_t, std::u16string>>& strings,
                                      uint32_t count) {
    std::vector<uint8_t> b;
    put32(b, count);
    for (const auto& [id, str] : strings) {
        put32(b, id);
        for (char16_t c : str)
            put16(b, uint16_t(c));
        put16(b, 0);
    }
    return b;
}

void testStringTable() {
    const std::vector<std::pair<uint32_t, std::u16string>> strings = {
        {1, u" 'Et Tu?'"},
        {23, u"Stardate 8988.4.  The type 6 world Sakuntula has significant deposits of dilithium."},
        {22, u"~STELLAR CARTOGRAPHY~"},
        {5, u""},
    };
    StringTable t;
    CHECK(!t.parse(stringTableBytes(strings, 4)));
    CHECK(t.strings().size() == 4);
    CHECK(t.find(1) && *t.find(1) == " 'Et Tu?'");
    CHECK(t.find(22) && *t.find(22) == "~STELLAR CARTOGRAPHY~");
    CHECK(t.find(5) && t.find(5)->empty());
    CHECK(t.find(99) == nullptr);

    CHECK(t.parse(stringTableBytes(strings, 5)));    // count too high
    CHECK(t.parse(stringTableBytes(strings, 3)));    // bytes left over
    auto cut = stringTableBytes(strings, 4);
    cut.resize(cut.size() - 1);
    CHECK(t.parse(cut));                             // last string unterminated
}

// Format 6: u32 6, u32 palette count, palette (blue, green, red, 0),
// u32 0, u32 width, u32 height, indices, alpha.
// sizeStored false: the u32 after the palette is 1 and width/height are omitted.
std::vector<uint8_t> textureBytes(uint32_t w, uint32_t h, uint32_t format = 6, bool sizeStored = true) {
    std::vector<uint8_t> b;
    put32(b, format);
    put32(b, 2);
    b.insert(b.end(), {0x10, 0x20, 0xf0, 0}); // index 0: red 0xf0, green 0x20, blue 0x10
    b.insert(b.end(), {0xff, 0x00, 0x00, 0}); // index 1: pure blue
    if (sizeStored) {
        put32(b, 0);
        put32(b, w);
        put32(b, h);
    } else {
        put32(b, 1);
    }
    for (uint32_t i = 0; i < w * h; ++i)
        b.push_back(uint8_t(i % 2));          // indices alternate 0, 1
    if (format == 6)
        for (uint32_t i = 0; i < w * h; ++i)
            b.push_back(uint8_t(i * 10));     // alpha
    return b;
}

void testTexture() {
    Texture t;
    CHECK(!decodeTexture(textureBytes(4, 2), t));
    CHECK(t.width == 4 && t.height == 2 && t.rgba.size() == 32);
    CHECK(t.rgba[0] == 0xf0 && t.rgba[1] == 0x20 && t.rgba[2] == 0x10 && t.rgba[3] == 0); // pixel 0
    CHECK(t.rgba[4] == 0x00 && t.rgba[5] == 0x00 && t.rgba[6] == 0xff && t.rgba[7] == 10); // pixel 1
    CHECK(t.rgba[31] == 70);

    // Format 1: one layer, opaque.
    CHECK(!decodeTexture(textureBytes(4, 2, 1), t));
    CHECK(t.format == 1 && t.width == 4 && t.height == 2 && !t.sizeGuessed);
    CHECK(t.rgba[4] == 0x00 && t.rgba[6] == 0xff && t.rgba[7] == 255);

    // Size not stored: inferred as square, or 2:1 when not square.
    CHECK(!decodeTexture(textureBytes(4, 4, 1, false), t));
    CHECK(t.width == 4 && t.height == 4 && t.sizeGuessed);
    CHECK(!decodeTexture(textureBytes(8, 4, 6, false), t));
    CHECK(t.width == 8 && t.height == 4 && t.sizeGuessed && t.rgba[7] == 10);
    CHECK(decodeTexture(textureBytes(5, 3, 1, false), t)); // 15 pixels: no shape to infer

    CHECK(decodeTexture(textureBytes(4, 2, 7), t)); // unknown format
    auto bad = textureBytes(4, 2);
    bad.pop_back();
    CHECK(decodeTexture(bad, t));                  // wrong size
    CHECK(decodeTexture(textureBytes(0, 2), t));   // zero width
}

} // namespace

int main() {
    auto tmp = std::filesystem::temp_directory_path() / "reluva_cat_tests";
    testNormalizeName();
    testRead(tmp);
    testRejectsCorruption(tmp);
    testStringTable();
    testTexture();
    std::filesystem::remove_all(tmp);
    if (failures)
        std::fprintf(stderr, "%d check(s) failed\n", failures);
    else
        std::printf("All tests passed\n");
    return failures ? 1 : 0;
}
