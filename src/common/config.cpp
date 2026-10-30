// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "common/config.h"

#include "common/fs.h"

#include <fstream>

namespace reluva {

static std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    size_t b = s.find_first_not_of(ws);
    if (b == std::string::npos)
        return {};
    size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

// A value as written: quoted and escaped when it wouldn't read back as it
// is (spaces at either end, line breaks or other control characters, a
// leading quote).
static std::string quote(const std::string& v) {
    bool plain = v.empty() || (v.front() != ' ' && v.front() != '\t' && v.front() != '"' && v.back() != ' ' &&
                               v.back() != '\t');
    for (unsigned char c : v)
        plain = plain && c >= 0x20 && c != 0x7f;
    if (plain)
        return v;
    std::string out = "\"";
    for (char c : v) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += c; break;
        }
    }
    return out + '"';
}

static std::string unquote(const std::string& v) {
    if (v.size() < 2 || v.front() != '"' || v.back() != '"')
        return v; // as written by hand, or before values were quoted
    std::string out;
    for (size_t i = 1; i + 1 < v.size(); ++i) {
        char c = v[i];
        if (c == '\\' && i + 2 < v.size()) {
            c = v[++i];
            c = c == 'n' ? '\n' : c == 'r' ? '\r' : c == 't' ? '\t' : c;
        }
        out += c;
    }
    return out;
}

Error Config::load(const std::filesystem::path& path) {
    path_ = path;
    sections_.clear();
    unreadable_ = false;

    std::error_code ec;
    if (!std::filesystem::exists(path, ec) && !ec)
        return Error::success(); // a missing file just means a fresh config
    std::ifstream in(path, std::ios::binary);
    if (ec || !in || !std::filesystem::is_regular_file(path, ec)) {
        unreadable_ = true;
        return Error::fail("Can't read " + fs::toUtf8(path) + ": settings won't be saved over it");
    }

    // Read as bytes, so a line may end in "\r\n" (as Windows editors write)
    // or "\n" (as Reluva writes, on every system): trim() drops the '\r'.
    std::string line, section;
    for (bool first = true; std::getline(in, line); first = false) {
        if (first && line.starts_with("\xEF\xBB\xBF"))
            line.erase(0, 3); // the UTF-8 mark Notepad has put at the start
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';')
            continue;
        if (line.front() == '[' && line.back() == ']') {
            section = trim(line.substr(1, line.size() - 2));
            sections_[section];
            continue;
        }
        size_t eq = line.find('=');
        if (eq == std::string::npos || section.empty())
            continue;
        sections_[section][trim(line.substr(0, eq))] = unquote(trim(line.substr(eq + 1)));
    }
    if (in.bad()) {
        unreadable_ = true;
        return Error::fail("Error reading " + fs::toUtf8(path) + ": settings won't be saved over it");
    }
    return Error::success();
}

Error Config::save() const {
    if (unreadable_)
        return Error::fail("Not saving settings over " + fs::toUtf8(path_) + ", which couldn't be read");
    std::string out;
    for (const auto& [name, values] : sections_) {
        out += '[' + name + "]\n";
        for (const auto& [k, v] : values)
            out += k + '=' + quote(v) + '\n';
        out += '\n';
    }
    if (Error e = fs::replaceFile(path_, out))
        return Error::fail("Can't save settings: " + e.message);
    return Error::success();
}

std::string Config::get(const std::string& section, const std::string& key,
                        const std::string& fallback) const {
    auto s = sections_.find(section);
    if (s == sections_.end())
        return fallback;
    auto v = s->second.find(key);
    return v == s->second.end() ? fallback : v->second;
}

void Config::set(const std::string& section, const std::string& key, const std::string& value) {
    sections_[section][key] = value;
}

bool Config::hasSection(const std::string& section) const { return sections_.count(section) != 0; }

void Config::removeSection(const std::string& section) { sections_.erase(section); }

std::vector<std::string> Config::targets() const {
    std::vector<std::string> out;
    for (const auto& [name, values] : sections_)
        if (name != kGlobalSection)
            out.push_back(name);
    return out;
}

} // namespace reluva
