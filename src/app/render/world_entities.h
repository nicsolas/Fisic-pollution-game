#pragma once

#include <raylib.h>

#include <string>
#include <string_view>

#include "core/collectible.h"
#include "core/player.h"
#include "core/simulation.h"

namespace eco::app {

// Rendering world-space (da chiamare dentro BeginMode2D / EndMode2D)
void DrawScientistProtagonist(const eco::Player& player, const eco::WorldContext& world);
void DrawSceneMachineAndOverlaysWorldSpace(const eco::WorldContext& world);

// Helper visivi condivisi tra world-space e HUD/modal di calibrazione
[[nodiscard]] Color EraAccentColor(eco::CollectibleEra era) noexcept;
void DrawArrow2D(Vector2 start, Vector2 end, float thickness, Color color);
[[nodiscard]] int ActivatedCoreMonumentsCount(const eco::WorldContext& world) noexcept;
[[nodiscard]] std::string FitModalText(
    std::string_view text,
    int max_width_px,
    int font_size);

} // namespace eco::app
