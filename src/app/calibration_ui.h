#pragma once

#include "core/player.h"
#include "core/simulation.h"

namespace eco::app {

void DrawScientistProtagonist(const eco::Player& player, const eco::WorldContext& world);
void DrawSceneMachineAndOverlaysWorldSpace(const eco::WorldContext& world);
void DrawHudAndCalibrationModalScreenSpace(eco::WorldContext& world);

} // namespace eco::app
