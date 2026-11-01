// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "bindings.h"

namespace port {
// The frontend and virtual-controller tests use this same polling path.
inline uint32_t ReadGamepadInput(SDL_Gamepad *gamepad, const Bindings &bindings, int player,
                                      const BoundTable &table = MakeBoundTable(4, "", true)) {
    if (!gamepad || player < 0 || player >= kBoundPlayers) return 0;
    uint32_t mask = 0;
    bool explicitStickDirections = false;
    for (int action = 0; action < table.table.count; ++action) {
        const auto input = bindings.pad[player][action];
        if (action < 4 && recomp::IsStickAxis(input.axis)) explicitStickDirections = true;
        if (recomp::PadInputPressed(gamepad, input)) mask |= table.bits[action];
    }
    // Existing profiles keep their automatic left stick. Explicit stick
    // directions replace that fallback, so inversion/right-stick layouts do
    // not also generate unwanted directions from the old mapping.
    if (!explicitStickDirections) {
        const PadInput legacy[] = {{-1, SDL_GAMEPAD_AXIS_LEFTY, -1}, {-1, SDL_GAMEPAD_AXIS_LEFTY, 1},
                                   {-1, SDL_GAMEPAD_AXIS_LEFTX, -1}, {-1, SDL_GAMEPAD_AXIS_LEFTX, 1}};
        for (int direction = 0; direction < 4; ++direction) {
            bool assigned = false;
            for (int action = 0; action < table.table.count; ++action)
                assigned |= bindings.pad[player][action] == legacy[direction];
            if (!assigned && recomp::PadInputPressed(gamepad, legacy[direction])) mask |= kBoundActions[direction].bit;
        }
    }
    return mask;
}
} // namespace port
