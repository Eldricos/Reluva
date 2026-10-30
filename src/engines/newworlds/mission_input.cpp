// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/mission_ui.h"
#include "engines/newworlds/viewers.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace reluva::newworlds {

void MissionViewer::selectUnits(const std::vector<int>& ids) {
    world_->select(ids);
    play_.selectedBuilding = play_.razeAsked = play_.cancelAsked = play_.cancelFrom = 0;
    play_.colonyDetails = false;
}

// The selection's order on another side's unit or building: the field
// work chosen (S, M, E, C), else a scout or science vehicle scans it (the
// game's default for them: a science scan) and the rest attack it.
void MissionViewer::orderOn(int target) {
    std::vector<int> attackers;
    for (int id : world_->selection()) {
        const Unit* u = world_->unit(id);
        if (!u)
            continue;
        if (fieldWork_)
            fieldWork_->cancel(id);
        const FieldWork::Kind kind = play_.pendingOrder == 'm'   ? FieldWork::Medical
                                     : play_.pendingOrder == 'e' ? FieldWork::Engineering
                                                            : FieldWork::Science;
        if (fieldWork_ && play_.pendingOrder == 'c' && FieldWork::canCapture(*u) && fieldWork_->capture(id, target))
            continue;
        if (fieldWork_ && play_.pendingOrder != 'c' && FieldWork::canScan(*u) && fieldWork_->scan(id, target, kind))
            continue;
        attackers.push_back(id);
    }
    // Attacks only on what the player is hostile to.
    const Unit* t = world_->unit(target);
    if (t && world_->stance(playerSlot_, t->slot) != 3)
        attackers.clear();
    if (!attackers.empty() && play_.pendingOrder == 'a')
        world_->assault(attackers, target);
    else if (!attackers.empty() && play_.pendingOrder != 's' && play_.pendingOrder != 'm' && play_.pendingOrder != 'e')
        world_->attack(attackers, target);
    play_.pendingOrder = 0;
}

bool MissionViewer::playEvent(const Event& ev) {
    if (ev.type == EventType::MouseMove || ev.type == EventType::MouseDown || ev.type == EventType::MouseUp)
        mouseX_ = ev.mouseX, mouseY_ = ev.mouseY, mouseSeen_ = true;
    const bool ctrl = (ev.mods & keymod::Ctrl) != 0, shift = (ev.mods & keymod::Shift) != 0,
               alt = (ev.mods & keymod::Alt) != 0;
    // Pause (and the window losing the focus) freezes the mission: no
    // time passes and nothing on the map can be touched; nothing says so
    // (exe.md#keys).
    if ((ev.type == EventType::KeyDown && ev.key == key::Pause) || ev.type == EventType::FocusLost ||
        ev.type == EventType::FocusGained) {
        if (ev.type == EventType::KeyDown)
            play_.paused = !play_.paused;
        else
            unfocused_ = ev.type == EventType::FocusLost;
        play_.boxing = false;
        return true;
    }
    if (play_.paused || unfocused_)
        return true;
    // Escape's box: quit the mission, yes or no; the mission goes on
    // meanwhile.
    if (play_.quitAsked) {
        bool yes = false, no = false;
        if (ev.type == EventType::KeyDown)
            yes = ev.key == 'y' || ev.key == key::Return, no = ev.key == 'n' || ev.key == key::Escape;
        else if (ev.type == EventType::MouseDown && ev.button == MouseButton::Left)
            yes = quitYes_.contains(ev.mouseX, ev.mouseY), no = quitNo_.contains(ev.mouseX, ev.mouseY);
        if (yes) {
            play_.quitAsked = false;
            if (flow_ == Flow::Playing)
                endFlow();
            else
                stopPlay();
            return true;
        }
        if (no)
            play_.quitAsked = false, redraw();
        if (yes || no || ev.type == EventType::KeyDown || ev.type == EventType::MouseDown)
            return true;
    }
    // The tutorials wait for keys, by the game's (Windows) key codes.
    if (ev.type == EventType::KeyDown && run_ && !run_->finished()) {
        int code = 0;
        if (ev.key == key::Tab)
            code = 9;
        else if (ev.key == key::Up || ev.key == key::Down || ev.key == key::Left || ev.key == key::Right)
            code = ev.key == key::Left ? 37 : ev.key == key::Up ? 38 : ev.key == key::Right ? 39 : 40;
        else if (ev.key >= key::F1 && ev.key < key::F1 + 12)
            code = 112 + int(ev.key - key::F1);
        else if (ev.key >= key::Kp0 && ev.key <= key::Kp0 + 9)
            code = 96 + int(ev.key - key::Kp0);
        else if (ev.key == key::LCtrl || ev.key == key::RCtrl)
            code = 17;
        else if (ev.key >= '0' && ev.key <= '9')
            code = int(ev.key);
        else if (ev.key == key::Home)
            code = 36;
        if (code) {
            const ScriptEvent k{"key", {std::to_string(code)}};
            deliver(k, run_->fire(k));
        }
    }
    switch (ev.type) {
    case EventType::KeyDown:
        if (ev.key == key::Escape && play_.trainingView) {
            play_.trainingView = false;
            break;
        }
        if (ev.key == key::Escape && (!play_.playbook.empty() || play_.staffView || play_.pickedOfficer >= 0)) {
            play_.playbook.clear();
            play_.staffView = false;
            play_.pickedOfficer = -1;
            break;
        }
        if (ev.key == key::Escape && (play_.researchMenu || play_.vehicleMenu)) {
            play_.researchMenu = play_.vehicleMenu = false;
            break;
        }
        if (ev.key == key::Escape && (play_.buildMenu || !play_.placing.empty())) {
            play_.buildMenu = false;
            play_.placing.clear();
            map_.setGhost({}, 0, 0, false);
            break;
        }
        if (ev.key == key::Escape) {
            play_.quitAsked = true;
            break;
        }
        // R: the research topics open to the colony; a number starts one.
        if (ev.key == 'r' && research_) {
            play_.researchMenu = !play_.researchMenu;
            play_.buildMenu = play_.vehicleMenu = false;
            break;
        }
        // V: the vehicles the yards can make (the game's vehicle menu); a
        // number orders one.
        if (ev.key == 'v' && !shift && !ctrl && !alt && economy_) {
            play_.vehicleMenu = !play_.vehicleMenu;
            play_.menuPage = 0;
            play_.buildMenu = play_.researchMenu = false;
            break;
        }
        if (play_.vehicleMenu && economy_ && ev.key >= '0' && ev.key <= '9') {
            const size_t n = ev.key == '0' ? 9 : size_t(ev.key - '1');
            const auto menu = vehicleMenu();
            if (n < menu.size())
                economy_->orderVehicle(*menu[n]);
            break;
        }
        if (play_.researchMenu && research_ && ev.key >= '1' && ev.key <= '9') {
            const auto offered = research_->offered();
            if (size_t(ev.key - '1') < offered.size())
                research_->start(offered[size_t(ev.key - '1')]);
            break;
        }
        // Shift or Ctrl + B / V: the camera to the next of the colony's
        // buildings / fighting vehicles (exe.md#keys).
        if ((ev.key == 'b' || ev.key == 'v') && (shift || ctrl)) {
            cycleCamera(ev.key == 'b');
            break;
        }
        // C: the colony's details (the hub's INFO page).
        if (ev.key == 'c' && !alt && !ctrl) {
            for (size_t i = 0; economy_ && i < colonyObjects().size() && i < structureIds().size(); ++i)
                if (const ColonyStructure* c = structureOf(colonyObjects()[i]); c && c->type == "colony-hub") {
                    world_->clearSelection();
                    if (play_.selectedBuilding != colonyObjects()[i])
                        play_.razeAsked = play_.cancelAsked = play_.cancelFrom = 0;
                    play_.selectedBuilding = colonyObjects()[i];
                    play_.colonyDetails = true;
                    play_.buildMenu = play_.vehicleMenu = play_.researchMenu = false;
                    break;
                }
            break;
        }
        // A, G, S, T, P: all the colony's APCs, ground scouts, science
        // vehicles, tanks, photon artillery selected.
        if (!alt && !ctrl && (ev.key == 'a' || ev.key == 'g' || ev.key == 's' || ev.key == 't' || ev.key == 'p')) {
            selectAllOfKind(char(ev.key));
            break;
        }
        if (ev.key == 'b') {
            play_.researchMenu = play_.vehicleMenu = false;
            play_.buildMenu = !play_.buildMenu;
            play_.menuPage = 0;
            play_.placing.clear();
            map_.setGhost({}, 0, 0, false);
            break;
        }
        if (play_.buildMenu && ev.key >= '0' && ev.key <= '9') {
            // 1-9, then 0 for the tenth.
            const size_t n = ev.key == '0' ? 9 : size_t(ev.key - '1');
            const auto menu = buildMenu();
            if (n < menu.size())
                choose(*menu[n]);
            break;
        }
        if (ev.key == key::Tab) {
            play_.tricorderFull = !play_.tricorderFull;
            break;
        }
        // Space: the camera to the next of the colony's objects lately hit
        // (the game's alerts), in their order, round again.
        if (ev.key == ' ' && world_) {
            std::vector<const Unit*> alerted;
            for (const Unit& u : world_->units())
                if (u.slot == playerSlot_ && u.hp > 0 && sim_->alerted(u.id))
                    alerted.push_back(&u);
            if (!alerted.empty()) {
                size_t next = 0;
                for (size_t i = 0; i < alerted.size(); ++i)
                    if (alerted[i]->id == play_.alertedLast)
                        next = (i + 1) % alerted.size();
                play_.alertedLast = alerted[next]->id;
                cameraTo(*alerted[next]);
            }
            break;
        }
        if (ev.key == key::Home) {
            float x = 0, y = 0;
            if (hubPosition(x, y))
                map_.centreOn(x, y);
            break;
        }
        // Battle groups 1-5, as the game's (exe.md#keys).
        if (ev.key >= '1' && ev.key <= '5') {
            const int n = int(ev.key - '0');
            float x = 0, y = 0;
            if (ctrl)
                world_->setGroup(n);
            else if (shift && world_->groupCentre(n, x, y))
                map_.centreOn(x, y);
            else if (!shift && world_->selectGroup(n))
                selectUnits(world_->selection());
            showUnits();
            break;
        }
        if (ev.key == 'i') {
            interface_ = !interface_;
            break;
        }
        // Field work, for the selection (Reluva's keys for the playbook's
        // orders, under Alt, the game's own letters being taken): Alt+S,
        // M, E a science, medical or engineering scan and Alt+C a capture
        // (on the next unit or building clicked); O takes aboard the
        // officers of the building beside it (Shift+O puts them down).
        // Alt+A attack en route (on the ground) or assault (on a target),
        // Alt+P protect a spot (or, mobile shield generators, the building
        // clicked), Y back to the rally point.
        if (alt && (ev.key == 's' || ev.key == 'm' || ev.key == 'e' || ev.key == 'c' || ev.key == 'a' || ev.key == 'p') &&
            !world_->selection().empty()) {
            play_.pendingOrder = play_.pendingOrder == char(ev.key) ? 0 : char(ev.key);
            break;
        }
        // Ctrl+X: the script's walkthrough a step on (a testing aid, as X
        // is on the missions page).
        if (ev.key == 'x' && ctrl && run_) {
            if (walk_.empty())
                walk_ = run_->walkthrough(), walkPos_ = 0;
            stepScript();
            break;
        }
        if (ev.key == 'y' && !world_->selection().empty()) {
            rallySelected();
            break;
        }
        if (ev.key == 'o' && fieldWork_) {
            int moved = 0;
            for (int id : world_->selection())
                moved += shift ? fieldWork_->disembark(id) : fieldWork_->board(id);
            walkNote_ = std::to_string(moved) + (shift ? " officers put down" : " officers aboard");
            break;
        }
        if (map_.gameKey(ev.key, mouseX_, mouseY_))
            break;
        return true; // the data viewer's keys don't apply in play
    case EventType::MouseDown: {
        // The panels are over the map, and the full-screen Tricorder over
        // them: what's under them isn't clicked.
        const int panel = panelAt(ev.mouseX, ev.mouseY);
        if (!play_.placing.empty() && !ctrl &&
            (ev.button == MouseButton::Right ||
             (ev.button == MouseButton::Left && panel < 0 && !play_.tricorderFull))) {
            // Left click on the map builds where the hologram is green;
            // right click puts it away.
            if (ev.button == MouseButton::Left)
                updateGhost(), placeBuilding();
            else {
                play_.placing.clear();
                map_.setGhost({}, 0, 0, false);
            }
            break;
        }
        // The playbook open: a click on one of its icons gives that order;
        // any other click (or the right button) puts it away.
        if (!play_.playbook.empty()) {
            for (size_t i = 0; ev.button == MouseButton::Left && i < play_.playbook.size(); ++i)
                if (playbookIcon(int(i)).contains(ev.mouseX, ev.mouseY)) {
                    playbookOrder(play_.playbook[i].order);
                    break;
                }
            play_.playbook.clear();
            redraw();
            break;
        }
        // A click on a Tricorder's map (a left click, or a double right
        // click) takes the camera there. The full-screen one takes every
        // click: the darkened panels and the hidden map under it aren't
        // there to click.
        {
            const MapViewer::Relief relief = tricorderRelief();
            float wx = 0, wy = 0;
            const bool onMap = (play_.tricorderFull || panel >= 0) && relief.w > 0 &&
                               Rect{relief.x, relief.y, relief.w, relief.h}.contains(ev.mouseX, ev.mouseY) &&
                               map_.reliefToWorld(relief, ev.mouseX, ev.mouseY, wx, wy);
            if (onMap && !ctrl &&
                (ev.button == MouseButton::Left || (ev.button == MouseButton::Right && ev.clicks >= 2))) {
                map_.stopFollowing();
                play_.followed = 0;
                map_.centreOn(wx, wy);
                break;
            }
            if (play_.tricorderFull)
                break;
        }
        // A panel: its buttons, the topmost first; the rest of it covers
        // the map.
        if (panel >= 0) {
            if (ev.button == MouseButton::Left || (ev.button == MouseButton::Right && !ctrl))
                pressButton(ev.mouseX, ev.mouseY, ev.button == MouseButton::Right, ev.clicks);
            break;
        }
        if (ev.button == MouseButton::Left && ctrl) {
            map_.zoomAt(ev.mouseX, ev.mouseY, true);
        } else if (ev.button == MouseButton::Right && ctrl) {
            map_.zoomAt(ev.mouseX, ev.mouseY, false);
        } else if (ev.button == MouseButton::Right && ev.clicks >= 2) {
            // A double right click takes the camera to the ground there, on
            // the map or the Tricorder (above; exe.md#cameras).
            float wx = 0, wy = 0;
            if (map_.screenToGround(ev.mouseX, ev.mouseY, wx, wy)) {
                map_.stopFollowing();
                play_.followed = 0;
                map_.centreOn(wx, wy);
            }
        } else if (ev.button == MouseButton::Right) {
            // Right-click drops the selection, or the camera's lock.
            if (map_.locked())
                map_.unlock();
            else
                selectUnits({});
            showUnits();
        } else if (ev.button == MouseButton::Left) {
            play_.boxing = true;
            boxX_ = ev.mouseX, boxY_ = ev.mouseY;
            return true;
        }
        break;
    }
    case EventType::MouseMove:
        if (!play_.placing.empty())
            updateGhost();
        // What the panels show under the pointer (the menu's item, the
        // Tricorder's readout, the playbook's names) follows it.
        if (!play_.boxing) {
            if (interface_ || !play_.playbook.empty())
                redraw();
            return true;
        }
        break;
    case EventType::MouseUp: {
        if (ev.button != MouseButton::Left || !play_.boxing)
            return true;
        play_.boxing = false;
        const int x0 = std::min(boxX_, ev.mouseX), x1 = std::max(boxX_, ev.mouseX);
        const int y0 = std::min(boxY_, ev.mouseY), y1 = std::max(boxY_, ev.mouseY);
        std::vector<int> picked;
        if (x1 - x0 > 6 || y1 - y0 > 6) {
            // A box: the side's units inside it.
            for (const Unit& u : world_->units()) {
                int sx = 0, sy = 0;
                if (u.slot == playerSlot_ && !u.building &&
                    map_.worldToScreen(u.x, u.y, map_.groundAtWorld(u.x, u.y), sx, sy) &&
                    sx >= x0 && sx <= x1 && sy >= y0 && sy <= y1)
                    picked.push_back(u.id);
            }
            selectUnits(picked);
        } else {
            // A click: the nearest unit to it, if close; else, with units
            // selected, they go there; a double click on the ground locks
            // the camera on it.
            // With units selected, a click on another side's unit or
            // building sends them to attack it.
            int best = -1, bestD = 18 * 18, other = -1, otherD = 18 * 18;
            for (const Unit& u : world_->units()) {
                int sx = 0, sy = 0;
                if (!map_.worldToScreen(u.x, u.y, map_.groundAtWorld(u.x, u.y) + (u.building ? 1500 : 300), sx, sy))
                    continue;
                const int d = (sx - ev.mouseX) * (sx - ev.mouseX) + (sy - ev.mouseY) * (sy - ev.mouseY);
                if (u.slot == playerSlot_ && !u.building && d < bestD)
                    best = u.id, bestD = d;
                else if (u.slot != playerSlot_ && visible(u) && d < (u.building ? 2 : 1) * otherD)
                    other = u.id, otherD = u.building ? d / 2 : d;
            }
            float gx = 0, gy = 0;
            // Protecting one of the colony's buildings: mobile shield
            // generators guard it (the game's "protect area"), the rest
            // hold the spot.
            int ownBuilding = -1, ownD = 30 * 30;
            if (play_.pendingOrder == 'p')
                for (const Unit& u : world_->units()) {
                    int sx = 0, sy = 0;
                    if (u.slot != playerSlot_ || !u.building ||
                        !map_.worldToScreen(u.x, u.y, map_.groundAtWorld(u.x, u.y) + 1500, sx, sy))
                        continue;
                    const int d = (sx - ev.mouseX) * (sx - ev.mouseX) + (sy - ev.mouseY) * (sy - ev.mouseY);
                    if (d < ownD)
                        ownBuilding = u.id, ownD = d;
                }
            // A building clicked with nothing to order: selected, its
            // details in the panel (the colony's, or another side's seen).
            int building = -1, buildingD = 30 * 30;
            if (!play_.pendingOrder)
                for (const Unit& u : world_->units()) {
                    int sx = 0, sy = 0;
                    if (!u.building || u.kind.model.empty() || (u.slot != playerSlot_ && !visible(u)) ||
                        !map_.worldToScreen(u.x, u.y, map_.groundAtWorld(u.x, u.y) + 1500, sx, sy))
                        continue;
                    const int d = (sx - ev.mouseX) * (sx - ev.mouseX) + (sy - ev.mouseY) * (sy - ev.mouseY);
                    if (d < buildingD)
                        building = u.id, buildingD = d;
                }
            // An officer picked on the STAFF panel: the building or vehicle
            // clicked takes them.
            if (play_.pickedOfficer >= 0 && fieldWork_) {
                const int into = best >= 0 ? best
                                 : building >= 0 && world_->unit(building)->slot == playerSlot_ ? building
                                                                                                 : -1;
                if (into >= 0) {
                    walkNote_ = fieldWork_->moveOfficer(size_t(play_.pickedOfficer), into) ? "officer moved"
                                                                                        : "can't take an officer";
                    play_.pickedOfficer = -1;
                    showUnits();
                    break;
                }
            }
            // A double click with units selected: the playbook, for what
            // was clicked (another side's object, one of the colony's
            // buildings, or the ground).
            if (ev.clicks >= 2 && !world_->selection().empty() && best < 0) {
                float px = 0, py = 0;
                map_.screenToGround(ev.mouseX, ev.mouseY, px, py);
                const int target = other >= 0 ? other
                                   : building >= 0 && world_->unit(building)->slot == playerSlot_ ? building
                                                                                                   : 0;
                openPlaybook(target, px, py);
                break;
            }
            const bool ordering = !world_->selection().empty() && other >= 0 && building == other;
            if (building >= 0 && best < 0 && !ordering && world_->selection().empty()) {
                world_->clearSelection();
                if (building != play_.selectedBuilding) // (a question is dropped with its building)
                    play_.razeAsked = play_.cancelAsked = play_.cancelFrom = 0;
                play_.selectedBuilding = building;
                if (ev.clicks >= 2)
                    doubleClicked(building);
            } else if (best >= 0 && ev.clicks >= 2 && (other < 0 || otherD >= bestD)) {
                selectUnits({best});
                doubleClicked(best);
            } else if (ownBuilding >= 0 && !world_->selection().empty()) {
                const Unit& b = *world_->unit(ownBuilding);
                const std::vector<int> chosen = world_->selection();
                std::vector<int> shields, others;
                for (int id : chosen)
                    if (const Unit* u = world_->unit(id))
                        (u->name == "mobile-shield-generator" ? shields : others).push_back(id);
                world_->protect(shields, {ownBuilding});
                world_->select(others);
                world_->moveSelected(b.x, b.y - 1.2f * kTile, UnitOrder::Protect);
                world_->protect(others);
                world_->select(chosen);
                play_.pendingOrder = 0;
                orderSpoken(false);
            } else if (other >= 0 && !world_->selection().empty() && (best < 0 || otherD < bestD)) {
                orderOn(other);
                orderSpoken(false);
            }
            else if (best >= 0)
                selectUnits({best});
            else if (map_.screenToGround(ev.mouseX, ev.mouseY, gx, gy)) {
                if (!world_->selection().empty()) {
                    for (int id : world_->selection())
                        if (fieldWork_)
                            fieldWork_->cancel(id);
                    // A move fires on the way (cloaking units hold fire);
                    // attack en route fires, cloaked or not; protect holds
                    // the spot.
                    world_->moveSelected(gx, gy, play_.pendingOrder == 'a'   ? UnitOrder::EnRoute
                                                 : play_.pendingOrder == 'p' ? UnitOrder::Protect
                                                                        : UnitOrder::Move);
                    if (play_.pendingOrder == 'p')
                        world_->protect(world_->selection());
                    play_.pendingOrder = 0;
                    orderSpoken(true);
                }
                else if (ev.clicks >= 2)
                    map_.lockOn(gx, gy);
            }
        }
        showUnits();
        break;
    }
    case EventType::MouseWheel: map_.handleEvent(ev); return true;
    default: return true;
    }
    redraw();
    updateTitle();
    return true;
}

// The playbook a double click opens (exe.md#orders): for another side's
// object, the attack orders (and capture, for armed APCs on a building),
// or the scans when the selection is all scouts and science vehicles; for
// one of the colony's buildings with mobile shield generators selected,
// protect area; otherwise, on the ground, the move orders.
void MissionViewer::openPlaybook(int target, float gx, float gy) {
    play_.playbook.clear();
    const std::vector<int> sel = world_->selection();
    if (sel.empty())
        return;
    bool scanners = true, apcs = false, shields = false;
    for (int id : sel)
        if (const Unit* u = world_->unit(id)) {
            scanners = scanners && FieldWork::canScan(*u);
            apcs = apcs || FieldWork::canCapture(*u);
            shields = shields || u->name == "mobile-shield-generator";
        }
    const Unit* t = target ? world_->unit(target) : nullptr;
    // By the player's posture towards it (exe.md#scans-and-captures): on a
    // hostile one, scans (scouts and science vehicles) or attacks, and a
    // building's capture (armed APCs); on a neutral one only the scans; on
    // a friendly one nothing (as the ground). Props offer nothing.
    const int posture = t && t->slot != playerSlot_ ? world_->stance(playerSlot_, t->slot) : 1;
    const CombatType* ct = t ? combatType(t->type) : nullptr;
    if (t && ct && (ct->mask & 0x800000))
        t = nullptr;
    if (t && t->slot != playerSlot_ && (posture == 3 || (posture == 2 && scanners))) {
        if (scanners)
            play_.playbook = {{"science", "SCIENCE SCAN", 's'},
                              {"medical", "MEDICAL SCAN", 'm'},
                              {"engine", "ENGINEERING SCAN", 'e'}};
        else
            play_.playbook = {{"pbattack", "FOLLOW AND ATTACK ENEMY", 'f'},
                              {"pbbombrd", "ASSAULT ENEMY/LOCATION", 'b'}};
        if (!scanners && apcs && t->building && posture == 3)
            play_.playbook.push_back({"pbcaptre", "CAPTURE BUILDING", 'c'});
    } else if (t && t->building && t->slot == playerSlot_ && shields) {
        play_.playbook = {{"pbprohre", "PROTECT AREA", 'a'}};
    } else {
        t = nullptr;
        play_.playbook = {{"pbdefend", "ATTACK EN ROUTE", 'r'},
                     {"pbprotct", "PROTECT FROM LOCATION", 'l'},
                     {"pbmove", "MOVE WITHOUT ATTACKING", 'v'},
                     {"pbreturn", "RETURN TO VEHICLE RALLY POINT", 'y'}};
    }
    play_.playbookTarget = t ? t->id : 0;
    play_.playbookX = gx, play_.playbookY = gy;
}

void MissionViewer::playbookOrder(char order) {
    const int target = play_.playbookTarget;
    play_.playbook.clear();
    if (!world_ || world_->selection().empty())
        return;
    switch (order) {
    case 'f':
    case 'b':
    case 'c':
    case 's':
    case 'm':
    case 'e':
        if (world_->unit(target)) {
            play_.pendingOrder = order == 'f' ? 0 : order == 'b' ? 'a' : order;
            orderOn(target);
        }
        break;
    case 'a': {
        std::vector<int> shields;
        for (int id : world_->selection())
            if (const Unit* u = world_->unit(id); u && u->name == "mobile-shield-generator")
                shields.push_back(id);
        world_->protect(shields, {target});
        break;
    }
    case 'y': rallySelected(); break;
    default:
        for (int id : world_->selection())
            if (fieldWork_)
                fieldWork_->cancel(id);
        world_->moveSelected(play_.playbookX, play_.playbookY, order == 'r'   ? UnitOrder::EnRoute
                                                     : order == 'l' ? UnitOrder::Protect
                                                                    : UnitOrder::Move);
        if (order == 'l')
            world_->protect(world_->selection());
    }
    showUnits();
}

// The selected armed vehicles back to the rally point.
void MissionViewer::rallySelected() {
    float hx = 0, hy = 0;
    if (!hubPosition(hx, hy))
        return;
    for (int id : world_->selection())
        if (const Unit* u = world_->unit(id); u && u->weapon.kind != WeaponKind::None) {
            const auto [rx, ry] = rallySpot(hx, hy, play_.rallied++);
            const auto [px, py] = world_->openSpot(rx, ry);
            world_->rally(id, px, py);
        }
}

int MissionViewer::panelAt(int x, int y) const {
    if (!interface_)
        return -1;
    for (size_t i = play_.panels.size(); i-- > 0;)
        if (const PanelArea& p = play_.panels[i]; x >= p.x && y >= p.y && x < p.x + p.w && y < p.y + p.h)
            return int(i);
    return -1;
}

// A click on a panel's button: true if one was hit. The buttons drawn last
// are on top: those of the topmost panel under the click, or of panels
// drawn after it; a panel's own covers those under it.
bool MissionViewer::pressButton(int x, int y, bool right, int clicks) {
    const int panel = panelAt(x, y);
    const size_t first = panel < 0 ? 0 : play_.panels[size_t(panel)].firstButton;
    for (size_t i = play_.buttons.size(); i-- > first;) {
        const PanelButton& b = play_.buttons[i];
        if (x < b.x || y < b.y || x >= b.x + b.w || y >= b.y + b.h)
            continue;
        const bool entry = b.action.rfind("msel", 0) == 0 && b.action.size() > 4 && std::isdigit(uint8_t(b.action[4]));
        if (right && !entry)
            return false;
        const Unit* u = play_.selectedBuilding && world_ ? world_->unit(play_.selectedBuilding) : nullptr;
        if (entry && world_) {
            // A MULTISELECTION entry: right click ticks it; a click selects
            // it alone; a double click is a double click on it.
            const int id = std::atoi(b.action.c_str() + 4);
            if (right) {
                if (!play_.multiTicked.erase(id))
                    play_.multiTicked.insert(id);
            } else if (clicks >= 2) {
                doubleClicked(id);
            } else {
                selectUnits({id});
            }
        } else if (b.action == "msel-delete" && world_) {
            std::vector<int> kept;
            for (int id : world_->selection())
                if (!play_.multiTicked.count(id))
                    kept.push_back(id);
            world_->select(kept);
            play_.multiTicked.clear();
        } else if (b.action == "msel-invert" && world_) {
            std::set<int> flipped;
            for (int id : world_->selection())
                if (!play_.multiTicked.count(id))
                    flipped.insert(id);
            play_.multiTicked = std::move(flipped);
        } else if (b.action == "msel-prev" || b.action == "msel-next") {
            play_.multiPage += b.action == "msel-next" ? 1 : -1;
        } else if (b.action == "staff") {
            play_.staffView = !play_.staffView;
            play_.trainingView = false;
            play_.pickedOfficer = -1;
        } else if (b.action == "training") {
            play_.trainingView = true;
        } else if (b.action.rfind("course", 0) == 0 && fieldWork_) {
            fieldWork_->train(size_t(std::atoi(b.action.c_str() + 6)));
        } else if (b.action.rfind("menu-item", 0) == 0) {
            const size_t n = size_t(std::atoi(b.action.c_str() + 9));
            play_.menuFlash = {int(n), nowMs_};
            if (play_.buildMenu) {
                const auto menu = buildMenu();
                if (n < menu.size())
                    choose(*menu[n]);
            } else if (play_.vehicleMenu && economy_) {
                const auto menu = vehicleMenu();
                if (n < menu.size())
                    economy_->orderVehicle(*menu[n]);
            }
        } else if (b.action == "menu-prev" || b.action == "menu-next") {
            play_.menuPage += b.action == "menu-next" ? 1 : -1;
        } else if (b.action == "menu-close") {
            play_.buildMenu = play_.vehicleMenu = false;
        } else if (b.action.rfind("comms-", 0) == 0) {
            const int n = int(log_.size());
            const int shown = play_.commsShown < 0 || play_.commsShown >= n ? n - 1 : play_.commsShown;
            if (b.action == "comms-delete" && n > 0) {
                log_.erase(log_.begin() + shown);
                play_.commsShown = std::min(shown, int(log_.size()) - 1);
            } else if (b.action == "comms-first" && n > 0) {
                play_.commsShown = 0;
            } else if (b.action == "comms-prev" && n > 0) {
                play_.commsShown = std::max(0, shown - 1);
            } else if (b.action == "comms-next" && n > 0) {
                play_.commsShown = std::min(n - 1, shown + 1);
            } else if (b.action == "comms-last") {
                play_.commsShown = -1;
            } else if (b.action == "comms-mission") {
                play_.commandShown = !play_.commandShown;
            } else if (b.action == "comms-close") {
                play_.commsClosed = true;
            }
        } else if (b.action == "tri-comms") {
            play_.commsClosed = false;
        } else if (b.action == "tri-zoom") {
            tricorderZoom_ = (tricorderZoom_ + 1) % 3;
        } else if (b.action == "tri-ally") {
            play_.allyOpen = !play_.allyOpen, play_.allyPick = -1;
        } else if (b.action.rfind("ally-race", 0) == 0) {
            const int slot = std::atoi(b.action.c_str() + 9);
            play_.allyPick = play_.allyPick == slot ? -1 : slot;
        } else if (b.action.rfind("ally-set", 0) == 0 && world_ && play_.allyPick >= 0) {
            // The player's posture towards that race (theirs unchanged);
            // set neutral, it turns hostile at their first shot.
            sim_->setPosture(play_.allyPick, std::atoi(b.action.c_str() + 8));
            play_.allyPick = -1;
        } else if (b.action == "info") {
            play_.colonyDetails = !play_.colonyDetails;
        } else if (b.action.rfind("jump", 0) == 0) {
            jumpTo(std::atoi(b.action.c_str() + 4));
        } else if (b.action == "raze" && u) {
            play_.razeAsked = u->id;
        } else if (b.action == "cancel-yes") {
            // (Only the question asked on this building's details.)
            const std::vector<int> pending = pendingBuilds();
            if (sim_ && u && play_.cancelFrom == u->id && play_.colonyDetails &&
                std::find(pending.begin(), pending.end(), play_.cancelAsked) != pending.end() &&
                sim_->cancelBuild(play_.cancelAsked))
                showColony();
            play_.cancelAsked = play_.cancelFrom = 0;
        } else if (b.action == "cancel-no") {
            play_.cancelAsked = play_.cancelFrom = 0;
        } else if (b.action.rfind("cancel", 0) == 0 && u) {
            play_.cancelAsked = std::atoi(b.action.c_str() + 6);
            play_.cancelFrom = u->id;
        } else if (b.action == "raze-no") {
            play_.razeAsked = 0;
        } else if (b.action == "raze-yes" && u && economy_) {
            play_.razeAsked = 0;
            if (sim_)
                sim_->raze(u->id);
        } else if (b.action == "build") {
            play_.buildMenu = !(u && u->name == "vehicle-yard");
            play_.vehicleMenu = u && u->name == "vehicle-yard";
            play_.menuPage = 0;
        } else if (b.action == "research") {
            play_.researchMenu = true;
        } else if ((b.action == "crew-" || b.action == "crew+") && u && economy_) {
            for (size_t i = 0; i < colonyObjects().size() && i < structureIds().size(); ++i)
                if (colonyObjects()[i] == u->id)
                    for (const ColonyStructure& c : economy_->structures())
                        if (c.id == structureIds()[i]) {
                            const BuildingType* t = buildingType(c.type);
                            const int now = c.target < 0 ? (t ? t->crewSlots : 0) : c.target;
                            economy_->setCrewTarget(c.id, now + (b.action == "crew+" ? 1 : -1));
                        }
        } else if (b.action.rfind("officer", 0) == 0) {
            // A click picks the officer; a second on the same card brings
            // the camera to whatever they're in (exe.md#crew-and-efficiency).
            const int i = std::atoi(b.action.c_str() + 7);
            if (i == play_.pickedOfficer && nowMs_ - play_.officerClickMs < 400 && fieldWork_ &&
                size_t(i) < fieldWork_->officers().size())
                if (const Unit* in = world_ ? world_->unit(fieldWork_->officers()[size_t(i)].in) : nullptr)
                    cameraTo(*in);
            play_.pickedOfficer = i;
            play_.officerClickMs = nowMs_;
        }
        redraw();
        return true;
    }
    return false;
}

// JUMP TO, a left slot: the camera to the next of the colony's objects of
// its class (round again after the last), those being razed or destroyed
// skipped; a right slot: battle group n selected, and the camera on it.
void MissionViewer::jumpTo(int slot) {
    if (!world_)
        return;
    if (slot >= 5) {
        float x = 0, y = 0;
        if (world_->selectGroup(slot - 4)) {
            selectUnits(world_->selection());
            if (world_->groupCentre(slot - 4, x, y))
                map_.centreOn(x, y);
        }
        showUnits();
        return;
    }
    const char side = this->side();
    const int race = side == 'K' ? 1 : side == 'R' ? 2 : 0;
    auto matches = [&](const Unit& u) {
        if (u.slot != playerSlot_ || u.hp <= 0 || (sim_ && sim_->razing(u.id)))
            return false;
        const CombatType* t = combatType(u.type);
        if (slot == 1)
            return t && (t->mask & 0x8) != 0; // the mines
        if (slot == 4)
            return t && (t->mask & 0x4440100) != 0; // fighting, science and scouting vehicles
        return u.type == kJumpIcons[race][slot];
    };
    std::vector<const Unit*> all;
    for (const Unit& u : world_->units())
        if (matches(u))
            all.push_back(&u);
    if (all.empty())
        return;
    size_t next = 0;
    for (size_t i = 0; i < all.size(); ++i)
        if (all[i]->id == play_.jumpedTo[size_t(slot)])
            next = (i + 1) % all.size();
    play_.jumpedTo[size_t(slot)] = all[next]->id;
    cameraTo(*all[next]);
}

// A, G, S, T, P (exe.md#keys): the colony's vehicles of a kind selected.
void MissionViewer::selectAllOfKind(char k) {
    if (!world_)
        return;
    std::vector<int> ids;
    for (const Unit& u : world_->units()) {
        if (u.slot != playerSlot_ || u.building || u.hp <= 0)
            continue;
        const std::string& n = u.name;
        const bool artillery = n.find("artillery") != std::string::npos && n.find("defensive") == std::string::npos;
        const bool is = k == 'a'   ? n.find("apc") != std::string::npos
                        : k == 'g' ? n == "ground-scout"
                        : k == 's' ? n == "science-vehicle"
                        : k == 'p' ? artillery
                                   : (n.find("tank") != std::string::npos || n.find("battery") != std::string::npos);
        if (is)
            ids.push_back(u.id);
    }
    selectUnits(ids);
    showUnits();
}

// Shift or Ctrl + B / V: the camera on to the next of the colony's
// buildings, or of its fighting, science and scouting vehicles.
void MissionViewer::cycleCamera(bool buildings) {
    if (!world_)
        return;
    std::vector<const Unit*> all;
    for (const Unit& u : world_->units()) {
        if (u.slot != playerSlot_ || u.hp <= 0 || u.building != buildings)
            continue;
        const CombatType* t = combatType(u.type);
        if (!buildings && !(t && (t->mask & 0x4440100)))
            continue;
        all.push_back(&u);
    }
    if (all.empty())
        return;
    int& last = buildings ? play_.cycledBuilding : play_.cycledVehicle;
    size_t next = 0;
    for (size_t i = 0; i < all.size(); ++i)
        if (all[i]->id == last)
            next = (i + 1) % all.size();
    last = all[next]->id;
    cameraTo(*all[next]);
}

// A double click on one of the player's objects (exe.md#cameras): a
// finished building plays its own sound (exe.md#sounds), from where it
// stands: full by the camera's spot, fading out 8 blocks off, panned to
// its side; only one at a time. In F1 the camera then follows it; in F2
// and F3 it stays put.
void MissionViewer::doubleClicked(int unit) {
    const Unit* u = world_ ? world_->unit(unit) : nullptr;
    if (!u || u->slot != playerSlot_)
        return;
    if (const auto [archive, stem] = buildingSound(u->type); !archive.empty() && u->built >= 1) {
        if (mixer_)
            mixer_->stop(objectSoundHandle_);
        objectSoundHandle_ = playAt(archive, stem, u->x, u->y, 2);
    }
    followInF1(unit);
}

// F1's camera on to one of the player's objects, following it.
void MissionViewer::followInF1(int unit) {
    const Unit* u = world_ ? world_->unit(unit) : nullptr;
    if (!u || u->slot != playerSlot_ || map_.view() != MapViewer::View::FirstPerson)
        return;
    play_.followed = unit;
    map_.startFollowing();
    const Pose at = drawnAt(*u);
    map_.follow(at.x, at.y, at.heading, u->kind.model);
}

// The camera to one of the player's objects (JUMP TO, Space): in F1 it
// follows it, as after a double click (but without the sound); else it
// centres on it.
void MissionViewer::cameraTo(const Unit& u) {
    if (map_.view() == MapViewer::View::FirstPerson && u.slot == playerSlot_)
        followInF1(u.id);
    else
        map_.centreOn(u.x, u.y);
}

} // namespace reluva::newworlds
