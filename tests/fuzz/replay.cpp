// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// Every fuzz target over its corpus (corpus/<target>/, every file), without
// libFuzzer: the seeds and the inputs that once crashed, run in the normal
// test run. A crash fails it as it would the fuzzer.
//   reluva_fuzz_replay <corpus dir> [target]

#include "fuzz.h"

#include "common/fs.h"

#include <algorithm>
#include <cstdio>
#include <iterator>

namespace reluva::fuzz {
std::vector<Registered>& targets() {
    static std::vector<Registered> all;
    return all;
}
} // namespace reluva::fuzz

int main(int argc, char** argv) {
    reluva::fs::Utf8Args utf8(argc, argv);
    if (argc < 2) {
        std::fprintf(stderr, "usage: reluva_fuzz_replay <corpus dir> [target]\n");
        return 2;
    }
    const std::filesystem::path root = reluva::fs::fromUtf8(argv[1]);
    int failed = 0;
    for (const auto& [name, run] : reluva::fuzz::targets()) {
        if (argc > 2 && std::string(argv[2]) != name)
            continue;
        std::vector<std::filesystem::path> files;
        std::error_code ec;
        for (const auto& e : std::filesystem::directory_iterator(root / name, ec))
            if (e.is_regular_file())
                files.push_back(e.path());
        std::sort(files.begin(), files.end());
        if (files.empty()) {
            std::fprintf(stderr, "%s: no corpus in %s\n", name, reluva::fs::toUtf8(root / name).c_str());
            ++failed;
            continue;
        }
        for (const auto& f : files) {
            std::ifstream in(f, std::ios::binary);
            const std::vector<uint8_t> data{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
            run(data.data(), data.size());
        }
        std::printf("%s: %zu inputs\n", name, files.size());
    }
    return failed ? 1 : 0;
}
