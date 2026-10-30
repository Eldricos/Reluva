// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/types.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace reluva {

// INI-style settings file. The [reluva] section holds global settings; every
// other section is a game target (one installed game the user added).
// Values are UTF-8; one with spaces at either end, a line break or a
// leading quote is written quoted ("...", with \\ \" \n \r \t escapes)
// and read back as it was. Unquoted values are read as they stand.
class Config {
public:
    static constexpr const char* kGlobalSection = "reluva";

    // A missing file is a fresh config. One that exists but can't be read
    // is an error, and save() then refuses to write over it.
    Error load(const std::filesystem::path& path);
    // Replaces the file whole (written beside it, then moved over it).
    Error save() const;

    std::string get(const std::string& section, const std::string& key,
                    const std::string& fallback = {}) const;
    void set(const std::string& section, const std::string& key, const std::string& value);

    bool hasSection(const std::string& section) const;
    void removeSection(const std::string& section);
    std::vector<std::string> targets() const;

private:
    std::filesystem::path path_;
    bool unreadable_ = false; // load() found a file it couldn't read: keep it
    std::map<std::string, std::map<std::string, std::string>> sections_;
};

} // namespace reluva
