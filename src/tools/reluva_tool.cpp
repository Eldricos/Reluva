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

int cmdCatProbe(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: reluva-tool cat-probe <file.cat>\n");
        return 2;
    }
    Bytes d;
    if (!readFile(argv[2], d))
        return 1;

    const uint32_t tocSize = rd32(d, 8);
    const uint32_t count = rd32(d, 0x0c);
    const size_t tocEnd = 0x0c + size_t(tocSize);
    std::printf("file size %zu; root chunk tag %u size %u (%s)\n", d.size(), rd16(d, 0), rd32(d, 2),
                rd32(d, 2) + 6 == d.size() ? "matches" : "MISMATCH");
    std::printf("table chunk tag %u size %u, ends at 0x%zx; %u entries\n\n", rd16(d, 6), tocSize, tocEnd, count);

    std::printf("%4s %6s %9s %4s %8s  %-16s %s\n", "idx", "at", "chunk at", "tag", "size", "modified (UTC)",
                "name (indented by folder)");
    std::vector<size_t> folderEnds; // chunk end positions of open folders
    std::vector<std::pair<uint16_t, unsigned>> tagCounts;
    size_t pos = 0x10;
    size_t expectedChunk = tocEnd; // each chunk must start where the previous one ended
    unsigned files = 0, folders = 0, indexGaps = 0, chainBreaks = 0, sizeMismatches = 0;
    int prevIndex = -1;
    for (unsigned i = 0; i < count; ++i) {
        const int index = rd16(d, pos + 4);
        if (pos + 8 > d.size() || index <= prevIndex || d[pos + 6] != 0 || d[pos + 7] != d[0x17]) {
            std::printf("entry #%u at 0x%zx doesn't look like an entry (index %d after %d)\n", i, pos, index,
                        prevIndex);
            hexdump(d, pos > 0x20 ? pos - 0x20 : 0, 0x80);
            std::printf("RESULT: FAIL at entry #%u\n", i);
            return 1;
        }
        if (prevIndex >= 0 && index != prevIndex + 1)
            indexGaps += unsigned(index - prevIndex - 1);
        prevIndex = index;

        const size_t chunk = size_t(rd32(d, pos)) + tocSize;
        const uint16_t tag = rd16(d, chunk);
        const uint32_t size = rd32(d, chunk + 2);
        const bool chainOk = chunk == expectedChunk;
        chainBreaks += !chainOk;
        while (!folderEnds.empty() && chunk >= folderEnds.back())
            folderEnds.pop_back();
        const std::string indent(folderEnds.size() * 2, ' ');

        bool counted = false;
        for (auto& tc : tagCounts)
            if (tc.first == tag) {
                ++tc.second;
                counted = true;
            }
        if (!counted)
            tagCounts.push_back({tag, 1});

        if (tag == kFolderTag) {
            std::printf("%4d %6zx %9zx%s %4x %8u  %-16s %s<folder>\n", index, pos, chunk, chainOk ? " " : "!", tag,
                        size, "", indent.c_str());
            folderEnds.push_back(chunk + 6 + size);
            expectedChunk = chunk + 6; // children follow the folder header
            pos += kFolderEntrySize;
            ++folders;
            continue;
        }
        const size_t p = pos + 8;
        const uint32_t metaSize = rd32(d, p + 0x116);
        sizeMismatches += metaSize != size;
        std::printf("%4d %6zx %9zx%s %4x %8u%s %-16s %s%s\n", index, pos, chunk, chainOk ? " " : "!", tag, size,
                    metaSize == size ? " " : "!", formatTime(rd32(d, p + 0x11e)).c_str(), indent.c_str(),
                    readName(d, p).c_str());
        expectedChunk = chunk + 6 + size;
        pos += kFileEntrySize;
        ++files;
    }

    const bool tableOk = pos == tocEnd;
    std::printf("\n%u files, %u folders, %u index gaps; table walk ended at 0x%zx, table chunk ends at 0x%zx\n", files,
                folders, indexGaps, pos, tocEnd);
    std::printf("last chunk ends at %zu, file size %zu (%lld bytes after)\n", expectedChunk, d.size(),
                (long long)d.size() - (long long)expectedChunk);
    std::printf("chunk tags:");
    for (auto& tc : tagCounts)
        std::printf("  0x%x x%u", tc.first, tc.second);
    std::printf("\n");
    const bool ok = tableOk && chainBreaks == 0 && sizeMismatches == 0 && expectedChunk <= d.size();
    std::printf("RESULT: %s (%u files, %u folders, table %s, %u chain breaks, %u size mismatches, %lld trailing bytes)\n",
                ok ? "OK" : "FAIL", files, folders, tableOk ? "exact" : "MISMATCH", chainBreaks, sizeMismatches,
                (long long)d.size() - (long long)expectedChunk);
    return ok ? 0 : 1;
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
