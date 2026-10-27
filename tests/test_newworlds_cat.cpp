// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// Tests for the New Worlds .cat reader against archives built here, following
// docs/formats/newworlds/cat.md, so no game data is needed.

#include "engines/newworlds/ai_sides.h"
#include "engines/newworlds/arf.h"
#include "engines/newworlds/briefing.h"
#include "engines/newworlds/briefing_data.h"
#include "engines/newworlds/cat_archive.h"
#include "engines/newworlds/colony.h"
#include "engines/newworlds/economy.h"
#include "engines/newworlds/field_work.h"
#include "engines/newworlds/fog.h"
#include "engines/newworlds/repairs.h"
#include "engines/newworlds/research.h"
#include "engines/newworlds/script_effects.h"
#include "engines/newworlds/fif.h"
#include "engines/newworlds/string_table.h"
#include "engines/newworlds/texture.h"
#include "engines/newworlds/geo.h"
#include "engines/newworlds/gmf.h"
#include "engines/newworlds/levels.h"
#include "engines/newworlds/mdl.h"
#include "engines/newworlds/mission_script.h"
#include "engines/newworlds/mission_sim.h"
#include "engines/newworlds/placement.h"
#include "engines/newworlds/model_pose.h"
#include "engines/newworlds/model_textures.h"
#include "engines/newworlds/rasterizer.h"
#include "engines/newworlds/star_system.h"
#include "engines/newworlds/world.h"

#include <algorithm>
#include <numeric>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>
#include <map>
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
// keyIndex >= 0: colour-keyed layout (u32 1, u32 key, width, height).
std::vector<uint8_t> textureBytes(uint32_t w, uint32_t h, uint32_t format = 6, int keyIndex = -1) {
    std::vector<uint8_t> b;
    put32(b, format);
    put32(b, 2);
    b.insert(b.end(), {0x10, 0x20, 0xf0, 0}); // index 0: red 0xf0, green 0x20, blue 0x10
    b.insert(b.end(), {0xff, 0x00, 0x00, 0}); // index 1: pure blue
    if (keyIndex >= 0) {
        put32(b, 1);
        put32(b, uint32_t(keyIndex));
    } else {
        put32(b, 0);
    }
    put32(b, w);
    put32(b, h);
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
    CHECK(t.rgba[0] == 0xf0 && t.rgba[1] == 0x20 && t.rgba[2] == 0x10 && t.rgba[3] == 255); // pixel 0
    CHECK(t.rgba[4] == 0x00 && t.rgba[5] == 0x00 && t.rgba[6] == 0xff && t.rgba[7] == 255); // pixel 1
    CHECK(t.alpha.size() == 8 && t.alpha[0] == 0 && t.alpha[1] == 10 && t.alpha[7] == 70); // separate layer

    // Format 1: one layer, opaque.
    CHECK(!decodeTexture(textureBytes(4, 2, 1), t));
    CHECK(t.format == 1 && t.width == 4 && t.height == 2 && !t.colourKeyed);
    CHECK(t.rgba[4] == 0x00 && t.rgba[6] == 0xff && t.rgba[7] == 255 && t.alpha.empty());

    // Colour key: pixels using the key index become transparent.
    CHECK(!decodeTexture(textureBytes(5, 3, 1, 1), t));
    CHECK(t.colourKeyed && t.keyIndex == 1 && t.width == 5 && t.height == 3);
    CHECK(t.rgba[3] == 255 && t.rgba[7] == 0);            // index 0 opaque, index 1 keyed out
    CHECK(!decodeTexture(textureBytes(4, 2, 6, 0), t));
    CHECK(t.rgba[3] == 0 && t.rgba[7] == 255 && t.alpha[1] == 10); // key is separate from the alpha layer

    CHECK(decodeTexture(textureBytes(4, 2, 7), t)); // unknown format
    auto bad = textureBytes(4, 2);
    bad.pop_back();
    CHECK(decodeTexture(bad, t));                  // wrong size
    CHECK(decodeTexture(textureBytes(0, 2), t));   // zero width
}

void putF(std::vector<uint8_t>& b, float f) {
    uint32_t v;
    std::memcpy(&v, &f, 4);
    put32(b, v);
}

// A two-sided triangle laid out as in docs/formats/newworlds/geo.md: face 0
// in the file header, face 1 its back, one edge group (or none, as in
// terrain tiles).
// Two triangles back to back. With twoParts the back is part 1, with its
// own copy of the vertices.
std::vector<uint8_t> triangleGeo(bool withEdges = true, bool oddAttachment = false, bool twoParts = false) {
    const float pos[3][2] = {{0, 0}, {100, 0}, {0, 200}};
    const float uv[3][2] = {{0, 0}, {65536, 0}, {0, 65536}};
    const uint32_t parts = twoParts ? 2 : 1;
    std::vector<uint8_t> b;
    auto corner = [&](uint32_t v) {
        put32(b, v);
        putF(b, uv[v][0]);
        putF(b, uv[v][1]);
        for (int i = 0; i < 5; ++i)
            put32(b, 0);
    };
    put32(b, 0); put32(b, 2); put32(b, 3); put32(b, 0); // header = face 0: 2 faces, 3 corners
    corner(0); corner(1); corner(2);
    put32(b, 0); put32(b, 0); put32(b, 3); put32(b, parts - 1); // face 1, the back
    corner(2); corner(1); corner(0);
    put32(b, parts - 1); put32(b, 1); put32(b, parts); put32(b, 0); // bounds, no attachments
    putF(b, 0); putF(b, 0); putF(b, 224);
    put32(b, 1); put32(b, 0); put32(b, 0);
    if (oddAttachment) // odd-length data puts the part data off 4-byte alignment
        b.insert(b.end(), {0x1c, 0x00, 0x0c});
    put32(b, 0); put32(b, 1); put32(b, parts);          // part data
    for (uint32_t part = 0; part < parts; ++part) {
        if (part > 0)
            put32(b, 0);                                // end of the previous part
        put32(b, part); put32(b, 3);                    // part 1 starts with 1, as in ptank_l
        for (auto& p : pos) {
            putF(b, p[0]); putF(b, p[1]); putF(b, 0);
        }
        put32(b, 1); put32(b, 2);                       // two normals
        putF(b, 0); putF(b, 0); putF(b, 1);
        putF(b, 0); putF(b, 0); putF(b, -1);
        if (withEdges && !twoParts) {
            put32(b, 1); put32(b, 1); put32(b, 3);      // one group of three edges
            const uint32_t edges[3][2] = {{0, 1}, {1, 2}, {2, 0}};
            for (auto& e : edges) {
                put32(b, 0); put32(b, e[0]); put32(b, e[1]); put32(b, 0); put32(b, 1);
            }
        } else {
            put32(b, 0);
        }
    }
    return b;
}

void testGeo() {
    GeoModel m;
    Error err = parseGeo(triangleGeo(), m);
    CHECK(!err);
    if (err) {
        std::fprintf(stderr, "  %s\n", err.message.c_str());
        return;
    }
    CHECK(m.parts.size() == 1 && m.parts[0].vertices.size() == 3 && m.parts[0].vertices[2].y == 200.0f);
    CHECK(m.faces.size() == 2 && m.faces[0].corners.size() == 3 && m.faces[1].corners[0].vertex == 2);
    CHECK(m.faces[0].corners[1].vertex == 1 && m.faces[0].corners[1].u == 1.0f);
    const auto& part = m.parts[0];
    CHECK(part.edges.size() == 3 && part.edges[1].v1 == 2 && part.edges[1].faceB == 1);
    CHECK(part.normalLists.size() == 1 && part.normalLists[0].size() == 2 && m.radius == 224.0f);

    CHECK(!parseGeo(triangleGeo(false), m) && m.parts[0].edges.empty()); // terrain-style: no edges

    err = parseGeo(triangleGeo(true, true), m);
    CHECK(!err && m.parts[0].vertices.size() == 3 && m.parts[0].vertices[1].x == 100.0f);

    err = parseGeo(triangleGeo(false, false, true), m);
    CHECK(!err && m.parts.size() == 2 && m.faces[1].part == 1 && m.parts[1].vertices.size() == 3 &&
          m.parts[1].node == 1);

    auto bad = triangleGeo();
    bad[bad.size() - 16] = 7; // last edge now starts at vertex 7
    CHECK(parseGeo(bad, m));
}

// A 2 x 1 map: 2 tiles, 1 block, both cells using it.
std::vector<uint8_t> tinyGmf() {
    std::vector<uint8_t> b;
    auto put16 = [&](uint16_t v) { b.push_back(uint8_t(v)); b.push_back(uint8_t(v >> 8)); };
    put16(2); put16(2); put16(1); put16(2); put16(1);
    for (int i = 0; i < 2; ++i) { // type 21, contents unknown
        put16(21); put16(4); put16(0); put32(b, 0);
    }
    for (uint32_t i = 0; i < 2; ++i) { // type 20: number, 0, model, flags, u16, 8 x 8 grid
        put16(20); put16(18 + 64); put16(0);
        put32(b, i); put32(b, 0); put32(b, 6 + i); put32(b, 0x18000 + 0x4000 * i); put16(0);
        for (int c = 0; c < 64; ++c)
            b.push_back(uint8_t(c == 0 ? 2 : 11));
    }
    put16(19); put16(22); put16(0); // block: 6 bytes, then four tile numbers
    for (int i = 0; i < 6; ++i)
        b.push_back(0);
    for (uint32_t t : {0u, 1u, 1u, 0u})
        put32(b, t);
    put32(b, 3); put32(b, 0x00a1b277); put32(b, 0xffffffffu); put32(b, 0xffffffffu); // lists
    for (uint32_t cell = 0; cell < 2; ++cell) {                                     // grid
        put16(0); put16(50); put16(0); put32(b, cell * 2); // one object, then none
    }
    // The colour table: magenta, then one colour (B, G, R), then the end.
    const uint8_t palette[8] = {0xff, 0x00, 0xff, 0x00, 0xf9, 0xc6, 0x42, 0x00};
    b.insert(b.end(), palette, palette + 8);
    b.insert(b.end(), 1024 - 8, 0);
    return b;
}

void testGmf() {
    GmfMap m;
    Error err = parseGmf(tinyGmf(), m);
    CHECK(!err);
    if (err) {
        std::fprintf(stderr, "  %s\n", err.message.c_str());
        return;
    }
    CHECK(m.width == 2 && m.height == 1 && m.tiles.size() == 2 && m.blocks.size() == 1 && m.cells.size() == 2);
    CHECK(m.tiles[1].model == 7 && m.tiles[0].layers[0] == 2 && m.tiles[0].layers[1] == 11);
    CHECK(m.tiles[0].rotation == 2 && m.tiles[1].rotation == 3);
    CHECK(m.blocks[0][2] == 1 && m.cells[1].block == 0 && m.cells[1].value == 50);
    CHECK(m.scenery.size() == 1 && m.scenery[0].bx == 0 && m.scenery[0].x == 7 && m.scenery[0].y == 7);
    CHECK(m.scenery[0].type == 1 && m.scenery[0].turn == 4 && m.scenery[0].variant == 2 && m.scenery[0].extra == 50);
    CHECK(m.palette.size() == 2 && m.palette[1][0] == 0x42 && m.palette[1][1] == 0xc6 && m.palette[1][2] == 0xf9);
    // Model numbers: the planet's fixed tile list first, then the rest, lod last.
    const std::vector<std::string> geos = {"geo/map/d/brid_01.geo", "geo/map/d/d_01.geo", "geo/map/d/lod/d_01.geo",
                                           "geo/map/d/needle_0.geo", "geo/map/d/d_26.geo", "geo/map/d/ramp.geo"};
    const std::vector<size_t> order = tileModelOrder(geos);
    // brid_01 at 27, ramp 29, d_26 38, needle_0 51, then the lod model after the 55 places.
    CHECK(order.size() == 56 && order[0] == 1 && order[27] == 0 && order[24] == kNoModel && order[29] == 5 &&
          order[38] == 4 && order[51] == 3 && order[55] == 2);
    auto bad = tinyGmf();
    bad[bad.size() - 1024 - 10] = 5; // the last cell uses block 5 of 1
    CHECK(parseGmf(bad, m));
}

// A metamodel: title, four name lists (the first with leftovers after it,
// as when its count is larger than its entries), then u32 0 and the states.
void testMdl() {
    std::vector<uint8_t> b;
    auto name = [&](const char* n, uint32_t v) {
        char buf[32] = {};
        std::snprintf(buf, sizeof buf, "%s", n);
        b.insert(b.end(), buf, buf + 32);
        put32(b, v);
    };
    char title[32] = "testmetamodel";
    b.insert(b.end(), title, title + 32);
    put32(b, 3); name("main", 0); name("turret_1", 1); name("terminate", 0xffffffffu);
    put32(b, 0x150); put32(b, 0x009a0300); // leftovers
    put32(b, 1); name("gun_muzzle_1", 0); name("terminate", 0xffffffffu);
    put32(b, 0); name("terminate", 0xffffffffu);
    put32(b, 0); name("terminate", 0xffffffffu);
    put32(b, 0); put32(b, 2); name("normal", 0); name("low detail", 1); name("terminate", 0xffffffffu);
    MdlModel m;
    Error err = parseMdl(b, m);
    CHECK(!err);
    CHECK(m.title == "testmetamodel" && m.parts.size() == 2 && m.parts[1].name == "turret_1" && m.parts[1].value == 1);
    CHECK(m.points.size() == 1 && m.points[0].name == "gun_muzzle_1" && m.places.empty() && m.animated.empty());
    CHECK(m.states.size() == 2 && m.states[1] == "low detail");
}

// An animation frame: u32 count, then per part 9 f32 rotation, f32 scale and
// a position chunk (u16 0x1c, u32 12, 3 f32).
void testArf() {
    std::vector<uint8_t> b;
    auto putf = [&](float f) { uint32_t v; std::memcpy(&v, &f, 4); put32(b, v); };
    put32(b, 2);
    for (int part = 0; part < 2; ++part) {
        for (float f : {0.f, 0.f, -1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f})
            putf(f);
        b.push_back(0x1c); b.push_back(0);
        put32(b, 12);
        putf(float(part)); putf(2); putf(52);
    }
    std::vector<PartTransform> t;
    CHECK(!parseArf(b, t));
    CHECK(t.size() == 2 && t[1].position.x == 1 && t[1].position.z == 52 && t[0].rotation[2] == -1 && t[0].scale == 1);
    b.pop_back();
    CHECK(bool(parseArf(b, t))); // a byte short
}


void testFif() {
    // Three glyphs from 32: space (2 wide), '!' (1 wide), '"' (none).
    std::vector<uint8_t> b(std::begin("FONTPROP"), std::end("FONTPROP") - 1);
    b.push_back(32); b.push_back(0); b.push_back(3); b.push_back(0);
    put32(b, 2); put32(b, 4); put32(b, 0);
    for (uint32_t v : {0u, 0u, 2u, 4u, 2u, 0u, 1u, 4u, 0u, 0u, 0u, 0u})
        put32(b, v);
    GameFont f;
    CHECK(!parseFif(b, f));
    CHECK(f.first == 32 && f.lineHeight == 4 && f.size == 2 && f.glyphs.size() == 3);
    CHECK(f.glyphs[1].x == 2 && f.glyphs[1].width == 1 && f.glyphs[2].height == 0);
    // No glyph for '"' or U+2603: drawn as '?', which this font lacks too.
    CHECK(f.glyph('!') == &f.glyphs[1] && f.glyph('"') == nullptr && f.glyph(0x2603) == nullptr);
    CHECK(f.textWidth(" ! !") == 2 + 1 + 2 + 1);
    // '!' is a full-coverage column: drawn red over black at x = 2 after the space.
    f.imageWidth = 3, f.imageHeight = 4;
    f.coverage = {0, 0, 255, 0, 0, 255, 0, 0, 128, 0, 0, 0};
    Surface s;
    s.create(8, 4, PixelFormat::RGBA32);
    CHECK(f.drawText(s, 0, 0, " !", {200, 0, 0}) == 3);
    CHECK(s.row(0)[2 * 4] == 200 && s.row(2)[2 * 4] == 100 && s.row(3)[2 * 4] == 0 && s.row(0)[1 * 4] == 0);
    b.push_back(0);
    CHECK(bool(parseFif(b, f))); // a byte too many
}

void testLevels() {
    std::vector<Level> l;
    CHECK(!parseLevelList("// format comment\r\nF1,2,Power Corrupts\r\n// J1,6,Defiant\r\n"
                          "J1,3,Into the Arena\r\n\r\n//C9,2,(Bonus) \"Slaver's Run\"  \r\n",
                          l));
    CHECK(l.size() == 4 && l[0].shortName == "F1" && l[0].maxPlayers == 2 && l[0].title == "Power Corrupts");
    CHECK(l[1].commentedOut && l[1].title == "Defiant" && !l[2].commentedOut);
    CHECK(l[3].commentedOut && l[3].title == "(Bonus) \"Slaver's Run\"");
    CHECK(levelForMap(l, "map/f1.gmf") == &l[0] && levelForMap(l, "MAP\\J1.GMF") == &l[2]);
    CHECK(levelForMap(l, "map/kurt64.gmf") == nullptr);
    CHECK(bool(parseLevelList("F1,two,Power Corrupts\n", l)));

    CHECK(missionSides("F1") == std::vector<char>{'F'} && missionSides("K12") == std::vector<char>{'K'});
    CHECK(missionSides("J4").size() == 3 && missionSides("C1") == std::vector<char>{'F'});
    // Winning J9 wins a campaign (its race's movie), not F14, K14 or R14.
    CHECK(isCampaignFinale("J9") && !isCampaignFinale("F14") && !isCampaignFinale("R12"));
    CHECK(missionTableName("F1", 'F') == "str/eng/miss/fed01.stb");
    CHECK(missionTableName("K12", 'K') == "str/eng/miss/klg12.stb");
    CHECK(missionTableName("R2", 'R') == "str/eng/miss/rom02.stb");
    CHECK(missionTableName("J1", 'K') == "str/eng/miss/jnt01klg.stb");
    CHECK(missionTableName("C5", 'F') == "str/eng/miss/cvr03.stb");
    CHECK(missionTableName("Test", 'F') == "str/eng/miss/tst.stb" && missionTableName("E1", 'F').empty());
    CHECK(missionMapNames("F1") == std::vector<std::string>{"map/f1.gmf"});
    CHECK((missionMapNames("R14") == std::vector<std::string>{"map/r14.gmf", "map/r12.gmf"}));
    CHECK(isMissionClip("F1005", "F1", 'F') && isMissionClip("f1015", "F1", 'F'));
    CHECK(!isMissionClip("F12005", "F1", 'F') && isMissionClip("F12005", "F12", 'F'));
    CHECK(isMissionClip("K12009A", "K12", 'K') && !isMissionClip("K12009", "K1", 'K'));
    CHECK(isMissionClip("J1R011", "J1", 'R') && !isMissionClip("J1R011", "J1", 'F'));
    CHECK(planetTheme("ICEWRLD.CAT") == "IceStart" && planetTheme("fedgui.cat").empty());
}

// A string table's bytes: u32 count, then u32 id and UTF-16LE text each.
std::vector<uint8_t> makeTable(const std::vector<std::pair<uint32_t, std::string>>& strings) {
    std::vector<uint8_t> b;
    auto u32 = [&](uint32_t v) {
        for (int i = 0; i < 4; ++i)
            b.push_back(uint8_t(v >> (8 * i)));
    };
    u32(uint32_t(strings.size()));
    for (const auto& [id, text] : strings) {
        u32(id);
        for (char c : text)
            b.push_back(uint8_t(c)), b.push_back(0);
        b.push_back(0), b.push_back(0);
    }
    return b;
}

void testBriefing() {
    CHECK(clipBefore("K12009", "K12009A") && clipBefore("K12009A", "K12010") && !clipBefore("K12010", "K12009A"));
    StringTable table;
    CHECK(!table.parse(makeTable({{1, "'Test.'"},
                                  {2, "~INCOMING TRANSMISSION~Test"},
                                  {3, "~STELLAR CARTOGRAPHY~Systems"},
                                  {4, "~PLANETARY DATA~A planet"},
                                  {5, "~SPEECH~ "},
                                  {6, "~OBJECTIVE~Do it"},
                                  {7, "~MISSION PASS~Done"}})));
    // A mission the executable has no table for: the general rule.
    auto steps = briefingScript("Z1", 'F', table, {"Z1007", "Z1005", "Z1006", "Z1008"});
    CHECK(steps.size() == 4 && steps[0].clip == "Z1005" && steps[0].page == 3 &&
          steps[0].view == BriefingView::Galaxy);
    CHECK(steps[1].page == 4 && steps[1].view == BriefingView::StarSystem);
    CHECK(steps[2].clip == "Z1007" && steps[2].page == 6 && steps[2].view == BriefingView::Ground);
    CHECK(steps[3].clip == "Z1008" && steps[3].page == 0 && steps[3].view == BriefingView::Emblem);
    CHECK(!hasRecordedBriefing("Z1", 'F') && hasRecordedBriefing("R8", 'R') && hasRecordedBriefing("k12", 'K'));
    // The executable's tables: K12's lettered clip as its own step; a
    // joint mission's clips carry the side's letter; each opens with the
    // silent transmission page; each step its minimum (script seconds).
    steps = briefingScript("K12", 'K', table, {});
    CHECK(steps.size() == 14 && steps[8].clip == "K12009A" && steps[9].clip == "K12010");
    steps = briefingScript("J8", 'K', table, {});
    CHECK(steps[1].clip.empty() && steps[1].page == 7 && steps[3].clip == "J8K005");
    steps = briefingScript("F1", 'F', table, {});
    CHECK(steps[0].clip.empty() && steps[0].page == 3 && steps[4].view == BriefingView::Galaxy &&
          std::abs(steps[4].minimum - 8) < 1e-9);
    CHECK(steps[8].clip == "F1009" && steps[8].siteA == 438 && steps[8].siteB == 218); // the colony hub
    CHECK(steps[10].view == BriefingView::Model && steps[10].model == "geo/bad/tt.geo"); // the temple
    // F1012's relief view draws its ground in grid lines (scheme 2); the rest the ring.
    CHECK(steps[11].clip == "F1012" && steps[11].scheme == 2 && steps[10].scheme == 0);
    // F1's shuttle scene: the ground steps after the held star system
    // follow the briefing shuttle along its path.
    CHECK(steps[6].scene >= 0 && steps[7].scene == steps[6].scene && briefingScene(steps[6].scene) &&
          briefingScene(steps[6].scene)->type == 6 && briefingScene(steps[6].scene)->ways == 2);
    // The officer numbers the mission's mail carries, by section (a
    // message's face is the portrait at that number less one).
    CHECK(messageOfficer("F1", 'F', "science") == 5 && messageOfficer("f1", 'F', "engineering") == 3 &&
          messageOfficer("F1", 'F', "nobody") == 0);
    CHECK(missionSides("C10") == std::vector<char>{'K'} && missionSides("C11") == std::vector<char>{'R'});
}

void testMissionScripts() {
    std::vector<MissionScript> s;
    CHECK(!parseMissionScripts("mission T F\nsite base 8 16\nobjective 5\nobjective 6 hidden\nlose 9 destroyed hub\n"
                               "on built-all yard*2 gen|adv\n  say command \"Built at {site:base}.\"\n  complete 5\n"
                               "  reveal 6\n  timer t 10\non timer t\n  complete 6\non all-completed\n  win\n",
                               s));
    CHECK(s.size() == 1 && s[0].sites.at("base") == std::make_pair(8, 16) && s[0].objectives.size() == 2);
    MissionRun run(s[0], nullptr);
    CHECK(run.objectiveState(6) == MissionRun::State::Hidden);
    CHECK(run.fire({"built", {"yard"}}).empty() && run.fire({"built", {"adv"}}).empty());
    auto out = run.fire({"built", {"yard"}}); // the second yard: all built
    CHECK(out.size() == 3 && out[0].text == "Built at <008:016>." && out[0].sender == "Command");
    CHECK(out[1].kind == MissionRun::Output::Completed && out[2].kind == MissionRun::Output::Revealed);
    // (Script seconds: 0.8 s each.)
    CHECK(!run.finished() && run.nextTimer() == 8);
    out = run.advance(8);
    CHECK(run.won() && out.back().kind == MissionRun::Output::Won);
    MissionRun lost(s[0], nullptr);
    CHECK(lost.fire({"destroyed", {"hub"}}).back().kind == MissionRun::Output::Lost && lost.finished());
    CHECK(bool(parseMissionScripts("mission T F\non start\n  frobnicate\n", s)));
    // Signals, counted; a random choice; actions for the game to carry out.
    // Handlers switched on and off by label; a fail that ends the mission
    // after its delay.
    CHECK(!parseMissionScripts("mission T F\nobjective 1\non start\n  arm watch\non scanned science temple as watch dormant\n"
                               "  complete 1\n  disarm guard\nfail 9 destroyed temple as guard\non all-completed\n  win\n",
                               s));
    MissionRun sw(s[0], nullptr);
    CHECK(sw.fire({"scanned", {"science", "temple"}}).empty()); // still dormant
    sw.fire({"start", {}});
    sw.fire({"scanned", {"science", "temple"}});
    CHECK(sw.won());
    CHECK(!parseMissionScripts("mission T F\nobjective 1\nfail 9 destroyed temple\non destroyed temple\n"
                               "  say command \"Gone.\"\n",
                               s));
    MissionRun fl(s[0], nullptr);
    out = fl.fire({"destroyed", {"temple"}});
    CHECK(fl.failing() && !fl.finished() && out.size() == 2 && out[0].kind == MissionRun::Output::Failing &&
          out[0].objective == 9 && out[1].text == "Gone.");
    CHECK(fl.advance(15).empty() && fl.advance(1).back().kind == MissionRun::Output::Lost && fl.finished() && !fl.won());
    // A monitor armed late sees the state as it is.
    CHECK(!parseMissionScripts("mission T F\nobjective 1\nobjective 2\non start\n  arm rich\n  arm yard\n"
                               "on stock nitrium 800 as rich dormant\n  complete 1\n"
                               "on built construction-yard as yard dormant\n  complete 2\n", s));
    MissionRun late(s[0], nullptr);
    late.fire({"stock", {"nitrium", "900"}});
    late.fire({"built", {"construction-yard"}});
    CHECK(late.objectiveState(1) == MissionRun::State::Incomplete);
    late.fire({"start", {}});
    CHECK(late.objectiveState(1) == MissionRun::State::Complete && late.objectiveState(2) == MissionRun::State::Complete);
    // A walkthrough step waits on the timers that arm its handler, and no
    // longer: nothing waits for an event no off handler wants.
    CHECK(!parseMissionScripts("mission T F\nobjective 1\non start\n  timer wake 20\n  timer later 100\n"
                               "on timer wake\n  arm scan\non scanned science site as scan dormant\n  complete 1\n", s));
    MissionRun walker(s[0], nullptr);
    double waited = -1;
    walker.walk({"start", {}}, waited);
    CHECK(waited == 0 && walker.waitsOnTimers({"scanned", {"science", "site"}}) && !walker.waitsOnTimers({"built", {"x"}}));
    walker.walk({"scanned", {"science", "site"}}, waited);
    CHECK(waited == 16 && walker.objectiveState(1) == MissionRun::State::Complete && walker.nextTimer() == 64);
    CHECK(bool(parseMissionScripts("mission T F\non start\n  arm nothing\n", s))); // no such label
    CHECK(bool(parseMissionScripts("mission T F\non start dormant\n  win\n", s)));  // dormant needs a label
    CHECK(!parseMissionScripts("mission T F\nobjective 5\non start\n  signal hit\n  signal hit\n  timer t 5\n  stop t\n"
                               "  ai K assault-1\n  hostile taubat\n  give dilithium 100\n  random a b\n"
                               "on signal hit*2\n  complete 5\non all-completed\n  win\n",
                               s));
    MissionRun sig(s[0], nullptr);
    out = sig.fire({"start", {}});
    CHECK(sig.won() && sig.nextTimer() < 0);
    CHECK(std::count_if(out.begin(), out.end(), [](const auto& o) { return o.kind == MissionRun::Output::Ai && o.sender == "K"; }) == 1);
    CHECK(std::count_if(out.begin(), out.end(), [](const auto& o) { return o.kind == MissionRun::Output::Stance; }) == 1);
    CHECK(bool(parseMissionScripts("  say command \"x\"\n", s)));

    // Reluva's scripts all read, and each recorded mission's walkthrough
    // plays through without losing; those whose recordings reached the end
    // win.
    CHECK(missionScripts().size() >= 31);
    const MissionScript* generic = findMissionScript("generic", 'F');
    CHECK(generic != nullptr);
    for (const MissionScript& m : missionScripts()) {
        if (&m == generic)
            continue;
        MissionRun r(m, generic);
        for (const ScriptEvent& e : r.walkthrough()) {
            double waited = 0;
            r.walk(e, waited); // timers first while a timer has yet to arm its handler
        }
        // Time passes (two hours at most: some timers repeat for ever).
        for (double t = 0; !r.finished() && r.nextTimer() >= 0 && t < 7200; t += r.nextTimer())
            r.advance(r.nextTimer());
        const bool endsInWin = std::any_of(m.handlers.begin(), m.handlers.end(), [](const auto& h) {
            return std::any_of(h.actions.begin(), h.actions.end(), [](const auto& a) { return a.verb == "win"; });
        });
        if (r.finished() && !r.won())
            std::fprintf(stderr, "%s (%c) lost in its walkthrough\n", m.mission.c_str(), m.side);
        if (endsInWin && !r.won())
            std::fprintf(stderr, "%s (%c) didn't win its walkthrough\n", m.mission.c_str(), m.side);
        CHECK(!(r.finished() && !r.won()));
        CHECK(!endsInWin || r.won());
    }
    // And each armed fail lost: in a fresh run, its event (as the
    // walkthrough would give it) loses the mission within a minute.
    for (const MissionScript& m : missionScripts()) {
        if (&m == generic)
            continue;
        for (const MissionScript::Handler& h : m.handlers) {
            const bool fails = std::any_of(h.actions.begin(), h.actions.end(),
                                           [](const auto& a) { return a.verb == "lose" || a.verb == "fail"; });
            if (!fails || h.dormant || h.event.kind == "timer" || h.event.kind == "signal" || h.event.kind == "built-all")
                continue;
            MissionRun r(m, generic);
            ScriptEvent ev = h.event;
            if (ev.kind == "count" && ev.args.size() == 3)
                ev.args = {ev.args[0], ev.args[2]};
            for (std::string& a : ev.args)
                if (a == "*")
                    a = "somewhere";
            r.fire(ev);
            for (double t = 0; !r.finished() && r.nextTimer() >= 0 && t < 60; t += r.nextTimer())
                r.advance(r.nextTimer());
            if (!(r.finished() && !r.won()))
                std::fprintf(stderr, "%s (%c) line %d: its fail doesn't lose\n", m.mission.c_str(), m.side, h.line);
            CHECK(r.finished() && !r.won());
        }
    }
    // The generic rules speak for any mission.
    MissionRun f1(*findMissionScript("F1", 'F'), generic);
    out = f1.fire({"mine-exhausted", {}, 404, 220});
    CHECK(out.size() == 1 && out[0].text == "The mine at 404, 220 has run out of minerals.");
    out = f1.fire({"built", {"vehicle-yard"}});
    CHECK(!out.empty() && out[0].text == "Vehicle yard finished.");
    // The game's spoken announcements.
    CHECK(announcementFor({"built", {"construction-yard"}}, {}, 'F') == "B_fcycom");
    CHECK(announcementFor({"built", {"accommodation"}}, {}, 'K') == "A_kacom");
    CHECK(announcementFor({"built", {"sickbay"}}, {}, 'R').empty());
    CHECK(announcementFor({"power-low", {}}, {}, 'K') == "klowpowr");
    CHECK(announcementFor({"short-crew", {}}, {}, 'F').empty()); // the officers say it (MissionSim::takeVoices)
    // The vehicles' voices: selected, ordered, refusing.
    CHECK(unitLine(8, 0, 0, 0) == "fpv004d" && unitLine(8, 1, 1, 1) == "fpv008b" && unitLine(8, 2, 0, 0) == "fneg1");
    CHECK(unitLine(228, 0, 1, 1) == "kmdb002b" && unitLine(63, 1, 0, 0) == "rsv010e" && unitLine(97, 0, 0, 1) == "ffgs003d");
    CHECK(unitLine(56, 0, 0, 0) == "fapc003a" && unitLine(54, 0, 0, 0).empty() && unitLine(55, 0, 0, 0).empty());
    // Their start-up sounds (the unarmed APCs have one, though no voice).
    CHECK(driveSound(37) == std::make_pair(std::string("fedactor.cat"), std::string("fscostar")));
    CHECK(driveSound(36).second == "kdtstart" && driveSound(88).second == "kapcstar" && driveSound(12).first.empty());
    // The buildings' own sounds, from their race's build archive; none for vehicles.
    CHECK(buildingSound(23) == std::make_pair(std::string("fedbuild.cat"), std::string("fcomstn")));
    CHECK(buildingSound(69) == std::make_pair(std::string("rombuild.cat"), std::string("fshield")));
    CHECK(buildingSound(75).second == "karsenal" && buildingSound(8).first.empty() && buildingSound(97).first.empty());
    CHECK(announcementFor({"built", {"vehicle-yard"}}, {{MissionRun::Output::Completed, {}, {}, 16, {}}}, 'R') == "robjmet");
    CHECK(announcementFor({"x", {}}, {{MissionRun::Output::Completed, {}, {}, 16, {}}, {MissionRun::Output::Won, {}, {}, 0, {}}},
                          'F') == "fmiswin");
}

void testColony() {
    CHECK(colonySite("F1", 'F').a == 407 && colonySite("F1", 'F').b == 209);
    CHECK(colonySite("J2", 'K').a == 92 && colonySite("J2", 'F').a == 220 && colonySite("J1", 'R').b == 442);
    CHECK(colonySite("F8", 'F').a < 0);
    bool matched = false;
    CHECK(buildingModel("vehicle-yard", 'F', &matched) == "geo/fed/fvpy_a.geo" && matched);
    CHECK(buildingModel("power-generator", 'F', &matched) == "geo/fed/fpgs.geo" && !matched);
    CHECK(buildingModel("colony-hub", 'K') == "geo/kln/kch.geo");
    CHECK(buildingModel("accommodation", 'K', &matched) == "geo/kln/kab.geo" && matched); // the Arsenal
    CHECK(buildingModel("sickbay", 'R') == "geo/rom/ruh_a.geo");
    CHECK(buildingModel("transporter-pad", 'R', &matched) == "geo/rom/rbcs.geo" && matched);
    // The game's landing pad is the transporter pad (exe.md).
    CHECK(buildingModel("transporter-pad", 'F') == "geo/fed/landpad.geo" && buildingModel("disruptor-tank", 'K').empty());

    // A flat 8 x 8 block map, a deposit in block (6, 4), a cliff along
    // block row 2's west half, nothing beyond column 7; buildings one tile
    // square, the accommodation block two.
    ColonyGround ground{8, 8, [](int bx, int by) { return bx == 6 && by == 4 ? 1 : 0; },
                        [](int a, int b) {
                            if (b >= 64)
                                return std::nanf("");
                            return a >= 16 && a < 24 && b < 32 ? 5000.0f : 0.0f;
                        },
                        [](const std::string& model) {
                            return model.find("accom") != std::string::npos ? std::pair{12000.0f, 12000.0f}
                                                                            : std::pair{6000.0f, 6000.0f};
                        }};
    ColonyLayout colony('F', ground);
    CHECK(colony.start(36, 36));
    CHECK(colony.buildings().size() == 1 && colony.buildings()[0].model == "geo/fed/fch.geo");
    // The pad: round the hub, off the cliff and the deposit, on the map.
    const ColonyBuilding start = colony.buildings()[0];
    CHECK(!colony.pad().empty());
    bool underHub = false;
    for (const auto& [tx, ty] : colony.pad()) {
        underHub = underHub || (tx == start.b / 4 && ty == start.a / 4) || (tx == (start.b - 1) / 4 && ty == (start.a - 1) / 4);
        CHECK(tx < 16 && !(ty >= 4 && ty < 6 && tx < 8) && !(tx / 2 == 6 && ty / 2 == 4));
        CHECK(std::abs(tx * 4 + 2 - start.b) <= 16 && std::abs(ty * 4 + 2 - start.a) <= 16);
    }
    CHECK(underHub);
    for (const char* b : {"construction-yard", "vehicle-yard", "power-generator", "accommodation", "mining-station",
                          "colony-hub-upgrade", "resource-processor", "sickbay"})
        CHECK(colony.add(b));
    CHECK(!colony.add("colony-hub-upgrade")); // once
    CHECK(!colony.add("disruptor-tank"));     // a unit
    CHECK(!colony.add("science-station-upgrade"));
    const auto& all = colony.buildings();
    CHECK(all.size() == 9);
    const ColonyBuilding& hub = all[0];
    CHECK(std::abs(hub.a - 36) <= 24 && std::abs(hub.b - 36) <= 24);
    for (const ColonyBuilding& c : all) {
        CHECK(c.b < 64 && !(c.a >= 14 && c.a < 26 && c.b < 34)); // on the map, off the cliff
        if (c.name == "colony-hub-upgrade")
            CHECK(c.a == hub.a && c.b == hub.b);
        else if (c.name == "mining-station")
            CHECK(c.b / 8 == 6 && c.a / 8 == 4); // on the deposit
        else
            CHECK(!(c.b / 8 == 6 && c.a / 8 == 4));
        CHECK(std::abs(c.a - hub.a) <= 32 && std::abs(c.b - hub.b) <= 32); // close by
    }
    // None overlap (one tile each, two for accommodation).
    auto half = [](const ColonyBuilding& c) { return c.name == "accommodation" ? 4 : 2; };
    for (size_t i = 0; i < all.size(); ++i)
        for (size_t j = i + 1; j < all.size(); ++j)
            if (all[j].name != "colony-hub-upgrade")
                CHECK(std::abs(all[i].a - all[j].a) >= half(all[i]) + half(all[j]) ||
                      std::abs(all[i].b - all[j].b) >= half(all[i]) + half(all[j]));
    // No ground at all: no colony.
    ColonyGround none = ground;
    none.heightAt = [](int, int) { return std::nanf(""); };
    ColonyLayout nowhere('K', none);
    CHECK(!nowhere.start(36, 36) && !nowhere.add("vehicle-yard"));
}

void testStarSystem() {
    // The system a table names most, by letters only.
    StringTable table;
    CHECK(!table.parse(makeTable({{1, "~STELLAR CARTOGRAPHY~Near Alpha and the F'Thorc system"},
                                  {2, "~PLANETARY DATA~F'thorc III, past ALPHA's belt; fthorc"}})));
    CHECK(starSystemName(table, {"alpha", "fthorc", "taal"}) == "fthorc");
    CHECK(starSystemName(table, {"taal"}).empty());
    CHECK(planetPicture("icewrld.cat") == "ice" && planetPicture("ALNWRLD.CAT") == "alien4" && planetPicture("x").empty());

    // A model: a starfield wall (texture 2), a star at the middle (two
    // slices numbered 0), a planet (two slices in 1, their backs in 0) and
    // a ringed planet (a slice in 3, the ring in 4, backs in 0).
    GeoModel m;
    m.parts.resize(1);
    auto quad = [&](float cx, float cy, float z, float r, uint32_t slot, bool back) {
        auto& v = m.parts[0].vertices;
        const uint32_t base = uint32_t(v.size());
        for (const auto& [dx, dy] : {std::pair{-r, -r}, std::pair{r, -r}, std::pair{r, r}, std::pair{-r, r}})
            v.push_back({cx + dx, cy + dy, z});
        auto face = [&](std::initializer_list<uint32_t> corners, uint32_t number) {
            GeoModel::Face f;
            for (const uint32_t c : corners)
                f.corners.push_back({base + c, 0, 0, {0, 0, 0, 0, 0}});
            f.corners.back().extra[4] = number;
            m.faces.push_back(f);
        };
        if (back)
            face({1, 0, 2}, 0), face({2, 0, 3}, 0);
        face({0, 1, 2, 3}, slot);
    };
    quad(0, 0, 0, 4000, 2, false);              // the wall
    quad(0, 0, 30, 70, 0, false), quad(0, 0, -30, 70, 0, false); // the star
    quad(2000, 0, 50, 80, 1, true), quad(2000, 0, -50, 80, 1, true);
    quad(-2000, 0, 0, 140, 4, true), quad(-2000, 0, 40, 90, 3, true), quad(-2000, 0, -40, 90, 3, true);
    const StarSystemLayout layout = readStarSystem(m);
    CHECK(layout.backdropSlots == std::vector<int>{2});
    CHECK(layout.bodies.size() == 3);
    int stars = 0, ringed = 0;
    for (const auto& b : layout.bodies) {
        stars += b.star;
        if (b.ringSlot >= 0) {
            CHECK(b.ringSlot == 4 && b.slot == 3 && std::fabs(b.centre.x + 2000) < 1);
            ++ringed;
        }
    }
    CHECK(stars == 1 && ringed == 1);
    // Backs take their slice's number; the ring's back the ring's.
    CHECK(layout.faceSlot[0] == 2 && layout.faceSlot[3] == 1 && layout.faceSlot[4] == 1);
    CHECK(layout.faceSlot[m.faces.size() - 9] == 4 && layout.faceSlot[m.faces.size() - 8] == 4);
    CHECK(layout.faceBody[0] == -1 && layout.faceBody[1] >= 0);
    CHECK(starSystemTextures(layout, "ice", "bezouk")[0] == "sun3");
    const auto tex = starSystemTextures(layout, "ice");
    CHECK(tex.size() == 5 && tex[0] == "sun" && tex[1] == "ice" && tex[2] == "stars1");
    CHECK(tex[3] == "red" && tex[4] == "red_r");
    const int look = missionBody(layout);
    CHECK(look >= 0 && layout.bodies[size_t(look)].slot == 1);
}

void testModelPose() {
    // A made-up tank: hull (node 0), turret (node 1) and gun (node 2), the
    // turret's axis at (0, 0, 10) and the gun's elevation point at
    // (0, -5, 12); the gun along -y.
    GeoModel g;
    g.parts.resize(3);
    for (uint32_t i = 0; i < 3; ++i)
        g.parts[i].node = i;
    g.attachments.resize(2);
    g.attachments[0].position = {0, 0, 10};
    g.attachments[1].position = {0, -5, 12};
    MdlModel m;
    m.parts = {{"main", 0}, {"turret", 1}, {"gun", 2}, {"muzzle_flash", 2}};
    m.points = {{"turret_axis", 0}, {"gun_elevate", 1}};
    CHECK(hasTurret(m) && !hasTurret(MdlModel{}));
    auto near = [](GeoModel::Vec3 a, float x, float y, float z) {
        return std::fabs(a.x - x) < 1e-3f && std::fabs(a.y - y) < 1e-3f && std::fabs(a.z - z) < 1e-3f;
    };
    const float q = 3.14159265f / 2;
    Pose p = turretPose(g, m, q, 0);
    CHECK(!p[0].moved && near(p[1].apply({1, 0, 10}), 0, 1, 10));
    CHECK(near(p[2].apply({0, -15, 12}), 15, 0, 12)); // the gun turns with the turret
    p = turretPose(g, m, 0, q);
    CHECK(!p[1].moved || near(p[1].apply({1, 0, 10}), 1, 0, 10));
    CHECK(near(p[2].apply({0, -15, 12}), 0, -5, 22)); // raised straight up
    // An animation frame: the arm, stored at the origin, turned a quarter
    // turn and moved to (1, 0, 0).
    m.parts = {{"main", 0}, {"arm", 1}};
    m.animated = {{"arm", 0}};
    m.reference.resize(1);
    std::vector<PartTransform> frame(1);
    const float r[9] = {0, -1, 0, 1, 0, 0, 0, 0, 1};
    std::copy(r, r + 9, frame[0].rotation);
    frame[0].position = {1, 0, 0};
    g.parts.resize(2);
    p = framePose(g, m, frame);
    CHECK(!p[0].moved && p[1].moved && near(p[1].apply({1, 0, 0}), 1, 1, 0));
}

void testEconomy() {
    auto count = [](const std::vector<ScriptEvent>& ev, const char* kind) {
        return std::count_if(ev.begin(), ev.end(), [&](const ScriptEvent& e) { return e.kind == kind; });
    };
    auto run = [](Economy& e, double seconds) {
        std::vector<ScriptEvent> all;
        for (double t = 0; t < seconds; t += 0.25)
            for (ScriptEvent& x : e.advance(0.25))
                all.push_back(std::move(x));
        return all;
    };
    // Magnesite (kind 5) in block 2,3 holding 30, nothing else.
    ColonyMap map;
    map.depositKind = [](int bx, int by) { return bx == 2 && by == 3 ? 5 : 0; };
    map.depositAmount = [](int bx, int by) { return bx == 2 && by == 3 ? 30 : 0; };
    Economy e('F', {500, 1000, 2300, 500, 4000, 5000}, 50, map);
    e.addBuilt("colony-hub", 0, 0);
    const BuildingType* yard = buildingType("construction-yard");
    const BuildingType* mine = buildingType("mining-station");
    CHECK(yard && mine && e.available(*yard) && !e.available(*mine) && !e.available(*buildingType("colony-hub")));
    CHECK(yard->costFor('F')[Nitrium] == 50 && yard->costFor('R')[Nitrium] == 500); // per side
    CHECK(buildingType("power-generator")->costFor('F')[Dilithium] == 35);
    CHECK(e.order("mining-station", 0, 0, 4, 6) == 0); // no processor yet
    // The cost is taken at the order.
    CHECK(e.order("construction-yard", 1000, 1000, 0, 0) != 0);
    CHECK(e.stock()[Thermoconc] == 2211 && e.stock()[Nitrium] == 3950 && e.colonySize() == 2);
    auto ev = run(e, 44);
    CHECK(count(ev, "built") == 0);
    ev = run(e, 2);
    CHECK(count(ev, "built") == 1 && ev[0].args[0] == "construction-yard" && e.capacity() == 50);
    // Upgrades: once each, after their building, the advanced after the improved.
    CHECK(!e.available(*buildingType("colony-hub-advanced")) && e.available(*buildingType("colony-hub-upgrade")));
    CHECK(!e.available(*buildingType("security-center")));
    CHECK(e.order("colony-hub-upgrade", 0, 0, 0, 0) != 0 && !e.available(*buildingType("colony-hub-upgrade")));
    // Short of materials: nothing spent, and the colony is told.
    Economy poor('F', {}, 50, map);
    poor.addBuilt("colony-hub", 0, 0);
    CHECK(poor.order("construction-yard", 0, 0, 0, 0) == 0);
    {
        const auto said = poor.advance(0.1);
        CHECK(count(said, "short-resources") == 1 && poor.structures().size() == 1);
        // ... and its officer reports construction delayed (exe.md#sounds).
        CHECK(std::any_of(said.begin(), said.end(), [](const ScriptEvent& ev) { return ev.kind == "officer" && ev.args[0] == "002"; }));
    }
    // Power: three yards' buffers draining faster than the hub's 600 a tick
    // refills them.
    Economy busy('F', {}, 50, map);
    busy.addBuilt("colony-hub", 0, 0);
    for (int i = 0; i < 4; ++i)
        busy.addBuilt("science-station", 0, 0);
    ev = run(busy, 120);
    CHECK(busy.demandMet() < 0.75f && count(ev, "power-low") == 2); // once a minute
    // A mine on the deposit: magnesite refined into nitrium, ten stock
    // units a tonne; the block's 30 takes, then dry.
    e.addBuilt("resource-processor", 0, 0);
    CHECK(e.order("mining-station", 0, 0, 0, 0) == 0); // no deposit there
    const int nitrium = e.stock()[Nitrium];
    CHECK(e.order("mining-station", 5 * kTile, 7 * kTile, 5, 7) != 0);
    bool dry = false;
    int at = -1, bt = -1;
    for (int i = 0; i < 2000 && !dry; ++i)
        for (const ScriptEvent& x : e.advance(0.25))
            if (x.kind == "mine-exhausted")
                dry = true, at = x.a, bt = x.b;
    CHECK(dry && at == 3 * 8 + 4 && bt == 2 * 8 + 4);
    run(e, 120);
    const Stock s = e.stock();
    // Of 30 takes, 2 in 8 are the cell's own ore and 1 in 8 each kind: about
    // 11 t of magnesite (110 nitrium), 4 t of each other ore.
    CHECK(s[Nitrium] - nitrium >= 100 && s[Nitrium] - nitrium <= 120);
    CHECK(e.oreStock()[4] < 0.01f);
    // Housing raises the capacity, up to 90.
    Economy home('F', {}, 85, map);
    home.addBuilt("accommodation", 0, 0);
    home.addBuilt("farm", 0, 0);
    CHECK(home.capacity() == 90);
    // The transports (exe.md#transports): with the hub's pool all in the
    // block, a yard's crew sent off rides back to the hub, and the crew
    // asked for again rides from the block; nobody is lost on the way.
    Economy ferry('F', {}, 50, map);
    ferry.addBuilt("colony-hub", 0, 0);
    ferry.addBuilt("accommodation", 0, 0);
    const int cyard = ferry.addBuilt("construction-yard", 0, 0);
    ferry.setTransports(true);
    const int everyone = ferry.people();
    int crew0 = 0;
    for (const ColonyStructure& c : ferry.structures())
        if (c.id == cyard)
            crew0 = c.crew;
    CHECK(crew0 > 0);
    ferry.setCrewTarget(cyard, 0);
    CHECK(ferry.rides().size() == 1 && ferry.rides()[0].from == cyard && ferry.rides()[0].toPool &&
          ferry.rides()[0].people == crew0 && ferry.people() == everyone);
    ferry.setCrewTarget(cyard, crew0);
    CHECK(ferry.rides().size() == 2 && ferry.rides()[1].to == cyard && !ferry.rides()[1].toPool &&
          ferry.people() == everyone);
    std::vector<int> ids;
    for (const Economy::Ride& r : ferry.rides())
        ids.push_back(r.id);
    for (int id : ids)
        ferry.rideLoaded(id), ferry.rideDelivered(id);
    for (const ColonyStructure& c : ferry.structures())
        if (c.id == cyard)
            CHECK(c.crew == crew0 && c.coming.empty());
    CHECK(ferry.rides().empty() && ferry.people() == everyone);
}

void testWorld() {
    // A 10 x 10 tile plain with a wall of closed tiles down column 5 (open
    // only at row 9), and a cliff (a 3000-unit step) between columns 1 and 2
    // in rows 0-2.
    Terrain t;
    t.width = t.height = 10;
    t.moveClass.assign(100, 1);
    for (int y = 0; y < 9; ++y)
        t.moveClass[size_t(y) * 10 + 5] = 0;
    t.heightAt = [](float x, float y) { return x >= 2 * kTile && y < 3 * kTile ? 3000.0f : 0.0f; };
    World w(t);
    CHECK(!w.passable(5, 0) && w.passable(5, 9) && !w.passable(-1, 0) && !w.passable(10, 0));
    CHECK(!w.canStep(1, 0, 1, 0) && w.canStep(1, 5, 1, 0) && !w.canStep(4, 0, 1, 0));
    CHECK(!w.canStep(4, 9, 1, -1)); // a diagonal past the wall's end
    // Round the wall, through its gap at the top.
    const auto path = w.findPath(3.5f * kTile, 4.5f * kTile, 7.5f * kTile, 4.5f * kTile);
    CHECK(!path.empty() && std::fabs(path.back().first - 7.5f * kTile) < 1 && std::fabs(path.back().second - 4.5f * kTile) < 1);
    bool viaGap = false;
    for (const auto& [x, y] : path)
        viaGap = viaGap || int(y / kTile) == 9;
    CHECK(viaGap);
    // A closed goal moves to an open tile beside it.
    const auto toWall = w.findPath(3.5f * kTile, 4.5f * kTile, 5.5f * kTile, 4.5f * kTile);
    CHECK(!toWall.empty() && int(toWall.back().first / kTile) != 5);

    // Units: selection, orders, groups, arrival.
    const int a = w.addUnit('F', groundScout('F'), 1.5f * kTile, 5.5f * kTile);
    const int b = w.addUnit('F', groundScout('F'), 1.5f * kTile, 6.5f * kTile);
    const int c = w.addUnit('K', groundScout('K'), 8.5f * kTile, 8.5f * kTile);
    CHECK(groundScout('R').model == "geo/rom/rfgs.geo");
    w.select({a, b});
    CHECK(w.selection() == (std::vector<int>{a, b}));
    w.setGroup(1);
    w.clearSelection();
    CHECK(w.selection().empty() && w.selectGroup(1) && w.selection().size() == 2 && !w.selectGroup(2));
    w.moveSelected(3.5f * kTile, 5.5f * kTile);
    CHECK(w.unit(a)->moving() && w.unit(b)->moving() && !w.unit(c)->moving());
    for (int i = 0; i < 200 && w.advance(0.1); ++i) {
    }
    const Unit& ua = *w.unit(a);
    const Unit& ub = *w.unit(b);
    CHECK(!ua.moving() && !ub.moving());
    CHECK(std::hypot(ua.x - 3.5f * kTile, ua.y - 5.5f * kTile) < 2000 && std::hypot(ua.x - ub.x, ua.y - ub.y) > 500);
    float gx = 0, gy = 0;
    CHECK(w.groupCentre(1, gx, gy) && std::fabs(gx - 3.5f * kTile) < 1500 && !w.groupCentre(3, gx, gy));
}

void testCombat() {
    Terrain t;
    t.width = t.height = 20;
    t.moveClass.assign(400, 1);
    World w(t);
    const CombatType* disruptor = combatType(10);
    const CombatType* phaser = combatType(8);
    const CombatType* house = combatType(114);
    CHECK(disruptor && phaser && house);
    // The figures the game plays with (rewritten at its start-up).
    CHECK(disruptor->weapon.kind == WeaponKind::Single && disruptor->weapon.damage == 30 && disruptor->hp == 70);
    CHECK(phaser->weapon.kind == WeaponKind::Burst && phaser->weapon.damage == 10 && phaser->weapon.magazine == 4 &&
          phaser->weapon.reload == 40 && phaser->weapon.range == 20000 && phaser->hp == 70);
    CHECK(house->building && house->hp == 600 && std::string(house->name) == "taubat-house");
    // A Klingon tank hostile to the Federation finds the tank in range and
    // hits it, 30 a shot; the Federation (neutral) can't answer.
    w.setStance(1, 0, 3);
    const int fed = spawnObject(w, 0, 'F', *phaser, 5 * kTile, 5 * kTile, {});
    const int kli = spawnObject(w, 1, 'K', *disruptor, 6.5f * kTile, 5 * kTile, {});
    CHECK(w.stance(1, 0) == 3 && w.stance(0, 1) == 2 && w.stance(1, 1) == 1);
    // (Once it has looked round and turned its turret half a turn: 1.6 s.)
    for (int i = 0; i < 25; ++i)
        w.advance(0.1);
    CHECK(w.unit(fed) && w.unit(fed)->hp == 40 && w.unit(kli) && w.unit(kli)->hp == 70);
    bool shot = false;
    for (const WorldEvent& e : w.takeEvents())
        shot = shot || (e.kind == WorldEvent::Shot && e.from == kli && e.weapon == WeaponKind::Single);
    CHECK(shot);
    for (int i = 0; i < 60 && w.unit(fed); ++i)
        w.advance(0.1);
    CHECK(!w.unit(fed)); // three shots
    // The player's tank (1.2 times the hit points) ordered onto a house:
    // 40 a burst every 2.2 s, so 15 bursts for its 600.
    if (w.unit(kli))
        w.remove(kli); // (it and the phaser tank, which answered its fire, trade evenly)
    const int mine = spawnObject(w, 0, 'F', *phaser, 12 * kTile, 12 * kTile, {}, {}, true);
    const int home = spawnObject(w, 3, 'T', *house, 14 * kTile, 12 * kTile, "taubat-colony");
    CHECK(w.unit(mine)->maxHp == 84.0f && !w.unit(mine)->ai);
    // Cadet mode: twice the hit points instead of the race's bonus.
    setCadetMode(true);
    const int cadet = spawnObject(w, 0, 'F', *phaser, 12 * kTile, 20 * kTile, {}, {}, true);
    setCadetMode(false);
    CHECK(w.unit(cadet)->maxHp == 140.0f);
    w.remove(cadet);
    w.attack({mine}, home);
    for (int i = 0; i < 100; ++i)
        w.advance(0.1);
    CHECK(w.unit(home) && w.unit(home)->hp < 600);
    for (int i = 0; i < 300; ++i)
        w.advance(0.1);
    CHECK(!w.unit(home));
    // A Klingon tank hostile to it beside it: they exchange fire.
    const int kli2 = spawnObject(w, 1, 'K', *disruptor, w.unit(mine)->x, w.unit(mine)->y + 12000, {});
    CHECK(scriptNames(*w.unit(kli2), 0) == std::vector<std::string>{"klingon-disruptor-tank"});
    w.takeEvents();
    bool fired = false, answered = false;
    for (int i = 0; i < 100 && !(fired && answered); ++i) {
        w.advance(0.1);
        for (const WorldEvent& e : w.takeEvents()) {
            fired = fired || (e.kind == WorldEvent::Shot && e.from == kli2 && e.to == mine);
            answered = answered || (e.kind == WorldEvent::Shot && e.from == mine && e.to == kli2);
        }
    }
    CHECK(fired && answered);
}

void testAiSides() {
    // F1 on an open plain as big as its map: the Klingon colony and the
    // Taubat placed, stances as the mission sets them, an assault sent.
    Terrain t;
    t.width = t.height = 128;
    t.moveClass.assign(128 * 128, 1);
    World w(t);
    AiSides ai("F1", 0, w);
    ai.start();
    int kli = 0, taubat = 0, mine = 0;
    for (const Unit& u : w.units())
        kli += u.slot == 1, taubat += u.slot == 3, mine += u.slot == 0;
    CHECK(kli > 10 && taubat > 10 && mine == 0);
    CHECK(w.stance(1, 0) == 3 && w.stance(0, 1) == 2 && w.stance(3, 0) == 2);
    CHECK(ai.slotsNamed("klingon") == std::vector<int>{1} && ai.slotsNamed("taubat") == std::vector<int>{3});
    CHECK(ai.slotsNamed("metar") == std::vector<int>{5} && ai.raceOf(3) == 'T');
    auto first = ai.observe({});
    auto has = [&](const std::vector<ScriptEvent>& ev, const std::string& what, int n) {
        return std::any_of(ev.begin(), ev.end(), [&](const ScriptEvent& e) {
            return e.kind == "count" && e.args.size() == 2 && e.args[0] == what && (n < 0 || e.args[1] == std::to_string(n));
        });
    };
    CHECK(has(first, "klingon-hub", 1) && has(first, "taubat-house", -1) && has(first, "taubat-colony", -1));
    CHECK(ai.observe({}).empty()); // nothing changed
    CHECK(!ai.order("klingon", "no-such-order"));
    // (An assault runs only while its target side has anything: a
    // Federation building far off.)
    spawnObject(w, 0, 'F', *combatType(23), 60 * kTile, 100 * kTile, {});
    CHECK(ai.order("klingon", "initial-assault-one"));
    const auto running = ai.running();
    CHECK(std::count(running.begin(), running.end(), "initial-assault-one") == 1);
    ai.advance(1.0);
    int driving = 0;
    for (const Unit& u : w.units())
        driving += u.slot == 1 && u.moving();
    CHECK(driving >= 2);
    // An assault doesn't read its message: an abort is a trigger too.
    CHECK(ai.order("klingon", "abort initial-assault-one"));
    ai.advance(1.0);
    const auto after = ai.running();
    CHECK(std::count(after.begin(), after.end(), "initial-assault-one") >= 1);
    ai.stance("taubat", "hostile");
    CHECK(w.stance(3, 0) == 3 && w.stance(0, 3) == 2);
    // The Klingons' named builds start armed: the construction yard's site
    // goes up at 60 script seconds, 48 s (not counted until it stands),
    // free to them.
    const Economy* kliEco = ai.colony(1);
    CHECK(kliEco && !ai.colony(0) && !ai.colony(3));
    const Stock before = kliEco->stock();
    auto yardSite = [&]() -> const Unit* {
        for (const Unit& u : w.units())
            if (u.slot == 1 && u.name == "construction-yard")
                return &u;
        return nullptr;
    };
    for (int i = 0; i < 45; ++i) // (2 s gone already)
        ai.advance(1.0);
    CHECK(!yardSite());
    ai.advance(2.0);
    CHECK(yardSite() && yardSite()->built < 1 && !has(ai.observe({}), "klingon-construction-yard", -1));
    CHECK(kliEco->stock() == before);
    for (int i = 0; i < 200 && yardSite()->built < 1; ++i)
        ai.advance(1.0);
    CHECK(yardSite()->built >= 1 && has(ai.observe({}), "klingon-construction-yard", 1));
    // Destroyed, it's built again (a named build keeps its building).
    const int firstYard = yardSite()->id;
    Unit gone = *yardSite();
    w.remove(firstYard);
    WorldEvent killed;
    killed.kind = WorldEvent::Destroyed;
    killed.what = gone;
    ai.observe({killed});
    ai.advance(2.0);
    CHECK(yardSite() && yardSite()->id != firstYard && yardSite()->built < 1);
    // A group the side has no idle units for waits for its vehicle yard
    // (built at 180 s) to make them, then sets off.
    std::vector<int> tanks;
    for (const Unit& u : w.units())
        if (u.slot == 1 && u.type == 10)
            tanks.push_back(u.id);
    for (int id : tanks)
        w.remove(id);
    CHECK(ai.order("klingon", "assault-random1"));
    ai.advance(1.0);
    auto kliTanks = [&] {
        int n = 0;
        for (const Unit& u : w.units())
            n += u.slot == 1 && u.type == 10;
        return n;
    };
    const int tanksBefore = kliTanks();
    bool made = false;
    for (int i = 0; i < 1500 && !made; ++i) {
        ai.advance(1.0);
        made = kliTanks() > tanksBefore;
    }
    CHECK(made && kliEco->stock() == before);
    for (int i = 0; i < 4; ++i)
        ai.advance(0.5), w.advance(0.5);
    bool off = false;
    for (const Unit& u : w.units())
        off = off || (u.slot == 1 && u.type == 10 && u.moving());
    CHECK(off);

    // A named build whose own building already stands on its spot keeps
    // it rather than building another, and rebuilds it once it's gone.
    {
        World w3(t);
        AiSides ai3("F1", 0, w3);
        ai3.start();
        const int yard = spawnObject(w3, 1, 'K', *combatType(53), 181.0f / 8 * 16384, 186.0f / 8 * 16384, {});
        for (int i = 0; i < 62; ++i)
            ai3.advance(1.0);
        int yards = 0;
        for (const Unit& u : w3.units())
            yards += u.slot == 1 && u.name == "construction-yard";
        CHECK(yards == 1 && w3.unit(yard));
        WorldEvent gone;
        gone.kind = WorldEvent::Destroyed;
        gone.what = *w3.unit(yard);
        w3.remove(yard);
        ai3.observe({gone});
        ai3.advance(2.0);
        yards = 0;
        for (const Unit& u : w3.units())
            yards += u.slot == 1 && u.name == "construction-yard";
        CHECK(yards == 1);
    }

    // The first assault reaches the Federation colony and fights it: shots
    // both ways (the player's tanks told to fight them: neutral, they'd
    // answer only once hit), the losses reported as the scripts name them.
    World w2(t);
    AiSides ai2("F1", 0, w2);
    ai2.start();
    w2.setStance(0, 1, 3);
    const float hubX = 218.0f / 8 * 16384, hubY = 438.0f / 8 * 16384;
    const int hub = spawnObject(w2, 0, 'F', *combatType(23), hubX, hubY, {}, {}, true);
    for (int i = 0; i < 4; ++i)
        spawnObject(w2, 0, 'F', *combatType(8), hubX - 8000 + float(i) * 2500, hubY - 6000, {}, {}, true);
    for (const char* o : {"initial-assault-one", "initial-assault-two", "initial-assault-three"})
        CHECK(ai2.order("klingon", o));
    bool kliFired = false, fedFired = false;
    std::vector<ScriptEvent> heard;
    for (int i = 0; i < 6000 && !(kliFired && fedFired && !heard.empty()); ++i) {
        ai2.advance(0.1);
        w2.advance(0.1);
        const auto events = w2.takeEvents();
        for (const WorldEvent& e : events)
            if (e.kind == WorldEvent::Shot) {
                kliFired = kliFired || e.fromSlot == 1;
                fedFired = fedFired || e.fromSlot == 0;
            }
        for (const ScriptEvent& e : ai2.observe(events))
            if (e.kind == "destroyed")
                heard.push_back(e);
    }
    CHECK(kliFired && fedFired && !heard.empty());
    CHECK(w2.unit(hub) || std::any_of(heard.begin(), heard.end(), [](const ScriptEvent& e) {
              return e.args[0] == "colony-hub";
          }));
}

void testFieldWork() {
    // F1 on an open plain: the Taubat placed, the Federation's science
    // vehicle and armed APC by them, its hub and sickbay by the colony.
    Terrain t;
    t.width = t.height = 128;
    t.moveClass.assign(128 * 128, 1);
    World w(t);
    AiSides ai("F1", 0, w);
    ai.start();
    auto at = [](int a, int b) { return std::pair<float, float>{float(b) / 8 * 16384, float(a) / 8 * 16384}; };
    std::map<std::string, std::pair<int, int>> sites{{"temple", {332, 152}}};
    auto find = [&](const std::string& label) {
        for (const Unit& u : w.units())
            if (u.label == label)
                return u.id;
        return 0;
    };
    const int mine = find("taubat-mine"), temple = find("taubat-temple"), colony = find("taubat-colony");
    CHECK(mine && temple && colony);
    CHECK(w.unit(temple)->kind.model == "geo/bad/tt.geo");
    const auto [hx, hy] = at(438, 218);
    spawnObject(w, 0, 'F', *combatType(23), hx, hy, {}, {}, true);
    const int sickbay = spawnObject(w, 0, 'F', *combatType(123), hx + 16000, hy, {}, {}, true);
    FieldWork fw("F1", 0, w, sites);
    fw.placeOfficers();
    CHECK(fw.officers().size() == 5);
    // Each officer wears its own face from the race's list (C1's Bellah 14).
    {
        const auto c1 = officers("C1", 0);
        CHECK(!c1.empty() && std::string(c1[0]->name) == "Bellah" && c1[0]->face == 14);
    }
    std::set<std::string> wanted{"scan-started science taubat-mine", "scanned science taubat-mine",
                                 "scanned strong-medical taubat-colony", "entered taubat-temple",
                                 "captured taubat-temple", "held temple 10", "vacated taubat-temple"};
    auto handled = [&](const ScriptEvent& e) {
        std::string s = e.kind;
        for (const auto& a : e.args)
            s += " " + a;
        return wanted.count(s) > 0;
    };
    std::vector<std::string> heard;
    auto run = [&](double seconds) {
        for (int i = 0; i < int(seconds * 10); ++i) {
            ai.advance(0.1);
            w.advance(0.1);
            for (const ScriptEvent& e : fw.advance(0.1, handled)) {
                std::string s = e.kind;
                for (const auto& a : e.args)
                    s += " " + a;
                heard.push_back(s);
            }
        }
    };
    auto heardIt = [&](const std::string& s) { return std::find(heard.begin(), heard.end(), s) != heard.end(); };
    // A weak science scan of the mine: started after 1 s, done after 10 s more.
    const auto [mx, my] = at(392, 206);
    const int sci = spawnObject(w, 0, 'F', *combatType(62), mx, my, {}, {}, true);
    CHECK(FieldWork::canScan(*w.unit(sci)) && !fw.strong(sci, FieldWork::Medical));
    CHECK(fw.scan(sci, mine, FieldWork::Science));
    run(3);
    CHECK(heardIt("scan-started science taubat-mine") && !heardIt("scanned science taubat-mine"));
    CHECK(fw.status(sci) == "SCANNING");
    run(10);
    CHECK(heardIt("scanned science taubat-mine"));
    // The doctor aboard (Kowalski, medical, skill 0.5) makes a medical scan strong.
    w.unit(sci)->x = hx + 16000 - 5000, w.unit(sci)->y = hy;
    fw.cancel(sci);
    CHECK(fw.board(sci) >= 1 && fw.strong(sci, FieldWork::Medical) && !fw.strong(sci, FieldWork::Science));
    const auto [cx, cy] = at(326, 176);
    w.unit(sci)->x = cx, w.unit(sci)->y = cy - 10000;
    CHECK(fw.scan(sci, colony, FieldWork::Medical));
    run(13);
    CHECK(heardIt("scanned strong-medical taubat-colony"));
    // A scan no one wants: the "nothing" reply.
    heard.clear();
    const int house = find("taubat-colony-1");
    CHECK(house && fw.scan(sci, house, FieldWork::Engineering));
    run(6);
    CHECK(heardIt("scanned-nothing engineering"));
    fw.cancel(sci);
    // The temple taken: fired on to 80 %, the team in for 16.8 s, then held.
    const auto [tx, ty] = at(338, 150);
    const int apc = spawnObject(w, 0, 'F', *combatType(56), tx, ty - 15000, {}, {}, true);
    CHECK(FieldWork::canCapture(*w.unit(apc)) && !FieldWork::canCapture(*w.unit(sci)));
    CHECK(!fw.capture(sci, temple) && !fw.capture(apc, sickbay) && fw.capture(apc, temple));
    for (int i = 0; i < 600 && fw.status(apc) == "ATTACKING"; ++i)
        run(0.1);
    CHECK(w.unit(temple) && w.unit(temple)->hp <= w.unit(temple)->maxHp * 0.8f);
    run(15);
    CHECK(fw.heldBy(temple) == -1 && !heardIt("entered taubat-temple"));
    run(18);
    CHECK(fw.heldBy(temple) == 0 && w.unit(temple)->hp == w.unit(temple)->maxHp);
    CHECK(heardIt("entered taubat-temple") && heardIt("held temple 10") && heardIt("captured taubat-temple"));
    // The temple is all of its side's colony: held, the share is whole.
    CHECK(w.unit(temple)->heldBy == 0 && heardIt("captured taubat-temple 100"));
    fw.cancel(apc);
    run(0.2);
    CHECK(fw.heldBy(temple) == -1 && heardIt("vacated taubat-temple") && w.unit(temple)->heldBy == -1);
}

// Officer training (exe.md#research): C1's Bellah (operations, 0.25) and
// Dante (engineering, 0.75) may train, Gage may not; a course takes
// 150000 points at the side's 1000 a tick, shared between those running,
// and raises its officer a band.
void testTraining() {
    Terrain t;
    t.width = t.height = 8;
    t.moveClass.assign(64, 1);
    World w(t);
    FieldWork fw("C1", 0, w, {});
    const auto& offs = fw.officers();
    CHECK(offs.size() == 4 && fw.courses().size() == 2);
    CHECK(std::string(offs[fw.courses()[0].officer].officer->name) == "Bellah");
    CHECK(std::string(rankTitle('F', offs[0].officer->rank)) == "Ens." && std::string(rankTitle('F', offs[1].officer->rank)) == "Lt.");
    CHECK(std::string(rankTitle('F', offs[2].officer->rank)) == "Dr.");
    CHECK(std::string(skillWord(0.25f)) == "BASIC" && std::string(skillWord(0.75f)) == "HIGHLY SKILLED" &&
          std::string(skillWord(1)) == "EXPERT");
    auto handled = [](const ScriptEvent&) { return false; };
    CHECK(fw.train(0) && !fw.train(0) && fw.train(1));
    // Two at once: 500 a tick each, 300 ticks (15 s).
    fw.advance(14.9, handled);
    CHECK(!fw.courses()[0].finished && fw.courses()[0].running);
    fw.advance(0.2, handled);
    CHECK(fw.courses()[0].finished && fw.courses()[1].finished);
    CHECK(offs[0].skill == 0.5f && offs[1].skill == 1.0f && offs[3].skill == 0.5f);
    CHECK(!fw.train(0));
}

// Bees (exe.md#bees): construction bees fly from the hub at 150 units a
// tick and build a site four to a site, each a quarter of the type's pace;
// the next site waits for them; done, they fly home. A mine sends ore
// only while a cargo bee serves it.
void testBees() {
    auto built = [](Economy& e, double seconds) {
        int n = 0;
        for (double t = 0; t < seconds; t += 0.05)
            for (const ScriptEvent& x : e.advance(0.05))
                n += x.kind == "built";
        return n;
    };
    ColonyMap map;
    map.depositKind = [](int, int) { return 0; };
    map.depositAmount = [](int, int) { return 0; };
    Economy e('F', {5000, 5000, 5000, 5000, 5000, 5000}, 50, map);
    e.addBuilt("colony-hub", 0, 0);
    e.setBees(4, 0);
    CHECK(e.bees().size() == 4 && e.bees()[0].home);
    // A yard (45 s with four) 15000 off: the bees leave the hub one at a
    // time through its dock (1 s each without its model's points) and fly
    // 15 ticks (1000 a tick).
    const int yard = e.order("construction-yard", 15000, 0, 0, 0);
    const int second = e.order("construction-yard", -15000, 0, 0, 0);
    CHECK(yard && second);
    {
        double t = 0, four = -1, done = -1;
        for (; t < 120 && done < 0; t += 0.05) {
            for (const ScriptEvent& x : e.advance(0.05))
                if (x.kind == "built")
                    done = t;
            if (four < 0 && e.beesAt(yard) == 4)
                four = t;
        }
        CHECK(four > 4 && four < 5.5 && done > 47 && done < 50);
    }
    // On to the second (30000 away: 30 ticks), all four, its 45 s.
    {
        double t = 0, four = -1, done = -1;
        for (; t < 120 && done < 0; t += 0.05) {
            for (const ScriptEvent& x : e.advance(0.05))
                if (x.kind == "built")
                    done = t;
            if (four < 0 && e.beesAt(second) == 4)
                four = t;
        }
        CHECK(four > 1 && four < 2 && done > 46 && done < 47);
    }
    built(e, 14); // home, in through the hub's dock one at a time
    CHECK(std::all_of(e.bees().begin(), e.bees().end(), [](const Economy::Bee& b) { return b.home; }));
    // One bee: four times as long.
    Economy one('F', {5000, 5000, 5000, 5000, 5000, 5000}, 50, map);
    one.addBuilt("colony-hub", 0, 0);
    one.setBees(1, 0);
    CHECK(one.order("construction-yard", 3000, 0, 0, 0)); // 45 s with four
    CHECK(built(one, 180) == 0 && built(one, 2) == 1);
    // Pods (exe.md#bees): the mine fills its store at 2 t a second, packs
    // 80 t into two pods of 40, its worker bee flies them out to its pad;
    // with a cargo bee they reach the processor, without one they wait
    // there.
    ColonyMap ore;
    ore.depositKind = [](int bx, int by) { return bx == 2 && by == 3 ? 5 : 0; };
    ore.depositAmount = [](int bx, int by) { return bx == 2 && by == 3 ? 300 : 0; };
    for (int cargo : {0, 1}) {
        Economy m('F', {5000, 5000, 5000, 5000, 5000, 5000}, 50, ore);
        m.addBuilt("colony-hub", 0, 0);
        m.addBuilt("construction-yard", 20000, 0);
        m.addBuilt("resource-processor", 20000, 20000);
        m.addBuilt("storage-facility", -20000, 0);
        m.setBees(4, cargo, 1); // the processor and storage get a worker bee free; one spare
        CHECK(std::count_if(m.bees().begin(), m.bees().end(), [](const Economy::Bee& b) { return b.worker; }) == 3);
        CHECK(m.order("mining-station", 2.5f * 16384, 3.5f * 16384, 5, 7));
        CHECK(built(m, 100) == 1); // 22 s out, 60 s building
        built(m, 45);
        const bool parked = std::any_of(m.pods().begin(), m.pods().end(), [](const Economy::Pod& p) {
            return p.state == Economy::Pod::Parked && !p.refined;
        });
        const auto stock = m.oreStock();
        const float arrived = std::accumulate(stock.begin(), stock.end(), 0.0f);
        if (cargo) {
            built(m, 60);
            const auto later = m.oreStock();
            CHECK(std::accumulate(later.begin(), later.end(), 0.0f) > 0);
            // The processor's goods, packed once there's a pod's worth,
            // reach the storage facility's own store by the same bees.
            int storage = 0;
            for (const ColonyStructure& c : m.structures())
                if (c.type == "storage-facility")
                    storage = c.id;
            const Stock before = m.storeOf(storage);
            built(m, 400);
            CHECK(m.storeOf(storage)[Nitrium] > before[Nitrium]);
        } else {
            CHECK(parked && arrived == 0);
            built(m, 60);
            const auto later = m.oreStock();
            CHECK(std::accumulate(later.begin(), later.end(), 0.0f) == 0);
            CHECK(m.pods().size() <= 4); // the mine stops with four waiting
        }
        for (const Economy::Pod& p : m.pods())
            CHECK(p.load[size_t(4)] > 0 || p.refined); // magnesite (kind 5) in every ore pod
    }
    // A yard: an L round the outer corner of the quarter block its dock
    // lets out into (here the building's middle, without a model: the
    // block's north-east quarter), 256 in from the edges, 992 apart.
    {
        Economy y('F', {5000, 5000, 5000, 5000, 5000, 5000}, 50, ore);
        const int st = y.addBuilt("storage-facility", 2.5f * 16384, 3.5f * 16384);
        const ColonyStructure* c = y.structure(st);
        CHECK(c != nullptr);
        if (c) {
            const float ox = 2.5f * 16384, oy = 3.5f * 16384;
            CHECK(y.padPlace(*c, 0) == std::make_pair(ox + 7936, oy + 0.0f));
            CHECK(y.padPlace(*c, 8) == std::make_pair(ox + 7936, oy + 7936));
            CHECK(y.padPlace(*c, 15) == std::make_pair(ox + 992, oy + 7936));
            CHECK(y.dockIn(*c, 150) == 20 && y.dockOut(*c, 150) == 20); // no model: 20 ticks
        }
    }
    // Without bees nothing goes up.
    Economy none('F', {5000, 5000, 5000, 5000, 5000, 5000}, 50, map);
    none.addBuilt("colony-hub", 0, 0);
    none.setBees(0, 0);
    none.order("power-generator", 3000, 0, 0, 0);
    CHECK(built(none, 60) == 0);
}

// Stores and razing (exe.md#stores, #demolishing): the mission's stock
// sits in the first storage facility, costs come from the facilities in
// turn, a vehicle's from the first alone; a facility lost takes its
// store; a razed building gives half its cost back and empties first.
void testStoresAndRazing() {
    ColonyMap map;
    map.depositKind = [](int, int) { return 0; };
    map.depositAmount = [](int, int) { return 0; };
    Economy e('F', {500, 1000, 2300, 500, 4000, 5000}, 50, map);
    e.addBuilt("colony-hub", 0, 0);
    const int first = e.addBuilt("storage-facility", 20000, 0);
    const int second = e.addBuilt("storage-facility", 40000, 0);
    CHECK(e.storeOf(first)[Polycomp] == 1000 && e.storeOf(second)[Polycomp] == 0 && e.stock()[Polycomp] == 1000);
    e.give(Polycomp, 50);
    CHECK(e.storeOf(first)[Polycomp] == 1050);
    // The hub and storage facilities can't be razed.
    CHECK(!e.razable(first) && !e.razable(1));
    const int gen = e.addBuilt("power-generator", 0, 20000);
    CHECK(e.razable(gen));
    const Stock before = e.stock();
    const Stock cost = buildingType("power-generator")->costFor('F');
    CHECK(e.raze(gen) && !e.razable(gen) && !e.evacuated(gen));
    e.advance(0.3); // a few ticks, nobody inside
    CHECK(e.evacuated(gen));
    for (int m = 0; m < kMaterials; ++m)
        CHECK(e.stock()[size_t(m)] == before[size_t(m)] + cost[size_t(m)] / 2);
    // The first facility lost: its store goes out of the stock.
    e.remove(first);
    CHECK(e.stock()[Polycomp] == 0 && e.storeOf(first)[Polycomp] == 0);
    // Gifts now go to the second.
    e.give(Dilithium, 10);
    CHECK(e.storeOf(second)[Dilithium] == 10);
    // A site cancelled: gone, its cost back; and no more than 10 going up.
    {
        e.addBuilt("construction-yard", -60000, 0);
        e.give(Dilithium, 5000), e.give(Thermoconc, 5000), e.give(Nitrium, 50000), e.give(Polycomp, 500);
        const Stock had = e.stock();
        const int site = e.order("power-generator", 60000, 60000, 0, 0);
        CHECK(site && e.stock()[Dilithium] < had[Dilithium] && e.cancel(site) && !e.structure(site));
        CHECK(e.stock()[Dilithium] == had[Dilithium] && !e.cancel(1)); // (a finished building isn't a site)
        std::vector<int> sites;
        for (int i = 0; i < 11; ++i)
            sites.push_back(e.order("power-generator", 60000 + 20000.0f * float(i), 80000, 0, 0));
        CHECK(e.pendingBuilds() == 10 && sites[9] && !sites[10]);
        for (int id : sites)
            if (id)
                e.cancel(id);
    }
    // A razed building's crew walk back to the hub's pool (20 s).
    const int bay0 = e.addBuilt("sickbay", 0, -40000);
    const int pool0 = e.pool();
    CHECK(e.raze(bay0) && e.walking() > 0 && e.pool() == pool0);
    e.advance(21);
    CHECK(e.walking() == 0 && e.pool() > pool0);
    // With transports, an unarmed APC takes them out instead: the building
    // stands until they're aboard.
    e.setTransports(true);
    const int bay1 = e.addBuilt("sickbay", 0, -60000);
    CHECK(e.raze(bay1)); // (those waiting for a ride count as on their way)
    e.advance(2);
    CHECK(!e.evacuated(bay1));
    int ride = 0;
    for (const Economy::Ride& r : e.rides())
        if (r.from == bay1)
            ride = r.id;
    CHECK(ride != 0);
    e.rideLoaded(ride);
    e.advance(0.3);
    CHECK(e.evacuated(bay1));
    e.setTransports(false);
    // A building's crew die with it (the sickbay's DEAD); not a razed one's.
    const int bay = e.addBuilt("sickbay", 0, -20000);
    int crew = 0;
    for (const ColonyStructure& c : e.structures())
        if (c.id == bay)
            crew = c.crew;
    CHECK(crew > 0 && e.dead() == 0);
    e.remove(bay);
    CHECK(e.dead() == crew);
}

// Projectiles (exe.md#sides-and-combat): a disruptor bolt flies 5400
// units a tick and hurts its target only on arrival; a side's own in the
// line of fire stop it firing, but one that drives in after passes the
// shot through (no friendly fire); a shell flies its arc.
void testProjectiles() {
    Terrain t;
    t.width = t.height = 40;
    t.moveClass.assign(1600, 1);
    // A gun's pitch: a target on a cliff 20000 above, 15000 off, is past
    // the phaser tank's 20 degrees up, so never fired on.
    for (const float rise : {0.0f, 20000.0f}) {
        Terrain cliff = t;
        cliff.heightAt = [rise](float x, float) { return x > 150000 ? rise : 0.0f; };
        World c(cliff);
        c.setStance(0, 1, 3);
        const int tank = spawnObject(c, 0, 'F', *combatType(8), 140000, 20 * 8192.0f, {});
        const int above = spawnObject(c, 1, 'K', *combatType(10), 155000, 20 * 8192.0f, {});
        c.unit(above)->holdFire = true;
        bool shot = false;
        for (int i = 0; i < 100; ++i) {
            c.advance(0.05);
            for (const WorldEvent& e : c.takeEvents())
                shot = shot || (e.kind == WorldEvent::Shot && e.from == tank);
        }
        CHECK(shot == (rise == 0)); // (on level ground it fires)
    }
    World w(t);
    w.setStance(1, 0, 3);
    const float y = 20 * 8192.0f;
    const int back = spawnObject(w, 1, 'K', *combatType(10), 100000, y, {});
    const int front = spawnObject(w, 1, 'K', *combatType(10), 106000, y, {});
    const int fed = spawnObject(w, 0, 'F', *combatType(8), 116000, y, {});
    w.unit(front)->holdFire = true;
    const float hp = w.unit(fed)->hp;
    bool fired = false;
    for (int ticks = 0; ticks < 200 && !fired; ++ticks) {
        w.advance(0.05);
        for (const WorldEvent& e : w.takeEvents())
            fired = fired || (e.kind == WorldEvent::Shot && e.from == back);
    }
    CHECK(!fired); // its own tank is in the way
    w.unit(front)->y = w.unit(front)->lastY = y + 8000;
    for (int ticks = 0; ticks < 200 && !fired; ++ticks) {
        w.advance(0.05);
        for (const WorldEvent& e : w.takeEvents())
            fired = fired || (e.kind == WorldEvent::Shot && e.from == back);
    }
    CHECK(fired && !w.projectiles().empty());
    w.unit(front)->y = w.unit(front)->lastY = y; // back into the line, after the shot
    // 16000 away: three ticks out, the front tank no shield.
    CHECK(w.unit(fed)->hp == hp);
    bool hit = false;
    for (int i = 0; i < 8; ++i) {
        w.advance(0.05);
        for (const WorldEvent& e : w.takeEvents())
            hit = hit || (e.kind == WorldEvent::Hit && e.to == fed);
    }
    CHECK(hit && (!w.unit(fed) || w.unit(fed)->hp < hp));
    CHECK(w.unit(front)->hp == w.unit(front)->maxHp);
    // A defensive turret's shots take 300 power each; without it, it's
    // silent; a tank pays nothing.
    {
        World p(t);
        p.setStance(0, 1, 3);
        p.setStance(1, 0, 3);
        int asked = 0;
        p.setShotPower([&](const Unit&, float power) {
            asked += int(power);
            return false;
        });
        const int turret = spawnObject(p, 0, 'F', *combatType(80), 100000, y, {}, {}, true);
        const int tank = spawnObject(p, 1, 'K', *combatType(10), 112000, y, {});
        bool turretFired = false, tankFired = false;
        for (int i = 0; i < 100; ++i) {
            p.advance(0.05);
            for (const WorldEvent& e : p.takeEvents())
                if (e.kind == WorldEvent::Shot)
                    turretFired = turretFired || e.from == turret, tankFired = tankFired || e.from == tank;
        }
        CHECK(!turretFired && tankFired && asked >= 300);
    }
    // A turret that turns only 60 degrees each way (the Romulan armed APC)
    // won't pick an enemy behind it; told to attack, it drives at it and
    // fires once it's in the arc.
    {
        World r(t);
        r.setStance(2, 0, 3);
        r.setStance(0, 2, 2);
        const int apc = spawnObject(r, 2, 'R', *combatType(64), 100000, y, {}, {}, true);
        CHECK(std::abs(combatType(64)->weapon.arc - 1.047f) < 0.01f);
        r.unit(apc)->heading = 0; // facing east
        const int behind = spawnObject(r, 0, 'F', *combatType(8), 88000, y, {});
        auto fired = [&](int ticks) {
            bool any = false;
            for (int i = 0; i < ticks; ++i) {
                r.advance(0.05);
                for (const WorldEvent& e : r.takeEvents())
                    any = any || (e.kind == WorldEvent::Shot && e.from == apc);
            }
            return any;
        };
        CHECK(!fired(100) && std::abs(r.unit(apc)->heading) < 0.01f);
        r.select({apc});
        r.attack({apc}, behind);
        CHECK(fired(200));
    }
    // A shell: up its arc and down onto a building 18000 away.
    World a(t);
    a.setStance(0, 1, 3);
    const int gun = spawnObject(a, 0, 'F', *combatType(47), 100000, y, {});
    const int hut = spawnObject(a, 1, 'T', *combatType(114), 118000, y, {}, {}, true);
    float highest = 0;
    bool landed = false;
    for (int i = 0; i < 400 && !landed; ++i) {
        a.advance(0.05);
        for (const Projectile& p : a.projectiles())
            highest = std::max(highest, p.z);
        for (const WorldEvent& e : a.takeEvents())
            landed = landed || (e.kind == WorldEvent::Hit && e.to == hut);
    }
    // (The low arc: about 13 degrees up at that range, topping out near 1000.)
    CHECK(gun && landed && highest > 800 && highest < 1300);
    // A shot leads a target crossing in front of it: aimed where the
    // target's last step takes it in the shot's time.
    {
        World l(t);
        l.setStance(1, 0, 3);
        const int gun = spawnObject(l, 1, 'K', *combatType(10), 100000, 100000, {});
        const int scout = spawnObject(l, 0, 'F', *combatType(97), 100000, 115000, {});
        l.moveTo(scout, 250000, 115000);
        float aimX = 0, atX = 0;
        for (int i = 0; i < 200 && aimX == 0; ++i) {
            l.advance(0.05);
            for (const WorldEvent& e : l.takeEvents())
                if (e.kind == WorldEvent::Shot && e.from == gun && l.unit(scout))
                    aimX = e.x1, atX = l.unit(scout)->x;
        }
        CHECK(aimX > atX + 100);
    }
}

void testRivalsAndGroups() {
    Terrain t;
    t.width = t.height = 128;
    t.moveClass.assign(128 * 128, 1);
    auto said = [](const std::vector<ScriptEvent>& ev, const std::string& want) {
        return std::any_of(ev.begin(), ev.end(), [&](const ScriptEvent& e) {
            std::string s = e.kind;
            for (const auto& a : e.args)
                s += " " + a;
            return s == want;
        });
    };
    // A side's groups by class and its colony's size against its start:
    // F1's Taubat, a hangar (large) lost.
    {
        World w(t);
        AiSides ai("F1", 0, w);
        ai.start();
        int houses = 0, hangar = 0;
        for (const Unit& u : w.units())
            if (u.slot == 3 && u.building) {
                houses += u.name == "taubat-house";
                if (u.name == "taubat-hangar")
                    hangar = u.id;
            }
        const auto first = ai.observe({});
        CHECK(houses > 0 && hangar && said(first, "count taubat-small-buildings " + std::to_string(houses)));
        CHECK(said(first, "colony-size taubat 100") && said(first, "count klingon-fighting-units 12"));
        WorldEvent gone;
        gone.kind = WorldEvent::Destroyed;
        gone.what = *w.unit(hangar);
        w.remove(hangar);
        const auto after = ai.observe({gone});
        auto share = [](const std::vector<ScriptEvent>& ev, const char* kind) {
            for (const ScriptEvent& e : ev)
                if (e.kind == kind && e.args.size() == 2 && e.args[0] == "taubat")
                    return std::atoi(e.args[1].c_str());
            return -1;
        };
        const int lost = share(after, "colony-size");
        CHECK(lost > 0 && lost < 100 && share(after, "colony-total") == lost);
        // A house captured: out of its size (kind 8), still in its total (kind 9).
        for (const Unit& u : w.units())
            if (u.slot == 3 && u.name == "taubat-house") {
                w.unit(u.id)->heldBy = 0;
                break;
            }
        const auto held = ai.observe({});
        CHECK(share(held, "colony-size") < lost && share(held, "colony-total") == -1);
    }
    // Go_Futile goes where each raid sends it: J2's three hangar captures
    // to each other (not the destruction monitor), none of J4's.
    {
        auto hands = [](const char* mission, const char* name) {
            std::set<std::string> out;
            for (const Raid* r : raids(mission))
                if (std::string(r->name) == name)
                    for (const char* f : r->futile)
                        if (*f)
                            out.insert(f);
            return out;
        };
        CHECK(hands("J2", "kli-hangar-capture-objective") ==
              (std::set<std::string>{"fed-hangar-capture-objective", "rom-hangar-capture-objective"}));
        CHECK(hands("J2", "taubat-hangar-destruction-monitor").empty());
        CHECK(hands("J4", "kli-sci-blk-raid").empty() && hands("J4", "hostage-sci-blk-raid").empty());
    }
    // A rival's watch: J3's Klingon raid on a workshop is won when it's
    // destroyed, by anyone (80 ticks, then 480: 4 s and 24 s).
    {
        World w(t);
        AiSides ai("J3", 0, w);
        ai.start();
        FieldWork fw("J3", 0, w, {});
        int target = 0;
        for (const Unit& u : w.units())
            if (u.label == "raid-building-one")
                target = u.id;
        CHECK(target != 0);
        std::vector<ScriptEvent> heard;
        auto run = [&](double seconds) {
            for (int i = 0; i < int(seconds * 10); ++i) {
                auto ev = fw.advance(0.1, [](const ScriptEvent&) { return true; });
                heard.insert(heard.end(), ev.begin(), ev.end());
            }
        };
        run(1);
        w.remove(target);
        run(26);
        CHECK(!said(heard, "captured-by klingon raid-building-one"));
        run(3);
        CHECK(said(heard, "captured-by klingon raid-building-one"));
    }
}

void testVehicleProduction() {
    auto run = [](Economy& e, double seconds) {
        std::vector<std::pair<int, int>> out;
        for (double t = 0; t < seconds - 1e-9; t += 0.25) {
            e.advance(0.25);
            for (const auto& v : e.takeVehicles())
                out.push_back(v);
        }
        return out;
    };
    ColonyMap map;
    Economy e('F', {500, 1000, 2300, 500, 4000, 5000}, 50, map);
    e.addBuilt("colony-hub", 0, 0);
    e.addBuilt("construction-yard", 20000, 0);
    const VehicleType* apc = vehicleType(56);
    const VehicleType* tank = vehicleType(8);
    CHECK(apc && tank && apc->value == 240 && tank->value == 640);
    CHECK(!e.orderVehicle(*apc)); // no vehicle yard
    const int yard = e.addBuilt("vehicle-yard", 0, 20000);
    CHECK(e.vehicleAvailable(*apc) && !e.vehicleAvailable(*tank) && !e.vehicleAvailable(*vehicleType(10)));
    // The armed transport: 240 x 300 units at 300 a tick and efficiency
    // 0.8 (no officer): 300 ticks, 15 s, then 9.5 s rolling out.
    CHECK(e.orderVehicle(*apc) && e.stock()[Duranium] == 4970);
    CHECK(run(e, 24).empty());
    auto out = run(e, 1);
    CHECK(out.size() == 1 && out[0].first == 56 && out[0].second == yard);
    // The panels' figures: the colony's consumption, each consumer's supply.
    CHECK(e.consumption() > 0 && e.consumption() <= 1);
    CHECK(std::all_of(e.structures().begin(), e.structures().end(),
                      [](const ColonyStructure& c) { return c.supply > 0 && c.supply <= 1; }));
    // An officer of the yards' speciality at 0.5: efficiency 1.0, 12 s.
    e.setOfficer(yard, 0.5f);
    CHECK(e.orderVehicle(*apc));
    CHECK(run(e, 21).empty() && run(e, 1).size() == 1);
    // The phaser tank wants the extended yard and the phaser coil lab (a
    // Federation lab, on the science station).
    e.addBuilt("vehicle-yard-upgrade", 0, 20000);
    e.addBuilt("science-station", 20000, 20000);
    CHECK(!e.vehicleAvailable(*tank) && e.available(*buildingType("phaser-coil-lab")));
    e.addBuilt("phaser-coil-lab", 20000, 20000);
    CHECK(e.vehicleAvailable(*tank) && !e.available(*buildingType("phaser-coil-lab")));
    Economy k('K', {500, 1000, 2300, 500, 4000, 5000}, 50, map);
    k.addBuilt("construction-yard", 0, 0);
    k.addBuilt("science-station", 0, 0);
    CHECK(!k.available(*buildingType("phaser-coil-lab")) && k.available(*buildingType("disruptor-matrix")));
    // At the colony's size limit the yard takes no orders.
    Economy full('F', {500, 1000, 2300, 500, 4000, 5000}, 3, map);
    full.addBuilt("colony-hub", 0, 0);
    full.addBuilt("construction-yard", 0, 0);
    full.addBuilt("vehicle-yard", 0, 0);
    CHECK(!full.orderVehicle(*apc));
}

void testCrew() {
    auto run = [](Economy& e, double seconds) {
        std::vector<ScriptEvent> all;
        for (double t = 0; t < seconds - 1e-9; t += 0.25)
            for (ScriptEvent& x : e.advance(0.25))
                all.push_back(std::move(x));
        return all;
    };
    ColonyMap map;
    // The roster of 150, five of them officers: the hub takes in the other
    // 145; buildings standing at the start have their crew from it.
    Economy e('F', {500, 1000, 2300, 500, 4000, 5000}, 50, map);
    e.setOfficers(5);
    e.addBuilt("colony-hub", 0, 0);
    CHECK(e.pool() == 145 && e.inOrbit() == 0 && e.people() == 150);
    e.addBuilt("construction-yard", 20000, 0);
    const int yard = e.addBuilt("vehicle-yard", 0, 20000);
    CHECK(e.crew() == 4 && e.pool() == 141);
    for (const ColonyStructure& s : e.structures())
        if (s.id == yard)
            CHECK(s.crew == 3 && e.efficiency(s) > 0.79f && e.efficiency(s) < 0.81f);
    // A sickbay ordered: its crew walk over once it's built (20 s).
    CHECK(e.order("sickbay", 30000, 0, 0, 0) != 0);
    run(e, 41);
    CHECK(e.has("sickbay") && e.pool() == 140);
    int sickbayCrew = -1;
    for (const ColonyStructure& s : e.structures())
        if (s.type == "sickbay")
            sickbayCrew = s.crew;
    CHECK(sickbayCrew == 0);
    run(e, 21);
    for (const ColonyStructure& s : e.structures())
        if (s.type == "sickbay")
            sickbayCrew = s.crew;
    CHECK(sickbayCrew == 1);
    // The crew assigned: the yard cut to one sends two back to the hub's
    // pool; raised again, it asks for them (they walk over).
    const int poolBefore = e.pool();
    e.setCrewTarget(yard, 1);
    auto yardCrew = [&] {
        for (const ColonyStructure& s : e.structures())
            if (s.id == yard)
                return s.crew;
        return -1;
    };
    CHECK(yardCrew() == 1 && e.pool() == poolBefore + 2);
    e.setCrewTarget(yard, 9); // (held to its 3 slots)
    CHECK(e.pool() == poolBefore);
    run(e, 21);
    CHECK(yardCrew() == 3);
    // Life support: 150 people at 16 a tick, bought back by the hub at
    // 16.384 a unit of power, divided by its efficiency (0.8): 183 a tick.
    CHECK(e.lifeSupportPower() > 170 && e.lifeSupportPower() < 190);
    // Too few people: the yard waits for its crew, works at nothing, and
    // an order for a vehicle tells the colony it's short of crew.
    Economy few('F', {500, 1000, 2300, 500, 4000, 5000}, 50, map);
    few.setOfficers(148);
    few.addBuilt("colony-hub", 0, 0);
    const int empty = few.addBuilt("vehicle-yard", 0, 0);
    for (const ColonyStructure& s : few.structures())
        if (s.id == empty)
            CHECK(s.crew == 0 && s.wanted == 3 && few.efficiency(s) == 0);
    CHECK(few.orderVehicle(*vehicleType(97)));
    const auto told = run(few, 1);
    CHECK(std::any_of(told.begin(), told.end(), [](const ScriptEvent& ev) { return ev.kind == "short-crew"; }));
    CHECK(std::any_of(told.begin(), told.end(), [](const ScriptEvent& ev) { return ev.kind == "officer" && ev.args[0] == "001"; }));
    run(few, 30);
    CHECK(few.takeVehicles().empty());
}

void testScriptEffects() {
    // A script using the effects, on F1's sides over an open plain.
    std::vector<MissionScript> parsed;
    const char* text = "mission F1 F\n"
                       "zone pad 400 200 420 240\n"
                       "on start\n"
                       "  give dilithium 100\n"
                       "  take duranium 50\n"
                       "  spawn klingon disruptor-tank 3 410 220 raiders\n"
                       "  damage raiders 50\n"
                       "  meteors pad 2\n"
                       "on timer x\n"
                       "  beam-up raiders\n";
    CHECK(!parseMissionScripts(text, parsed) && parsed[0].zones.at("pad")[2] == 420);
    std::vector<MissionScript> bad;
    CHECK(parseMissionScripts("mission T F\non start\n  meteors nowhere 3\n", bad));
    CHECK(parseMissionScripts("mission T F\non start\n  damage x lots\n", bad));
    Terrain t;
    t.width = t.height = 128;
    t.moveClass.assign(128 * 128, 1);
    World w(t);
    AiSides ai("F1", 0, w);
    ai.start();
    Economy e('F', {500, 1000, 2300, 500, 4000, 5000}, 50, ColonyMap{});
    ScriptEffects fx(0, w, &ai, &e, &parsed[0]);
    MissionRun run(parsed[0], nullptr);
    for (const MissionRun::Output& o : run.fire({"start", {}}))
        if (o.kind == MissionRun::Output::Effect || o.kind == MissionRun::Output::Stock)
            fx.apply(o.sender, o.text);
    CHECK(e.stock()[Dilithium] == 600 && e.stock()[Duranium] == 4950);
    int raiders = 0;
    for (const Unit& u : w.units())
        if (u.label == "raiders") {
            ++raiders;
            CHECK(u.slot == 1 && u.name == "disruptor-tank" && u.hp == 35); // at half
        }
    CHECK(raiders == 3);
    // The zone sees them; the meteors fall on it.
    std::vector<std::string> heard;
    auto step = [&](double seconds) {
        for (int i = 0; i < int(seconds * 10); ++i)
            for (const ScriptEvent& ev : fx.advance(0.1)) {
                std::string line = ev.kind;
                for (const auto& a : ev.args)
                    line += " " + a;
                heard.push_back(line);
            }
    };
    step(2.2);
    auto has = [&](const std::string& line) { return std::find(heard.begin(), heard.end(), line) != heard.end(); };
    CHECK(has("inside klingon pad") && has("inside raiders pad") && has("inside any pad") && has("outside taubat pad"));
    CHECK(!fx.sparks().empty());
    // Beamed up: gone, and the group reported.
    fx.apply("beam-up", "raiders");
    step(0.2);
    CHECK(has("beamed-up raiders"));
    for (const Unit& u : w.units())
        CHECK(u.label != "raiders");
    step(1.1);
    CHECK(has("outside klingon pad"));
    // A route; what stands, second by second.
    const int scout = spawnObject(w, 0, 'F', *combatType(97), 100000, 100000, "geology-scout", {}, true);
    fx.apply("move", "geology-scout 100 100 120 100");
    CHECK(w.unit(scout)->moving());
    e.addBuilt("colony-hub", 0, 0);
    heard.clear();
    step(3.05);
    CHECK(has("standing colony-hub 3"));
    std::vector<MissionScript> holdScript;
    CHECK(!parseMissionScripts("mission T F\non standing colony-hub 2\n  win\non infection 3\n  win\n", holdScript));
    MissionRun hold(holdScript[0], nullptr);
    CHECK(!hold.fire({"standing", {"colony-hub", "1"}}).size() && hold.fire({"standing", {"colony-hub", "2"}}).size());
    // The disease among the colony's plain crew (in its buildings and the
    // hub's pool): one of them visited each tick, 16 a second, catching it
    // on a roll under 10 of 101; once the cure begins, one who has it
    // recovers on a roll under 60. Health stays whole but for the rare
    // roll under 4.
    e.addBuilt("construction-yard", 0, 0);
    e.addBuilt("vehicle-yard", 0, 0);
    CHECK(e.crew() == 4);
    fx.apply("infect", "");
    heard.clear();
    step(60);
    CHECK(fx.infection() >= 25 && has("infection 25"));
    const int held = e.crew() + e.pool();
    CHECK(fx.healthSum() < int64_t(held) * 256 && fx.healthSum() > int64_t(held) * 256 * 9 / 10);
    fx.apply("cure", "");
    step(240);
    CHECK(fx.infection() == 0 && has("cured"));
}

void testRepairsShieldsCloaking() {
    Terrain t;
    t.width = t.height = 128;
    t.moveClass.assign(128 * 128, 1);
    // A vehicle mends a point, then waits 6400 / its full hit points ticks.
    {
        World w(t);
        const int tank = spawnObject(w, 0, 'F', *combatType(8), 100000, 100000, {});
        w.unit(tank)->hp = 10;
        Repairs r(w);
        r.tick();
        CHECK(w.unit(tank)->hp == 11);
        for (int i = 0; i < 91; ++i) // 6400 / 70
            r.tick();
        CHECK(w.unit(tank)->hp == 11);
        r.tick();
        CHECK(w.unit(tank)->hp == 12);
        // A site isn't mended.
        w.unit(tank)->built = 0.5f;
        for (int i = 0; i < 500; ++i)
            r.tick();
        CHECK(w.unit(tank)->hp == 12);
    }
    // A building with a construction yard filling the pool: a step every
    // 8 ticks, of 3 points with a skill-1 engineer in it; without a yard
    // the slow rate (1500 hp: a point every 4 / 3 + 1 ticks, cut down: 2).
    auto hubAfter = [&](bool yard) {
        World w(t);
        const int hub = spawnObject(w, 0, 'F', *combatType(23), 100000, 100000, {});
        if (yard)
            spawnObject(w, 0, 'F', *combatType(77), 140000, 100000, {});
        w.unit(hub)->hp = 300;
        Repairs r(w);
        r.setEngineering([&](int id) { return id == hub ? 1.0f : 0.0f; });
        for (int i = 0; i < 80; ++i)
            r.tick();
        return w.unit(hub)->hp;
    };
    CHECK(hubAfter(true) == 330 && hubAfter(false) == 340);

    // Shields: half the damage inside its own side's field, for the kinds
    // it covers; a security officer's skill divides what's left.
    {
        World w(t);
        const float bx = 10.5f * 16384, by = 10.5f * 16384;
        spawnObject(w, 1, 'K', *combatType(29), bx, by, {});
        const int tank = spawnObject(w, 1, 'K', *combatType(10), bx, by + 16384, {});
        const int bee = spawnObject(w, 1, 'K', *combatType(46), bx, by + 16384, {});
        const int fed = spawnObject(w, 0, 'F', *combatType(8), bx + 16384, by, {});
        const int far = spawnObject(w, 1, 'K', *combatType(10), bx + 5 * 16384, by, {});
        Fog fog(64, 64);
        fog.refresh(w);
        CHECK(shieldedDamage(*w.unit(tank), 40, &fog, 0) == 20);
        CHECK(shieldedDamage(*w.unit(tank), 40, &fog, 1) == 4);
        CHECK(shieldedDamage(*w.unit(bee), 40, &fog, 0) == 40);
        CHECK(shieldedDamage(*w.unit(fed), 40, &fog, 0) == 40);
        CHECK(shieldedDamage(*w.unit(far), 40, &fog, 0) == 40);
        // A weak generator's field shrinks to a plus: the corner blocks are out.
        for (const Unit& u : w.units())
            if (u.name == "shield-generator")
                w.unit(u.id)->efficiency = 0.5f;
        fog.refresh(w);
        CHECK(fog.has(10, 11, Fog::Shielded, 1) && !fog.has(11, 11, Fog::Shielded, 1));
    }
    // The economy offers the defences once what they need stands.
    {
        Economy e('F', {500, 1000, 2300, 500, 4000, 5000}, 50, ColonyMap{});
        e.addBuilt("colony-hub", 0, 0);
        e.addBuilt("construction-yard", 20000, 0);
        const BuildingType& shield = *buildingType("shield-generator");
        const BuildingType& turret = *buildingType("defensive-weapon");
        CHECK(!e.available(shield) && !e.available(turret));
        for (const char* b : {"construction-yard-upgrade", "security-center", "science-station", "shield-buffer-lab",
                              "colony-hub-upgrade"})
            e.addBuilt(b, 0, 0);
        CHECK(e.available(shield) && !e.available(turret));
        e.addBuilt("shield-generator", 40000, 0);
        CHECK(e.available(turret));
    }

    // Cloaking: 300 ticks seen, a 63-tick fade, then out of everyone's
    // sights; to fire it decloaks (63 ticks without a shot).
    {
        World w(t);
        w.setStance(0, 2, 3);
        w.setStance(2, 0, 3);
        const int rom = spawnObject(w, 2, 'R', *combatType(35), 100000, 100000, {});
        auto ticks = [&](int n) {
            for (int i = 0; i < n; ++i)
                w.advance(0.05);
        };
        ticks(301);
        CHECK(w.unit(rom)->fading > 0 && !w.unit(rom)->cloaked);
        ticks(63);
        CHECK(w.unit(rom)->cloaked);
        w.takeEvents();
        const int fed = spawnObject(w, 0, 'F', *combatType(8), 110000, 100000, {});
        ticks(12);
        CHECK(!w.unit(rom)->cloaked && w.unit(rom)->fading > 0);
        bool romFired = false, fedFired = false;
        for (const WorldEvent& e : w.takeEvents())
            romFired = romFired || (e.kind == WorldEvent::Shot && e.from == rom);
        CHECK(!romFired);
        ticks(70);
        for (const WorldEvent& e : w.takeEvents()) {
            romFired = romFired || (e.kind == WorldEvent::Shot && e.from == rom);
            fedFired = fedFired || (e.kind == WorldEvent::Shot && e.from == fed);
        }
        CHECK(romFired && fedFired);
    }
}

void testOrders() {
    Terrain t;
    t.width = t.height = 128;
    t.moveClass.assign(128 * 128, 1);
    auto shotsFrom = [](World& w, int id, bool& whileMoving) {
        int n = 0;
        for (const WorldEvent& e : w.takeEvents())
            if (e.kind == WorldEvent::Shot && e.from == id) {
                ++n;
                whileMoving = whileMoving || w.unit(id)->moving();
            }
        return n;
    };
    // A move fires on the way and keeps going; a cloaking unit's move
    // doesn't fire, its attack en route does.
    {
        World w(t);
        w.setStance(1, 0, 3);
        const int tank = spawnObject(w, 0, 'F', *combatType(8), 100000, 100000, {}, {}, true);
        spawnObject(w, 1, 'K', *combatType(37), 100000, 112000, {}); // a scout: unarmed, fired on
        w.select({tank});
        w.moveSelected(100000, 400000);
        bool moving = false;
        int shots = 0;
        for (int i = 0; i < 40; ++i) {
            w.advance(0.05);
            shots += shotsFrom(w, tank, moving);
        }
        CHECK(shots > 0 && moving && w.unit(tank)->y > 100500);
    }
    auto cloakedShots = [&](UnitOrder order) {
        World w(t);
        w.setStance(0, 2, 3);
        const int rom = spawnObject(w, 2, 'R', *combatType(35), 100000, 100000, {}, {}, true);
        spawnObject(w, 0, 'F', *combatType(97), 100000, 112000, {});
        w.select({rom});
        w.moveSelected(100000, 400000, order);
        bool moving = false;
        int shots = 0;
        for (int i = 0; i < 200; ++i) {
            w.advance(0.05);
            shots += shotsFrom(w, rom, moving);
        }
        return shots;
    };
    CHECK(cloakedShots(UnitOrder::Move) == 0 && cloakedShots(UnitOrder::EnRoute) > 0);
    // An assault fires from where the unit stands: out of range, it waits.
    {
        World w(t);
        w.setStance(0, 1, 3);
        const int tank = spawnObject(w, 0, 'F', *combatType(8), 100000, 100000, {}, {}, true);
        const int far = spawnObject(w, 1, 'K', *combatType(46), 100000, 160000, {});
        w.assault({tank}, far);
        w.advance(2);
        CHECK(w.unit(tank)->y == 100000 && w.unit(tank)->target == far);
    }
    // Protecting a spot: after whoever attacks the side, then back.
    {
        World w(t);
        w.setStance(1, 0, 3);
        w.setStance(0, 1, 3);
        const int guard = spawnObject(w, 0, 'F', *combatType(8), 100000, 100000, {}, {}, true);
        const int hub = spawnObject(w, 0, 'F', *combatType(23), 300000, 100000, {}, {}, true);
        w.protect({guard});
        const int raider = spawnObject(w, 1, 'K', *combatType(37), 300000, 115000, {}); // a scout
        w.unit(raider)->weapon = combatType(8)->weapon; // armed (weakly), to provoke
        w.unit(raider)->weapon.damage = 1;
        w.unit(raider)->hp = w.unit(raider)->maxHp = 6;
        bool left = false;
        for (int i = 0; i < 4000 && w.unit(raider); ++i) {
            w.advance(0.05);
            w.takeEvents();
            left = left || std::hypot(w.unit(guard)->x - 100000, w.unit(guard)->y - 100000) > 40000;
        }
        CHECK(left && !w.unit(raider) && w.unit(hub));
        for (int i = 0; i < 4000 && std::hypot(w.unit(guard)->x - 100000, w.unit(guard)->y - 100000) > 9000; ++i)
            w.advance(0.05);
        CHECK(std::hypot(w.unit(guard)->x - 100000, w.unit(guard)->y - 100000) <= 9000);
    }
    // The rally point: a 4 x 4 grid in the hub's block.
    {
        const auto [x0, y0] = rallySpot(20000, 40000, 0);
        const auto [x5, y5] = rallySpot(20000, 40000, 5);
        CHECK(x0 == 16384 + 2560 && y0 == 32768 + 2560 && x5 == x0 + 4096 && y5 == y0 + 4096);
    }
}

void testFog() {
    Terrain t;
    t.width = t.height = 64;
    t.moveClass.assign(64 * 64, 1);
    World w(t);
    Fog fog(32, 32);
    auto count = [&](Fog::Channel c, int slot) {
        int n = 0;
        for (int by = 0; by < 32; ++by)
            for (int bx = 0; bx < 32; ++bx)
                n += fog.has(bx, by, c, slot);
        return n;
    };
    const float block = 16384;
    // A scout (class 5): a disc of 37 blocks seen and surveyed, nothing scanned.
    const int scout = spawnObject(w, 0, 'F', *combatType(97), 10.5f * block, 10.5f * block, {}, {}, true);
    fog.refresh(w);
    CHECK(count(Fog::Seen, 0) == 37 && count(Fog::Surveyed, 0) == 37 && count(Fog::Scanned, 0) == 0);
    CHECK(fog.has(10, 13, Fog::Seen, 0) && !fog.has(10, 14, Fog::Seen, 0) && fog.has(13, 10, Fog::Seen, 0) &&
          !fog.has(13, 13, Fog::Seen, 0) && !fog.has(10, 10, Fog::Seen, 1));
    // A Klingon tank (class 3): 21 blocks, for its own side.
    spawnObject(w, 1, 'K', *combatType(10), 20.5f * block, 20.5f * block, {});
    // A science vehicle (class 4): the 3 x 3 round it scanned too.
    spawnObject(w, 0, 'F', *combatType(62), 25.5f * block, 5.5f * block, {}, {}, true);
    fog.refresh(w);
    CHECK(count(Fog::Seen, 1) == 21 && count(Fog::Scanned, 0) == 9 && fog.has(25, 5, Fog::Deep, 0));
    // The marks are of now: moved away, the old blocks clear within the
    // 64 ticks it takes to go round, a residue a tick.
    w.unit(scout)->x = 2.5f * block, w.unit(scout)->y = 25.5f * block;
    fog.tick(w);
    CHECK(fog.has(10, 10, Fog::Seen, 0) || fog.has(10, 11, Fog::Seen, 0));
    for (int i = 0; i < 64; ++i)
        fog.tick(w);
    CHECK(!fog.has(10, 10, Fog::Seen, 0) && fog.has(2, 25, Fog::Seen, 0) && fog.seenAt(2.5f * block, 25.5f * block, 0));
    CHECK(count(Fog::Seen, 0) == 37 + 34); // three of the scout's blocks off the map's edge
}

void testResearch() {
    auto names = [](const std::vector<ScriptEvent>& ev) {
        std::vector<std::string> out;
        for (const ScriptEvent& e : ev)
            out.push_back(e.kind + " " + e.args[0]);
        return out;
    };
    // F1: the plague topic opens with the script's one message; alone it
    // takes 150000 points at 1000 a tick: 150 ticks, 7.5 s.
    Research f1("F1", 0);
    CHECK(f1.topics().size() == 1 && f1.topics()[0].name == "taubat-colony" && f1.topics()[0].field == 1);
    CHECK(f1.offered().empty() && !f1.start(0));
    f1.unlock("taubat-colony-research");
    CHECK(f1.offered() == std::vector<size_t>{0});
    CHECK(f1.advance(10).empty()); // not started: nothing
    CHECK(f1.start(0) && !f1.start(0));
    CHECK(f1.advance(7.4).empty() && f1.topics()[0].running);
    const auto done = names(f1.advance(0.2));
    CHECK(done == (std::vector<std::string>{"researched taubat-colony", "researched taubat-colony-research"}));
    CHECK(f1.topics()[0].finished && f1.offered().empty());
    // J6: the biodata wants its name twice.
    Research j6("J6", 1);
    j6.unlock("metar-biodata");
    CHECK(j6.offered().empty());
    j6.unlock("metar-biodata");
    CHECK(j6.offered().size() == 1);
    // F2: two topics share the points: the 50000 one at 500 a tick (5 s),
    // then the 150000 one alone (100000 left: 5 s more).
    Research f2("F2", 0);
    f2.unlock("crash-site-research");
    f2.unlock("disease-research");
    for (size_t i : f2.offered())
        CHECK(f2.start(i));
    CHECK(f2.advance(4.9).empty());
    CHECK(names(f2.advance(0.2)) == (std::vector<std::string>{"researched disease", "researched disease-research"}));
    CHECK(f2.advance(4.8).empty());
    CHECK(names(f2.advance(0.2)).size() == 2);
    // A topic the mission doesn't have: open with its message, 100000 points.
    Research own("C4", 0);
    own.unlock("ruins");
    CHECK(own.offered().size() == 1 && own.topics()[0].points == 100000);
    // The script's action reaches the game.
    std::vector<MissionScript> parsed;
    CHECK(!parseMissionScripts("mission T F\non start\n  research taubat-colony-research\n", parsed));
    MissionRun run(parsed[0], nullptr);
    const auto out = run.fire({"start", {}});
    CHECK(std::any_of(out.begin(), out.end(), [](const MissionRun::Output& o) {
        return o.kind == MissionRun::Output::Research && o.text == "taubat-colony-research";
    }));
}

void testModelTextures() {
    std::vector<TextureFile> tex;
    for (const char* name : {"k_bop_a", "k_bop_b", "k_bop_g", "kcs", "kcs_g", "kcs_l", "k_bop2_a", "kwb"})
        tex.push_back({name, "kln", {0, 0}});
    tex.push_back({"kwb", "fed", {0, 0}}); // same name elsewhere: the model's own folder wins
    for (const char* name : {"fapc", "fapc_g", "fapc_l", "fconb", "fconb_a", "fhf_g", "fpdt", "fpdt_s", "fvpy", "fvpy",
                             "landpad"})
        tex.push_back({name, "fed", {0, 0}});
    auto names = [&](const char* model, size_t needed) {
        std::string out;
        for (const TextureFile* t : findModelTextures(model, tex, needed))
            out += (out.empty() ? "" : " ") + (t ? t->stem + (t->folder == "kln" ? "" : "@" + t->folder) : "?");
        return out;
    };
    CHECK(names("geo/kln/k_bop.geo", 3) == "k_bop_a k_bop_b k_bop_g"); // not k_bop2_a
    CHECK(names("geo/kln/k_bop_l.geo", 3) == "k_bop_a k_bop_b k_bop_g"); // falls back without _l
    CHECK(names("geo/kln/kcs_l.geo", 1) == "kcs_l");                     // its own texture first
    CHECK(names("geo/kln/kcs_s.geo", 1) == "?");                          // outlines don't fall back
    CHECK(names("geo/fed/kwb.geo", 1) == "kwb@fed");
    CHECK(names("geo/fed/fhf.geo", 3) == "fhf_g@fed ? ?");       // own textures from 0, _g included
    CHECK(names("geo/fed/fpdt.geo", 2) == "fpdt@fed ?");          // fpdt_s is the outline's
    CHECK(names("geo/fed/fvpy_a.geo", 1) == "fvpy@fed");          // duplicates count once
    CHECK(names("geo/fed/landpd_d.geo", 2) == "landpad@fed ?");   // a shortened name
    CHECK(names("geo/fed/fapc2.geo", 3) == "fapc@fed fapc_g@fed fapc_l@fed"); // a number apart
    CHECK(names("geo/fed/fapc2_l.geo", 1) == "fapc_l@fed");                   // its low-detail texture
    CHECK(names("geo/fed/fconbee.geo", 2) == "fconb@fed fconb_a@fed");        // the texture's name shortened
    // A door: one upright 1000-unit face showing the whole texture, number 1;
    // a roof face (number 2) isn't.
    GeoModel g;
    g.parts.resize(1);
    g.parts[0].vertices = {{0, 0, 0}, {1000, 0, 0}, {1000, 0, 1000}, {0, 0, 1000},
                           {0, 0, 1000}, {1000, 0, 1000}, {1000, 1000, 1000}, {0, 1000, 1000}};
    auto face = [&](uint32_t first, uint32_t slot) {
        GeoModel::Face f;
        for (uint32_t i = 0; i < 4; ++i)
            f.corners.push_back({first + i, float(i == 1 || i == 2), float(i >= 2), {0, 0, 0, 0, slot}});
        g.faces.push_back(f);
    };
    face(4, 0);
    face(0, 1);
    face(4, 2);
    CHECK(isDoorSlot(g, 1) && !isDoorSlot(g, 2) && !isDoorSlot(g, 3));
    std::vector<TextureFile> doors = {{"door3", "fed", {0, 0}}, {"kcdoor", "kln", {0, 0}}};
    std::vector<const TextureFile*> slots(3, nullptr);
    addDoorTextures(g, "geo/fed/fch.geo", doors, slots);
    CHECK(!slots[0] && slots[1] == &doors[0] && !slots[2]);
    doors.push_back({"fch_door", "fed", {0, 0}}); // a model's own door wins
    addDoorTextures(g, "geo/fed/fch.geo", doors, slots);
    CHECK(slots[1] == &doors[2]);
}

// The colony's transports in play (exe.md#transports): F1's two unarmed
// APCs start in the hub; with an accommodation block holding everyone, a
// building's crew sent off rides back to the hub and its crew asked for
// again rides out from the block, an APC driving each way and home.
void testTransports() {
    Terrain t;
    t.width = t.height = 128;
    t.moveClass.assign(size_t(t.width) * size_t(t.height), 1);
    t.heightAt = [](float, float) { return 0.0f; };
    ColonyMap map;
    map.depositKind = [](int, int) { return 0; };
    map.depositAmount = [](int, int) { return 0; };
    std::vector<ColonyBuilding> colony = MissionSim::placedColony("F1", 'F');
    colony.push_back({"accommodation", "", 446, 210}); // takes in the hub's whole pool
    MissionSim sim("F1", 'F', t, map, 64, 64, colony);
    CHECK(sim.transports() == 2 && sim.transportsHoused() == 2);
    for (const Unit& u : sim.world().units())
        CHECK(!(u.slot == sim.playerSlot() && u.name == "unarmed-apc"));
    const ColonyStructure* hub = sim.economy().hub();
    CHECK(hub != nullptr);
    if (!hub)
        return;
    CHECK(hub->pool == 0);
    int store = 0, crew = 0;
    for (const ColonyStructure& c : sim.economy().structures())
        if (c.type == "sickbay")
            store = c.id, crew = c.crew;
    CHECK(store > 0 && crew > 0);
    const int everyone = sim.economy().people();
    sim.economy().setCrewTarget(store, 0);
    sim.economy().setCrewTarget(store, crew);
    CHECK(sim.economy().rides().size() == 2);
    sim.advance(1);
    CHECK(sim.transportsHoused() == 0 && sim.transports() == 2);
    int out = 0;
    for (const Unit& u : sim.world().units())
        out += u.slot == sim.playerSlot() && u.name == "unarmed-apc";
    CHECK(out == 2);
    for (int i = 0; i < 300 && (!sim.economy().rides().empty() || sim.transportsHoused() < 2); ++i)
        sim.advance(1);
    CHECK(sim.economy().rides().empty() && sim.transportsHoused() == 2 && sim.economy().people() == everyone);
    for (const ColonyStructure& c : sim.economy().structures())
        if (c.id == store)
            CHECK(c.crew == crew);
}

// Building placement (exe.md#building-placement): footprints of a block,
// a tile or a quarter; building ground for the first two, open ground for
// mines, anything for a quarter; the map's outer ring out; seen blocks
// only; a block's parts taken by what stands there (another side's block
// closed to the smaller ones); no unit in the way.
void testPlacement() {
    Terrain t;
    t.width = t.height = 20; // 10 x 10 blocks
    t.moveClass.assign(400, 1);
    t.ground.assign(400, kBuildGround);
    for (int ty = 0; ty < 20; ++ty)
        for (int tx = 12; tx < 14; ++tx) // block column 6: open ground
            t.ground[size_t(ty) * 20 + size_t(tx)] = kOpenGround;
    World w(t);
    const float B = 16384;
    auto at = [&](float bx, float by) { return std::make_pair(bx * B, by * B); };
    auto test = [&](int type, std::pair<float, float> p, int slot = 0) {
        return placementTest(w, type, p.first, p.second, slot, [](int, int) { return true; });
    };
    CHECK(footprintOf(23) == Footprint::Block && footprintOf(53) == Footprint::Tile &&
          footprintOf(80) == Footprint::Quarter && footprintOf(45) == Footprint::Mine);
    CHECK(snapped(Footprint::Block, 3.2f * B, 3.9f * B) == std::make_pair(3.5f * B, 3.5f * B));
    CHECK(snapped(Footprint::Tile, 3.2f * B, 3.9f * B) == std::make_pair(3.25f * B, 3.75f * B));
    CHECK(test(23, at(0.5f, 3.5f)) == PlaceFail::Edge && test(23, at(9.5f, 3.5f)) == PlaceFail::Edge);
    CHECK(test(23, at(3.5f, 3.5f)) == PlaceFail::None);
    CHECK(test(23, at(6.5f, 3.5f)) == PlaceFail::Ground && test(45, at(6.5f, 3.5f)) == PlaceFail::None &&
          test(45, at(3.5f, 3.5f)) == PlaceFail::Ground && test(80, at(6.2f, 3.2f)) == PlaceFail::None);
    CHECK(placementTest(w, 23, 3.5f * B, 3.5f * B, 0, [](int, int) { return false; }) == PlaceFail::Unseen);
    // A hub in block 3,3 fills it; a yard tile beside it in block 3,4 fits,
    // and a gun's quarter next to the yard; another side's tile there doesn't.
    spawnObject(w, 0, 'F', *combatType(23), 3.5f * B, 3.5f * B, {});
    CHECK(test(53, at(3.25f, 3.25f)) == PlaceFail::Taken && test(80, at(3.1f, 3.1f)) == PlaceFail::Taken);
    spawnObject(w, 0, 'K', *combatType(53), 3.25f * B, 4.25f * B, {});
    CHECK(test(53, at(3.25f, 4.25f)) == PlaceFail::Taken && test(80, at(3.6f, 4.1f)) == PlaceFail::None);
    CHECK(test(53, at(3.75f, 4.75f), 1) == PlaceFail::Taken && test(53, at(3.75f, 4.75f), 0) == PlaceFail::None);
    CHECK(test(23, at(3.5f, 4.5f)) == PlaceFail::Taken);
    CHECK(placementBlocker(w, 82, 3.25f * B, 4.25f * B) != 0 && placementBlocker(w, 82, 4.25f * B, 4.25f * B) == 0);
    // A unit standing on the spot.
    spawnObject(w, 0, 'F', *combatType(8), 5.5f * B, 5.5f * B, {});
    CHECK(test(23, at(5.5f, 5.5f)) == PlaceFail::Units && test(23, at(4.5f, 7.5f)) == PlaceFail::None);
}

void testRasterizer() {
    Surface surf;
    surf.create(16, 16, PixelFormat::RGBA32);
    Rasterizer r;
    r.begin(surf);
    // Two triangles making the square 2..12; every pixel inside is drawn once.
    std::vector<int> hits(16 * 16, 0);
    auto count = [&](float, float, uint8_t* px) {
        ++hits[size_t(px - surf.pixels.data()) / 4];
        return true;
    };
    const ScreenVertex a{2, 2, 1, 0, 0}, b{12, 2, 1, 1, 0}, c{12, 12, 1, 1, 1}, d{2, 12, 1, 0, 1};
    r.drawTriangle(a, b, c, count);
    r.begin(surf); // fresh depth so the second triangle isn't hidden by equal depth
    r.drawTriangle(a, c, d, count);
    bool exact = true;
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
            exact &= hits[size_t(y * 16 + x)] == ((x >= 2 && x < 12 && y >= 2 && y < 12) ? 1 : 0);
    CHECK(exact);

    // The nearer triangle wins whichever is drawn first; u/v interpolate.
    for (int order = 0; order < 2; ++order) {
        r.begin(surf);
        const ScreenVertex n0{0, 0, 1.0f, 0, 0}, n1{16, 0, 1.0f, 1, 0}, n2{0, 16, 1.0f, 0, 1};
        const ScreenVertex f0{0, 0, 0.5f, 0, 0}, f1{16, 0, 0.5f, 0, 0}, f2{0, 16, 0.5f, 0, 0};
        auto paint = [](uint8_t value) {
            return [value](float, float, uint8_t* px) {
                px[0] = value;
                return true;
            };
        };
        if (order == 0) {
            r.drawTriangle(n0, n1, n2, paint(1));
            r.drawTriangle(f0, f1, f2, paint(2));
        } else {
            r.drawTriangle(f0, f1, f2, paint(2));
            r.drawTriangle(n0, n1, n2, paint(1));
        }
        CHECK(surf.row(2)[2 * 4] == 1);
    }
    float seenU = -1;
    r.begin(surf);
    r.drawTriangle({0, 0, 1, 0, 0}, {16, 0, 1, 1, 0}, {0, 16, 1, 0, 0},
                   [&](float u, float, uint8_t* px) {
                       if (px == surf.row(0) + 8 * 4)
                           seenU = u;
                       return true;
                   });
    CHECK(std::fabs(seenU - 8.5f / 16) < 1e-4f);
}

} // namespace

// Savestates (docs/savestates.md): a mission saved after two minutes and
// loaded into a fresh one carries on exactly as the first, for each race's
// first mission on a flat map (its AI, economies, script and all).
void testSaveState() {
    Terrain t;
    t.width = t.height = 128;
    t.moveClass.assign(size_t(t.width) * size_t(t.height), 1);
    t.heightAt = [](float, float) { return 0.0f; };
    ColonyMap map;
    map.depositKind = [](int bx, int by) { return (bx + by) % 9 == 0 ? 1 : 0; };
    map.depositAmount = [](int, int) { return 500; };
    auto state = [](MissionSim& sim) {
        save::Serializer s = save::Serializer::saver();
        sim.syncState(s);
        return s.take();
    };
    for (const auto& [mission, side] : {std::pair{"F1", 'F'}, std::pair{"K1", 'K'}, std::pair{"R1", 'R'}}) {
        const std::vector<ColonyBuilding> colony = MissionSim::placedColony(mission, side);
        MissionSim a(mission, side, t, map, 64, 64, colony);
        a.advance(120);
        const std::vector<uint8_t> saved = state(a);
        MissionSim b(mission, side, t, map, 64, 64, colony);
        save::Serializer in = save::Serializer::loader(saved);
        b.syncState(in);
        CHECK(in.ok() && in.atEnd());
        CHECK(state(b) == saved); // loaded whole
        a.advance(180);
        b.advance(180);
        CHECK(state(a) == state(b)); // and on the same way
        CHECK(a.world().units().size() == b.world().units().size() && a.clock() == b.clock());
        // Another mission's state isn't taken.
        MissionSim other("F2", 'F', t, map, 64, 64, MissionSim::placedColony("F2", 'F'));
        save::Serializer wrong = save::Serializer::loader(saved);
        other.syncState(wrong);
        CHECK(!wrong.ok());
    }
}

int main() {
    auto tmp = std::filesystem::temp_directory_path() / "reluva_cat_tests";
    testNormalizeName();
    testRead(tmp);
    testRejectsCorruption(tmp);
    testStringTable();
    testTexture();
    testGeo();
    testGmf();
    testMdl();
    testArf();
    testFif();
    testLevels();
    testBriefing();
    testMissionScripts();
    testColony();
    testStarSystem();
    testModelPose();
    testEconomy();
    testWorld();
    testCombat();
    testAiSides();
    testFieldWork();
    testResearch();
    testVehicleProduction();
    testCrew();
    testScriptEffects();
    testFog();
    testRepairsShieldsCloaking();
    testOrders();
    testRivalsAndGroups();
    testTraining();
    testBees();
    testStoresAndRazing();
    testTransports();
    testSaveState();
    testPlacement();
    testProjectiles();
    testModelTextures();
    testRasterizer();
    std::filesystem::remove_all(tmp);
    if (failures)
        std::fprintf(stderr, "%d check(s) failed\n", failures);
    else
        std::printf("All tests passed\n");
    return failures ? 1 : 0;
}
