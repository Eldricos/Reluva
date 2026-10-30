// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/types.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace reluva::fs {

namespace stdfs = std::filesystem;

std::string toLower(std::string_view s);

// Paths and UTF-8 strings (what SDL, the config file and the UI use). A
// narrow std::string given to a path, or path::string(), is the ANSI code
// page on Windows (and string() throws on what it can't convert), so every
// boundary between the two goes through these.
stdfs::path fromUtf8(std::string_view utf8);
std::string toUtf8(const stdfs::path& path);

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
