// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026. Local Input Lab extension to cps2recomp.
#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace port {

inline constexpr int kInputPlayers = 4;
inline constexpr int kInputAttackActions = 12; // six buttons, x3 punches, x3 kicks, four custom combinations
inline constexpr int kInputCustomCount = 4;
inline constexpr const char *kInputButtonNames[] = {"LP", "MP", "HP", "LK", "MK", "HK"};
inline constexpr const char *kInputActionNames[] = {"lp", "mp", "hp", "lk", "mk", "hk", "ppp", "kkk",
                                                   "custom1", "custom2", "custom3", "custom4"};
inline constexpr const char *kInputActionTitles[] = {"Light punch", "Medium punch", "Hard punch", "Light kick",
    "Medium kick", "Hard kick", "x3 punches", "x3 kicks", "Custom 1", "Custom 2", "Custom 3", "Custom 4"};

struct PlayerInputProfile {
    bool neutralize = false;
    int autofireDelay = 1; // emulated frames on, then the same number off; first press is immediate
    std::array<bool, kInputAttackActions> autofire{};
    std::array<uint8_t, kInputCustomCount> custom{}; // six-button masks, LP in bit 0
};
struct InputProfile {
    std::array<PlayerInputProfile, kInputPlayers> players{};
};

InputProfile LoadInputProfile(const std::string &path);
bool SaveInputProfile(const std::string &path, const InputProfile &profile);
std::string InputMaskName(unsigned mask);
uint16_t NeutralizeInputDirections(uint16_t buttons);

// Called only for host input, before a real Tick. Speculative run-ahead frames do not advance it.
class InputProcessor {
public:
    uint16_t Apply(int player, uint32_t raw, uint64_t frame, const InputProfile &profile);
    void Reset() { states_ = {}; }
private:
    struct State { bool held = false; uint64_t start = 0; };
    std::array<std::array<State, kInputAttackActions>, kInputPlayers> states_{};
};

InputProfile &CurrentInputProfile();
void LoadCurrentInputProfile(const std::string &path);
bool SaveCurrentInputProfile();

} // namespace port
