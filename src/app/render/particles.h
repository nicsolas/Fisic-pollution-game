#pragma once

#include <raylib.h>

#include "core/math_types.h"
#include "core/simulation.h"

namespace eco::app {

// Scala globale della densita' di particelle e atmosfera (1.0f = Alta, 0.5f = Media, 0.0f = Disattivata).
void SetParticleQualityScale(float scale) noexcept;
[[nodiscard]] float GetParticleQualityScale() noexcept;

// Sistema particellare cosmetico differenziato per distretto/struttura + burst raccolta reliquie.
// Non modifica MAI lo stato della simulazione.
void DrawWorldAtmosphericParticles(
    const eco::WorldContext& world,
    eco::Vec2 camera_target);

// Post-processing atmosferico screen-space (raggi d'alba, foschia/cenere di pece, vignettatura).
void DrawScreenSpaceAtmosphere(
    const eco::WorldContext& world);

} // namespace eco::app
