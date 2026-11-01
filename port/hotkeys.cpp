// SPDX-License-Identifier: GPL-3.0-only
#include "hotkeys.h"
#include "pad_hotkeys.h"
#include "../recomp-common/settings.h"
#include <filesystem>
#include <charconv>
#include <algorithm>

namespace port {
unsigned HotkeyModifiers(SDL_Keymod mods) {
    return ((mods & SDL_KMOD_CTRL) ? 1u : 0u) | ((mods & SDL_KMOD_SHIFT) ? 2u : 0u) |
           ((mods & SDL_KMOD_ALT) ? 4u : 0u) | ((mods & SDL_KMOD_GUI) ? 8u : 0u);
}
Hotkeys DefaultHotkeys() {
    Hotkeys keys;
    for (int n = 0; n < kHotkeyCount; ++n) keys[n] = kHotkeys[n].usual;
    return keys;
}
Hotkeys LoadHotkeys(const std::string &path) {
    auto keys = DefaultHotkeys();
    recomp::Ini ini(path); ini.load();
    for (int n = 0; n < kHotkeyCount; ++n) {
        std::string name = kHotkeys[n].name;
        if (name == "reset" && ini.get("hotkeys", name).empty()) name = "menu_alt"; // old alternate-menu slot becomes Reset
        const auto text = ini.get("hotkeys", name);
        if (text.empty()) continue;
        keys[n].key = text == "none" ? SDL_SCANCODE_UNKNOWN : SDL_GetScancodeFromName(text.c_str());
        if (keys[n].key == SDL_SCANCODE_ESCAPE) keys[n].key = SDL_SCANCODE_UNKNOWN;
        const auto mods = ini.get("hotkeys", name + "_mods");
        unsigned number = 0;
        const auto parsed = std::from_chars(mods.data(), mods.data() + mods.size(), number);
        keys[n].mods = parsed.ec == std::errc{} && parsed.ptr == mods.data() + mods.size() ? number & 15u : 0u;
    }
    return keys;
}
bool SaveHotkeys(const std::string &path, const Hotkeys &keys) {
    if (path.empty()) return false;
    recomp::Ini ini(path); ini.load();
    for (int n = 0; n < kHotkeyCount; ++n) {
        const auto key = keys[n].key == SDL_SCANCODE_ESCAPE ? SDL_SCANCODE_UNKNOWN : keys[n].key;
        ini.set("hotkeys", kHotkeys[n].name, key == SDL_SCANCODE_UNKNOWN ? "none" : SDL_GetScancodeName(key));
        ini.set("hotkeys", std::string(kHotkeys[n].name) + "_mods", std::to_string(keys[n].mods & 15u));
    }
    return ini.save();
}
void BindHotkey(Hotkeys &keys, int action, Hotkey key) {
    if (action < 0 || action >= kHotkeyCount || key.key == SDL_SCANCODE_ESCAPE) return;
    key.mods &= 15u;
    if (key.key != SDL_SCANCODE_UNKNOWN)
        for (auto &other : keys)
            if (other.key == key.key && other.mods == key.mods) other = {};
    keys[action] = key;
}
std::string HotkeyLabel(Hotkey key) {
    if (key.key == SDL_SCANCODE_UNKNOWN) return "Not set";
    std::string label;
    if (key.mods & 1) label += "Ctrl+";
    if (key.mods & 2) label += "Shift+";
    if (key.mods & 4) label += "Alt+";
    if (key.mods & 8) label += "Win+";
    return label + SDL_GetScancodeName(key.key);
}
HotkeyAction FindHotkey(const Hotkeys &keys, SDL_Scancode key, SDL_Keymod mods, bool repeat) {
    if (key == SDL_SCANCODE_ESCAPE) return repeat ? HotkeyAction::None : HotkeyAction::Menu;
    if (key == SDL_SCANCODE_UNKNOWN) return HotkeyAction::None;
    for (int n = 0; n < kHotkeyCount; ++n)
        if (keys[n].key == key && keys[n].mods == HotkeyModifiers(mods) && (!repeat || kHotkeys[n].repeat))
            return static_cast<HotkeyAction>(n);
    return HotkeyAction::None;
}
bool HotkeyReserved(const Hotkeys &keys, SDL_Scancode key, SDL_Keymod mods) {
    return FindHotkey(keys, key, mods) != HotkeyAction::None;
}
bool HotkeyHeld(const Hotkeys &keys, HotkeyAction action, const bool *state, SDL_Keymod mods) {
    const auto &one = keys[static_cast<int>(action)];
    return one.key != SDL_SCANCODE_UNKNOWN && state[one.key] && one.mods == HotkeyModifiers(mods);
}
std::string DefaultHotkeysPath() {
    const auto base = std::filesystem::path(SDL_GetBasePath());
    if (std::filesystem::exists(base / "input-lab-portable.txt")) return (base / "user" / "hotkeys.ini").string();
    char *pref = SDL_GetPrefPath("", "CPS-2 Recomp");
    const auto folder = pref ? std::filesystem::path(pref) : base / "user";
    SDL_free(pref);
    return (folder / "hotkeys.ini").string();
}

const char *PadHotkeyName(int n) {
    static constexpr const char *extra[] = {"menu_alternate", "menu_combination", "zoom_hold", "multi_next", "fast_forward", "tag"};
    return n < kHotkeyCount ? kHotkeys[n].name : extra[n - kHotkeyCount];
}
const char *PadHotkeyTitle(int n) {
    static constexpr const char *extra[] = {"Open / close menu (alternate)", "Open / close menu (combination)",
        "Hold to pan camera", "Next game (Multi game)", "Fast-forward (Multi game, hold)", "Tag / extra button (hold)"};
    if (n == static_cast<int>(HotkeyAction::Pause)) return "Pause / resume (offline)";
    if (n == static_cast<int>(HotkeyAction::Step)) return "Advance one frame (while paused)";
    if (n == static_cast<int>(HotkeyAction::Chat)) return "Open chat (online)";
    return n < kHotkeyCount ? kHotkeys[n].title : extra[n - kHotkeyCount];
}
std::string PadChordText(const PadChord &chord) {
    if (chord.empty()) return "none";
    std::string text;
    for (auto input : chord) { if (!text.empty()) text += '&'; text += recomp::PadText(input); }
    return text;
}
PadChord PadChordFromText(const std::string &text) {
    if (text.empty() || text == "none") return {};
    PadChord chord;
    size_t begin = 0;
    while (begin < text.size()) {
        const auto end = text.find('&', begin);
        auto input = recomp::PadFromText(text.substr(begin, end == std::string::npos ? end : end - begin));
        if (input == recomp::PadInput{} || std::find(chord.begin(), chord.end(), input) != chord.end() || chord.size() >= 3) return {};
        chord.push_back(input);
        if (end == std::string::npos) break;
        begin = end + 1;
        if (begin == text.size()) return {};
    }
    std::sort(chord.begin(), chord.end(), [](auto a, auto b) { return recomp::PadText(a) < recomp::PadText(b); });
    return chord;
}
std::string PadChordLabel(const PadChord &chord) {
    if (chord.empty()) return "Not set";
    std::string label;
    for (auto input : chord) {
        if (!label.empty()) label += " + ";
        std::string text = recomp::PadText(input);
        if (input.axis >= 0) {
            static constexpr const char *axisNames[] = {"Left stick", "Left stick", "Right stick", "Right stick", "Left trigger", "Right trigger"};
            text = axisNames[input.axis];
            if (recomp::IsStickAxis(input.axis)) text += input.axis % 2 ? (input.direction < 0 ? " up" : " down") : (input.direction < 0 ? " left" : " right");
        } else {
            switch (input.button) {
            case SDL_GAMEPAD_BUTTON_SOUTH: text = "A / Cross"; break;
            case SDL_GAMEPAD_BUTTON_EAST: text = "B / Circle"; break;
            case SDL_GAMEPAD_BUTTON_WEST: text = "X / Square"; break;
            case SDL_GAMEPAD_BUTTON_NORTH: text = "Y / Triangle"; break;
            case SDL_GAMEPAD_BUTTON_DPAD_UP: text = "D-pad up"; break;
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN: text = "D-pad down"; break;
            case SDL_GAMEPAD_BUTTON_DPAD_LEFT: text = "D-pad left"; break;
            case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: text = "D-pad right"; break;
            case SDL_GAMEPAD_BUTTON_LEFT_STICK: text = "Left stick click"; break;
            case SDL_GAMEPAD_BUTTON_RIGHT_STICK: text = "Right stick click"; break;
            case SDL_GAMEPAD_BUTTON_GUIDE: text = "Guide"; break;
            case SDL_GAMEPAD_BUTTON_BACK: text = "Back / Select"; break;
            case SDL_GAMEPAD_BUTTON_START: text = "Start"; break;
            case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: text = "Left shoulder"; break;
            case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: text = "Right shoulder"; break;
            default: break;
            }
        }
        label += text;
    }
    return label;
}
PadHotkeys DefaultPadHotkeys() {
    PadHotkeys keys{};
    auto set = [&](HotkeyAction a, const char *text) { keys[static_cast<int>(a)] = PadChordFromText(text); };
    set(HotkeyAction::Menu, "leftstick");
    set(HotkeyAction::Pause, "rightstick");
    set(HotkeyAction::Chat, "rightstick");
    set(HotkeyAction::Step, "lefttrigger");
    set(HotkeyAction::Restart, "leftshoulder");
    set(HotkeyAction::ZoomPlayer1, "leftstick&leftshoulder");
    set(HotkeyAction::ZoomPlayer2, "leftstick&rightshoulder");
    set(HotkeyAction::ZoomBoth, "leftstick&rightstick");
    keys[kPadMenuAlt] = PadChordFromText("guide");
    keys[kPadMenuChord] = PadChordFromText("start&back");
    keys[kPadZoomHold] = PadChordFromText("leftstick");
    keys[kPadMultiNext] = PadChordFromText("leftshoulder");
    keys[kPadFastForward] = PadChordFromText("lefttrigger");
    keys[kPadTag] = PadChordFromText("lefttrigger");
    return keys;
}
PadHotkeys LoadPadHotkeys(const std::string &path) {
    auto keys = DefaultPadHotkeys();
    recomp::Ini ini(path); ini.load();
    for (int n = 0; n < kPadHotkeyCount; ++n) {
        const auto text = ini.get("gamepad_hotkeys", PadHotkeyName(n));
        if (!text.empty()) keys[n] = PadChordFromText(text);
    }
    return keys;
}
bool SavePadHotkeys(const std::string &path, const PadHotkeys &keys) {
    if (path.empty()) return false;
    recomp::Ini ini(path); ini.load();
    for (int n = 0; n < kPadHotkeyCount; ++n) ini.set("gamepad_hotkeys", PadHotkeyName(n), PadChordText(keys[n]));
    return ini.save(); // Preserve every keyboard shortcut and unknown setting.
}
void PadShortcutState::Event(const SDL_Event &event) {
    if ((event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event.type == SDL_EVENT_GAMEPAD_BUTTON_UP) && event.gbutton.button < buttons.size())
        buttons[event.gbutton.button] = event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN;
    if (event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION && event.gaxis.axis < axes.size()) axes[event.gaxis.axis] = event.gaxis.value;
}
void PadShortcutState::Poll(SDL_Gamepad *pad) {
    for (int n = 0; n < SDL_GAMEPAD_BUTTON_COUNT; ++n) buttons[n] = SDL_GetGamepadButton(pad, static_cast<SDL_GamepadButton>(n));
    for (int n = 0; n < SDL_GAMEPAD_AXIS_COUNT; ++n) axes[n] = SDL_GetGamepadAxis(pad, static_cast<SDL_GamepadAxis>(n));
}
bool PadShortcutState::Down(recomp::PadInput input) const {
    if (input.button >= 0) return input.button < static_cast<int>(buttons.size()) && buttons[input.button];
    if (input.axis < 0 || input.axis >= static_cast<int>(axes.size())) return false;
    return input.direction < 0 ? axes[input.axis] < -recomp::kPadThreshold : axes[input.axis] > recomp::kPadThreshold;
}
bool PadShortcutState::Down(const PadChord &chord) const {
    return !chord.empty() && std::all_of(chord.begin(), chord.end(), [&](auto input) { return Down(input); });
}
bool PadChordContains(const PadChord &larger, const PadChord &smaller) {
    return !smaller.empty() && std::all_of(smaller.begin(), smaller.end(), [&](auto input) {
        return std::find(larger.begin(), larger.end(), input) != larger.end();
    });
}
bool PadShortcutEligible(int n, const PadShortcutContext &c) {
    const auto action = static_cast<HotkeyAction>(n);
    if (n == static_cast<int>(HotkeyAction::Menu) || n == kPadMenuAlt || n == kPadMenuChord) return true;
    if (action == HotkeyAction::Fullscreen || action == HotkeyAction::FullscreenAlt) return true;
    if (action == HotkeyAction::Chat) return c.online;
    if (c.online) return false;
    if (c.menu) return action == HotkeyAction::Reset || action == HotkeyAction::Test;
    if (action == HotkeyAction::Step) return c.paused;
    if (action == HotkeyAction::Restart) return !c.multi;
    if (n == kPadMultiNext || n == kPadFastForward) return c.multi;
    if (n == kPadTag) return !c.paused && !c.multi;
    if (n == kPadZoomHold || action == HotkeyAction::ZoomPlayer1 || action == HotkeyAction::ZoomPlayer2 || action == HotkeyAction::ZoomBoth) return c.zoom;
    return true;
}
void BindPadHotkey(PadHotkeys &keys, int action, const PadChord &chord) {
    if (action < 0 || action >= kPadHotkeyCount) return;
    const auto normalized = PadChordFromText(PadChordText(chord));
    auto menu = [](int n) { return n == static_cast<int>(HotkeyAction::Menu) || n == kPadMenuAlt || n == kPadMenuChord; };
    if (!normalized.empty()) for (int n = 0; n < kPadHotkeyCount; ++n) {
        if (n == action || keys[n] != normalized) continue;
        // The tap-to-menu / hold-to-pan pairing is intentional. Pause/chat
        // also share a default button, but their contexts never overlap.
        if ((menu(n) && action == kPadZoomHold) || (menu(action) && n == kPadZoomHold)) continue;
        for (int mask = 0; mask < 32; ++mask) {
            PadShortcutContext c{bool(mask & 1), bool(mask & 2), bool(mask & 4), bool(mask & 8), bool(mask & 16)};
            if (PadShortcutEligible(action, c) && PadShortcutEligible(n, c)) { keys[n].clear(); break; }
        }
    }
    keys[action] = normalized;
}
PadHotkeyBits PadShortcutRouter::Update(const PadHotkeys &keys, PadHotkeyBits held, bool menuOpen) {
    const int menus[] = {static_cast<int>(HotkeyAction::Menu), kPadMenuAlt, kPadMenuChord};
    for (int n = 0; n < kPadHotkeyCount; ++n) {
        if (!held[n]) continue;
        for (int m = 0; m < kPadHotkeyCount; ++m) if (held[m] && keys[m].size() > keys[n].size() && PadChordContains(keys[m], keys[n])) {
            held[n] = false; used[n] = true; deferred[n] = false;
            break;
        }
    }
    if (held[kPadZoomHold]) {
        const bool moved = state.buttons[SDL_GAMEPAD_BUTTON_DPAD_UP] || state.buttons[SDL_GAMEPAD_BUTTON_DPAD_DOWN] ||
            state.buttons[SDL_GAMEPAD_BUTTON_DPAD_LEFT] || state.buttons[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] ||
            std::abs(int(state.axes[SDL_GAMEPAD_AXIS_LEFTX])) > recomp::kPadThreshold ||
            std::abs(int(state.axes[SDL_GAMEPAD_AXIS_LEFTY])) > recomp::kPadThreshold;
        if (moved) for (int n : menus) if (deferred[n]) { used[n] = true; deferred[n] = false; }
    }
    PadHotkeyBits actions;
    for (int n : menus) if (!state.Down(keys[n])) {
        if (deferred[n] && !used[n] && !menuOpen) actions[n] = true;
        deferred[n] = used[n] = false;
    }
    const auto pressed = edges.Pressed(held);
    for (int n = 0; n < kPadHotkeyCount; ++n) {
        if (!pressed[n] || used[n]) continue;
        const bool menu = n == static_cast<int>(HotkeyAction::Menu) || n == kPadMenuAlt || n == kPadMenuChord;
        if (menu && !menuOpen && held[kPadZoomHold] && keys[n] == keys[kPadZoomHold]) deferred[n] = true;
        else actions[n] = true;
    }
    for (int n = 0; n < kPadHotkeyCount; ++n) if (!state.Down(keys[n])) used[n] = false;
    // Multiple editable alternatives may intentionally name the same button.
    // They represent one command, so do not open and immediately close a menu.
    auto command = [](int n) {
        if (n == kPadMenuAlt || n == kPadMenuChord) return static_cast<int>(HotkeyAction::Menu);
        if (n == static_cast<int>(HotkeyAction::FullscreenAlt)) return static_cast<int>(HotkeyAction::Fullscreen);
        return n;
    };
    for (int n = 0; n < kPadHotkeyCount; ++n) if (actions[n])
        for (int m = 0; m < n; ++m) if (actions[m] && command(m) == command(n)) { actions[n] = false; break; }
    return actions;
}
} // namespace port
