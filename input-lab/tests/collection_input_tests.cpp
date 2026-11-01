// SPDX-License-Identifier: GPL-3.0-only
// Regression cases introduced by the v1.2 collection: filtered actions and four physical players.
#include "../../port/keyboards.h"
#include "../../port/gamepad_assignment.h"
#include <cstdio>
#include <cstdlib>
using namespace port;
static unsigned checks = 0;
static void Check(bool ok, const char *why) { ++checks; if (!ok) { std::fprintf(stderr, "FAIL: %s\n", why); std::exit(1); } }
int main() {
    const auto &table = MakeBoundTable(3, "Attack | Jump | Shoot", false);
    Check(table.table.players == 3, "three-player game retains all three slots");
    int custom4 = -1;
    for (int a = 0; a < table.table.count; ++a)
        if (std::string(table.table.actions[a].name) == "custom4") custom4 = a;
    Check(custom4 >= 0 && custom4 != 17, "filtered game keeps Custom 4 under its actual table index");
    auto bindings = recomp::DefaultBindings(table.table);
    for (int p = 0; p < 3; ++p) BindKeyboardKey(bindings, table.table, p, custom4, SDL_SCANCODE_R);
    Check(bindings.key[0][custom4] == SDL_SCANCODE_R && bindings.key[1][custom4] == SDL_SCANCODE_R,
          "assigning identical filtered bindings never removes another player's key");
    KeyboardConfig config; config.enabled = true; config.mirrorP1 = false;
    KeyboardState state; state.Focus(true);
    for (int p = 0; p < 4; ++p) {
        config.device[p] = "physical-" + std::to_string(p);
        state.Connect("main-" + std::to_string(p), "Same model", config.device[p]);
        state.Connect("extra-" + std::to_string(p), "Auxiliary HID", config.device[p]);
        state.Key("main-" + std::to_string(p), SDL_SCANCODE_R, true);
    }
    InputProfile profile; InputProcessor processor;
    for (int p = 0; p < 4; ++p) profile.players[p].custom[3] = 5;
    for (int p = 0; p < 3; ++p) {
        auto raw = ReadKeyboardPlayerButtons(state, config, p, bindings, Hotkeys{}, table);
        Check(raw == (1u << 17), "filtered binding produces Custom 4 trigger, not an unrelated button");
        Check(processor.Apply(p, raw, 10, profile) == 0x50, "named game receives Attack and Shoot simultaneously");
    }
    state.Key("main-0", SDL_SCANCODE_R, false);
    Check(!state.ForPlayer(config, 0)[SDL_SCANCODE_R] && state.ForPlayer(config, 1)[SDL_SCANCODE_R] &&
          state.ForPlayer(config, 2)[SDL_SCANCODE_R] && state.ForPlayer(config, 3)[SDL_SCANCODE_R],
          "release in one of four keyboards cannot release the other three");
    state.Key("extra-3", SDL_SCANCODE_R, true); state.Key("main-3", SDL_SCANCODE_R, false);
    Check(state.ForPlayer(config, 3)[SDL_SCANCODE_R], "fourth player's auxiliary interface keeps the held key");
    GamepadAssignments pads;
    AssignGamepad(pads, 1, "A"); AssignGamepad(pads, 3, "B");
    std::vector<GamepadDevice> devices{{1,"A","DualSense"},{2,"B","DualSense"},{3,"C","Pad"},{4,"D","Pad"}};
    auto assigned = ResolveGamepadAssignments(pads, devices);
    Check(assigned[1] == 1 && assigned[3] == 2 && assigned[0] == 3 && assigned[2] == 4,
          "explicit second/fourth pads are reserved before automatic first/third slots");
    devices.erase(devices.begin()); assigned = ResolveGamepadAssignments(pads, devices);
    Check(!assigned[1] && assigned[3] == 2, "disconnecting P2 preserves P4 without stealing another pad");
    AssignGamepad(pads, 2, "B");
    Check(pads.device[3] == "none" && pads.device[1] == "A", "moving a pad across four slots affects only its old owner");
    std::printf("PASS: %u collection input checks (filtered actions, four players, auxiliary interfaces).\n", checks);
}
