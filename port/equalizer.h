// SPDX-License-Identifier: BSD-3-Clause
// Five-band DSP adapted from MAME src/emu/audio_effects/eq.{h,cpp}.
// Copyright (c) Olivier Galibert. Adaptation copyright (c) 2026 Input Lab contributors.
// See licenses/mame-equalizer.txt for the complete notice and license.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace port {
inline constexpr int kEqBands = 5;
inline constexpr double kEqDefaultQ = 0.7071067;
inline constexpr const char *kEqBandNames[] = {"Low Band", "Low Mid Band", "Mid Band", "High Mid Band", "High Band"};
inline constexpr double kEqFrequencyDefault[] = {100, 330, 1000, 3300, 10000};
inline constexpr double kEqFrequencyMin[] = {20, 100, 100, 100, 500};
inline constexpr double kEqFrequencyMax[] = {2000, 10000, 10000, 10000, 16000};
struct EqualizerSettings {
    bool active = true, lowShelf = true, highShelf = true;
    std::array<double, kEqBands> frequency{100, 330, 1000, 3300, 10000};
    std::array<double, kEqBands> q{kEqDefaultQ, kEqDefaultQ, kEqDefaultQ, kEqDefaultQ, kEqDefaultQ};
    std::array<double, kEqBands> gain{};
    bool operator==(const EqualizerSettings &) const = default;
};
EqualizerSettings NormalizeEqualizer(EqualizerSettings settings);
EqualizerSettings LoadEqualizer(const std::string &path);
bool SaveEqualizer(const std::string &path, const EqualizerSettings &settings);
EqualizerSettings &CurrentEqualizer();
void LoadCurrentEqualizer(const std::string &path);
bool SaveCurrentEqualizer();
bool SetEqualizerFeature(size_t index, int value); // active, low Shelf/Peak, high Shelf/Peak
bool SetEqualizerSlider(size_t index, double value); // band * 3 + frequency/Q/gain
bool ResetCurrentEqualizer();
double EqualizerSlider(size_t index);
double EqualizerFrequencyStep(size_t band, double old, int direction, bool coarse);

// A presentation-only processor. Its histories are deliberately absent from
// machine state, rollback, input recordings and network hashes.
class Equalizer {
public:
    void Reset();
    void ProcessS16(const std::vector<int16_t> &source, std::vector<float> &dest, int rate, const EqualizerSettings &settings);
    void ProcessFloat(std::vector<float> &stereo, int rate, const EqualizerSettings &settings);
    static int16_t Quantize(float sample);
private:
    struct Filter { float a1 = 0, a2 = 0, b0 = 1, b1 = 0, b2 = 0; };
    struct History { float v0 = 0, v1 = 0, v2 = 0; void Push(float v) { v2 = v1; v1 = v0; v0 = v; } };
    std::array<Filter, kEqBands> filters{};
    std::array<std::array<History, kEqBands + 1>, 2> history{};
    EqualizerSettings previous;
    int sampleRate = 0;
    bool filtering = false;
    void Configure(int rate, const EqualizerSettings &settings);
    void Build(int band);
    float Sample(float value, int channel);
};
} // namespace port
