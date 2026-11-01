// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "launcher.h"
void DrawGlobalControls(launcher::PageContext &page);
bool ConsumeGlobalControlsEvent(const SDL_Event &event);
bool CapturingGlobalControls();
