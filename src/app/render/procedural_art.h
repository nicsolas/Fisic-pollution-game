#pragma once

#include <cstdint>

#include "core/math_types.h"
#include "core/simulation.h"

namespace eco::app {

// Hash deterministico 2D per variazioni procedurali (ciottoli, erba, fiori, crepe, rilievi)
[[nodiscard]] std::uint32_t Hash2(int x, int y, int seed) noexcept;
[[nodiscard]] float Hash2Float01(int x, int y, int seed) noexcept;

// Rendering completo della mappa Overworld de "La Conca di Meridia" (6 distretti 36x50,
// viewport culling su camera_target, pavimentazioni intarsiate, mosaici astronomici,
// canali con riflessi e sponde scolpite, Ponte degli Archi, rovine 2.5D e 3 portali dedicati).
void DrawOverworldProceduralMap(const eco::WorldContext& world);

// Flora diversificata per bioma, bio-cristalli geodetici, colonnine/lanterne e stele diegetiche
// nell'Overworld (con anti-sovrapposizione automatica rispetto a macchine, NPC, portali e reliquie).
void DrawOverworldProceduralProps(const eco::WorldContext& world);

// Sfondo stratificato in parallasse e tilemap 2.5D dedicati a ciascuna delle 3 strutture interne
// (Cisterne Ipogee, Torre del Pendolo, Camera Geotermica).
void DrawStructureProceduralBackdropAndMap(
    const eco::WorldContext& world,
    eco::Vec2 camera_target);

} // namespace eco::app
