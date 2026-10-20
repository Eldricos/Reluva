// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/colony.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <utility>

namespace reluva::newworlds {

namespace {

struct SiteEntry {
    const char* mission;
    char side;
    int a, b;
    const char* source;
};

// From the play pages (and briefing.cpp's sites, the same places). A mine
// the colony reported exhausted early lies beside it; where two did, the
// middle of them.
constexpr SiteEntry kSites[] = {
    {"F1", 'F', 407, 209, "the start's Tricorder readout"},
    {"J2", 'F', 208, 424, "between two early mines"},
    {"F12", 'F', 212, 196, "a mine"},
    {"J9", 'F', 312, 392, "between two early mines"},
    {"C9", 'F', 260, 300, "a mine"},
    {"K1", 'K', 244, 92, "a mine"},
    {"K2", 'K', 300, 284, "between two early mines"},
    {"J1", 'K', 52, 148, "a mine"},
    {"J2", 'K', 92, 404, "a mine"},
    {"J3", 'K', 204, 172, "a mine"},
    {"K7", 'K', 160, 440, "between two early mines"},
    {"J4", 'K', 340, 180, "the first mine"},
    {"K12", 'K', 84, 252, "a mine"},
    {"K14", 'K', 100, 292, "a mine"},
    {"J9", 'K', 308, 368, "between two early mines"},
    {"C10", 'K', 292, 364, "the first mine"},
};

struct ModelEntry {
    const char* building;
    const char* fed;
    const char* kln;
    const char* rom;
};

// Models by the scripts' building names. Upper case: matched in recordings
// (play-f1.md and play-k1.md's tables); lower case: by the model's name
// alone (fpgs, kpgs: power generator station), a guess. Upgrades stand on
// the building they upgrade.
constexpr ModelEntry kModels[] = {
    {"colony-hub", "FCH", "KCH", "rch"},
    {"colony-hub-upgrade", "FCH_B", "KCH_B", "rch_b"},
    {"vehicle-yard", "FVPY_A", "KVY", "rvy"},
    {"construction-yard", "FCY", "KCY", "rcy_a"},
    {"resource-processor", "FRPS_A", "KRPS", ""},
    {"power-generator", "fpgs", "KPGS", "rps1"},
    {"advanced-generator", "fpgs2", "KPGS2", "rps2"},
    {"mining-station", "fms", "KMS", "rms"},
    {"accommodation", "ACCOM", "", ""},
    {"sickbay", "fsb", "KSB_A", ""},
    {"science-station", "fss_a", "KSS_A", ""},
    {"science-station-upgrade", "fss_b", "kss_b", ""},
    {"security-center", "FSC", "", ""},
    {"hydroponics-farm", "FHF", "", ""},
    {"livestock-farm", "", "KLF", ""},
    {"storage-facility", "", "KSF", ""},
    {"arsenal", "", "KAB", ""},
    {"transporter-pad", "landpad", "", ""},
    {"photon-turret", "fpdt", "", ""},
    {"disruptor-turret", "", "KSG", ""},
    {"static-disruptor", "", "kdws", ""},
};

constexpr std::pair<const char*, const char*> kUpgrades[] = {
    {"colony-hub-upgrade", "colony-hub"},
    {"science-station-upgrade", "science-station"},
};

// The ground counts as flat if it rises less than this across a block's
// middle, and near the hub's height if within this of it.
constexpr float kFlat = 1024, kNear = 2048;
// How many tiles from the site the hub may move, and how far around it
// counts as its room.
constexpr int kSearch = 6, kRoom = 3;

} // namespace

ColonySite colonySite(std::string_view shortName, char side) {
    for (const SiteEntry& s : kSites)
        if (shortName == s.mission && side == s.side)
            return {s.a, s.b, s.source};
    return {};
}

std::string buildingModel(std::string_view building, char side, bool* matched) {
    for (const ModelEntry& m : kModels) {
        if (building != m.building)
            continue;
        const std::string name = side == 'K' ? m.kln : side == 'R' ? m.rom : m.fed;
        if (name.empty())
            break;
        if (matched)
            *matched = std::isupper(static_cast<unsigned char>(name[0])) != 0;
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        const char* folder = side == 'K' ? "kln" : side == 'R' ? "rom" : "fed";
        return "geo/" + std::string(folder) + "/" + lower + ".geo";
    }
    if (matched)
        *matched = false;
    return {};
}

namespace {

constexpr int kTile = 4;            // map positions per tile
constexpr float kTileUnits = 8192;  // model units per tile
constexpr float kGap = 1024;        // kept clear around a building

int ceilDiv(float v, float d) { return std::max(1, int(std::ceil(v / d))); }

} // namespace

bool ColonyLayout::tileFree(int tx, int ty, bool mine) const {
    if (tx < 0 || ty < 0 || tx >= ground_.width * 2 || ty >= ground_.height * 2)
        return false;
    for (const Rect& r : taken_)
        if (tx >= r.x && tx < r.x + r.w && ty >= r.y && ty < r.y + r.h)
            return false;
    if ((ground_.deposit(tx / 2, ty / 2) != 0) != mine)
        return false;
    // Flat: its middle and four points near its corners within kFlat.
    const int a = ty * kTile + 2, b = tx * kTile + 2;
    float lo = ground_.heightAt(a, b), hi = lo;
    if (std::isnan(lo))
        return false;
    for (const auto& [da, db] : {std::pair{-2, -2}, std::pair{-2, 1}, std::pair{1, -2}, std::pair{1, 1}}) {
        const float h = ground_.heightAt(a + da, b + db);
        if (std::isnan(h))
            return false;
        lo = std::min(lo, h), hi = std::max(hi, h);
    }
    return hi - lo < kFlat && (!heightSet_ || std::fabs(lo - hubHeight_) < kNear);
}

bool ColonyLayout::fits(const Rect& r, bool mine) const {
    // A mining station needs its middle on the deposit, not all of it.
    if (mine)
        return tileFree(r.x + r.w / 2, r.y + r.h / 2, true) && [&] {
            for (int y = r.y; y < r.y + r.h; ++y)
                for (int x = r.x; x < r.x + r.w; ++x)
                    if (!tileFree(x, y, true) && !tileFree(x, y, false))
                        return false;
            return true;
        }();
    for (int y = r.y; y < r.y + r.h; ++y)
        for (int x = r.x; x < r.x + r.w; ++x)
            if (!tileFree(x, y, false))
                return false;
    return true;
}

ColonyLayout::Rect ColonyLayout::footprint(const std::string& model, int tx, int ty) const {
    const auto [w, h] = ground_.size ? ground_.size(model) : std::pair{kTileUnits, kTileUnits};
    return {tx, ty, ceilDiv(w + kGap, kTileUnits), ceilDiv(h + kGap, kTileUnits)};
}

void ColonyLayout::place(std::string_view name, const std::string& model, const Rect& r) {
    taken_.push_back(r);
    buildings_.push_back({std::string(name), model, r.y * kTile + r.h * kTile / 2, r.x * kTile + r.w * kTile / 2});
}

bool ColonyLayout::start(int a, int b) {
    buildings_.clear();
    taken_.clear();
    heightSet_ = false;
    const std::string model = buildingModel("colony-hub", side_);
    if (model.empty() || ground_.width <= 0)
        return false;
    // Of the places near the site, the one with the most free tiles
    // around it (within kRoom), less three for each tile it is from the
    // site.
    const int sx = b / kTile, sy = a / kTile;
    int best = -1;
    Rect bestRect{};
    for (int r = 0; r <= kSearch; ++r)
        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx) {
                if (std::max(std::abs(dx), std::abs(dy)) != r)
                    continue;
                const Rect rect = footprint(model, sx + dx, sy + dy);
                heightSet_ = false;
                if (!fits(rect, false))
                    continue;
                // Room at this place's height.
                hubHeight_ = ground_.heightAt(rect.y * kTile + 2, rect.x * kTile + 2);
                heightSet_ = true;
                int room = 0;
                for (int y = rect.y - kRoom; y < rect.y + rect.h + kRoom; ++y)
                    for (int x = rect.x - kRoom; x < rect.x + rect.w + kRoom; ++x)
                        room += tileFree(x, y, false);
                const int score = room + 3 * (kSearch - r);
                if (score > best)
                    best = score, bestRect = rect;
            }
    if (best < 0)
        return false;
    hubHeight_ = ground_.heightAt(bestRect.y * kTile + 2, bestRect.x * kTile + 2);
    heightSet_ = true;
    place("colony-hub", model, bestRect);
    return true;
}

bool ColonyLayout::add(std::string_view building) {
    if (buildings_.empty())
        return false;
    const std::string model = buildingModel(building, side_);
    if (model.empty())
        return false;
    for (const auto& [upgrade, base] : kUpgrades) {
        if (building != upgrade)
            continue;
        // On the building it upgrades, once.
        for (const ColonyBuilding& c : buildings_)
            if (c.name == building)
                return false;
        for (size_t i = 0; i < buildings_.size(); ++i)
            if (buildings_[i].name == base) {
                const int a = buildings_[i].a, b = buildings_[i].b;
                buildings_.push_back({std::string(building), model, a, b});
                return true;
            }
        return false;
    }
    const bool mine = building == "mining-station";
    // The free place nearest the hub's middle (by the building's middle).
    const Rect hub = taken_.front();
    const float hx = float(hub.x) + float(hub.w) / 2, hy = float(hub.y) + float(hub.h) / 2;
    const int range = 2 * std::max(ground_.width, ground_.height);
    for (int r = 1; r < range; ++r) {
        std::vector<Rect> ring;
        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx)
                if (std::max(std::abs(dx), std::abs(dy)) == r)
                    ring.push_back(footprint(model, int(std::floor(hx)) + dx, int(std::floor(hy)) + dy));
        auto distance = [&](const Rect& p) {
            const float x = float(p.x) + float(p.w) / 2 - hx, y = float(p.y) + float(p.h) / 2 - hy;
            return x * x + y * y;
        };
        std::stable_sort(ring.begin(), ring.end(), [&](const Rect& p, const Rect& q) { return distance(p) < distance(q); });
        for (const Rect& rect : ring)
            if (fits(rect, mine)) {
                place(building, model, rect);
                return true;
            }
    }
    return false;
}

} // namespace reluva::newworlds
