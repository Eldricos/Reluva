// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "common/savestate.h"

#include <algorithm>
#include <bit>
#include <cstdio>
#include <fstream>

namespace reluva::save {

void Serializer::raw(void* p, size_t n) {
    auto* b = static_cast<uint8_t*>(p);
    if (saving_) {
        const size_t at = bytes_.size();
        bytes_.insert(bytes_.end(), b, b + n);
        if constexpr (std::endian::native == std::endian::big)
            std::reverse(bytes_.begin() + long(at), bytes_.end());
        return;
    }
    if (!ok() || bytes_.size() - pos_ < n) {
        fail("the state ends early");
        std::memset(b, 0, n);
        return;
    }
    std::memcpy(b, bytes_.data() + pos_, n);
    if constexpr (std::endian::native == std::endian::big)
        std::reverse(b, b + n);
    pos_ += n;
}

uint32_t Serializer::count(size_t n, size_t minBytes) {
    uint32_t c = uint32_t(n);
    raw(&c, 4);
    if (loading() && ok() && uint64_t(c) * std::max<size_t>(minBytes, 1) > bytes_.size() - pos_) {
        fail("a list longer than the state");
        c = 0;
    }
    return ok() ? c : 0;
}

void makeThumbnail(const uint8_t* rgba, int w, int h, int pitch, Header& out) {
    out.thumbnail.clear();
    out.thumbWidth = out.thumbHeight = 0;
    if (!rgba || w <= 0 || h <= 0)
        return;
    // Fit, keeping the shape; each thumbnail pixel the average of its box.
    const double scale = std::min(double(kThumbWidth) / w, double(kThumbHeight) / h);
    const int tw = std::max(1, int(w * scale)), th = std::max(1, int(h * scale));
    out.thumbWidth = tw, out.thumbHeight = th;
    out.thumbnail.resize(size_t(tw) * th * 4);
    for (int y = 0; y < th; ++y)
        for (int x = 0; x < tw; ++x) {
            const int x0 = x * w / tw, x1 = std::max(x0 + 1, (x + 1) * w / tw);
            const int y0 = y * h / th, y1 = std::max(y0 + 1, (y + 1) * h / th);
            uint32_t sum[4] = {};
            for (int sy = y0; sy < y1; ++sy)
                for (int sx = x0; sx < x1; ++sx)
                    for (int c = 0; c < 4; ++c)
                        sum[c] += rgba[size_t(sy) * pitch + size_t(sx) * 4 + c];
            const uint32_t n = uint32_t((x1 - x0) * (y1 - y0));
            for (int c = 0; c < 4; ++c)
                out.thumbnail[(size_t(y) * tw + x) * 4 + c] = uint8_t(sum[c] / n);
        }
}

namespace {

constexpr char kMagic[4] = {'R', 'L', 'V', 'S'};

uint32_t crc32(const std::vector<uint8_t>& d) {
    static uint32_t table[256];
    static bool built = false;
    if (!built) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k)
                c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
        built = true;
    }
    uint32_t c = 0xFFFFFFFFu;
    for (uint8_t b : d)
        c = table[(c ^ b) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

void syncHeader(Serializer& s, Header& h) {
    s(h.engineId)(h.gameId)(h.target)(h.stateVersion)(h.description)(h.created)(h.reluvaVersion);
    s(h.thumbWidth)(h.thumbHeight)(h.thumbnail);
    if (s.loading() && h.thumbnail.size() != size_t(std::max(h.thumbWidth, 0)) * std::max(h.thumbHeight, 0) * 4) {
        h.thumbnail.clear();
        h.thumbWidth = h.thumbHeight = 0;
    }
}

void putU32(std::ofstream& out, uint32_t v) {
    const uint8_t b[4] = {uint8_t(v), uint8_t(v >> 8), uint8_t(v >> 16), uint8_t(v >> 24)};
    out.write(reinterpret_cast<const char*>(b), 4);
}

bool getU32(std::ifstream& in, uint32_t& v) {
    uint8_t b[4];
    if (!in.read(reinterpret_cast<char*>(b), 4))
        return false;
    v = uint32_t(b[0]) | uint32_t(b[1]) << 8 | uint32_t(b[2]) << 16 | uint32_t(b[3]) << 24;
    return true;
}

// Reads the magic, the format and the header; leaves `in` at the state.
Error openFile(const std::filesystem::path& file, std::ifstream& in, Header& header) {
    in.open(file, std::ios::binary);
    if (!in)
        return Error::fail("can't open " + file.string());
    char magic[4];
    uint32_t format = 0, size = 0;
    if (!in.read(magic, 4) || std::memcmp(magic, kMagic, 4) != 0)
        return Error::fail("not a Reluva savestate");
    if (!getU32(in, format) || format != kFormatVersion)
        return Error::fail("a savestate of another format (" + std::to_string(format) + ")");
    std::error_code ec;
    const uint64_t fileSize = std::filesystem::file_size(file, ec);
    if (!getU32(in, size) || ec || size > fileSize)
        return Error::fail("a damaged savestate");
    std::vector<uint8_t> bytes(size);
    if (!in.read(reinterpret_cast<char*>(bytes.data()), size))
        return Error::fail("a damaged savestate");
    Serializer s = Serializer::loader(std::move(bytes));
    header = {};
    syncHeader(s, header);
    if (!s.ok())
        return Error::fail("a damaged savestate header: " + s.error());
    return Error::success();
}

} // namespace

Error writeFile(const std::filesystem::path& file, const Header& header, const std::vector<uint8_t>& state) {
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(), ec);
    Serializer s = Serializer::saver();
    Header h = header;
    syncHeader(s, h);
    // Written beside it first, so a failed save leaves the old one.
    std::filesystem::path temp = file;
    temp += ".tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out)
            return Error::fail("can't write " + temp.string());
        out.write(kMagic, 4);
        putU32(out, kFormatVersion);
        putU32(out, uint32_t(s.bytes().size()));
        out.write(reinterpret_cast<const char*>(s.bytes().data()), std::streamsize(s.bytes().size()));
        putU32(out, uint32_t(state.size()));
        out.write(reinterpret_cast<const char*>(state.data()), std::streamsize(state.size()));
        putU32(out, crc32(state));
        if (!out.flush())
            return Error::fail("can't write " + temp.string());
    }
    std::filesystem::rename(temp, file, ec);
    if (ec)
        return Error::fail("can't save to " + file.string() + ": " + ec.message());
    return Error::success();
}

Error readHeader(const std::filesystem::path& file, Header& header) {
    std::ifstream in;
    return openFile(file, in, header);
}

Error readFile(const std::filesystem::path& file, Header& header, std::vector<uint8_t>& state) {
    std::ifstream in;
    if (Error e = openFile(file, in, header))
        return e;
    uint32_t size = 0, crc = 0;
    std::error_code ec;
    const uint64_t fileSize = std::filesystem::file_size(file, ec);
    if (!getU32(in, size) || ec || size > fileSize)
        return Error::fail("a damaged savestate");
    state.resize(size);
    if (!in.read(reinterpret_cast<char*>(state.data()), size) || !getU32(in, crc) || crc != crc32(state))
        return Error::fail("a damaged savestate (its check doesn't match)");
    return Error::success();
}

std::filesystem::path slotPath(const std::filesystem::path& dir, int slot) {
    char name[16];
    std::snprintf(name, sizeof name, "slot%02d.rlvs", slot);
    return dir / name;
}

std::vector<Slot> listSlots(const std::filesystem::path& dir) {
    std::vector<Slot> out;
    std::error_code ec;
    for (int n = 1; n <= kMaxSlot; ++n) {
        const auto p = slotPath(dir, n);
        if (!std::filesystem::is_regular_file(p, ec))
            continue;
        Slot s;
        s.number = n;
        if (Error e = readHeader(p, s.header))
            s.error = e.message;
        out.push_back(std::move(s));
    }
    return out;
}

int freeSlot(const std::filesystem::path& dir) {
    std::error_code ec;
    for (int n = 1; n <= kMaxSlot; ++n)
        if (!std::filesystem::exists(slotPath(dir, n), ec))
            return n;
    return 0;
}

} // namespace reluva::save
