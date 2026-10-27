// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/research.h"

#include <algorithm>

namespace reluva::newworlds {

namespace {

constexpr double kTick = 0.05; // the game's 20 ticks a second
// A topic the script names that the mission doesn't have (a placeholder).
constexpr int kOwnPoints = 100000;

} // namespace

Research::Research(std::string_view mission, int slot) {
    for (const ResearchTopic* t : researchTopics(mission, slot)) {
        Topic topic;
        topic.name = t->topic;
        topic.field = t->field;
        topic.points = t->points;
        topic.locks = t->locks;
        topics_.push_back(std::move(topic));
    }
}

Research::Topic* Research::find(const std::string& name) {
    // The scripts may add "-research" or "-research-topic".
    std::string base = name;
    for (const std::string suffix : {"-research-topic", "-research"})
        if (base.size() > suffix.size() && base.compare(base.size() - suffix.size(), suffix.size(), suffix) == 0)
            base.resize(base.size() - suffix.size());
    for (Topic& t : topics_)
        if (t.name == base || t.name == name)
            return &t;
    return nullptr;
}

void Research::unlock(const std::string& name) {
    Topic* t = find(name);
    if (!t) {
        Topic own;
        own.name = name;
        own.points = kOwnPoints;
        own.locks = 1;
        topics_.push_back(std::move(own));
        t = &topics_.back();
    }
    if (name != t->name && std::find(t->aliases.begin(), t->aliases.end(), name) == t->aliases.end())
        t->aliases.push_back(name);
    --t->locks;
}

std::vector<size_t> Research::offered() const {
    std::vector<size_t> out;
    for (size_t i = 0; i < topics_.size(); ++i)
        if (topics_[i].open() && !topics_[i].finished)
            out.push_back(i);
    return out;
}

bool Research::start(size_t index) {
    if (index >= topics_.size())
        return false;
    Topic& t = topics_[index];
    if (!t.open() || t.finished || t.running)
        return false;
    t.running = true;
    queue_.push_back(index);
    return true;
}

std::vector<ScriptEvent> Research::advance(double seconds) {
    std::vector<ScriptEvent> out;
    clock_ += seconds;
    while (clock_ >= kTick) {
        clock_ -= kTick;
        if (queue_.empty())
            continue;
        // The side's points shared out (whole points, as the game's).
        const int share = kPointsPerTick / int(queue_.size());
        for (size_t i : queue_) {
            Topic& t = topics_[i];
            t.done = std::min(t.points, t.done + share);
            if (t.done >= t.points) {
                t.finished = true;
                t.running = false;
                out.push_back({"researched", {t.name}});
                for (const std::string& a : t.aliases)
                    out.push_back({"researched", {a}});
            }
        }
        queue_.erase(std::remove_if(queue_.begin(), queue_.end(), [&](size_t i) { return topics_[i].finished; }),
                     queue_.end());
    }
    return out;
}

void Research::syncState(save::Serializer& s) {
    s(topics_)(queue_)(clock_);
}

} // namespace reluva::newworlds
