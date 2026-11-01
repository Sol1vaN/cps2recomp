// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026. Local Input Lab extension to cps2recomp.
#include "input_controls.h"
#include "../../port/keyboards.h"
#include "../../port/input_profile.h"
#include "layout.h"
#include "../../port/gamepad_assignment.h"


namespace {
void DrawGamepadAssignments(launcher::PageContext &page) {
    using namespace layout; using namespace ui;
    const auto &palette = ui::palette();
    const auto path = (page.prefs.path().parent_path() / "gamepads.ini").string();
    auto config = port::LoadGamepadAssignments(path);
    std::vector<port::GamepadDevice> devices;
    int count = 0; auto *ids = SDL_GetGamepads(&count);
    for (int n = 0; n < count; ++n) if (auto *pad = SDL_OpenGamepad(ids[n])) {
        devices.push_back(port::DescribeGamepad(pad)); SDL_CloseGamepad(pad);
    }
    SDL_free(ids);
    text("GAMEPADS - SHARED BY ALL GAMES", Font::Semibold, size::kDetail, palette.secondary);
    gap(8);
    text("Choose which physical gamepad controls each player. A single gamepad can be assigned to Player 2 while Player 1 uses a keyboard.",
         Font::Regular, size::kDetail, palette.secondary, page.width);
    gap(10);
    ImGui::BeginDisabled(page.playing);
    Card card(page.width);
    for (int player = 0; player < port::kBoundPlayersMost; ++player) {
        if (player) inset_separator(page.width);
        ImGui::PushID(920 + player);
        std::vector<std::string> values{"auto", "none"}, labels{"Automatic", "Not assigned"};
        for (size_t n = 0; n < devices.size(); ++n) {
            values.push_back(devices[n].key);
            labels.push_back(devices[n].name + " (Controller " + std::to_string(n + 1) + ")");
        }
        int selected = 0;
        for (size_t n = 0; n < values.size(); ++n) if (values[n] == config.device[player]) selected = static_cast<int>(n);
        if (selected == 0 && config.device[player] != "auto") {
            values.push_back(config.device[player]); labels.push_back("Saved gamepad (disconnected)"); selected = static_cast<int>(values.size() - 1);
        }
        const auto resolved = port::ResolveGamepadAssignments(config, devices);
        std::string status = "No gamepad assigned or connected.";
        for (const auto &device : devices) if (resolved[player] == device.id) status = "Controls this player: " + device.name;
        const std::string title = "Player " + std::to_string(player + 1) + " gamepad";
        row(page.width, title.c_str(), status.c_str(), 390, 36, [&] {
            if (dropdown("##physical_gamepad", labels, selected, 390)) {
                port::AssignGamepad(config, player, values[selected]);
                port::SaveGamepadAssignments(path, config);
            }
        });
        ImGui::PopID();
    }
    card.end(); ImGui::EndDisabled(); gap(18);
}
} // namespace

void DrawHardwareControls(launcher::PageContext &page) {
    DrawGamepadAssignments(page);
    DrawKeyboardControls(page);
}
void DrawInputControls(launcher::PageContext &page, int player) {
    using namespace layout;
    using namespace ui;
    const auto keyboardConfig = port::LoadKeyboards((page.prefs.path().parent_path() / "keyboards.ini").string());
    if (player == 1 && keyboardConfig.enabled && keyboardConfig.mirrorP1) {
        text("Player 2 uses Player 1's keys, autofire and Custom options while separate keyboards are enabled. "
             "Edit Player 1 to change that shared layout. Player 2's saved options stay intact.",
             Font::Regular, size::kBody, palette().secondary, page.width);
        return;
    }
    auto profile = port::LoadInputProfile(page.settings.path().string());
    auto &one = profile.players[player];
    bool changed = false;
    const auto &p = palette();
    text("INPUT OPTIONS - THIS GAME ONLY", Font::Semibold, size::kDetail, p.secondary);
    gap(10);
    ImGui::BeginDisabled(page.playing);
    Card options(page.width);
    row(page.width, "Neutralize Opposing Directions (MAME)",
        "Left + Right cancel. Up + Down cancel. Left + Down + Right leaves Down.", 46.0f, 26.0f, [&] {
            if (toggle("##neutral", one.neutralize)) { one.neutralize = !one.neutralize; changed = true; }
        });
    inset_separator(page.width);
    std::vector<std::string> delays;
    for (int n = 1; n <= 30; n++) delays.push_back(std::to_string(n) + (n == 1 ? " frame on / off" : " frames on / off"));
    int delay = one.autofireDelay - 1;
    row(page.width, "Autofire delay", "Starts pressed immediately; timing follows game frames, including slow motion.",
        230.0f, 36.0f, [&] { if (dropdown("##autofire_delay", delays, delay, 230.0f)) {
            one.autofireDelay = delay + 1; changed = true;
        }});
    options.end();
    gap(18);
    text("AUTOFIRE", Font::Semibold, size::kDetail, p.secondary);
    gap(10);
    Card fire(page.width);
    for (int action = 0; action < port::kInputAttackActions; action++) {
        if (action) inset_separator(page.width);
        ImGui::PushID(action);
        row(page.width, port::kInputActionTitles[action], "", 46.0f, 26.0f, [&] {
            if (toggle("##fire", one.autofire[action])) { one.autofire[action] = !one.autofire[action]; changed = true; }
        });
        ImGui::PopID();
    }
    fire.end();
    gap(18);
    text("CUSTOM COMBINATIONS", Font::Semibold, size::kDetail, p.secondary);
    gap(6);
    text("Each Custom button presses all selected buttons together. Bind its key or gamepad button below.",
         Font::Regular, size::kDetail, p.secondary, page.width);
    gap(10);
    Card custom(page.width);
    for (int slot = 0; slot < port::kInputCustomCount; slot++) {
        if (slot) inset_separator(page.width);
        const std::string label = "Custom " + std::to_string(slot + 1);
        const std::string summary = port::InputMaskName(one.custom[slot]);
        ImGui::PushID(slot);
        row(page.width, label.c_str(), summary.c_str(), 330.0f, 26.0f, [&] {
            for (int bit = 0; bit < 6; bit++) {
                if (bit) ImGui::SameLine(0.0f, px(6));
                const bool active = (one.custom[slot] & (1u << bit)) != 0;
                if (button(port::kInputButtonNames[bit], active ? Kind::Primary : Kind::Secondary, 48.0f, true, 26.0f, 16.0f)) {
                    one.custom[slot] ^= static_cast<uint8_t>(1u << bit);
                    changed = true;
                }
            }
        });
        ImGui::PopID();
    }
    custom.end();
    ImGui::EndDisabled();
    if (changed) {
        if (!port::SaveInputProfile(page.settings.path().string(), profile)) {
            text("Could not save this game's input options.", Font::Regular, size::kDetail, p.text, page.width);
        }
        page.settings.load(); // later launch/feature saves must retain these input sections
    }
}
