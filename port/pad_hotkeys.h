// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "hotkeys.h"
#include "../recomp-common/launcher/src/bindings.h"
#include <bitset>
#include <vector>

namespace port {
using PadChord = std::vector<recomp::PadInput>;
inline constexpr int kPadMenuAlt = kHotkeyCount, kPadMenuChord = kHotkeyCount + 1,
    kPadZoomHold = kHotkeyCount + 2, kPadMultiNext = kHotkeyCount + 3,
    kPadFastForward = kHotkeyCount + 4, kPadTag = kHotkeyCount + 5,
    kPadHotkeyCount = kHotkeyCount + 6;
using PadHotkeys = std::array<PadChord, kPadHotkeyCount>;
using PadHotkeyBits = std::bitset<kPadHotkeyCount>;
const char *PadHotkeyName(int index);
const char *PadHotkeyTitle(int index);
std::string PadChordText(const PadChord &chord);
PadChord PadChordFromText(const std::string &text);
std::string PadChordLabel(const PadChord &chord);
PadHotkeys DefaultPadHotkeys();
PadHotkeys LoadPadHotkeys(const std::string &path);
bool SavePadHotkeys(const std::string &path, const PadHotkeys &keys);
void BindPadHotkey(PadHotkeys &keys, int action, const PadChord &chord);

// An event-owned snapshot per physical controller, so taps within a single poll
// and releases from another controller cannot erase an edge.
struct PadShortcutState {
    std::array<bool, SDL_GAMEPAD_BUTTON_COUNT> buttons{};
    std::array<Sint16, SDL_GAMEPAD_AXIS_COUNT> axes{};
    void Event(const SDL_Event &event);
    void Poll(SDL_Gamepad *pad);
    bool Down(recomp::PadInput input) const;
    bool Down(const PadChord &chord) const;
};
bool PadChordContains(const PadChord &larger, const PadChord &smaller);
struct PadShortcutContext {
    bool online = false, paused = false, menu = false, multi = false, zoom = false;
};
bool PadShortcutEligible(int index, const PadShortcutContext &context);
struct PadShortcutEdges {
    PadHotkeyBits held;
    PadHotkeyBits Pressed(PadHotkeyBits now) { auto pressed = now & ~held; held = now; return pressed; }
};
struct PadShortcutRouter {
    PadShortcutState state;
    PadShortcutEdges edges;
    PadHotkeyBits deferred, used;
    PadHotkeyBits Update(const PadHotkeys &keys, PadHotkeyBits held, bool menuOpen);
};
} // namespace port
