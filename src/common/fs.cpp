// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "common/fs.h"
#include "common/md5.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX // (MinGW's libstdc++ defines it already)
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#endif

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

// `name` against `pattern`: '*' any run, '?' any one character (a whole
// UTF-8 sequence), ASCII letters in either case.
static bool wildcardMatch(std::string_view pattern, std::string_view name) {
    const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? char(c - 'A' + 'a') : c; };
    const auto charEnd = [&](size_t i) { // past the UTF-8 character at name[i]
        ++i;
        while (i < name.size() && (static_cast<unsigned char>(name[i]) & 0xC0) == 0x80)
            ++i;
        return i;
    };
    size_t p = 0, n = 0, starP = std::string_view::npos, starN = 0;
    while (n < name.size()) {
        if (p < pattern.size() && pattern[p] == '*') {
            starP = p++, starN = n;
        } else if (p < pattern.size() && pattern[p] == '?') {
            ++p, n = charEnd(n);
        } else if (p < pattern.size() && lower(pattern[p]) == lower(name[n])) {
            ++p, ++n;
        } else if (starP != std::string_view::npos) {
            p = starP + 1, n = starN = charEnd(starN);
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*')
        ++p;
    return p == pattern.size();
}

std::vector<std::string> expandWildcards(std::string_view pattern) {
#ifdef _WIN32
    const size_t slash = pattern.find_last_of("/\\");
#else
    const size_t slash = pattern.rfind('/');
#endif
    const size_t start = slash == std::string_view::npos ? 0 : slash + 1;
    const std::string_view folder = pattern.substr(0, start), last = pattern.substr(start);
    if (last.find_first_of("*?") == std::string_view::npos || folder.find_first_of("*?") != std::string_view::npos)
        return {std::string(pattern)};
    std::vector<std::string> out;
    std::error_code ec;
    for (stdfs::directory_iterator it(folder.empty() ? stdfs::path(".") : fromUtf8(folder), ec), end;
         !ec && it != end; it.increment(ec)) {
        const std::string name = toUtf8(it->path().filename());
        if (name[0] != '.' || last[0] == '.') // (as shells do: hidden ones only asked for)
            if (wildcardMatch(last, name))
                out.push_back(std::string(folder) + name);
    }
    if (out.empty())
        return {std::string(pattern)};
    std::sort(out.begin(), out.end());
    return out;
}

Utf8Args::Utf8Args([[maybe_unused]] int& argc, [[maybe_unused]] char**& argv, [[maybe_unused]] bool wildcards) {
#ifdef _WIN32
    // What the tools print is UTF-8 too; a console would show it in its
    // own code page otherwise.
    SetConsoleOutputCP(CP_UTF8);
    int n = 0;
    LPWSTR* wide = CommandLineToArgvW(GetCommandLineW(), &n);
    if (!wide)
        return; // (left as they were)
    for (int i = 0; i < n; ++i) {
        const int len = WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, nullptr, 0, nullptr, nullptr);
        std::string a(size_t(std::max(len, 1)), '\0');
        if (len > 0)
            WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, a.data(), len, nullptr, nullptr);
        a.pop_back(); // the terminator
        if (wildcards && i > 0)
            for (std::string& e : expandWildcards(a))
                strings_.push_back(std::move(e));
        else
            strings_.push_back(std::move(a));
    }
    LocalFree(wide);
    for (std::string& a : strings_)
        pointers_.push_back(a.data());
    pointers_.push_back(nullptr);
    argc = int(strings_.size());
    argv = pointers_.data();
#endif
}

// Moves `from` over `to`, replacing it in one step: either `to` is the new
// file or it's still the old one.
static void moveOver(const stdfs::path& from, const stdfs::path& to, std::error_code& ec) {
#ifdef _WIN32
    // Said outright rather than left to std::filesystem::rename, whose
    // Windows behaviour has differed between standard libraries. Another
    // program (a virus scanner, an indexer, an editor) may hold the file
    // open for a moment, so a refusal is tried again a few times.
    for (int attempt = 0;; ++attempt) {
        if (MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            ec.clear();
            return;
        }
        const DWORD why = GetLastError();
        if (attempt >= 4 || (why != ERROR_ACCESS_DENIED && why != ERROR_SHARING_VIOLATION)) {
            ec = std::error_code(int(why), std::system_category());
            return;
        }
        Sleep(50);
    }
#else
    stdfs::rename(from, to, ec); // rename(2): atomic, replaces `to`
#endif
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
    moveOver(temp, file, ec);
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
