#include "core/ecosystem.h"

#include <cmath>
#include <cstdint>

namespace eco {

namespace {

[[nodiscard]] float SafeNonNegative(float value) noexcept {
    if (!std::isfinite(value) || value <= 0.0f) {
        return 0.0f;
    }
    return value;
}

} // namespace

void EcoSystem::Reset() noexcept {
    pollution_level = kInitialPollution;
    clean_energy = 0.0f;
    dirty_energy = 0.0f;
    ui_phase = UiPhase::Phase1_Minimal;
    architect_recruited = false;
}

void EcoSystem::ApplyCleanActivation(float energy_gain, float pollution_reduction) noexcept {
    const float gain = SafeNonNegative(energy_gain);
    const float reduction = SafeNonNegative(pollution_reduction);

    clean_energy = Clamp(clean_energy + gain, kMinEcoValue, kMaxEcoValue);
    pollution_level = Clamp(pollution_level - reduction, kMinEcoValue, kMaxEcoValue);

    if (clean_energy > 0.0f && ui_phase == UiPhase::Phase1_Minimal) {
        ui_phase = UiPhase::Phase2_Telemetry;
    }
}

void EcoSystem::ApplyRiskyActivation(float energy_gain, float pollution_increase) noexcept {
    const float gain = SafeNonNegative(energy_gain);
    const float increase = SafeNonNegative(pollution_increase);

    clean_energy = Clamp(clean_energy + gain, kMinEcoValue, kMaxEcoValue);
    pollution_level = Clamp(pollution_level + increase, kMinEcoValue, kMaxEcoValue);

    if (clean_energy > 0.0f && ui_phase == UiPhase::Phase1_Minimal) {
        ui_phase = UiPhase::Phase2_Telemetry;
    }
}

void EcoSystem::ApplyDirtyBypass(float energy_gain, float pollution_increase) noexcept {
    const float gain = SafeNonNegative(energy_gain);
    const float increase = SafeNonNegative(pollution_increase);

    dirty_energy = Clamp(dirty_energy + gain, kMinEcoValue, kMaxEcoValue);
    pollution_level = Clamp(pollution_level + increase, kMinEcoValue, kMaxEcoValue);
}

void EcoSystem::ApplyPollutionDelta(float delta) noexcept {
    if (!std::isfinite(delta)) {
        return;
    }
    pollution_level = Clamp(pollution_level + delta, kMinEcoValue, kMaxEcoValue);
}

EcoHealthBand EcoSystem::GlobalHealthBand() const noexcept {
    if (pollution_level >= 65.0f) {
        return EcoHealthBand::Critical;
    }
    if (pollution_level >= 34.0f) {
        return EcoHealthBand::Polluted;
    }
    if (pollution_level > 18.0f) {
        return EcoHealthBand::Recovering;
    }
    return EcoHealthBand::Flourishing;
}

void EcoSystem::SetArchitectRecruited(bool recruited) noexcept {
    architect_recruited = recruited;
    if (recruited) {
        ui_phase = UiPhase::Phase3_EngineOverlay;
    } else if (ui_phase == UiPhase::Phase3_EngineOverlay) {
        ui_phase = MaxUnlockedUiPhase();
    }
}

UiPhase EcoSystem::MaxUnlockedUiPhase() const noexcept {
    if (architect_recruited) {
        return UiPhase::Phase3_EngineOverlay;
    }
    if (clean_energy > 0.0f) {
        return UiPhase::Phase2_Telemetry;
    }
    return UiPhase::Phase1_Minimal;
}

void EcoSystem::CycleUiPhase() noexcept {
    const int max_phase = static_cast<int>(MaxUnlockedUiPhase());
    const int current = static_cast<int>(ui_phase);
    const int next = (current >= max_phase) ? 1 : (current + 1);
    ui_phase = static_cast<UiPhase>(next);
}

float EcoSystem::PollutionRatio() const noexcept {
    return Clamp(pollution_level / kMaxEcoValue, 0.0f, 1.0f);
}

float EcoSystem::TotalEnergy() const noexcept {
    return clean_energy + dirty_energy;
}

ColorRgba EcoSystem::TintColor(ColorRgba base) const noexcept {
    const float t = PollutionRatio();
    if (t <= 0.0f) {
        return base;
    }

    const float r0 = static_cast<float>(base.r);
    const float g0 = static_cast<float>(base.g);
    const float b0 = static_cast<float>(base.b);

    // Rec. 601 perceptual luminance
    const float luma = 0.299f * r0 + 0.587f * g0 + 0.114f * b0;
    const float desat = t * 0.65f;

    const float r1 = Lerp(r0, luma, desat);
    const float g1 = Lerp(g0, luma, desat);
    const float b1 = Lerp(b0, luma, desat);

    const float brightness = Lerp(1.0f, 0.45f, t);
    const float r_out = r1 * brightness * Lerp(1.0f, 0.98f, t);
    const float g_out = g1 * brightness;
    const float b_out = b1 * brightness * Lerp(1.0f, 0.90f, t);

    auto to_u8 = [](float ch) noexcept -> std::uint8_t {
        return static_cast<std::uint8_t>(Clamp(std::round(ch), 0.0f, 255.0f));
    };

    return {to_u8(r_out), to_u8(g_out), to_u8(b_out), base.a};
}

float EcoSystem::SmogAlpha() const noexcept {
    return PollutionRatio() * 0.45f;
}

} // namespace eco
