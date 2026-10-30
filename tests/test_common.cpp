// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// Small self-contained tests for common/. No framework, to keep deps at zero.

#include "common/audio/decode.h"
#include "common/audio/mixer.h"
#include "common/audio/mp3.h"
#include "common/audio/wav.h"
#include "data/tone440_mp3.h"
#include "common/config.h"
#include "common/detection.h"
#include "common/font.h"
#include "common/fs.h"
#include "common/md5.h"
#include "common/text.h"
#include "common/video/bink.h"
#include "common/savestate.h"

#include <algorithm>
#include <cstdio>
#include <vector>
#include <optional>
#include <map>
#include <cstring>
#include <memory>
#include <fstream>
#include <iterator>
#include <string>

using namespace reluva;

static int failures = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, \
                         __LINE__, #cond);                              \
            ++failures;                                                 \
        }                                                               \
    } while (0)

static std::string md5Of(const std::string& s) {
    Md5 m;
    m.update(s.data(), s.size());
    return m.hexDigest();
}

static void testMd5() {
    CHECK(md5Of("") == "d41d8cd98f00b204e9800998ecf8427e");
    CHECK(md5Of("abc") == "900150983cd24fb0d6963f7d28e17f72");
    CHECK(md5Of("The quick brown fox jumps over the lazy dog") == "9e107d9d372bb6826bd81d3542a419d6");
    // Crosses the 56-byte padding boundary.
    CHECK(md5Of(std::string(64, 'a')) == "014842d480b571495a4a0363793f7367");
}

static void testFsAndDetection(const std::filesystem::path& tmp) {
    std::filesystem::remove_all(tmp);
    std::filesystem::create_directories(tmp / "Data");
    {
        std::ofstream(tmp / "Data" / "GAME.DAT", std::ios::binary) << "abc";
    }

    auto found = fs::findFile(tmp, "data/game.dat");
    CHECK(found.has_value());
    CHECK(!fs::findFile(tmp, "data/missing.dat"));
    CHECK(fs::md5File(*found, kDetectionHashBytes) == "900150983cd24fb0d6963f7d28e17f72");

    std::vector<GameVariant> table = {
        {"good", "Good", "en", "windows", {{"DATA/game.dat", "900150983cd24fb0d6963f7d28e17f72", 3}}},
        {"badsize", "Bad size", "en", "windows", {{"data/game.dat", "", 4}}},
        {"badmd5", "Bad md5", "en", "windows", {{"data/game.dat", "00000000000000000000000000000000", -1}}},
    };
    auto results = detectFromTable("eng", table, tmp);
    CHECK(results.size() == 1);
    CHECK(!results.empty() && results[0].gameId == "good");

    // UTF-8 names through paths and back (what SDL and the config hold).
    const std::string cafe = "Caf\xc3\xa9 \xe2\x82\xac";
    const std::filesystem::path dir = tmp / fs::fromUtf8(cafe);
    std::filesystem::create_directories(dir);
    { std::ofstream(dir / "X.BIN") << "x"; }
    CHECK(fs::toUtf8(dir.filename()) == cafe);
    CHECK(fs::findFile(tmp, cafe + "/x.bin").has_value());
    CHECK(fs::toUtf8(fs::fromUtf8("a/b")) == "a/b");
    // replaceFile: a new file, then over it; nothing left beside it.
    CHECK(!fs::replaceFile(dir / "r.txt", "one"));
    CHECK(!fs::replaceFile(dir / "r.txt", "two\n"));
    std::ifstream in(dir / "r.txt", std::ios::binary);
    std::string got((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHECK(got == "two\n" && !std::filesystem::exists(dir / "r.txt.tmp"));
    CHECK(bool(fs::replaceFile(tmp / "missing" / "r.txt", "x")));

    std::filesystem::remove_all(tmp);
}

static void testConfig(const std::filesystem::path& tmp) {
    std::filesystem::create_directories(tmp);
    auto path = tmp / "test.ini";
    {
        Config c;
        c.load(path);
        c.set(Config::kGlobalSection, "last_target", "foo");
        c.set("foo", "path", "/games/foo bar");
        CHECK(!c.save());
    }
    Config c;
    CHECK(!c.load(path));
    CHECK(c.get("foo", "path") == "/games/foo bar");
    CHECK(c.targets().size() == 1);
    CHECK(c.get("foo", "missing", "dflt") == "dflt");

    // Values that wouldn't read back as written are quoted; the rest, and
    // files from before, read as they stand.
    const std::map<std::string, std::string> odd = {
        {"spaces", "  C:\\Games\\New Worlds  "}, {"lines", "one\ntwo\r\n"}, {"quoted", "\"hi\" \\"},
        {"plain", "C:\\Games\\x \"y\""}, {"empty", ""}, {"tab", "\tx"}, {"utf8", "Caf\xc3\xa9"}};
    for (const auto& [k, v] : odd)
        c.set("foo", k, v);
    CHECK(!c.save());
    Config d;
    CHECK(!d.load(path));
    for (const auto& [k, v] : odd)
        CHECK(d.get("foo", k, "?") == v);
    CHECK(!std::filesystem::exists(tmp / "test.ini.tmp"));
    {
        std::ofstream(path) << "[old]\r\npath = C:\\Games\\Old \r\nname=\"x\\ny\"\r\n";
    }
    CHECK(!d.load(path) && d.get("old", "path") == "C:\\Games\\Old" && d.get("old", "name") == "x\ny");

    // One that's there but can't be read isn't saved over.
    const auto unreadable = tmp / "dir.ini";
    std::filesystem::create_directories(unreadable);
    Config u;
    CHECK(bool(u.load(unreadable)));
    u.set("foo", "path", "x");
    CHECK(bool(u.save()) && std::filesystem::is_directory(unreadable));
    Config fresh;
    CHECK(!fresh.load(tmp / "none.ini") && fresh.targets().empty());
    std::filesystem::remove_all(tmp);
}

static void testUtf16() {
    const uint16_t hello[] = {'H', 'i', ' ', 0x00e9, 0x20ac, 0xd83d, 0xde80}; // e-acute, euro, rocket
    CHECK(text::utf16leToUtf8(hello, 7) == "Hi \xc3\xa9\xe2\x82\xac\xf0\x9f\x9a\x80");
    const uint16_t lone[] = {0xd800, 'x'};
    CHECK(text::utf16leToUtf8(lone, 2) == "\xef\xbf\xbdx");
    CHECK(text::utf16leToUtf8(nullptr, 0).empty());
}

static void testFont() {
    using namespace reluva;
    CHECK(font::length("Caf\xc3\xa9 \xe2\x80\x99") == 6); // é and ’ are one character each
    Surface s;
    s.create(20, 12, PixelFormat::RGBA32);
    const int end = font::drawText(s, 1, 1, "I|", {255, 0, 0});
    CHECK(end == 1 + 2 * font::kAdvance);
    // 'I' has its middle column set from row 1 to 5 and a bar on row 0.
    auto red = [&](int x, int y) { return s.row(y)[x * 4] == 255 && s.row(y)[x * 4 + 1] == 0; };
    CHECK(red(3, 1) && red(2, 1) && red(4, 1) && !red(1, 2) && red(3, 4) && !red(0, 0));
    CHECK(red(9, 7) && !red(9, 8)); // '|' runs rows 0-6
    font::drawText(s, 15, 10, "W", {255, 0, 0}, 2); // clipped, not out of bounds
}

static void testWav() {
    using namespace reluva::audio;
    // One 8-byte IMA ADPCM block: header (sample 100, index 0), then nibbles
    // 4, 0, 0, ... (low nibble first); the fact chunk keeps 5 frames.
    std::vector<uint8_t> b;
    auto put32 = [](std::vector<uint8_t>& v, uint32_t x) {
        for (int i = 0; i < 4; ++i)
            v.push_back(uint8_t(x >> (8 * i)));
    };
    auto put16 = [&](uint16_t v) { b.push_back(uint8_t(v)); b.push_back(uint8_t(v >> 8)); };
    auto tag = [&](const char* t) { b.insert(b.end(), t, t + 4); };
    tag("RIFF"); put32(b, 0); tag("WAVE");
    tag("fmt "); put32(b, 20);
    put16(0x11); put16(1); put32(b, 22050); put32(b, 22050); put16(8); put16(4); put16(2); put16(9);
    tag("fact"); put32(b, 4); put32(b, 5);
    tag("data"); put32(b, 8);
    for (uint8_t v : {100, 0, 0, 0, 0x04, 0, 0, 0})
        b.push_back(v);
    Sound s;
    CHECK(!decodeWav(b, s));
    CHECK(s.rate == 22050 && s.channels == 1 && s.format == 0x11);
    CHECK((s.samples == std::vector<int16_t>{100, 107, 108, 109, 109}));
    // Back through 16-bit PCM unchanged.
    Sound pcm;
    CHECK(!decodeWav(encodePcmWav(s), pcm));
    CHECK(pcm.format == 1 && pcm.rate == 22050 && pcm.samples == s.samples);
    // A chunk claiming nearly 4 GB ends the walk (it doesn't wrap round).
    std::vector<uint8_t> big(b.begin(), b.begin() + 40);
    tag("junk"), put32(b, 0xFFFFFFF0u);
    big.insert(big.end(), b.end() - 8, b.end());
    CHECK(bool(decodeWav(big, s)));
    b.resize(20);
    CHECK(bool(decodeWav(b, s))); // cut inside the fmt chunk
}

static void testMp3() {
    using namespace reluva::audio;
    const std::vector<uint8_t> tone(std::begin(kTone440Mp3), std::end(kTone440Mp3));
    Sound s;
    CHECK(!decodeMp3(tone, s));
    CHECK(s.format == kFormatMp3 && s.rate == 22050 && s.channels == 1);
    // About 0.2 s (whole frames of 576 samples, plus the encoder's delay).
    CHECK(s.frames() >= 4410 && s.frames() <= 4410 + 3 * 576);
    // 440 Hz: about 2 x 440 x 0.1 = 88 zero crossings in the middle 0.1 s.
    int crossings = 0;
    for (size_t i = 1102; i < 3307; ++i)
        crossings += (s.samples[i - 1] < 0) != (s.samples[i] < 0);
    CHECK(crossings >= 84 && crossings <= 92);
    // The same after an ID3v2 tag (header, 5 bytes of padding), and through decodeSound.
    std::vector<uint8_t> tagged = {'I', 'D', '3', 3, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0};
    tagged.insert(tagged.end(), tone.begin(), tone.end());
    Sound t;
    CHECK(!decodeSound(tagged, t) && t.samples == s.samples);
    CHECK(bool(decodeMp3(std::vector<uint8_t>(100, 0), t))); // no frames
}

static void testMixer() {
    using namespace reluva::audio;
    auto make = [](int rate, int channels, std::vector<int16_t> samples) {
        auto s = std::make_shared<Sound>();
        s->rate = rate, s->channels = channels, s->samples = std::move(samples);
        return std::shared_ptr<const Sound>(s);
    };
    std::vector<int16_t> out(40);
    Mixer m(22050);
    // A sound at the output rate plays sample for sample, then stops and frees its channel.
    const SoundHandle a = m.play(make(22050, 1, std::vector<int16_t>(10, 1000)), SoundType::Effects);
    CHECK(a != kNoSound && m.isPlaying(a));
    m.mix(out.data(), 12);
    CHECK(out[0] == 1000 && out[1] == 1000 && out[18] == 1000 && out[19] == 1000 && out[20] == 0 && out[23] == 0);
    CHECK(!m.isPlaying(a));
    m.mix(out.data(), 4);
    CHECK(out[0] == 0 && out[7] == 0); // silence with nothing playing
    // Two sounds add up, and clamp at 16 bits.
    m.play(make(22050, 1, {30000, 1000}), SoundType::Effects);
    m.play(make(22050, 1, {30000, -3000}), SoundType::Music);
    m.mix(out.data(), 2);
    CHECK(out[0] == 32767 && out[2] == -2000 && out[3] == -2000);
    // Volumes: the sound's and its type's multiply.
    m.setTypeVolume(SoundType::Music, kMaxVolume / 2);
    m.play(make(22050, 1, {1000}), SoundType::Music, kMaxVolume / 2);
    m.mix(out.data(), 1);
    CHECK(out[0] == 250 && out[1] == 250 && m.typeVolume(SoundType::Music) == 128);
    m.setTypeVolume(SoundType::Music, kMaxVolume);
    // Pan: full right silences the left.
    m.play(make(22050, 1, {1000}), SoundType::Speech, kMaxVolume, kMaxPan);
    m.mix(out.data(), 1);
    CHECK(out[0] == 0 && out[1] == 1000);
    // Resampling: an 11025 Hz ramp at 22050 Hz gets a frame in between each.
    const SoundHandle r = m.play(make(11025, 1, {0, 100, 200, 300}), SoundType::Effects);
    m.mix(out.data(), 4);
    CHECK(out[0] == 0 && out[2] == 50 && out[4] == 100 && out[6] == 150 && m.position(r) == 2.0);
    m.stopAll();
    // Stereo keeps its channels; a loop repeats until stopped.
    const SoundHandle l = m.play(make(22050, 2, {10, -10, 20, -20}), SoundType::Music, kMaxVolume, 0, true);
    m.mix(out.data(), 5);
    CHECK(out[0] == 10 && out[1] == -10 && out[2] == 20 && out[3] == -20 && out[4] == 10 && out[8] == 10);
    CHECK(m.isPlaying(l) && m.isTypePlaying(SoundType::Music) && !m.isTypePlaying(SoundType::Speech));
    m.stopType(SoundType::Music);
    m.mix(out.data(), 2);
    CHECK(!m.isPlaying(l) && out[0] == 0);
    // A stream plays what it's given, waits in silence, and ends once
    // finished and played out.
    const SoundHandle st = m.playStream(22050, 1, SoundType::Music);
    const int16_t part[2] = {500, 600};
    m.append(st, part, 2);
    m.mix(out.data(), 3);
    CHECK(out[0] == 500 && out[2] == 600 && out[4] == 0 && m.isPlaying(st));
    m.append(st, part, 1);
    m.finishStream(st);
    m.mix(out.data(), 2);
    CHECK(out[0] == 500 && out[2] == 0 && !m.isPlaying(st));
    // Across chunks, resampled, the played ones let go: the ramp goes on.
    const SoundHandle ramp = m.playStream(11025, 1, SoundType::Music);
    bool smooth = true;
    int16_t next = 0;
    for (int k = 0; k < 500; ++k) {
        const int16_t chunk[3] = {int16_t(next), int16_t(next + 2), int16_t(next + 4)};
        next = int16_t(next + 6);
        m.append(ramp, chunk, 3);
        std::vector<int16_t> o(12);
        m.mix(o.data(), 6); // 3 frames' worth at twice the rate
        for (int i = 0; i < 6; ++i) // (the last holds: the frame after isn't there yet)
            smooth = smooth && o[i * 2] == int16_t(6 * k + std::min(i, 4));
    }
    CHECK(smooth && m.position(ramp) == 1500.0);
    m.stopAll();
    // Nothing to play.
    CHECK(m.play(nullptr, SoundType::Effects) == kNoSound && m.play(make(22050, 1, {}), SoundType::Effects) == kNoSound);
}

namespace savetest {
enum class Kind : uint8_t { A, B };
struct Thing {
    int id = 0;
    float x = 0;
    Kind kind = Kind::A;
    std::vector<std::string> names;
    std::optional<double> maybe;
};
void syncState(save::Serializer& s, Thing& t) { s(t.id)(t.x)(t.kind)(t.names)(t.maybe); }
} // namespace savetest

static void testSavestate(const std::filesystem::path& tmp) {
    using savetest::Thing;
    // A round trip through the serializer: the same values back, every byte used.
    std::vector<Thing> things(2);
    things[0] = {7, 1.5f, savetest::Kind::B, {"a", "bc"}, 2.25};
    things[1].id = -3;
    std::map<int, std::vector<bool>> flags{{1, {true, false, true}}, {5, {}}};
    std::unique_ptr<Thing> owned = std::make_unique<Thing>(things[0]);
    int64_t big = -1234567890123;
    uint8_t grid[2][3] = {{1, 2, 3}, {4, 5, 6}};
    save::Serializer out = save::Serializer::saver();
    out(things)(flags)(owned)(big)(grid);
    std::vector<Thing> things2;
    std::map<int, std::vector<bool>> flags2;
    std::unique_ptr<Thing> owned2;
    int64_t big2 = 0;
    uint8_t grid2[2][3] = {};
    save::Serializer in = save::Serializer::loader(out.bytes());
    in(things2)(flags2)(owned2)(big2)(grid2);
    CHECK(in.ok() && in.atEnd());
    CHECK(things2.size() == 2 && things2[0].id == 7 && things2[0].x == 1.5f && things2[0].kind == savetest::Kind::B);
    CHECK(things2[0].names == std::vector<std::string>({"a", "bc"}) && things2[0].maybe == 2.25 && !things2[1].maybe);
    CHECK(flags2 == flags && owned2 && owned2->names.size() == 2 && big2 == big && grid2[1][2] == 6);
    // Cut short, or with a silly length: it fails instead of reading past the end.
    std::vector<uint8_t> cut(out.bytes().begin(), out.bytes().begin() + 10);
    save::Serializer short_ = save::Serializer::loader(cut);
    things2.clear();
    short_(things2)(flags2);
    CHECK(!short_.ok());
    save::Serializer huge = save::Serializer::loader({0xFF, 0xFF, 0xFF, 0x7F});
    std::vector<int> v;
    huge(v);
    CHECK(!huge.ok() && v.empty());
    // A count each element can't have its bytes for (an int takes 4).
    save::Serializer few = save::Serializer::loader({3, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8});
    few(v);
    CHECK(!few.ok() && v.empty());
    // One that the bytes could back if elements were a byte each: grown as
    // they're read, not all at once (a Thing takes 14 here).
    std::vector<uint8_t> zeros(1 << 20);
    zeros[0] = 0xFF, zeros[1] = 0xFF, zeros[2] = 0x0F;
    save::Serializer many = save::Serializer::loader(zeros);
    many(things2);
    CHECK(!many.ok() && things2.size() <= (1 << 20) / 14 && things2.capacity() < 200000);

    // Strings and byte lists are their bytes in order, numbers little-
    // endian, on any host (the files are the same everywhere).
    save::Serializer bytes = save::Serializer::saver();
    std::string abc = "abc";
    std::vector<uint8_t> blob = {9, 8};
    uint16_t word = 0x0102;
    bytes(abc)(blob)(word);
    CHECK((bytes.bytes() == std::vector<uint8_t>{3, 0, 0, 0, 'a', 'b', 'c', 2, 0, 0, 0, 9, 8, 2, 1}));
    std::string abc2;
    std::vector<uint8_t> blob2;
    uint16_t word2 = 0;
    save::Serializer bytesIn = save::Serializer::loader(bytes.bytes());
    bytesIn(abc2)(blob2)(word2);
    CHECK(bytesIn.ok() && bytesIn.atEnd() && abc2 == abc && blob2 == blob && word2 == word);

    // A checked enum: one past the last refused.
    save::Serializer kinds = save::Serializer::saver();
    savetest::Kind kind = savetest::Kind::B;
    save::syncEnum(kinds, kind, savetest::Kind::B);
    kind = savetest::Kind::A;
    save::Serializer kindIn = save::Serializer::loader(kinds.bytes());
    save::syncEnum(kindIn, kind, savetest::Kind::B);
    CHECK(kindIn.ok() && kind == savetest::Kind::B);
    save::Serializer kindBad = save::Serializer::loader(kinds.bytes());
    save::syncEnum(kindBad, kind, savetest::Kind::A);
    CHECK(!kindBad.ok() && kind == savetest::Kind::A);

    // Files: the header alone, or with the state; a damaged one refused.
    const auto dir = tmp / "saves";
    std::filesystem::remove_all(dir);
    save::Header h;
    h.engineId = "testbed", h.gameId = "testbed", h.target = "testbed", h.stateVersion = 3;
    h.description = "Testing", h.created = 1793000000;
    std::vector<uint8_t> screen(64 * 48 * 4, 200);
    save::makeThumbnail(screen.data(), 64, 48, 64 * 4, h);
    CHECK(h.thumbWidth == 160 && h.thumbHeight == 120 && h.thumbnail.size() == 160 * 120 * 4 && h.thumbnail[5] == 200);
    CHECK(save::freeSlot(dir) == 1);
    CHECK(save::writeFile(save::slotPath(dir, 1), h, out.bytes()).ok);
    CHECK(save::writeFile(save::slotPath(dir, 4), h, {}).ok);
    CHECK(save::freeSlot(dir) == 2);
    const std::vector<save::Slot> slots = save::listSlots(dir);
    CHECK(slots.size() == 2 && slots[0].number == 1 && slots[1].number == 4 && slots[0].header.description == "Testing");
    save::Header back;
    std::vector<uint8_t> state;
    CHECK(save::readFile(save::slotPath(dir, 1), back, state).ok && state == out.bytes() && back.stateVersion == 3);
    {
        std::fstream f(save::slotPath(dir, 1), std::ios::in | std::ios::out | std::ios::binary);
        f.seekp(-6, std::ios::end);
        f.put('\x55');
    }
    CHECK(!save::readFile(save::slotPath(dir, 1), back, state).ok);
    CHECK(save::readHeader(save::slotPath(dir, 1), back).ok); // the header is still fine
    std::ofstream(dir / "slot05.rlvs") << "nonsense";
    CHECK(save::listSlots(dir).size() == 3 && !save::listSlots(dir)[2].error.empty());
}

// A one-frame 8x8 Bink movie (version i) built bit by bit: each plane a
// single fill block, or (scaled) a 16x16 fill block, which on an 8x8 plane
// must keep to its one 8x8 quarter.
static std::vector<uint8_t> binkMovie(bool scaled, uint32_t headerSize = 0) {
    std::vector<uint8_t> bytes;
    uint64_t acc = 0;
    int nbits = 0;
    auto put = [&](uint32_t v, int n) { // low bit first
        acc |= uint64_t(v) << nbits;
        nbits += n;
        while (nbits >= 8) {
            bytes.push_back(uint8_t(acc));
            acc >>= 8, nbits -= 8;
        }
    };
    auto align32 = [&] {
        while (nbits || bytes.size() % 4)
            put(0, 1);
    };
    put(0, 32); // the frame's leading word (version i)
    bool first = true;
    for (const int colour : {0x5A, 0xC0, 0x40}) { // Y, then V and U from version h
        put(0, 4 * 23);                           // every tree the plain one
        if (scaled && first) {
            put(1, 10), put(1, 1), put(1, 4); // block types: one, scaled
            put(1, 9), put(1, 1), put(6, 4);  // 16x16 block types: one, fill
        } else {
            put(1, 10), put(1, 1), put(6, 4); // block types: one, all fill
            put(0, 9);                        // no 16x16 block types
        }
        put(1, 10), put(1, 1), put(colour >> 4, 4), put(colour & 15, 4); // one colour
        for (int i = 0; i < 6; ++i)
            put(0, 10); // no patterns, motion, DCs or runs
        align32();
        first = false;
    }
    std::vector<uint8_t> file(48);
    auto le = [&](size_t at, uint32_t v) {
        for (int i = 0; i < 4; ++i)
            file[at + i] = uint8_t(v >> (8 * i));
    };
    std::memcpy(file.data(), "BIKi", 4);
    le(4, headerSize ? headerSize : uint32_t(48 + bytes.size() - 8)), le(8, 1), le(12, uint32_t(bytes.size()));
    le(20, 8), le(24, 8), le(28, 15), le(32, 1), le(36, 0), le(40, 0), le(44, 48);
    file.insert(file.end(), bytes.begin(), bytes.end());
    return file;
}

static void testBink(const std::filesystem::path& tmp) {
    std::filesystem::create_directories(tmp);
    auto write = [&](const char* name, const std::vector<uint8_t>& file) {
        const auto path = tmp / name;
        std::ofstream(path, std::ios::binary)
            .write(reinterpret_cast<const char*>(file.data()), std::streamsize(file.size()));
        return path;
    };
    const auto path = write("fill.bik", binkMovie(false));

    video::BinkMovie movie;
    CHECK(movie.open(path).ok);
    CHECK(movie.width() == 8 && movie.height() == 8 && movie.frames() == 1 && movie.fps() == 15 && !movie.hasAudio());
    CHECK(movie.nextFrame().ok && movie.frame() == 1);
    CHECK(movie.plane(0)[0] == 0x5A && movie.plane(0)[63] == 0x5A);
    CHECK(movie.plane(1)[0] == 0x40 && movie.plane(2)[0] == 0xC0 && movie.plane(1)[movie.stride(1) * 3 + 3] == 0x40);
    std::vector<uint8_t> rgba(8 * 8 * 4);
    movie.toRgba(rgba.data(), 8 * 4);
    CHECK(rgba[0] > rgba[1] && rgba[0] > rgba[2] && rgba[3] == 255); // red-ish (V high, U low)
    CHECK(!movie.nextFrame().ok); // past the end
    CHECK(!movie.open(tmp / "missing.bik").ok);

    // A 16x16 block on an 8x8 plane: filled, and nothing written past it.
    CHECK(movie.open(write("scaled.bik", binkMovie(true))).ok);
    CHECK(movie.nextFrame().ok && movie.plane(0).size() == 64 && movie.plane(0)[0] == 0x5A &&
          movie.plane(0)[63] == 0x5A);

    // A header claiming a far bigger file: its size is the real one.
    CHECK(movie.open(write("big.bik", binkMovie(false, 0x7FFFFFF0))).ok);
    CHECK(movie.nextFrame().ok && movie.plane(0)[0] == 0x5A);

    // An audio track whose packet claims 4 GB: an error, not an allocation.
    {
        const std::vector<uint8_t> plain = binkMovie(false);
        std::vector<uint8_t> file(plain.begin(), plain.begin() + 44);
        file[40] = 1;                                                          // one track
        const uint8_t track[12] = {0, 0, 0, 0, 0x22, 0x56, 0, 0, 0, 0, 0, 0}; // 22050 Hz, mono, RDFT
        file.insert(file.end(), track, track + 12);
        const uint8_t index[4] = {60, 0, 0, 0}, size[4] = {0xF0, 0xFF, 0xFF, 0xFF};
        file.insert(file.end(), index, index + 4);
        file.insert(file.end(), size, size + 4);
        file.insert(file.end(), plain.begin() + 48, plain.end());
        const uint32_t total = uint32_t(file.size() - 8);
        for (int i = 0; i < 4; ++i)
            file[4 + i] = uint8_t(total >> (8 * i));
        CHECK(movie.open(write("audio.bik", file)).ok && movie.hasAudio());
        audio::Sound sound;
        CHECK(!movie.decodeAudio(sound).ok);
    }

    // Implausible sizes are refused, not allocated.
    std::vector<uint8_t> huge = binkMovie(false);
    huge[21] = 0x40; // 16392 wide
    CHECK(!movie.open(write("huge.bik", huge)).ok);
}

int main() {
    auto tmp = std::filesystem::temp_directory_path() / "reluva_tests";
    testMd5();
    testUtf16();
    testFont();
    testWav();
    testMp3();
    testMixer();
    testBink(tmp / "bink");
    testSavestate(tmp / "save");
    testFsAndDetection(tmp / "fs");
    testConfig(tmp / "cfg");
    if (failures)
        std::fprintf(stderr, "%d check(s) failed\n", failures);
    else
        std::printf("All tests passed\n");
    return failures ? 1 : 0;
}
