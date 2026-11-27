#pragma once

#include "core/ecosystem.h"
#include "core/math_types.h"
#include "core/player.h"

namespace eco {

inline constexpr float kSolarEfficiencyThreshold = 0.80f;
inline constexpr float kHydroEfficiencyThreshold = 0.80f;
inline constexpr float kPendulumEfficiencyThreshold = 0.80f;
inline constexpr float kHarmonicEfficiencyThreshold = 0.80f;
inline constexpr float kThermalEfficiencyThreshold = 0.80f;
inline constexpr float kAstrolabeEfficiencyThreshold = 0.85f;

inline constexpr float kWaterDensity = 1000.0f;
inline constexpr float kGravityWater = 9.81f;
inline constexpr float kDefaultHydroTolerancePa = 20000.0f;
inline constexpr float kDefaultPendulumToleranceSec = 0.45f;
inline constexpr float kDefaultHarmonicQFactor = 3.0f;
inline constexpr float kDefaultCarnotColdTempK = 300.0f;
inline constexpr float kDefaultThermalToleranceK = 150.0f;

inline constexpr float kDefaultMachineTriggerRadius = 76.0f;
inline constexpr float kDefaultScannerRange = 280.0f;

inline constexpr const char* kMachineInteractPrompt =
    "Premi [E] per calibrare il monumento";
inline constexpr const char* kCombustionBypassLabel =
    "Usa Fornace a Pece di Emergenza (Bypass)";

[[nodiscard]] constexpr const char* MachineName(MachineType type) noexcept {
    switch (type) {
        case MachineType::SolarArray:
            return "Eliostato Solare";
        case MachineType::HydroTurbine:
            return "Turbina Idrostatica";
        case MachineType::PendulumRegulator:
            return "Pendolo Isochronos";
        case MachineType::HarmonicResonator:
            return "Risonatore Armonico";
        case MachineType::ThermalExchanger:
            return "Scambiatore di Carnot";
        case MachineType::CentralAstrolabe:
            return "Astrolabio Centrale";
        case MachineType::Count:
            break;
    }
    return "Monumento";
}

[[nodiscard]] constexpr const char* MachineLawName(MachineType type) noexcept {
    switch (type) {
        case MachineType::SolarArray:
            return "Ottica - Legge di Lambert";
        case MachineType::HydroTurbine:
            return "Idrostatica - Legge di Stevino";
        case MachineType::PendulumRegulator:
            return "Meccanica - Isocronismo di Galileo";
        case MachineType::HarmonicResonator:
            return "Acustica - Risonanza Armonica";
        case MachineType::ThermalExchanger:
            return "Termodinamica - Ciclo di Carnot";
        case MachineType::CentralAstrolabe:
            return "Sintesi - Interferenza di Fase";
        case MachineType::Count:
            break;
    }
    return "Legge Fisica";
}

[[nodiscard]] constexpr const char* MachineUnit(MachineType type) noexcept {
    switch (type) {
        case MachineType::SolarArray:
            return "deg";
        case MachineType::HydroTurbine:
            return "m";
        case MachineType::PendulumRegulator:
            return "m";
        case MachineType::HarmonicResonator:
            return "Hz";
        case MachineType::ThermalExchanger:
            return "K";
        case MachineType::CentralAstrolabe:
            return "deg";
        case MachineType::Count:
            break;
    }
    return "";
}

[[nodiscard]] constexpr const char* MachineFormulaLabel(MachineType type) noexcept {
    switch (type) {
        case MachineType::SolarArray:
            return "eta = cos(theta - theta_sole)";
        case MachineType::HydroTurbine:
            return "P = rho * g * h";
        case MachineType::PendulumRegulator:
            return "T = 2*pi * sqrt(L / g)";
        case MachineType::HarmonicResonator:
            return "R = 1 / sqrt(1 + Q^2 * (f/f0 - f0/f)^2)";
        case MachineType::ThermalExchanger:
            return "eta_C = 1 - T_fredda / T_calda";
        case MachineType::CentralAstrolabe:
            return "I = cos^2((phi - phi_0) / 2)";
        case MachineType::Count:
            break;
    }
    return "";
}

[[nodiscard]] float ComputeSolarEfficiency(float panel_angle_deg, float sun_angle_deg) noexcept;

[[nodiscard]] float ComputeHydroEfficiency(
    float head_m,
    float target_head_m,
    float tolerance_pa = kDefaultHydroTolerancePa) noexcept;

[[nodiscard]] float ComputePendulumPeriod(
    float length_m,
    float gravity = kGravityWater) noexcept;

[[nodiscard]] float ComputePendulumEfficiency(
    float length_m,
    float target_length_m,
    float period_tolerance_s = kDefaultPendulumToleranceSec) noexcept;

[[nodiscard]] float ComputeHarmonicEfficiency(
    float freq_hz,
    float target_freq_hz,
    float q_factor = kDefaultHarmonicQFactor) noexcept;

[[nodiscard]] float ComputeCarnotEta(
    float hot_temp_k,
    float cold_temp_k = kDefaultCarnotColdTempK) noexcept;

[[nodiscard]] float ComputeThermalEfficiency(
    float hot_temp_k,
    float target_hot_k,
    float tolerance_k = kDefaultThermalToleranceK) noexcept;

[[nodiscard]] float ComputeAstrolabeEfficiency(
    float phase_deg,
    float target_phase_deg) noexcept;

struct TrialExperimentReport {
    bool executed = false;
    MachineType machine_type = MachineType::SolarArray;
    HypothesisKind tested_hypothesis = HypothesisKind::HarmonicEquilibrium;
    float tested_param = 0.0f;
    float tested_stabilizer = 0.65f;
    float measured_efficiency = 0.0f;
    float thermal_or_hydraulic_stress = 0.0f; // 0..1
    int error_direction = 0;                  // -1 = basso, 0 = in banda, +1 = alto
    bool harmonic_band_hit = false;
    bool critical_band_hit = false;
    float pollution_delta_applied = 0.0f;
    const char* empirical_reading = "";
    const char* world_consequence = "";
};

struct Machine {
    MachineType type = MachineType::SolarArray;
    MachineState state = MachineState::Broken;
    CalibrationApproach approach = CalibrationApproach::None;
    LocationId location = LocationId::Overworld;
    ZoneId zone = ZoneId::DawnTerraces;
    Vec2 pos{0.0f, 0.0f};
    float trigger_radius = kDefaultMachineTriggerRadius;
    float current_param = 0.0f;
    float target_param = 45.0f;
    float min_param = -90.0f;
    float max_param = 180.0f;
    float step_param = 1.0f;
    float tolerance_pa = kDefaultHydroTolerancePa;
    float aux_param = 0.0f;
    float stabilizer_value = 0.68f;
    float clean_stabilizer_target = 0.70f;
    float clean_stabilizer_half_window = 0.16f;
    float risky_param_min = 48.0f;
    float risky_param_max = 68.0f;
    float risky_max_stabilizer = 0.40f;
    float clean_energy_reward = 18.0f;
    float clean_pollution_reduction = 16.0f;
    float risky_energy_reward = 32.0f;
    float risky_pollution_increase = 4.0f;
    float dirty_energy_reward = 22.0f;
    float dirty_pollution_increase = 22.0f;
    float applied_clean_energy = 0.0f;
    float applied_dirty_energy = 0.0f;
    float applied_pollution_delta = 0.0f;
    int trials_count = 0;
    bool unlocked = true;
    bool allow_bypass = true;
    bool prompt_visible = false;

    [[nodiscard]] static Machine CreateDefaultSolar(Vec2 pos = {552.0f, 1416.0f}) noexcept;
    [[nodiscard]] static Machine CreateDefaultHydro(Vec2 pos = {648.0f, 648.0f}) noexcept;
    [[nodiscard]] static Machine CreateDefaultPendulum(Vec2 pos = {720.0f, 360.0f}) noexcept;
    [[nodiscard]] static Machine CreateDefaultHarmonic(Vec2 pos = {1752.0f, 840.0f}) noexcept;
    [[nodiscard]] static Machine CreateDefaultThermal(Vec2 pos = {1272.0f, 456.0f}) noexcept;
    [[nodiscard]] static Machine CreateDefaultAstrolabe(Vec2 pos = {1800.0f, 1416.0f}) noexcept;

    [[nodiscard]] float EvaluateEfficiency(float param) const noexcept;
    [[nodiscard]] float LiveEfficiency() const noexcept;
    [[nodiscard]] float Threshold() const noexcept;
    [[nodiscard]] bool IsInHarmonicStabilizerBand(float stab, bool widened_by_relic = false) const noexcept;
    [[nodiscard]] bool IsInCriticalOverdriveBand(float param, float stab) const noexcept;
    [[nodiscard]] float EvaluateRiskyOutput(float param, float stab) const noexcept;
    [[nodiscard]] float EvaluateOperationalStress(float param, float stab) const noexcept;
    [[nodiscard]] float DistanceToPlayer(const Player& player) const noexcept;
    [[nodiscard]] bool IsPlayerInRange(const Player& player) const noexcept;
    [[nodiscard]] bool AnyActivePlayerInRange(const PlayerManager& players) const noexcept;
    void UpdateProximity(const PlayerManager& players) noexcept;
};

struct CalibrationModalState {
    bool is_open = false;
    MachineType target_machine = MachineType::SolarArray;
    float slider_value = 0.0f;
    float stabilizer_value = 0.68f;
    float preview_efficiency = 0.0f;
    float preview_risky_output = 0.0f;
    float preview_stress = 0.0f;
    bool can_confirm_clean = false;
    bool can_confirm_risky = false;
    bool precision_relic_active = false;   // Dawn_FirstMeasureStele
    bool piezometric_relic_active = false; // Waters_SluiceStele
    bool olive_seed_bonus_active = false;  // Dawn_WhiteOliveSeed
    bool harmonic_band_revealed = false;
    bool hard_physics_mode = false;
    HypothesisKind selected_hypothesis = HypothesisKind::HarmonicEquilibrium;
    InquiryStage modal_stage = InquiryStage::Observe;
    int trials_run = 0;
    TrialExperimentReport last_trial{};

    void Reset() noexcept;
    void OpenFor(
        const Machine& machine,
        bool has_precision_relic = false,
        bool has_piezo_relic = false,
        bool has_olive_seed_bonus = false,
        InquiryStage current_stage = InquiryStage::Observe,
        bool hard_mode = false) noexcept;
    void SetParameter(float value, const Machine& machine) noexcept;
    void AdjustParameter(float delta, const Machine& machine) noexcept;
    void SetStabilizer(float value, const Machine& machine) noexcept;
    void AdjustStabilizer(float delta, const Machine& machine) noexcept;
    void SelectHypothesis(HypothesisKind kind) noexcept;
    void CycleHypothesis(int delta = 1) noexcept;
    void RecomputePreviews(const Machine& machine) noexcept;
    TrialExperimentReport RunTrial(Machine& machine, EcoSystem& ecosystem) noexcept;
    bool ConfirmClean(Machine& machine, EcoSystem& ecosystem) noexcept;
    bool ConfirmRisky(Machine& machine, EcoSystem& ecosystem) noexcept;
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
