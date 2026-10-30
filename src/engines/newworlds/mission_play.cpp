// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/mission_ui.h"
#include "engines/newworlds/mission_surface.h"
#include "engines/newworlds/viewers.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace reluva::newworlds {

namespace {

// The menus' order, as the training missions' pages show it; what they
// didn't show after.
int menuRank(std::string_view id) {
    static const char* const kOrder[] = {"storage-facility", "power-generator", "construction-yard", "vehicle-yard",
                                         "resource-processor", "mining-station", "transporter-pad", "landing-pad",
                                         "science-station", "sickbay", "security-center",
                                         "cargo-bee", "construction-bee", "worker-bee", "ground-scout", "armed-apc",
                                         "photon-artillery", "advanced-photon-artillery"};
    for (size_t i = 0; i < std::size(kOrder); ++i)
        if (id == kOrder[i])
            return int(i);
    return int(std::size(kOrder));
}

// A bee's height above the ground in flight (showUnits): construction
// and cargo bees cruise in eight lanes 9000-12500 up (exe.md#docking),
// worker bees 500 up, rising off and settling back down.
float beeLift(const Economy::Bee& b) {
    const float f = b.flight > 0 ? float(b.flown) / float(b.flight) : 0;
    const float height = b.worker ? 500.0f : 9000.0f + 500.0f * float(b.id % 8);
    return b.flight > 0 ? height * std::min(1.0f, 4 * std::min(f, 1 - f)) : 0;
}

// Whether showUnits draws a bee: out of the hub, not at work on a site
// (the map draws those at its corners), not waiting docked inside its
// building.
bool beeShown(const Economy::Bee& b) {
    return !(b.home || b.working || (b.worker && b.flight == 0 && b.pods.empty()));
}

} // namespace

// What combat did since the last step, to draw: sparks where shots
// struck, blasts where something was destroyed, the colony's lost
// buildings flattened.
void MissionViewer::combatStep() {
    if (!sim_)
        return;
    const auto events = sim_->takeEvents();
    bool rebuild = false;
    for (const WorldEvent& e : events) {
        // A vehicle setting off: its start-up sound from where it set out
        // (any side's; not over its own still playing).
        if (e.kind == WorldEvent::Moved) {
            if (const auto [archive, stem] = driveSound(e.what.type); !archive.empty() &&
                                                                      !(mixer_ && mixer_->isPlaying(driveHandles_[e.from])))
                driveHandles_[e.from] = playAt(archive, stem, e.x0, e.y0, 1);
            continue;
        }
        if (e.kind == WorldEvent::Hit)
            play_.flashes.push_back({e.x1, e.y1, e.x1, e.y1, e.weapon, nowMs_ + 200});
        if (e.kind != WorldEvent::Destroyed)
            continue;
        play_.blasts.push_back({e.x0, e.y0, e.what.building ? 5000.0f : 2500.0f, nowMs_});
        rebuild = rebuild || e.what.building;
    }
    size_t others = 0;
    for (const Unit& u : world_->units())
        others += u.building && u.slot != playerSlot_ && !u.kind.model.empty() && visible(u);
    if (rebuild || others != shownBuildings_)
        showColony();
    if (!events.empty())
        showUnits();
    play_.flashes.erase(
        std::remove_if(play_.flashes.begin(), play_.flashes.end(), [&](const Flash& f) { return f.until <= nowMs_; }),
        play_.flashes.end());
    play_.blasts.erase(std::remove_if(play_.blasts.begin(), play_.blasts.end(),
                                      [&](const Blast& b) { return b.start + 800 <= nowMs_; }),
                       play_.blasts.end());
    if (!play_.flashes.empty() || !play_.blasts.empty() || !events.empty() || !world_->projectiles().empty() ||
        (effects_ && !effects_->sparks().empty()) || (fieldWork_ && !fieldWork_->scanning().empty()))
        redraw(); // (they move, flicker or march on as time passes)
}

// Shots as beams (phasers orange, disruptors green) or a shell flying,
// blasts where something was destroyed, and a hit point bar over what's
// damaged or selected: only what the player sees (the fog hides the rest).
void MissionViewer::drawCombat() {
    if (!world_ || play_.tricorderFull)
        return;
    auto toScreen = [&](float x, float y, float up, int& sx, int& sy) {
        return map_.worldToScreen(x, y, map_.groundAtWorld(x, y) + up, sx, sy);
    };
    auto seen = [&](float x, float y) { return !fog_ || fog_->seenAt(x, y, playerSlot_); };
    // The projectiles in flight: a phaser pulse an orange streak, a
    // disruptor bolt a green one, a shell a bright dot up its arc; each as
    // far between its last two ticks as the units are (drawnAt), a beam
    // or bolt its tick's length along its way (not back past where it set
    // out), a pulse's from its shooter's muzzle as drawn.
    const float share = sim_ ? float(sim_->tickShare()) : 1;
    for (const Projectile& p : world_->projectiles()) {
        if (p.fromSlot != playerSlot_ && !seen(p.x, p.y))
            continue;
        int ax = 0, ay = 0, bx = 0, by = 0;
        if (p.kind == WeaponKind::Artillery) {
            const float x = p.px + (p.x - p.px) * share, y = p.py + (p.y - p.py) * share;
            if (!map_.worldToScreen(x, y, p.pz + (p.z - p.pz) * share + 300, ax, ay))
                continue;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    plot(screen_, ax + dx, ay + dy, 255, 240, 160);
            continue;
        }
        float ox = p.x - p.dx * p.speed * float(p.age), oy = p.y - p.dy * p.speed * float(p.age);
        float run = std::max(float(p.age) - 1 + share, 0.0f) * p.speed;
        if (p.kind != WeaponKind::Single) {
            ox = p.ox, oy = p.oy;
            if (const Unit* s = world_->unit(p.from)) {
                const Pose at = drawnAt(*s);
                ox = at.x, oy = at.y;
            }
            run = std::min(run, p.range);
        }
        const float tail = std::max(run - p.speed, 0.0f);
        if (!toScreen(ox + p.dx * tail, oy + p.dy * tail, 600, ax, ay) ||
            !toScreen(ox + p.dx * run, oy + p.dy * run, 600, bx, by))
            continue;
        if (p.kind == WeaponKind::Single) {
            line(screen_, ax, ay, bx, by, 90, 255, 90);
            line(screen_, ax, ay + 1, bx, by + 1, 40, 200, 40);
        } else {
            line(screen_, ax, ay, bx, by, 255, 170, 60);
        }
    }
    // Where they struck: a spark.
    for (const Flash& f : play_.flashes) {
        int cx = 0, cy = 0;
        if (!seen(f.x1, f.y1) || !toScreen(f.x1, f.y1, 500, cx, cy))
            continue;
        const bool green = f.weapon == WeaponKind::Single;
        for (int d = -3; d <= 3; ++d) {
            plot(screen_, cx + d, cy, 255, green ? 255 : 230, green ? 140 : 120);
            plot(screen_, cx, cy + d, 255, green ? 255 : 230, green ? 140 : 120);
        }
    }
    for (const Blast& b : play_.blasts) {
        int cx = 0, cy = 0, ex = 0, ey = 0;
        if (!seen(b.x, b.y) || !toScreen(b.x, b.y, 300, cx, cy) || !toScreen(b.x + b.size, b.y, 300, ex, ey))
            continue;
        const float t = float(nowMs_ - b.start) / 800.0f;
        const int r = std::max(2, int(std::hypot(float(ex - cx), float(ey - cy)) * (0.3f + t)));
        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx)
                if (dx * dx + dy * dy <= r * r && (dx * dx + dy * dy) * 3 >= r * r * int(t * 3))
                    plot(screen_, cx + dx, cy + dy, 255, uint8_t(220 - 160 * t), uint8_t(80 - 70 * t));
    }
    // The script's effects: meteors (a falling streak, then a blast),
    // storm flashes, beams (a column of light).
    if (effects_)
        for (const ScriptEffects::Spark& sp : effects_->sparks()) {
            int gx = 0, gy = 0, tx = 0, ty = 0;
            if (!toScreen(sp.x, sp.y, 0, gx, gy))
                continue;
            const float t = float(sp.age);
            if (sp.kind == 0 && toScreen(sp.x, sp.y, 6000 * std::max(0.0f, 0.3f - t) / 0.3f, tx, ty)) {
                line(screen_, tx, ty - 20, tx, ty, 255, 200, 120);
                if (t > 0.3f)
                    for (int dy = -4; dy <= 4; ++dy)
                        for (int dx = -4; dx <= 4; ++dx)
                            if (dx * dx + dy * dy <= int(16 * (1 - t)))
                                plot(screen_, gx + dx, gy + dy, 255, 160, 60);
            } else if (sp.kind == 1 && t < 0.15f && toScreen(sp.x, sp.y, 8000, tx, ty)) {
                line(screen_, tx, ty, gx + 6, (ty + gy) / 2, 200, 220, 255);
                line(screen_, gx + 6, (ty + gy) / 2, gx, gy, 200, 220, 255);
            } else if (sp.kind == 2 && toScreen(sp.x, sp.y, 4000, tx, ty)) {
                for (int dx = -2; dx <= 2; ++dx)
                    if ((int(nowMs_ / 40) + dx) % 2)
                        line(screen_, gx + dx, gy, tx + dx, ty, 180, 220, 255);
            }
        }
    // Scans as a dotted pale blue line; held buildings marked.
    if (fieldWork_) {
        for (const auto& [from, to] : fieldWork_->scanning()) {
            const Unit* a = world_->unit(from);
            const Unit* b = world_->unit(to);
            int ax = 0, ay = 0, bx = 0, by = 0;
            if (!a || !b || !visible(*a) || !visible(*b))
                continue;
            const Pose pa = drawnAt(*a), pb = drawnAt(*b);
            if (!toScreen(pa.x, pa.y, 700, ax, ay) || !toScreen(pb.x, pb.y, 800, bx, by))
                continue;
            const int n = std::max({std::abs(bx - ax), std::abs(by - ay), 1});
            const int phase = int(nowMs_ / 60 % 6);
            for (int i = 0; i <= n && n < 4000; ++i)
                if ((i + phase) % 6 < 3)
                    plot(screen_, ax + (bx - ax) * i / n, ay + (by - ay) * i / n, 140, 210, 255);
        }
        for (const Unit& u : world_->units())
            if (const int holder = fieldWork_->heldBy(u.id); holder >= 0 && visible(u)) {
                int sx = 0, sy = 0;
                if (toScreen(u.x, u.y, 3500, sx, sy))
                    ui_.drawSmall(screen_, sx - ui_.smallWidth("CAPTURED") / 2, sy - 12, "CAPTURED",
                                  holder == playerSlot_ ? Color{120, 255, 120} : Color{255, 90, 60});
            }
    }
    for (const Unit& u : world_->units()) {
        if (!(u.selected || u.hp < u.maxHp) || u.hp <= 0 || !visible(u))
            continue;
        int sx = 0, sy = 0;
        if (const Pose at = drawnAt(u); !toScreen(at.x, at.y, u.building ? 3000.0f : 1500.0f, sx, sy))
            continue;
        const int w = u.building ? 24 : 14, filled = int(float(w) * u.hp / u.maxHp);
        for (int x = 0; x < w; ++x)
            for (int y = 0; y < 2; ++y) {
                const bool on = x < filled;
                const float health = u.hp / u.maxHp;
                plot(screen_, sx - w / 2 + x, sy + y, on ? uint8_t(health < 0.5f ? 255 : 80) : 40,
                     on ? uint8_t(health < 0.25f ? 60 : 220) : 40, on ? 60 : 40);
            }
    }
}

// Whether the player sees a thing: its own always, another side's on a
// block its fog marks as seen (the game draws nothing else).
bool MissionViewer::visible(const Unit& u) const {
    return u.slot == playerSlot_ || ((!fog_ || fog_->seenAt(u.x, u.y, playerSlot_)) && !u.cloaked);
}

// Turning the short way round; one that appeared or jumped this tick
// (further than it drives in one) is where it is.
MissionViewer::Pose MissionViewer::drawnAt(const Unit& u) const {
    const float share = sim_ ? float(sim_->tickShare()) : 1;
    const float dx = u.x - u.lastX, dy = u.y - u.lastY, reach = u.kind.speed / 20 * 1.01f + 1;
    if (share >= 1 || dx * dx + dy * dy > reach * reach)
        return {u.x, u.y, u.heading, u.turretYaw};
    auto turn = [share](float from, float to) { return from + std::remainder(to - from, 2 * 3.14159265f) * share; };
    return {u.lastX + dx * share, u.lastY + dy * share, turn(u.lastHeading, u.heading),
            turn(u.lastTurretYaw, u.turretYaw)};
}

// As each tick begins: the bees as the tick before left them become where
// they're drawn from (with several ticks in a frame, so from where the
// last began, as the units' last poses are).
void MissionViewer::noteBees() {
    play_.beesBefore.clear();
    if (economy_)
        for (const Economy::Bee& b : economy_->bees())
            if (beeShown(b))
                play_.beesBefore[b.id] = {b.x, b.y, b.heading, beeLift(b)};
}

void MissionViewer::showUnits() {
    std::vector<MapViewer::UnitDraw> draw;
    if (world_)
        for (const Unit& u : world_->units())
            if (!u.building && !u.kind.model.empty() && visible(u)) {
                // A cloaking unit shimmers over its 63-tick fades, then is
                // drawn as it is (the side's own; others' are gone): its
                // own side's most at the middle of a fade, another side's
                // most as it nears cloaked (exe.md#cloaking).
                float shimmer = 0;
                if (u.fading > 0) {
                    const float towards = float(u.cloaking ? 63 - u.fading : u.fading) / 63;
                    shimmer = u.slot == playerSlot_ ? 97 * std::sin(3.14159265f * towards) : 98 * towards;
                }
                const Pose at = drawnAt(u);
                MapViewer::UnitDraw d{u.kind.model, at.x, at.y, at.heading, u.selected, at.turretYaw, 0, shimmer};
                d.health = u.maxHp > 0 ? u.hp / u.maxHp : 1;
                draw.push_back(d);
            }
    // The colony's bees out of the hub: flying (rising off and settling
    // back down, beeLift), or docked at a mine or the processor; those at
    // work on a site the map draws at its corners. Each is drawn between
    // where the last two ticks found it (one just out, or gone further
    // than a bee flies in a tick or two, where it is); a pod aboard hangs
    // under its bee.
    if (economy_) {
        const BeeModels& models = beeModels(side());
        const float share = sim_ ? float(sim_->tickShare()) : 1;
        for (const Economy::Bee& b : economy_->bees()) {
            if (!beeShown(b))
                continue;
            float x = b.x, y = b.y, heading = b.heading, lift = beeLift(b);
            if (const auto it = play_.beesBefore.find(b.id);
                it != play_.beesBefore.end() && std::hypot(x - it->second[0], y - it->second[1]) < 2500) {
                const auto [x0, y0, heading0, lift0] = it->second;
                x = x0 + (x - x0) * share, y = y0 + (y - y0) * share;
                heading = heading0 + std::remainder(heading - heading0, 2 * 3.14159265f) * share;
                lift = lift0 + (lift - lift0) * share;
            }
            const char* model = b.worker ? models.worker : b.cargo ? models.cargo : models.construction;
            MapViewer::UnitDraw d{model, x, y, heading};
            d.lift = lift;
            draw.push_back(d);
            for (size_t k = 0; k < b.pods.size(); ++k) {
                MapViewer::UnitDraw c{models.pod, x + float(k) * 600, y, heading};
                c.lift = std::max(0.0f, lift - 400);
                draw.push_back(c);
            }
        }
        // The cargo pods standing on the yards.
        for (const Economy::Pod& p : economy_->pods())
            if (p.state == Economy::Pod::Parked)
                draw.push_back({models.pod, p.x, p.y, 0});
    }
    map_.setUnits(std::move(draw));
}

std::vector<const BuildingType*> MissionViewer::buildMenu() const {
    std::vector<const BuildingType*> out;
    if (!economy_)
        return out;
    const char side = this->side();
    // What can be built now, as the game's menu offers it: buildings the
    // side has a model for, and upgrades.
    for (const BuildingType& t : buildingTypes())
        if (economy_->available(t) && (!t.upgradeOf.empty() || !buildingModel(t.id, side).empty()))
            out.push_back(&t);
    // (Upgrades, which the game offers on their building's own page, last.)
    std::stable_sort(out.begin(), out.end(), [](const BuildingType* a, const BuildingType* b) {
        return std::pair{!a->upgradeOf.empty(), menuRank(a->id)} < std::pair{!b->upgradeOf.empty(), menuRank(b->id)};
    });
    return out;
}

std::vector<const VehicleType*> MissionViewer::vehicleMenu() const {
    std::vector<const VehicleType*> out = economy_ ? economy_->vehicleMenu() : std::vector<const VehicleType*>{};
    std::stable_sort(out.begin(), out.end(), [](const VehicleType* a, const VehicleType* b) {
        const CombatType *ta = combatType(a->type), *tb = combatType(b->type);
        return menuRank(ta ? ta->name : "") < menuRank(tb ? tb->name : "");
    });
    return out;
}

void MissionViewer::choose(const BuildingType& t) {
    if (!economy_ || !economy_->available(t))
        return;
    if (!t.upgradeOf.empty()) {
        // An upgrade or add-on goes on its building at once, its model
        // (where it has one) on the building: the building selected if it
        // can take it, else the first of its kind still without it (each
        // building has its own, exe.md#upgrades-and-add-ons). One the
        // stock can't pay is refused, with Engineering's report.
        const ColonyStructure* picked = structureOf(play_.selectedBuilding);
        if (const ColonyStructure* s = economy_->structure(economy_->hostFor(t, picked ? picked->id : 0))) {
            if (!economy_->affordable(t)) {
                economy_->order(t.id, s->x, s->y, 0, 0);
            } else if (colony_ && colony_->add(t.id)) {
                const ColonyBuilding& c = colony_->buildings().back();
                sim_->placeUpgrade(t.id, c.model, c.a, c.b, s->x, s->y);
                showColony();
            } else if (colony_ && buildingModel(t.id, side()).empty()) {
                // Shown on its own building's spot.
                const auto& ids = sim_->structureIds();
                for (size_t i = 0; i < ids.size() && i < sim_->colony().size(); ++i)
                    if (ids[i] == s->id) {
                        const ColonyBuilding& c = sim_->colony()[i];
                        sim_->placeUpgrade(t.id, "", c.a, c.b, s->x, s->y);
                        break;
                    }
            }
        }
        play_.buildMenu = false;
        return;
    }
    play_.placing = t.id;
    play_.buildMenu = false;
    play_.ghostA = play_.ghostB = -1;
    updateGhost();
}

// The hologram follows the pointer, on the tile grid, green where the
// building can go; gone while the pointer is over a panel (or the
// full-screen Tricorder is up), the map under it not to be built on.
void MissionViewer::updateGhost() {
    if (play_.placing.empty() || !colony_)
        return;
    float x = 0, y = 0;
    int a = -1, b = -1;
    bool ok = false;
    if ((!playing_ || (!play_.tricorderFull && panelAt(mouseX_, mouseY_) < 0)) &&
        map_.screenToGround(mouseX_, mouseY_, x, y)) {
        const int pa = int(y / MapViewer::kBlockUnits * 8), pb = int(x / MapViewer::kBlockUnits * 8);
        // In play, the game's own test (exe.md#building-placement).
        if (sim_ ? !sim_->placementSpot(play_.placing, x, y, a, b, ok)
                 : !colony_->spotAt(play_.placing, pa, pb, a, b, ok))
            a = b = -1;
    }
    if (a == play_.ghostA && b == play_.ghostB && ok == play_.ghostOk)
        return;
    play_.ghostA = a, play_.ghostB = b, play_.ghostOk = ok;
    map_.setGhost(a < 0 ? std::string() : buildingModel(play_.placing, side()), a, b, ok);
}

void MissionViewer::placeBuilding() {
    if (play_.placing.empty() || !colony_ || !economy_ || !play_.ghostOk || play_.ghostA < 0)
        return;
    if (!sim_ ||
        sim_->placeBuilding(play_.placing, buildingModel(play_.placing, side()), play_.ghostA, play_.ghostB) < 0)
        return;
    colony_->put(play_.placing, play_.ghostA, play_.ghostB);
    play_.placing.clear();
    map_.setGhost({}, 0, 0, false);
    showColony();
}

void MissionViewer::showProgress() {
    // The other sides' sites; one finished loses its bee.
    const size_t own = buildingsShown_ - play_.otherBuildings.size();
    bool finished = false;
    for (size_t i = 0; world_ && i < play_.otherBuildings.size(); ++i)
        if (const Unit* u = world_->unit(play_.otherBuildings[i].first);
            u && u->built != play_.otherBuildings[i].second) {
            finished = finished || u->built >= 1;
            play_.otherBuildings[i].second = u->built;
            map_.setBuildingProgress(own + i, u->built);
        }
    if (finished) {
        showColony();
        return;
    }
    if (!economy_)
        return;
    const auto& all = economy_->structures();
    for (size_t i = 0; i < structureIds().size(); ++i)
        for (const ColonyStructure& c : all)
            if (c.id == structureIds()[i]) {
                map_.setBuildingProgress(i, c.progress, economy_->beesAt(c.id));
                if (Unit* u = world_ && i < colonyObjects().size() && colonyObjects()[i] > 0 ? world_->unit(colonyObjects()[i]) : nullptr) {
                    u->built = c.progress;
                    u->efficiency = economy_->efficiency(c);
                }
            }
}

} // namespace reluva::newworlds
