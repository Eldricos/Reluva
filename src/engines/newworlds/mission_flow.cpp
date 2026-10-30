// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/mission_ui.h"
#include "engines/newworlds/mission_surface.h"
#include "engines/newworlds/viewers.h"

#include <algorithm>
#include <cstdlib>

namespace reluva::newworlds {

namespace {

// MISSION REPORT: in the screen's right half, kReportMargin in from its
// sides, at most kReportW wide and at least kReportRight from the screen's
// edge; a line and kReportFoot more under its lines.
constexpr int kReportW = 300, kReportMargin = 10, kReportRight = 8, kReportFoot = 16;
// CONTINUE: its label kContinuePadX and kContinuePadY in from the edges, the
// button kContinueRight and kContinueFoot in from the screen's corner.
constexpr int kContinuePadX = 8, kContinuePadY = 4, kContinueRight = 16, kContinueFoot = 12;

} // namespace

void MissionViewer::startFlow() {
    stopBriefing();
    stopPlay();
    flow_ = Flow::Loading;
    flowStartMs_ = kFlowUnstarted; // (timed from the next update: the clock may be stale)
    dirty_ = true;
}

void MissionViewer::endFlow() {
    const Flow was = flow_;
    flow_ = Flow::None;
    if (was == Flow::Report && flowWon_ && isCampaignFinale(missions_[size_t(current_)].shortName))
        campaignWon_ = side();
    if (was == Flow::Briefing)
        stopBriefing();
    stopPlay();
    dirty_ = true;
    redraw();
    updateTitle();
}

// The loading boxes' heading: the mission as the game labels it
// ("FEDERATION LEVEL 01", "JOINT LEVEL 03", the tutorials, the bonus
// missions).
std::string MissionViewer::levelLabel() const {
    const std::string& m = missions_[size_t(current_)].shortName;
    const int n = std::atoi(m.c_str() + 1);
    auto two = [](int v) { return (v < 10 ? "0" : "") + std::to_string(v); };
    switch (m.empty() ? ' ' : m[0]) {
    case 'F': return "FEDERATION LEVEL " + two(n);
    case 'K': return "KLINGON LEVEL " + two(n);
    case 'R': return "ROMULAN LEVEL " + two(n);
    case 'J': return "JOINT LEVEL " + two(n);
    case 'C':
        if (n >= 9 && n <= 11)
            return std::string("BONUS ") + (n == 9 ? "FEDERATION" : n == 10 ? "KLINGON" : "ROMULAN") + " MISSION";
        return "TUTORIAL";
    default: return missions_[size_t(current_)].title;
    }
}

// The run's screens: the loading boxes over the three emblems (with the
// mission's label, what's being readied and how far it has got), and the
// end: the side's won or lost screen with the MISSION REPORT (each
// objective complete or not) and CONTINUE.
void MissionViewer::drawFlow() {
    const Level& level = missions_[size_t(current_)];
    const char side = this->side();
    if (uiSide_ != side) {
        uiSide_ = side;
        ui_ = GameUi{};
        ui_.load(archives_, side);
    }
    fillScreen(screen_, Color{});
    auto blit = [&](const Texture& t, int x0, int y0, int w, int h) {
        for (int y = 0; y < h; ++y) {
            if (y0 + y < 0 || y0 + y >= screen_.h)
                continue;
            uint8_t* row = screen_.row(y0 + y);
            const int sy = y * t.height / h;
            for (int x = 0; x < w; ++x) {
                if (x0 + x < 0 || x0 + x >= screen_.w)
                    continue;
                const uint8_t* p = &t.rgba[(size_t(sy) * size_t(t.width) + size_t(x * t.width / w)) * 4];
                uint8_t* o = row + (x0 + x) * 4;
                o[0] = p[0], o[1] = p[1], o[2] = p[2];
            }
        }
    };
    if (!ui_.loaded()) {
        drawText(kPanelMargin, kPanelMargin, "no GUI archive for this side", kNoteColor);
        return;
    }
    const int lh = ui_.smallHeight();
    if (flow_ == Flow::Report) {
        // Won: the race's emblem (the game never shows its f_win tiles);
        // lost: the lose tiles (exe.md#the-selection-panel).
        if (auto it = losses_.find(side); !flowWon_ && it != losses_.end())
            blit(it->second, 0, 0, screen_.w, screen_.h);
        else if (auto e = emblems_.find(side); e != emblems_.end())
            blit(e->second, 0, 0, screen_.w, screen_.h);
        // MISSION REPORT, right of centre, under the MISSION LOG heading:
        // won, each objective listed, complete or incomplete; lost, only the
        // fail criterion that fired.
        std::vector<std::pair<std::string, Color>> lines;
        lines.push_back({"MISSION LOG", ui_.brightColor()});
        const int w = std::min(kReportW, screen_.w / 2 - 2 * kReportMargin);
        if (!flowWon_ && run_ && run_->failedCriterion()) {
            const std::string* t = table_.find(run_->failedCriterion());
            for (const std::string& l :
                 ui_.wrapSmall((t ? *t : std::to_string(run_->failedCriterion())) + " - FAILED", w - 2 * kPanelInset))
                lines.push_back({l, ui_.textColor()});
        } else if (const MissionScript* script = findMissionScript(level.shortName, side); flowWon_ && script && run_)
            for (const MissionScript::Objective& o : script->objectives) {
                const MissionRun::State st = run_->objectiveState(o.id);
                if (st == MissionRun::State::Hidden)
                    continue;
                const std::string* t = table_.find(o.id);
                const std::string line = "MISSION OBJECTIVE: " + (t ? *t : std::to_string(o.id)) +
                                         (st == MissionRun::State::Complete ? " - COMPLETE" : " - INCOMPLETE");
                for (const std::string& l : ui_.wrapSmall(line, w - 2 * kPanelInset))
                    lines.push_back({l, st == MissionRun::State::Complete ? ui_.brightColor() : ui_.textColor()});
            }
        const int h = kPanelTitleH + int(lines.size()) * lh + lh + kReportFoot;
        const int x = std::min(screen_.w / 2 + kReportMargin, screen_.w - w - kReportRight), y = (screen_.h - h) / 2;
        ui_.drawPanel(screen_, x, y, w, h, "MISSION REPORT");
        for (size_t i = 0; i < lines.size(); ++i)
            ui_.drawSmall(screen_, x + kPanelInset, y + kPanelFirstLine + int(i) * lh, lines[i].first, lines[i].second);
        // CONTINUE, bottom right.
        const std::string label = "CONTINUE";
        const int bw = ui_.smallWidth(label) + 2 * kContinuePadX, bh = lh + 2 * kContinuePadY;
        continue_ = {screen_.w - bw - kContinueRight, screen_.h - bh - kContinueFoot, bw, bh};
        fillRect(screen_, continue_.x, continue_.y, bw, bh, Color{10, 20, 40});
        frameRect(screen_, continue_.x, continue_.y, bw, bh, ui_.textColor());
        ui_.drawSmall(screen_, continue_.x + kContinuePadX, continue_.y + kContinuePadY, label, ui_.brightColor());
        return;
    }
    // The loading boxes: the three emblems, and a box with the lines.
    const int ew = screen_.w / 4, eh = ew * 3 / 4, gap = screen_.w / 16;
    const int ex = (screen_.w - 3 * ew - 2 * gap) / 2, ey = screen_.h / 6;
    int k = 0;
    for (char e : {'F', 'K', 'R'}) {
        if (auto it = emblems_.find(e); it != emblems_.end())
            blit(it->second, ex + k * (ew + gap), ey, ew, eh);
        ++k;
    }
    const uint64_t length = flow_ == Flow::Starting ? kStartingMs : kLoadingMs;
    const uint64_t since = flowStartMs_ == kFlowUnstarted || nowMs_ < flowStartMs_ ? 0 : nowMs_ - flowStartMs_;
    const int done = int(std::min<uint64_t>(100, since * 100 / std::max<uint64_t>(1, length)));
    std::vector<std::string> lines;
    if (flow_ == Flow::Debriefing)
        lines = {"PREPARING THE DEBRIEFING", levelLabel(), "COLLECTING THE OFFICERS' REPORTS"};
    else
        lines = {"STARTING COLONY SYSTEMS", levelLabel(), flow_ == Flow::Loading ? "READYING THE BRIEFING" : "READYING THE MAP",
                 "LOADING COMMAND ROUTINES", "BRINGING THE OFFICERS ON STATION"};
    lines.push_back("DONE " + std::to_string(done) + "%");
    const int w = std::max(240, screen_.w / 3), h = kPanelTitleH + int(lines.size()) * lh;
    const int x = (screen_.w - w) / 2, y = ey + eh + screen_.h / 12;
    ui_.drawPanel(screen_, x, y, w, h, flow_ == Flow::Debriefing ? "DEBRIEFING" : "LOADING");
    for (size_t i = 0; i < lines.size(); ++i)
        ui_.drawSmall(screen_, x + kPanelInset, y + kPanelFirstLine + int(i) * lh, lines[i],
                      i == 1 ? ui_.brightColor() : ui_.textColor());
}

} // namespace reluva::newworlds
