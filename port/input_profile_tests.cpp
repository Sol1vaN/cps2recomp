// SPDX-License-Identifier: GPL-3.0-only
#include "input_profile.h"
#include "bindings.h"
#include "hotkeys.h"
#include "tuning.h"
#include <cmath>
#include <limits>
#include "window_layout.h"
#include "../games/sfa2/src/hidden_selection.h"
#include "../games/vampire_savior/src/hidden_selection.h"
#include "../games/puzzle_fighter/src/hidden_selection.h"
#include "menu.h"
#include "png_write.h"
#include "../runtime/game.h"
#include "../recomp-common/settings.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

static int checks = 0;
namespace cps2recomp {
    std::string testSettings;
    Game &TheGame() { static Game game; return game; }
    const std::string &SettingsPath() { return testSettings; }
    int FeatureSetting(const char *) { return 0; }
    void SetFeatureValue(const char *, int) {}
    int SlowMotion() { return 1; }
    std::vector<recomp::FeatureInfo> FeatureList() { return {}; }
}
static void Check(bool ok, const char *label) {
    ++checks;
    if (!ok) throw std::runtime_error(label);
}
int main() {
    using namespace port;
    try {
        InputProcessor input;
        InputProfile profile;
        for (unsigned raw = 0; raw < 4096; ++raw)
            Check(input.Apply(0, raw, raw, profile) == raw, "disabled options preserve all original bits");
        profile.players[0].neutralize = true;
        Check(input.Apply(0, 7, 0, profile) == 4, "Left+Down+Right becomes Down");
        Check(input.Apply(0, 15 | 0x410, 0, profile) == 0x410, "opposite pairs cancel without clearing attacks/start");
        Check(input.Apply(0, 6, 0, profile) == 6, "diagonals preserved");
        Check(input.Apply(1, 7, 0, profile) == 7, "player options independent");
        profile.players[0].autofire[0] = true;
        input.Reset();
        Check(input.Apply(0, 0x10, 100, profile) == 0x10, "autofire starts pressed immediately");
        Check(input.Apply(0, 0x10, 100, profile) == 0x10, "same frame is stable");
        Check(input.Apply(0, 0x10, 101, profile) == 0, "one-frame release");
        Check(input.Apply(0, 0x10, 102, profile) == 0x10, "one-frame repeat");
        input.Apply(0, 0, 103, profile);
        Check(input.Apply(0, 0x10, 103, profile) == 0x10, "new press restarts immediately");
        Check(input.Apply(0, 0x10, 50, profile) == 0x10, "rewinding restarts safely");
        profile.players[0].autofireDelay = 2;
        input.Reset();
        for (unsigned n = 0; n < 12; ++n)
            Check(input.Apply(0, 0x10, n, profile) == ((n / 2) % 2 ? 0 : 0x10), "delay in emulated frames");
        Check(input.Apply(0, 1u << 12, 0, profile) == 0x70, "x3 punches");
        Check(input.Apply(0, 1u << 13, 0, profile) == 0x380, "x3 kicks");
        Check(input.Apply(0, 1u << 14, 0, profile) == 0, "empty custom produces no buttons");
        profile.players[0].custom[0] = 5;
        Check(input.Apply(0, 1u << 14, 0, profile) == 0x50, "custom LP+HP");
        profile.players[0].autofire[8] = true;
        profile.players[0].autofire[0] = false;
        input.Reset();
        Check(input.Apply(0, (1u << 14) | 0x10, 0, profile) == 0x50, "overlapping custom and manual combine");
        Check(input.Apply(0, (1u << 14) | 0x10, 2, profile) == 0x10, "off custom cannot release manual LP");
        Check(input.Apply(0, 0x10, 3, profile) == 0x10, "releasing custom cannot release manual LP");
        Check(input.Apply(0, 1u << 17, 3, profile) == 0, "extended trigger never leaks into game mask");
        profile.players[0].custom[3] = 63;
        Check(input.Apply(0, 1u << 17, 3, profile) == 0x3F0, "Custom4 high trigger bit preserved");
        Check(InputMaskName(5) == "LP+HP" && InputMaskName(0) == "Off", "combination names");

        const auto folder = std::filesystem::current_path() / "input_test_settings";
        std::filesystem::create_directories(folder);
        const auto a = (folder / "sfa2.ini").string();
        const auto b = (folder / "puzzle.ini").string();
        { std::ofstream out(b); } // repeatable test setup; never read a prior test run's profile
        { std::ofstream out(a); out << "# retain comment\n[keyboard1]\nup=W\n[features]\npreset=custom\n"; }
        Check(SaveInputProfile(a, profile), "save profile");
        auto loaded = LoadInputProfile(a);
        Check(loaded.players[0].neutralize && loaded.players[0].custom[3] == 63 && loaded.players[0].autofire[8], "round trip");
        Check(!LoadInputProfile(b).players[0].neutralize, "other game unaffected");
        recomp::Ini ini(a); ini.load();
        Check(ini.get("keyboard1", "up") == "W" && ini.get("features", "preset") == "custom", "unrelated settings preserved");
        ini.set("input1", "autofire_delay", "999");
        ini.set("input1", "custom1", "-1");
        ini.set("input1", "custom2", "broken");
        ini.save();
        loaded = LoadInputProfile(a);
        Check(loaded.players[0].autofireDelay == 30 && loaded.players[0].custom[0] == 0 && loaded.players[0].custom[1] == 0, "invalid settings clamped safely");
        Check(!SaveInputProfile("", profile), "unspecified path cannot overwrite settings");
        const auto binds = LoadBindings(a);
        Check(binds.key[0][0] == SDL_SCANCODE_W, "existing key mapping preserved");
        Check(binds.key[0][12] == SDL_SCANCODE_I && binds.key[0][13] == SDL_SCANCODE_K, "default x3 shortcuts");
        Check(kBoundActionCount == 18 && kBoundActions[17].bit == (1u << 17), "all custom triggers bound");
        Check(binds.pad[0][12].button == -1 && binds.pad[0][12].axis == -1, "extra gamepad triggers default unbound");
        cps2recomp::testSettings = b;
        Menu menu;
        menu.Open(false);
        menu.Press(recomp::kMenuDown); // official Game page precedes Controllers
        menu.Press(recomp::kMenuRight); // Controllers on the main page
        menu.Press(recomp::kMenuRight); // Player 1
        menu.Press(recomp::kMenuConfirm); // Neutralize
        Check(LoadInputProfile(b).players[0].neutralize, "in-game Controllers page reachable and persists neutralization");
        menu.Press(recomp::kMenuDown);
        menu.Press(recomp::kMenuRight); // delay 2
        menu.Press(recomp::kMenuDown);
        menu.Press(recomp::kMenuConfirm); // LP autofire
        for (int n = 0; n < 12; ++n) menu.Press(recomp::kMenuDown);
        for (int n = 0; n < 3; ++n) menu.Press(recomp::kMenuRight); // Custom1 = LP+MP
        loaded = LoadInputProfile(b);
        Check(loaded.players[0].autofireDelay == 2 && loaded.players[0].autofire[0] && loaded.players[0].custom[0] == 3,
              "in-game autofire and custom combination persisted");
        std::vector<uint32_t> picture(896 * 672, 0x302820);
        menu.Draw(picture.data(), 896, 672);
        WritePng("in_game_input_menu.png", picture.data(), 896, 672);
        menu.Press(recomp::kMenuBack);
        menu.Press(recomp::kMenuDown);
        menu.Press(recomp::kMenuRight); // Player 2
        menu.Press(recomp::kMenuConfirm);
        Check(LoadInputProfile(b).players[1].neutralize && LoadInputProfile(b).players[0].custom[0] == 3,
              "in-game player 2 independent");
        Check(LoadInputProfile(a).players[0].custom[3] == 63, "in-game editing leaves other game intact");
        auto shortcuts = DefaultHotkeys();
        Check(FindHotkey(shortcuts, SDL_SCANCODE_F5, SDL_KMOD_NONE) == HotkeyAction::Save, "default save shortcut");
        Check(FindHotkey(shortcuts, SDL_SCANCODE_RETURN, SDL_KMOD_RALT) == HotkeyAction::FullscreenAlt, "alternate full screen chord");
        Check(FindHotkey(shortcuts, SDL_SCANCODE_RETURN, SDL_KMOD_NONE) == HotkeyAction::None, "plain Return stays a game control");
        Check(FindHotkey(shortcuts, SDL_SCANCODE_P, SDL_KMOD_NONE, true) == HotkeyAction::None, "pause ignores key repeat");
        Check(FindHotkey(shortcuts, SDL_SCANCODE_O, SDL_KMOD_NONE, true) == HotkeyAction::Step, "frame step can repeat");
        BindHotkey(shortcuts, static_cast<int>(HotkeyAction::Save), {SDL_SCANCODE_F8, 1});
        Check(FindHotkey(shortcuts, SDL_SCANCODE_F8, SDL_KMOD_LCTRL) == HotkeyAction::Save &&
              FindHotkey(shortcuts, SDL_SCANCODE_F8, SDL_KMOD_RCTRL) == HotkeyAction::Save, "side-independent Ctrl chord");
        Check(!HotkeyReserved(shortcuts, SDL_SCANCODE_F8, SDL_KMOD_NONE), "chord does not consume plain key");
        BindHotkey(shortcuts, static_cast<int>(HotkeyAction::Load), {SDL_SCANCODE_F8, 1});
        Check(shortcuts[static_cast<int>(HotkeyAction::Save)].key == SDL_SCANCODE_UNKNOWN, "duplicate shortcut moves to new action");
        BindHotkey(shortcuts, static_cast<int>(HotkeyAction::Load), {SDL_SCANCODE_ESCAPE});
        Check(shortcuts[static_cast<int>(HotkeyAction::Load)].key == SDL_SCANCODE_F8, "Escape cannot be assigned");
        for (auto &key : shortcuts) key = {};
        Check(FindHotkey(shortcuts, SDL_SCANCODE_ESCAPE, SDL_KMOD_CTRL) == HotkeyAction::Menu, "Escape remains fixed with all shortcuts cleared");
        Check(FindHotkey(shortcuts, SDL_SCANCODE_ESCAPE, SDL_KMOD_NONE, true) == HotkeyAction::None, "held Escape does not flicker menu");
        BindHotkey(shortcuts, static_cast<int>(HotkeyAction::Pause), {SDL_SCANCODE_F8, 3});
        const auto global = (folder / "hotkeys.ini").string();
        Check(SaveHotkeys(global, shortcuts), "global shortcuts save");
        const auto restored = LoadHotkeys(global);
        Check(FindHotkey(restored, SDL_SCANCODE_F8, SDL_KMOD_CTRL | SDL_KMOD_SHIFT) == HotkeyAction::Pause, "global shortcut round trip");
        Check(LoadInputProfile(a).players[0].custom[3] == 63 && LoadInputProfile(b).players[0].custom[0] == 3,
              "global shortcuts never modify game settings");
        Check(HotkeyLabel({SDL_SCANCODE_F5, 1}) == "Ctrl+F5", "shortcut label");
        BindHotkey(shortcuts, static_cast<int>(HotkeyAction::Pause), {SDL_SCANCODE_DELETE});
        Check(FindHotkey(shortcuts, SDL_SCANCODE_DELETE, SDL_KMOD_NONE) == HotkeyAction::Pause,
              "Delete is assignable; only Escape is reserved");
        const auto desktop = InitialLauncherBounds({0, 0, 2560, 1400});
        Check(desktop.w == 1440 && desktop.h > 1100 && desktop.x > 0 && desktop.y > 0 &&
              desktop.x + desktop.w < 2560 && desktop.y + desktop.h < 1400, "large default window leaves desktop margins");
        const auto otherMonitor = InitialLauncherBounds({-1920, 80, 1920, 1000});
        Check(otherMonitor.x > -1920 && otherMonitor.y > 80 && otherMonitor.x + otherMonitor.w < 0,
              "usable display bounds and offsets respected");
        for (const int character : {0,2,4,15,16}) {
            const int hidden = inputlab::ToggleAlpha2(character);
            Check(hidden != character && inputlab::ToggleAlpha2(hidden) == character, "Alpha 2 START toggles each variant and back");
        }
        Check(inputlab::ToggleAlpha2(1) == 1, "ordinary Alpha 2 characters unaffected");
        Check(inputlab::ToggleVampire(2,2) == 0x12 && inputlab::ToggleVampire(2,0x12) == 2, "Gallon toggle");
        Check(inputlab::ToggleVampire(8,8) == 0x18 && inputlab::ToggleVampire(8,0x18) == 8, "Bishamon toggle");
        for (int normal = 0; normal < 8; ++normal) {
            int choice = normal;
            for (const int expected : {8,9,10,normal}) {
                choice = inputlab::CyclePuzzle(normal,choice);
                Check(choice == expected, "Puzzle Fighter cycles all three hidden characters and back");
            }
        }
        const auto legacyKeys = (folder / "legacy_hotkeys.ini").string();
        { std::ofstream out(legacyKeys); out << "[hotkeys]\nmenu=Tab\nmenu_alt=F8\nmenu_alt_mods=1\n"; }
        auto migrated = LoadHotkeys(legacyKeys);
        Check(FindHotkey(migrated, SDL_SCANCODE_F8, SDL_KMOD_CTRL) == HotkeyAction::Reset, "legacy alternate menu binding becomes Reset game");
        Check(FindHotkey(migrated, SDL_SCANCODE_TAB, SDL_KMOD_NONE) == HotkeyAction::Menu, "primary menu binding retained");
        Check(FindHotkey(migrated, SDL_SCANCODE_F8, SDL_KMOD_CTRL, true) == HotkeyAction::None, "held Reset cannot repeatedly reboot");
        Check(FindHotkey(migrated, SDL_SCANCODE_ESCAPE, SDL_KMOD_NONE) == HotkeyAction::Menu, "Escape remains fixed after migration");
        Check(std::string(kHotkeys[static_cast<int>(HotkeyAction::Reset)].title) == "Reset game", "Reset game title");
        Check(DefaultHotkeys()[static_cast<int>(HotkeyAction::Reset)].key == SDL_SCANCODE_UNKNOWN, "new Reset binding starts unbound");
        Check(SaveHotkeys(legacyKeys, migrated) && LoadHotkeys(legacyKeys)[static_cast<int>(HotkeyAction::Reset)].mods == 1, "migrated Reset round trip");

        Tuning tune;
        tune.values = {151, 175, 65.25, 0.97, 1.05, 0.9};
        Check(SaveTuning(a, tune), "game sliders save");
        const auto restoredTune = LoadTuning(a);
        for (size_t i = 0; i < kTuneCount; ++i) Check(std::abs(restoredTune.values[i] - tune.values[i]) < 1e-10, "numeric slider precision round trip");
        Check(LoadTuning(b).values == Tuning{}.values, "other game's clocks and picture stay at default");
        recomp::Ini preserved(a); preserved.load();
        Check(preserved.get("keyboard1", "up") == "W" && LoadInputProfile(a).players[0].custom[3] == 63, "slider saving preserves bindings and Custom");
        preserved.set("sliders", "maincpu_percent", "999");
        preserved.set("sliders", "audiocpu_percent", "-25");
        preserved.set("sliders", "gamma", "nan");
        preserved.set("sliders", "refresh_hz", "infinity");
        preserved.set("sliders", "contrast", "1,2");
        preserved.save();
        auto bounded = LoadTuning(a);
        Check(bounded.Get(Tune::MainCpu) == 400 && bounded.Get(Tune::AudioCpu) == 25, "CPU slider limits");
        Check(bounded.Get(Tune::Gamma) == 1 && bounded.Get(Tune::Refresh) == kNativeRefreshHz && bounded.Get(Tune::Contrast) == 1, "invalid and non-finite slider values return to defaults");
        Check(!SaveTuning("", tune), "missing slider path never overwrites settings");
        PictureTuning colour;
        uint32_t pixels[] = {0xAA000000, 0xBB808080, 0xCCFFFFFF, 0xDD804020};
        colour.Apply(pixels, 4, Tuning{});
        Check(pixels[0] == 0xAA000000 && pixels[1] == 0xBB808080 && pixels[2] == 0xCCFFFFFF && pixels[3] == 0xDD804020, "default colour controls are bit-identical");
        Tuning black; black.values[3] = 0;
        colour.Apply(pixels, 4, black);
        Check(pixels[0] == 0xAA000000 && pixels[1] == 0xBB000000 && pixels[2] == 0xCC000000, "zero brightness is black without changing alpha");
        uint32_t grey[] = {0xFF000000, 0xFF808080, 0xFFFFFFFF};
        Tuning gamma; gamma.values[5] = 2;
        colour.Apply(grey, 3, gamma);
        Check(grey[0] == 0xFF000000 && grey[2] == 0xFFFFFFFF && (grey[1] & 255) > 128, "gamma changes midtones and keeps black and white");
        Tuning flat; flat.values[4] = 0;
        colour.Apply(grey, 3, flat);
        Check(grey[0] == 0xFF808080 && grey[1] == grey[0] && grey[2] == grey[0], "zero contrast maps all input shades to middle grey");

        cps2recomp::testSettings = b;
        Menu sliderMenu;
        sliderMenu.Open(false);
        sliderMenu.Press(recomp::kMenuDown); // official Game page precedes Controllers
        sliderMenu.Press(recomp::kMenuDown);
        sliderMenu.Press(recomp::kMenuRight); // actual Slider controls page
        sliderMenu.Press(recomp::kMenuRight);
        Check(LoadTuning(b).Get(Tune::MainCpu) == 100.1, "in-game slider changes and persists CPU clock");
        sliderMenu.Press(recomp::kMenuRight | recomp::kMenuFast);
        Check(LoadTuning(b).Get(Tune::MainCpu) == 101.1, "coarse slider step");
        sliderMenu.Press(recomp::kMenuConfirm);
        Check(LoadTuning(b).Get(Tune::MainCpu) == 100, "Enter restores a slider's native value");
        sliderMenu.Press(recomp::kMenuDown);
        sliderMenu.Press(recomp::kMenuRight);
        Check(LoadTuning(b).Get(Tune::AudioCpu) == 100.1, "audio CPU slider independent of main CPU");
        for (int i = 0; i < 350; ++i) sliderMenu.Press(recomp::kMenuRight | recomp::kMenuFast);
        Check(LoadTuning(b).Get(Tune::AudioCpu) == 400, "slider clamps without wrapping");
        std::vector<uint32_t> slidersPicture(896 * 672, 0x00604020);
        sliderMenu.Draw(slidersPicture.data(), 896, 672);
        WritePng("in_game_sliders.png", slidersPicture.data(), 896, 672);
        for (int i = 0; i < 5; ++i) sliderMenu.Press(recomp::kMenuDown);
        sliderMenu.Press(recomp::kMenuConfirm); // Reset sliders command, not Reset game
        const auto resetTune = LoadTuning(b);
        bool allDefault = true;
        for (size_t i = 0; i < kTuneCount; ++i) allDefault = allDefault && std::abs(resetTune.values[i] - kTuning[i].usual) < 1e-10;
        Check(allDefault, "Reset sliders restores only current game's six values");
        Check(LoadTuning(a).Get(Tune::MainCpu) == 400 && LoadInputProfile(b).players[0].custom[0] == 3, "Reset sliders preserves other game and input settings");

        std::cout << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAILED: " << error.what() << "\n";
        return 1;
    }
}
