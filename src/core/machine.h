#pragma once

#include "core/ecosystem.h"
#include "core/math_types.h"
#include "core/player.h"

namespace eco {

inline constexpr float kSolarEfficiencyThreshold = 0.80f;
inline constexpr float kHydroEfficiencyThreshold = 0.80f;
inline constexpr float kWaterDensity = 1000.0f;
inline constexpr float kGravityWater = 9.81f;
inline constexpr float kDefaultHydroTolerancePa = 20000.0f;
inline constexpr float kDefaultMachineTriggerRadius = 72.0f;
inline constexpr float kDefaultScannerRange = 250.0f;

inline constexpr const char* kMachineInteractPrompt =
    "Premi Interagisci per riavviare l'impianto";
inline constexpr const char* kCombustionBypassLabel =
    "Usa Generatore di Emergenza a Combustione (Bypass)";

[[nodiscard]] float ComputeSolarEfficiency(float panel_angle_deg, float sun_angle_deg) noexcept;

[[nodiscard]] float ComputeHydroEfficiency(
    float head_m,
    float target_head_m,
    float tolerance_pa = kDefaultHydroTolerancePa) noexcept;

struct Machine {
    MachineType type = MachineType::SolarArray;
    MachineState state = MachineState::Broken;
    Vec2 pos{0.0f, 0.0f};
    float trigger_radius = kDefaultMachineTriggerRadius;
    float current_param = 0.0f;
    float target_param = 45.0f;
    float tolerance_pa = kDefaultHydroTolerancePa;
    float clean_energy_reward = 35.0f;
    float clean_pollution_reduction = 15.0f;
    float dirty_energy_reward = 35.0f;
    float dirty_pollution_increase = 25.0f;
    bool prompt_visible = false;

    [[nodiscard]] static Machine CreateDefaultSolar(Vec2 pos = {360.0f, 504.0f}) noexcept;
    [[nodiscard]] static Machine CreateDefaultHydro(Vec2 pos = {520.0f, 580.0f}) noexcept;

    [[nodiscard]] float EvaluateEfficiency(float param) const noexcept;
    [[nodiscard]] float LiveEfficiency() const noexcept;
    [[nodiscard]] float Threshold() const noexcept;
    [[nodiscard]] float DistanceToPlayer(const Player& player) const noexcept;
    [[nodiscard]] bool IsPlayerInRange(const Player& player) const noexcept;
    [[nodiscard]] bool AnyActivePlayerInRange(const PlayerManager& players) const noexcept;
    void UpdateProximity(const PlayerManager& players) noexcept;
};

struct CalibrationModalState {
    bool is_open = false;
    MachineType target_machine = MachineType::SolarArray;
    float slider_value = 0.0f;
    float preview_efficiency = 0.0f;
    bool can_confirm_clean = false;

    void Reset() noexcept;
    void OpenFor(const Machine& machine) noexcept;
    void SetParameter(float value, const Machine& machine) noexcept;
    void AdjustParameter(float delta, const Machine& machine) noexcept;
    bool ConfirmClean(Machine& machine, EcoSystem& ecosystem) noexcept;
    void TriggerBypass(Machine& machine, EcoSystem& ecosystem) noexcept;
    void Cancel() noexcept;
};

struct EmpiricalScanReading {
    bool active = false;
    bool has_target = false;
    MachineType target_type = MachineType::SolarArray;
    float measured_target_value = 0.0f;
    float measured_distance = 0.0f;
    float measured_efficiency = 0.0f;
};

[[nodiscard]] EmpiricalScanReading PerformEmpiricalScan(
    int player_id,
    const PlayerManager& players,
    const Machine& scene_machine,
    float max_range = kDefaultScannerRange) noexcept;

} // namespace eco
