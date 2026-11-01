// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// The in-game menu (menu.h): the shared menu told what a CPS-2 game's is made of.
#include "menu.h"
#include "input_profile.h"
#include "bindings.h"
#include "tuning.h"
#include "equalizer.h"

#include <cstring>
#include <cmath>

#include "../machine/machine.h"
#include "../runtime/game.h"

#include <algorithm>
#include <iterator>

namespace port {

namespace {
    // The first page's own rows
    enum { kCoin = 1, kStart2, kReset, kTest, kResetOptions };

    // The speeds, slowest first, each about 1.4 times the one before: the game runs `frames` frames in `ticks` of
    // the screen's
    struct Speed {
        const char *name;
        int frames, ticks;
    };
    const Speed kSpeeds[] = {{"1/8", 1, 8}, {"1/6", 1, 6}, {"1/4", 1, 4}, {"1/3", 1, 3}, {"1/2", 1, 2}, {"3/4", 3, 4}, {"Full", 1, 1},
                             {"1.5x", 3, 2}, {"2x", 2, 1}, {"3x", 3, 1}, {"4x", 4, 1}, {"6x", 6, 1}, {"8x", 8, 1}};
    constexpr int kFull = 6;
} // namespace

void Menu::Open(bool withStates) {
    const cps2recomp::Game &game = cps2recomp::TheGame();
    recomp::MenuConfig config;
    // Online, Display and Game first (issue #196), then the game's pages that belong at the head; then three groups
    // with a line between them: the save states, what the game offers, and leaving. Under Game: the speed, Mute, and
    // the Board, Cheats and Sound pages (a first page of fewer rows).
    config.groups.push_back({{recomp::MenuRow::Page, 0, "Online"}, {recomp::MenuRow::Page, 0, "Display"},
                             {recomp::MenuRow::Page, 0, "Game"}});
    config.pages.push_back({"Game", "", "", true, {recomp::MenuRow::Speed, recomp::MenuRow::Mute}});
    for (const char *under : {"Board", "Cheats", "Sound"}) {
        config.pages.push_back({under, "Game"});
    }
    std::vector<recomp::MenuRow> top;
    for (const cps2recomp::Page &page : game.pages) {
        // (such pages are ways of running the game: their rows do not count features that are on)
        config.pages.push_back({page.name, page.parent, page.description, false});
        if (page.top && !*page.parent) {
            top.push_back({recomp::MenuRow::Page, 0, page.name});
        }
    }
    if (!top.empty()) {
        config.groups.push_back(std::move(top));
    }
    if (withStates) {
        config.groups.push_back({{recomp::MenuRow::Save}, {recomp::MenuRow::Load}, {recomp::MenuRow::Slot}});
    }
    config.groups.push_back({
        {recomp::MenuRow::Pages},
        {recomp::MenuRow::Page, 0, "Controllers"},
        {recomp::MenuRow::Page, 0, "Slider controls"},
        {recomp::MenuRow::Action, kCoin, "Insert coin", "Puts a coin in the first slot."},
        {recomp::MenuRow::Action, kStart2, "Player 2 joins",
         "Presses player 2's Start: with a coin in (or free play), a second player to practise against."},
        {recomp::MenuRow::Action, kTest, "Test menu",
         "The game's own operator menu (the board's test switch, F2): its settings, such as difficulty and rounds. "
         "Its Exit row goes back."},
    });
    config.groups.push_back({
        {recomp::MenuRow::Action, kResetOptions, "Reset all options",
         "Puts every option back as it first was (the launcher's Enhanced preset), and the speed to full. The game's own "
         "settings (its Test menu) and the controls stay as they are."},
        {recomp::MenuRow::Action, kReset, "Reset game", "Starts the board again, as if switched off and on."},
        {recomp::MenuRow::Quit},
        {recomp::MenuRow::Resume},
    });
    for (const cps2recomp::Feature &feature : game.features) {
        config.features.push_back({feature.category, feature.title, feature.description, {feature.choices.begin(), feature.choices.end()}});
    }
    for (const cps2recomp::Command &command : game.commands) {
        if (hosting && !strcmp(command.name, "online_host")) {
            config.commands.push_back({command.category, "Stop hosting",
                                       "Others can no longer join: the match's code stops working. Your game goes on as it is."});
            continue;
        }
        config.commands.push_back({command.category, command.title, command.description});
    }
    LoadCurrentInputProfile(cps2recomp::SettingsPath());
    const size_t gameFeatures = config.features.size();
    constexpr size_t fieldsPerPlayer = 18; // neutral, delay, 12 autofire switches, 4 masks
    config.pages.push_back({"Controllers", "", "Input options for this game only. Assign keys and gamepad buttons on the launcher's Controllers page.", false});
    std::vector<std::string> delayNames, combinations;
    for (int n = 1; n <= 30; n++) delayNames.push_back(std::to_string(n) + (n == 1 ? " frame" : " frames"));
    for (unsigned mask = 0; mask < 64; mask++) combinations.push_back(InputMaskName(mask));
    for (int player = 0; player < players; player++) {
        const std::string page = "Player " + std::to_string(player + 1) + " input";
        config.pages.push_back({page, "Controllers", "Settings are separate for this game and player. Custom buttons press the selected attacks simultaneously.", false});
        config.features.push_back({page, "Neutralize Opposing Directions (MAME)",
            "Cancels Left+Right and Up+Down. Left+Down+Right leaves Down."});
        config.features.push_back({page, "Autofire delay", "This many game frames pressed, then the same number released. The first press is immediate.", delayNames});
        for (int action = 0; action < kInputAttackActions; action++)
            config.features.push_back({page, std::string("Autofire: ") + kInputActionTitles[action], "Repeats this button or simultaneous combination while its trigger is held."});
        for (int custom = 0; custom < kInputCustomCount; custom++)
            config.features.push_back({page, "Custom " + std::to_string(custom + 1), "Select the buttons pressed together by this Custom trigger. Its key and gamepad binding are in the launcher.", combinations});
    }
    LoadCurrentEqualizer(cps2recomp::SettingsPath());
    const size_t eqFeatures = config.features.size();
    config.features.push_back({"Equalizer", "Mode", "Bypass leaves the original audio unchanged. Active applies this game's five-band equalizer.", {"Bypass", "Active"}});
    config.features.push_back({"Equalizer", "Mode", "Low Band: Shelf changes the low end; Peak affects a band around the chosen frequency.", {"Shelf", "Peak"}});
    config.features.push_back({"Equalizer", "Mode", "High Band: Shelf changes the high end; Peak affects a band around the chosen frequency.", {"Shelf", "Peak"}});
    recomp::MenuPage eqPage{"Equalizer", "Sound", "Five-band MAMEPPK equalizer for this game only. Applies to board audio and custom soundtrack playback.", false};
    using Detail = recomp::MenuDetailRow;
    eqPage.details.push_back({Detail::Feature, eqFeatures});
    for (size_t band = 0; band < kEqBands; ++band) {
        eqPage.details.push_back({Detail::Heading, 0, kEqBandNames[band]});
        if (band == 0 || band == 4) eqPage.details.push_back({Detail::Feature, eqFeatures + (band == 0 ? 1 : 2)});
        const size_t slider = kTuneCount + band * 3;
        eqPage.details.push_back({Detail::Slider, slider});
        eqPage.details.push_back({Detail::Slider, slider + 1, "", [band] {
            return band == 0 ? !CurrentEqualizer().lowShelf : band == 4 ? !CurrentEqualizer().highShelf : true;
        }});
        eqPage.details.push_back({Detail::Slider, slider + 2});
    }
    eqPage.details.push_back({Detail::Command, game.commands.size() + 1});
    config.pages.push_back(std::move(eqPage));
    LoadCurrentTuning(cps2recomp::SettingsPath());
    config.pages.push_back({"Slider controls", "", "CPU clocks, video timing and picture controls for this game only.", false});
    for (const auto &info : kTuning) {
        config.sliders.push_back({"Slider controls", info.title, info.description, info.suffix,
                                  info.minimum, info.maximum, info.step, info.usual, info.decimals});
    }
    for (size_t band = 0; band < kEqBands; ++band) {
        const std::string name = kEqBandNames[band];
        config.sliders.push_back({"Equalizer", "Frequency", name + ": frequency. Clamped below Nyquist at playback's actual sample rate.", " Hz",
                                  kEqFrequencyMin[band], kEqFrequencyMax[band], 1, kEqFrequencyDefault[band], 0});
        config.sliders.push_back({"Equalizer", "Q factor", name + ": resonance / bandwidth. Only used in Peak mode.", "", 0.1, 10, 0.01, kEqDefaultQ, 2});
        config.sliders.push_back({"Equalizer", "Gain", name + ": boost or cut, from -12 to +12 dB. Enter restores 0 dB.", " dB", -12, 12, 0.1, 0, 1, true});
    }
    config.sliderValue = [](size_t index) { return index < kTuneCount ? CurrentTuning().values[index] : EqualizerSlider(index - kTuneCount); };
    config.sliderStep = [](size_t index, double old, int direction, bool coarse) {
        if (index >= kTuneCount && (index - kTuneCount) % 3 == 0) return EqualizerFrequencyStep((index - kTuneCount) / 3, old, direction, coarse);
        const double unit = index < kTuneCount ? kTuning[index].step : (index - kTuneCount) % 3 == 1 ? 0.01 : 0.1;
        return std::round((old + direction * unit * (coarse ? 10 : 1)) / unit) * unit;
    };
    config.setSlider = [this](size_t index, double value) {
        if (index >= kTuneCount) {
            if (!SetEqualizerSlider(index - kTuneCount, value)) m_menu.Note("Could not save this game's equalizer.");
            return;
        }
        if (!SetCurrentTuning(index, value)) m_menu.Note("Could not save this game's sliders.");
        else if (index == static_cast<size_t>(Tune::AudioCpu) && !audioCpuLive && value != 100)
            m_menu.Note("Reset game to apply the audio CPU clock to this older driver-based save state.");
    };
    config.commands.push_back({"Slider controls", "Reset sliders", "Restores the six sliders for this game only. Controls, cheats and global shortcuts are preserved."});
    config.commands.push_back({"Equalizer", "Reset All", "Restores Active, Shelf bass/treble, the five default frequencies, Q 0.71 and zero gain for this game only."});
    config.value = [gameFeatures, eqFeatures](size_t feature) {
        if (feature >= eqFeatures) {
            const auto &eq = CurrentEqualizer();
            return feature == eqFeatures ? int(eq.active) : feature == eqFeatures + 1 ? int(!eq.lowShelf) : int(!eq.highShelf);
        }
        if (feature < gameFeatures) return cps2recomp::FeatureSetting(cps2recomp::TheGame().features[feature].name);
        const size_t player = (feature - gameFeatures) / fieldsPerPlayer;
        const size_t field = (feature - gameFeatures) % fieldsPerPlayer;
        const auto &one = CurrentInputProfile().players[player];
        if (field == 0) return one.neutralize ? 1 : 0;
        if (field == 1) return one.autofireDelay - 1;
        if (field < 14) return one.autofire[field - 2] ? 1 : 0;
        return static_cast<int>(one.custom[field - 14]);
    };
    config.setValue = [this, gameFeatures, eqFeatures](size_t feature, int value) {
        if (feature >= eqFeatures) {
            if (!SetEqualizerFeature(feature - eqFeatures, value)) m_menu.Note("Could not save this game's equalizer.");
            return;
        }
        if (feature < gameFeatures) {
            cps2recomp::SetFeatureValue(cps2recomp::TheGame().features[feature].name, value);
            return;
        }
        const size_t player = (feature - gameFeatures) / fieldsPerPlayer;
        const size_t field = (feature - gameFeatures) % fieldsPerPlayer;
        auto &one = CurrentInputProfile().players[player];
        if (field == 0) one.neutralize = value != 0;
        else if (field == 1) one.autofireDelay = std::clamp(value + 1, 1, 30);
        else if (field < 14) one.autofire[field - 2] = value != 0;
        else one.custom[field - 14] = static_cast<uint8_t>(std::clamp(value, 0, 63));
        if (!SaveCurrentInputProfile()) m_menu.Note("Could not save this game's input settings.");
    };
    // Retain the official fast and slow speeds, and its favourite-choice filtering.
    {
        recomp::Ini settings(cps2recomp::SettingsPath()); settings.load();
        std::vector<std::vector<bool>> offered;
        for (const auto &info : cps2recomp::FeatureList()) offered.push_back(recomp::MenuChoices(settings, info));
        config.offered = [offered](size_t f, int choice) {
            return f >= offered.size() || choice < 0 || choice >= static_cast<int>(offered[f].size()) || offered[f][choice];
        };
    }
    Speeds(config);
    config.speedDescription = "Left/Right: slow motion. While playing, P (or a click of the right stick) stops the game and starts it "
                              "again; O (or, once stopped, the left trigger) goes on by one frame; - and = change the speed.";
    config.muted = [this] { return muted; };
    config.setMuted = [this](bool to) { muted = to; };
    config.confirmName = "Enter or A";
    config.closeName = "Esc";
    m_menu.Configure(std::move(config));
    m_menu.Open();
}

Menu::Menu() {
    // (so that - and = work before the menu is first opened)
    recomp::MenuConfig config;
    Speeds(config);
    m_menu.Configure(std::move(config));
}

void Menu::Speeds(recomp::MenuConfig &config) {
    for (const Speed &speed : kSpeeds) {
        config.speeds.push_back(speed.name);
    }
    config.normalSpeed = kFull;
}

Menu::Rate Menu::GameRate() const {
    const Speed &speed = kSpeeds[std::clamp(m_menu.Speed(), 0, static_cast<int>(std::size(kSpeeds)) - 1)];
    // (the slower of the player's choice and what the game's sources ask for: runtime/game.h)
    const int slowBy = cps2recomp::SlowMotion();
    if (slowBy > 1 && speed.frames * slowBy > speed.ticks) {
        return {1, slowBy};
    }
    return {speed.frames, speed.ticks};
}

void Menu::SetFullSpeed() {
    m_menu.SetSpeed(kFull);
}

void Menu::Faster(int by) {
    m_menu.SetSpeed(m_menu.Speed() + by);
}

Menu::Action Menu::Buttons(uint16_t pressed) {
    unsigned buttons = 0;
    buttons |= (pressed & cps2recomp::kUp) ? recomp::kMenuUp : 0;
    buttons |= (pressed & cps2recomp::kDown) ? recomp::kMenuDown : 0;
    buttons |= (pressed & cps2recomp::kLeft) ? recomp::kMenuLeft : 0;
    buttons |= (pressed & cps2recomp::kRight) ? recomp::kMenuRight : 0;
    if (pressed & (cps2recomp::kPunch1 | cps2recomp::kPunch2 | cps2recomp::kPunch3 | cps2recomp::kKick1 | cps2recomp::kKick2 |
                   cps2recomp::kKick3 | cps2recomp::kStart)) {
        buttons |= recomp::kMenuConfirm;
    }
    // (a press of something else, a coin button say, still takes the note away, as it always did)
    if (pressed && !buttons && m_menu.IsOpen()) {
        m_menu.Note("");
    }
    return Press(buttons);
}

Menu::Action Menu::Press(unsigned buttons) {
    const recomp::Menu::Chosen chosen = m_menu.Buttons(buttons);
    switch (chosen.kind) {
    case recomp::Menu::Chosen::Save: return Action::Save;
    case recomp::Menu::Chosen::Load: return Action::Load;
    case recomp::Menu::Chosen::Quit: return Action::Quit;
    case recomp::Menu::Chosen::Command:
        if (static_cast<size_t>(chosen.id) == cps2recomp::TheGame().commands.size() + 1) {
            if (!ResetCurrentEqualizer()) m_menu.Note("Could not save this game's equalizer.");
            m_menu.Refresh();
            return Action::None;
        }
        if (static_cast<size_t>(chosen.id) == cps2recomp::TheGame().commands.size()) {
            CurrentTuning() = Tuning{};
            if (!SaveCurrentTuning()) m_menu.Note("Could not save this game's sliders.");
            return Action::None;
        }
        m_command = static_cast<size_t>(chosen.id);
        return Action::Command;
    case recomp::Menu::Chosen::Action:
        return chosen.id == kCoin           ? Action::Coin
               : chosen.id == kStart2       ? Action::Start2
               : chosen.id == kTest         ? Action::Test
               : chosen.id == kResetOptions ? Action::ResetOptions
                                            : Action::Reset;
    default: return Action::None;
    }
}

} // namespace port
