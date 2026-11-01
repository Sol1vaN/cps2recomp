// SPDX-License-Identifier: GPL-3.0-only
#include "gamepad_assignment.h"
#include "input_profile.h"
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

static int checks = 0;
static void Check(bool value, const char *why) {
    ++checks;
    if (!value) throw std::runtime_error(std::string(why) + ": " + SDL_GetError());
}
struct VirtualPad {
    SDL_JoystickID id = 0;
    SDL_Gamepad *pad = nullptr;
    SDL_Joystick *joystick = nullptr;
    VirtualPad(int product) {
        SDL_VirtualJoystickDesc desc{};
        SDL_INIT_INTERFACE(&desc);
        desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
        desc.vendor_id = 0x1209; desc.product_id = static_cast<Uint16>(product);
        desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
        desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
        desc.axis_mask = (1u << SDL_GAMEPAD_AXIS_COUNT) - 1;
        desc.button_mask = (1u << SDL_GAMEPAD_BUTTON_COUNT) - 1;
        desc.name = "Input Lab analog regression controller";
        id = SDL_AttachVirtualJoystick(&desc);
        Check(id && SDL_IsGamepad(id), "virtual device has a gamepad mapping");
        pad = SDL_OpenGamepad(id);
        Check(pad != nullptr, "open virtual gamepad");
        joystick = SDL_GetGamepadJoystick(pad);
        Clear();
    }
    void Axis(int axis, int value) {
        Check(SDL_SetJoystickVirtualAxis(joystick, axis, static_cast<Sint16>(value)), "set physical axis");
        SDL_UpdateGamepads();
    }
    void Clear() {
        for (int axis = 0; axis < SDL_GAMEPAD_AXIS_COUNT; ++axis)
            Axis(axis, axis >= SDL_GAMEPAD_AXIS_LEFT_TRIGGER ? -32768 : 0);
        for (int button = 0; button < SDL_GAMEPAD_BUTTON_COUNT; ++button)
            Check(SDL_SetJoystickVirtualButton(joystick, button, false), "release button");
        SDL_UpdateGamepads();
    }
    ~VirtualPad() {
        if (pad) SDL_CloseGamepad(pad);
        if (id) SDL_DetachVirtualJoystick(id);
    }
};

int main(int argc, char **argv) {
    using namespace port;
    try {
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        Check(SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_EVENTS), "initialize native SDL input");
        if (argc > 1 && std::strcmp(argv[1], "--list") == 0) {
            int count = 0; auto *ids = SDL_GetGamepads(&count);
            for (int n = 0; n < count; ++n) if (auto *pad = SDL_OpenGamepad(ids[n])) {
                const auto device = DescribeGamepad(pad);
                std::cout << "GAMEPAD\t" << device.key << "\t" << device.name << "\n";
                SDL_CloseGamepad(pad);
            }
            SDL_free(ids); SDL_Quit(); return 0;
        }
        {
            VirtualPad first(0x1311), second(0x1312);
            SDL_Event event{};
            while (SDL_PollEvent(&event)) {}
            for (int axis = 0; axis < 4; ++axis) {
                for (int sign : {-1, 1}) {
                    const PadInput input{-1, axis, sign};
                    Check(recomp::PadFromText(recomp::PadText(input)) == input, "signed axis round trip");
                    first.Axis(axis, sign < 0 ? -32768 : 32767);
                    bool captured = false;
                    while (SDL_PollEvent(&event))
                        if (event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION && event.gaxis.which == first.id &&
                            recomp::PadInputFromEvent(event) == input) captured = true;
                    Check(captured, "real SDL motion event accepted by launcher capture helper");
                    Check(recomp::PadInputPressed(first.pad, input), "signed axis held");
                    Check(!recomp::PadInputPressed(first.pad, {-1, axis, -sign}), "opposite half not held");
                    for (int rest : {0, -16000, 16000}) {
                        event = {};
                        event.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
                        event.gaxis.axis = static_cast<Uint8>(axis);
                        event.gaxis.value = static_cast<Sint16>(rest);
                        Check(recomp::PadInputFromEvent(event) == PadInput{}, "drift/deadzone cannot bind");
                    }
                    first.Axis(axis, 0);
                    Check(!recomp::PadInputPressed(first.pad, input), "axis release clears held state");
                }
            }
            for (const auto &invalid : {"leftx", "righty", "lefttrigger-", "none+", "rightx?", "bad"})
                Check(recomp::PadFromText(invalid) == PadInput{}, "invalid axes cannot become a binding");
            Check(recomp::PadText({-1, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER}) == "righttrigger", "legacy trigger format retained");
            Check(recomp::PadText({SDL_GAMEPAD_BUTTON_DPAD_UP, -1}) == "dpup", "legacy button format retained");

            auto bindings = DefaultBindings();
            first.Clear(); second.Clear();
            first.Axis(SDL_GAMEPAD_AXIS_LEFTX, -32768);
            first.Axis(SDL_GAMEPAD_AXIS_LEFTY, -32768);
            Check(ReadGamepadInput(first.pad, bindings, 0) == 10, "legacy left stick diagonal unchanged");
            Check(ReadGamepadInput(second.pad, bindings, 1) == 0, "other pad remains neutral");
            first.Clear();
            Check(SDL_SetJoystickVirtualButton(first.joystick, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, true), "press D-pad");
            SDL_UpdateGamepads();
            Check(ReadGamepadInput(first.pad, bindings, 0) == 1, "legacy D-pad works");
            first.Clear();
            first.Axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 32767);
            Check(ReadGamepadInput(first.pad, bindings, 0) == (1u << 9), "legacy trigger still attacks");

            const std::array<PadInput, 4> right{{{-1, SDL_GAMEPAD_AXIS_RIGHTY, -1}, {-1, SDL_GAMEPAD_AXIS_RIGHTY, 1},
                                              {-1, SDL_GAMEPAD_AXIS_RIGHTX, -1}, {-1, SDL_GAMEPAD_AXIS_RIGHTX, 1}}};
            const auto path = std::filesystem::current_path() / "analog-bindings-test.ini";
            {
                std::ofstream file(path);
                file << "[features]\npreset=pure\n[keyboard1]\nup=W\n";
                for (int player = 0; player < 2; ++player) {
                    file << "[gamepad" << player + 1 << "]\n";
                    for (int dir = 0; dir < 4; ++dir)
                        file << kBoundActions[dir].name << "=" << recomp::PadText(right[dir]) << "\n";
                }
            }
            bindings = LoadBindings(path.string());
            Check(bindings.key[0][0] == SDL_SCANCODE_W, "loading signed axes retains keyboard layout");
            for (int player = 0; player < 2; ++player) {
                auto &device = player == 0 ? first : second;
                for (int dir = 0; dir < 4; ++dir) {
                    first.Clear(); second.Clear();
                    device.Axis(right[dir].axis, right[dir].direction < 0 ? -32768 : 32767);
                    Check(ReadGamepadInput(device.pad, bindings, player) == kBoundActions[dir].bit, "saved right-stick direction reaches game");
                }
            }
            first.Clear(); second.Clear();
            first.Axis(SDL_GAMEPAD_AXIS_RIGHTX, 32767);
            first.Axis(SDL_GAMEPAD_AXIS_RIGHTY, -32768);
            first.Axis(SDL_GAMEPAD_AXIS_LEFTX, -32768);
            Check(ReadGamepadInput(first.pad, bindings, 0) == 9, "explicit right stick replaces automatic left directions");
            second.Axis(SDL_GAMEPAD_AXIS_RIGHTX, 32767);
            first.Axis(SDL_GAMEPAD_AXIS_RIGHTX, 0);
            first.Axis(SDL_GAMEPAD_AXIS_RIGHTY, 0);
            Check(ReadGamepadInput(first.pad, bindings, 0) == 0 && ReadGamepadInput(second.pad, bindings, 1) == 1,
                  "one controller release cannot clear the other controller");
            bindings.pad[0][2] = {-1, SDL_GAMEPAD_AXIS_LEFTX, 1};
            bindings.pad[0][3] = {-1, SDL_GAMEPAD_AXIS_LEFTX, -1};
            first.Clear(); first.Axis(SDL_GAMEPAD_AXIS_LEFTX, 32767);
            Check(ReadGamepadInput(first.pad, bindings, 0) == 2, "inverted mapping cannot generate phantom opposite direction");
            bindings = DefaultBindings();
            bindings.pad[0][6] = {-1, SDL_GAMEPAD_AXIS_LEFTX, 1};
            Check(ReadGamepadInput(first.pad, bindings, 0) == (1u << 6), "axis assigned to attack is not also a legacy direction");
            Check(ReadGamepadInput(nullptr, bindings, 0) == 0, "disconnected device releases all input");
            // Regression: one physical controller, keyboard P1 and gamepad P2.
            const auto deviceA = DescribeGamepad(first.pad), deviceB = DescribeGamepad(second.pad);
            Check(deviceA.key != deviceB.key, "distinct physical identity fixtures");
            GamepadAssignments assignments;
            std::vector<SDL_Gamepad *> onePad{first.pad}, twoPads{first.pad, second.pad};
            AssignGamepad(assignments, 1, deviceA.key);
            auto resolved = ResolveGamepadAssignments(assignments, {deviceA});
            Check(resolved[0] == 0 && resolved[1] == first.id, "single controller reserved for P2 before automatic P1");
            first.Clear(); second.Clear(); bindings = DefaultBindings();
            Check(SDL_SetJoystickVirtualButton(first.joystick, SDL_GAMEPAD_BUTTON_BACK, true), "press P2 Back/Select");
            SDL_UpdateGamepads();
            auto masks = ReadAssignedGamepads(onePad, bindings, assignments);
            Check(masks[0] == 0 && masks[1] == (1u << 11), "single gamepad Back reaches P2 Coin only");
            Check((masks[0] | (1u << 4)) == (1u << 4), "independent keyboard P1 input retained");
            first.Clear();
            Check(SDL_SetJoystickVirtualButton(first.joystick, SDL_GAMEPAD_BUTTON_START, true), "press P2 Start");
            SDL_UpdateGamepads(); masks = ReadAssignedGamepads(onePad, bindings, assignments);
            Check(masks[0] == 0 && masks[1] == (1u << 10), "single gamepad Start reaches P2 only");
            for (int action = 0; action < kBoundActionCount; ++action) {
                const auto input = bindings.pad[1][action];
                if (input.button < 0 && input.axis < 0) continue;
                first.Clear();
                if (input.button >= 0) Check(SDL_SetJoystickVirtualButton(first.joystick, input.button, true), "press assigned P2 button");
                else first.Axis(input.axis, 32767);
                SDL_UpdateGamepads(); masks = ReadAssignedGamepads(onePad, bindings, assignments);
                Check(masks[0] == 0 && (masks[1] & kBoundActions[action].bit), "all configured P2 controls routed from controller 1");
            }
            first.Clear(); first.Axis(SDL_GAMEPAD_AXIS_LEFTY, -32768);
            masks = ReadAssignedGamepads(onePad, bindings, assignments);
            Check(masks[0] == 0 && masks[1] == 8, "P2 analog direction uses physical assignment");
            auto changedA = deviceA, changedB = deviceB;
            changedA.id = 90; changedB.id = 91;
            resolved = ResolveGamepadAssignments(assignments, {changedB, changedA});
            Check(resolved[1] == 90 && resolved[0] == 91, "reconnection IDs and enumeration order do not swap explicit P2");
            resolved = ResolveGamepadAssignments(assignments, {changedB});
            Check(resolved[1] == 0 && resolved[0] == 91, "disconnected explicit P2 never falls back to another pad");
            resolved = ResolveGamepadAssignments(assignments, {});
            Check(resolved[0] == 0 && resolved[1] == 0, "all disconnected releases both slots");
            GamepadAssignments automatic;
            resolved = ResolveGamepadAssignments(automatic, {deviceA, deviceB});
            Check(resolved[0] == first.id && resolved[1] == second.id, "old automatic two-pad layout retained");
            AssignGamepad(assignments, 0, deviceA.key);
            Check(assignments.device[1] == "none", "explicit reassignment cannot duplicate a physical pad");
            AssignGamepad(assignments, 1, deviceA.key);
            Check(assignments.device[0] == "none", "moving physical pad to P2 clears explicit P1 ownership only");
            auto ambiguous = deviceA; ambiguous.id = 400;
            resolved = ResolveGamepadAssignments(assignments, {deviceA, ambiguous});
            Check(resolved[1] == 0, "indistinguishable device identities fail closed");
            const auto routingFile = (std::filesystem::current_path() / "gamepads-test.ini").string();
            { std::ofstream file(routingFile); file << "[unrelated]\nkeep=yes\n"; }
            Check(SaveGamepadAssignments(routingFile, assignments), "persist physical gamepad assignments");
            const auto reloaded = LoadGamepadAssignments(routingFile);
            Check(reloaded.device == assignments.device, "physical assignments survive restart");
            recomp::Ini preserved(routingFile); preserved.load();
            Check(preserved.get("unrelated", "keep") == "yes", "assignment save retains unrelated settings");
            Check(PlayerPadInputBound(bindings, 1, {SDL_GAMEPAD_BUTTON_BACK, -1}), "Back is bound to assigned P2 game control");
            Check(!PlayerPadInputBound(bindings, -1, {SDL_GAMEPAD_BUTTON_BACK, -1}), "unassigned controller has no game shortcut owner");

        }
        SDL_Quit();
        std::cout << "GAMEPAD_TESTS_PASS checks=" << checks << " native SDL virtual controllers=2\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "GAMEPAD_TESTS_FAIL " << error.what() << "\n";
        SDL_Quit();
        return 1;
    }
}

