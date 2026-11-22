#pragma once

#include <cstdint>

#include "core/ecosystem.h"
#include "core/simulation.h"
#include "core/tilemap.h"

namespace eco::app {

// Hash deterministico 2D per variazioni procedurali (ciottoli, erba, fiori, crepe)
[[nodiscard]] std::uint32_t Hash2(int x, int y, int seed) noexcept;
[[nodiscard]] float Hash2Float01(int x, int y, int seed) noexcept;

// Rendering completo della mappa Overworld (terreno organico, sentieri, Canale del Crati,
// Ponte Bucci, rovine 2.5D con ombre portate, portale della Diga e flora procedurale).
void DrawOverworldProceduralMap(
    const eco::Tilemap& tilemap,
    const eco::EcoSystem& ecosystem,
    float sim_time);

// Alberi, cespugli, bio-cristalli e cartelli diegetici nell'Overworld.
void DrawOverworldProceduralProps(
    const eco::WorldContext& world);

// Sfondo stratificato in parallasse e tilemap metallica 2.5D per la Diga (StructureState).
void DrawStructureProceduralBackdropAndMap(
    const eco::Tilemap& tilemap,
    const eco::EcoSystem& ecosystem,
    eco::Vec2 camera_target,
    float sim_time);

} // namespace eco::app
