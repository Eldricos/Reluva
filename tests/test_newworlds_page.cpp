// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// The New Worlds missions page over the real game, as a regression test:
// the page driven headless with a fixed clock and seeded random input (its
// keys, briefings, play and runs, panels, saves and loads), every frame's
// screen, window title and sound folded into a hash, checked at named
// points against tests/data/newworlds_page.golden.
//
//   reluva_newworlds_page_tests [game folder] [--update] [--log <file>]
//
// The game folder (the .cat files and level.dat) is the argument, else
// $RELUVA_NEWWORLDS_DATA; without one the test is skipped (it passes).
// --log writes every frame's line (hash, title, sound) to a file, to diff
// two builds' runs; --shots <folder> writes the screen at each point.
//
// A change meant to alter what the page shows or plays makes the test
// fail at the first point it reaches: look (--shots), then write the new
// hashes with --update and commit the golden file with the change. The
// hashes hold for one compiler and build type (floating point); the file
// was made with GCC, Release.

#include "common/audio/mixer.h"
#include "common/events.h"
#include "common/fs.h"
#include "common/savestate.h"
#include "common/system.h"
#include "engines/newworlds/cat_archive.h"
#include "engines/newworlds/levels.h"
#include "engines/newworlds/viewers.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#ifndef RELUVA_PAGE_GOLDEN
#define RELUVA_PAGE_GOLDEN "tests/data/newworlds_page.golden"
#endif

using namespace reluva;
using namespace reluva::newworlds;

namespace {

constexpr int kRate = 22050;
// Random actions in each mission played.
constexpr int kActions = 100;

uint64_t gNow = 1000;

struct FakeSystem : System {
    std::string title;
    Error initGraphics(int, int, PixelFormat) override { return Error::success(); }
    void setPalette(const Color*, int, int) override {}
    void updateScreen(const Surface&) override {}
    void setWindowTitle(const std::string& t) override { title = t; }
    bool pollEvent(Event&) override { return false; }
    uint64_t getMillis() const override { return gNow; }
    void delayMillis(uint32_t) override {}
    Error startAudio(AudioCallback) override { return Error::fail("none"); }
    void stopAudio() override {}
    int audioRate() const override { return kRate; }
};

uint64_t fnv(const void* data, size_t n, uint64_t h = 1469598103934665603ull) {
    const auto* p = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < n; ++i)
        h = (h ^ p[i]) * 1099511628211ull;
    return h;
}

std::string hex(uint64_t v) {
    char buf[20];
    std::snprintf(buf, sizeof buf, "%016llx", (unsigned long long)v);
    return buf;
}

struct Options {
    std::filesystem::path data, log, shots;
    bool update = false;
};

// The log file, by its path as it is (path::string() is the ANSI code page
// on Windows).
FILE* openLog(const std::filesystem::path& file) {
#ifdef _WIN32
    return _wfopen(file.c_str(), L"w");
#else
    return std::fopen(file.c_str(), "w");
#endif
}

// The run: the checked points ("<name> <frame> <hash>", the hash of every
// frame so far), in order.
std::vector<std::string> run(const Options& o) {
    Archives archives;
    std::vector<std::filesystem::path> paths;
    for (const auto& e : std::filesystem::directory_iterator(o.data))
        if (e.path().extension() == ".cat")
            paths.push_back(e.path());
    std::sort(paths.begin(), paths.end());
    for (const auto& p : paths) {
        auto cat = std::make_unique<CatArchive>();
        if (!cat->open(p))
            archives.push_back(std::move(cat));
    }
    std::vector<Level> levels;
    {
        std::ifstream in(o.data / "level.dat", std::ios::binary);
        const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        std::vector<std::string> skipped;
        parseLevelList(text, levels, &skipped);
    }
    FakeSystem sys;
    Surface screen;
    screen.create(640, 480, PixelFormat::RGBA32);
    audio::Mixer mixer(kRate);
    MissionViewer mv(sys, screen, archives, levels, &mixer, o.data);
    mv.seed(7);

    std::vector<std::string> points;
    std::unique_ptr<FILE, int (*)(FILE*)> log(o.log.empty() ? nullptr : openLog(o.log),
                                              [](FILE* f) { return f ? std::fclose(f) : 0; });
    uint64_t hash = fnv("page", 4);
    int frame = 0;
    std::vector<int16_t> sound;
    // A note in the run's hash (and log): what a save or load said.
    auto note = [&](const std::string& s) {
        hash = fnv(s.data(), s.size(), hash);
        if (log)
            std::fprintf(log.get(), "%s\n", s.c_str());
    };
    auto frameStep = [&](int ms) {
        gNow += uint64_t(ms);
        const int n = kRate * ms / 1000;
        sound.assign(size_t(n) * 2, 0);
        mixer.mix(sound.data(), n);
        mv.update(gNow);
        const uint64_t screenHash = fnv(screen.pixels.data(), screen.pixels.size());
        const uint64_t soundHash = fnv(sound.data(), sound.size() * sizeof(int16_t));
        char line[96];
        std::snprintf(line, sizeof line, "%d %s %s p%d r%d s%c sp%d mu%d fx%d ", frame++, hex(screenHash).c_str(),
                      hex(soundHash).c_str(), mv.playing(), mv.inRun(), mv.side() ? mv.side() : '-',
                      mixer.isTypePlaying(audio::SoundType::Speech), mixer.isTypePlaying(audio::SoundType::Music),
                      mixer.isTypePlaying(audio::SoundType::Effects));
        hash = fnv(line, std::strlen(line), hash);
        hash = fnv(sys.title.data(), sys.title.size(), hash);
        if (log)
            std::fprintf(log.get(), "%s| %s\n", line, sys.title.c_str());
    };
    auto point = [&](const std::string& name) {
        points.push_back(name + " " + std::to_string(frame) + " " + hex(hash));
        if (log)
            std::fprintf(log.get(), "point %s\n", points.back().c_str());
        if (o.shots.empty())
            return;
        std::ofstream out(o.shots / (name + ".ppm"), std::ios::binary);
        out << "P6\n640 480\n255\n";
        for (size_t i = 0; i < screen.pixels.size(); i += 4)
            out.write(reinterpret_cast<const char*>(&screen.pixels[i]), 3);
    };
    auto send = [&](Event ev) {
        // As the engine: Escape and Tab off the page aren't the page's.
        const bool game = mv.playing() || mv.inRun();
        if (ev.type == EventType::KeyDown && (ev.key == key::Escape || ev.key == key::Tab) && !game)
            return;
        mv.handleEvent(ev);
    };
    auto keyEv = [&](uint32_t k, uint16_t mods = 0) {
        Event e;
        e.type = EventType::KeyDown;
        e.key = k;
        e.mods = mods;
        send(e);
        e.type = EventType::KeyUp;
        send(e);
    };
    auto mouse = [&](EventType t, int x, int y, MouseButton b = MouseButton::None, int clicks = 0, uint16_t mods = 0) {
        Event e;
        e.type = t;
        e.mouseX = std::clamp(x, 0, 639);
        e.mouseY = std::clamp(y, 0, 479);
        e.button = b;
        e.clicks = clicks;
        e.mods = mods;
        send(e);
    };
    auto click = [&](int x, int y, MouseButton b = MouseButton::Left, int clicks = 1, uint16_t mods = 0) {
        mouse(EventType::MouseMove, x, y);
        mouse(EventType::MouseDown, x, y, b, clicks, mods);
        mouse(EventType::MouseUp, x, y, b, clicks, mods);
    };
    auto frames = [&](int n, int ms = 33) {
        for (int i = 0; i < n; ++i)
            frameStep(ms);
    };

    mv.activate();
    frames(3);
    point("page");
    // The page's keys.
    for (uint32_t k : {key::Down, key::Down, key::Up, uint32_t('b'), uint32_t('b'), uint32_t('i')}) {
        keyEv(k);
        frames(2);
    }
    point("page-interface");
    for (uint32_t k : {uint32_t('i'), uint32_t('c'), uint32_t('c'), uint32_t('v'), uint32_t('n'), uint32_t('p'),
                       uint32_t(key::Space), uint32_t('m'), uint32_t('m'), uint32_t('k'), uint32_t('k')}) {
        keyEv(k);
        frames(2);
    }
    // Every mission's page, and its sides.
    for (size_t i = 0; i < mv.count(); ++i) {
        keyEv(key::Right);
        frames(1);
        keyEv('j');
        frames(1);
    }
    point("pages");

    std::mt19937 rng(12345);
    auto rnd = [&](int n) { return int(rng() % uint32_t(n)); };
    // The missions played (F3) or run (G), turn about: a few of each kind.
    for (const char* tag : {"F1", "K2", "R8", "J1", "C4"}) {
        int pick = -1;
        for (size_t i = 0; i < mv.count(); ++i)
            if (mv.mission(i).shortName == tag)
                pick = int(i);
        if (pick < 0) {
            point(std::string(tag) + "-missing");
            continue;
        }
        for (int tries = 0; mv.shown() != pick && tries < 100; ++tries) {
            keyEv(key::Right);
            frames(1, 16);
        }
        if (std::strcmp(tag, "J1") == 0) {
            keyEv('j');
            frames(1);
        }
        // The briefing: played a while, steps skipped, stopped.
        keyEv(key::Return);
        frames(3);
        for (int i = 0; i < 40; ++i)
            frameStep(100);
        point(std::string(tag) + "-briefing");
        for (int i = 0; i < 3; ++i) {
            keyEv(key::Return);
            frames(10, 50);
        }
        keyEv(key::Backspace);
        frames(2);
        // The script walked through on the page.
        for (int i = 0; i < 6; ++i) {
            keyEv('x');
            frames(2);
        }
        keyEv('z');
        frames(2);
        point(std::string(tag) + "-script");
        if (std::strchr("KJ", tag[0])) {
            keyEv('g');
            frames(5);
            for (int i = 0; i < 6; ++i) {
                keyEv(key::Return);
                frames(5, 50);
            }
            frames(30, 50);
        } else {
            keyEv(key::F1 + 2);
            frames(3);
        }
        point(std::string(tag) + "-play");
        // Directed: the colony's details, its panels, the Tricorder.
        keyEv('c');
        frames(2);
        for (int i = 0; i < 30; ++i) {
            // Drawn into locals, last argument first: the order GCC
            // evaluated the arguments in when the golden was made.
            const int py = rnd(300), px = 470 + rnd(170);
            click(px, py);
            frames(1, 50);
        }
        point(std::string(tag) + "-panels");
        keyEv(key::Tab);
        frames(2);
        {
            const int ty = 40 + rnd(400), tx = 100 + rnd(440);
            click(tx, ty);
        }
        frames(2);
        point(std::string(tag) + "-tricorder");
        keyEv(key::Tab);
        frames(2);
        // The random part.
        bool saved = false;
        std::vector<uint8_t> state;
        for (int a = 0; a < kActions; ++a) {
            if (!mv.playing()) {
                if (mv.inRun()) {
                    keyEv(key::Escape);
                    frames(1);
                }
                keyEv(key::F1 + 2);
                frames(2);
            }
            const int r = rnd(100);
            int x = 0, y = 0;
            const int z = rnd(10);
            if (z < 4)
                x = 460 + rnd(180), y = rnd(480); // the panels
            else if (z < 6)
                x = 150 + rnd(300), y = 360 + rnd(120); // the bottom bar
            else if (z < 7)
                x = rnd(270), y = rnd(120); // the minimap
            else
                x = rnd(640), y = rnd(480);
            if (r < 25) {
                mouse(EventType::MouseMove, x, y);
            } else if (r < 45) {
                const int ctrl = rnd(12), clicks = rnd(5), button = rnd(4);
                click(x, y, button ? MouseButton::Left : MouseButton::Right, clicks ? 1 : 2,
                      ctrl ? 0 : keymod::Ctrl);
            } else if (r < 50) {
                // A box.
                const int x2 = x + rnd(200) - 100, y2 = y + rnd(200) - 100;
                mouse(EventType::MouseMove, x, y);
                mouse(EventType::MouseDown, x, y, MouseButton::Left, 1);
                frameStep(30);
                mouse(EventType::MouseMove, x2, y2);
                frameStep(30);
                mouse(EventType::MouseUp, x2, y2, MouseButton::Left, 1);
            } else if (r < 55) {
                // The playbook: the scouts, a double click, its column.
                keyEv(rnd(2) ? 'g' : 'a');
                const int gx = 100 + rnd(300), gy = 100 + rnd(250);
                click(gx, gy, MouseButton::Left, 1);
                mouse(EventType::MouseDown, gx, gy, MouseButton::Left, 2);
                mouse(EventType::MouseUp, gx, gy, MouseButton::Left, 2);
                frameStep(30);
                const int my = 300 + rnd(180), mx = 400 + rnd(140);
                mouse(EventType::MouseMove, mx, my);
                frameStep(30);
                const int cy = 320 + rnd(160), cx = 420 + rnd(110);
                click(cx, cy);
            } else if (r < 95) {
                static const uint32_t kKeys[] = {
                    'b', 'v', 'r', 'c', 'a', 'g', 's', 't', 'p', '1', '2', '3', '4', '5', '0', '6', '7', '8', '9',
                    key::Tab, key::Space, key::Home, 'i', 'y', 'o', key::Escape, 'n', key::F1, key::F1 + 1,
                    key::F1 + 2, key::Kp0 + 4, key::Kp0 + 6, key::Kp0 + 8, key::Kp0 + 2, key::Up, key::Down,
                    key::Left, key::Right, 'x', 'q', 'w', 'e', 'z', '+', '-', key::PageUp, key::PageDown};
                const uint32_t k = kKeys[rnd(int(std::size(kKeys)))];
                const int m = rnd(20);
                const uint16_t mods = m == 0 ? keymod::Shift : m == 1 ? keymod::Ctrl : m == 2 ? keymod::Alt : 0;
                if (k == key::Escape && rnd(3) == 0) {
                    // The quit box, answered.
                    keyEv(k);
                    frameStep(30);
                    keyEv(rnd(3) ? 'n' : 'y');
                } else {
                    keyEv(k, mods);
                }
            } else if (r < 97) {
                Event e;
                e.type = EventType::MouseWheel;
                e.mouseX = x, e.mouseY = y;
                e.wheelY = rnd(2) ? 1 : -1;
                send(e);
            } else if (r < 98) {
                keyEv(key::Pause);
                frames(2);
                keyEv(key::Pause);
            } else {
                // A save; now and then one loaded back.
                std::string why;
                const bool can = mv.canSave(why);
                note("save " + std::to_string(can) + " " + why + " " + mv.stateDescription());
                if (can) {
                    save::Serializer s = save::Serializer::saver();
                    const Error e = mv.syncState(s);
                    note("saved " + std::to_string(e.ok) + " " + std::to_string(s.bytes().size()) + " " +
                         hex(fnv(s.bytes().data(), s.bytes().size())));
                    if (!saved || rnd(2)) {
                        state = s.bytes();
                        saved = true;
                    } else {
                        save::Serializer l = save::Serializer::loader(state);
                        const Error e2 = mv.syncState(l);
                        note("loaded " + std::to_string(e2.ok) + " " + e2.message);
                    }
                }
            }
            const int stepMs = 20 + rnd(80), count = 1 + rnd(3);
            frames(count, stepMs);
            if (a == kActions / 2)
                point(std::string(tag) + "-mid");
        }
        // Leave.
        for (int tries = 0; (mv.playing() || mv.inRun()) && tries < 20; ++tries) {
            if (mv.paused())
                keyEv(key::Pause);
            if (mv.quitAsked())
                keyEv('n');
            keyEv(key::Escape);
            frames(1);
            if (mv.quitAsked())
                keyEv('y');
            frames(2);
        }
        point(std::string(tag) + "-end " + std::to_string(mv.playing()) + std::to_string(mv.inRun()));
    }
    mv.deactivate();
    frames(1);
    point("done");
    return points;
}

} // namespace

int main(int argc, char** argv) {
    fs::Utf8Args utf8(argc, argv);
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--update")
            o.update = true;
        else if ((a == "--log" || a == "--shots") && i + 1 < argc)
            (a == "--log" ? o.log : o.shots) = fs::fromUtf8(argv[++i]);
        else if (!a.empty() && a[0] != '-')
            o.data = fs::fromUtf8(a);
        else {
            std::fprintf(stderr, "usage: %s [game folder] [--update] [--log <file>] [--shots <folder>]\n", argv[0]);
            return 2;
        }
    }
    if (o.data.empty())
        if (const char* env = std::getenv("RELUVA_NEWWORLDS_DATA"))
            o.data = env;
    std::error_code ec;
    if (o.data.empty() || !std::filesystem::is_regular_file(o.data / "level.dat", ec)) {
        std::printf("skipped: no New Worlds game folder (give one, or set RELUVA_NEWWORLDS_DATA)\n");
        return 0;
    }
    if (!o.shots.empty())
        std::filesystem::create_directories(o.shots, ec);

    const std::vector<std::string> points = run(o);
    if (o.update) {
        std::ofstream out(fs::fromUtf8(RELUVA_PAGE_GOLDEN), std::ios::binary);
        out << "# The New Worlds missions page's run (tests/test_newworlds_page.cpp): each point's name,\n"
               "# frame and hash of every frame so far. Rewrite with reluva_newworlds_page_tests --update.\n";
        for (const std::string& p : points)
            out << p << "\n";
        if (!out) {
            std::fprintf(stderr, "couldn't write %s\n", RELUVA_PAGE_GOLDEN);
            return 1;
        }
        std::printf("wrote %zu points to %s\n", points.size(), RELUVA_PAGE_GOLDEN);
        return 0;
    }

    std::vector<std::string> golden;
    std::ifstream in(fs::fromUtf8(RELUVA_PAGE_GOLDEN));
    for (std::string line; std::getline(in, line);)
        if (!line.empty() && line[0] != '#')
            golden.push_back(line);
    if (golden.empty()) {
        std::fprintf(stderr, "no golden hashes in %s (make them with --update)\n", RELUVA_PAGE_GOLDEN);
        return 1;
    }
    for (size_t i = 0; i < std::max(points.size(), golden.size()); ++i) {
        const std::string got = i < points.size() ? points[i] : "(nothing)";
        const std::string want = i < golden.size() ? golden[i] : "(nothing)";
        if (got != want) {
            std::fprintf(stderr,
                         "the page differs from %s first at point %zu:\n  got  %s\n  want %s\n"
                         "(--shots <folder> to look; --update if the change is meant)\n",
                         RELUVA_PAGE_GOLDEN, i + 1, got.c_str(), want.c_str());
            return 1;
        }
    }
    std::printf("%zu points as golden\n", points.size());
    return 0;
}
