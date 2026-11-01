// SPDX-License-Identifier: BSD-3-Clause
// DSP adapted from MAME src/emu/audio_effects/eq.cpp, copyright Olivier Galibert.
// Adaptation and per-game settings copyright (c) 2026 Input Lab contributors.
// The complete BSD-3-Clause notice is in licenses/mame-equalizer.txt.
#include "equalizer.h"
#include "../recomp-common/settings.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <locale>
#include <numbers>
#include <sstream>

namespace port {
namespace {
EqualizerSettings current;
std::string currentPath;
double Finite(double value, double usual, double low, double high) {
    return std::isfinite(value) ? std::clamp(value, low, high) : usual;
}
double Read(const recomp::Ini &ini, const std::string &key, double usual) {
    const auto text = ini.get("equalizer", key);
    double value = usual;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size() && std::isfinite(value) ? value : usual;
}
std::string Number(double value) {
    std::ostringstream text; text.imbue(std::locale::classic()); text << std::setprecision(15) << value; return text.str();
}
}
EqualizerSettings NormalizeEqualizer(EqualizerSettings settings) {
    for (int n = 0; n < kEqBands; ++n) {
        settings.frequency[n] = std::round(Finite(settings.frequency[n], kEqFrequencyDefault[n], kEqFrequencyMin[n], kEqFrequencyMax[n]));
        settings.q[n] = Finite(settings.q[n], kEqDefaultQ, 0.1, 10);
        settings.gain[n] = std::round(Finite(settings.gain[n], 0, -12, 12) * 10) / 10;
    }
    return settings;
}
EqualizerSettings LoadEqualizer(const std::string &path) {
    recomp::Ini ini(path); ini.load(); EqualizerSettings result;
    result.active = Read(ini, "mode", 1) != 0;
    result.lowShelf = Read(ini, "low_shelf", 1) != 0;
    result.highShelf = Read(ini, "high_shelf", 1) != 0;
    for (int n = 0; n < kEqBands; ++n) {
        const auto suffix = std::to_string(n + 1);
        result.frequency[n] = Read(ini, "f" + suffix, result.frequency[n]);
        result.q[n] = Read(ini, "q" + suffix, result.q[n]);
        result.gain[n] = Read(ini, "db" + suffix, result.gain[n]);
    }
    return NormalizeEqualizer(result);
}
bool SaveEqualizer(const std::string &path, const EqualizerSettings &settings) {
    if (path.empty()) return false;
    const auto chosen = NormalizeEqualizer(settings);
    recomp::Ini ini(path); ini.load(); // Preserve game features, controls, sliders and unknown keys.
    ini.set("equalizer", "mode", chosen.active ? "1" : "0");
    ini.set("equalizer", "low_shelf", chosen.lowShelf ? "1" : "0");
    ini.set("equalizer", "high_shelf", chosen.highShelf ? "1" : "0");
    for (int n = 0; n < kEqBands; ++n) {
        const auto suffix = std::to_string(n + 1);
        ini.set("equalizer", "f" + suffix, Number(chosen.frequency[n]));
        ini.set("equalizer", "q" + suffix, Number(chosen.q[n]));
        ini.set("equalizer", "db" + suffix, Number(chosen.gain[n]));
    }
    return ini.save();
}
EqualizerSettings &CurrentEqualizer() { return current; }
void LoadCurrentEqualizer(const std::string &path) { currentPath = path; current = LoadEqualizer(path); }
bool SaveCurrentEqualizer() { current = NormalizeEqualizer(current); return SaveEqualizer(currentPath, current); }
bool ResetCurrentEqualizer() { current = {}; return SaveCurrentEqualizer(); }
bool SetEqualizerFeature(size_t index, int value) {
    if (index == 0) current.active = value != 0;
    else if (index == 1) current.lowShelf = value == 0;
    else if (index == 2) current.highShelf = value == 0;
    else return false;
    return SaveCurrentEqualizer();
}
bool SetEqualizerSlider(size_t index, double value) {
    if (index >= kEqBands * 3) return false;
    (index % 3 == 0 ? current.frequency : index % 3 == 1 ? current.q : current.gain)[index / 3] = value;
    return SaveCurrentEqualizer();
}
double EqualizerSlider(size_t index) {
    if (index >= kEqBands * 3) return 0;
    return (index % 3 == 0 ? current.frequency : index % 3 == 1 ? current.q : current.gain)[index / 3];
}
double EqualizerFrequencyStep(size_t band, double old, int direction, bool coarse) {
    // MAME's normal pseudo-logarithmic stepping, with Shift for coarse changes.
    double step = direction > 0 ? (old >= 5000 ? 1000 : old >= 2500 ? 500 : old >= 1000 ? 250 : old >= 500 ? 100 : old >= 100 ? 50 : 10)
                                : (old <= 100 ? 10 : old <= 500 ? 50 : old <= 1000 ? 100 : old <= 2500 ? 250 : old <= 5000 ? 500 : 1000);
    if (coarse) step *= 10;
    const double grid = direction > 0 ? std::floor(old / step) + 1 : std::ceil(old / step) - 1;
    return std::clamp(grid * step, kEqFrequencyMin[band], kEqFrequencyMax[band]);
}
void Equalizer::Reset() { history = {}; sampleRate = 0; }
void Equalizer::Configure(int rate, const EqualizerSettings &settings) {
    const auto normalized = NormalizeEqualizer(settings);
    rate = std::max(rate, 4);
    if (rate == sampleRate && normalized == previous) return;
    previous = normalized; sampleRate = rate; history = {};
    filtering = previous.active && std::any_of(previous.gain.begin(), previous.gain.end(), [](double db) { return db != 0; });
    for (int n = 0; n < kEqBands; ++n) Build(n);
}
void Equalizer::Build(int band) {
    auto &fi = filters[band]; fi = {};
    if (previous.gain[band] == 0) return;
    const float db = static_cast<float>(previous.gain[band]);
    const float f = std::clamp(float(previous.frequency[band]), 1.0f, sampleRate / 2.0f - 1.0f);
    const float K = std::tan(std::numbers::pi * f / sampleRate), K2 = K * K;
    constexpr double SQRT2 = std::numbers::sqrt2;
    // The same Zölzer peak/shelf coefficients as MAMEPPK's eq.cpp.
    if (band == 0 && previous.lowShelf) {
        const float V = std::pow(10, std::abs(db) / 20);
        if (db > 0) {
            const float d = 1 + SQRT2 * K + K2;
            fi.b0 = (1 + std::sqrt(2 * V) * K + V * K2) / d;
            fi.b1 = 2 * (V * K2 - 1) / d;
            fi.b2 = (1 - std::sqrt(2 * V) * K + V * K2) / d;
            fi.a1 = 2 * (K2 - 1) / d; fi.a2 = (1 - SQRT2 * K + K2) / d;
        } else {
            const float d = 1 + std::sqrt(2 * V) * K + V * K2;
            fi.b0 = (1 + SQRT2 * K + K2) / d; fi.b1 = 2 * (K2 - 1) / d;
            fi.b2 = (1 - SQRT2 * K + K2) / d;
            fi.a1 = 2 * (V * K2 - 1) / d; fi.a2 = (1 - std::sqrt(2 * V) * K + V * K2) / d;
        }
    } else if (band == 4 && previous.highShelf) {
        const float V = std::pow(10, db / 20);
        if (db > 0) {
            const float d = 1 + SQRT2 * K + K2;
            fi.b0 = (V + std::sqrt(2 * V) * K + K2) / d; fi.b1 = 2 * (K2 - V) / d;
            fi.b2 = (V - std::sqrt(2 * V) * K + K2) / d;
            fi.a1 = 2 * (K2 - 1) / d; fi.a2 = (1 - SQRT2 * K + K2) / d;
        } else {
            const float d = 1 + std::sqrt(2 * V) * K + V * K2;
            fi.b0 = V * (1 + SQRT2 * K + K2) / d; fi.b1 = 2 * V * (K2 - 1) / d;
            fi.b2 = V * (1 - SQRT2 * K + K2) / d;
            fi.a1 = 2 * (V * K2 - 1) / d; fi.a2 = (1 - std::sqrt(2 * V) * K + V * K2) / d;
        }
    } else {
        const float V = std::pow(10, db / 20), Q = previous.q[band];
        if (db > 0) {
            const float d = 1 + K / Q + K2;
            fi.b0 = (1 + V * K / Q + K2) / d; fi.b1 = 2 * (K2 - 1) / d;
            fi.b2 = (1 - V * K / Q + K2) / d; fi.a1 = fi.b1; fi.a2 = (1 - K / Q + K2) / d;
        } else {
            const float d = 1 + K / (V * Q) + K2;
            fi.b0 = (1 + K / Q + K2) / d; fi.b1 = 2 * (K2 - 1) / d;
            fi.b2 = (1 - K / Q + K2) / d; fi.a1 = fi.b1; fi.a2 = (1 - K / (V * Q) + K2) / d;
        }
    }
}
float Equalizer::Sample(float value, int channel) {
    if (!filtering) return value;
    auto &h = history[channel]; h[0].Push(value);
    for (int n = 0; n < kEqBands; ++n) {
        const auto &f = filters[n]; const auto &x = h[n]; auto &y = h[n + 1];
        y.Push(f.b0 * x.v0 + f.b1 * x.v1 + f.b2 * x.v2 - f.a1 * y.v0 - f.a2 * y.v1);
    }
    if (!std::isfinite(h.back().v0)) { h = {}; return 0; }
    return h.back().v0;
}
void Equalizer::ProcessS16(const std::vector<int16_t> &source, std::vector<float> &dest, int rate, const EqualizerSettings &settings) {
    Configure(rate, settings); dest.resize(source.size());
    for (size_t n = 0; n < source.size(); ++n) dest[n] = Sample(source[n] / 32768.0f, n % 2);
}
void Equalizer::ProcessFloat(std::vector<float> &stereo, int rate, const EqualizerSettings &settings) {
    Configure(rate, settings);
    for (size_t n = 0; n < stereo.size(); ++n) stereo[n] = Sample(stereo[n], n % 2);
}
int16_t Equalizer::Quantize(float sample) {
    if (!std::isfinite(sample)) return 0;
    return static_cast<int16_t>(std::lround(std::clamp(double(sample) * 32768.0, -32768.0, 32767.0)));
}
} // namespace port
