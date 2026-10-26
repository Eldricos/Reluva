// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "common/video/bink.h"

#include "common/video/bink_tables.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <complex>
#include <cstring>
#include <numbers>

namespace reluva::video {

namespace {

uint32_t le32(const uint8_t* p) {
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}

// Bits read low bit first from little-endian bytes (Bink's order).
class Bits {
public:
    Bits(const uint8_t* data, size_t bytes) : data_(data), size_(bytes * 8) {}
    uint32_t get(int n) {
        const uint32_t v = peek(n);
        pos_ += n;
        return v;
    }
    uint32_t peek(int n) const {
        if (n == 0)
            return 0;
        uint64_t w = 0;
        const size_t byte = pos_ >> 3;
        for (int i = 0; i < 8 && byte + i < size_ / 8; ++i)
            w |= uint64_t(data_[byte + i]) << (8 * i);
        return uint32_t((w >> (pos_ & 7)) & ((uint64_t(1) << n) - 1));
    }
    bool bit() { return get(1) != 0; }
    void skip(size_t n) { pos_ += n; }
    void align32() { pos_ = (pos_ + 31) & ~size_t(31); }
    size_t pos() const { return pos_; }
    long left() const { return long(size_) - long(pos_); }

private:
    const uint8_t* data_;
    size_t size_, pos_ = 0;
};

// The 16 fixed trees as lookup tables on the next 7 bits: the leaf and its
// code's length.
struct TreeTable {
    uint8_t leaf[128], length[128];
};
const TreeTable* treeTables() {
    static TreeTable tables[16];
    static bool built = false;
    if (!built) {
        for (int t = 0; t < 16; ++t)
            for (int v = 0; v < 128; ++v)
                for (int s = 0; s < 16; ++s) {
                    const int len = kBinkTreeLengths[t][s];
                    if ((v & ((1 << len) - 1)) == kBinkTreeCodes[t][s]) {
                        tables[t].leaf[v] = uint8_t(s);
                        tables[t].length[v] = uint8_t(len);
                        break;
                    }
                }
        built = true;
    }
    return tables;
}

// A tree in use: which of the 16, and the symbol each leaf stands for.
struct Tree {
    int table = 0;
    uint8_t symbol[16] = {};
    int read(Bits& b) const {
        const TreeTable& t = treeTables()[table];
        const uint32_t v = b.peek(7);
        b.skip(t.length[v]);
        return symbol[t.leaf[v]];
    }
};

// Two runs of `size` symbols merged in the order the bits choose.
void merge(Bits& b, uint8_t* dst, const uint8_t* src, int size) {
    const uint8_t* src2 = src + size;
    int size1 = size, size2 = size;
    do {
        if (!b.bit()) {
            *dst++ = *src++;
            --size1;
        } else {
            *dst++ = *src2++;
            --size2;
        }
    } while (size1 && size2);
    while (size1--)
        *dst++ = *src++;
    while (size2--)
        *dst++ = *src2++;
}

void readTree(Bits& b, Tree& tree) {
    tree.table = int(b.get(4));
    if (!tree.table) {
        for (int i = 0; i < 16; ++i)
            tree.symbol[i] = uint8_t(i);
        return;
    }
    if (b.bit()) {
        // The first few symbols listed, the rest in order.
        bool used[16] = {};
        int len = int(b.get(3));
        for (int i = 0; i <= len; ++i) {
            tree.symbol[i] = uint8_t(b.get(4));
            used[tree.symbol[i]] = true;
        }
        for (int i = 0; i < 16 && len < 15; ++i)
            if (!used[i])
                tree.symbol[++len] = uint8_t(i);
    } else {
        // 0-15 shuffled by up to four rounds of merging.
        uint8_t a[16], c[16];
        uint8_t *in = a, *out = c;
        for (int i = 0; i < 16; ++i)
            in[i] = uint8_t(i);
        const int rounds = int(b.get(2));
        for (int i = 0; i <= rounds; ++i) {
            const int size = 1 << i;
            for (int t = 0; t < 16; t += size << 1)
                merge(b, out + t, in + t, size);
            std::swap(in, out);
        }
        std::memcpy(tree.symbol, in, 16);
    }
}

// The kinds of data a plane's rows carry, each in its own bundle.
enum Source { BlockTypes, SubBlockTypes, Colours, Pattern, XOff, YOff, IntraDc, InterDc, Run, Sources };

enum Block { Skip, Scaled, Motion, RunBlock, Residue, Intra, Fill, Inter, PatternBlock, Raw };

struct Bundle {
    int lengthBits = 0;
    Tree tree;
    std::vector<int16_t> data;
    int decoded = 0; // -1 once the plane's values are all in
    int read = 0;
};

constexpr uint8_t kRleLengths[4] = {4, 8, 12, 32};

int log2floor(unsigned v) { return int(std::bit_width(v)) - 1; }

// One plane's decoder state.
class PlaneDecoder {
public:
    PlaneDecoder(char version, Bits& bits, std::vector<Bundle>& bundles, Tree* colourHigh)
        : version_(version), b_(bits), bundles_(bundles), colourHigh_(colourHigh) {}

    bool decode(uint8_t* dst, const uint8_t* prev, int stride, int width, int height, int bw, int bh);

private:
    // Whether a bundle wants more values now (a new count read; false
    // while it still holds some or has had them all).
    bool count(Bundle& bu, int& n) {
        if (bu.decoded < 0 || bu.decoded > bu.read)
            return false;
        n = int(b_.get(bu.lengthBits));
        if (!n) {
            bu.decoded = -1;
            return false;
        }
        return true;
    }
    bool readRuns(Bundle& bu);
    bool readMotion(Bundle& bu);
    bool readBlockTypes(Bundle& bu);
    bool readPatterns(Bundle& bu);
    bool readColours(Bundle& bu);
    bool readDcs(Bundle& bu, bool hasSign);
    int value(Source s) {
        Bundle& bu = bundles_[s];
        return bu.read < int(bu.data.size()) ? bu.data[bu.read++] : 0;
    }
    int readCoefficients(int32_t block[64], int& count, int index[64]);
    void readResidue(int16_t block[64], int masks);
    bool copyMotion(uint8_t* dst, const uint8_t* prev, const uint8_t* start, const uint8_t* end, int stride);

    char version_;
    Bits& b_;
    std::vector<Bundle>& bundles_;
    Tree* colourHigh_;
    int lastHigh_ = 0;
};

bool PlaneDecoder::readRuns(Bundle& bu) {
    int n;
    if (!count(bu, n))
        return true;
    if (bu.decoded + n > int(bu.data.size()))
        return false;
    if (b_.bit()) {
        const int v = int(b_.get(4));
        std::fill_n(bu.data.begin() + bu.decoded, n, int16_t(v));
        bu.decoded += n;
    } else {
        for (const int end = bu.decoded + n; bu.decoded < end;)
            bu.data[bu.decoded++] = int16_t(bu.tree.read(b_));
    }
    return true;
}

bool PlaneDecoder::readMotion(Bundle& bu) {
    int n;
    if (!count(bu, n))
        return true;
    if (bu.decoded + n > int(bu.data.size()))
        return false;
    auto sign = [&](int v) { return v && b_.bit() ? -v : v; };
    if (b_.bit()) {
        const int v = sign(int(b_.get(4)));
        std::fill_n(bu.data.begin() + bu.decoded, n, int16_t(v));
        bu.decoded += n;
    } else {
        for (const int end = bu.decoded + n; bu.decoded < end;)
            bu.data[bu.decoded++] = int16_t(sign(bu.tree.read(b_)));
    }
    return true;
}

bool PlaneDecoder::readBlockTypes(Bundle& bu) {
    int n;
    if (!count(bu, n))
        return true;
    if (version_ == 'k') {
        n ^= 0xBB;
        if (!n) {
            bu.decoded = -1;
            return true;
        }
    }
    if (bu.decoded + n > int(bu.data.size()))
        return false;
    const int end = bu.decoded + n;
    if (b_.bit()) {
        std::fill_n(bu.data.begin() + bu.decoded, n, int16_t(b_.get(4)));
        bu.decoded = end;
    } else {
        int last = 0;
        while (bu.decoded < end) {
            const int v = bu.tree.read(b_);
            if (v < 12) {
                last = v;
                bu.data[bu.decoded++] = int16_t(v);
            } else {
                const int run = kRleLengths[v - 12];
                if (end - bu.decoded < run)
                    return false;
                std::fill_n(bu.data.begin() + bu.decoded, run, int16_t(last));
                bu.decoded += run;
            }
        }
    }
    return true;
}

bool PlaneDecoder::readPatterns(Bundle& bu) {
    int n;
    if (!count(bu, n))
        return true;
    if (bu.decoded + n > int(bu.data.size()))
        return false;
    for (const int end = bu.decoded + n; bu.decoded < end;) {
        int v = bu.tree.read(b_);
        v |= bu.tree.read(b_) << 4;
        bu.data[bu.decoded++] = int16_t(v);
    }
    return true;
}

bool PlaneDecoder::readColours(Bundle& bu) {
    int n;
    if (!count(bu, n))
        return true;
    if (bu.decoded + n > int(bu.data.size()))
        return false;
    auto one = [&] {
        lastHigh_ = colourHigh_[lastHigh_].read(b_);
        int v = lastHigh_ << 4 | bu.tree.read(b_);
        if (version_ < 'i') {
            // Older files store the colour as sign and magnitude about 128.
            const int sign = (v & 0x80) ? -1 : 0;
            v = (((v & 0x7F) ^ sign) - sign) + 0x80;
        }
        return int16_t(v & 0xFF);
    };
    if (b_.bit()) {
        std::fill_n(bu.data.begin() + bu.decoded, n, one());
        bu.decoded += n;
    } else {
        for (const int end = bu.decoded + n; bu.decoded < end;)
            bu.data[bu.decoded++] = one();
    }
    return true;
}

bool PlaneDecoder::readDcs(Bundle& bu, bool hasSign) {
    int n;
    if (!count(bu, n))
        return true;
    if (bu.decoded + n > int(bu.data.size()))
        return false;
    // The first value whole (11 bits), the rest as differences in groups
    // of eight, each group's width given.
    int v = int(b_.get(11 - hasSign));
    if (v && hasSign && b_.bit())
        v = -v;
    bu.data[bu.decoded++] = int16_t(v);
    --n;
    for (int i = 0; i < n; i += 8) {
        const int group = std::min(n - i, 8);
        const int width = int(b_.get(4));
        for (int j = 0; j < group; ++j) {
            if (width) {
                int d = int(b_.get(width));
                if (d && b_.bit())
                    d = -d;
                v += d;
                if (v < -32768 || v > 32767)
                    return false;
            }
            bu.data[bu.decoded++] = int16_t(v);
        }
    }
    return true;
}

// A DCT block's coefficients: which are set, by bit planes from the
// highest down; returns the quantizer's index (or -1).
int PlaneDecoder::readCoefficients(int32_t block[64], int& count, int index[64]) {
    int coef[128], mode[128];
    int start = 64, end = 64;
    count = 0;
    auto add = [&](int c, int m) {
        coef[end] = c;
        mode[end++] = m;
    };
    add(4, 0), add(24, 0), add(44, 0), add(1, 3), add(2, 3), add(3, 3);
    auto set = [&](int c, int bits) {
        int t;
        if (!bits) {
            t = b_.bit() ? -1 : 1;
        } else {
            t = int(b_.get(bits)) | 1 << bits;
            if (b_.bit())
                t = -t;
        }
        block[kBinkScan[c]] = t;
        index[count++] = c;
    };
    for (int bits = int(b_.get(4)) - 1; bits >= 0; --bits) {
        for (int pos = start; pos < end;) {
            if (!(mode[pos] | coef[pos]) || !b_.bit()) {
                ++pos;
                continue;
            }
            int c = coef[pos];
            const int m = mode[pos];
            if (m == 0 || m == 2) {
                if (m == 0) {
                    coef[pos] = c + 4;
                    mode[pos] = 1;
                } else {
                    coef[pos] = 0;
                    mode[pos++] = 0;
                }
                for (int i = 0; i < 4; ++i, ++c) {
                    if (b_.bit()) {
                        coef[--start] = c;
                        mode[start] = 3;
                    } else {
                        set(c, bits);
                    }
                }
            } else if (m == 1) {
                mode[pos] = 2;
                for (int i = 0; i < 3; ++i) {
                    c += 4;
                    add(c, 2);
                }
            } else {
                set(c, bits);
                coef[pos] = 0;
                mode[pos++] = 0;
            }
            if (b_.left() < 0)
                return -1;
        }
    }
    return int(b_.get(4));
}

void unquantize(int32_t block[64], const int32_t quant[64], int count, const int index[64]) {
    auto q = [](int32_t v, int32_t f) { return int32_t(uint32_t(v) * uint32_t(f)) >> 11; };
    block[0] = q(block[0], quant[0]);
    for (int i = 0; i < count; ++i) {
        const int c = index[i];
        block[kBinkScan[c]] = q(block[kBinkScan[c]], quant[c]);
    }
}

// Residue after motion: bit planes of a mask, each setting new
// coefficients or refining set ones, up to `masks` changes.
void PlaneDecoder::readResidue(int16_t block[64], int masks) {
    int coef[128], mode[128], nonzero[64];
    int start = 64, end = 64, nonzeros = 0;
    auto add = [&](int c, int m) {
        coef[end] = c;
        mode[end++] = m;
    };
    add(4, 0), add(24, 0), add(44, 0), add(0, 2);
    for (int mask = 1 << b_.get(3); mask; mask >>= 1) {
        for (int i = 0; i < nonzeros; ++i) {
            if (!b_.bit())
                continue;
            if (block[nonzero[i]] < 0)
                block[nonzero[i]] = int16_t(block[nonzero[i]] - mask);
            else
                block[nonzero[i]] = int16_t(block[nonzero[i]] + mask);
            if (--masks < 0)
                return;
        }
        for (int pos = start; pos < end;) {
            if (!(coef[pos] | mode[pos]) || !b_.bit()) {
                ++pos;
                continue;
            }
            int c = coef[pos];
            const int m = mode[pos];
            if (m == 0 || m == 2) {
                if (m == 0) {
                    coef[pos] = c + 4;
                    mode[pos] = 1;
                } else {
                    coef[pos] = 0;
                    mode[pos++] = 0;
                }
                for (int i = 0; i < 4; ++i, ++c) {
                    if (b_.bit()) {
                        coef[--start] = c;
                        mode[start] = 3;
                    } else {
                        nonzero[nonzeros++] = kBinkScan[c];
                        block[kBinkScan[c]] = int16_t(b_.bit() ? -mask : mask);
                        if (--masks < 0)
                            return;
                    }
                }
            } else if (m == 1) {
                mode[pos] = 2;
                for (int i = 0; i < 3; ++i) {
                    c += 4;
                    add(c, 2);
                }
            } else {
                nonzero[nonzeros++] = kBinkScan[c];
                block[kBinkScan[c]] = int16_t(b_.bit() ? -mask : mask);
                coef[pos] = 0;
                mode[pos++] = 0;
                if (--masks < 0)
                    return;
            }
            if (b_.left() < 0)
                return;
        }
    }
}

// Bink's 8x8 inverse DCT (12-bit constants, 11-bit products); `put`
// stores the result (wrapped to a byte), else adds it.
void idct(int32_t block[64], uint8_t* dst, int stride, bool put) {
    auto mul = [](int x, int y) { return int(uint32_t(x) * uint32_t(y)) >> 11; };
    constexpr int A1 = 2896, A2 = 2217, A3 = 3784, A4 = -5352;
    auto transform = [&](const int* s, int step, int* d, bool round) {
        const int a0 = s[0] + s[4 * step], a1 = s[0] - s[4 * step];
        const int a2 = s[2 * step] + s[6 * step], a3 = mul(A1, s[2 * step] - s[6 * step]);
        const int a4 = s[5 * step] + s[3 * step], a5 = s[5 * step] - s[3 * step];
        const int a6 = s[step] + s[7 * step], a7 = s[step] - s[7 * step];
        const int b0 = a4 + a6, b1 = mul(A3, a5 + a7);
        const int b2 = mul(A4, a5) - b0 + b1, b3 = mul(A1, a6 - a4) - b2, b4 = mul(A2, a7) + b3 - b1;
        const int out[8] = {a0 + a2 + b0, a1 + a3 - a2 + b2, a1 - a3 + a2 + b3, a0 - a2 - b4,
                            a0 - a2 + b4, a1 - a3 + a2 - b3, a1 + a3 - a2 - b2, a0 + a2 - b0};
        for (int i = 0; i < 8; ++i)
            d[i * step] = round ? (out[i] + 0x7F) >> 8 : out[i];
    };
    int src[64], temp[64];
    for (int i = 0; i < 64; ++i)
        src[i] = block[i];
    for (int i = 0; i < 8; ++i) {
        const int* s = src + i;
        if (!(s[8] | s[16] | s[24] | s[32] | s[40] | s[48] | s[56]))
            for (int k = 0; k < 8; ++k)
                temp[i + 8 * k] = s[0];
        else
            transform(s, 8, temp + i, false);
    }
    for (int i = 0; i < 8; ++i) {
        int row[8];
        transform(temp + 8 * i, 1, row, true);
        for (int j = 0; j < 8; ++j)
            dst[i * stride + j] = put ? uint8_t(row[j]) : uint8_t(dst[i * stride + j] + row[j]);
    }
}

bool PlaneDecoder::copyMotion(uint8_t* dst, const uint8_t* prev, const uint8_t* start, const uint8_t* end,
                              int stride) {
    const int x = value(XOff), y = value(YOff);
    const ptrdiff_t at = (prev - start) + x + ptrdiff_t(y) * stride;
    if (at < 0 || at > end - start)
        return false;
    const uint8_t* ref = start + at;
    for (int i = 0; i < 8; ++i)
        std::memcpy(dst + i * stride, ref + i * stride, 8);
    return true;
}

bool PlaneDecoder::decode(uint8_t* frame, const uint8_t* last, int stride, int width, int height, int bw, int bh) {
    if (version_ == 'k' && b_.bit()) {
        const uint8_t fill = uint8_t(b_.get(8));
        for (int y = 0; y < height; ++y)
            std::memset(frame + y * stride, fill, size_t(width));
        b_.align32();
        return true;
    }
    const int w = (std::max(width, 8) + 7) & ~7;
    bundles_[BlockTypes].lengthBits = log2floor(unsigned((w >> 3) + 511)) + 1;
    bundles_[SubBlockTypes].lengthBits = log2floor(unsigned((w >> 4) + 511)) + 1;
    bundles_[Colours].lengthBits = log2floor(unsigned(bw * 64 + 511)) + 1;
    bundles_[IntraDc].lengthBits = bundles_[InterDc].lengthBits = bundles_[XOff].lengthBits =
        bundles_[YOff].lengthBits = log2floor(unsigned((w >> 3) + 511)) + 1;
    bundles_[Pattern].lengthBits = log2floor(unsigned((bw << 3) + 511)) + 1;
    bundles_[Run].lengthBits = log2floor(unsigned(bw * 48 + 511)) + 1;
    for (int s = 0; s < Sources; ++s) {
        if (s == Colours) {
            for (int i = 0; i < 16; ++i)
                readTree(b_, colourHigh_[i]);
            lastHigh_ = 0;
        }
        if (s != IntraDc && s != InterDc)
            readTree(b_, bundles_[s].tree);
        bundles_[s].decoded = bundles_[s].read = 0;
    }
    const uint8_t* refStart = last;
    const uint8_t* refEnd = last + (bw - 1 + stride * (bh - 1)) * 8;
    int map[64];
    for (int i = 0; i < 64; ++i)
        map[i] = (i & 7) + (i >> 3) * stride;

    int32_t dct[64];
    int16_t residue[64];
    uint8_t small[64];
    int count, index[64];
    // A run block: runs of one colour or of single colours, along one of
    // the 16 patterns.
    auto runs = [&](auto put) {
        const uint8_t* scan = kBinkPatterns[b_.get(4)];
        int i = 0;
        do {
            const int run = value(Run) + 1;
            i += run;
            if (i > 64)
                return false;
            if (b_.bit()) {
                const int v = value(Colours);
                for (int j = 0; j < run; ++j)
                    put(*scan++, v);
            } else {
                for (int j = 0; j < run; ++j)
                    put(*scan++, value(Colours));
            }
        } while (i < 63);
        if (i == 63)
            put(*scan++, value(Colours));
        return true;
    };
    auto intra = [&](uint8_t* dst, int s, const int32_t (*quant)[64], Source dc, bool put) {
        std::fill_n(dct, 64, 0);
        dct[0] = value(dc);
        const int q = readCoefficients(dct, count, index);
        if (q < 0 || q > 15)
            return false;
        unquantize(dct, quant[q], count, index);
        idct(dct, dst, s, put);
        return true;
    };

    for (int by = 0; by < bh; ++by) {
        if (!readBlockTypes(bundles_[BlockTypes]) || !readBlockTypes(bundles_[SubBlockTypes]) ||
            !readColours(bundles_[Colours]) || !readPatterns(bundles_[Pattern]) || !readMotion(bundles_[XOff]) ||
            !readMotion(bundles_[YOff]) || !readDcs(bundles_[IntraDc], false) || !readDcs(bundles_[InterDc], true) ||
            !readRuns(bundles_[Run]))
            return false;
        uint8_t* dst = frame + 8 * by * stride;
        const uint8_t* prev = last + 8 * by * stride;
        for (int bx = 0; bx < bw; ++bx, dst += 8, prev += 8) {
            int blk = value(BlockTypes);
            // A 16x16 block's other quarters.
            if (((by & 1) || (bx & 1)) && blk == Scaled) {
                ++bx;
                dst += 8;
                prev += 8;
                continue;
            }
            switch (blk) {
            case Skip:
                for (int i = 0; i < 8; ++i)
                    std::memcpy(dst + i * stride, prev + i * stride, 8);
                break;
            case Scaled: {
                blk = value(SubBlockTypes);
                switch (blk) {
                case RunBlock:
                    if (!runs([&](int at, int v) { small[at] = uint8_t(v); }))
                        return false;
                    break;
                case Intra:
                    if (!intra(small, 8, kBinkIntraQuant, IntraDc, true))
                        return false;
                    break;
                case Fill: {
                    const uint8_t v = uint8_t(value(Colours));
                    for (int i = 0; i < 16; ++i)
                        std::memset(dst + i * stride, v, 16);
                    break;
                }
                case PatternBlock: {
                    const int c[2] = {value(Colours), value(Colours)};
                    for (int j = 0; j < 8; ++j) {
                        int v = value(Pattern);
                        for (int i = 0; i < 8; ++i, v >>= 1)
                            small[i + j * 8] = uint8_t(c[v & 1]);
                    }
                    break;
                }
                case Raw:
                    for (int i = 0; i < 64; ++i)
                        small[i] = uint8_t(value(Colours));
                    break;
                default:
                    return false;
                }
                if (blk != Fill)
                    for (int j = 0; j < 16; ++j)
                        for (int i = 0; i < 16; ++i)
                            dst[j * stride + i] = small[(j >> 1) * 8 + (i >> 1)];
                ++bx;
                dst += 8;
                prev += 8;
                break;
            }
            case Motion:
                if (!copyMotion(dst, prev, refStart, refEnd, stride))
                    return false;
                break;
            case RunBlock:
                if (!runs([&](int at, int v) { dst[map[at]] = uint8_t(v); }))
                    return false;
                break;
            case Residue: {
                if (!copyMotion(dst, prev, refStart, refEnd, stride))
                    return false;
                std::fill_n(residue, 64, int16_t(0));
                readResidue(residue, int(b_.get(7)));
                for (int j = 0; j < 8; ++j)
                    for (int i = 0; i < 8; ++i)
                        dst[j * stride + i] = uint8_t(dst[j * stride + i] + residue[j * 8 + i]);
                break;
            }
            case Intra:
                if (!intra(dst, stride, kBinkIntraQuant, IntraDc, true))
                    return false;
                break;
            case Fill: {
                const uint8_t v = uint8_t(value(Colours));
                for (int i = 0; i < 8; ++i)
                    std::memset(dst + i * stride, v, 8);
                break;
            }
            case Inter:
                if (!copyMotion(dst, prev, refStart, refEnd, stride) ||
                    !intra(dst, stride, kBinkInterQuant, InterDc, false))
                    return false;
                break;
            case PatternBlock: {
                const int c[2] = {value(Colours), value(Colours)};
                for (int i = 0; i < 8; ++i) {
                    int v = value(Pattern);
                    for (int j = 0; j < 8; ++j, v >>= 1)
                        dst[i * stride + j] = uint8_t(c[v & 1]);
                }
                break;
            }
            case Raw:
                for (int i = 0; i < 8; ++i)
                    for (int j = 0; j < 8; ++j)
                        dst[i * stride + j] = uint8_t(value(Colours));
                break;
            default:
                return false;
            }
        }
        if (b_.left() < 0)
            return false;
    }
    b_.align32(); // the next plane starts on a 32-bit boundary
    return true;
}

// Bink audio: blocks of a transform's coefficients, quantized by bands.
class AudioDecoder {
public:
    AudioDecoder(int rate, int channels, bool dct, bool versionB) : dct_(dct), versionB_(versionB) {
        int bits = rate < 22050 ? 9 : rate < 44100 ? 10 : 11;
        int sampleRate = rate;
        if (!dct) {
            // The RDFT kind interleaves the channels in one transform.
            sampleRate *= channels;
            channels_ = 1;
            if (!versionB)
                bits += log2floor(unsigned(channels));
        } else {
            channels_ = channels;
        }
        frameLen_ = 1 << bits;
        overlap_ = frameLen_ / 16;
        const double root = dct ? frameLen_ / (std::sqrt(double(frameLen_)) * 32768.0)
                                : 2.0 / (std::sqrt(double(frameLen_)) * 32768.0);
        root_ = float(root);
        for (int i = 0; i < 96; ++i)
            quantTable_[i] = std::exp(float(i) * 0.15289164787221953823f) * root_;
        static constexpr uint16_t kCritical[25] = {100,  200,  300,  400,  510,  630,  770,  920,  1080,
                                                   1270, 1480, 1720, 2000, 2320, 2700, 3150, 3700, 4400,
                                                   5300, 6400, 7700, 9500, 12000, 15500, 24500};
        const int half = (sampleRate + 1) / 2;
        for (bands_ = 1; bands_ < 25; ++bands_)
            if (half <= kCritical[bands_ - 1])
                break;
        band_[0] = 2;
        for (int i = 1; i < bands_; ++i)
            band_[i] = (kCritical[i - 1] * frameLen_ / half) & ~1;
        band_[bands_] = frameLen_;
        previous_.assign(size_t(channels_), std::vector<float>(size_t(overlap_)));
    }

    // Decodes a packet's blocks onto `out` (interleaved).
    bool packet(const uint8_t* data, size_t size, std::vector<float>& out) {
        if (size < 4)
            return false;
        Bits b(data + 4, size - 4);
        while (b.left() > 0) {
            if (!block(b, out))
                return false;
            b.align32();
        }
        return true;
    }

private:
    float readFloat(Bits& b) {
        const int power = int(b.get(5));
        float f = std::ldexp(float(b.get(23)), power - 23);
        return b.bit() ? -f : f;
    }

    bool block(Bits& b, std::vector<float>& out) {
        static constexpr uint8_t kRle[16] = {2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 32, 64};
        std::vector<std::vector<float>> pcm(static_cast<size_t>(channels_));
        std::vector<float> coeffs(static_cast<size_t>(frameLen_) + 2);
        if (dct_)
            b.skip(2);
        for (int ch = 0; ch < channels_; ++ch) {
            if (versionB_) {
                coeffs[0] = std::bit_cast<float>(b.get(32)) * root_;
                coeffs[1] = std::bit_cast<float>(b.get(32)) * root_;
            } else {
                coeffs[0] = readFloat(b) * root_;
                coeffs[1] = readFloat(b) * root_;
            }
            float quant[25];
            for (int i = 0; i < bands_; ++i)
                quant[i] = quantTable_[std::min<uint32_t>(b.get(8), 95)];
            int k = 0;
            float q = quant[0];
            for (int i = 2; i < frameLen_;) {
                int j;
                if (versionB_)
                    j = i + 16;
                else
                    j = b.bit() ? i + kRle[b.get(4)] * 8 : i + 8;
                j = std::min(j, frameLen_);
                const int width = int(b.get(4));
                if (!width) {
                    std::fill(coeffs.begin() + i, coeffs.begin() + j, 0.0f);
                    i = j;
                    while (band_[k] < i)
                        q = quant[k++];
                } else {
                    for (; i < j; ++i) {
                        if (band_[k] == i)
                            q = quant[k++];
                        const int c = int(b.get(width));
                        coeffs[size_t(i)] = c ? (b.bit() ? -q * float(c) : q * float(c)) : 0.0f;
                    }
                }
                if (b.left() < 0)
                    return false;
            }
            pcm[size_t(ch)] = inverseRdft(coeffs);
        }
        for (int ch = 0; ch < channels_; ++ch) {
            std::vector<float>& p = pcm[size_t(ch)];
            const int span = overlap_ * channels_;
            if (!first_)
                for (int i = 0, j = ch; i < overlap_; ++i, j += channels_)
                    p[size_t(i)] = (previous_[size_t(ch)][size_t(i)] * float(span - j) + p[size_t(i)] * float(j)) /
                                   float(span);
            std::copy(p.end() - overlap_, p.end(), previous_[size_t(ch)].begin());
        }
        first_ = false;
        const int keep = frameLen_ - overlap_;
        if (channels_ == 1) {
            out.insert(out.end(), pcm[0].begin(), pcm[0].begin() + keep);
        } else {
            for (int i = 0; i < keep; ++i)
                for (int ch = 0; ch < channels_; ++ch)
                    out.push_back(pcm[size_t(ch)][size_t(i)]);
        }
        return true;
    }

    // An in-place radix-2 complex FFT (forward when sign is -1).
    static void fft(std::vector<std::complex<float>>& a, int sign) {
        const size_t n = a.size();
        for (size_t i = 1, j = 0; i < n; ++i) {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1)
                j ^= bit;
            j ^= bit;
            if (i < j)
                std::swap(a[i], a[j]);
        }
        for (size_t len = 2; len <= n; len <<= 1) {
            const double ang = sign * 2 * std::numbers::pi / double(len);
            for (size_t i = 0; i < n; i += len)
                for (size_t k = 0; k < len / 2; ++k) {
                    const std::complex<float> w(float(std::cos(ang * double(k))), float(std::sin(ang * double(k))));
                    const std::complex<float> u = a[i + k], v = a[i + k + len / 2] * w;
                    a[i + k] = u + v;
                    a[i + k + len / 2] = u - v;
                }
        }
    }

    // The RDFT kind: coefficients 0 and 1 are the DC and Nyquist terms,
    // then (re, im) pairs; the result is half the real inverse transform.
    std::vector<float> inverseRdft(const std::vector<float>& c) const {
        const int n = frameLen_;
        std::vector<std::complex<float>> spec(static_cast<size_t>(n));
        spec[0] = c[0];
        spec[size_t(n / 2)] = c[1];
        for (int k = 1; k < n / 2; ++k) {
            const std::complex<float> v(c[size_t(2 * k)], c[size_t(2 * k + 1)]);
            spec[size_t(k)] = v;
            spec[size_t(n - k)] = std::conj(v);
        }
        fft(spec, kRdftSign);
        std::vector<float> out(static_cast<size_t>(n));
        for (int i = 0; i < n; ++i)
            out[size_t(i)] = spec[size_t(i)].real() * kRdftScale;
        return out;
    }

public:
    static inline int kRdftSign = -1;
    static inline float kRdftScale = 0.5f;

private:
    bool dct_, versionB_;
    int channels_ = 1;
    int frameLen_ = 0, overlap_ = 0, bands_ = 0;
    float root_ = 0;
    float quantTable_[96] = {};
    int band_[26] = {};
    std::vector<std::vector<float>> previous_;
    bool first_ = true;
};

} // namespace

struct BinkMovie::State {
    std::vector<Bundle> bundles;
    Tree colourHigh[16];
    std::unique_ptr<AudioDecoder> audio;
    uint32_t audioFrame = 0; // the next frame whose audio is due
};

BinkMovie::BinkMovie() : state_(std::make_unique<State>()) {}
BinkMovie::~BinkMovie() = default;

Error BinkMovie::open(const std::filesystem::path& file) {
    file_.close();
    file_.clear();
    file_.open(file, std::ios::binary);
    if (!file_)
        return Error::fail("can't open " + file.string());
    uint8_t h[44];
    if (!file_.read(reinterpret_cast<char*>(h), sizeof h))
        return Error::fail("too short for a Bink movie");
    if (h[0] != 'B' || h[1] != 'I' || h[2] != 'K' || h[3] < 'c' || h[3] > 'k')
        return Error::fail("not a Bink 1 movie (versions c-k)");
    version_ = char(h[3]);
    fileSize_ = le32(h + 4) + 8;
    const uint32_t frames = le32(h + 8);
    width_ = int(le32(h + 20));
    height_ = int(le32(h + 24));
    fpsNum_ = le32(h + 28);
    fpsDen_ = le32(h + 32);
    flags_ = le32(h + 36);
    const uint32_t tracks = le32(h + 40);
    if (!frames || frames > 1000000 || width_ <= 0 || width_ > 7680 || height_ <= 0 || height_ > 4800 || !fpsNum_ ||
        !fpsDen_ || tracks > 256)
        return Error::fail("bad Bink header");
    if (version_ == 'k')
        file_.seekg(4, std::ios::cur);
    tracks_.clear();
    if (tracks) {
        std::vector<uint8_t> t(size_t(tracks) * 12);
        if (!file_.read(reinterpret_cast<char*>(t.data()), std::streamsize(t.size())))
            return Error::fail("bad Bink audio header");
        for (uint32_t i = 0; i < tracks; ++i) {
            const uint8_t* p = t.data() + tracks * 4 + i * 4;
            const int flags = p[2] | p[3] << 8;
            tracks_.push_back({p[0] | p[1] << 8, (flags & 0x2000) != 0, (flags & 0x1000) != 0});
        }
    }
    std::vector<uint8_t> idx(size_t(frames) * 4);
    if (!file_.read(reinterpret_cast<char*>(idx.data()), std::streamsize(idx.size())))
        return Error::fail("bad Bink frame index");
    index_.resize(frames);
    for (uint32_t i = 0; i < frames; ++i)
        index_[i] = le32(idx.data() + i * 4) & ~1u;
    for (uint32_t i = 0; i < frames; ++i)
        if ((i + 1 < frames ? index_[i + 1] : fileSize_) <= index_[i])
            return Error::fail("bad Bink frame index");

    const int bw = (width_ + 7) >> 3, bh = (height_ + 7) >> 3;
    const int cbw = (width_ + 15) >> 4, cbh = (height_ + 15) >> 4;
    ystride_ = bw * 8, yrows_ = bh * 8, cstride_ = cbw * 8, crows_ = cbh * 8;
    for (int p = 0; p < 4; ++p) {
        const size_t n = p == 1 || p == 2 ? size_t(cstride_) * crows_ : size_t(ystride_) * yrows_;
        // Black to start with (video range).
        planes_[p].assign(n, p == 1 || p == 2 ? 128 : p == 3 ? 255 : 16);
        last_[p] = planes_[p];
    }
    next_ = 0;
    state_->bundles.assign(Sources, Bundle{});
    for (Bundle& b : state_->bundles)
        b.data.assign(size_t(bw) * bh * 64, 0);
    state_->audio.reset();
    if (!tracks_.empty() && !tracks_[0].dct)
        state_->audio = std::make_unique<AudioDecoder>(tracks_[0].rate, tracks_[0].stereo ? 2 : 1, false, false);
    state_->audioFrame = 0;
    return Error::success();
}

Error BinkMovie::readPacket(uint32_t frame, std::vector<uint8_t>& out) {
    const uint32_t start = index_[frame], end = frame + 1 < index_.size() ? index_[frame + 1] : fileSize_;
    out.resize(end - start);
    file_.clear();
    file_.seekg(start);
    if (!file_.read(reinterpret_cast<char*>(out.data()), std::streamsize(out.size())))
        return Error::fail("Bink frame " + std::to_string(frame) + " is cut short");
    return Error::success();
}

Error BinkMovie::nextFrame() {
    if (next_ >= index_.size())
        return Error::fail("past the movie's end");
    if (Error e = readPacket(next_, packet_); !e.ok)
        return e;
    // Skip the audio tracks' parts.
    size_t at = 0;
    for (size_t t = 0; t < tracks_.size(); ++t) {
        if (at + 4 > packet_.size())
            return Error::fail("bad Bink frame");
        at += 4 + le32(packet_.data() + at);
    }
    if (at > packet_.size())
        return Error::fail("bad Bink frame");
    for (int p = 0; p < 4; ++p)
        std::swap(planes_[p], last_[p]);
    Bits bits(packet_.data() + at, packet_.size() - at);
    const size_t total = (packet_.size() - at) * 8;
    const int bw = ystride_ / 8, bh = yrows_ / 8, cbw = cstride_ / 8, cbh = crows_ / 8;
    PlaneDecoder dec(version_, bits, state_->bundles, state_->colourHigh);
    if (flags_ & 0x00100000) { // alpha
        if (version_ >= 'i')
            bits.skip(32);
        if (!dec.decode(planes_[3].data(), last_[3].data(), ystride_, width_, height_, bw, bh))
            return Error::fail("bad Bink alpha plane in frame " + std::to_string(next_));
    }
    if (version_ >= 'i')
        bits.skip(32);
    for (int plane = 0; plane < 3; ++plane) {
        // From version h the chroma planes come V first.
        const int p = plane == 0 || version_ < 'h' ? plane : plane ^ 3;
        const bool chroma = plane != 0;
        if (!dec.decode(planes_[p].data(), last_[p].data(), chroma ? cstride_ : ystride_, width_ >> chroma,
                        height_ >> chroma, chroma ? cbw : bw, chroma ? cbh : bh))
            return Error::fail("bad Bink plane " + std::to_string(plane) + " in frame " + std::to_string(next_));
        if (bits.pos() >= total)
            break;
    }
    ++next_;
    return Error::success();
}

void BinkMovie::toRgba(uint8_t* out, int pitch) const {
    // BT.601, video range (version k is full range).
    const bool full = version_ == 'k';
    for (int y = 0; y < height_; ++y) {
        uint8_t* row = out + size_t(y) * pitch;
        const uint8_t* py = planes_[0].data() + size_t(y) * ystride_;
        const uint8_t* pu = planes_[1].data() + size_t(y >> 1) * cstride_;
        const uint8_t* pv = planes_[2].data() + size_t(y >> 1) * cstride_;
        for (int x = 0; x < width_; ++x) {
            const int Y = full ? py[x] * 256 : (py[x] - 16) * 298;
            const int U = pu[x >> 1] - 128, V = pv[x >> 1] - 128;
            const int k = full ? 256 : 255;
            const int r = (Y + (full ? 359 : 409) * V * k / 255 + 128) >> 8;
            const int g = (Y - (full ? 88 : 100) * U * k / 255 - (full ? 183 : 208) * V * k / 255 + 128) >> 8;
            const int b = (Y + (full ? 454 : 516) * U * k / 255 + 128) >> 8;
            row[x * 4 + 0] = uint8_t(std::clamp(r, 0, 255));
            row[x * 4 + 1] = uint8_t(std::clamp(g, 0, 255));
            row[x * 4 + 2] = uint8_t(std::clamp(b, 0, 255));
            row[x * 4 + 3] = 255;
        }
    }
}

Error BinkMovie::decodeAudio(uint32_t frame, std::vector<int16_t>& out) {
    if (tracks_.empty())
        return Error::success();
    if (!state_->audio)
        return Error::fail("Bink's DCT audio isn't supported (only the RDFT kind)");
    if (frame != state_->audioFrame || frame >= index_.size())
        return Error::fail("Bink audio decoded out of order");
    ++state_->audioFrame;
    file_.clear();
    file_.seekg(index_[frame]);
    uint8_t size[4];
    if (!file_.read(reinterpret_cast<char*>(size), 4))
        return Error::fail("Bink audio cut short");
    const uint32_t n = le32(size);
    if (n < 4)
        return Error::success();
    audioPacket_.resize(n);
    if (!file_.read(reinterpret_cast<char*>(audioPacket_.data()), n))
        return Error::fail("Bink audio cut short");
    std::vector<float> pcm;
    if (!state_->audio->packet(audioPacket_.data(), audioPacket_.size(), pcm))
        return Error::fail("bad Bink audio in frame " + std::to_string(frame));
    for (const float v : pcm)
        out.push_back(int16_t(std::clamp(std::lround(v * 32768.0f), -32768L, 32767L)));
    return Error::success();
}

Error BinkMovie::decodeAudio(audio::Sound& out) {
    out = {};
    if (tracks_.empty())
        return Error::success();
    out.rate = audioRate();
    out.channels = audioChannels();
    out.format = kFormatBink;
    for (uint32_t f = state_->audioFrame; f < index_.size(); ++f)
        if (Error e = decodeAudio(f, out.samples))
            return e;
    return Error::success();
}

} // namespace reluva::video
