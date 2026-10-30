// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "common/fs.h"
#include "common/md5.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <vector>

namespace reluva::fs {

std::string toLower(std::string_view s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

stdfs::path fromUtf8(std::string_view utf8) { return stdfs::path(std::u8string(utf8.begin(), utf8.end())); }

std::string toUtf8(const stdfs::path& path) {
    const std::u8string u = path.u8string();
    return std::string(u.begin(), u.end());
}

Error replaceFile(const stdfs::path& file, std::string_view contents) {
    stdfs::path temp = file;
    temp += ".tmp";
    std::error_code ec;
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out)
            return Error::fail("can't write " + toUtf8(temp));
        out.write(contents.data(), std::streamsize(contents.size()));
        if (!out.flush()) {
            out.close();
            stdfs::remove(temp, ec);
            return Error::fail("can't write " + toUtf8(temp));
        }
    }
    stdfs::rename(temp, file, ec); // replaces `file` (on Windows too)
    if (ec) {
        const std::string why = ec.message();
        stdfs::remove(temp, ec);
        return Error::fail("can't save to " + toUtf8(file) + ": " + why);
    }
    return Error::success();
}

static std::optional<stdfs::path> findEntry(const stdfs::path& dir, const std::string& lowerName) {
    std::error_code ec;
    for (stdfs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
        if (toLower(toUtf8(it->path().filename())) == lowerName)
            return it->path();
    }
    return std::nullopt;
}

std::optional<stdfs::path> findFile(const stdfs::path& dir, std::string_view relative) {
    stdfs::path current = dir;
    size_t start = 0;
    while (start <= relative.size()) {
        size_t slash = relative.find('/', start);
        if (slash == std::string_view::npos)
            slash = relative.size();
        std::string_view part = relative.substr(start, slash - start);
        if (!part.empty()) {
            auto found = findEntry(current, toLower(part));
            if (!found)
                return std::nullopt;
            current = *found;
        }
        start = slash + 1;
    }
    std::error_code ec;
    if (!stdfs::is_regular_file(current, ec))
        return std::nullopt;
    return current;
}

std::optional<std::string> md5File(const stdfs::path& file, uint64_t maxBytes) {
    std::ifstream in(file, std::ios::binary);
    if (!in)
        return std::nullopt;

    Md5 md5;
    std::vector<char> buf(64 * 1024);
    uint64_t remaining = maxBytes ? maxBytes : UINT64_MAX;
    while (remaining > 0 && in) {
        auto want = static_cast<std::streamsize>(std::min<uint64_t>(buf.size(), remaining));
        in.read(buf.data(), want);
        std::streamsize got = in.gcount();
        if (got <= 0)
            break;
        md5.update(buf.data(), static_cast<size_t>(got));
        remaining -= static_cast<uint64_t>(got);
    }
    if (in.bad())
        return std::nullopt;
    return md5.hexDigest();
}

} // namespace reluva::fs
