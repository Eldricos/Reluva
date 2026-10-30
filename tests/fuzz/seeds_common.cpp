// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// The common/ fuzz targets' seeds, made with the tests' own builders
// (test_common.cpp, compiled in with its main() renamed; savestate.cpp for
// the layout its target loads) and the small files the tests write out:
//   reluva_fuzz_seeds_common tests/fuzz/corpus

#define main reluva_common_tests_main
#include "test_common.cpp"
#undef main
#include "fuzz/savestate.cpp"

#include <cmath>

namespace {
std::filesystem::path gRoot;
void write(const std::string& target, const std::string& name, const std::vector<uint8_t>& bytes) {
    std::filesystem::create_directories(gRoot / target);
    std::ofstream(gRoot / target / name, std::ios::binary)
        .write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
}
void put32(std::vector<uint8_t>& b, uint32_t v) {
    for (int i = 0; i < 4; ++i)
        b.push_back(uint8_t(v >> (8 * i)));
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: reluva_fuzz_seeds_common <corpus dir>\n");
        return 2;
    }
    gRoot = argv[1];
    // Bink: the 8x8 movies, and one with an audio track (as testBink's,
    // its packet a sane size: a sample count and nothing more).
    write("bink", "fill.bik", binkMovie(false));
    write("bink", "scaled.bik", binkMovie(true));
    for (const uint8_t flags : {0x00, 0x10}) { // RDFT, DCT
        const std::vector<uint8_t> plain = binkMovie(false);
        std::vector<uint8_t> file(plain.begin(), plain.begin() + 44);
        file[40] = 1;
        const uint8_t track[12] = {0, 0, 0, 0, 0x22, 0x56, 0, flags, 0, 0, 0, 0}; // 22050 Hz, mono
        file.insert(file.end(), track, track + 12);
        put32(file, 60);
        std::vector<uint8_t> packet;
        put32(packet, 8);
        put32(packet, 512);
        put32(packet, 0);
        file.insert(file.end(), packet.begin(), packet.end());
        file.insert(file.end(), plain.begin() + 48, plain.end());
        const uint32_t total = uint32_t(file.size() - 8), largest = uint32_t(file.size() - 60);
        for (int i = 0; i < 4; ++i)
            file[4 + size_t(i)] = uint8_t(total >> (8 * i)), file[12 + size_t(i)] = uint8_t(largest >> (8 * i));
        write("bink", flags ? "audio_dct.bik" : "audio_rdft.bik", file);
    }
    // Sound: the MP3 tone (and behind an ID3v2 tag), 16-bit PCM and testWav's
    // IMA ADPCM block.
    const std::vector<uint8_t> tone(std::begin(kTone440Mp3), std::end(kTone440Mp3));
    write("sound", "tone440.mp3", tone);
    std::vector<uint8_t> tagged = {'I', 'D', '3', 3, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0};
    tagged.insert(tagged.end(), tone.begin(), tone.end());
    write("sound", "tagged.mp3", tagged);
    audio::Sound pcm;
    pcm.rate = 11025, pcm.channels = 2, pcm.format = 1;
    for (int i = 0; i < 64; ++i)
        pcm.samples.push_back(int16_t(8000 * std::sin(i * 0.3)));
    write("sound", "stereo16.wav", audio::encodePcmWav(pcm));
    {
        std::vector<uint8_t> b;
        auto put16 = [&](uint16_t v) { b.push_back(uint8_t(v)); b.push_back(uint8_t(v >> 8)); };
        auto tag = [&](const char* t) { b.insert(b.end(), t, t + 4); };
        tag("RIFF"); put32(b, 0); tag("WAVE");
        tag("fmt "); put32(b, 20);
        put16(0x11); put16(1); put32(b, 22050); put32(b, 22050); put16(8); put16(4); put16(2); put16(9);
        tag("fact"); put32(b, 4); put32(b, 5);
        tag("data"); put32(b, 8);
        for (uint8_t v : {100, 0, 0, 0, 0x04, 0, 0, 0})
            b.push_back(v);
        write("sound", "adpcm.wav", b);
    }
    // Savestate: a state as its target loads it, alone and in a file.
    {
        State st;
        st.things.resize(2);
        st.things[0].names = {"a", "bc"};
        st.things[0].maybe = 2.5;
        st.things[1].entry = &kTable[2];
        st.flags = {{1, {true, false, true}}, {5, {}}};
        st.queues["q"] = {1, -2};
        st.ids = {3, 9};
        st.owned = std::make_unique<Thing>();
        st.blob = {1, 2, 3};
        save::Serializer s = save::Serializer::saver();
        s(st);
        const std::vector<uint8_t> state = s.take();
        write("savestate", "state.bin", state);
        save::Header h;
        h.engineId = "newworlds", h.gameId = "stnw", h.description = "seed", h.reluvaVersion = "0.1.0";
        h.thumbWidth = 4, h.thumbHeight = 3; // (a small one, to keep the seed small)
        h.thumbnail.assign(4 * 3 * 4, 128);
        const auto path = std::filesystem::temp_directory_path() / "reluva_fuzz_seed.rlvs";
        if (save::writeFile(path, h, state)) {
            std::fprintf(stderr, "couldn't write %s\n", path.string().c_str());
            return 1;
        }
        std::ifstream in(path, std::ios::binary);
        write("savestate", "file.rlvs", {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()});
        std::filesystem::remove(path);
    }
    return 0;
}
