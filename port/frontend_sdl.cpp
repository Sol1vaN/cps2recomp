// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of cps2recomp.
//
// The window: SDL3 picture, sound and controls around Port::RunFrame().
#include "game_info.h"

#include "bindings.h"
#include "gamepad_assignment.h"
#include "input_profile.h"
#include "hotkeys.h"
#include "pad_hotkeys.h"
#include <map>
#include "raw_keyboards.h"
#include "input_trace.h"
#include "port.h"
#include "png_write.h"
#include "present.h"
#include "touch.h"

#include "../runtime/game.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>

using namespace cps2recomp;

namespace port {

#ifdef __APPLE__
void HandFrontToLauncher(); // (front_macos.mm)
#endif

namespace {

    // What each player's keys and gamepad buttons do: the player's choices (bindings.h; the launcher's
    // Controllers page), or the usual layout. A game started from the collection's launcher has the table every
    // game there shares; one with a launcher of its own, its own table.
    const BoundTable &Table() {
        static const BoundTable &table = MakeBoundTable(GamePlayers(), GameButtons(), InCollection());
        return table;
    }
    Bindings g_bindings = recomp::DefaultBindings(Table().table);
    Hotkeys g_hotkeys = DefaultHotkeys();
    PadHotkeys g_padHotkeys = DefaultPadHotkeys();
    PadShortcutContext g_padContext;
    GamepadAssignments g_gamepadAssignments;
    std::vector<SDL_Gamepad *> g_gamepads;
    int PadPlayer(SDL_Gamepad *gp) {
        return gp ? AssignedGamepadPlayer(SDL_GetGamepadID(gp), ResolveGamepadAssignments(g_gamepadAssignments, DescribeGamepads(g_gamepads))) : -1;
    }
    int Players() {
        return Table().table.players;
    }

    static_assert(kBoundActions[0].bit == kUp && kBoundActions[1].bit == kDown && kBoundActions[2].bit == kLeft &&
                  kBoundActions[3].bit == kRight && kBoundActions[4].bit == kPunch1 && kBoundActions[9].bit == kKick3 &&
                  kBoundActions[10].bit == kStart && kBoundActions[11].bit == kCoin &&
                  kBoundActions[12].bit == (1u << 12) && kBoundActions[17].bit == (1u << 17));

    constexpr int kDead = 16000; // how far a stick or a trigger must go to count

    bool KeyBound(SDL_Scancode key) {
        for (int player = 0; player < Players(); player++) {
            for (int action = 0; action < Table().table.count; action++) {
                if (g_bindings.key[player][action] == key) {
                    return true;
                }
            }
        }
        return false;
    }
    bool ButtonBound(SDL_GamepadButton button, int player) {
        return PlayerPadInputBound(g_bindings, player, {button, -1}, Table());
    }

    bool ShortcutGameBound(int n, int player) {
        const auto &chord = g_padHotkeys[n];
        return chord.size() == 1 && PlayerPadInputBound(g_bindings, player, chord.front(), Table());
    }
    bool ShortcutHeld(SDL_Gamepad *gp, int player, int n) {
        if (!PadShortcutEligible(n, g_padContext) || ShortcutGameBound(n, player)) return false;
        const auto &chord = g_padHotkeys[n];
        return !chord.empty() && std::all_of(chord.begin(), chord.end(), [&](auto input) { return recomp::PadInputPressed(gp, input); });
    }

    // The camera's zoom (zoom.h) is the left stick's click held and a shoulder pressed: left for player 1, right for
    // player 2, the right stick's click for both fighters at once; the d-pad (or the stick) moves the camera. While the
    // click is held (and is not the game's), the shoulders and the directions are not the game's. On the keyboard,
    // Left Shift held, Q, E and W, and the arrows.
    bool ZoomHeld(SDL_Gamepad *gp) {
        return gp && PadPlayer(gp) >= 0 && ShortcutHeld(gp, PadPlayer(gp), kPadZoomHold);
    }
    bool Shoulder(int button) {
        return button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER || button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
    }
    bool DPad(int button) {
        return button == SDL_GAMEPAD_BUTTON_DPAD_UP || button == SDL_GAMEPAD_BUTTON_DPAD_DOWN || button == SDL_GAMEPAD_BUTTON_DPAD_LEFT ||
               button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
    }
    bool KeyZoomHeld(const bool *keys) {
        return Zoom::Possible() && keys[SDL_SCANCODE_LSHIFT] && !KeyBound(SDL_SCANCODE_LSHIFT);
    }
    bool KeyZoom(SDL_Scancode key) {
        return key == SDL_SCANCODE_Q || key == SDL_SCANCODE_E || key == SDL_SCANCODE_W || key == SDL_SCANCODE_UP || key == SDL_SCANCODE_DOWN ||
               key == SDL_SCANCODE_LEFT || key == SDL_SCANCODE_RIGHT;
    }

    uint32_t ReadKeyboard(const bool *keys, int player, bool globalShortcuts = true) {
        uint32_t mask = ReadKeyboardButtons(g_bindings, keys, player, g_hotkeys,
                                            SDL_GetModState(), globalShortcuts, Table());
        if (KeyZoomHeld(keys)) {
            for (int a = 0; a < Table().table.count; ++a)
                if (KeyZoom(g_bindings.key[player][a])) mask &= ~Table().bits[a];
        }
        return mask;
    }
    uint32_t ReadGamepad(SDL_Gamepad *gp, int player) {
        uint32_t mask = ReadGamepadInput(gp, g_bindings, player, Table());
        for (int n = 0; n < kPadHotkeyCount; ++n) {
            if (!ShortcutHeld(gp, player, n)) continue;
            for (int a = 0; a < Table().table.count; ++a)
                if (std::find(g_padHotkeys[n].begin(), g_padHotkeys[n].end(), g_bindings.pad[player][a]) != g_padHotkeys[n].end()) mask &= ~Table().bits[a];
        }
        if (ZoomHeld(gp)) {
            mask &= ~uint32_t(15);
            for (int a = 0; a < Table().table.count; ++a)
                if (Shoulder(g_bindings.pad[player][a].button)) mask &= ~Table().bits[a];
        }
        return mask;
    }

    // A Multi game run's window: where the game before left it (each game is a program of its own, and its window
    // would open where the system puts a new one). The game ending for the next writes it down; the next game reads
    // it, if it was written moments ago (not a run before), and opens its window there, as tall.
    struct Placement {
        int x = 0, y = 0, height = 0;
        bool maximized = false;
    };
    std::filesystem::path PlacementFile() {
        std::error_code error;
        const std::filesystem::path folder = std::filesystem::temp_directory_path(error);
        return error ? std::filesystem::path() : folder / "cps2recomp-multi-window.txt";
    }
    void KeepPlacement(SDL_Window *window) {
        const std::filesystem::path file = PlacementFile();
        if (file.empty() || (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN)) {
            return;
        }
        Placement at;
        int width = 0;
        SDL_GetWindowPosition(window, &at.x, &at.y);
        SDL_GetWindowSize(window, &width, &at.height);
        at.maximized = SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED;
        std::ofstream(file) << at.x << ' ' << at.y << ' ' << at.height << ' ' << at.maximized << '\n';
    }
    bool TakePlacement(Placement &at) {
        const std::filesystem::path file = PlacementFile();
        std::error_code error;
        if (file.empty() || !std::filesystem::exists(file, error)) {
            return false;
        }
        const auto age = std::filesystem::file_time_type::clock::now() - std::filesystem::last_write_time(file, error);
        bool fresh = !error && age < std::chrono::minutes(1);
        {
            std::ifstream in(file);
            fresh = fresh && (in >> at.x >> at.y >> at.height >> at.maximized) && at.height > 0;
        }
        std::filesystem::remove(file, error);
        return fresh;
    }

} // namespace

int RunWindowed(Port &port) {
    SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, "256");
    if (!port.opt.settingsPath.empty()) {
        g_bindings = LoadBindings(port.opt.settingsPath, Table());
    }
    LoadCurrentInputProfile(port.opt.settingsPath);
    InputProcessor inputProcessor;
    g_hotkeys = LoadHotkeys(port.opt.hotkeysPath.empty() ? DefaultHotkeysPath() : port.opt.hotkeysPath);
    g_padHotkeys = LoadPadHotkeys(port.opt.hotkeysPath.empty() ? DefaultHotkeysPath() : port.opt.hotkeysPath);
    const auto keyboardPath = (std::filesystem::path(port.opt.hotkeysPath.empty() ? DefaultHotkeysPath() : port.opt.hotkeysPath).parent_path() / "keyboards.ini").string();
    auto keyboardConfig = LoadKeyboards(keyboardPath);
    g_gamepadAssignments = LoadGamepadAssignments((std::filesystem::path(keyboardPath).parent_path() / "gamepads.ini").string());
    InputTraceConfigure(keyboardConfig);
    RawKeyboards keyboards;
    if (keyboardConfig.enabled) SDL_SetHintWithPriority(SDL_HINT_WINDOWS_RAW_KEYBOARD, "0", SDL_HINT_OVERRIDE);
    SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, "256");
    if (!port.opt.settingsPath.empty()) {
        g_bindings = LoadBindings(port.opt.settingsPath);
    }
    InputTraceProfile(KeyboardActionOptions(keyboardConfig, CurrentInputProfile()), g_bindings);
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    // (the window: so many times the board's 224 lines, --scale's or the Display page's)
    const int scale = port.opt.scale > 0 ? port.opt.scale : WindowSize();
    // (the window's name: game.cfg's "title" where a game has one, else its name)
#ifdef GAME_TITLE
    const char *title = GAME_TITLE;
#else
    const char *title = GAME_NAME;
#endif
    // (the picture's shape: the board's 384 x 224 shown 4:3 unless the Display page says otherwise, and as
    // much wider as the widest picture the game's sources may ask for; 4:3 for a picture that fills it)
    const int columns = TheGame().widestPicture ? std::clamp(TheGame().widestPicture(), +Video::kWidth, +Video::kMaxWidth) : Video::kWidth;
    auto windowShape = [](int across) { return PictureShape(across, Video::kHeight, DisplayShape(), 4.0f / 3.0f * across / Video::kWidth); };
    // (in a Multi game run, a window where the game before's was: a full screen one is full screen as asked)
    Placement placed;
    const bool carried = !MultiGame().empty() && TakePlacement(placed) && !port.opt.fullscreen;
    const int windowHeight = carried ? placed.height : Video::kHeight * scale;
    // The window, as an OpenGL window for a shader (shading.h: nothing else draws to it), `at` a place for it (`width`
    // 0: as wide as the picture's shape makes it)
    auto makeWindow = [&](bool opengl, const Placement *at, int width, int height, bool fullscreen) {
        const SDL_PropertiesID properties = SDL_CreateProperties();
        SDL_SetStringProperty(properties, SDL_PROP_WINDOW_CREATE_TITLE_STRING, title);
        SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width > 0 ? width : std::lround(height * windowShape(columns)));
        SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
        if (at) {
            SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_X_NUMBER, at->x);
            SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_Y_NUMBER, at->y);
        }
#ifdef __ANDROID__
        fullscreen = true; // (a phone's whole screen, its bars hidden)
#endif
        SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER,
                              SDL_WINDOW_RESIZABLE | (fullscreen ? SDL_WINDOW_FULLSCREEN : 0) | (at && at->maximized ? SDL_WINDOW_MAXIMIZED : 0) |
                                  (opengl ? SDL_WINDOW_OPENGL : 0) | (getenv("CPS2RECOMP_HIDDEN_WINDOW") ? SDL_WINDOW_HIDDEN : 0)); // (hidden: for tests)
        SDL_Window *made = SDL_CreateWindowWithProperties(properties);
        SDL_DestroyProperties(properties);
        // (in front: on macOS a program started by one that is not, as the launcher is behind a Multi game run's
        // last game, opens behind the other windows)
        if (made && !getenv("CPS2RECOMP_HIDDEN_WINDOW")) {
            SDL_RaiseWindow(made);
        }
        return made;
    };
    // The picture goes to the window through SDL's GPU API, or through SDL's renderer where there is no
    // device for that (and the dummy video driver of the tests has none) or when asked: --renderer sdl. With a
    // shader preset chosen (the Display page's Shader, or --shader) it goes through the preset with OpenGL instead,
    // where librashader is beside the game and makes the preset; else as without one.
    Presenter presenter;
    std::string presentError;
    const bool wantGpu = port.opt.renderer != "sdl" && strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0;
    const bool canShade = strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0;
    std::string preset = canShade ? port.ShaderPreset() : "";
    int shaderValue = FeatureValue("display_shader");
    std::string shaderError;
    if (!preset.empty() && !Shading::Available(shaderError)) {
        fprintf(stderr, "shader: %s; the picture is drawn without it\n", shaderError.c_str());
        preset.clear();
    }
    SDL_Window *window = makeWindow(!preset.empty(), carried ? &placed : nullptr, 0, windowHeight, port.opt.fullscreen);
    // (opened as the preset says, or as without one when that fails)
    auto openPresenter = [&]() {
        if (!preset.empty()) {
            if (presenter.OpenShaded(window, preset, shaderError)) {
                return true;
            }
            fprintf(stderr, "shader %s: %s; the picture is drawn without it\n", preset.c_str(), shaderError.c_str());
            preset.clear();
            // (an OpenGL window is drawn to by nothing else: one that is not, where this one is)
            Placement at;
            int width = 0;
            SDL_GetWindowPosition(window, &at.x, &at.y);
            SDL_GetWindowSize(window, &width, &at.height);
            at.maximized = SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED;
            const bool fullscreen = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN;
            SDL_DestroyWindow(window);
            window = makeWindow(false, &at, width, at.height, fullscreen);
            if (!window) {
                presentError = SDL_GetError();
                return false;
            }
        }
        return presenter.Open(window, wantGpu, presentError);
    };
    if (!window || !openPresenter()) {
        fprintf(stderr, "SDL window: %s\n", window ? presentError.c_str() : SDL_GetError());
        return 1;
    }
    if (wantGpu && !presenter.UsesGpu() && !presenter.Shaded()) {
        fprintf(stderr, "no GPU device (%s): the picture goes through SDL's renderer\n", presentError.c_str());
    }
    auto sayPresenter = [&]() {
        if (presenter.Shaded() && getenv("CPS2RECOMP_SHADER_SHOT")) {
            presenter.Shader()->keepLast = true;
        }
        if (port.opt.stats) {
            fprintf(stderr, "picture: %s\n", presenter.Shaded()  ? ("OpenGL, shader " + preset).c_str()
                                             : presenter.UsesGpu() ? SDL_GetGPUDeviceDriver(presenter.Device())
                                                                   : SDL_GetRendererName(presenter.Renderer()));
        }
    };
    sayPresenter();
    // The Display page's Shader changed in the menu: another preset in the same window, or the window made again
    // for or without OpenGL, where it is and as large
    auto followShader = [&]() {
        const int value = FeatureValue("display_shader");
        if (value == shaderValue || !port.opt.shader.empty() || !canShade) {
            return;
        }
        shaderValue = value;
        std::string next = port.ShaderPreset();
        if (!next.empty() && !Shading::Available(shaderError)) {
            fprintf(stderr, "shader: %s; the picture is drawn without it\n", shaderError.c_str());
            next.clear();
        }
        if (next == preset) {
            return;
        }
        if (!next.empty() && presenter.Shaded()) {
            if (presenter.SetPreset(next, shaderError)) {
                preset = next;
                sayPresenter();
            } else {
                fprintf(stderr, "shader %s: %s; the one before is kept\n", next.c_str(), shaderError.c_str());
            }
            return;
        }
        Placement at;
        int width = 0;
        SDL_GetWindowPosition(window, &at.x, &at.y);
        SDL_GetWindowSize(window, &width, &at.height);
        at.maximized = SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED;
        const bool fullscreen = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN;
        presenter.Close();
        SDL_DestroyWindow(window);
        preset = next;
        window = makeWindow(!preset.empty(), &at, width, at.height, fullscreen);
        if (!window || !openPresenter()) {
            fprintf(stderr, "SDL window: %s\n", window ? presentError.c_str() : SDL_GetError());
            port.quit = true;
            return;
        }
        if (keyboardConfig.enabled && !keyboards.Retarget(window)) {
            fprintf(stderr, "%s\n", keyboards.Error().c_str());
            port.quit = true;
            return;
        }
        sayPresenter();
    };

    // Sound: how much is kept waiting to be played (seconds), and how much is too much. The device is asked
    // for small buffers, so that little waits there either. (CPS2RECOMP_AUDIO_LOG: what is waiting, printed
    // once a second, and nothing heard.)
    constexpr double kAudioWaiting = 0.028, kAudioMost = 0.11;
    const bool audioLog = getenv("CPS2RECOMP_AUDIO_LOG") != nullptr;
    double audioLeast = 1;
    int audioDropped = 0;
    // (--mute starts with no sound; the menu's Sound row switches it on, so the device is opened all the same)
    if (keyboardConfig.enabled && !keyboards.Open(window)) {
        fprintf(stderr, "%s\n", keyboards.Error().c_str());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Separate keyboards", keyboards.Error().c_str(), window);
        presenter.Close(); SDL_DestroyWindow(window); SDL_Quit(); return 1;
    }
    keyboardConfig=keyboards.State().ResolveConfig(keyboardConfig);
    InputTraceConfigure(keyboardConfig);
    Equalizer boardEqualizer, musicEqualizer;
    std::vector<float> boardMade;
    uint64_t audioEpoch = port.audioEpoch;
    SDL_AudioStream *audio = nullptr;
    port.menu.muted = port.opt.mute;
    bool silent = false;
    {
        const SDL_AudioSpec spec{SDL_AUDIO_F32, 2, QSound::kSampleRate};
        audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
        if (audio) {
            SDL_ResumeAudioStreamDevice(audio);
            if (audioLog) {
                SDL_AudioSpec device{};
                int frames = 0;
                SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(audio), &device, &frames);
                fprintf(stderr, "audio: device %d Hz, %d channels, %d frames a buffer (%s)\n", device.freq, device.channels, frames, SDL_GetCurrentAudioDriver());
            }
        } else {
            fprintf(stderr, "Audio: %s\n", SDL_GetError());
        }
    }
    // The custom soundtrack (soundtrack.h): a stream of its own, at its files' rate, opened when it first plays
    SDL_AudioStream *music = nullptr;
    SDL_AudioSpec musicSpec{SDL_AUDIO_F32, 2, 0};
    std::vector<float> musicMade;
    bool musicSilent = false;

    auto &gamepads = g_gamepads;
    gamepads.clear();
    // The in-game menu: opened and closed by Esc, F1, a gamepad stick's click or its Start and Select
    // together (the game is left from its "Quit game"). While it is open the game stands still and the
    // controls work the menu; buttons held as it closes do not reach the game until let go.
    std::vector<uint32_t> menuBase, menuPicture, shown;
    uint32_t menuWidth = 0, menuHeight = 0;
    bool menuDirty = false;
    uint32_t previousPad = 0;
    std::array<uint32_t, Machine::kMaxPlayers> heldAtClose{};
    bool releaseAtClose = false;
    unsigned previousMenu = 0;
    int triggerHeld = 0;

    // (with a zoom to work, the left stick's click opens the menu when it is let go, unless a shoulder was pressed
    // while it was held: then it was the zoom's)
    using ShortcutRuntime = PadShortcutRouter;
    std::map<SDL_JoystickID, ShortcutRuntime> padShortcuts;
    auto toggleMenu = [&]() {
        if (port.net) {
            // (online: no menu, the game goes on for both players; Esc gives up a wait, and in a match asks whether to
            // leave it: Port::leaving)
            if (port.connecting && !port.net->Arcade()) {
                port.LeaveOnline();
            } else {
                port.leaving = !port.leaving;
            }
            return;
        }
        if (port.menu.IsOpen()) {
            port.menu.Close();
        } else {
            port.MenuPicture(menuBase, menuWidth, menuHeight);
            port.menu.Open(true);
            menuDirty = true;
        }
    };
    // Online, the chat (Port::typing): T on the keyboard, the right stick's click or the touch screen's Chat button
    // opens a line to type, and the system's text input with it (on a Steam Deck or a phone, its own on-screen
    // keyboard); Enter sends it, Esc (or the click again) puts it away. Meanwhile the keyboard is the chat's, and after
    // it, not the game's until every key is let go (the Enter that sent it is not the game's Start).
    uint64_t openedByT = 0; // (when T opened it: its own letter, if the system sends it as text too, is not the line's)
    bool keysAfterChat = false;
    auto chatOpen = [&](bool open) {
        if (open == port.typing || (open && (!port.net || port.connecting || port.leaving))) {
            return;
        }
        port.typing = open;
        port.typed.clear();
        if (open) {
            int w = 0, h = 0;
            SDL_GetWindowSize(window, &w, &h);
            const SDL_Rect area{0, h * 3 / 4, w / 2, h / 12};
            SDL_SetTextInputArea(window, &area, 0);
            SDL_StartTextInput(window);
        } else {
            SDL_StopTextInput(window);
            keysAfterChat = true;
        }
    };
    auto quickState = [&](bool save) {
        if (port.net) {
            return; // (online: no save states)
        }
        std::string error;
        const std::string path = port.StatePath(port.menu.Slot());
        if (!(save ? port.SaveState(path, error) : port.LoadState(path, error))) {
            fprintf(stderr, "state %d: %s\n", port.menu.Slot(), error.c_str());
        }
    };
    // A Multi game run (the launcher's Multi game page): N or a gamepad's left shoulder for the next game, F or
    // the left trigger held to fast-forward (each when the player has not given it to the game)
    const bool multi = !MultiGame().empty();
    // (a touch screen's controls: touch.h)
    TouchControls touch;
    std::vector<uint32_t> touchPicture;
    // (the menu's Zoom row, as last seen: a change there is the camera's, and a zoom's press is the row's)
    int zoomRow = port.zoom.Choice();
    const double kFrameNs = 1e9 / Machine::kFramesPerSecond;
    auto handleHotkey = [&](HotkeyAction action) {
                if(InputTraceEnabled() && action!=HotkeyAction::None)
                    InputTraceNote("HOTKEY",std::to_string(static_cast<int>(action)));
                if (port.net && action != HotkeyAction::Chat && action != HotkeyAction::Menu && action != HotkeyAction::Fullscreen &&
                    action != HotkeyAction::FullscreenAlt) return;
                switch (action) {
                case HotkeyAction::Menu:
                    toggleMenu(); releaseAtClose = true; break;
                case HotkeyAction::Reset: port.Key('r'); menuDirty = true; break;
                case HotkeyAction::Save:
                case HotkeyAction::Load:
                    if (!port.menu.IsOpen()) quickState(action == HotkeyAction::Save);
                    break;
                case HotkeyAction::PreviousSlot: port.menu.SetSlot(port.menu.Slot() - 1); break;
                case HotkeyAction::NextSlot: port.menu.SetSlot(port.menu.Slot() + 1); break;
                case HotkeyAction::Pause: if (!port.menu.IsOpen()) port.Key('p'); break;
                case HotkeyAction::Step: if (!port.menu.IsOpen()) port.Key('o'); break;
                case HotkeyAction::Slower: port.Key('-'); break;
                case HotkeyAction::Faster: port.Key('='); break;
                case HotkeyAction::Restart: if (!port.menu.IsOpen()) port.Key('k', kKeyRestart); break;
                case HotkeyAction::Extra: if (!port.menu.IsOpen()) port.Key('k', kKeyExtra); break;
                case HotkeyAction::Test: port.test = !port.test; break;
                case HotkeyAction::Chat: chatOpen(!port.typing); break;
                case HotkeyAction::ZoomPlayer1: if (!port.menu.IsOpen()) port.zoom.Press(0); break;
                case HotkeyAction::ZoomPlayer2: if (!port.menu.IsOpen()) port.zoom.Press(1); break;
                case HotkeyAction::ZoomBoth: if (!port.menu.IsOpen()) port.zoom.Both(); break;
                case HotkeyAction::Fullscreen:
                case HotkeyAction::FullscreenAlt:
                    SDL_SetWindowFullscreen(window, !(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN));
                    break;
                default: break;
                }
    };
    auto padContext = [&] {
        return PadShortcutContext{port.net != nullptr, port.paused, port.menu.IsOpen(), multi,
                                  Zoom::Possible() && !port.net};
    };
    auto processPadShortcuts = [&](SDL_Gamepad *gp, ShortcutRuntime &runtime) {
        const int player = PadPlayer(gp);
        if (player < 0 || player >= Players()) { runtime = {}; return; }
        g_padContext = padContext();
        PadHotkeyBits held;
        for (int n = 0; n < kPadHotkeyCount; ++n)
            held[n] = PadShortcutEligible(n, g_padContext) && !ShortcutGameBound(n, player) && runtime.state.Down(g_padHotkeys[n]);
        const auto pressed = runtime.Update(g_padHotkeys, held, g_padContext.menu);
        for (int n = 0; n < kPadHotkeyCount; ++n) {
            if (!pressed[n]) continue;
            const bool menu = n == static_cast<int>(HotkeyAction::Menu) || n == kPadMenuAlt || n == kPadMenuChord;
            if (menu) {
                handleHotkey(HotkeyAction::Menu);
            } else if (n < kHotkeyCount) handleHotkey(static_cast<HotkeyAction>(n));
            else if (n == kPadMultiNext) port.Key('n');
        }
    };
    uint64_t menuRepeatAt = 0;
    unsigned menuRepeatDirection = 0;
    auto keyPress = [&](SDL_Scancode key, SDL_Keymod mods, bool repeat, const bool *keys) {
        const auto action = FindHotkey(g_hotkeys, key, mods, repeat);
        if (action != HotkeyAction::None) { handleHotkey(action); return; }
        if (key == SDL_SCANCODE_AC_BACK) { toggleMenu(); return; }
        if (KeyBound(key)) return;
        if (!port.menu.IsOpen() && !repeat && KeyZoomHeld(keys) &&
            (key == SDL_SCANCODE_Q || key == SDL_SCANCODE_E || key == SDL_SCANCODE_W)) {
            if (key == SDL_SCANCODE_W) port.zoom.Both();
            else port.zoom.Press(key == SDL_SCANCODE_E ? 1 : 0);
        } else if (port.net && key == SDL_SCANCODE_T && !repeat) {
            chatOpen(true); openedByT = port.typing ? SDL_GetTicks() : 0;
        } else if (multi && !port.menu.IsOpen() && key == SDL_SCANCODE_N && !repeat) port.Key('n');
    };
    uint64_t nextFrame = SDL_GetTicksNS();
    bool running = true;
    while (running && !port.quit && (port.opt.frames == 0 || port.frame < port.opt.frames)) {
        InputTraceFrame(port.frame);
        SDL_Event ev;
        if (port.typing && (!port.net || port.connecting)) {
            chatOpen(false); // (the match over, or waited for again)
        }
        while (SDL_PollEvent(&ev)) {
            if (touch.Event(ev)) {
                continue;
            }
            if (port.typing && ev.type == SDL_EVENT_TEXT_INPUT) {
                if (openedByT && SDL_GetTicks() - openedByT < 250 && (!strcmp(ev.text.text, "t") || !strcmp(ev.text.text, "T"))) {
                    openedByT = 0;
                    continue;
                }
                openedByT = 0;
                for (const char *c = ev.text.text; *c; c++) {
                    if (*c >= 32 && *c < 127 && port.typed.size() < Netplay::kChatMost) {
                        port.typed += *c;
                    }
                }
                continue;
            }
            if (port.typing && ev.type == SDL_EVENT_KEY_DOWN) {
                if (ev.key.scancode == SDL_SCANCODE_RETURN || ev.key.scancode == SDL_SCANCODE_KP_ENTER) {
                    port.Say();
                    chatOpen(false);
                } else if (ev.key.scancode == SDL_SCANCODE_ESCAPE || ev.key.scancode == SDL_SCANCODE_AC_BACK) {
                    chatOpen(false);
                } else if (ev.key.scancode == SDL_SCANCODE_BACKSPACE && !port.typed.empty()) {
                    port.typed.pop_back();
                }
                continue; // (every other key the line's, as text)
            }
            switch (ev.type) {
            case SDL_EVENT_QUIT: running = false; break;
            case SDL_EVENT_KEY_DOWN:
                InputTraceSDL(ev.key.scancode, true, ev.key.repeat, ev.key.mod, ev.key.which);
                if (!keyboardConfig.enabled) keyPress(ev.key.scancode, ev.key.mod, ev.key.repeat, SDL_GetKeyboardState(nullptr));
                break;
            case SDL_EVENT_KEY_UP:
                InputTraceSDL(ev.key.scancode, false, ev.key.repeat, ev.key.mod, ev.key.which);
                break;
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            case SDL_EVENT_GAMEPAD_BUTTON_UP:
            case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
                const auto id = ev.type == SDL_EVENT_GAMEPAD_AXIS_MOTION ? ev.gaxis.which : ev.gbutton.which;
                auto *gp = SDL_GetGamepadFromID(id);
                auto &runtime = padShortcuts[id];
                runtime.state.Event(ev);
                processPadShortcuts(gp, runtime);
                break;
            }
            case SDL_EVENT_GAMEPAD_ADDED:
                if (SDL_Gamepad *gp = SDL_OpenGamepad(ev.gdevice.which)) {
                    gamepads.push_back(gp);
                }
                break;
            case SDL_EVENT_GAMEPAD_REMOVED:
                padShortcuts.erase(ev.gdevice.which);
                for (auto it = gamepads.begin(); it != gamepads.end(); ++it) {
                    if (SDL_GetGamepadID(*it) == ev.gdevice.which) {
                        SDL_CloseGamepad(*it);
                        gamepads.erase(it);
                        break;
                    }
                }
                break;
            default: break;
            }
        }

        for (auto *gp : gamepads) {
            auto &runtime = padShortcuts[SDL_GetGamepadID(gp)];
            runtime.state.Poll(gp);
            processPadShortcuts(gp, runtime);
        }
        g_padContext = padContext();
        if (keyboardConfig.enabled) {
            keyboards.PollFocus();
            keyboardConfig = keyboards.State().ResolveConfig(keyboardConfig);
            for (const auto &press : keyboards.State().TakePresses())
                if (!port.typing && KeyboardHotkeyAllowed(keyboardConfig, press))
                    keyPress(press.key, press.mods, press.repeat, keyboards.State().ForPlayer(keyboardConfig, 0).data());
        }
        if (touch.TakeMenuPress()) {
            toggleMenu();
        }
        port.PumpHost(); // (a match hosted from the menu, listened for, the menu open or not)
        touch.Chat(port.net != nullptr); // (online, from the start or a match hosted from the menu)
        if (touch.TakeZoomPress()) {
            if (port.net) {
                chatOpen(!port.typing);
            } else {
                port.zoom.Both();
            }
        }
        if (Zoom::Possible()) {
            const int asked = FeatureValue("zoom");
            if (asked != zoomRow) {
                port.zoom.Choose(asked);
            }
            zoomRow = port.zoom.Choice();
            if (asked != zoomRow) {
                SetFeatureForNow("zoom", zoomRow);
                menuDirty = true;
            }
        }
        if (port.opt.moviePlay.empty() && !port.opt.autoPlay) {
            // The keyboard is every player's (each has the keys chosen for it); the first gamepad is player
            // 1's, a second player 2's, and so on
            std::array<uint32_t, Machine::kMaxPlayers> pads{}, keyboardMasks{}, gamepadMasks{};
            std::array<uint16_t, Machine::kMaxPlayers> tags{};
            int keyCount = 0;
            const bool *keys = SDL_GetKeyboardState(&keyCount);
            KeyboardState::Keys menuKeys{};
            SDL_Keymod menuMods = SDL_GetModState();
            if (keyboardConfig.enabled) {
                for (int player = 0; player < Players(); ++player) {
                    const auto &own = keyboards.State().ForPlayer(keyboardConfig, player);
                    for (size_t k = 0; k < menuKeys.size(); ++k) menuKeys[k] = menuKeys[k] || own[k];
                }
                keys = menuKeys.data(); keyCount = static_cast<int>(menuKeys.size());
                menuMods = keyboards.State().Modifiers(menuKeys);
            }
            if (keysAfterChat) keysAfterChat = std::any_of(keys, keys + keyCount, [](bool down) { return down; });
            if (!port.typing && !keysAfterChat) {
                for (int player = 0; player < Players(); ++player)
                    pads[player] = keyboardConfig.enabled ? ReadKeyboardPlayerButtons(keyboards.State(), keyboardConfig, player, g_bindings, g_hotkeys, Table())
                                                          : ReadKeyboard(keys, player);
                const auto &primary = keyboards.State().ForPlayer(keyboardConfig, 0);
                port.service = HotkeyHeld(g_hotkeys, HotkeyAction::Service, keyboardConfig.enabled ? primary.data() : keys,
                                         keyboardConfig.enabled ? keyboards.State().Modifiers(primary) : menuMods);
            } else port.service = false;
            keyboardMasks = pads;
            bool hurry = multi && keys[SDL_SCANCODE_F] && !KeyBound(SDL_SCANCODE_F);
            // (the camera moved by the arrows with Left Shift held, and by each gamepad's d-pad or stick with its click held)
            if (!port.menu.IsOpen()) {
                int across = 0, down = 0;
                if (KeyZoomHeld(keys) && !(SDL_GetModState() & SDL_KMOD_ALT)) {
                    across += keys[SDL_SCANCODE_RIGHT] - keys[SDL_SCANCODE_LEFT];
                    down += keys[SDL_SCANCODE_DOWN] - keys[SDL_SCANCODE_UP];
                }
                for (SDL_Gamepad *gp : gamepads) {
                    if (ZoomHeld(gp)) {
                        const int x = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFTX), y = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFTY);
                        const int right = SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_RIGHT) || x > kDead;
                        const int left = SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_LEFT) || x < -kDead;
                        const int lower = SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_DOWN) || y > kDead;
                        const int higher = SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_UP) || y < -kDead;
                        across += right - left;
                        down += lower - higher;
                        // (the click was the camera's, not the menu's, once it has been moved with it)
                        if (right || left || lower || higher) {
                            for (int n : {static_cast<int>(HotkeyAction::Menu), kPadMenuAlt, kPadMenuChord}) {
                                auto &runtime = padShortcuts[SDL_GetGamepadID(gp)];
                                if (runtime.deferred[n]) { runtime.used[n] = true; runtime.deferred[n] = false; }
                            }
                        }
                    }
                }
                port.zoom.Pan(std::clamp(across, -1, 1), std::clamp(down, -1, 1));
            }
            pads[0] |= touch.Read(port.menu.IsOpen());
            bool trigger = false, menuShortcutHeld = false;
            const auto assigned = ResolveGamepadAssignments(g_gamepadAssignments, DescribeGamepads(gamepads));
            for (auto *gp : gamepads) {
                const int player = AssignedGamepadPlayer(SDL_GetGamepadID(gp), assigned);
                if (player < 0 || player >= Players()) continue;
                gamepadMasks[player] = ReadGamepad(gp, player);
                pads[player] |= gamepadMasks[player];
                trigger = trigger || ShortcutHeld(gp, player, static_cast<int>(HotkeyAction::Step));
                hurry = hurry || ShortcutHeld(gp, player, kPadFastForward);
                port.service = port.service || ShortcutHeld(gp, player, static_cast<int>(HotkeyAction::Service));
                if (ShortcutHeld(gp, player, kPadTag)) tags[player] = kTag;
                for (int n : {static_cast<int>(HotkeyAction::Menu), kPadMenuAlt, kPadMenuChord})
                    menuShortcutHeld = menuShortcutHeld || ShortcutHeld(gp, player, n);
            }
            port.hurry = hurry && !port.menu.IsOpen();
            triggerHeld = trigger && port.paused ? triggerHeld + 1 : 0;
            if (!port.menu.IsOpen() && triggerHeld > 24 && triggerHeld % 4 == 0) port.Key('o');
            uint32_t all = 0;
            for (uint32_t one : pads) all |= one;
            uint32_t pressed = all & ~previousPad;
            previousPad = all;
            // The menu's buttons are the same whatever the game's are set to: the arrows, the d-pad and the
            // left stick (and the game's own directions); Space, Enter and A choose; Backspace and B go back
            unsigned menuNow = 0;
            menuNow |= (previousPad & kUp) || keys[SDL_SCANCODE_UP] ? recomp::kMenuUp : 0;
            menuNow |= (previousPad & kDown) || keys[SDL_SCANCODE_DOWN] ? recomp::kMenuDown : 0;
            menuNow |= (previousPad & kLeft) || keys[SDL_SCANCODE_LEFT] ? recomp::kMenuLeft : 0;
            menuNow |= (previousPad & kRight) || keys[SDL_SCANCODE_RIGHT] ? recomp::kMenuRight : 0;
            menuNow |= keys[SDL_SCANCODE_SPACE] || (keys[SDL_SCANCODE_RETURN] && !(menuMods & SDL_KMOD_ALT)) ? recomp::kMenuConfirm : 0;
            menuNow |= keys[SDL_SCANCODE_BACKSPACE] ? recomp::kMenuBack : 0;
            for (SDL_Gamepad *gp : gamepads) {
                menuNow |= SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_UP) ? recomp::kMenuUp : 0;
                menuNow |= SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_DOWN) ? recomp::kMenuDown : 0;
                menuNow |= SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_LEFT) ? recomp::kMenuLeft : 0;
                menuNow |= SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_RIGHT) ? recomp::kMenuRight : 0;
                menuNow |= SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_SOUTH) ? recomp::kMenuConfirm : 0;
                menuNow |= SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_EAST) ? recomp::kMenuBack : 0;
            }
            menuNow |= touch.Menu();
            unsigned menuPressed = menuNow & ~previousMenu;
            previousMenu = menuNow;
            const unsigned direction = menuNow & (recomp::kMenuUp | recomp::kMenuDown | recomp::kMenuLeft | recomp::kMenuRight);
            const uint64_t now = SDL_GetTicksNS();
            if (!port.menu.IsOpen() || direction != menuRepeatDirection || !direction) {
                menuRepeatDirection = direction;
                menuRepeatAt = now + 300000000ull;
            } else if (now >= menuRepeatAt) {
                menuPressed |= direction;
                menuRepeatAt = now + 45000000ull;
            }
            if ((menuPressed & (recomp::kMenuLeft | recomp::kMenuRight)) && (menuMods & SDL_KMOD_SHIFT)) menuPressed |= recomp::kMenuFast;
            if (menuShortcutHeld) {
                menuPressed = 0;
            }
            if (port.menu.IsOpen()) {
                if (menuPressed) {
                    port.MenuAct(port.menu.Press(menuPressed));
                    menuDirty = true;
                } else if (pressed) {
                    port.menu.Note(""); // (a press of something else still takes the note away)
                    menuDirty = true;
                }
                if (!port.menu.IsOpen()) {
                    releaseAtClose = true;
                }
            } else if (port.leaving) {
                // (online, "Leave the match?": Confirm leaves it, Back stays; this side's player stands still meanwhile)
                if (menuPressed & recomp::kMenuConfirm) {
                    port.LeaveOnline();
                } else if (menuPressed & recomp::kMenuBack) {
                    port.leaving = false;
                }
                releaseAtClose = true;
            }
            if (releaseAtClose) { heldAtClose = pads; releaseAtClose = false; }
            const auto options = KeyboardActionOptions(keyboardConfig, CurrentInputProfile());
            InputTraceProfile(options, g_bindings);
            for (int player = 0; player < Machine::kMaxPlayers; ++player) {
                heldAtClose[player] &= pads[player];
                port.pad[player] = inputProcessor.Apply(player, pads[player] & ~heldAtClose[player], port.frame, options) | tags[player];
                InputTracePlayer(keyboards.State(), keyboardConfig, player, keyboardMasks[player], gamepadMasks[player],
                                 heldAtClose[player], port.pad[player], port.menu.IsOpen(), port.paused);
            }
        }

        // The window takes the picture's shape again when that changes (widescreen, or the Display page's
        // shape), and its size when the Display page's size does: a window only, not full screen or maximized
        {
            static int shownColumns = columns, shownSize = scale;
            static Shape shownShape = DisplayShape();
            const int now = TheGame().widestPicture ? std::clamp(TheGame().widestPicture(), +Video::kWidth, +Video::kMaxWidth) : columns;
            const int size = port.opt.scale > 0 ? port.opt.scale : WindowSize();
            if (now != shownColumns || DisplayShape() != shownShape || size != shownSize) {
                if (!(SDL_GetWindowFlags(window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MAXIMIZED))) {
                    int windowWidth = 0, windowHeight = 0;
                    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
                    if (size != shownSize) {
                        windowHeight = Video::kHeight * size;
                    }
                    SDL_SetWindowSize(window, static_cast<int>(std::lround(windowHeight * windowShape(now))), windowHeight);
                }
                shownColumns = now;
                shownShape = DisplayShape();
                shownSize = size;
            }
        }
        followShader();
        if (port.quit) {
            break;
        }
        // The picture in the window as the Display page says (present.h)
        int width = 0, height = 0;
        presenter.Size(width, height);
        presenter.SetScaling(DisplayScaling());
        const SDL_FRect where = FitPicture(width, height, port.ShownWidth(), Video::kHeight, DisplayShape(), DisplayScaling());
        touch.Layout(width, height, where);
        int touchWidth = 0, touchHeight = 0;
        if (touch.TakePicture(touchPicture, touchWidth, touchHeight)) {
            presenter.SetOverlay(touchPicture.empty() ? nullptr : touchPicture.data(), touchWidth, touchHeight);
        }

        if (port.menu.IsOpen()) {
            // The game's picture as it was, with the menu over it
            if (menuDirty) {
                menuDirty = false;
                menuPicture = menuBase;
                port.menu.Draw(menuPicture.data(), static_cast<int>(menuWidth), static_cast<int>(menuHeight));
            }
            // (not through a shader: the menu is drawn three times the board's size, and is not the game's picture)
            presenter.Show(menuPicture.data(), static_cast<int>(menuWidth), static_cast<int>(menuHeight), where, nullptr, false);
            SDL_Delay(8);
            nextFrame = SDL_GetTicksNS();
            continue;
        }

        // (the game's time to the screen's: under 1 slowed, over 1 faster than the board)
        const auto rate = port.menu.GameRate();
        const double pace = port.paused ? 1.0 : static_cast<double>(rate.frames) / rate.ticks;
        const bool ran = port.Tick();

        if (audioEpoch != port.audioEpoch) {
            audioEpoch = port.audioEpoch; boardEqualizer.Reset(); musicEqualizer.Reset();
            if (audio) SDL_ClearAudioStream(audio);
            if (music) SDL_ClearAudioStream(music);
        }
        if (audio && silent != (port.menu.muted || audioLog)) {
            silent = port.menu.muted || audioLog;
            SDL_SetAudioStreamGain(audio, silent ? 0.0f : 1.0f);
        }
        std::vector<int16_t> &made = port.machine->audio;
        if (audio && ran && !made.empty()) {
            // The game makes sound at its own pace and the sound device takes it at its own, and the two
            // clocks never quite agree. So the sound is played a touch faster when it is piling up and a
            // touch slower when it is running out (a fraction of a percent, far too little to hear), which
            // keeps a frame or two of it waiting: no gaps, no growing delay.
            // (Slowed, the sound is played slower by as much; faster, faster.)
            double waiting = SDL_GetAudioStreamQueued(audio) / (8.0 * QSound::kSampleRate) / pace;
            // (When much more than that is waiting (the game caught up after being held up, say) what waits
            // is thrown away: a click once, rather than sound that stays late for the seconds it would take
            // to play it off.)
            if (waiting > kAudioMost) {
                SDL_ClearAudioStream(audio);
                waiting = 0;
                audioDropped++;
            }
            const double ratio = (1.0 + std::clamp((waiting - kAudioWaiting) * 0.5, -0.02, 0.02)) * pace;
            SDL_SetAudioStreamFrequencyRatio(audio, static_cast<float>(ratio));
            audioLeast = std::min(audioLeast, waiting);
            if (audioLog && port.frame % 60 == 0) {
                fprintf(stderr, "audio: frame %llu, %.0f ms waiting (least %.0f), ratio %.4f, thrown away %d times\n",
                        static_cast<unsigned long long>(port.frame), waiting * 1000, audioLeast * 1000, ratio, audioDropped);
                audioLeast = 1;
            }
            boardEqualizer.ProcessS16(made, boardMade, QSound::kSampleRate, CurrentEqualizer());
            SDL_PutAudioStreamData(audio, boardMade.data(), static_cast<int>(boardMade.size() * sizeof(float)));
        }
        made.clear();
        if (ran && port.soundtrackOn && port.soundtrack.Any()) {
            // (fed as the game's sound is: the game's time's worth each frame, kept a frame or two ahead of the device)
            port.soundtrack.Make(port.machine->soundtrack.state, !FastForward() && !port.hurry, musicMade);
            if (!musicMade.empty()) {
                const int rate = port.soundtrack.Rate();
                if (!music) {
                    musicSpec.freq = rate;
                    music = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &musicSpec, nullptr, nullptr);
                    if (music) {
                        SDL_ResumeAudioStreamDevice(music);
                    } else {
                        fprintf(stderr, "Audio (soundtrack): %s\n", SDL_GetError());
                    }
                } else if (rate != musicSpec.freq) {
                    musicSpec.freq = rate;
                    SDL_SetAudioStreamFormat(music, &musicSpec, nullptr);
                }
            }
            if (music && !musicMade.empty()) {
                if (musicSilent != (port.menu.muted || audioLog)) {
                    musicSilent = port.menu.muted || audioLog;
                    SDL_SetAudioStreamGain(music, musicSilent ? 0.0f : 1.0f);
                }
                double waiting = SDL_GetAudioStreamQueued(music) / (8.0 * musicSpec.freq) / pace;
                if (waiting > kAudioMost) {
                    SDL_ClearAudioStream(music);
                    waiting = 0;
                }
                const double ratio = (1.0 + std::clamp((waiting - kAudioWaiting) * 0.5, -0.02, 0.02)) * pace;
                SDL_SetAudioStreamFrequencyRatio(music, static_cast<float>(ratio));
                musicEqualizer.ProcessFloat(musicMade, musicSpec.freq, CurrentEqualizer());
                SDL_PutAudioStreamData(music, musicMade.data(), static_cast<int>(musicMade.size() * sizeof(float)));
            }
        }

        // (zoomed: the part of the picture the camera looks at, the readout's panels as they are)
        Zoomed zoomed;
        const bool isZoomed = port.Windowed(shown, zoomed);
        presenter.Show(shown.data(), port.ShownWidth(), Video::kHeight, where, isZoomed ? &zoomed : nullptr);

        if (!port.opt.unthrottled && !FastForward()) {
            nextFrame += static_cast<uint64_t>(1e9 / port.machine->refreshHz);
            const uint64_t now = SDL_GetTicksNS();
            if (nextFrame > now) {
                SDL_DelayPrecise(nextFrame - now);
            } else if (now - nextFrame > 100000000ull) {
                nextFrame = now; // (fell far behind: start counting again)
            }
        }
    }
    // (for the checks: what a shaded window showed last, CPS2RECOMP_SHADER_SHOT=FILE.png)
    if (const char *shot = getenv("CPS2RECOMP_SHADER_SHOT"); shot && presenter.Shaded()) {
        std::vector<uint32_t> pixels;
        int width = 0, height = 0;
        if (presenter.Shader()->ReadBack(pixels, width, height)) {
            WritePng(shot, pixels.data(), static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        }
    }
    for (SDL_Gamepad *gp : gamepads) {
        SDL_CloseGamepad(gp);
    }
    if (audio) {
        SDL_DestroyAudioStream(audio);
    }
    if (music) {
        SDL_DestroyAudioStream(music);
    }
    // (on to a Multi game run's next game: its window where this one is; on macOS the launcher starts it, and must
    // be in front for it to come forward)
    if (ProgramEnded() == kNextGame && !MultiGame().empty()) {
        KeepPlacement(window);
#ifdef __APPLE__
        HandFrontToLauncher();
#endif
    }
    presenter.Close();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

} // namespace port
