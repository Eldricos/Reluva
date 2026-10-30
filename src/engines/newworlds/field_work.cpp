// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/field_work.h"

#include "engines/newworlds/ai_sides.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace reluva::newworlds {

namespace {

constexpr float kBlock = 16384;
// From the executable (exe.md#scans-and-captures): how near an officer's
// building a vehicle must be to take the officer aboard (from the building's
// entry; here, its walls), how many a vehicle carries, the strength a
// strong scan needs, and how long an away team takes to go in.
constexpr float kOfficerReach = 1414;
constexpr size_t kOfficerSeats = 16;
constexpr float kStrongSkill = 0.5f;
constexpr int kTeamTicks = 336;
// The raids' and watches' delays are the game's raw 50 ms ticks
// (exe.md#the-simulation-clock), as the simulation's.
constexpr double kScriptTicks = 20;
constexpr double kTick = 0.05;
// Placeholders: a target's size (its model's bounding radius), how near
// its entry an APC stops, how near a site counts as the target's, and a
// watch for a scan no raid covers (the game has none; the scripts' own
// targets need one): its reply 2 ticks after the scanner comes in range,
// as the game's routine reply (exe.md#scans-and-captures), 8 s to finish.
constexpr float kBuildingSize = 6000, kVehicleSize = 1000;
constexpr float kEntry = 7000;
constexpr float kSiteNear = 2 * kBlock;
constexpr int kOwnStart = 2, kOwnHold = 160;

int conditionOf(FieldWork::Kind kind, bool strong) { return 4 + int(kind) + (strong ? 3 : 0); }

std::string scanName(int condition) {
    static const char* kNames[] = {"science", "medical", "engineering", "strong-science", "strong-medical",
                                   "strong-engineering"};
    return condition == 3 ? "cell" : condition >= 4 && condition <= 9 ? kNames[condition - 4] : "";
}

} // namespace

const char* FieldWork::kindName(Kind kind, bool strong) {
    static const char* kNames[] = {"science", "medical", "engineering", "strong-science", "strong-medical",
                                   "strong-engineering"};
    return kNames[int(kind) + (strong ? 3 : 0)];
}

FieldWork::FieldWork(std::string_view mission, int playerSlot, World& world,
                     std::map<std::string, std::pair<int, int>> sites)
    : mission_(mission), player_(playerSlot), world_(world), sites_(std::move(sites)) {
    for (const Raid* r : raids(mission))
        if (r->side >= 0 && r->side <= 5) {
            Watch w;
            w.raid = r;
            w.condition = r->condition;
            w.side = r->side;
            watches_.push_back(std::move(w));
        }
    for (const SideInfo* s : slotsOf(mission))
        races_[s->slot] = s->race;
    for (const Officer* o : reluva::newworlds::officers(mission, playerSlot)) {
        if (o->train && o->skill < 1)
            courses_.push_back({officers_.size()});
        officers_.push_back({o, 0, o->skill});
    }
}

bool FieldWork::train(size_t course) {
    if (course >= courses_.size() || courses_[course].finished || courses_[course].running)
        return false;
    courses_[course].running = true;
    return true;
}

bool FieldWork::canScan(const Unit& u) { return u.name == "ground-scout" || u.name == "science-vehicle"; }
bool FieldWork::canCapture(const Unit& u) { return u.name == "armed-apc" || u.name == "cloaked-apc"; }
bool FieldWork::canCarry(const Unit& u) {
    return u.name == "science-vehicle" || u.name == "armed-apc" || u.name == "cloaked-apc" || u.name == "unarmed-apc";
}

void FieldWork::placeOfficers() {
    int hub = 0;
    for (const Unit& u : world_.units())
        if (u.slot == player_ && u.name == "colony-hub")
            hub = u.id;
    for (OfficerAt& o : officers_) {
        o.in = hub;
        for (const Unit& u : world_.units())
            if (u.slot == player_ && u.type == o.officer->building) {
                o.in = u.id;
                break;
            }
    }
}

std::vector<const Officer*> FieldWork::aboard(int vehicle) const {
    std::vector<const Officer*> out;
    for (const OfficerAt& o : officers_)
        if (o.in == vehicle)
            out.push_back(o.officer);
    return out;
}

int FieldWork::board(int vehicle) {
    const Unit* v = world_.unit(vehicle);
    if (!v || !canCarry(*v))
        return 0;
    const Unit* from = nullptr;
    float best = kOfficerReach + kBuildingSize;
    for (const Unit& u : world_.units())
        if (u.building && u.slot == player_ && std::hypot(u.x - v->x, u.y - v->y) <= best &&
            std::any_of(officers_.begin(), officers_.end(), [&](const OfficerAt& o) { return o.in == u.id; }))
            best = std::hypot(u.x - v->x, u.y - v->y), from = &u;
    if (!from)
        return 0;
    int moved = 0;
    for (OfficerAt& o : officers_)
        if (o.in == from->id && aboard(vehicle).size() < kOfficerSeats)
            o.in = vehicle, ++moved;
    return moved;
}

bool FieldWork::moveOfficer(size_t officer, int into) {
    const Unit* u = world_.unit(into);
    if (officer >= officers_.size() || officers_[officer].in < 0 || !u || u->slot != player_ ||
        (!u->building && !canCarry(*u)) || (!u->building && aboard(into).size() >= kOfficerSeats))
        return false;
    officers_[officer].in = into;
    return true;
}

void FieldWork::destroyed(int unit) {
    for (OfficerAt& o : officers_)
        if (o.in == unit && unit > 0)
            o.in = -1;
}

int FieldWork::disembark(int vehicle) {
    const Unit* v = world_.unit(vehicle);
    if (!v)
        return 0;
    const Unit* into = nullptr;
    float best = kOfficerReach + kBuildingSize;
    for (const Unit& u : world_.units())
        if (u.building && u.slot == player_ && std::hypot(u.x - v->x, u.y - v->y) <= best)
            best = std::hypot(u.x - v->x, u.y - v->y), into = &u;
    if (!into)
        return 0;
    int moved = 0;
    for (OfficerAt& o : officers_)
        if (o.in == vehicle)
            o.in = into->id, ++moved;
    return moved;
}

float FieldWork::skill(int unit, int speciality) const {
    float best = 0;
    for (const OfficerAt& o : officers_) {
        if (o.in != unit || unit <= 0)
            continue;
        float v = 0;
        if (o.officer->speciality == speciality)
            v = o.skill;
        else if (o.officer->speciality == 7 && (speciality == 1 || speciality == 2))
            v = std::max(o.skill - 0.25f, 0.25f);
        best = std::max(best, v);
    }
    return best;
}

bool FieldWork::strong(int unit, Kind kind) const {
    return skill(unit, kind == Science ? 4 : kind == Medical ? 3 : 1) >= kStrongSkill;
}

bool FieldWork::scan(int unit, int target, Kind kind) {
    const Unit* u = world_.unit(unit);
    const Unit* t = world_.unit(target);
    if (!u || !t || !canScan(*u) || unit == target || t->slot == player_)
        return false;
    cancel(unit);
    scans_[unit] = {target, kind, 0};
    return true;
}

bool FieldWork::capture(int unit, int target) {
    const Unit* u = world_.unit(unit);
    const Unit* t = world_.unit(target);
    if (!u || !t || !canCapture(*u) || !t->building || t->slot == player_ || holds_.count(target))
        return false;
    // One away team at a time.
    for (const auto& [id, c] : captures_)
        if (c.target == target && id != unit)
            return false;
    cancel(unit);
    captures_[unit] = {target};
    return true;
}

void FieldWork::cancel(int unit) {
    scans_.erase(unit);
    if (auto it = captures_.find(unit); it != captures_.end()) {
        if (it->second.state == 3)
            release(it->second.target);
        captures_.erase(it);
        if (Unit* u = world_.unit(unit))
            u->holdFire = false;
    }
}

void FieldWork::release(int building) {
    auto it = holds_.find(building);
    if (it == holds_.end())
        return;
    for (const auto& [apc, c] : captures_)
        if (c.target == building && c.state == 3)
            if (Unit* b = world_.unit(building))
                b->hp = std::min(c.savedHp, b->maxHp);
    if (Unit* b = world_.unit(building))
        b->heldBy = -1;
    holds_.erase(it);
}

int FieldWork::heldBy(int building) const {
    auto it = holds_.find(building);
    return it == holds_.end() ? -1 : it->second.slot;
}

std::string FieldWork::status(int unit) const {
    if (auto it = scans_.find(unit); it != scans_.end())
        return it->second.ticks > 0 ? "SCANNING" : "TO SCAN";
    if (auto it = captures_.find(unit); it != captures_.end())
        return it->second.state == 0 ? "ATTACKING" : it->second.state == 1 ? "TO CAPTURE" : it->second.state == 2 ? "AWAY TEAM IN" : "HOLDING";
    return {};
}

std::vector<std::pair<int, int>> FieldWork::scanning() const {
    std::vector<std::pair<int, int>> out;
    for (const auto& [id, s] : scans_)
        if (s.ticks > 0)
            out.push_back({id, s.target});
    return out;
}

float FieldWork::reach(const Unit& scanner, const Unit& target) const {
    const CombatType* t = combatType(scanner.type);
    return (t ? t->scan : 0) + (target.building ? kBuildingSize : kVehicleSize);
}

std::vector<std::string> FieldWork::namesOf(const Unit& u) const {
    std::vector<std::string> out = scriptNames(u, player_);
    for (const auto& [site, at] : sites_) {
        const float sx = float(at.second) / 8 * kBlock, sy = float(at.first) / 8 * kBlock;
        const float d = std::hypot(u.x - sx, u.y - sy);
        if (d > kSiteNear || std::find(out.begin(), out.end(), site) != out.end())
            continue;
        // Only the object nearest the site goes by its name (J3's workshops
        // stand round its core building's site).
        if (std::none_of(world_.units().begin(), world_.units().end(), [&](const Unit& v) {
                return v.id != u.id && v.building == u.building && std::hypot(v.x - sx, v.y - sy) < d;
            }))
            out.push_back(site);
    }
    return out;
}

bool FieldWork::conditionMet(const Watch& w) const {
    const std::string& label = w.raid ? w.raid->label : std::string();
    bool any = false;
    for (const Unit& u : world_.units()) {
        if (w.target ? u.id != w.target : u.label != label)
            continue;
        any = true;
        if (w.condition == 2) {
            if (heldBy(u.id) == w.side)
                return true;
        } else if (w.condition == 3) {
            // Its block marked scanned for the side in the fog (any scanner).
            if (fog_ && fog_->has(int(u.x / kBlock), int(u.y / kBlock), Fog::Scanned, w.side))
                return true;
        } else if (w.condition >= 4 && w.side == player_) {
            // (Only the player's units scan: the computer's sides never do.)
            if (auto it = scanned_.find(u.id); it != scanned_.end() && it->second[size_t(w.condition)] >= tick_ - 1)
                return true;
        }
    }
    // All destroyed: the group was on the map and none of it is left.
    return w.condition == 1 && w.seen && !any;
}

// A watch's success sends Go_Futile to the rivals' raids it names (as
// the mission wires it; most race-for-it raids name the other two): one
// under way is beaten (the player hears it), the rest will find their
// success futile.
void FieldWork::succeeded(const Watch& done, std::vector<ScriptEvent>& out) {
    if (!done.raid)
        return;
    for (Watch& w : watches_) {
        if (&w == &done || !w.raid ||
            std::none_of(std::begin(done.raid->futile), std::end(done.raid->futile),
                         [&](const char* n) { return *n && std::string_view(n) == w.raid->name; }))
            continue;
        if (w.started && !w.done) {
            w.done = true;
            if (w.side == player_)
                out.push_back({"beaten", {w.raid->label}});
        } else if (!w.done) {
            w.futile = true;
        }
    }
}

// The share of each side's colony the player's teams hold: small
// buildings count 1, large 3, out of the colony's size at the start.
void FieldWork::shares(std::vector<ScriptEvent>& out) {
    auto weight = [](const Unit& u) {
        const CombatType* t = combatType(u.type);
        return !t || !u.building || u.built < 1 ? 0 : (t->mask & 0x4) ? 3 : (t->mask & 0x2) ? 1 : 0;
    };
    if (colonyBase_.empty())
        for (const Unit& u : world_.units())
            if (u.heldBy < 0)
                colonyBase_[u.slot] += weight(u);
    // Told under every label the owner's buildings carry (a script names
    // the colony by any of them: J1's "taubat-colony").
    std::map<int, int> held;            // per owner: the weight the player holds
    std::map<std::string, int> owners;  // label: its owner
    for (const auto& [id, h] : holds_)
        if (const Unit* u = world_.unit(id); u && h.slot == player_)
            held[u->slot] += weight(*u);
    for (const Unit& u : world_.units())
        if (u.building && !u.label.empty() && held.count(u.slot))
            owners[u.label] = u.slot;
    for (const auto& [label, owner] : owners) {
        const int base = colonyBase_[owner];
        const int share = base > 0 ? held[owner] * 100 / base : 0;
        if (auto it = shared_.find(label); it == shared_.end() || it->second != share) {
            shared_[label] = share;
            out.push_back({"captured", {label, std::to_string(share)}});
        }
    }
}

std::vector<ScriptEvent> FieldWork::advance(double seconds, const std::function<bool(const ScriptEvent&)>& handled) {
    std::vector<ScriptEvent> out;
    clock_ += seconds;
    while (clock_ >= kTick) {
        clock_ -= kTick;
        tick(out, handled);
    }
    return out;
}

void FieldWork::tick(std::vector<ScriptEvent>& out, const std::function<bool(const ScriptEvent&)>& handled) {
    ++tick_;
    // Training: the points shared out between the courses running.
    int running = 0;
    for (const Course& c : courses_)
        running += c.running;
    for (Course& c : courses_)
        if (c.running && (c.done = std::min(kCoursePoints, c.done + kTrainingPerTick / running)) >= kCoursePoints) {
            c.running = false, c.finished = true;
            float& skill = officers_[c.officer].skill;
            skill = std::min(1.0f, skill + 0.25f);
        }
    // Scans: in range, a pulse on the target; out of range, drive closer.
    std::set<int> pulsed;
    for (auto it = scans_.begin(); it != scans_.end();) {
        Unit* u = world_.unit(it->first);
        const Unit* t = world_.unit(it->second.target);
        if (!u || !t) {
            it = scans_.erase(it);
            continue;
        }
        ScanOrder& s = it->second;
        if (std::hypot(t->x - u->x, t->y - u->y) < reach(*u, *t)) {
            u->path.clear();
            const int cond = conditionOf(s.kind, strong(u->id, s.kind));
            auto& marks = scanned_[t->id];
            if (marks[0] == 0)
                marks.fill(-10);
            marks[size_t(cond)] = tick_;
            ++s.ticks;
            pulsed.insert(t->id);
            // The game's reply on the scan's second tick in a row, once a
            // session: here only where no handler waits for the scan.
            if (++session_[t->id] == 2) {
                const std::string kind = scanName(cond);
                bool wanted = false;
                for (const std::string& n : namesOf(*t))
                    wanted = wanted || (handled && (handled({"scan-started", {kind, n}}) || handled({"scanned", {kind, n}})));
                if (!wanted)
                    out.push_back({"scanned-nothing", {kind}});
                // A scan none of the player's raids covers gets a watch of
                // its own, so the scripts' targets (their sites) still hear
                // of it. (A rival's raid on it doesn't hear the player.)
                const bool covered = std::any_of(watches_.begin(), watches_.end(), [&](const Watch& w) {
                    return w.raid && w.side == player_ && w.raid->condition == cond && w.raid->label == t->label &&
                           !t->label.empty();
                });
                const bool watched = std::any_of(watches_.begin(), watches_.end(), [&](const Watch& w) {
                    return !w.raid && w.target == t->id && w.condition == cond;
                });
                if (!covered && !watched && wanted) {
                    Watch w;
                    w.target = t->id;
                    w.condition = cond;
                    w.start = kOwnStart, w.hold = kOwnHold;
                    w.side = player_;
                    w.ticks = 2; // (its scan's two ticks so far; its names are taken on the next)
                    watches_.push_back(std::move(w));
                }
            }
        } else {
            s.ticks = 0;
            if (!u->moving())
                world_.setPath(u->id, world_.findPath(u->x, u->y, t->x, t->y));
        }
        ++it;
    }
    for (auto it = session_.begin(); it != session_.end();)
        it = pulsed.count(it->first) ? std::next(it) : session_.erase(it);

    // Captures.
    for (auto it = captures_.begin(); it != captures_.end();) {
        Unit* u = world_.unit(it->first);
        Unit* t = world_.unit(it->second.target);
        CaptureOrder& c = it->second;
        if (!t) {
            holds_.erase(c.target);
            if (u)
                u->holdFire = false;
            it = captures_.erase(it);
            continue;
        }
        if (!u) {
            // The APC lost: its team stays in (the game doesn't let go).
            it = captures_.erase(it);
            continue;
        }
        const float d = std::hypot(t->x - u->x, t->y - u->y);
        if (c.state == 0) {
            if (t->hp > t->maxHp * 0.8f) {
                if (u->target != t->id)
                    world_.attack({u->id}, t->id);
            } else {
                u->target = 0, u->ordered = false, u->firing = false, u->holdFire = true;
                c.state = 1;
            }
        }
        if (c.state == 1) {
            if (d <= kEntry) {
                u->path.clear();
                c.state = 2;
                c.timer = kTeamTicks;
            } else if (!u->moving()) {
                world_.setPath(u->id, world_.findPath(u->x, u->y, t->x, t->y));
            }
        } else if (c.state == 2 && --c.timer <= 0) {
            c.savedHp = t->hp;
            t->hp = t->maxHp;
            holds_[t->id] = {u->slot, u->id, 0};
            t->heldBy = u->slot;
            heldNames_[t->id] = namesOf(*t);
            c.state = 3;
            // A building no raid watches: taken at once.
            if (u->slot == player_ && std::none_of(watches_.begin(), watches_.end(), [&](const Watch& w) {
                    return w.raid && w.side == player_ && w.condition == 2 && !t->label.empty() &&
                           w.raid->label == t->label;
                })) {
                Watch w;
                w.target = t->id;
                w.condition = 2;
                w.side = player_;
                watches_.push_back(std::move(w));
            }
        }
        ++it;
    }
    // Held buildings: the script seconds held (kScriptSecond, as the
    // game's capture monitors count them).
    for (auto it = holds_.begin(); it != holds_.end();) {
        if (!world_.unit(it->first)) {
            it = holds_.erase(it);
            continue;
        }
        Hold& h = it->second;
        const int before = int(h.seconds / kScriptSecond);
        h.seconds += kTick;
        const int now = int(h.seconds / kScriptSecond);
        if (h.slot == player_ && now != before)
            for (const std::string& n : heldNames_[it->first])
                out.push_back({"held", {n, std::to_string(now)}});
        ++it;
    }

    // The watches: the condition for start ticks, then for hold ticks.
    shares(out);
    for (Watch& w : watches_) {
        if (w.condition == 1 && w.raid && !w.seen)
            w.seen = std::any_of(world_.units().begin(), world_.units().end(),
                                 [&](const Unit& u) { return u.label == w.raid->label; });
        const bool met = conditionMet(w);
        const double startS = (w.raid ? w.raid->start : w.start) / kScriptTicks;
        const double holdS = (w.raid ? w.raid->hold : w.hold) / kScriptTicks;
        const bool capture = w.condition == 2;
        auto send = [&](const std::string& kind) {
            // A rival's watch: only its success is heard, by its race.
            if (w.side != player_) {
                if (kind == "captured" || kind == "scanned")
                    for (const std::string& n : w.names)
                        out.push_back({kind + "-by", {raceName(races_[w.side]), n}});
                return;
            }
            // Ours after a rival's success: futile.
            if (w.futile && (kind == "captured" || kind == "scanned")) {
                for (const std::string& n : w.names)
                    out.push_back({"futile", {n}});
                return;
            }
            // (The player's "all destroyed" watch is heard as destroyed.)
            if (w.condition == 1)
                return;
            for (const std::string& n : w.names)
                out.push_back(capture ? ScriptEvent{kind, {n}} : ScriptEvent{kind, {scanName(w.condition), n}});
        };
        if (met) {
            if (w.ticks == 0 || w.names.empty()) {
                // Who it's about: the first of the group met.
                w.names.clear();
                if (w.raid)
                    w.names.push_back(w.raid->label);
                for (const Unit& u : world_.units())
                    if (w.target ? u.id == w.target : u.label == (w.raid ? w.raid->label : "")) {
                        for (const std::string& n : capture && heldNames_.count(u.id) ? heldNames_[u.id] : namesOf(u))
                            if (std::find(w.names.begin(), w.names.end(), n) == w.names.end())
                                w.names.push_back(n);
                        break;
                    }
            }
            ++w.ticks;
            const double held = w.ticks * kTick;
            if (!w.started && held >= startS) {
                w.started = true;
                send(capture ? "entered" : "scan-started");
            }
            if (w.started && !w.done && held >= startS + holdS) {
                w.done = true;
                send(capture || w.condition == 1 ? "captured" : "scanned");
                succeeded(w, out);
            }
        } else if (w.ticks > 0) {
            if (w.started && !w.done)
                send(capture ? "capture-lost" : "scan-lost");
            else if (w.done && capture)
                send("vacated");
            w.ticks = 0;
            w.started = false;
            if (capture)
                w.done = false; // a building can be taken again
        }
    }
    // A scan's own watch ends with it.
    watches_.erase(std::remove_if(watches_.begin(), watches_.end(), [](const Watch& w) { return !w.raid && w.ticks == 0; }),
                   watches_.end());
}

const char* rankTitle(char race, int rank) {
    static const char* const kFed[] = {"Ens.", "Lt.", "Lt.Cdr.", "Cdr.", "Capt.", "Comm.", "Adm.", "Dr."};
    static const char* const kKli[] = {"Mang.", "Sog.'", "La'", "Hod", "Tlh.", "'aj.", "Adm.", "Qel."};
    static const char* const kRom[] = {"Lgn.", "Trib.", "Cnsl.", "Sntr.", "Gen.", "Imp.", "Adm.", "Srgn."};
    if (rank < 1 || rank > 8)
        return "";
    return (race == 'K' ? kKli : race == 'R' ? kRom : kFed)[rank - 1];
}

const char* specialityName(int speciality) {
    static const char* const kNames[] = {"ENGINEERING", "SECURITY", "MEDICAL",    "SCIENCE",  "INTELLIGENCE",
                                         "MINING",      "OPERATIONS", "LOGISTICS", "PERSONNEL"};
    return speciality >= 1 && speciality <= 9 ? kNames[speciality - 1] : "";
}

const char* skillWord(float skill) {
    static const char* const kWords[] = {"UNTRAINED", "BASIC", "SKILLED", "HIGHLY SKILLED", "EXPERT"};
    return kWords[std::clamp(int(skill * 4), 0, 4)];
}

void FieldWork::syncState(save::Serializer& s) {
    s(sites_)(scans_)(captures_)(holds_);
    // The officers, each by its slot and place in the mission's list.
    const uint32_t n = s.count(officers_.size());
    if (s.loading())
        officers_.assign(n, OfficerAt{nullptr, 0, 0});
    for (OfficerAt& o : officers_) {
        int slot = o.officer ? o.officer->slot : -1;
        s(slot);
        save::syncIndex(s, o.officer, newworlds::officers(mission_, slot));
        s(o.in)(o.skill);
    }
    s(courses_)(scanned_)(session_);
    const std::vector<const Raid*> raidList = raids(mission_);
    const uint32_t w = s.count(watches_.size());
    if (s.loading())
        watches_.assign(w, Watch{});
    for (Watch& v : watches_) {
        save::syncIndex(s, v.raid, raidList);
        s(v.target)(v.condition)(v.start)(v.hold)(v.ticks)(v.started)(v.done)(v.side)(v.futile)(v.seen)(v.names);
    }
    s(pending_)(heldNames_)(races_)(colonyBase_)(shared_)(clock_)(tick_);
}

} // namespace reluva::newworlds
