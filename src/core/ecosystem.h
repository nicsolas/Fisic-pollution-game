#pragma once

#include "core/math_types.h"

namespace eco {

class EcoSystem {
public:
    float pollution_level = kInitialPollution;
    float clean_energy = 0.0f;
    float dirty_energy = 0.0f;
    UiPhase ui_phase = UiPhase::Phase1_Minimal;
    bool architect_recruited = false;

    void Reset() noexcept;

    void ApplyCleanActivation(float energy_gain, float pollution_reduction) noexcept;
    void ApplyDirtyBypass(float energy_gain, float pollution_increase) noexcept;
    void SetArchitectRecruited(bool recruited) noexcept;

    [[nodiscard]] UiPhase MaxUnlockedUiPhase() const noexcept;
    void CycleUiPhase() noexcept;

    [[nodiscard]] float PollutionRatio() const noexcept;
    [[nodiscard]] float TotalEnergy() const noexcept;
    [[nodiscard]] ColorRgba TintColor(ColorRgba base) const noexcept;
    [[nodiscard]] float SmogAlpha() const noexcept;
};

} // namespace eco
