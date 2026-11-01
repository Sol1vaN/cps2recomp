// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <SDL3/SDL.h>
#include <array>
#include <string>

namespace port {
enum class HotkeyAction { Menu, Reset, Pause, Step, Slower, Faster, Restart, Extra,
    Save, Load, PreviousSlot, NextSlot, Test, Fullscreen, FullscreenAlt, Service,
    Chat, ZoomPlayer1, ZoomPlayer2, ZoomBoth, Count, None };
inline constexpr int kHotkeyCount = static_cast<int>(HotkeyAction::Count);
struct Hotkey { SDL_Scancode key = SDL_SCANCODE_UNKNOWN; unsigned mods = 0; };
struct HotkeyInfo { const char *name; const char *title; bool repeat; Hotkey usual; };
inline const std::array<HotkeyInfo, kHotkeyCount> kHotkeys = {{
    {"menu", "Open / close menu", false, {SDL_SCANCODE_F1}},
    {"reset", "Reset game", false, {SDL_SCANCODE_UNKNOWN}},
    {"pause", "Pause / resume", false, {SDL_SCANCODE_P}},
    {"step", "Advance one frame", true, {SDL_SCANCODE_O}},
    {"slower", "Slower", true, {SDL_SCANCODE_MINUS}},
    {"faster", "Faster", true, {SDL_SCANCODE_EQUALS}},
    {"restart", "Start over (training)", false, {SDL_SCANCODE_BACKSPACE}},
    {"extra", "Game's extra training action", false, {SDL_SCANCODE_BACKSLASH}},
    {"save", "Save state", false, {SDL_SCANCODE_F5}},
    {"load", "Load state", false, {SDL_SCANCODE_F9}},
    {"previous_slot", "Previous state slot", true, {SDL_SCANCODE_F6}},
    {"next_slot", "Next state slot", true, {SDL_SCANCODE_F7}},
    {"test", "Operator's test switch", false, {SDL_SCANCODE_F2}},
    {"fullscreen", "Toggle full screen", false, {SDL_SCANCODE_F11}},
    {"fullscreen_alt", "Toggle full screen (alternate)", false, {SDL_SCANCODE_RETURN, 4}},
    {"service", "Service button (hold)", true, {SDL_SCANCODE_9}},
    {"chat", "Open chat (online)", false, {SDL_SCANCODE_T, 1}},
    {"zoom_player1", "Zoom: focus Player 1", false, {SDL_SCANCODE_Q, 2}},
    {"zoom_player2", "Zoom: focus Player 2", false, {SDL_SCANCODE_E, 2}},
    {"zoom_both", "Zoom: both fighters", false, {SDL_SCANCODE_W, 2}},
}};
// Compact, side-independent modifiers: Ctrl=1, Shift=2, Alt=4, GUI=8.
unsigned HotkeyModifiers(SDL_Keymod mods);
using Hotkeys = std::array<Hotkey, kHotkeyCount>;
Hotkeys DefaultHotkeys();
Hotkeys LoadHotkeys(const std::string &path);
bool SaveHotkeys(const std::string &path, const Hotkeys &keys);
void BindHotkey(Hotkeys &keys, int action, Hotkey key);
std::string HotkeyLabel(Hotkey key);
HotkeyAction FindHotkey(const Hotkeys &keys, SDL_Scancode key, SDL_Keymod mods, bool repeat = false);
bool HotkeyReserved(const Hotkeys &keys, SDL_Scancode key, SDL_Keymod mods);
bool HotkeyHeld(const Hotkeys &keys, HotkeyAction action, const bool *state, SDL_Keymod mods);
std::string DefaultHotkeysPath();
} // namespace port
