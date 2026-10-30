// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// The New Worlds fuzz targets' seeds, made with the tests' own builders
// (test_newworlds_cat.cpp, compiled in with its main() renamed) and the
// small files its tests write out by hand:
//   reluva_fuzz_seeds_newworlds tests/fuzz/corpus

#define main reluva_newworlds_tests_main
#include "test_newworlds_cat.cpp"
#undef main

namespace {
std::filesystem::path gRoot;
void write(const std::string& target, const std::string& name, const std::vector<uint8_t>& bytes) {
    std::filesystem::create_directories(gRoot / target);
    std::ofstream(gRoot / target / name, std::ios::binary)
        .write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
}
std::vector<uint8_t> text(std::string_view s) { return {s.begin(), s.end()}; }
} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: reluva_fuzz_seeds_newworlds <corpus dir>\n");
        return 2;
    }
    gRoot = argv[1];
    Builder cat;
    cat.add(sampleItems());
    write("cat_archive", "sample.cat", cat.build());
    write("string_table", "four.stb", stringTableBytes({{1, u" 'Et Tu?'"}, {22, u"~STELLAR CARTOGRAPHY~"}, {5, u""}}, 3));
    write("texture", "format6.lbp", textureBytes(4, 3));
    write("texture", "keyed.lbp", textureBytes(3, 2, 5, 1));
    write("geo", "triangle.geo", triangleGeo());
    write("geo", "two_parts.geo", triangleGeo(false, true, true));
    write("gmf", "tiny.gmf", tinyGmf());
    write("levels", "level.dat",
          text("// format comment\r\nF1,2,Power Corrupts\r\n// J1,6,Defiant\r\nJ1,3,Into the Arena\r\n\r\n"
               "//C9,2,(Bonus) \"Slaver's Run\"  \r\nF2,two,Bad\n"));
    // As testMdl, testArf and testFif build theirs.
    {
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
        put32(b, 0x150); put32(b, 0x009a0300);
        put32(b, 1); name("gun_muzzle_1", 0); name("terminate", 0xffffffffu);
        put32(b, 0); name("terminate", 0xffffffffu);
        put32(b, 0); name("terminate", 0xffffffffu);
        put32(b, 0); put32(b, 2); name("normal", 0); name("low detail", 1); name("terminate", 0xffffffffu);
        write("mdl", "test.mdl", b);
    }
    {
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
        write("arf", "two.arf", b);
    }
    {
        // The fif target's input: the font's length, the font, its image.
        std::vector<uint8_t> font(std::begin("FONTPROP"), std::end("FONTPROP") - 1);
        font.push_back(32); font.push_back(0); font.push_back(3); font.push_back(0);
        put32(font, 2); put32(font, 4); put32(font, 0);
        for (uint32_t v : {0u, 0u, 2u, 4u, 2u, 0u, 1u, 4u, 0u, 0u, 0u, 0u})
            put32(font, v);
        const std::vector<uint8_t> image = textureBytes(4, 4);
        std::vector<uint8_t> b;
        put16(b, uint16_t(font.size()));
        b.insert(b.end(), font.begin(), font.end());
        b.insert(b.end(), image.begin(), image.end());
        write("fif", "three.fif", b);
    }
    // A state saved two minutes into F1, as mission_state.cpp loads it.
    {
        Terrain t;
        t.width = t.height = 128;
        t.moveClass.assign(size_t(t.width) * size_t(t.height), 1);
        t.heightAt = [](float, float) { return 0.0f; };
        ColonyMap map;
        map.depositKind = [](int bx, int by) { return (bx + by) % 9 == 0 ? 1 : 0; };
        map.depositAmount = [](int, int) { return 500; };
        MissionSim sim("F1", 'F', t, map, 64, 64, MissionSim::placedColony("F1", 'F'));
        sim.advance(120);
        save::Serializer s = save::Serializer::saver();
        sim.syncState(s);
        write("mission_state", "f1_120s.state", s.take());
    }
    return 0;
}
