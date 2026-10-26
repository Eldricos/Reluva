// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/briefing.h"

#include "engines/newworlds/ai_data.h"
#include "engines/newworlds/briefing_data.h"

#include <algorithm>
#include <cctype>

namespace reluva::newworlds {

namespace {

using V = BriefingView;

// A clip's number and letter: "K12009A" -> (9, 'A'); the number is the
// last three digits before an optional letter.
std::pair<int, char> clipOrder(const std::string& stem) {
    size_t end = stem.size();
    char letter = 0;
    if (end && std::isalpha(static_cast<unsigned char>(stem[end - 1])))
        letter = char(std::toupper(static_cast<unsigned char>(stem[--end])));
    int n = 0;
    for (size_t i = end >= 3 ? end - 3 : 0; i < end; ++i)
        n = n * 10 + (std::isdigit(static_cast<unsigned char>(stem[i])) ? stem[i] - '0' : 0);
    return {n, letter};
}

// "~HEADING~text" -> "HEADING"; empty if the string isn't a page.
std::string headingOf(const std::string& text) {
    if (text.size() < 2 || text[0] != '~')
        return {};
    const size_t close = text.find('~', 1);
    return close == std::string::npos ? std::string() : text.substr(1, close - 1);
}

// Pages the generic rule shows: headed, with text, not a transmission or
// the pass and fail pages.
bool briefingPage(const std::string& text) {
    std::string heading = headingOf(text);
    if (heading.empty())
        return false;
    for (char& c : heading)
        c = char(std::toupper(static_cast<unsigned char>(c)));
    if (heading.find("TRANSMISSION") != std::string::npos || heading.find("RECEIVING") != std::string::npos ||
        heading.rfind("MISSION PASS", 0) == 0 || heading.rfind("MISSION FAIL", 0) == 0)
        return false;
    const std::string body = text.substr(heading.size() + 2);
    return body.find_first_not_of(" \t\r\n") != std::string::npos;
}

} // namespace

bool clipBefore(const std::string& a, const std::string& b) { return clipOrder(a) < clipOrder(b); }

bool hasRecordedBriefing(std::string_view shortName, char side) {
    return !briefingRows(shortName, char(std::toupper(static_cast<unsigned char>(side)))).empty();
}

std::vector<BriefingStep> briefingScript(std::string_view shortName, char side, const StringTable& table,
                                         const std::vector<std::string>& clips) {
    std::vector<BriefingStep> out;
    // The executable's table: each step's clip, page, view and object.
    if (const auto rows = briefingRows(shortName, char(std::toupper(static_cast<unsigned char>(side)))); !rows.empty()) {
        static constexpr V kViews[] = {V::Relief, V::Ground, V::Model, V::Galaxy, V::StarSystem, V::Emblem};
        for (const BriefingRow* r : rows) {
            BriefingStep step;
            step.clip = r->clip;
            step.page = r->page;
            step.view = kViews[std::clamp(r->view, 0, 5)];
            step.siteA = r->a, step.siteB = r->b;
            if (const CombatType* t = r->type ? combatType(r->type) : nullptr)
                step.model = t->model;
            step.minimum = r->minimum * kScriptSecond;
            step.scene = r->scene;
            step.scheme = r->scheme;
            out.push_back(std::move(step));
        }
        return out;
    }
    std::vector<std::string> sorted = clips;
    std::sort(sorted.begin(), sorted.end(), clipBefore);
    std::vector<uint32_t> pages;
    for (const StringTable::String& s : table.strings())
        if (briefingPage(s.text))
            pages.push_back(s.id);
    const size_t n = std::max(pages.size(), sorted.size());
    for (size_t i = 0; i < n; ++i) {
        BriefingStep step;
        step.clip = i < sorted.size() ? sorted[i] : std::string();
        step.page = i < pages.size() ? pages[i] : 0;
        step.view = i >= pages.size() ? V::Emblem : i == 0 ? V::Galaxy : i == 1 ? V::StarSystem : V::Ground;
        out.push_back(std::move(step));
    }
    return out;
}

} // namespace reluva::newworlds
