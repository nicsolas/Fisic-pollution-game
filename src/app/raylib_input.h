#pragma once

#include "core/input_types.h"

namespace eco::app {

// Polls Raylib keyboard (P0 WASD/ZQSD, P1 Arrows) and gamepads 0..3 into AllPlayersInput.
// Must only be called after InitWindow() (never in headless mode).
[[nodiscard]] eco::AllPlayersInput PollRaylibInput();

} // namespace eco::app
