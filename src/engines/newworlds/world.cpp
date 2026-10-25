// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/world.h"

#include <algorithm>
#include <cmath>
#include <queue>

namespace reluva::newworlds {

namespace {

// Placeholder: a ground scout crosses a block in about four seconds, by
// eye from the recordings; to be timed in the game.
constexpr float kScoutSpeed = 4000;
// Spacing of units sent to one point.
constexpr float kSpread = 2500;
// How far past a building's middle a shot reaches it (its walls); a
// placeholder.
constexpr float kBuildingReach = 6000;
// Turrets (exe.md#sides-and-combat): they turn speeding up and then
// slowing down onto their target, at 128 of the game's 65536ths of a turn
// a tick each tick, and fire only when on it. They look for a target
// every 8-11 ticks. A shell's reach on flat ground (its speed and gravity).
constexpr float kTurretPush = 128.0f / 65536 * 2 * 3.14159265f;
constexpr int kRescan = 8;
constexpr float kShellReach = 36850;
// Cloaking (exe.md#cloaking): ticks seen before it cloaks, and a fade's.
constexpr int kCloakAfter = 300, kCloakFade = 63;
// Protecting (exe.md#orders): how often it looks for an attack on the
// side, and how lately an object must have been hit to count (Reluva's:
// the game asks whether its attacker is still about).
constexpr int kProtectEvery = 8, kUnderAttack = 100;

// Whether a unit keeps driving while it fires: one moving on the
// player's move or attack-en-route order (a cloaking unit not on a move).
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

bool World::passable(int tx, int ty) const {
    if (tx < 0 || ty < 0 || tx >= terrain_.width || ty >= terrain_.height)
        return false;
    return terrain_.moveClass[size_t(ty) * size_t(terrain_.width) + size_t(tx)] != 0;
}

bool World::canStep(int tx, int ty, int dx, int dy) const {
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
    std::vector<float> cost(size_t(w) * size_t(h), 1e30f);
    std::vector<int> from(size_t(w) * size_t(h), -1);
    using Node = std::pair<float, int>; // estimate, tile index
    std::priority_queue<Node, std::vector<Node>, std::greater<>> open;
    auto guess = [&](int x, int y) {
        const float dx = float(std::abs(x - gx)), dy = float(std::abs(y - gy));
        return std::max(dx, dy) + 0.4142f * std::min(dx, dy);
    };
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
    u.x = x, u.y = y, u.heading = heading;
    units_.push_back(std::move(u));
    return units_.back().id;
}

Unit* World::unit(int id) {
    for (Unit& u : units_)
        if (u.id == id)
            return &u;
    return nullptr;
}

const Unit* World::unit(int id) const {
    for (const Unit& u : units_)
        if (u.id == id)
            return &u;
    return nullptr;
}

void World::remove(int id) {
    units_.erase(std::remove_if(units_.begin(), units_.end(), [&](const Unit& u) { return u.id == id; }), units_.end());
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

void World::moveTo(int id, float x, float y) {
    if (Unit* u = unit(id); u && !u->building) {
        u->path = u->flies ? std::vector<std::pair<float, float>>{{x, y}} : findPath(u->x, u->y, x, y);
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
    bool any = false;
    tickClock_ += seconds * 20;
    while (tickClock_ >= 1) {
        tickClock_ -= 1;
        combatTick();
        any = true;
    }
    for (Unit& u : units_) {
        if ((u.firing && !firesOnTheMove(u)) || u.building)
            continue;
        float left = u.kind.speed * float(seconds);
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
            any = true;
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
    auto distance = [](const Unit& a, const Unit& b) { return std::hypot(b.x - a.x, b.y - a.y); };
    auto inRange = [&](const Unit& u, const Unit& t) {
        const float range = u.weapon.kind == WeaponKind::Artillery ? std::min(u.weapon.range, kShellReach) : u.weapon.range;
        return distance(u, t) <= range + (t.building ? kBuildingReach : 0);
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
            float best = 1e30f;
            for (Unit& v : units_)
                if (v.hp > 0 && v.id != u.id && !v.cloaked && v.built >= 1 && (u.ai ? prey(u.slot, v.slot) : hostile(u, v)) && inRange(u, v) &&
                    distance(u, v) < best)
                    best = distance(u, v), t = &v;
        }
        if (!t && u.lastAttacker && looks)
            if (Unit* a = unit(u.lastAttacker); a && a->hp > 0 && !a->cloaked && inRange(u, *a))
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
        if (!inRange(u, *t)) {
            // Told to attack it: drive after it (an assault doesn't: it
            // fires from where it stands).
            u.firing = false;
            if (u.order == UnitOrder::Assault) {
                u.path.clear();
                continue;
            }
            if (!u.building && (u.path.empty() || (tick_ + u.id) % 20 == 0))
                u.path = u.flies ? std::vector<std::pair<float, float>>{{t->x, t->y}} : findPath(u.x, u.y, t->x, t->y);
            continue;
        }
        u.firing = true;
        if (!onTheMove)
            u.path.clear();
        // Cloaked, it decloaks to fire; it can't while fading either way.
        if (u.cloaks && (u.cloaked || u.fading > 0)) {
            if (u.cloaked)
                u.cloaked = false, u.fading = kCloakFade, u.cloaking = false;
            turnTurret(u, std::atan2(t->y - u.y, t->x - u.x) - (u.building ? 0 : u.heading));
            continue;
        }
        // On target, or turning onto it.
        if (!turnTurret(u, std::atan2(t->y - u.y, t->x - u.x) - (u.building ? 0 : u.heading)))
            continue;
        // A burst: a pulse a tick, its reload counted from the tick after
        // the last.
        if (u.burstLeft > 0) {
            fire(u, *t);
            --u.burstLeft;
        } else if (u.cooldown == 0) {
            fire(u, *t);
            const int pulses = u.weapon.kind == WeaponKind::Burst ? std::max(1, u.weapon.magazine) : 1;
            u.burstLeft = pulses - 1;
            u.cooldown = u.weapon.reload + (pulses > 1 ? pulses : 0);
        }
    }
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
    if (Unit* c = u.chasing ? unit(u.chasing) : nullptr; c && c->hp > 0 && !c->cloaked) {
        if (u.weapon.kind == WeaponKind::None && (u.path.empty() || std::hypot(u.path.back().first - c->x,
                                                                               u.path.back().second - c->y) > kTile))
            u.path = findPath(u.x, u.y, c->x, c->y); // a shield generator brings its field along
        return;
    }
    u.chasing = 0;
    const Unit* nearest = nullptr;
    float best = 1e30f;
    for (const Unit& v : units_) {
        if (v.slot != u.slot || v.hp <= 0 || tick_ - v.attackedTick > kUnderAttack)
            continue;
        if (u.order == UnitOrder::ProtectArea && std::find(u.protects.begin(), u.protects.end(), v.id) == u.protects.end())
            continue;
        if (const Unit* a = unit(v.lastAttacker); a && a->hp > 0 && !a->cloaked && a->slot != u.slot)
            if (const float d = std::hypot(a->x - u.x, a->y - u.y); d < best)
                best = d, nearest = a;
    }
    if (nearest) {
        u.chasing = nearest->id;
        if (u.weapon.kind != WeaponKind::None) {
            u.target = nearest->id;
            u.ordered = true;
        } else {
            u.path = findPath(u.x, u.y, nearest->x, nearest->y);
        }
        return;
    }
    if (!u.moving() && !u.firing && std::hypot(u.x - u.holdX, u.y - u.holdY) > kTile) {
        u.target = 0, u.ordered = false;
        u.path = findPath(u.x, u.y, u.holdX, u.holdY);
    }
}

// Turns a turret towards an angle (from straight ahead), speeding up and
// slowing down onto it; true once it's there.
bool World::turnTurret(Unit& u, float want) {
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

void World::fire(Unit& u, Unit& t) {
    // Each shot: its damage times the shooter's security skill + 1
    // (artillery's as it is). (A building still going up takes no harm.)
    float damage = float(u.weapon.damage * std::max(1, u.weapon.guns));
    if (u.weapon.kind != WeaponKind::Artillery && marksmanship_)
        damage = std::trunc(damage * (marksmanship_(u) + 1));
    if (t.built >= 1)
        t.hp -= damage_ ? damage_(t, damage) : damage;
    t.lastAttacker = u.id;
    t.attackedTick = tick_;
    WorldEvent e;
    e.kind = WorldEvent::Shot;
    e.from = u.id, e.to = t.id;
    e.fromSlot = u.slot, e.toSlot = t.slot;
    e.x0 = u.x, e.y0 = u.y, e.x1 = t.x, e.y1 = t.y;
    e.weapon = u.weapon.kind;
    events_.push_back(std::move(e));
}

} // namespace reluva::newworlds
