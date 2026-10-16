// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// reluva-tool: developer utilities for examining game data files.
//
//   reluva-tool hexdump <file> [offset] [length]   offset may be hex (0x..) or
//                                                  negative (from the end)
//   reluva-tool cat-probe <file>                   test the current .cat layout
//                                                  hypothesis and report where it breaks

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

// Current hypothesis (see docs/formats/newworlds/cat.md):
//   0x00 u16 1, 0x02 u32 file size - 6, 0x06 u16 5, 0x08 u32 ?, 0x0c u32 count
//   then entries: u32 offset, u16 index, u8 0, u8 type, char path[260], metadata
struct Entry {
    size_t start;
    uint32_t offset, size, mtime;
    unsigned index, type;
    std::string name;
};

constexpr size_t kPathLen = 260;
constexpr size_t kEntrySize = 334;

int cmdCatProbe(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: reluva-tool cat-probe <file.cat>\n");
        return 2;
    }
    Bytes d;
    if (!readFile(argv[2], d))
        return 1;

    std::printf("file size        %zu\n", d.size());
    std::printf("0x00 u16         %u\n", rd16(d, 0));
    std::printf("0x02 u32         %u  (file size - 6 = %zu)\n", rd32(d, 2), d.size() - 6);
    std::printf("0x06 u16         %u\n", rd16(d, 6));
    std::printf("0x08 u32         %u (0x%x)\n", rd32(d, 8), rd32(d, 8));
    const uint32_t count = rd32(d, 0x0c);
    std::printf("0x0c u32 count?  %u\n\n", count);

    // Find each entry by its signature (index, 0, type, "X:\") rather than
    // assuming a fixed stride, so any irregularity shows up.
    std::vector<Entry> entries;
    size_t pos = 0x10;
    for (unsigned i = 0; i < count && pos < d.size(); ++i) {
        size_t found = 0;
        for (size_t j = pos; j + 8 < d.size() && j < pos + 4096; ++j) {
            if (rd16(d, j) == i && d[j + 2] == 0 && d[j + 5] == ':' && d[j + 6] == '\\') {
                found = j;
                break;
            }
        }
        if (!found) {
            std::printf("entry %u: signature not found within 4 KB of 0x%zx\n", i, pos);
            hexdump(d, pos, 128);
            break;
        }
        Entry e;
        e.start = found - 4;
        e.offset = rd32(d, found - 4);
        e.index = i;
        e.type = d[found + 3];
        const size_t p = found + 4;
        for (size_t k = p; k < p + kPathLen && k < d.size() && d[k]; ++k)
            e.name += char(d[k]);
        e.size = rd32(d, p + 0x116);
        e.mtime = rd32(d, p + 0x11e);
        entries.push_back(e);
        pos = e.start + kEntrySize;
    }

    std::printf("%4s %8s %6s %4s %9s %8s %5s %-16s %s\n", "idx", "at", "stride", "type", "offset", "size",
                "gap", "modified (UTC)", "name");
    for (size_t i = 0; i < entries.size(); ++i) {
        const Entry& e = entries[i];
        long long stride = i ? (long long)(e.start - entries[i - 1].start) : 0;
        long long gap = i ? (long long)e.offset - (long long)entries[i - 1].offset - (long long)entries[i - 1].size : 0;
        std::printf("%4u %8zx %6lld   %02x %9u %8u %5lld %-16s %s\n", e.index, e.start, stride, e.type,
                    e.offset, e.size, gap, formatTime(e.mtime).c_str(), e.name.c_str());
    }
    if (entries.empty())
        return 1;

    const size_t tocEnd = entries.back().start + kEntrySize;
    std::printf("\nentries found: %zu of %u; table ends at 0x%zx (%zu)\n", entries.size(), count, tocEnd, tocEnd);
    std::printf("\nbytes at end of table:\n");
    hexdump(d, tocEnd, 64);

    // A stored WAV must start with "RIFF": comparing where RIFF really is with
    // the entry's offset reveals the base the offsets are relative to.
    std::printf("\nRIFF signatures:\n");
    for (size_t j = 0; j + 4 <= d.size(); ++j) {
        if (std::memcmp(&d[j], "RIFF", 4) != 0)
            continue;
        std::printf("  at 0x%zx (%zu)\n", j, j);
        for (const Entry& e : entries) {
            const std::string& n = e.name;
            if (n.size() > 4 && (n.compare(n.size() - 4, 4, ".wav") == 0 || n.compare(n.size() - 4, 4, ".WAV") == 0))
                std::printf("    if this is %s (offset %u): base = %lld\n", n.c_str(), e.offset,
                            (long long)j - (long long)e.offset);
        }
        hexdump(d, j >= 16 ? j - 16 : 0, 32);
    }

    std::printf("\nfirst bytes at offset+base for the first 3 entries, for some candidate bases:\n");
    const size_t candidates[] = {0, 0x0c, tocEnd, size_t(rd32(d, 8)), size_t(rd32(d, 8)) + 0x0c};
    for (size_t base : candidates) {
        std::printf(" base 0x%zx:\n", base);
        for (size_t i = 0; i < entries.size() && i < 3; ++i)
            hexdump(d, base + entries[i].offset, 16);
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc >= 2 && std::strcmp(argv[1], "hexdump") == 0)
        return cmdHexdump(argc, argv);
    if (argc >= 2 && std::strcmp(argv[1], "cat-probe") == 0)
        return cmdCatProbe(argc, argv);
    std::fprintf(stderr,
                 "usage:\n"
                 "  reluva-tool hexdump <file> [offset] [length]\n"
                 "  reluva-tool cat-probe <file.cat>\n");
    return 2;
}
