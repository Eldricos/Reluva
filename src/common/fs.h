#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace kestrel::fs {

namespace stdfs = std::filesystem;

std::string toLower(std::string_view s);

// Find a file under `dir` ignoring case, as old Windows/DOS games expect.
// `relative` may contain '/' separated components ("data/sound.bin").
std::optional<stdfs::path> findFile(const stdfs::path& dir, std::string_view relative);

// MD5 (hex) of the first `maxBytes` bytes of a file, or of the whole file
// when maxBytes is 0. Returns nullopt if the file can't be read.
std::optional<std::string> md5File(const stdfs::path& file, uint64_t maxBytes);

} // namespace kestrel::fs
