// SPDX-License-Identifier: GPL-3.0-only
#include "../../port/equalizer.h"
#include "../../port/menu.h"
#include "../../port/tuning.h"
#include "../../port/png_write.h"
#include "../../runtime/game.h"
#include "../../recomp-common/settings.h"
#include "mame_equalizer_oracle.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <SDL3/SDL.h>
using namespace port;
namespace cps2recomp {
std::string testSettings;
Game &TheGame() { static Game game; return game; }
const std::string &SettingsPath() { return testSettings; }
int FeatureSetting(const char *) { return 0; }
void SetFeatureValue(const char *, int) {}
int SlowMotion() { return 1; }
std::vector<recomp::FeatureInfo> FeatureList() { return {}; }
}
static int checks = 0;
void Check(bool ok, const char *text) { ++checks; if (!ok) throw std::runtime_error(text); }
double ToneGain(EqualizerSettings settings, double frequency, int rate = 48000) {
    std::vector<float> tone(rate * 4);
    for (size_t n = 0; n < tone.size() / 2; ++n) tone[n * 2] = 0.01f * std::sin(2 * std::numbers::pi * frequency * n / rate);
    Equalizer eq; eq.ProcessFloat(tone, rate, settings);
    double energy = 0;
    for (size_t n = rate; n < tone.size() / 2; ++n) { energy += tone[n * 2] * tone[n * 2]; Check(tone[n * 2 + 1] == 0, "independent stereo histories"); }
    return 20 * std::log10(std::sqrt(energy / rate) / (0.01 / std::sqrt(2.0)));
}
int main(int argc, char **argv) try {
    const auto folder = argc > 1 ? std::filesystem::path(argv[1]) : std::filesystem::temp_directory_path() / "cps2-equalizer-tests";
    std::filesystem::create_directories(folder);
    const auto a = (folder / "game-a.ini").string(), b = (folder / "game-b.ini").string();
    { std::ofstream f(a); f << "[keyboard1]\nup=W\n[features]\ndamage_hits=2\n[sliders]\nmaincpu_percent=150\n"; }
    std::filesystem::remove(b);
    EqualizerSettings selected; selected.gain[0] = 5; selected.gain[4] = 2; selected.lowShelf = false; selected.q[0] = 1.23;
    Check(SaveEqualizer(a, selected), "save per-game equalizer");
    Check(LoadEqualizer(a) == selected, "all fields roundtrip");
    Check(LoadEqualizer(b) == EqualizerSettings{}, "other game retains default EQ");
    recomp::Ini ini(a); ini.load(); Check(ini.get("keyboard1", "up") == "W" && ini.get("features", "damage_hits") == "2" && ini.get("sliders", "maincpu_percent") == "150", "EQ save preserves controls, cheats and clocks");
    ini.set("equalizer", "f1", "nan"); ini.set("equalizer", "q2", "999"); ini.set("equalizer", "db3", "-999"); ini.set("equalizer", "db4", "2,5"); ini.save();
    auto bounded = LoadEqualizer(a);
    Check(bounded.frequency[0] == 100 && bounded.q[1] == 10 && bounded.gain[2] == -12 && bounded.gain[3] == 0, "invalid values and ranges sanitized");
    Check(!SaveEqualizer("", selected), "no settings path cannot overwrite data");
    Check(EqualizerFrequencyStep(0, 100, 1, false) == 150 && EqualizerFrequencyStep(0, 100, -1, false) == 90, "MAME pseudo-log frequency stepping");
    std::vector<int16_t> fullRange(65536);
    for (int n = 0; n < 65536; ++n) fullRange[n] = static_cast<int16_t>(n - 32768);
    Equalizer eq; std::vector<float> converted;
    eq.ProcessS16(fullRange, converted, 24038, {});
    for (size_t n = 0; n < converted.size(); ++n) Check(Equalizer::Quantize(converted[n]) == fullRange[n], "flat EQ exact across all S16 values");
    auto bypass = selected; bypass.active = false;
    eq.ProcessS16(fullRange, converted, 24038, bypass);
    for (size_t n = 0; n < converted.size(); ++n) Check(Equalizer::Quantize(converted[n]) == fullRange[n], "Bypass is exact with nonzero configured gains");
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    Check(SDL_Init(SDL_INIT_AUDIO), "initialize isolated SDL audio driver");
    const SDL_AudioSpec pcm{SDL_AUDIO_S16, 2, 24038}, floating{SDL_AUDIO_F32, 2, 24038}, destination{SDL_AUDIO_F32, 2, 48000};
    auto *before = SDL_CreateAudioStream(&pcm, &destination), *after = SDL_CreateAudioStream(&floating, &destination);
    Check(before && after, "create old/new SDL conversion paths");
    Check(SDL_PutAudioStreamData(before, fullRange.data(), int(fullRange.size() * 2)) && SDL_PutAudioStreamData(after, converted.data(), int(converted.size() * 4)), "feed PCM and flat EQ paths");
    SDL_FlushAudioStream(before); SDL_FlushAudioStream(after);
    const int available = SDL_GetAudioStreamAvailable(before);
    Check(available == SDL_GetAudioStreamAvailable(after) && available > 0, "same resampled stream length");
    std::vector<uint8_t> pcmOutput(available), eqOutput(available);
    Check(SDL_GetAudioStreamData(before, pcmOutput.data(), available) == available && SDL_GetAudioStreamData(after, eqOutput.data(), available) == available && pcmOutput == eqOutput,
          "flat EQ matches original SDL audio stream conversion at device rate");
    SDL_DestroyAudioStream(before); SDL_DestroyAudioStream(after); SDL_Quit();
    for (int band = 0; band < 5; ++band) for (double db : {-6.0, 6.0}) {
        EqualizerSettings peak; peak.lowShelf = peak.highShelf = false; peak.gain[band] = db;
        Check(std::abs(ToneGain(peak, peak.frequency[band]) - db) < 0.035, "peak gain at centre frequency");
    }
    for (double db : {-6.0, 6.0}) {
        EqualizerSettings shelf; shelf.gain[0] = db;
        Check(std::abs(ToneGain(shelf, 20) - db) < 0.1, "low shelf asymptotic gain");
        shelf.gain[0] = 0; shelf.frequency[4] = 2000; shelf.gain[4] = db;
        Check(std::abs(ToneGain(shelf, 16000) - db) < 0.1, "high shelf asymptotic gain");
    }
    uint32_t random = 17;
    std::vector<float> signal(24038);
    for (auto &sample : signal) { random = random * 1664525u + 1013904223u; sample = (int(random >> 16) - 32768) / 65536.0f; }
    for (int rate : {8000, 24038, 44100, 48000, 96000}) for (int mode = 0; mode < 4; ++mode) {
        EqualizerSettings settings; settings.lowShelf = mode & 1; settings.highShelf = mode & 2;
        settings.gain = {5, -3, 2, -1, 2}; settings.q = {0.3, 0.71, 1.5, 3, 0.8};
        Equalizer port; auto actual = signal; port.ProcessFloat(actual, rate, settings);
        const auto expected = MameEqualizerOracle(rate, settings).Apply(signal);
        double difference = 0;
        for (size_t n = 0; n < actual.size(); ++n) { Check(std::isfinite(actual[n]), "finite output at all sample rates"); difference = std::max(difference, double(std::abs(actual[n] - expected[n]))); }
        Check(difference <= 0.000002, "matches original MAME coefficients and histories");
        Equalizer chunked; std::vector<float> blocks;
        for (size_t from = 0; from < signal.size(); from += 74) {
            std::vector<float> block(signal.begin() + from, signal.begin() + std::min(from + 74, signal.size()));
            chunked.ProcessFloat(block, rate, settings); blocks.insert(blocks.end(), block.begin(), block.end());
        }
        Check(blocks == actual, "block size does not change filtering");
    }
    EqualizerSettings extreme; extreme.gain.fill(12); extreme.q.fill(10); extreme.frequency = {20, 100, 10000, 10000, 16000}; extreme.lowShelf = extreme.highShelf = false;
    std::vector<float> impulse(300000); impulse[0] = 0.1;
    Equalizer stress; stress.ProcessFloat(impulse, 24038, extreme);
    for (float sample : impulse) Check(std::isfinite(sample) && std::abs(sample) < 1000, "extreme EQ remains stable");
    Check(Equalizer::Quantize(5) == 32767 && Equalizer::Quantize(-5) == -32768, "WAV peaks saturate without integer wrap");
    cps2recomp::testSettings = b;
    Menu menu; menu.Open(false); menu.Press(recomp::kMenuRight); // Game
    menu.Press(recomp::kMenuDown); menu.Press(recomp::kMenuDown); menu.Press(recomp::kMenuRight); // Sound
    menu.Press(recomp::kMenuRight); // Equalizer
    std::vector<uint32_t> picture(896 * 672, 0x00604020); menu.Draw(picture.data(), 896, 672);
    WritePng((folder / "equalizer-top.png").string().c_str(), picture.data(), 896, 672);
    menu.Press(recomp::kMenuLeft); Check(!LoadEqualizer(b).active, "Game -> Sound -> Equalizer Mode controls this game");
    menu.Press(recomp::kMenuRight); menu.Press(recomp::kMenuDown); menu.Press(recomp::kMenuRight);
    Check(!LoadEqualizer(b).lowShelf, "low band Shelf/Peak switch");
    menu.Press(recomp::kMenuDown); menu.Press(recomp::kMenuRight);
    Check(LoadEqualizer(b).frequency[0] == 150, "frequency changes via actual menu");
    menu.Press(recomp::kMenuDown); menu.Press(recomp::kMenuRight);
    Check(std::abs(LoadEqualizer(b).q[0] - 0.72) < 1e-10, "Peak exposes editable Q");
    menu.Press(recomp::kMenuDown); menu.Press(recomp::kMenuRight);
    Check(LoadEqualizer(b).gain[0] == 0.1, "gain changes and persists via actual menu");
    menu.Draw(picture.data(), 896, 672); WritePng((folder / "equalizer-peak.png").string().c_str(), picture.data(), 896, 672);
    for (int n = 0; n < 4; ++n) menu.Press(recomp::kMenuUp); // Mode, headings skipped
    menu.Press(recomp::kMenuUp); menu.Press(recomp::kMenuUp); menu.Press(recomp::kMenuConfirm); // Reset All
    Check(LoadEqualizer(b) == EqualizerSettings{}, "Reset All restores only this game's EQ");
    Check(LoadEqualizer(a).gain[0] == 5 && LoadTuning(a).Get(Tune::MainCpu) == 150, "reset preserves other game and unrelated settings");
    std::cout << "PASS: " << checks << " EQ checks; original MAME oracle, spectra, stereo, block continuity, limits, per-game persistence and menu.\n";
    return 0;
} catch (const std::exception &e) { std::cerr << "FAIL after " << checks << ": " << e.what() << '\n'; return 1; }
