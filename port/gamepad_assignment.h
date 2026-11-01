// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "gamepad_input.h"
#include "../recomp-common/settings.h"
#include <array>
#include <vector>

namespace port {
struct GamepadDevice {
    SDL_JoystickID id = 0;
    std::string key, name;
};
struct GamepadAssignments {
    std::array<std::string, kBoundPlayersMost> device{{"auto", "auto", "auto", "auto"}};
};
inline GamepadDevice DescribeGamepad(SDL_Gamepad *pad) {
    GamepadDevice result;
    if (!pad) return result;
    result.id = SDL_GetGamepadID(pad);
    const char *name = SDL_GetGamepadName(pad);
    result.name = name ? name : "Gamepad";
    char guid[33]{};
    SDL_GUIDToString(SDL_GetGamepadGUIDForID(result.id), guid, sizeof guid);
    const char *serial = SDL_GetGamepadSerial(pad), *path = SDL_GetGamepadPath(pad);
    if (serial && *serial) result.key = std::string("serial:") + guid + ":" + serial;
    else if (path && *path) result.key = std::string("path:") + path;
    else result.key = std::string("guid:") + guid + ":" + result.name;
    return result;
}
inline GamepadAssignments LoadGamepadAssignments(const std::string &path) {
    GamepadAssignments config;
    recomp::Ini ini(path); ini.load();
    for (int player = 0; player < kBoundPlayersMost; ++player) {
        const auto value = ini.get("gamepads", "player" + std::to_string(player + 1));
        if (!value.empty()) config.device[player] = value;
    }
    return config;
}
inline bool SaveGamepadAssignments(const std::string &path, const GamepadAssignments &config) {
    if (path.empty()) return false;
    recomp::Ini ini(path); ini.load();
    for (int player = 0; player < kBoundPlayersMost; ++player)
        ini.set("gamepads", "player" + std::to_string(player + 1), config.device[player]);
    return ini.save();
}
inline void AssignGamepad(GamepadAssignments &config, int player, const std::string &key) {
    if (player < 0 || player >= kBoundPlayersMost || key.empty()) return;
    config.device[player] = key;
    for (int other = 0; other < kBoundPlayersMost; ++other)
        if (other != player && key != "auto" && key != "none" && config.device[other] == key) config.device[other] = "none";
}
// Explicit physical devices are reserved before automatic slots. A single
// pad explicitly assigned to P2 must never also feed automatic P1.
inline std::array<SDL_JoystickID, kBoundPlayersMost> ResolveGamepadAssignments(
        const GamepadAssignments &config, const std::vector<GamepadDevice> &devices) {
    std::array<SDL_JoystickID, kBoundPlayersMost> result{};
    for (int player = 0; player < kBoundPlayersMost; ++player) {
        const auto &key = config.device[player];
        if (key == "auto" || key == "none") continue;
        SDL_JoystickID match = 0; int count = 0;
        for (const auto &device : devices) if (device.key == key) { match = device.id; ++count; }
        if (count == 1 && std::find(result.begin(), result.end(), match) == result.end()) result[player] = match;
    }
    for (int player = 0; player < kBoundPlayersMost; ++player) {
        if (config.device[player] != "auto") continue;
        for (const auto &device : devices)
            if (device.id && std::find(result.begin(), result.end(), device.id) == result.end()) { result[player] = device.id; break; }
    }
    return result;
}
inline std::vector<GamepadDevice> DescribeGamepads(const std::vector<SDL_Gamepad *> &pads) {
    std::vector<GamepadDevice> devices;
    for (auto *pad : pads) if (pad) devices.push_back(DescribeGamepad(pad));
    return devices;
}
inline int AssignedGamepadPlayer(SDL_JoystickID id, const std::array<SDL_JoystickID, kBoundPlayersMost> &assigned) {
    for (int player = 0; player < kBoundPlayersMost; ++player) if (id && assigned[player] == id) return player;
    return -1;
}
inline bool PlayerPadInputBound(const Bindings &bindings, int player, PadInput input,
                                 const BoundTable &table = MakeBoundTable(4, "", true)) {
    if (player < 0 || player >= kBoundPlayersMost) return false;
    for (int action = 0; action < table.table.count; ++action)
        if (bindings.pad[player][action] == input) return true;
    return false;
}
inline std::array<uint32_t, kBoundPlayersMost> ReadAssignedGamepads(const std::vector<SDL_Gamepad *> &pads,
        const Bindings &bindings, const GamepadAssignments &config,
        const BoundTable &table = MakeBoundTable(4, "", true)) {
    const auto assigned = ResolveGamepadAssignments(config, DescribeGamepads(pads));
    std::array<uint32_t, kBoundPlayersMost> masks{};
    for (auto *pad : pads) {
        const int player = pad ? AssignedGamepadPlayer(SDL_GetGamepadID(pad), assigned) : -1;
        if (player >= 0) masks[player] |= ReadGamepadInput(pad, bindings, player, table);
    }
    return masks;
}
} // namespace port
