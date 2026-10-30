// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/world.h"

#include <algorithm>
#include <cmath>
#include <queue>

namespace reluva::newworlds {

namespace {

// A ground scout: its pace of 0.3 path steps a tick, a block in 2.7 s.
constexpr float kScoutSpeed = 0.3f * kPaceSpeed;
// Units sent to one point: the game sends them all to it, and they pack
// round it by the 1024-unit box each keeps (exe.md#orders).
constexpr float kSpread = 1024;
// Turrets (exe.md#sides-and-combat): they turn speeding up and then
// slowing down onto their target, at 128 of the game's 65536ths of a turn
// a tick each tick, and fire only when on it. They look for a target
// every 8-11 ticks. A shell's reach on flat ground (its speed and gravity).
constexpr float kTurretPush = 128.0f / 65536 * 2 * 3.14159265f;
constexpr int kRescan = 8;
constexpr float kShellReach = 36850;
// A defensive turret's power a fire (exe.md#sides-and-combat), and which
// types pay it: the empires' defensive weapons and artillery (and their
// copies for slots 3-5).
constexpr float kShotPower = 300;
bool paysForShots(int type) {
    for (int t : {54, 80, 81, 82, 83, 84, 276, 277, 281, 282, 287, 288})
        if (t == type)
            return true;
    return false;
}
// Projectiles: a disruptor bolt's speed and a shell's (units a tick), the
// shells' gravity and longest flight; a phaser pulse runs a beam's length
// a tick (its model's; Reluva's figure here, the model not measured).
// A bolt is aimed as if it flew 2700 a tick, but flies twice that (the
// game doubles its launch once); a beam pulse moves a beam's length a
// tick (Reluva's 2500: the models' beam lengths aren't read yet).
constexpr float kBoltSpeed = 2700, kBoltFlight = 5400, kShellSpeed = 350, kGravity = 3, kBeamLength = 2500;
// What a shot passes through and what never blocks a line (exe.md#sides-
// and-combat): bees, unarmed APCs, effects and a few others by class.
constexpr int kNotHit = 0x1dea0;
// The classes that count as units or turrets (an enemy's never blocks).
constexpr int kUnitClasses = 0x4560110;
constexpr int kShellLife = 1000;
// An object's size to shots where its model's isn't known (World::setRadius
// gives the game's: its model's bounds radius).
constexpr float kBuildingRadius = 6000, kVehicleRadius = 1000;
// Cloaking (exe.md#cloaking): ticks seen before it cloaks, and a fade's.
constexpr int kCloakAfter = 300, kCloakFade = 63;
// Protecting (exe.md#orders): how often it looks for an attack on the
// side (an object counts as attacked while its attacker is live).
constexpr int kProtectEvery = 8;

// Whether a unit keeps driving while it fires: one moving on the
// player's move or attack-en-route order (a cloaking unit not on a move).
// Its class bits (exe.md#group-counts), from its type.
int classOf(const Unit& u) {
    const CombatType* t = combatType(u.type);
    return t ? t->mask : 0;
}

bool firesOnTheMove(const Unit& u) {
    return !u.ai && u.moving() &&
           ((u.order == UnitOrder::Move && !u.cloaks) || u.order == UnitOrder::EnRoute ||
            u.order == UnitOrder::Protect || u.order == UnitOrder::Rally);
}

} // namespace

UnitKind groundScout(char side) {
    if (side == 'K')
        return {"GROUND SCOUT", "geo/kln/gscout.geo", kScoutSpeed};
    if (side == 'R')
        return {"GROUND SCOUT", "geo/rom/rfgs.geo", kScoutSpeed};
    return {"GROUND SCOUT", "geo/fed/ffs.geo", kScoutSpeed};
}

World::World(Terrain terrain) : terrain_(std::move(terrain)) {}

void World::setTerrain(Terrain terrain) {
    terrain_ = std::move(terrain);
    pathInputsChanged();
}

void World::pathInputsChanged() {
    ++pathGeneration_;
    steps_.clear();
    centres_.clear();
    regions_.clear();
    regionTiles_.clear();
}

bool World::passable(int tx, int ty) const {
    if (tx < 0 || ty < 0 || tx >= terrain_.width || ty >= terrain_.height)
        return false;
    return terrain_.moveClass[size_t(ty) * size_t(terrain_.width) + size_t(tx)] != 0;
}

namespace {
// A step's bit in a tile's steps (World::steps_): the eight ways, row by
// row from the south-west.
constexpr std::array<std::pair<int, int>, 8> kStepWays{{{-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}}};
int stepBit(int dx, int dy) {
    const int k = (dy + 1) * 3 + (dx + 1);
    return k < 4 ? k : k - 1;
}
} // namespace

bool World::canStep(int tx, int ty, int dx, int dy) const {
    if (tx < 0 || ty < 0 || tx >= terrain_.width || ty >= terrain_.height || dx < -1 || dx > 1 || dy < -1 ||
        dy > 1 || (dx == 0 && dy == 0))
        return stepUncached(tx, ty, dx, dy);
    return (stepsAt(size_t(ty) * size_t(terrain_.width) + size_t(tx)) >> stepBit(dx, dy)) & 1;
}

uint8_t World::stepsAt(size_t tile) const {
    const int w = terrain_.width;
    if (steps_.empty()) {
        steps_.assign(size_t(w) * size_t(terrain_.height), 0);
        centres_.assign(steps_.size(), 0);
    }
    if (steps_[tile] & 0x100)
        return uint8_t(steps_[tile] & 0xff);
    // As stepUncached, each tile's middle's height asked once (bit 9: known).
    auto centre = [&](size_t i, float x, float y) {
        if (!(steps_[i] & 0x200))
            centres_[i] = terrain_.heightAt(x, y), steps_[i] = uint16_t(steps_[i] | 0x200);
        return centres_[i];
    };
    const int tx = int(tile % size_t(w)), ty = int(tile / size_t(w));
    uint16_t m = 0x100;
    for (int k = 0; k < 8; ++k) {
        const auto [dx, dy] = kStepWays[size_t(k)];
        if (!passable(tx + dx, ty + dy))
            continue;
        if (dx != 0 && dy != 0 && (!passable(tx + dx, ty) || !passable(tx, ty + dy)))
            continue;
        bool ok = true;
        if (terrain_.heightAt) {
            const float x0 = (float(tx) + 0.5f) * kTile, y0 = (float(ty) + 0.5f) * kTile;
            float last = centre(tile, x0, y0);
            for (int i = 1; i <= 4 && ok; ++i) {
                const float x = x0 + float(dx) * kTile * float(i) / 4, y = y0 + float(dy) * kTile * float(i) / 4;
                const float nx = (float(tx + dx) + 0.5f) * kTile, ny = (float(ty + dy) + 0.5f) * kTile;
                const float h = i == 4 && x == nx && y == ny ? centre(size_t(ty + dy) * size_t(w) + size_t(tx + dx), x, y)
                                                             : terrain_.heightAt(x, y);
                ok = !(std::fabs(h - last) > kMaxStep / 2); // (as stepUncached has it)
                last = h;
            }
        }
        if (ok)
            m = uint16_t(m | (1 << k));
    }
    steps_[tile] = uint16_t(steps_[tile] | m);
    return uint8_t(m & 0xff);
}

bool World::stepUncached(int tx, int ty, int dx, int dy) const {
    if (!passable(tx + dx, ty + dy))
        return false;
    if (dx != 0 && dy != 0 && (!passable(tx + dx, ty) || !passable(tx, ty + dy)))
        return false;
    if (!terrain_.heightAt)
        return true;
    // The ground along the way, from one tile's middle to the other's, a
    // quarter tile at a time.
    const float x0 = (float(tx) + 0.5f) * kTile, y0 = (float(ty) + 0.5f) * kTile;
    float last = terrain_.heightAt(x0, y0);
    for (int i = 1; i <= 4; ++i) {
        const float h = terrain_.heightAt(x0 + float(dx) * kTile * float(i) / 4, y0 + float(dy) * kTile * float(i) / 4);
        if (std::fabs(h - last) > kMaxStep / 2)
            return false;
        last = h;
    }
    return true;
}

std::vector<std::pair<float, float>> World::findPath(float x0, float y0, float x1, float y1) const {
    const int w = terrain_.width, h = terrain_.height;
    if (w <= 0 || h <= 0)
        return {};
    auto clampT = [](int v, int n) { return std::clamp(v, 0, n - 1); };
    const int sx = clampT(tileOf(x0), w), sy = clampT(tileOf(y0), h);
    int gx = clampT(tileOf(x1), w), gy = clampT(tileOf(y1), h);
    bool moved = false;
    if (!passable(gx, gy)) {
        // The nearest open tile, ring by ring.
        bool found = false;
        for (int r = 1; r < std::max(w, h) && !found; ++r)
            for (int dy = -r; dy <= r && !found; ++dy)
                for (int dx = -r; dx <= r && !found; ++dx)
                    if (std::max(std::abs(dx), std::abs(dy)) == r && passable(gx + dx, gy + dy))
                        gx += dx, gy += dy, found = true;
        if (!found)
            return {};
        moved = true;
    }
    const auto index = [w](int x, int y) { return size_t(y) * size_t(w) + size_t(x); };
    auto guess = [&](int x, int y) {
        const float dx = float(std::abs(x - gx)), dy = float(std::abs(y - gy));
        return std::max(dx, dy) + 0.4142f * std::min(dx, dy);
    };
    const size_t start = index(sx, sy);
    if (!regions_.empty() && regions_[start] >= 0 && regions_[index(gx, gy)] != regions_[start]) {
        // A goal out of the start's region: the search below would go over
        // all the region and stop at its nearest tile to the goal (the
        // first, as it counts them, of the nearest). If that's the start,
        // there's no way to take.
        float best = 1e30f;
        size_t nearest = start;
        for (const int i : regionTiles_[size_t(regions_[start])])
            if (guess(i % w, i / w) < best)
                best = guess(i % w, i / w), nearest = size_t(i);
        if (nearest == start)
            return {};
    }
    std::vector<float> cost(size_t(w) * size_t(h), 1e30f);
    std::vector<int> from(size_t(w) * size_t(h), -1);
    using Node = std::pair<float, int>; // estimate, tile index
    std::priority_queue<Node, std::vector<Node>, std::greater<>> open;
    cost[index(sx, sy)] = 0;
    open.push({guess(sx, sy), int(index(sx, sy))});
    bool reached = sx == gx && sy == gy;
    while (!open.empty() && !reached) {
        const auto [est, i] = open.top();
        open.pop();
        const int x = i % w, y = i / w;
        if (est - guess(x, y) > cost[size_t(i)] + 1e-3f)
            continue; // a stale entry
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                if ((dx == 0 && dy == 0) || !canStep(x, y, dx, dy))
                    continue;
                const size_t j = index(x + dx, y + dy);
                const float c = cost[size_t(i)] + (dx != 0 && dy != 0 ? 1.4142f : 1.0f);
                if (c >= cost[j])
                    continue;
                cost[j] = c;
                from[j] = i;
                if (x + dx == gx && y + dy == gy)
                    reached = true;
                open.push({c + guess(x + dx, y + dy), int(j)});
            }
    }
    if (!reached && passable(sx, sy) && (regions_.empty() || regions_[start] < 0)) {
        // All the start reaches searched: a region, if each step in it
        // goes both ways (then a search from any tile of it reaches just
        // it).
        std::vector<int> tiles;
        bool both = true;
        for (size_t i = 0; i < cost.size() && both; ++i) {
            if (cost[i] >= 1e30f)
                continue;
            tiles.push_back(int(i));
            const uint8_t m = stepsAt(i);
            for (int k = 0; k < 8 && both; ++k)
                if ((m >> k) & 1) {
                    const auto [dx, dy] = kStepWays[size_t(k)];
                    both = canStep(int(i) % w + dx, int(i) / w + dy, -dx, -dy);
                }
        }
        if (both) {
            if (regions_.empty())
                regions_.assign(cost.size(), -1);
            for (const int i : tiles)
                regions_[size_t(i)] = int(regionTiles_.size());
            regionTiles_.push_back(std::move(tiles));
        }
    }
    if (!reached) {
        // Out of reach (in a pit, say): as near as it can get instead.
        float best = 1e30f;
        int nearest = -1;
        for (size_t i = 0; i < cost.size(); ++i)
            if (cost[i] < 1e30f && guess(int(i) % w, int(i) / w) < best)
                best = guess(int(i) % w, int(i) / w), nearest = int(i);
        if (nearest < 0 || nearest == int(index(sx, sy)))
            return {};
        gx = nearest % w, gy = nearest / w;
        moved = true;
    }
    std::vector<std::pair<float, float>> path;
    for (int i = int(index(gx, gy)); i != int(index(sx, sy)); i = from[size_t(i)])
        path.push_back({(float(i % w) + 0.5f) * kTile, (float(i / w) + 0.5f) * kTile});
    std::reverse(path.begin(), path.end());
    // End on the point itself where it was open, not its tile's middle.
    if (!moved && !path.empty())
        path.back() = {x1, y1};
    else if (!moved)
        path.push_back({x1, y1});
    // Drop the waypoints a straight line could skip (the tiles it passes
    // all open), so units don't zigzag tile by tile.
    std::vector<std::pair<float, float>> out;
    auto clear = [&](std::pair<float, float> a, std::pair<float, float> b) {
        const float d = std::hypot(b.first - a.first, b.second - a.second);
        const int n = std::max(1, int(d / (kTile / 4)));
        int px = tileOf(a.first), py = tileOf(a.second);
        for (int k = 1; k <= n; ++k) {
            const float t = float(k) / float(n);
            const int qx = tileOf(a.first + (b.first - a.first) * t), qy = tileOf(a.second + (b.second - a.second) * t);
            if (qx != px || qy != py) {
                if (std::abs(qx - px) > 1 || std::abs(qy - py) > 1 || !canStep(px, py, qx - px, qy - py))
                    return false;
                px = qx, py = qy;
            }
        }
        return true;
    };
    std::pair<float, float> at{x0, y0};
    size_t k = 0;
    while (k < path.size()) {
        size_t far = k;
        while (far + 1 < path.size() && clear(at, path[far + 1]))
            ++far;
        out.push_back(path[far]);
        at = path[far];
        k = far + 1;
    }
    return out;
}

std::pair<float, float> World::openSpot(float x, float y) const {
    const int tx = tileOf(x), ty = tileOf(y);
    // Its ways out (0 if a vehicle already stands there).
    auto ways = [&](int cx, int cy) {
        int n = 0;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
                n += (dx || dy) && canStep(cx, cy, dx, dy);
        for (const Unit& u : units_)
            if (!u.building && tileOf(u.x) == cx && tileOf(u.y) == cy && (cx != tx || cy != ty))
                return 0;
        return passable(cx, cy) ? n : 0;
    };
    if (ways(tx, ty) >= 4)
        return {x, y};
    for (int r = 1; r < 8; ++r)
        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx)
                if (std::max(std::abs(dx), std::abs(dy)) == r && ways(tx + dx, ty + dy) >= 4)
                    return {(float(tx + dx) + 0.5f) * kTile, (float(ty + dy) + 0.5f) * kTile};
    return {x, y};
}

int World::addUnit(char side, const UnitKind& kind, float x, float y, float heading) {
    Unit u;
    u.id = nextId_++;
    u.side = side;
    u.kind = kind;
    u.x = u.lastX = x, u.y = u.lastY = y, u.heading = u.lastHeading = heading;
    units_.push_back(std::move(u));
    const int id = units_.back().id;
    if (id >= 0) {
        if (size_t(id) >= index_.size())
            index_.resize(size_t(id) + 1, -1);
        index_[size_t(id)] = int(units_.size()) - 1;
    }
    return id;
}

void World::reindex() {
    index_.assign(size_t(std::max(nextId_, 0)), -1);
    for (size_t i = 0; i < units_.size(); ++i) {
        const int id = units_[i].id;
        if (id < 0)
            continue;
        if (size_t(id) >= index_.size())
            index_.resize(size_t(id) + 1, -1);
        if (index_[size_t(id)] < 0) // (the first, as a search would find)
            index_[size_t(id)] = int(i);
    }
}

Unit* World::unit(int id) {
    return const_cast<Unit*>(std::as_const(*this).unit(id));
}

const Unit* World::unit(int id) const {
    if (id >= 0 && size_t(id) < index_.size()) {
        const int i = index_[size_t(id)];
        if (i >= 0 && size_t(i) < units_.size() && units_[size_t(i)].id == id)
            return &units_[size_t(i)];
    }
    // (Not indexed: none, unless its id was changed from outside.)
    for (const Unit& u : units_)
        if (u.id == id)
            return &u;
    return nullptr;
}

void World::remove(int id) {
    units_.erase(std::remove_if(units_.begin(), units_.end(), [&](const Unit& u) { return u.id == id; }), units_.end());
    reindex();
    for (Unit& u : units_)
        if (u.target == id)
            u.target = 0, u.ordered = false, u.firing = false;
}

int World::stance(int from, int to) const {
    if (from == to)
        return 1;
    if (from < 0 || to < 0 || from > 6 || to > 6)
        return 2;
    const int s = stances_[size_t(from)][size_t(to)];
    return s ? s : 2;
}

void World::setStance(int from, int to, int posture) {
    if (from >= 0 && to >= 0 && from <= 6 && to <= 6)
        stances_[size_t(from)][size_t(to)] = posture;
}

void World::attack(const std::vector<int>& ids, int target) {
    const Unit* t = unit(target);
    if (!t)
        return;
    for (int id : ids)
        if (Unit* u = unit(id); u && u->id != target && u->weapon.kind != WeaponKind::None) {
            u->target = target;
            u->ordered = true;
            u->order = UnitOrder::None;
            u->path.clear();
        }
}

void World::assault(const std::vector<int>& ids, int target) {
    attack(ids, target);
    for (int id : ids)
        if (Unit* u = unit(id); u && u->target == target)
            u->order = UnitOrder::Assault;
}

void World::protect(const std::vector<int>& ids, const std::vector<int>& buildings) {
    for (int id : ids)
        if (Unit* u = unit(id); u && !u->building) {
            u->order = buildings.empty() ? UnitOrder::Protect : UnitOrder::ProtectArea;
            u->protects = buildings;
            u->chasing = 0;
            // The spot: where it's going (protecting a location), or where
            // it stands (protecting an area).
            const auto [hx, hy] = u->path.empty() || !buildings.empty() ? std::pair{u->x, u->y} : u->path.back();
            u->holdX = hx, u->holdY = hy;
        }
}

void World::rally(int id, float x, float y) {
    moveTo(id, x, y);
    if (Unit* u = unit(id))
        u->order = UnitOrder::Rally;
}

void World::setPath(int id, std::vector<std::pair<float, float>> path) {
    Unit* u = unit(id);
    if (!u)
        return;
    u->path = std::move(path);
    if (!u->path.empty()) {
        WorldEvent e;
        e.kind = WorldEvent::Moved;
        e.from = u->id, e.fromSlot = u->slot;
        e.x0 = u->x, e.y0 = u->y;
        e.what = *u;
        events_.push_back(std::move(e));
    }
}

void World::moveTo(int id, float x, float y) {
    if (Unit* u = unit(id); u && !u->building) {
        setPath(id, u->flies ? std::vector<std::pair<float, float>>{{x, y}} : findPath(u->x, u->y, x, y));
        u->ordered = false;
        u->target = 0;
        u->firing = false;
    }
}

void World::select(const std::vector<int>& ids) {
    for (Unit& u : units_)
        u.selected = std::find(ids.begin(), ids.end(), u.id) != ids.end();
}

void World::clearSelection() {
    for (Unit& u : units_)
        u.selected = false;
}

std::vector<int> World::selection() const {
    std::vector<int> out;
    for (const Unit& u : units_)
        if (u.selected)
            out.push_back(u.id);
    return out;
}

void World::moveSelected(float x, float y, UnitOrder order) {
    std::vector<Unit*> sel;
    for (Unit& u : units_)
        if (u.selected)
            sel.push_back(&u);
    // Spots on a small grid round the point, nearest first.
    const int n = int(sel.size());
    const int cols = std::max(1, int(std::ceil(std::sqrt(float(n)))));
    for (int i = 0; i < n; ++i) {
        const float ox = (float(i % cols) - float(cols - 1) / 2) * kSpread;
        const float oy = (float(i / cols) - float((n - 1) / cols) / 2) * kSpread;
        moveTo(sel[size_t(i)]->id, x + ox, y + oy);
        sel[size_t(i)]->order = order;
    }
}

void World::setGroup(int n) {
    if (n < 1 || n > 9)
        return;
    groups_[size_t(n)] = selection();
}

bool World::selectGroup(int n) {
    if (n < 1 || n > 9)
        return false;
    auto& g = groups_[size_t(n)];
    g.erase(std::remove_if(g.begin(), g.end(), [&](int id) { return unit(id) == nullptr; }), g.end());
    if (g.empty())
        return false;
    select(g);
    return true;
}

bool World::groupCentre(int n, float& x, float& y) const {
    float sx = 0, sy = 0;
    int count = 0;
    for (const Unit& u : units_) {
        const bool in = n == 0 ? u.selected
                               : (n >= 1 && n <= 9 &&
                                  std::find(groups_[size_t(n)].begin(), groups_[size_t(n)].end(), u.id) !=
                                      groups_[size_t(n)].end());
        if (in)
            sx += u.x, sy += u.y, ++count;
    }
    if (count == 0)
        return false;
    x = sx / float(count), y = sy / float(count);
    return true;
}

bool World::advance(double seconds) {
    // Whole ticks only, each a combat tick and then a tick's movement, so
    // shots lead a target by one tick's motion and time cut finer or
    // coarser plays the same (exe.md#the-simulation-clock).
    bool any = false;
    tickClock_ += seconds * 20;
    while (tickClock_ >= 1 - 1e-9) {
        tickClock_ -= 1;
        combatTick();
        any = true;
        for (Unit& u : units_) {
            if ((u.firing && !firesOnTheMove(u)) || u.building)
                continue;
            float left = u.kind.speed / 20;
            while (left > 0 && !u.path.empty()) {
                const auto [tx, ty] = u.path.front();
                const float dx = tx - u.x, dy = ty - u.y, d = std::hypot(dx, dy);
                if (d > 1)
                    u.heading = std::atan2(dy, dx);
                if (d <= left) {
                    u.x = tx, u.y = ty;
                    left -= d;
                    u.path.erase(u.path.begin());
                } else {
                    u.x += dx / d * left, u.y += dy / d * left;
                    left = 0;
                }
            }
        }
    }
    return any;
}

std::vector<WorldEvent> World::takeEvents() {
    std::vector<WorldEvent> out;
    out.swap(events_);
    return out;
}

// A tick of combat (exe.md#sides-and-combat): each armed object keeps or finds a
// target and fires at it when it's in range and loaded. A burst weapon
// fires its magazine a few ticks apart, then reloads; others fire one
// shot and reload. What's out of hit points is destroyed.
void World::combatTick() {
    ++tick_;
    for (Unit& u : units_) {
        u.vx = u.x - u.lastX, u.vy = u.y - u.lastY;
        u.lastX = u.x, u.lastY = u.y;
        u.lastHeading = u.heading, u.lastTurretYaw = u.turretYaw;
    }
    // A line of fire, kept 8-11 ticks (artillery lobs over anything).
    auto clear = [&](Unit& u, const Unit& t) {
        if (u.weapon.kind == WeaponKind::Artillery)
            return true;
        if (u.sightTarget != t.id || tick_ - u.sightTick >= kRescan + (u.id & 3)) {
            u.sightTarget = t.id, u.sightTick = tick_;
            u.sightClear = lineClear(u, t);
        }
        return u.sightClear;
    };
    // (Nor props, class 0x800000: the Explorer's wreck, the Metar SAD.)
    auto pickable = [&](const Unit& v) { return (classOf(v) & (kNotHit | 0x800000)) == 0; };
    auto distance = [](const Unit& a, const Unit& b) { return std::hypot(b.x - a.x, b.y - a.y); };
    // Within the turret's arc (exe.md#sides-and-combat: some turn only so
    // far each way from straight ahead).
    auto inArc = [&](const Unit& u, const Unit& t) {
        if (u.weapon.arc >= 3.14f)
            return true;
        const float rel = std::remainder(std::atan2(t.y - u.y, t.x - u.x) - (u.building ? 0 : u.heading), 2 * 3.14159265f);
        return std::abs(rel) <= u.weapon.arc + 0.01f;
    };
    // Within its gun's pitch, the ground's heights between them (a lobbed
    // shell isn't held by it).
    auto inPitch = [&](const Unit& u, const Unit& t) {
        if (u.weapon.kind == WeaponKind::Artillery || !terrain_.heightAt)
            return true;
        const float pitch = std::atan2(terrain_.heightAt(t.x, t.y) - terrain_.heightAt(u.x, u.y),
                                       std::max(1.0f, distance(u, t)));
        return pitch <= u.weapon.up + 0.01f && pitch >= -u.weapon.down - 0.01f;
    };
    // To its edge; a shell no further than it can fly (to the middle).
    auto inRange = [&](const Unit& u, const Unit& t) {
        if (u.weapon.kind == WeaponKind::Artillery)
            return distance(u, t) <= std::min(u.weapon.range + radiusOf(t), kShellReach);
        return distance(u, t) <= u.weapon.range + radiusOf(t);
    };
    for (Unit& u : units_) {
        if (u.cloaks && u.hp > 0) {
            if (u.fading > 0) {
                if (--u.fading == 0) {
                    u.cloaked = u.cloaking;
                    u.cloakClock = 0;
                }
            } else if (!u.cloaked && ++u.cloakClock >= kCloakAfter) {
                u.fading = kCloakFade, u.cloaking = true;
            }
        }
        if ((u.order == UnitOrder::Protect || u.order == UnitOrder::ProtectArea) && u.hp > 0 &&
            (tick_ + u.id) % kProtectEvery == 0)
            protectTick(u);
        if (u.weapon.kind == WeaponKind::None || u.hp <= 0 || u.holdFire || u.built < 1)
            continue;
        if (u.cooldown > 0)
            --u.cooldown;
        Unit* t = u.target ? unit(u.target) : nullptr;
        if (t && (t->hp <= 0 || (!u.ordered && !(u.ai ? prey(u.slot, t->slot) : hostile(u, *t)) && t->id != u.lastAttacker)))
            t = nullptr;
        // A cloaked unit is no one's pick (only the player's own order keeps it).
        if (t && t->cloaked && (u.ai || !u.ordered))
            t = nullptr;
        if (t && !u.ordered && !inRange(u, *t))
            t = nullptr;
        // Finding one: the nearest enemy in range (a unit driving on its
        // player's orders doesn't stop to look; an AI's goes for its prey);
        // or whoever fired at it.
        // (A cloaking unit doesn't look on the move, so it stays cloaked,
        // unless it's attacking en route.)
        const bool onTheMove = firesOnTheMove(u);
        const bool looks = (u.ai || u.building || !u.moving() || onTheMove) &&
                           !(u.cloaks && u.moving() && !(onTheMove && u.order == UnitOrder::EnRoute));
        // It looks round every few ticks: for the nearest, unless told whom.
        const bool rescan = (tick_ + u.id) % (kRescan + (u.id & 3)) == 0;
        if (rescan && t && !u.ordered)
            t = nullptr;
        if (!t && looks && (rescan || u.target)) {
            // The nearest with a clear line (the first so in units_'
            // order among the nearest): those passing the cheaper tests,
            // nearest first, their lines checked until one is clear.
            candidates_.clear();
            for (Unit& v : units_)
                if (v.hp > 0 && v.id != u.id && !v.cloaked && v.built >= 1 && pickable(v) &&
                    (u.ai ? prey(u.slot, v.slot) : hostile(u, v)) && inRange(u, v) && inArc(u, v) && inPitch(u, v))
                    if (const float d = distance(u, v); d < 1e30f)
                        candidates_.push_back({d, &v});
            std::stable_sort(candidates_.begin(), candidates_.end(),
                             [](const auto& a, const auto& b) { return a.first < b.first; });
            for (const auto& [d, v] : candidates_)
                if (u.weapon.kind == WeaponKind::Artillery || lineClear(u, *v)) {
                    t = v;
                    break;
                }
        }
        if (!t && u.lastAttacker && looks)
            if (Unit* a = unit(u.lastAttacker); a && a->hp > 0 && !a->cloaked && inRange(u, *a) && inArc(u, *a) && inPitch(u, *a))
                t = a;
        if (!t) {
            u.target = 0, u.ordered = false, u.firing = false, u.burstLeft = 0;
            // Nothing in range: the turret follows the nearest enemy in
            // its box (a square its range across each way), ready.
            if (rescan) {
                u.tracking = 0;
                float best = 1e30f;
                if (looks)
                    for (const Unit& v : units_)
                        if (v.hp > 0 && v.id != u.id && !v.cloaked && v.built >= 1 &&
                            std::abs(v.x - u.x) <= u.weapon.range && std::abs(v.y - u.y) <= u.weapon.range &&
                            (u.ai ? prey(u.slot, v.slot) : hostile(u, v)) && distance(u, v) < best)
                            best = distance(u, v), u.tracking = v.id;
            }
            const Unit* k = u.tracking ? unit(u.tracking) : nullptr;
            turnTurret(u, k ? std::atan2(k->y - u.y, k->x - u.x) - (u.building ? 0 : u.heading) : 0);
            continue;
        }
        u.target = t->id;
        // A line blocked: one it chose (or an AI's) it gives up (it looks
        // again); one the player told it to attack it drives after, as
        // out of range.
        // (Out of range, the line doesn't matter yet: it drives after it.)
        const bool blocked = inRange(u, *t) && !clear(u, *t);
        if (blocked && (!u.ordered || u.ai)) {
            u.target = 0, u.firing = false, u.burstLeft = 0;
            continue;
        }
        if (!inRange(u, *t) || blocked) {
            // Told to attack it: drive after it (an assault doesn't: it
            // fires from where it stands).
            u.firing = false;
            if (u.order == UnitOrder::Assault) {
                u.path.clear();
                continue;
            }
            // (Planned again at most every 20 ticks, as the game re-plans.)
            if (!u.building && tick_ - u.planTick >= 20) {
                u.planTick = tick_;
                setPath(u.id, u.flies ? std::vector<std::pair<float, float>>{{t->x, t->y}} : findPath(u.x, u.y, t->x, t->y));
            }
            continue;
        }
        // Above or below what its gun reaches: it can't fire at it.
        if (!inPitch(u, *t)) {
            u.firing = false;
            continue;
        }
        // (Out of its arc it isn't firing yet: it drives round.)
        u.firing = inArc(u, *t);
        if (!onTheMove && u.firing)
            u.path.clear();
        // Cloaked, it decloaks to fire; it can't while fading either way.
        if (u.cloaks && (u.cloaked || u.fading > 0)) {
            if (u.cloaked)
                u.cloaked = false, u.fading = kCloakFade, u.cloaking = false;
            turnTurret(u, std::atan2(t->y - u.y, t->x - u.x) - (u.building ? 0 : u.heading));
            continue;
        }
        // Outside its turret's arc (one it was told to attack): the turret
        // waits at its stop, and a standing vehicle drives at it, for a
        // point a target's radius short of it on its own side, firing once
        // it comes into the arc (exe.md#orders: the game doesn't turn on
        // the spot).
        if (!inArc(u, *t)) {
            turnTurret(u, std::atan2(t->y - u.y, t->x - u.x) - (u.building ? 0 : u.heading));
            if (!u.building && !u.moving()) {
                const float dx = u.x - t->x, dy = u.y - t->y, d = std::max(1.0f, std::hypot(dx, dy));
                const float r = radiusOf(*t);
                setPath(u.id, findPath(u.x, u.y, t->x + dx / d * r, t->y + dy / d * r));
            }
            continue;
        }
        // On target, or turning onto it.
        if (!turnTurret(u, std::atan2(t->y - u.y, t->x - u.x) - (u.building ? 0 : u.heading)))
            continue;
        // A burst: a pulse a tick, its reload counted from the tick after
        // the last. A defensive turret pays its power for each (and once
        // more for the call that starts the reload); short, it doesn't
        // fire, and a burst breaks off.
        auto paid = [&](float power) { return !shotPower_ || !paysForShots(u.type) || shotPower_(u, power); };
        if (u.burstLeft > 0) {
            if (paid(kShotPower * (u.burstLeft == 1 ? 2 : 1))) {
                fire(u, *t);
                --u.burstLeft;
            } else {
                u.burstLeft = 0;
            }
        } else if (u.cooldown == 0 && paid(kShotPower)) {
            fire(u, *t);
            const int pulses = u.weapon.kind == WeaponKind::Burst ? std::max(1, u.weapon.magazine) : 1;
            u.burstLeft = pulses - 1;
            u.cooldown = u.weapon.reload + (pulses > 1 ? pulses : 0);
        }
    }
    flyProjectiles();
    // The destroyed go.
    std::vector<int> dead;
    for (const Unit& u : units_)
        if (u.hp <= 0)
            dead.push_back(u.id);
    for (int id : dead) {
        WorldEvent e;
        e.kind = WorldEvent::Destroyed;
        e.what = *unit(id);
        e.x0 = e.x1 = e.what.x, e.y0 = e.y1 = e.what.y;
        events_.push_back(std::move(e));
        remove(id);
        for (auto& g : groups_)
            g.erase(std::remove(g.begin(), g.end(), id), g.end());
    }
}

// A protecting unit, every few ticks: after the nearest attacker of its
// side (or of the buildings it guards) while one is about; back to its
// spot once none is.
void World::protectTick(Unit& u) {
    // Chasing an attacker: until it fails the game's target test (gone,
    // cloaked, unbuilt, no longer hostile); no leash.
    if (Unit* c = u.chasing ? unit(u.chasing) : nullptr; c && c->hp > 0 && !c->cloaked && stance(u.slot, c->slot) == 3) {
        if (u.weapon.kind == WeaponKind::None && (u.path.empty() || std::hypot(u.path.back().first - c->x,
                                                                               u.path.back().second - c->y) > kTile))
            u.path = findPath(u.x, u.y, c->x, c->y); // a shield generator brings its field along
        return;
    }
    u.chasing = 0;
    const Unit* nearest = nullptr;
    float best = 1e30f;
    for (const Unit& v : units_) {
        if (v.slot != u.slot || v.hp <= 0)
            continue;
        if (u.order == UnitOrder::ProtectArea && std::find(u.protects.begin(), u.protects.end(), v.id) == u.protects.end())
            continue;
        if (const Unit* a = liveAttacker(v))
            if (const float d = std::hypot(a->x - u.x, a->y - u.y); d < best)
                best = d, nearest = a;
    }
    if (nearest) {
        u.chasing = nearest->id;
        if (u.weapon.kind != WeaponKind::None) {
            u.target = nearest->id;
            u.ordered = true;
        } else {
            setPath(u.id, findPath(u.x, u.y, nearest->x, nearest->y));
        }
        return;
    }
    if (!u.moving() && !u.firing && std::hypot(u.x - u.holdX, u.y - u.holdY) > 256) {
        u.target = 0, u.ordered = false;
        setPath(u.id, findPath(u.x, u.y, u.holdX, u.holdY));
    }
}

// Turns a turret towards an angle (from straight ahead), speeding up and
// slowing down onto it; true once it's there.
bool World::turnTurret(Unit& u, float want) {
    // No further than its stops.
    want = std::remainder(want, 2 * 3.14159265f);
    if (u.weapon.arc < 3.14f)
        want = std::clamp(want, -u.weapon.arc, u.weapon.arc);
    float d = std::remainder(want - u.turretYaw, 2 * 3.14159265f);
    if (std::abs(d) <= kTurretPush && std::abs(u.turretSpin) <= kTurretPush) {
        u.turretYaw = std::remainder(want, 2 * 3.14159265f), u.turretSpin = 0;
        return true;
    }
    // Slow down if it couldn't stop in what's left, else speed up.
    const float stopping = u.turretSpin * u.turretSpin / (2 * kTurretPush);
    const bool towards = (d > 0) == (u.turretSpin > 0) || u.turretSpin == 0;
    if (towards && stopping < std::abs(d))
        u.turretSpin += d > 0 ? kTurretPush : -kTurretPush;
    else
        u.turretSpin -= u.turretSpin > 0 ? std::min(kTurretPush, u.turretSpin) : std::max(-kTurretPush, u.turretSpin);
    u.turretYaw = std::remainder(u.turretYaw + u.turretSpin, 2 * 3.14159265f);
    return false;
}

// Each shot: its damage times the shooter's security skill + 1
// (artillery's as it is).
float World::shotDamage(const Unit& u) const {
    float damage = float(u.weapon.damage * std::max(1, u.weapon.guns));
    if (u.weapon.kind != WeaponKind::Artillery && marksmanship_)
        damage = std::trunc(damage * (marksmanship_(u) + 1));
    return damage;
}

float World::hitDamage(const Unit& u, const Unit& t) const {
    const float damage = shotDamage(u);
    return damage_ ? damage_(t, damage) : damage;
}

void World::fire(Unit& u, Unit& t) {
    const float damage = shotDamage(u);
    // Aimed ahead of the target, standing or moving: where its last
    // tick's movement takes it in the time a shot at the turret's speed
    // takes to get there (one step, as the game's).
    const float speed = u.weapon.kind == WeaponKind::Artillery ? kShellSpeed
                        : u.weapon.kind == WeaponKind::Single  ? kBoltSpeed
                                                               : kBeamLength;
    const float uz = groundAt(u.x, u.y), tz = groundAt(t.x, t.y);
    const float ticks = std::hypot(std::hypot(t.x - u.x, t.y - u.y), tz - uz) / speed;
    const float ax = t.x + t.vx * ticks, ay = t.y + t.vy * ticks;
    Projectile p;
    p.from = u.id, p.fromSlot = u.slot, p.kind = u.weapon.kind, p.damage = damage;
    p.x = p.px = p.ox = u.x, p.y = p.py = p.oy = u.y;
    const float d = std::max(1.0f, std::hypot(ax - u.x, ay - u.y));
    p.dx = (ax - u.x) / d, p.dy = (ay - u.y) / d;
    if (u.weapon.kind == WeaponKind::Artillery) {
        // The game's solve: the shell's speed fixed, its angle found for the
        // aimed spot (the low arc, the high one when the line is blocked),
        // then its speed scaled so that along that way it comes down at the
        // target's distance now.
        const float dz = tz - uz, v2 = kShellSpeed * kShellSpeed, g = kGravity;
        const float disc = (g * dz - v2) * (g * dz - v2) - g * g * (d * d + dz * dz);
        if (disc < 0)
            return; // out of its reach
        const bool high = !lineClear(u, t);
        const float t2 = 2 * (v2 - g * dz + (high ? 1 : -1) * std::sqrt(disc)) / (g * g);
        const float flight = std::sqrt(std::max(t2, 1.0f));
        float vh = d / flight, vz = dz / flight + g * flight / 2;
        const float t0 = std::hypot(t.x - u.x, t.y - u.y) / vh;
        if (t0 * vz - dz > 0) {
            const float k = t0 / std::sqrt(2 * (t0 * vz - dz) / g);
            vh *= k, vz *= k;
        }
        p.z = p.pz = uz;
        p.vx = p.dx * vh, p.vy = p.dy * vh, p.vz = vz;
    } else {
        p.speed = u.weapon.kind == WeaponKind::Single ? kBoltFlight : speed;
        p.range = u.weapon.range + radiusOf(t);
    }
    projectiles_.push_back(p);
    // The target's turrets learn who fired at them.
    // The target records its attacker: a new one replaces the one it has
    // only if nearer (or that one is no longer live).
    if (const Unit* now = liveAttacker(t);
        !now || std::hypot(u.x - t.x, u.y - t.y) < std::hypot(now->x - t.x, now->y - t.y))
        t.lastAttacker = u.id;
    WorldEvent e;
    e.kind = WorldEvent::Shot;
    e.from = u.id, e.to = t.id;
    e.fromSlot = u.slot, e.toSlot = t.slot;
    e.x0 = u.x, e.y0 = u.y, e.x1 = ax, e.y1 = ay;
    e.weapon = u.weapon.kind;
    events_.push_back(std::move(e));
}

float World::groundAt(float x, float y) const { return terrain_.heightAt ? terrain_.heightAt(x, y) : 0; }

const Unit* World::liveAttacker(const Unit& v) const {
    const Unit* a = v.lastAttacker ? unit(v.lastAttacker) : nullptr;
    if (!a || a->hp <= 0 || a->built < 1 || a->cloaked || stance(v.slot, a->slot) != 3)
        return nullptr;
    return a;
}

float World::radiusOf(const Unit& u) const {
    if (u.radius <= 0)
        u.radius = radius_ ? radius_(u) : 0;
    if (u.radius <= 0)
        u.radius = u.building ? kBuildingRadius : kVehicleRadius;
    return u.radius;
}

bool World::lineClear(const Unit& u, const Unit& t) {
    const float sx = t.x - u.x, sy = t.y - u.y, len = std::hypot(sx, sy);
    if (len <= 0)
        return true;
    // The ground: from the shooter's turret to the target's middle (Reluva's
    // heights: half each one's radius up; the game's are its models').
    const float z0 = groundAt(u.x, u.y) + radiusOf(u) / 2, z1 = groundAt(t.x, t.y) + radiusOf(t) / 2;
    const int steps = int(len / 1024);
    for (int i = 1; i < steps; ++i) {
        const float f = float(i) / float(steps);
        if (groundAt(u.x + sx * f, u.y + sy * f) > z0 + (z1 - z0) * f)
            return false;
    }
    for (Unit& v : units_) {
        if (v.id == u.id || v.id == t.id || v.hp <= 0)
            continue;
        const bool enemy = stance(u.slot, v.slot) == 3;
        if (enemy && !(u.slot <= 2 && (classOf(v) & kUnitClasses) == 0))
            continue; // an enemy's unit never blocks; an enemy's building only a colony's line
        const float r = radiusOf(v);
        if (v.x < std::min(u.x, t.x) - r || v.x > std::max(u.x, t.x) + r || v.y < std::min(u.y, t.y) - r ||
            v.y > std::max(u.y, t.y) + r)
            continue;
        if (std::abs((v.x - u.x) * sy - (v.y - u.y) * sx) / len <= r)
            return false;
    }
    return true;
}

// A tick of the projectiles in flight: each moves on (a pulse from its
// shooter's muzzle as the turret now stands, a bolt straight on, a shell
// along its arc) and strikes the first object its path crossed this tick
// that isn't its shooter's side (nor a bee, unarmed APC or effect), or
// the ground (nothing), or runs out.
void World::flyProjectiles() {
    std::vector<Projectile> flying;
    for (Projectile p : projectiles_) {
        ++p.age;
        p.px = p.x, p.py = p.y, p.pz = p.z;
        bool done = false;
        if (p.kind == WeaponKind::Artillery) {
            p.x += p.vx, p.y += p.vy, p.z += p.vz - kGravity / 2; // an exact parabola
            p.vz -= kGravity;
            done = p.age >= kShellLife;
        } else if (p.kind == WeaponKind::Single) {
            p.x += p.dx * p.speed, p.y += p.dy * p.speed;
            done = float(p.age) * p.speed >= p.range;
        } else {
            if (const Unit* s = unit(p.from))
                p.ox = s->x, p.oy = s->y;
            const float run = std::min(float(p.age) * p.speed, p.range);
            p.px = p.ox + p.dx * (run - p.speed), p.py = p.oy + p.dy * (run - p.speed);
            p.x = p.ox + p.dx * run, p.y = p.oy + p.dy * run;
            done = run >= p.range;
        }
        // The first object along this tick's path.
        const float sx = p.x - p.px, sy = p.y - p.py, len2 = sx * sx + sy * sy;
        Unit* hit = nullptr;
        float best = 2;
        // Anything not its shooter's own side's, but bees, unarmed APCs and
        // effects: a sphere of the object's radius round where it stands
        // (a shell in three dimensions; a pulse or bolt flies level), the
        // path's ends stretched a quarter each way.
        for (Unit& v : units_) {
            if (v.hp <= 0 || v.id == p.from || v.slot == p.fromSlot || (classOf(v) & kNotHit))
                continue;
            const float r = radiusOf(v);
            const bool shell = p.kind == WeaponKind::Artillery;
            const float sz = shell ? p.z - p.pz : 0, vz = shell ? groundAt(v.x, v.y) - p.pz : 0;
            const float l2 = len2 + sz * sz;
            const float f = l2 > 0 ? std::clamp(((v.x - p.px) * sx + (v.y - p.py) * sy + vz * sz) / l2, -0.25f, 1.25f) : 0;
            const float ex = p.px + sx * f - v.x, ey = p.py + sy * f - v.y, ez = shell ? sz * f - vz : 0;
            if (ex * ex + ey * ey + ez * ez > r * r || f >= best)
                continue;
            hit = &v, best = f;
        }
        if (hit) {
            // (A building still going up takes no harm.)
            if (hit->built >= 1)
                hit->hp -= damage_ ? damage_(*hit, p.damage) : p.damage;
            hit->attackedTick = tick_;
            WorldEvent e;
            e.kind = WorldEvent::Hit;
            e.from = p.from, e.to = hit->id;
            e.fromSlot = p.fromSlot, e.toSlot = hit->slot;
            e.x0 = e.x1 = p.px + sx * std::clamp(best, 0.0f, 1.0f), e.y0 = e.y1 = p.py + sy * std::clamp(best, 0.0f, 1.0f);
            e.weapon = p.kind;
            events_.push_back(std::move(e));
            continue;
        }
        if (p.kind == WeaponKind::Artillery && p.z < groundAt(p.x, p.y))
            continue; // into the ground: nothing
        if (!done)
            flying.push_back(p);
    }
    projectiles_ = std::move(flying);
}

void World::syncState(save::Serializer& s) {
    s(stances_)(events_)(projectiles_)(tickClock_)(tick_)(units_)(groups_)(nextId_);
    if (s.loading()) {
        // Every id one handed out before (the index has a place for each
        // up to the next; a damaged save's mustn't make it huge).
        if (nextId_ < 1 || nextId_ > kMaxIds)
            s.fail("a unit count out of range");
        for (const Unit& u : units_)
            if (u.id < 1 || u.id >= nextId_)
                s.fail("a unit id out of range");
        if (!s.ok())
            units_.clear(), nextId_ = 1;
        for (Unit& u : units_)
            u.lastHeading = u.heading, u.lastTurretYaw = u.turretYaw;
    }
    reindex();
}

} // namespace reluva::newworlds
