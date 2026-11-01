// SPDX-License-Identifier: GPL-3.0-only
#include "global_controls.h"
#include "../../port/hotkeys.h"
#include "../../port/pad_hotkeys.h"
#include "layout.h"
#include <algorithm>

namespace {
int capture = -1;
int padCapture = -1;
SDL_JoystickID capturePad = 0;
port::PadShortcutState captureState;
port::PadChord candidate;
std::string path, saveError;
void CancelCapture() { capture = padCapture = -1; capturePad = 0; captureState = {}; candidate.clear(); }
bool Modifier(SDL_Scancode key) {
    return key == SDL_SCANCODE_LCTRL || key == SDL_SCANCODE_RCTRL || key == SDL_SCANCODE_LSHIFT ||
           key == SDL_SCANCODE_RSHIFT || key == SDL_SCANCODE_LALT || key == SDL_SCANCODE_RALT ||
           key == SDL_SCANCODE_LGUI || key == SDL_SCANCODE_RGUI;
}
}
bool CapturingGlobalControls() { return capture >= 0 || padCapture >= 0; }
bool ConsumeGlobalControlsEvent(const SDL_Event &event) {
    if (padCapture >= 0) {
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) { CancelCapture(); return false; }
        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.scancode == SDL_SCANCODE_ESCAPE) CancelCapture();
            else if (event.key.scancode == SDL_SCANCODE_BACKSPACE || event.key.scancode == SDL_SCANCODE_DELETE) {
                auto keys = port::LoadPadHotkeys(path); keys[padCapture].clear();
                saveError = port::SavePadHotkeys(path, keys) ? "" : "Could not save gamepad shortcuts.";
                CancelCapture();
            }
            return true;
        }
        const bool button = event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event.type == SDL_EVENT_GAMEPAD_BUTTON_UP;
        const bool axis = event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION;
        if (!button && !axis) return false;
        const auto id = button ? event.gbutton.which : event.gaxis.which;
        const auto input = recomp::PadInputFromEvent(event);
        if (!capturePad && input != recomp::PadInput{}) capturePad = id;
        if (!capturePad || capturePad != id) return true;
        captureState.Event(event);
        port::PadChord held;
        for (int n = 0; n < SDL_GAMEPAD_BUTTON_COUNT; ++n) if (captureState.buttons[n]) held.push_back({n, -1});
        for (int n = 0; n < SDL_GAMEPAD_AXIS_COUNT; ++n) {
            if (captureState.axes[n] > recomp::kPadThreshold) held.push_back({-1, n});
            else if (recomp::IsStickAxis(n) && captureState.axes[n] < -recomp::kPadThreshold) held.push_back({-1, n, -1});
        }
        if (held.size() > 3) { saveError = "Use at most three inputs in a shortcut."; CancelCapture(); return true; }
        if (held.size() >= candidate.size() && !held.empty()) candidate = port::PadChordFromText(port::PadChordText(held));
        if (held.empty() && !candidate.empty()) {
            auto keys = port::LoadPadHotkeys(path); port::BindPadHotkey(keys, padCapture, candidate);
            saveError = port::SavePadHotkeys(path, keys) ? "" : "Could not save gamepad shortcuts.";
            CancelCapture();
        }
        return true;
    }
    if (capture < 0) return false;
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) { capture = -1; return false; }
    if (event.type != SDL_EVENT_KEY_DOWN) return false;
    if (event.key.repeat || Modifier(event.key.scancode)) return true;
    if (event.key.scancode != SDL_SCANCODE_ESCAPE) {
        auto keys = port::LoadHotkeys(path);
        port::BindHotkey(keys, capture, port::Hotkey{event.key.scancode, port::HotkeyModifiers(event.key.mod)});
        saveError = port::SaveHotkeys(path, keys) ? "" : "Could not save global shortcuts.";
    }
    capture = -1;
    return true;
}
void DrawGlobalControls(launcher::PageContext &page) {
    using namespace ui;
    using namespace layout;
    const auto &p = palette();
    path = (page.prefs.path().parent_path() / "hotkeys.ini").string();
    auto keys = port::LoadHotkeys(path);
    text("WHILE PLAYING - GLOBAL SHORTCUTS", Font::Semibold, size::kDetail, p.secondary);
    gap(8);
    text("Shared by all games. Click a shortcut and press a key or a combination such as Ctrl+F5. "
         "Clear removes it; Esc cancels. Shortcut keys take priority over game controls.",
         Font::Regular, size::kDetail, p.secondary, page.width);
    gap(10);
    ImGui::BeginDisabled(page.playing);
    Card card(page.width);
    row(page.width, "Open / close menu (always available)", "Escape stays fixed.", 260.0f, 34.0f, [&] {
        text("Esc (fixed)", Font::Semibold, size::kBody, p.secondary);
    });
    for (int n = 0; n < port::kHotkeyCount; ++n) {
        inset_separator(page.width);
        ImGui::PushID(n + 700);
        const bool waiting = capture == n;
        const auto label = waiting ? std::string("Press a shortcut...") : port::HotkeyLabel(keys[n]);
        row(page.width, port::kHotkeys[n].title, "", 260.0f, 34.0f, [&] {
            ImRect rect;
            if (key_chip("##global", label.c_str(), waiting, 200.0f, rect)) { CancelCapture(); capture = n; }
            ImGui::SameLine(0.0f, px(8));
            if (button("Clear", Kind::Ghost, 52.0f, true, 34.0f, 12.0f)) {
                capture = -1;
                port::BindHotkey(keys, n, {});
                saveError = port::SaveHotkeys(path, keys) ? "" : "Could not save global shortcuts.";
            }
        });
        ImGui::PopID();
    }
    card.end();
    gap(10);
    if (button("Restore default shortcuts", Kind::Secondary, 240.0f)) {
        CancelCapture();
        saveError = port::SaveHotkeys(path, port::DefaultHotkeys()) ? "" : "Could not save global shortcuts.";
    }
    gap(24);
    text("GAMEPAD - GLOBAL SHORTCUTS", Font::Semibold, size::kDetail, p.secondary);
    gap(8);
    text("Shared by all games and assigned controllers. Click an action, hold a button or a combination (up to three inputs), "
         "then release. Sticks and triggers also work. Clear removes the shortcut; Esc cancels. "
         "Single inputs assigned to game controls keep their game function. Combinations take priority.",
         Font::Regular, size::kDetail, p.secondary, page.width);
    gap(10);
    auto pads = port::LoadPadHotkeys(path);
    Card padCard(page.width);
    std::vector<int> order{static_cast<int>(port::HotkeyAction::Menu), port::kPadMenuAlt, port::kPadMenuChord};
    for (int n = 0; n < port::kPadHotkeyCount; ++n)
        if (std::find(order.begin(), order.end(), n) == order.end()) order.push_back(n);
    bool first = true;
    for (int n : order) {
        if (!first) inset_separator(page.width);
        first = false;
        ImGui::PushID(n + 1700);
        const bool waiting = padCapture == n;
        const auto label = waiting ? (candidate.empty() ? std::string("Hold, then release...") : port::PadChordLabel(candidate)) : port::PadChordLabel(pads[n]);
        const float controlsWidth = std::min(430.0f, page.width * 0.58f);
        row(page.width, port::PadHotkeyTitle(n), "", controlsWidth, 34.0f, [&] {
            ImRect rect;
            const auto shown = fit_end(label, Font::Regular, size::kDetail + 0.5f, controlsWidth - 84);
            if (key_chip("##padglobal", shown.c_str(), waiting, controlsWidth - 60, rect)) { CancelCapture(); padCapture = n; }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", label.c_str());
            ImGui::SameLine(0.0f, px(8));
            if (button("Clear", Kind::Ghost, 52.0f, true, 34.0f, 12.0f)) {
                CancelCapture(); pads[n].clear();
                saveError = port::SavePadHotkeys(path, pads) ? "" : "Could not save gamepad shortcuts.";
            }
        });
        ImGui::PopID();
    }
    padCard.end();
    gap(10);
    if (button("Clear all gamepad shortcuts", Kind::Secondary, 250.0f)) {
        CancelCapture();
        saveError = port::SavePadHotkeys(path, {}) ? "" : "Could not save gamepad shortcuts.";
    }
    if (page.width >= 510) ImGui::SameLine(0.0f, px(8));
    else gap(8);
    if (button("Restore gamepad shortcuts", Kind::Secondary, 240.0f)) {
        CancelCapture();
        saveError = port::SavePadHotkeys(path, port::DefaultPadHotkeys()) ? "" : "Could not save gamepad shortcuts.";
    }
    ImGui::EndDisabled();
    if (!saveError.empty()) text(saveError.c_str(), Font::Regular, size::kDetail, p.text, page.width);
    gap(8);
    text("To remove every gamepad menu access, clear all three Open / close menu entries. "
         "When a menu input also pans the camera, a tap opens the menu; using the camera cancels that tap. Esc always remains available.",
         Font::Regular, size::kDetail, p.secondary, page.width);
    if (CapturingGlobalControls()) request_frames(2);
}
