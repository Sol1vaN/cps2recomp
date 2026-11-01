// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026. Local Input Lab extension to cps2recomp.
#pragma once
#include "launcher.h"
void DrawInputControls(launcher::PageContext &page, int player);

void DrawKeyboardControls(launcher::PageContext &page);
bool CapturingKeyboardControls();
bool ConsumeKeyboardControlsEvent(const SDL_Event &event);
SDL_Scancode PollKeyboardBinding(int player);

void DrawHardwareControls(launcher::PageContext &page);
