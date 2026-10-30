// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/types.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace reluva::fs {

namespace stdfs = std::filesystem;

std::string toLower(std::string_view s);

// Paths and UTF-8 strings (what SDL, the config file and the UI use). A
// narrow std::string given to a path, or path::string(), is the ANSI code
// page on Windows (and string() throws on what it can't convert), so every
// boundary between the two goes through these.
stdfs::path fromUtf8(std::string_view utf8);
std::string toUtf8(const stdfs::path& path);

// The files a command-line wildcard names, as a Unix shell expands it: `*`
// and `?` in the last part of `pattern` (UTF-8) matched against the names
// in its folder, ignoring ASCII case, sorted; `pattern` itself when nothing
// matches or it has no wildcard.
std::vector<std::string> expandWildcards(std::string_view pattern);

// The program's arguments as UTF-8, like every other string here. On
// Windows main() gets them in the ANSI code page, which can't hold every
// name; this takes them again from the UTF-16 command line and points
// argc/argv at UTF-8 copies (kept for this object's life), with wildcards
// expanded if asked (Windows' shells leave that to the program, so
// `reluva-tool geo-survey games\nw\*.cat` works as it does on Linux).
// Elsewhere it leaves them alone. Construct one first thing in main():
//     fs::Utf8Args utf8(argc, argv);
// (SDL's SDL_main does the same for the reluva program itself.)
class Utf8Args {
public:
    explicit Utf8Args(int& argc, char**& argv, bool wildcards = false);
    Utf8Args(const Utf8Args&) = delete;
    Utf8Args& operator=(const Utf8Args&) = delete;

#ifdef _WIN32
private:
    std::vector<std::string> strings_;
    std::vector<char*> pointers_;
#endif
};

// Replaces `file` with `contents` safely: written beside it first (file +
// ".tmp") and then moved over it, so a failed write leaves the old one.
Error replaceFile(const stdfs::path& file, std::string_view contents);

// Find a file under `dir` ignoring case, as old Windows/DOS games expect.
// `relative` may contain '/' separated components ("data/sound.bin").
std::optional<stdfs::path> findFile(const stdfs::path& dir, std::string_view relative);

// MD5 (hex) of the first `maxBytes` bytes of a file, or of the whole file
// when maxBytes is 0. Returns nullopt if the file can't be read.
std::optional<std::string> md5File(const stdfs::path& file, uint64_t maxBytes);

} // namespace reluva::fs
