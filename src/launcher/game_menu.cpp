// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "launcher/game_menu.h"

#include "backends/sdl3/sdl_system.h"
#include "common/fs.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <cstdio>
#include <ctime>

namespace reluva {

namespace {

std::string formatTime(int64_t t) {
    if (!t)
        return "";
    const std::time_t tt = std::time_t(t);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M", &tm);
    return buf;
}

// Why the game can't go on after a load it couldn't recover from.
constexpr const char* kBroken = "A load failed part way: load another save, or return to the launcher.";

std::string slotLabel(const save::Slot& s) {
    char num[8];
    std::snprintf(num, sizeof num, "%02d", s.number);
    if (!s.error.empty())
        return std::string(num) + "  (unreadable)";
    return std::string(num) + "  " + (s.header.description.empty() ? "(no description)" : s.header.description);
}

} // namespace

std::filesystem::path saveFolder(const std::filesystem::path& root, const std::string& target) {
    return root / fs::fromUtf8(target);
}

void SlotBrowser::clear() {
    for (auto& [n, t] : textures_)
        if (t)
            SDL_DestroyTexture(t);
    textures_.clear();
    slots_.clear();
}

void SlotBrowser::refresh(const std::filesystem::path& folder) {
    clear();
    slots_ = save::listSlots(folder);
}

SDL_Texture* SlotBrowser::thumbnail(const save::Slot& slot) {
    if (auto it = textures_.find(slot.number); it != textures_.end())
        return it->second;
    SDL_Texture* t = nullptr;
    const save::Header& h = slot.header;
    if (!h.thumbnail.empty()) {
        t = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, h.thumbWidth,
                              h.thumbHeight);
        if (t)
            SDL_UpdateTexture(t, nullptr, h.thumbnail.data(), h.thumbWidth * 4);
    }
    textures_[slot.number] = t;
    return t;
}

int SlotBrowser::draw(int& selected, int newSlot, float height) {
    int picked = 0;
    // Room beside it for the details (the thumbnail's width).
    const ImGuiStyle& style = ImGui::GetStyle();
    const float width = ImGui::GetContentRegionAvail().x - save::kThumbWidth * style.FontScaleDpi - style.ItemSpacing.x * 2;
    ImGui::BeginChild("slots", ImVec2(width, height), ImGuiChildFlags_Borders);
    if (newSlot) {
        char label[32];
        std::snprintf(label, sizeof label, "%02d  (new save)", newSlot);
        if (ImGui::Selectable(label, selected == newSlot, ImGuiSelectableFlags_AllowDoubleClick)) {
            selected = newSlot;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                picked = newSlot;
        }
    }
    for (const save::Slot& s : slots_) {
        const std::string label = slotLabel(s) + "##" + std::to_string(s.number);
        if (ImGui::Selectable(label.c_str(), selected == s.number, ImGuiSelectableFlags_AllowDoubleClick)) {
            selected = s.number;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                picked = s.number;
        }
    }
    if (!newSlot && slots_.empty())
        ImGui::TextDisabled("No saves yet.");
    ImGui::EndChild();
    return picked;
}

void SlotBrowser::drawDetails(int selected) {
    const auto it = std::find_if(slots_.begin(), slots_.end(), [&](const save::Slot& s) { return s.number == selected; });
    const float scale = ImGui::GetStyle().FontScaleDpi;
    if (it == slots_.end()) {
        ImGui::Dummy(ImVec2(save::kThumbWidth * scale, save::kThumbHeight * scale));
        ImGui::TextDisabled(selected ? "An empty slot." : "Pick a slot.");
        return;
    }
    if (SDL_Texture* t = thumbnail(*it))
        ImGui::Image(ImTextureRef(ImTextureID(t)), ImVec2(it->header.thumbWidth * scale, it->header.thumbHeight * scale));
    else
        ImGui::Dummy(ImVec2(save::kThumbWidth * scale, save::kThumbHeight * scale));
    if (!it->error.empty()) {
        ImGui::TextWrapped("Can't be read: %s", it->error.c_str());
        return;
    }
    ImGui::TextWrapped("%s", it->header.description.c_str());
    ImGui::TextDisabled("%s", formatTime(it->header.created).c_str());
}

GameMenu::GameMenu(SdlSystem& system, std::filesystem::path saveRoot) : sys_(system), root_(std::move(saveRoot)) {}

void GameMenu::attach(Engine* engine, const GameTarget& target) {
    engine_ = engine;
    target_ = target;
    broken_ = false;
}

Error GameMenu::save(int slot, const std::string& description) {
    if (!engine_)
        return Error::fail("No game is running.");
    std::string why;
    if (!engine_->canSaveState(why))
        return Error::fail(why);
    save::Serializer s = save::Serializer::saver();
    if (Error e = engine_->syncState(s))
        return e;
    save::Header h;
    h.engineId = target_.engineId, h.gameId = target_.gameId, h.target = target_.target;
    h.stateVersion = engine_->stateVersion();
    h.description = description;
    h.created = int64_t(std::time(nullptr));
    h.reluvaVersion = RELUVA_VERSION;
    const std::vector<uint8_t>& frame = sys_.lastFrame();
    if (!frame.empty())
        save::makeThumbnail(frame.data(), sys_.gameWidth(), sys_.gameHeight(), sys_.gameWidth() * 4, h);
    return save::writeFile(save::slotPath(saveFolder(root_, target_.target), slot), h, s.bytes());
}

Error GameMenu::load(int slot) {
    if (!engine_)
        return Error::fail("No game is running.");
    std::string why;
    if (!engine_->canLoadState(why))
        return Error::fail(why);
    save::Header h;
    std::vector<uint8_t> bytes;
    // (The file's errors read "a damaged savestate".)
    if (Error e = save::readFile(save::slotPath(saveFolder(root_, target_.target), slot), h, bytes))
        return Error::fail("Can't load it: " + e.message + ". Nothing was loaded.");
    if (h.engineId != target_.engineId || h.gameId != target_.gameId)
        return Error::fail("This save is from another game (" + h.gameId + ").");
    if (h.stateVersion != engine_->stateVersion())
        return Error::fail("This save is from another version of Reluva (" + h.reluvaVersion +
                           ") whose saves for this game this one can't read.");
    save::Serializer s = save::Serializer::loader(std::move(bytes));
    Error e = engine_->syncState(s);
    if (e.ok && !s.ok())
        e = Error::fail("The save is damaged: " + s.error());
    if (e.ok && !s.atEnd())
        e = Error::fail("The save has more in it than this version reads.");
    // Past this point the game holds what was read: on a failure it must
    // put itself right, or it can't be played or saved until a load works.
    // The message then says which.
    broken_ = false;
    if (!e.ok) {
        std::string done;
        broken_ = !engine_->loadFailed(done);
        if (broken_)
            done = kBroken;
        if (!done.empty())
            e.message += (e.message.empty() || e.message.back() == '.' ? " " : ". ") + done;
    }
    return e;
}

EventType GameMenu::run() { return loop(Page::Main, {}); }

EventType GameMenu::loadAtStart(int slot) {
    // A failure is shown on the menu's Load page; the game goes on from
    // where it started.
    if (Error e = load(slot))
        return loop(Page::Load, e.message);
    return EventType::None;
}

EventType GameMenu::loop(Page page, const std::string& problem) {
    SDL_Renderer* renderer = sys_.renderer();
    SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    SlotBrowser browser(renderer);
    const std::filesystem::path folder = saveFolder(root_, target_.target);
    int selected = 0;
    int newSlot = 0; // the Save page's empty slot (again after a save or a delete)
    std::string message, description;
    bool askOverwrite = false, askDelete = false;
    auto enter = [&](Page p) {
        page = p;
        browser.refresh(folder);
        askOverwrite = askDelete = false;
        newSlot = p == Page::Save ? save::freeSlot(folder) : 0;
        if (p == Page::Save) {
            selected = newSlot;
            description = engine_ ? engine_->stateDescription() : "";
            if (description.empty())
                description = target_.description;
        } else {
            // The newest readable save picked first.
            selected = 0;
            int64_t newest = -1;
            for (const save::Slot& s : browser.slots())
                if (s.error.empty() && s.header.created > newest)
                    newest = s.header.created, selected = s.number;
        }
    };
    if (page != Page::Main)
        enter(page);
    bool showProblem = !problem.empty();
    message = problem;
    EventType result = EventType::None;
    bool open = true;
    while (open) {
        // (Again each frame: a load changes them.)
        std::string why;
        const bool canSave = engine_ && !broken_ && engine_->canSaveState(why);
        const std::string saveWhy = broken_ ? kBroken : why;
        const bool canLoad = engine_ && engine_->canLoadState(why);
        const std::string loadWhy = why;

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL3_ProcessEvent(&ev);
            if (ev.type == SDL_EVENT_QUIT) {
                result = EventType::Quit;
                open = false;
            }
            if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.key == SDLK_ESCAPE && !ev.key.repeat &&
                !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId)) {
                if (page != Page::Main)
                    page = Page::Main;
                else if (!broken_)
                    open = false;
            }
            if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.key == SDLK_F5 && (ev.key.mod & SDL_KMOD_CTRL) &&
                !ev.key.repeat && !broken_)
                open = false;
        }
        if (!open)
            break;

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        const float scale = ImGui::GetStyle().FontScaleDpi;
        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x / 2, vp->WorkPos.y + vp->WorkSize.y / 2),
                                ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        const ImVec2 size = page == Page::Main ? ImVec2(260 * scale, 0) : ImVec2(560 * scale, 380 * scale);
        ImGui::SetNextWindowSize(size);
        const std::string title = (page == Page::Main ? "Reluva" : page == Page::Save ? "Save" : "Load") +
                                  std::string(" - ") + target_.description + "###menu";
        ImGui::Begin(title.c_str(), nullptr,
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoResize);
        const ImVec2 wide(-1, 0);
        if (page == Page::Main) {
            ImGui::BeginDisabled(broken_);
            if (ImGui::Button("Resume", wide))
                open = false;
            ImGui::EndDisabled();
            if (broken_ && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("%s", kBroken);
            ImGui::BeginDisabled(!canSave);
            if (ImGui::Button("Save...", wide))
                enter(Page::Save);
            ImGui::EndDisabled();
            if (!canSave && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("%s", saveWhy.c_str());
            ImGui::BeginDisabled(!canLoad);
            if (ImGui::Button("Load...", wide))
                enter(Page::Load);
            ImGui::EndDisabled();
            if (!canLoad && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                ImGui::SetTooltip("%s", loadWhy.c_str());
            ImGui::Spacing();
            if (ImGui::Button("Return to Launcher", wide)) {
                result = EventType::ReturnToLauncher;
                open = false;
            }
            if (ImGui::Button("Quit Reluva", wide)) {
                result = EventType::Quit;
                open = false;
            }
        } else {
            const bool saving = page == Page::Save;
            const float footer = ImGui::GetFrameHeightWithSpacing() * (saving ? 3.2f : 2.2f);
            ImGui::BeginGroup();
            const int picked = browser.draw(selected, newSlot, ImGui::GetContentRegionAvail().y - footer);
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            browser.drawDetails(selected);
            ImGui::EndGroup();
            const bool exists = std::any_of(browser.slots().begin(), browser.slots().end(),
                                            [&](const save::Slot& s) { return s.number == selected; });
            if (saving) {
                char buf[128];
                std::snprintf(buf, sizeof buf, "%s", description.c_str());
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputText("##description", buf, sizeof buf))
                    description = buf;
            }
            auto act = [&] {
                const Error e = saving ? save(selected, description) : load(selected);
                if (e) {
                    if (saving) {
                        browser.refresh(folder);
                        newSlot = save::freeSlot(folder);
                    }
                    message = e.message;
                    showProblem = true;
                } else {
                    open = false; // back to the game
                }
            };
            ImGui::BeginDisabled(!selected || (!saving && !exists));
            if (ImGui::Button(saving ? "Save" : "Load") || (picked && (saving || exists))) {
                if (saving && exists)
                    askOverwrite = true;
                else
                    act();
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(!exists);
            if (ImGui::Button("Delete"))
                askDelete = true;
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Back"))
                page = Page::Main;
            if (askOverwrite)
                ImGui::OpenPopup("Overwrite?");
            if (askDelete)
                ImGui::OpenPopup("Delete?");
            if (showProblem) {
                ImGui::OpenPopup("Problem");
                showProblem = false;
            }
            if (ImGui::BeginPopupModal("Overwrite?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Save over slot %02d?", selected);
                if (ImGui::Button("Yes")) {
                    askOverwrite = false;
                    ImGui::CloseCurrentPopup();
                    act();
                }
                ImGui::SameLine();
                if (ImGui::Button("No")) {
                    askOverwrite = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopupModal("Delete?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Delete slot %02d?", selected);
                if (ImGui::Button("Yes")) {
                    std::error_code ec;
                    std::filesystem::remove(save::slotPath(folder, selected), ec);
                    askDelete = false;
                    ImGui::CloseCurrentPopup();
                    browser.refresh(folder);
                    newSlot = save::freeSlot(folder);
                }
                ImGui::SameLine();
                if (ImGui::Button("No")) {
                    askDelete = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopupModal("Problem", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::PushTextWrapPos(400 * scale);
                ImGui::TextUnformatted(message.c_str());
                ImGui::PopTextWrapPos();
                if (ImGui::Button("OK"))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
        }
        ImGui::End();
        ImGui::Render();

        // The game's last frame, dimmed, letterboxed as in play.
        ImGuiIO& io = ImGui::GetIO();
        SDL_SetRenderScale(renderer, 1.0f, 1.0f);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        if (SDL_Texture* game = sys_.gameTexture()) {
            int ww = 0, wh = 0;
            SDL_GetCurrentRenderOutputSize(renderer, &ww, &wh);
            const float k = std::min(float(ww) / sys_.gameWidth(), float(wh) / sys_.gameHeight());
            const SDL_FRect r{(ww - sys_.gameWidth() * k) / 2, (wh - sys_.gameHeight() * k) / 2, sys_.gameWidth() * k,
                              sys_.gameHeight() * k};
            SDL_SetTextureColorMod(game, 110, 110, 110);
            SDL_RenderTexture(renderer, game, nullptr, &r);
            SDL_SetTextureColorMod(game, 255, 255, 255);
        }
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }
    browser.clear();
    return result;
}

} // namespace reluva
