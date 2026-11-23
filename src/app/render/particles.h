#pragma once

#include <raylib.h>

#include "core/math_types.h"
#include "core/simulation.h"

namespace eco::app {

// Configurazione in tempo reale degli effetti ambientali (regolabile dal Menu di Pausa / Impostazioni).
struct EnvironmentEffectsConfig {
    float particle_density = 1.0f;       // [0..1] densita' particelle globali
    float fog_haze_intensity = 1.0f;     // [0..1] foschia/nebbia, smog e tinta atmosferica di zona
    float sun_rays_intensity = 1.0f;     // [0..1] raggi solari / luce volumetrica
    float vignette_intensity = 1.0f;     // [0..1] vignettatura cinematografica
    float zone_weather_intensity = 1.0f; // [0..1] vento, polline, vapore fumarole, lucciole, scintille armoniche
    bool atmosphere_enabled = true;      // interruttore globale post-processing atmosferico
};

void SetEnvironmentEffectsConfig(const EnvironmentEffectsConfig& cfg) noexcept;
[[nodiscard]] const EnvironmentEffectsConfig& GetEnvironmentEffectsConfig() noexcept;

// Scala globale della densita' di particelle e atmosfera (1.0f = Alta, 0.5f = Media, 0.0f = Disattivata).
// Wrapper compatibile che aggiorna EnvironmentEffectsConfig::particle_density.
void SetParticleQualityScale(float scale) noexcept;
[[nodiscard]] float GetParticleQualityScale() noexcept;

// Sistema particellare cosmetico differenziato per zona/struttura + reazione live alle meccaniche di zona.
// Non modifica MAI lo stato della simulazione.
void DrawWorldAtmosphericParticles(
    const eco::WorldContext& world,
    eco::Vec2 camera_target);

// Post-processing atmosferico screen-space (tinta distretto, raggi solari, foschia/cenere, vignettatura).
void DrawScreenSpaceAtmosphere(
    const eco::WorldContext& world);

// Intramezzo visivo procedurale tra due zone (vignetta panoramica dedicata alla soglia, parallasse,
// dissolvenza morbida, spazio tipografico per titolo/versi e barra di avanzamento con hint di salto).
void DrawZoneInterludeOverlay(
    eco::ZoneId from_zone,
    eco::ZoneId to_zone,
    float progress,
    float sim_time,
    float pollution_ratio,
    const char* title,
    const char* body_line1,
    const char* body_line2,
    const char* skip_hint = "Salta",
    const char* direction_tag = nullptr);

// Overload di comodo che legge stato intramezzo e testi direttamente da WorldContext + ZoneInterludeCopy.
void DrawZoneInterludeOverlay(const eco::WorldContext& world);

} // namespace eco::app
