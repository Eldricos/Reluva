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
//   0x00 u16 1, 0x02 u32 file size - 6, 0x06 u16 5, 0x08 u32 data base, 0x0c u32 count
//   entries from 0x10: u32 offset, u16 index, u8 0, u8 archive byte, char path[260], metadata
//   stored file i: at base + offset, u16 tag, u32 size, then the data
struct Entry {
    size_t start = 0, stride = 0;
    uint32_t offset = 0, metaSize = 0, mtime = 0;
    unsigned index = 0;
    std::string name;
};

constexpr size_t kPathLen = 260;
constexpr size_t kEntrySize = 334;

std::string readName(const Bytes& d, size_t p) {
    std::string n;
    for (size_t k = p; k < p + kPathLen && k < d.size() && d[k]; ++k) {
        if (d[k] < 32 || d[k] >= 127)
            return "<binary>";
        n += char(d[k]);
    }
    return n.empty() ? "<empty>" : n;
}

int cmdCatProbe(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: reluva-tool cat-probe <file.cat>\n");
        return 2;
    }
    Bytes d;
    if (!readFile(argv[2], d))
        return 1;

    const uint32_t base = rd32(d, 8);
    const uint32_t count = rd32(d, 0x0c);
    const size_t declaredSize = size_t(rd32(d, 2)) + 6;
    std::printf("file size        %zu\n", d.size());
    std::printf("0x00 u16         %u\n", rd16(d, 0));
    std::printf("0x02 u32         %u  (file size - 6 = %zu)\n", rd32(d, 2), d.size() - 6);
    std::printf("0x06 u16         %u\n", rd16(d, 6));
    std::printf("0x08 u32 base?   %u (0x%x)\n", base, base);
    std::printf("0x0c u32 count?  %u\n", count);
    const uint8_t archiveByte = d.size() > 0x17 ? d[0x17] : 0;
    std::printf("archive byte     0x%02x\n\n", archiveByte);

    // Locate entry i by its index and archive byte, and an offset that moves
    // forward, without assuming a stride or a readable name.
    std::vector<Entry> entries;
    size_t pos = 0x10;
    for (unsigned i = 0; i < count && pos < d.size(); ++i) {
        const uint32_t minOffset = entries.empty() ? 0 : entries.back().offset + 1;
        size_t found = 0;
        for (size_t j = pos; j + 8 < d.size() && j < pos + 8192; ++j) {
            uint32_t off = rd32(d, j);
            if (rd16(d, j + 4) == i && d[j + 6] == 0 && d[j + 7] == archiveByte && off >= minOffset && off < declaredSize) {
                found = j;
                break;
            }
        }
        if (!found && i == 0)
            found = 0x10;
        if (!found) {
            std::printf("entry %u: not found within 8 KB of 0x%zx\n", i, pos);
            hexdump(d, pos, 128);
            break;
        }
        Entry e;
        e.start = found;
        e.offset = rd32(d, found);
        e.index = i;
        e.name = readName(d, found + 8);
        e.metaSize = rd32(d, found + 8 + 0x116);
        e.mtime = rd32(d, found + 8 + 0x11e);
        if (!entries.empty())
            entries.back().stride = e.start - entries.back().start;
        entries.push_back(e);
        pos = e.start + 8;
    }

    std::printf("%4s %7s %6s %9s %8s %8s %4s %8s  %-16s %s\n", "idx", "at", "stride", "offset", "metasize",
                "chainsz", "tag", "prefixsz", "modified (UTC)", "name");
    int mismatches = 0;
    for (size_t i = 0; i < entries.size(); ++i) {
        const Entry& e = entries[i];
        const size_t blob = size_t(base) + e.offset;
        const uint16_t tag = rd16(d, blob);
        const uint32_t prefixSize = rd32(d, blob + 2);
        long long chain = i + 1 < entries.size() ? (long long)entries[i + 1].offset - e.offset - 6 : -1;
        bool ok = chain < 0 || chain == prefixSize;
        mismatches += !ok;
        std::printf("%4u %7zx %6zu %9u %8u %8lld %4x %8u%s %-16s %s\n", e.index, e.start, e.stride, e.offset,
                    e.metaSize, chain, tag, prefixSize, ok ? " " : "!", formatTime(e.mtime).c_str(), e.name.c_str());
    }
    std::printf("\nentries found: %zu of %u, prefix/chain size mismatches: %d\n", entries.size(), count, mismatches);
    if (entries.empty())
        return 1;

    const Entry& last = entries.back();
    const size_t lastEnd = size_t(base) + last.offset + 6 + rd32(d, size_t(base) + last.offset + 2);
    std::printf("last stored file ends at %zu; file size %zu; %lld bytes after it\n", lastEnd, d.size(),
                (long long)d.size() - (long long)lastEnd);
    std::printf("entry table: 0x10 .. 0x%zx (last entry at 0x%zx); data base 0x%x\n",
                last.start + kEntrySize, last.start, base);

    std::printf("\nbytes before the first stored file:\n");
    hexdump(d, base > 0x60 ? base - 0x60 : 0, 0x80);

    std::printf("\nRIFF signatures (a stored WAV must start with one):\n");
    for (size_t j = 0; j + 4 <= d.size(); ++j) {
        if (std::memcmp(&d[j], "RIFF", 4) != 0)
            continue;
        const Entry* owner = nullptr;
        for (const Entry& e : entries)
            if (size_t(base) + e.offset + 6 == j)
                owner = &e;
        if (owner)
            std::printf("  0x%zx = data of entry %u (%s)\n", j, owner->index, owner->name.c_str());
        else
            std::printf("  0x%zx = not at the start of any entry's data\n", j);
    }

    std::printf("\ndistinct prefix tags:");
    std::vector<uint16_t> tags;
    for (const Entry& e : entries) {
        uint16_t t = rd16(d, size_t(base) + e.offset);
        bool seen = false;
        for (uint16_t x : tags)
            seen |= x == t;
        if (!seen) {
            tags.push_back(t);
            std::printf(" 0x%x", t);
        }
    }
    std::printf("\n");
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
