// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

// The fuzz targets (tests/fuzz/CMakeLists.txt): each file here is one
// target, written once for both builds. With RELUVA_FUZZ each
// is its own libFuzzer program (LLVMFuzzerTestOneInput); in the test run
// they're all in reluva_fuzz_replay, which runs each over its seed corpus
// (corpus/<name>/).

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>

namespace reluva::fuzz {

using Target = int (*)(const uint8_t* data, size_t size);
struct Registered {
    const char* name;
    Target run;
};
std::vector<Registered>& targets();
inline bool add(const char* name, Target run) {
    targets().push_back({name, run});
    return true;
}

inline std::vector<uint8_t> bytes(const uint8_t* data, size_t size) { return {data, data + size}; }

// For readers that take a file: the input written to one of this process's
// own (each target its own name, so replay's targets don't share one).
inline std::filesystem::path asFile(const char* target, const uint8_t* data, size_t size) {
    static const std::string id = std::to_string(std::random_device{}());
    const auto path = std::filesystem::temp_directory_path() / ("reluva_fuzz_" + id + "_" + target);
    std::ofstream(path, std::ios::binary | std::ios::trunc).write(reinterpret_cast<const char*>(data), std::streamsize(size));
    return path;
}

} // namespace reluva::fuzz

#ifdef RELUVA_FUZZ_REPLAY
#define RELUVA_FUZZ_TARGET(name)                                                                \
    static int fuzz_##name(const uint8_t* data, size_t size);                                   \
    [[maybe_unused]] static const bool registered_##name = reluva::fuzz::add(#name, fuzz_##name); \
    static int fuzz_##name
#else
#define RELUVA_FUZZ_TARGET(name) extern "C" int LLVMFuzzerTestOneInput
#endif
