#pragma once

#include "core/math_types.h"

namespace eco {

struct ZoneEcoStatus {
    EcoHealthBand band = EcoHealthBand::Polluted;
    float local_vitality = 0.50f;     // 0..1
    float flora_bloom = 0.35f;        // 0..1
    int fauna_count = 3;              // 0..12
    float current_speed_mult = 1.0f;  // moltiplicatore velocita' correnti canali
    float current_turbulence = 0.35f; // 0..1 schiuma/vortici
    bool brambles_cleared = false;    // sentieri bassi liberati dai rovi di pece
    bool risky_shortcut_open = false; // cresta arsa / sfioratore supercritico aperti
    bool laminar_ford_open = false;   // Guado degli Idraulici asciutto
    float healed_speed_bonus = 0.0f;  // +0.12f sui sentieri fioriti/guariti
};

class EcoSystem {
public:
    float pollution_level = kInitialPollution;
    float clean_energy = 0.0f;
    float dirty_energy = 0.0f;
    UiPhase ui_phase = UiPhase::Phase1_Minimal;
    bool architect_recruited = false;

    void Reset() noexcept;

    void ApplyCleanActivation(float energy_gain, float pollution_reduction) noexcept;
    void ApplyRiskyActivation(float energy_gain, float pollution_increase) noexcept;
    void ApplyDirtyBypass(float energy_gain, float pollution_increase) noexcept;
    void ApplyPollutionDelta(float delta) noexcept;
    void SetArchitectRecruited(bool recruited) noexcept;

    [[nodiscard]] EcoHealthBand GlobalHealthBand() const noexcept;
    [[nodiscard]] UiPhase MaxUnlockedUiPhase() const noexcept;
    void CycleUiPhase() noexcept;

    [[nodiscard]] float PollutionRatio() const noexcept;
    [[nodiscard]] float TotalEnergy() const noexcept;
    [[nodiscard]] ColorRgba TintColor(ColorRgba base) const noexcept;
    [[nodiscard]] float SmogAlpha() const noexcept;
};

} // namespace eco
