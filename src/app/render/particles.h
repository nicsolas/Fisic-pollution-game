#pragma once

#include <raylib.h>

#include "core/ecosystem.h"
#include "core/math_types.h"

namespace eco::app {

// Scala globale della densita' di particelle e atmosfera (1.0f = Alta, 0.5f = Media, 0.0f = Disattivata).
void SetParticleQualityScale(float scale) noexcept;
[[nodiscard]] float GetParticleQualityScale() noexcept;

// Sistema particellare cosmetico e post-processing screen-space (proprietario: artista).
// Non modifica MAI lo stato della simulazione.
void DrawWorldAtmosphericParticles(
    eco::GameMode mode,
    float sim_time,
    const eco::EcoSystem& ecosystem,
    eco::Vec2 camera_target);

void DrawScreenSpaceAtmosphere(
    eco::GameMode mode,
    float sim_time,
    const eco::EcoSystem& ecosystem);

} // namespace eco::app
