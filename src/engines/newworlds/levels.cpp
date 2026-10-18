// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/levels.h"

#include <cctype>

namespace reluva::newworlds {

namespace {

std::string_view trim(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.remove_prefix(1);
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.remove_suffix(1);
    return s;
}

bool sameName(std::string_view a, std::string_view b) {
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    return true;
}

// "F1,2,Power Corrupts" -> a level; false if the line isn't one.
bool parseLine(std::string_view line, Level& out) {
    const size_t a = line.find(','), b = a == std::string_view::npos ? a : line.find(',', a + 1);
    if (b == std::string_view::npos)
        return false;
    const std::string_view name = trim(line.substr(0, a)), players = trim(line.substr(a + 1, b - a - 1));
    if (name.empty() || players.empty())
        return false;
    int n = 0;
    for (char c : players) {
        if (!std::isdigit(static_cast<unsigned char>(c)))
            return false;
        n = n * 10 + (c - '0');
    }
    out = Level{std::string(name), n, std::string(trim(line.substr(b + 1))), false};
    return true;
}

} // namespace

Error parseLevelList(std::string_view text, std::vector<Level>& out) {
    out.clear();
    size_t lineNo = 0;
    while (!text.empty()) {
        ++lineNo;
        const size_t end = text.find('\n');
        std::string_view line = trim(text.substr(0, end));
        text = end == std::string_view::npos ? std::string_view() : text.substr(end + 1);
        if (line.empty())
            continue;
        Level level;
        if (line.substr(0, 2) == "//") {
            if (parseLine(trim(line.substr(2)), level)) {
                level.commentedOut = true;
                out.push_back(std::move(level));
            }
            continue;
        }
        if (!parseLine(line, level))
            return Error::fail("level.dat line " + std::to_string(lineNo) + " isn't \"name,players,title\"");
        out.push_back(std::move(level));
    }
    return Error::success();
}

const Level* levelForMap(const std::vector<Level>& levels, std::string_view mapName) {
    std::string_view stem = mapName;
    if (const size_t slash = stem.find_last_of("/\\"); slash != std::string_view::npos)
        stem.remove_prefix(slash + 1);
    if (const size_t dot = stem.rfind('.'); dot != std::string_view::npos)
        stem = stem.substr(0, dot);
    for (bool commented : {false, true})
        for (const Level& l : levels)
            if (l.commentedOut == commented && sameName(l.shortName, stem))
                return &l;
    return nullptr;
}

} // namespace reluva::newworlds
