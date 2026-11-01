// SPDX-License-Identifier: GPL-3.0-only
// Opt-in diagnostic instrumentation. Never used as an input source.
#pragma once
#include "keyboards.h"
#include <cstdint>
namespace port {
bool InputTraceEnabled();
void InputTraceFrame(uint64_t frame);
void InputTraceConfigure(const KeyboardConfig &config);
void InputTraceDevice(const KeyboardState &state, const std::string &id);
void InputTraceNote(const char *kind, const std::string &detail);
void InputTraceRaw(const KeyboardState &state, const std::string &id, SDL_Scancode key,
                   unsigned make, unsigned flags, unsigned vk, bool down, bool repeat, uint32_t messageMs);
void InputTraceFocus(bool focused);
void InputTraceSDL(SDL_Scancode key, bool down, bool repeat, SDL_Keymod mods, uint32_t keyboard);
void InputTracePlayer(const KeyboardState &state, const KeyboardConfig &config, int player,
                      uint32_t keyboard, uint32_t gamepad, uint32_t suppressed, uint16_t output, bool menu, bool paused);
void InputTraceProfile(const InputProfile &profile, const Bindings &bindings);
void InputTraceGame(const char *stage, uint64_t frame, uint16_t one, uint16_t two,
                    uint16_t io0 = 0, uint16_t io1 = 0, uint16_t io2 = 0);
} // namespace port
