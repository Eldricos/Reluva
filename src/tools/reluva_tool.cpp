// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// reluva-tool: developer utilities for examining game data files.
//
//   reluva-tool hexdump <file> [offset] [length]   offset may be hex (0x..) or
//                                                  negative (from the end)
//   reluva-tool cat-probe <file> [-q]              check a .cat file against the documented
//                                                  layout and work out unknown entry kinds
//   reluva-tool cat-list <file>                    list a .cat file's contents
//   reluva-tool cat-extract <file> <dir> [filter]  extract files whose name contains filter
//   reluva-tool stb-dump <file.stb>                print a string table
//   reluva-tool lbp-info <file> [out]              show a texture's header; if it decodes,
//                                                  write <out>_rgb.bmp and <out>_bgr.bmp

#include "engines/newworlds/cat_archive.h"
#include "engines/newworlds/string_table.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

using Bytes = std::vector<uint8_t>;

bool readFile(const char* path, Bytes& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "cannot open %s\n", path);
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(in), {});
    return true;
}

uint16_t rd16(const Bytes& d, size_t o) { return o + 2 <= d.size() ? uint16_t(d[o] | d[o + 1] << 8) : 0; }
uint32_t rd32(const Bytes& d, size_t o) {
    return o + 4 <= d.size() ? uint32_t(d[o]) | uint32_t(d[o + 1]) << 8 | uint32_t(d[o + 2]) << 16 |
                                   uint32_t(d[o + 3]) << 24
                             : 0;
}

void hexdump(const Bytes& d, size_t off, size_t len) {
    size_t end = std::min(d.size(), off + len);
    for (size_t row = off; row < end; row += 16) {
        std::printf("%08zx: ", row);
        for (size_t i = 0; i < 16; ++i) {
            if (row + i < end)
                std::printf("%02x", d[row + i]);
            else
                std::printf("  ");
            if (i % 2)
                std::printf(" ");
        }
        std::printf(" ");
        for (size_t i = 0; i < 16 && row + i < end; ++i) {
            uint8_t c = d[row + i];
            std::putchar(c >= 32 && c < 127 ? c : '.');
        }
        std::printf("\n");
    }
}

// Unix time -> "YYYY-MM-DD HH:MM" (UTC), without platform time functions.
std::string formatTime(uint32_t t) {
    int64_t days = t / 86400, secs = t % 86400;
    int64_t z = days + 719468, era = z / 146097, doe = z - era * 146097;
    int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100), mp = (5 * doy + 2) / 153;
    int64_t day = doy - (153 * mp + 2) / 5 + 1, month = mp < 10 ? mp + 3 : mp - 9;
    int64_t year = yoe + era * 400 + (month <= 2);
    char buf[32];
    std::snprintf(buf, sizeof buf, "%04d-%02d-%02d %02d:%02d", int(year), int(month), int(day),
                  int(secs / 3600), int(secs / 60 % 60));
    return buf;
}

long long parseNumber(const char* s) { return std::strtoll(s, nullptr, 0); }

int cmdHexdump(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: reluva-tool hexdump <file> [offset] [length]\n");
        return 2;
    }
    Bytes d;
    if (!readFile(argv[2], d))
        return 1;
    long long off = argc > 3 ? parseNumber(argv[3]) : 0;
    if (off < 0)
        off += static_cast<long long>(d.size());
    size_t len = argc > 4 ? size_t(parseNumber(argv[4])) : 256;
    hexdump(d, size_t(std::max(0LL, off)), len);
    return 0;
}

// Current understanding (see docs/formats/newworlds/cat.md): the file is a
// tree of chunks, each a u16 tag + u32 size + payload.
//   0x00 tag 1, size = rest of file
//   0x06 tag 5, size = table of contents: u32 count, then one entry per item
//        file entries are 334 bytes, folder entries 264
//   then the items themselves, each a chunk; folders (tag 0x29) contain the
//   chunks of the entries that follow them
// An entry's offset + the table size gives the absolute position of its chunk.
constexpr size_t kPathLen = 260;
constexpr size_t kFileEntrySize = 334;
constexpr size_t kFolderEntrySize = 264;
constexpr uint16_t kFolderTag = 0x29;

std::string readName(const Bytes& d, size_t p) {
    std::string n;
    for (size_t k = p; k < p + kPathLen && k < d.size() && d[k]; ++k) {
        if (d[k] < 32 || d[k] >= 127)
            return "<binary>";
        n += char(d[k]);
    }
    return n;
}

std::string extensionOf(const std::string& name) {
    size_t dot = name.rfind('.');
    return dot == std::string::npos || name.find('\\', dot) != std::string::npos ? "" : name.substr(dot);
}

// Rules for walking the table. Start with what is known; learn the rest.
struct TagRule {
    uint16_t tag;
    size_t entrySize;
    bool container;
    bool learned;
};

struct Walker {
    const Bytes& d;
    uint32_t tocSize;
    uint8_t archiveByte;
    std::vector<TagRule> rules;

    TagRule rule(uint16_t tag) const {
        for (const TagRule& r : rules)
            if (r.tag == tag)
                return r;
        return tag == kFolderTag ? TagRule{tag, kFolderEntrySize, true, false} : TagRule{tag, kFileEntrySize, false, false};
    }
    void learn(const TagRule& r) {
        for (TagRule& x : rules)
            if (x.tag == r.tag) {
                x = r;
                return;
            }
        rules.push_back(r);
    }
    size_t chunkOf(size_t entryPos) const { return size_t(rd32(d, entryPos)) + tocSize; }
    bool validEntry(size_t q, int prevIndex, size_t expectedChunk) const {
        return q + 8 <= d.size() && int(rd16(d, q + 4)) > prevIndex && d[q + 6] == 0 && d[q + 7] == archiveByte &&
               chunkOf(q) == expectedChunk;
    }
};

int cmdCatProbe(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: reluva-tool cat-probe <file.cat> [-q]\n");
        return 2;
    }
    Bytes d;
    if (!readFile(argv[2], d))
        return 1;
    const bool quiet = argc > 3 && std::strcmp(argv[3], "-q") == 0;

    const uint32_t tocSize = rd32(d, 8);
    const uint32_t count = rd32(d, 0x0c);
    const size_t tocEnd = 0x0c + size_t(tocSize);
    if (!quiet) {
        std::printf("file size %zu; root chunk tag %u size %u (%s)\n", d.size(), rd16(d, 0), rd32(d, 2),
                    rd32(d, 2) + 6 == d.size() ? "matches" : "MISMATCH");
        std::printf("table chunk tag %u size %u, ends at 0x%zx; %u entries\n\n", rd16(d, 6), tocSize, tocEnd, count);
        std::printf("%4s %6s %9s %4s %8s  %-16s %s\n", "idx", "at", "chunk at", "tag", "size", "modified (UTC)",
                    "name (indented by folder)");
    }

    Walker w{d, tocSize, d.size() > 0x17 ? d[0x17] : uint8_t(0), {}};
    std::vector<size_t> folderEnds;
    struct TagStat {
        uint16_t tag;
        unsigned count = 0, sizeMismatches = 0;
        std::vector<std::string> extensions;
    };
    std::vector<TagStat> stats;
    auto statFor = [&](uint16_t tag) -> TagStat& {
        for (TagStat& s : stats)
            if (s.tag == tag)
                return s;
        stats.push_back(TagStat{tag, 0, 0, {}});
        return stats.back();
    };

    size_t pos = 0x10, expectedChunk = tocEnd;
    size_t prevPos = 0, prevChunk = 0;
    uint32_t prevSize = 0;
    uint16_t prevTag = 0;
    int prevIndex = -1;
    unsigned entries = 0, indexGaps = 0;
    for (unsigned i = 0; i < count; ++i) {
        if (!w.validEntry(pos, prevIndex, expectedChunk)) {
            // The previous entry's rule was wrong. Try the other kind (file /
            // container) and any entry size, and keep the one that lets the
            // chain continue.
            // Take the nearest entry that fits either explanation: a farther
            // match can be a later entry that skips a container's children.
            bool recovered = false;
            const TagRule old = w.rule(prevTag);
            size_t bestQ = 0, bestWant = 0;
            bool bestContainer = false;
            for (int attempt = 0; attempt < 2 && i > 0; ++attempt) {
                const bool container = attempt == 0 ? old.container : !old.container;
                const size_t want = container ? prevChunk + 6 : prevChunk + 6 + prevSize;
                for (size_t q = prevPos + 8; q < prevPos + 4096 && q + 8 <= d.size() && (!bestQ || q < bestQ); ++q) {
                    if (w.validEntry(q, prevIndex, want)) {
                        bestQ = q;
                        bestWant = want;
                        bestContainer = container;
                        break;
                    }
                }
            }
            if (bestQ) {
                TagRule r{prevTag, bestQ - prevPos, bestContainer, true};
                w.learn(r);
                if (!quiet)
                    std::printf("  -> learned: tag 0x%x entries are %zu bytes, %s\n", prevTag, r.entrySize,
                                bestContainer ? "container" : "file");
                if (bestContainer && !old.container) // re-open what we treated as a file
                    folderEnds.push_back(prevChunk + 6 + prevSize);
                if (!bestContainer && old.container && !folderEnds.empty())
                    folderEnds.pop_back();
                pos = bestQ;
                expectedChunk = bestWant;
                recovered = true;
            }
            if (!recovered) {
                std::printf("entry #%u: no valid entry follows the one at 0x%zx (tag 0x%x)\n", i, prevPos, prevTag);
                hexdump(d, pos > 0x20 ? pos - 0x20 : 0, 0x80);
                std::printf("RESULT: FAIL at entry #%u after a tag 0x%x entry\n", i, prevTag);
                return 1;
            }
        }

        const int index = rd16(d, pos + 4);
        if (prevIndex >= 0 && index != prevIndex + 1)
            indexGaps += unsigned(index - prevIndex - 1);
        const size_t chunk = w.chunkOf(pos);
        const uint16_t tag = rd16(d, chunk);
        const uint32_t size = rd32(d, chunk + 2);
        const TagRule r = w.rule(tag);
        while (!folderEnds.empty() && chunk >= folderEnds.back())
            folderEnds.pop_back();
        const std::string indent(folderEnds.size() * 2, ' ');
        TagStat& st = statFor(tag);
        ++st.count;

        if (r.container) {
            if (!quiet)
                std::printf("%4d %6zx %9zx %4x %8u  %-16s %s<container>\n", index, pos, chunk, tag, size, "",
                            indent.c_str());
            folderEnds.push_back(chunk + 6 + size);
            expectedChunk = chunk + 6;
        } else {
            const size_t p = pos + 8;
            const std::string name = readName(d, p);
            const uint32_t metaSize = rd32(d, p + 0x116);
            const bool sizeOk = metaSize == size;
            st.sizeMismatches += !sizeOk;
            std::string ext = extensionOf(name);
            bool seen = false;
            for (const std::string& e : st.extensions)
                seen |= e == ext;
            if (!seen && !ext.empty())
                st.extensions.push_back(ext);
            if (!quiet)
                std::printf("%4d %6zx %9zx %4x %8u%s %-16s %s%s\n", index, pos, chunk, tag, size, sizeOk ? " " : "*",
                            formatTime(rd32(d, p + 0x11e)).c_str(), indent.c_str(), name.c_str());
            expectedChunk = chunk + 6 + size;
        }
        prevPos = pos;
        prevChunk = chunk;
        prevSize = size;
        prevTag = tag;
        prevIndex = index;
        pos += r.entrySize;
        ++entries;
    }

    const bool tableOk = pos == tocEnd;
    const long long trailing = (long long)d.size() - (long long)expectedChunk;
    if (!quiet) {
        std::printf("\n%u entries, %u index gaps; table walk ended at 0x%zx, table chunk ends at 0x%zx\n", entries,
                    indexGaps, pos, tocEnd);
        std::printf("last chunk ends at %zu, file size %zu\n", expectedChunk, d.size());
        std::printf("* = stored size differs from the size in the entry's copied metadata\n\n");
    }
    std::printf("tags:");
    for (const TagStat& s : stats) {
        const TagRule r = w.rule(s.tag);
        std::printf(" 0x%x x%u", s.tag, s.count);
        if (r.container)
            std::printf("(container");
        else
            std::printf("(");
        for (size_t k = 0; k < s.extensions.size(); ++k)
            std::printf("%s%s", k || r.container ? " " : "", s.extensions[k].c_str());
        if (s.sizeMismatches)
            std::printf(" *%u", s.sizeMismatches);
        std::printf(")");
    }
    std::printf("\n");
    for (const TagRule& r : w.rules)
        if (r.learned)
            std::printf("learned: tag 0x%x entry %zu bytes, %s\n", r.tag, r.entrySize, r.container ? "container" : "file");
    const bool ok = tableOk && trailing >= 0;
    std::printf("RESULT: %s (%u entries, table %s, %lld trailing bytes)\n", ok ? "OK" : "FAIL", entries,
                tableOk ? "exact" : "MISMATCH", trailing);
    return ok ? 0 : 1;
}


int cmdCatList(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: reluva-tool cat-list <file.cat>\n");
        return 2;
    }
    reluva::newworlds::CatArchive cat;
    if (reluva::Error err = cat.open(argv[2])) {
        std::fprintf(stderr, "%s\n", err.message.c_str());
        return 1;
    }
    const auto& entries = cat.entries();
    for (const auto& e : entries) {
        int depth = 0;
        for (int p = e.parent; p >= 0; p = entries[size_t(p)].parent)
            ++depth;
        std::printf("%4x %9u  %-16s %*s%s\n", e.tag, e.size, e.container ? "" : formatTime(e.modified).c_str(),
                    depth * 2, "", e.container ? "<container>" : e.name.c_str());
    }
    return 0;
}

int cmdCatExtract(int argc, char** argv) {
    if (argc < 4) {
        std::fprintf(stderr, "usage: reluva-tool cat-extract <file.cat> <output dir> [name filter]\n");
        return 2;
    }
    reluva::newworlds::CatArchive cat;
    if (reluva::Error err = cat.open(argv[2])) {
        std::fprintf(stderr, "%s\n", err.message.c_str());
        return 1;
    }
    const std::filesystem::path outDir = argv[3];
    const std::string filter = argc > 4 ? argv[4] : "";
    std::vector<uint8_t> data;
    unsigned written = 0;
    for (const auto& e : cat.entries()) {
        if (e.container || e.name.empty() || e.name.find(filter) == std::string::npos)
            continue;
        // Names come from the archive: refuse anything that could escape outDir.
        const std::filesystem::path rel(e.name);
        bool safe = !rel.is_absolute() && !rel.has_root_name();
        for (const auto& part : rel)
            safe &= part != "..";
        if (!safe) {
            std::fprintf(stderr, "skipping unsafe name %s\n", e.name.c_str());
            continue;
        }
        if (reluva::Error err = cat.read(e, data)) {
            std::fprintf(stderr, "%s\n", err.message.c_str());
            return 1;
        }
        const std::filesystem::path out = outDir / rel;
        std::error_code ec;
        std::filesystem::create_directories(out.parent_path(), ec);
        std::ofstream f(out, std::ios::binary);
        f.write(reinterpret_cast<const char*>(data.data()), std::streamsize(data.size()));
        if (!f) {
            std::fprintf(stderr, "cannot write %s\n", out.string().c_str());
            return 1;
        }
        ++written;
    }
    std::printf("extracted %u files to %s\n", written, outDir.string().c_str());
    return 0;
}


int cmdStbDump(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: reluva-tool stb-dump <file.stb>\n");
        return 2;
    }
    Bytes d;
    if (!readFile(argv[2], d))
        return 1;
    reluva::newworlds::StringTable table;
    reluva::Error err = table.parse(d);
    for (const auto& s : table.strings())
        std::printf("%6u  %s\n", s.id, s.text.c_str());
    if (err) {
        std::fprintf(stderr, "%s\n", err.message.c_str());
        return 1;
    }
    std::printf("-- %zu strings, layout matches exactly\n", table.strings().size());
    return 0;
}

// 8-bit paletted BMP, rows top-down. swapRB picks which palette byte is red.
bool writeBmp(const std::string& path, int w, int h, const uint8_t* palette, int paletteCount, const uint8_t* pixels,
              bool swapRB) {
    const int stride = (w + 3) & ~3;
    const uint32_t paletteBytes = 256 * 4, dataOffset = 14 + 40 + paletteBytes;
    const uint32_t fileSize = dataOffset + uint32_t(stride * h);
    Bytes out;
    auto p16 = [&](uint32_t v) { out.push_back(uint8_t(v)); out.push_back(uint8_t(v >> 8)); };
    auto p32 = [&](uint32_t v) { for (int i = 0; i < 4; ++i) out.push_back(uint8_t(v >> (8 * i))); };
    out.push_back('B');
    out.push_back('M');
    p32(fileSize); p32(0); p32(dataOffset);
    p32(40); p32(uint32_t(w)); p32(uint32_t(-h)); p16(1); p16(8); p32(0); p32(uint32_t(stride * h));
    p32(2835); p32(2835); p32(256); p32(0);
    for (int i = 0; i < 256; ++i) {
        uint8_t c0 = 0, c1 = 0, c2 = 0;
        if (i < paletteCount) {
            c0 = palette[i * 4];
            c1 = palette[i * 4 + 1];
            c2 = palette[i * 4 + 2];
        }
        // BMP palettes are stored blue, green, red, 0.
        uint8_t r = swapRB ? c2 : c0, b = swapRB ? c0 : c2;
        out.push_back(b);
        out.push_back(c1);
        out.push_back(r);
        out.push_back(0);
    }
    for (int y = 0; y < h; ++y) {
        out.insert(out.end(), pixels + size_t(y) * size_t(w), pixels + size_t(y + 1) * size_t(w));
        out.resize(out.size() + size_t(stride - w), 0);
    }
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(out.data()), std::streamsize(out.size()));
    return bool(f);
}

int cmdLbpInfo(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: reluva-tool lbp-info <file> [output prefix]\n");
        return 2;
    }
    Bytes d;
    if (!readFile(argv[2], d))
        return 1;
    // Hypothesis (docs/formats/newworlds/lbp.md): u32 format, u32 palette
    // count, palette (4 bytes each), 12-byte image header, 8-bit pixels.
    const uint32_t format = rd32(d, 0), paletteCount = rd32(d, 4);
    std::printf("size %zu, u32@0 (format?) %u, u32@4 (palette count?) %u\n", d.size(), format, paletteCount);
    const size_t paletteEnd = 8 + size_t(paletteCount) * 4;
    if (paletteCount > 256 || paletteEnd + 12 > d.size()) {
        std::printf("palette hypothesis doesn't fit; first bytes:\n");
        hexdump(d, 0, 64);
        return 1;
    }
    const uint32_t a = rd32(d, paletteEnd), b = rd32(d, paletteEnd + 4), c = rd32(d, paletteEnd + 8);
    const size_t pixelBytes = d.size() - paletteEnd - 12;
    std::printf("after palette at 0x%zx: u32 %u, u32 %u, u32 %u; %zu bytes follow\n", paletteEnd, a, b, c, pixelBytes);
    hexdump(d, paletteEnd, 32);
    std::printf("first palette entries:\n");
    hexdump(d, 8, 32);

    int w = 0, h = 0;
    if (size_t(a) * b == pixelBytes) {
        w = int(a);
        h = int(b);
        std::printf("=> %d x %d at 8 bits per pixel fits exactly\n", w, h);
    } else {
        std::printf("=> width x height doesn't match the remaining bytes; not decoding\n");
        return 1;
    }
    if (argc > 3) {
        const std::string prefix = argv[3];
        const uint8_t* pixels = d.data() + paletteEnd + 12;
        bool ok = writeBmp(prefix + "_rgb.bmp", w, h, d.data() + 8, int(paletteCount), pixels, false) &&
                  writeBmp(prefix + "_bgr.bmp", w, h, d.data() + 8, int(paletteCount), pixels, true);
        std::printf(ok ? "wrote %s_rgb.bmp and %s_bgr.bmp\n" : "failed writing %s_*.bmp\n", prefix.c_str(),
                    prefix.c_str());
        return ok ? 0 : 1;
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc >= 2 && std::strcmp(argv[1], "hexdump") == 0)
        return cmdHexdump(argc, argv);
    if (argc >= 2 && std::strcmp(argv[1], "cat-probe") == 0)
        return cmdCatProbe(argc, argv);
    if (argc >= 2 && std::strcmp(argv[1], "cat-list") == 0)
        return cmdCatList(argc, argv);
    if (argc >= 2 && std::strcmp(argv[1], "cat-extract") == 0)
        return cmdCatExtract(argc, argv);
    if (argc >= 2 && std::strcmp(argv[1], "stb-dump") == 0)
        return cmdStbDump(argc, argv);
    if (argc >= 2 && std::strcmp(argv[1], "lbp-info") == 0)
        return cmdLbpInfo(argc, argv);
    std::fprintf(stderr,
                 "usage:\n"
                 "  reluva-tool hexdump <file> [offset] [length]\n"
                 "  reluva-tool cat-probe <file.cat> [-q]\n"
                 "  reluva-tool cat-list <file.cat>\n"
                 "  reluva-tool cat-extract <file.cat> <output dir> [name filter]\n"
                 "  reluva-tool stb-dump <file.stb>\n"
                 "  reluva-tool lbp-info <file> [output prefix]\n");
    return 2;
}
