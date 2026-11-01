// SPDX-License-Identifier: GPL-3.0-only
#include "../../port/pad_hotkeys.h"
#include "../../recomp-common/settings.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace port;
int checks = 0;
void Check(bool ok, const char *label) { ++checks; if (!ok) throw std::runtime_error(label); }
PadHotkeyBits Update(PadShortcutRouter &router, const PadHotkeys &keys, PadShortcutContext context = {}) {
    PadHotkeyBits held;
    for (int n = 0; n < kPadHotkeyCount; ++n) held[n] = PadShortcutEligible(n, context) && router.state.Down(keys[n]);
    return router.Update(keys, held, context.menu);
}
void Button(PadShortcutRouter &router, int button, bool down) {
    SDL_Event ev{}; ev.type = down ? SDL_EVENT_GAMEPAD_BUTTON_DOWN : SDL_EVENT_GAMEPAD_BUTTON_UP;
    ev.gbutton.button = button; router.state.Event(ev);
}
int main() try {
    auto keys = DefaultPadHotkeys();
    const int menu = int(HotkeyAction::Menu), pause = int(HotkeyAction::Pause), chat = int(HotkeyAction::Chat);
    for (int n = 0; n < kPadHotkeyCount; ++n) {
        Check(PadChordFromText(PadChordText(keys[n])) == keys[n], "default chord roundtrip");
        Check(!PadChordLabel(keys[n]).empty(), "display label");
    }
    Check(PadChordFromText("start&back") == PadChordFromText("back&start"), "order independent chord");
    for (auto text : {"start&", "&back", "start&start", "leftx", "invalid", "a&b&x&y", "none&guide"})
        Check(PadChordFromText(text).empty(), "reject malformed chord");
    auto path = std::filesystem::temp_directory_path() / "cps2-pad-hotkeys-test.ini";
    auto keyboard = DefaultHotkeys(); keyboard[menu] = {SDL_SCANCODE_TAB};
    Check(SaveHotkeys(path.string(), keyboard), "save keyboard shortcuts");
    keys[menu].clear(); keys[kPadMenuAlt].clear(); keys[kPadMenuChord].clear();
    Check(SavePadHotkeys(path.string(), keys), "save cleared gamepad shortcuts");
    auto loaded = LoadPadHotkeys(path.string());
    Check(loaded == keys, "explicit none survives restart");
    Check(LoadHotkeys(path.string())[menu].key == SDL_SCANCODE_TAB, "gamepad save preserves keyboard");
    Check(SaveHotkeys(path.string(), keyboard), "save keyboard again");
    Check(LoadPadHotkeys(path.string()) == keys, "keyboard save preserves cleared gamepad");
    PadShortcutRouter a, b;
    for (int button : {SDL_GAMEPAD_BUTTON_GUIDE, SDL_GAMEPAD_BUTTON_LEFT_STICK, SDL_GAMEPAD_BUTTON_START, SDL_GAMEPAD_BUTTON_BACK}) {
        Button(a, button, true); Check(!Update(a, loaded)[menu] && !Update(a, loaded)[kPadMenuAlt] && !Update(a, loaded)[kPadMenuChord], "cleared menu does not open");
    }
    keys = DefaultPadHotkeys(); a = {}; b = {};
    {
        auto remap = keys;
        BindPadHotkey(remap, int(HotkeyAction::Reset), PadChordFromText("guide"));
        Check(remap[kPadMenuAlt].empty() && remap[int(HotkeyAction::Reset)] == PadChordFromText("guide"), "reusing Guide for reset removes conflicting menu access");
        BindPadHotkey(remap, pause, PadChordFromText("rightstick"));
        Check(remap[chat] == PadChordFromText("rightstick"), "remapping pause preserves online-only chat");
        BindPadHotkey(remap, menu, PadChordFromText("leftstick"));
        Check(remap[kPadZoomHold] == PadChordFromText("leftstick"), "tap and camera hold pairing retained");
    }
    Button(a, SDL_GAMEPAD_BUTTON_GUIDE, true);
    Check(Update(a, keys)[kPadMenuAlt], "Guide default opens menu");
    Check(!Update(a, keys)[kPadMenuAlt], "held Guide does not repeat");
    {
        auto duplicate = keys; duplicate[menu] = duplicate[kPadMenuAlt];
        PadShortcutRouter same; Button(same, SDL_GAMEPAD_BUTTON_GUIDE, true);
        const auto once = Update(same, duplicate);
        Check(once[menu] && !once[kPadMenuAlt] && once.count() == 1, "duplicate menu alternatives toggle only once");
    }
    Button(b, SDL_GAMEPAD_BUTTON_GUIDE, true); Check(Update(b, keys)[kPadMenuAlt], "second controller independent press");
    Button(a, SDL_GAMEPAD_BUTTON_GUIDE, false); Update(a, keys);
    Check(b.state.buttons[SDL_GAMEPAD_BUTTON_GUIDE] && !Update(b, keys)[kPadMenuAlt], "cross-device release does not cancel held state");
    a = {}; Button(a, SDL_GAMEPAD_BUTTON_LEFT_STICK, true);
    Check(Update(a, keys)[menu], "stick without camera opens immediately");
    a = {}; PadShortcutContext zoom{}; zoom.zoom = true;
    Button(a, SDL_GAMEPAD_BUTTON_LEFT_STICK, true); Check(!Update(a, keys, zoom)[menu], "camera tap waits for release");
    Button(a, SDL_GAMEPAD_BUTTON_LEFT_STICK, false); Check(Update(a, keys, zoom)[menu], "camera tap opens on release");
    a = {}; Button(a, SDL_GAMEPAD_BUTTON_LEFT_STICK, true); Update(a, keys, zoom);
    zoom.menu = true; Button(a, SDL_GAMEPAD_BUTTON_LEFT_STICK, false);
    Check(!Update(a, keys, zoom)[menu], "pending camera tap does not close a menu opened by another input");
    zoom.menu = false;
    a = {}; Button(a, SDL_GAMEPAD_BUTTON_LEFT_STICK, true); Update(a, keys, zoom);
    Button(a, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, true);
    Check(Update(a, keys, zoom)[int(HotkeyAction::ZoomPlayer1)], "zoom combination fires");
    Button(a, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, false); Check(!Update(a, keys, zoom)[menu], "chord release does not turn into menu press");
    Button(a, SDL_GAMEPAD_BUTTON_LEFT_STICK, false); Check(!Update(a, keys, zoom)[menu], "camera chord cancels menu tap");
    a = {}; Button(a, SDL_GAMEPAD_BUTTON_LEFT_STICK, true); Update(a, keys, zoom);
    a.state.axes[SDL_GAMEPAD_AXIS_LEFTX] = 20000; Update(a, keys, zoom);
    a.state.axes[SDL_GAMEPAD_AXIS_LEFTX] = 0; Update(a, keys, zoom);
    Button(a, SDL_GAMEPAD_BUTTON_LEFT_STICK, false); Check(!Update(a, keys, zoom)[menu], "panning cancels menu tap");
    a = {}; Button(a, SDL_GAMEPAD_BUTTON_START, true); Check(!Update(a, keys)[kPadMenuChord], "single Start does not open menu");
    Button(a, SDL_GAMEPAD_BUTTON_BACK, true); Check(Update(a, keys)[kPadMenuChord], "Start+Back opens once");
    Check(!Update(a, keys)[kPadMenuChord], "held combination does not repeat");
    a = {}; Button(a, SDL_GAMEPAD_BUTTON_RIGHT_STICK, true); auto offline = Update(a, keys);
    Check(offline[pause] && !offline[chat], "offline right stick pauses without chat");
    a = {}; Button(a, SDL_GAMEPAD_BUTTON_RIGHT_STICK, true); PadShortcutContext online{}; online.online = true;
    auto net = Update(a, keys, online); Check(net[chat] && !net[pause], "online right stick chats without pause");
    a = {}; keys[menu] = PadChordFromText("righttrigger"); keys[kPadZoomHold].clear();
    a.state.axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER] = 16000; Check(!Update(a, keys)[menu], "trigger threshold");
    a.state.axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER] = 16001; Check(Update(a, keys)[menu], "remap menu to trigger");
    keys[menu] = PadChordFromText("rightx-"); a = {};
    a.state.axes[SDL_GAMEPAD_AXIS_RIGHTX] = -20000; Check(Update(a, keys)[menu], "signed right stick shortcut");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    Check(SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_EVENTS), "native SDL initialized");
    SDL_VirtualJoystickDesc desc{}; SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD; desc.naxes = SDL_GAMEPAD_AXIS_COUNT; desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.axis_mask = (1u << SDL_GAMEPAD_AXIS_COUNT) - 1; desc.button_mask = (1u << SDL_GAMEPAD_BUTTON_COUNT) - 1;
    desc.name = "Input Lab global shortcuts regression";
    const auto id1 = SDL_AttachVirtualJoystick(&desc), id2 = SDL_AttachVirtualJoystick(&desc);
    auto *pad1 = SDL_OpenGamepad(id1), *pad2 = SDL_OpenGamepad(id2);
    Check(pad1 && pad2, "two native virtual gamepads opened");
    auto *joy1 = SDL_GetGamepadJoystick(pad1), *joy2 = SDL_GetGamepadJoystick(pad2);
    SDL_SetJoystickVirtualAxis(joy1, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, -32768);
    SDL_SetJoystickVirtualAxis(joy1, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, -32768);
    SDL_SetJoystickVirtualAxis(joy2, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, -32768);
    SDL_SetJoystickVirtualAxis(joy2, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, -32768);
    SDL_UpdateGamepads();
    keys = DefaultPadHotkeys(); a = {}; b = {};
    a.state.Poll(pad1); b.state.Poll(pad2); Update(a, keys); Update(b, keys);
    for (auto *joy : {joy1, joy2}) Check(SDL_SetJoystickVirtualButton(joy, SDL_GAMEPAD_BUTTON_GUIDE, true), "native Guide pressed");
    SDL_UpdateGamepads(); a.state.Poll(pad1); b.state.Poll(pad2);
    Check(Update(a, keys)[kPadMenuAlt] && Update(b, keys)[kPadMenuAlt], "native Guide opens on each controller");
    SDL_SetJoystickVirtualButton(joy1, SDL_GAMEPAD_BUTTON_GUIDE, false); SDL_UpdateGamepads();
    a.state.Poll(pad1); b.state.Poll(pad2);
    Check(!a.state.buttons[SDL_GAMEPAD_BUTTON_GUIDE] && b.state.buttons[SDL_GAMEPAD_BUTTON_GUIDE], "native cross-release isolated");
    Update(a, keys); Update(b, keys);
    keys[menu].clear(); keys[kPadMenuAlt].clear(); keys[kPadMenuChord].clear();
    SDL_SetJoystickVirtualButton(joy1, SDL_GAMEPAD_BUTTON_GUIDE, true);
    SDL_SetJoystickVirtualButton(joy1, SDL_GAMEPAD_BUTTON_LEFT_STICK, true);
    SDL_SetJoystickVirtualButton(joy1, SDL_GAMEPAD_BUTTON_START, true);
    SDL_SetJoystickVirtualButton(joy1, SDL_GAMEPAD_BUTTON_BACK, true);
    SDL_UpdateGamepads(); a.state.Poll(pad1);
    auto cleared = Update(a, keys);
    Check(!cleared[menu] && !cleared[kPadMenuAlt] && !cleared[kPadMenuChord], "native cleared inputs have no menu action");
    SDL_CloseGamepad(pad1); SDL_CloseGamepad(pad2); SDL_DetachVirtualJoystick(id1); SDL_DetachVirtualJoystick(id2); SDL_Quit();
    std::filesystem::remove(path);
    std::cout << "PASS: " << checks << " gamepad shortcut routing/persistence checks, including two native SDL virtual controllers.\n";
    return 0;
} catch (const std::exception &e) { std::cerr << "FAIL after " << checks << ": " << e.what() << '\n'; return 1; }
