// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026. Local Input Lab extension to cps2recomp.
#include "input_profile.h"
#include "../recomp-common/settings.h"

#include <algorithm>
#include <charconv>

namespace port {
namespace {
InputProfile current;
std::string currentPath;
int Number(const std::string &text, int fallback, int minimum, int maximum) {
    int value = fallback;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size()
        ? std::clamp(value, minimum, maximum) : fallback;
}
bool Enabled(const std::string &text) { return !text.empty() && !recomp::SettingOff(text); }
} // namespace

InputProfile LoadInputProfile(const std::string &path) {
    InputProfile profile;
    recomp::Ini ini(path);
    ini.load();
    for (int player = 0; player < kInputPlayers; player++) {
        auto &one = profile.players[player];
        const std::string section = "input" + std::to_string(player + 1);
        one.neutralize = Enabled(ini.get(section, "neutralize_opposing_directions"));
        one.autofireDelay = Number(ini.get(section, "autofire_delay"), 1, 1, 30);
        for (int action = 0; action < kInputAttackActions; action++)
            one.autofire[action] = Enabled(ini.get(section, std::string("autofire_") + kInputActionNames[action]));
        for (int custom = 0; custom < kInputCustomCount; custom++)
            one.custom[custom] = static_cast<uint8_t>(Number(ini.get(section, "custom" + std::to_string(custom + 1)), 0, 0, 63));
    }
    return profile;
}

bool SaveInputProfile(const std::string &path, const InputProfile &profile) {
    if (path.empty()) return false;
    recomp::Ini ini(path);
    ini.load();
    for (int player = 0; player < kInputPlayers; player++) {
        const auto &one = profile.players[player];
        const std::string section = "input" + std::to_string(player + 1);
        ini.set(section, "neutralize_opposing_directions", one.neutralize ? "1" : "0");
        ini.set(section, "autofire_delay", std::to_string(std::clamp(one.autofireDelay, 1, 30)));
        for (int action = 0; action < kInputAttackActions; action++)
            ini.set(section, std::string("autofire_") + kInputActionNames[action], one.autofire[action] ? "1" : "0");
        for (int custom = 0; custom < kInputCustomCount; custom++)
            ini.set(section, "custom" + std::to_string(custom + 1), std::to_string(one.custom[custom] & 63));
    }
    return ini.save();
}

std::string InputMaskName(unsigned mask) {
    std::string name;
    for (int bit = 0; bit < 6; bit++) {
        if (!(mask & (1u << bit))) continue;
        if (!name.empty()) name += "+";
        name += kInputButtonNames[bit];
    }
    return name.empty() ? "Off" : name;
}

uint16_t NeutralizeInputDirections(uint16_t buttons) {
    if ((buttons & 0x3u) == 0x3u) buttons &= ~uint16_t{0x3};
    if ((buttons & 0xCu) == 0xCu) buttons &= ~uint16_t{0xC};
    return buttons;
}

uint16_t InputProcessor::Apply(int player, uint32_t raw, uint64_t frame, const InputProfile &profile) {
    if (player < 0 || player >= kInputPlayers) return 0;
    const auto &one = profile.players[player];
    uint16_t result = static_cast<uint16_t>(raw & 0xC0Fu); // directions, Start and Coin
    for (int action = 0; action < kInputAttackActions; action++) {
        const uint32_t trigger = 1u << (action < 6 ? action + 4 : action + 6);
        auto &state = states_[player][action];
        if (!(raw & trigger)) { state.held = false; continue; }
        if (!state.held || frame < state.start) { state.held = true; state.start = frame; }
        const uint64_t delay = static_cast<uint64_t>(std::clamp(one.autofireDelay, 1, 30));
        if (one.autofire[action] && ((frame - state.start) / delay) % 2 != 0) continue;
        const uint16_t mask = action < 6 ? static_cast<uint16_t>(1u << (action + 4)) :
                              action == 6 ? 0x70u : action == 7 ? 0x380u :
                              static_cast<uint16_t>((one.custom[action - 8] & 63u) << 4);
        result |= mask;
    }
    return one.neutralize ? NeutralizeInputDirections(result) : result;
}

InputProfile &CurrentInputProfile() { return current; }
void LoadCurrentInputProfile(const std::string &path) { currentPath = path; current = LoadInputProfile(path); }
bool SaveCurrentInputProfile() { return SaveInputProfile(currentPath, current); }
} // namespace port
