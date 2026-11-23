#pragma once

#include <cstddef>
#include <cstdint>

#include "core/math_types.h"
#include "core/simulation.h"

namespace eco::app {

// Hash deterministico 2D per variazioni procedurali (ciottoli, erba, fiori, crepe, rilievi)
[[nodiscard]] std::uint32_t Hash2(int x, int y, int seed) noexcept;
[[nodiscard]] float Hash2Float01(int x, int y, int seed) noexcept;

// Gestione memoria cache procedurale per-zona (libera fisicamente la zona precedente allo switch)
void ReleaseOverworldProceduralCache() noexcept;
[[nodiscard]] std::size_t ProceduralArtCacheBytes() noexcept;

// Rendering completo della mappa della zona Overworld attiva (6 zone separate 26x36,
// viewport culling su camera_target, pavimentazioni dedicate per bioma, canali, ponti di giada,
// barriere solari, varchi di zona, meccaniche a terra, mosaici e murature 2.5D).
void DrawOverworldProceduralMap(const eco::WorldContext& world);

// Flora diversificata per bioma, affreschi ambientali, varchi monumentali, elementi elevati
// delle 6 meccaniche top-down (specchi/fasci solari, leve/chiuse, monoliti frangivento e raffiche,
// onde di risonanza, getti fumarole e valvole, raggi dei sigilli) e stele diegetiche.
void DrawOverworldProceduralProps(const eco::WorldContext& world);

// Sfondo stratificato in parallasse e tilemap 2.5D dedicati a ciascuna delle 3 strutture interne
// (Cisterne Ipogee, Torre del Pendolo, Camera Geotermica).
void DrawStructureProceduralBackdropAndMap(
    const eco::WorldContext& world,
    eco::Vec2 camera_target);

} // namespace eco::app
