#include "core/machine.h"

#include <algorithm>
#include <cmath>

namespace eco {

float ComputeSolarEfficiency(float panel_angle_deg, float sun_angle_deg) noexcept {
    if (!std::isfinite(panel_angle_deg) || !std::isfinite(sun_angle_deg)) {
        return 0.0f;
    }
    float delta_deg = std::fmod(panel_angle_deg - sun_angle_deg, 360.0f);
    if (!std::isfinite(delta_deg)) {
        return 0.0f;
    }
    if (delta_deg > 180.0f) {
        delta_deg -= 360.0f;
    } else if (delta_deg < -180.0f) {
        delta_deg += 360.0f;
    }
    if (std::fabs(delta_deg) >= 90.0f) {
        return 0.0f;
    }
    const float raw_cos = std::cos(deg2rad(delta_deg));
    if (!std::isfinite(raw_cos) || raw_cos <= 1e-6f) {
        return 0.0f;
    }
    return Clamp(raw_cos, 0.0f, 1.0f);
}

float ComputeHydroEfficiency(
    float head_m,
    float target_head_m,
    float tolerance_pa) noexcept {
    if (!std::isfinite(head_m) || !std::isfinite(target_head_m) ||
        !std::isfinite(tolerance_pa) || tolerance_pa <= 0.0f) {
        return 0.0f;
    }
    const float pressure_pa = kWaterDensity * kGravityWater * head_m;
    const float target_pa = kWaterDensity * kGravityWater * target_head_m;
    const float diff_pa = std::fabs(pressure_pa - target_pa);
    if (!std::isfinite(diff_pa)) {
        return 0.0f;
    }
    const float raw_eff = 1.0f - (diff_pa / tolerance_pa);
    if (!std::isfinite(raw_eff) || raw_eff <= 0.0f) {
        return 0.0f;
    }
    return Clamp(raw_eff, 0.0f, 1.0f);
}

float ComputePendulumPeriod(float length_m, float gravity) noexcept {
    if (!std::isfinite(length_m) || !std::isfinite(gravity) ||
        length_m <= 0.0f || gravity <= 0.0f) {
        return 0.0f;
    }
    const float period = 2.0f * kPi * std::sqrt(length_m / gravity);
    return std::isfinite(period) ? period : 0.0f;
}

float ComputePendulumEfficiency(
    float length_m,
    float target_length_m,
    float period_tolerance_s) noexcept {
    if (!std::isfinite(length_m) || !std::isfinite(target_length_m) ||
        !std::isfinite(period_tolerance_s) || period_tolerance_s <= 0.0f ||
        length_m <= 0.0f || target_length_m <= 0.0f) {
        return 0.0f;
    }
    const float period = ComputePendulumPeriod(length_m);
    const float target_period = ComputePendulumPeriod(target_length_m);
    const float diff = std::fabs(period - target_period);
    if (!std::isfinite(diff)) {
        return 0.0f;
    }
    const float raw_eff = 1.0f - (diff / period_tolerance_s);
    if (!std::isfinite(raw_eff) || raw_eff <= 0.0f) {
        return 0.0f;
    }
    return Clamp(raw_eff, 0.0f, 1.0f);
}

float ComputeHarmonicEfficiency(
    float freq_hz,
    float target_freq_hz,
    float q_factor) noexcept {
    if (!std::isfinite(freq_hz) || !std::isfinite(target_freq_hz) ||
        !std::isfinite(q_factor) || freq_hz <= 0.0f ||
        target_freq_hz <= 0.0f || q_factor <= 0.0f) {
        return 0.0f;
    }
    const float detune = (freq_hz / target_freq_hz) - (target_freq_hz / freq_hz);
    const float denom = std::sqrt(1.0f + (q_factor * q_factor * detune * detune));
    if (!std::isfinite(denom) || denom <= 0.0f) {
        return 0.0f;
    }
    return Clamp(1.0f / denom, 0.0f, 1.0f);
}

float ComputeCarnotEta(float hot_temp_k, float cold_temp_k) noexcept {
    if (!std::isfinite(hot_temp_k) || !std::isfinite(cold_temp_k) ||
        hot_temp_k <= cold_temp_k || cold_temp_k <= 0.0f) {
        return 0.0f;
    }
    return Clamp(1.0f - (cold_temp_k / hot_temp_k), 0.0f, 1.0f);
}

float ComputeThermalEfficiency(
    float hot_temp_k,
    float target_hot_k,
    float tolerance_k) noexcept {
    if (!std::isfinite(hot_temp_k) || !std::isfinite(target_hot_k) ||
        !std::isfinite(tolerance_k) || tolerance_k <= 0.0f ||
        hot_temp_k <= kDefaultCarnotColdTempK) {
        return 0.0f;
    }
    const float diff = std::fabs(hot_temp_k - target_hot_k);
    if (!std::isfinite(diff)) {
        return 0.0f;
    }
    const float raw_eff = 1.0f - (diff / tolerance_k);
    if (!std::isfinite(raw_eff) || raw_eff <= 0.0f) {
        return 0.0f;
    }
    return Clamp(raw_eff, 0.0f, 1.0f);
}

float ComputeAstrolabeEfficiency(float phase_deg, float target_phase_deg) noexcept {
    if (!std::isfinite(phase_deg) || !std::isfinite(target_phase_deg)) {
        return 0.0f;
    }
    const float delta_rad = deg2rad(phase_deg - target_phase_deg);
    const float half_cos = std::cos(delta_rad * 0.5f);
    if (!std::isfinite(half_cos)) {
        return 0.0f;
    }
    return Clamp(half_cos * half_cos, 0.0f, 1.0f);
}

Machine Machine::CreateDefaultSolar(Vec2 pos) noexcept {
    Machine m{};
    m.type = MachineType::SolarArray;
    m.state = MachineState::Broken;
    m.approach = CalibrationApproach::None;
    m.location = LocationId::Overworld;
    m.zone = ZoneId::DawnTerraces;
    m.pos = pos;
    m.trigger_radius = kDefaultMachineTriggerRadius;
    m.current_param = 0.0f;
    m.target_param = 45.0f;
    m.min_param = -90.0f;
    m.max_param = 180.0f;
    m.step_param = 1.0f;
    m.tolerance_pa = kDefaultHydroTolerancePa;
    m.aux_param = 0.0f;
    m.stabilizer_value = 0.68f;
    m.clean_stabilizer_target = 0.70f;
    m.clean_stabilizer_half_window = 0.16f;
    m.risky_param_min = 48.0f;
    m.risky_param_max = 68.0f;
    m.risky_max_stabilizer = 0.40f;
    m.clean_energy_reward = 18.0f;
    m.clean_pollution_reduction = 16.0f;
    m.risky_energy_reward = 32.0f;
    m.risky_pollution_increase = 4.0f;
    m.dirty_energy_reward = 22.0f;
    m.dirty_pollution_increase = 22.0f;
    m.unlocked = true;
    m.allow_bypass = true;
    m.prompt_visible = false;
    return m;
}

Machine Machine::CreateDefaultHydro(Vec2 pos) noexcept {
    Machine m{};
    m.type = MachineType::HydroTurbine;
    m.state = MachineState::Broken;
    m.approach = CalibrationApproach::None;
    m.location = LocationId::HypogeanCisterns;
    m.zone = ZoneId::CanalQuarter;
    m.pos = pos;
    m.trigger_radius = kDefaultMachineTriggerRadius;
    m.current_param = 6.5f;
    m.target_param = 10.0f;
    m.min_param = 0.0f;
    m.max_param = 20.0f;
    m.step_param = 0.25f;
    m.tolerance_pa = kDefaultHydroTolerancePa;
    m.aux_param = kDefaultHydroTolerancePa;
    m.stabilizer_value = 0.68f;
    m.clean_stabilizer_target = 0.70f;
    m.clean_stabilizer_half_window = 0.16f;
    m.risky_param_min = 12.5f;
    m.risky_param_max = 15.0f;
    m.risky_max_stabilizer = 0.40f;
    m.clean_energy_reward = 18.0f;
    m.clean_pollution_reduction = 16.0f;
    m.risky_energy_reward = 32.0f;
    m.risky_pollution_increase = 4.0f;
    m.dirty_energy_reward = 22.0f;
    m.dirty_pollution_increase = 22.0f;
    m.unlocked = true;
    m.allow_bypass = true;
    m.prompt_visible = false;
    return m;
}

Machine Machine::CreateDefaultPendulum(Vec2 pos) noexcept {
    Machine m{};
    m.type = MachineType::PendulumRegulator;
    m.state = MachineState::Broken;
    m.approach = CalibrationApproach::None;
    m.location = LocationId::PendulumTower;
    m.zone = ZoneId::WindPlateau;
    m.pos = pos;
    m.trigger_radius = kDefaultMachineTriggerRadius;
    m.current_param = 0.80f;
    m.target_param = 2.00f;
    m.min_param = 0.20f;
    m.max_param = 5.00f;
    m.step_param = 0.05f;
    m.tolerance_pa = kDefaultHydroTolerancePa;
    m.aux_param = kDefaultPendulumToleranceSec;
    m.stabilizer_value = 0.68f;
    m.clean_stabilizer_target = 0.70f;
    m.clean_stabilizer_half_window = 0.16f;
    m.risky_param_min = 2.40f;
    m.risky_param_max = 3.25f;
    m.risky_max_stabilizer = 0.40f;
    m.clean_energy_reward = 18.0f;
    m.clean_pollution_reduction = 12.0f;
    m.risky_energy_reward = 30.0f;
    m.risky_pollution_increase = 4.0f;
    m.dirty_energy_reward = 20.0f;
    m.dirty_pollution_increase = 20.0f;
    m.unlocked = true;
    m.allow_bypass = true;
    m.prompt_visible = false;
    return m;
}

Machine Machine::CreateDefaultHarmonic(Vec2 pos) noexcept {
    Machine m{};
    m.type = MachineType::HarmonicResonator;
    m.state = MachineState::Broken;
    m.approach = CalibrationApproach::None;
    m.location = LocationId::Overworld;
    m.zone = ZoneId::HangingGardens;
    m.pos = pos;
    m.trigger_radius = kDefaultMachineTriggerRadius;
    m.current_param = 65.0f;
    m.target_param = 120.0f;
    m.min_param = 40.0f;
    m.max_param = 240.0f;
    m.step_param = 1.0f;
    m.tolerance_pa = kDefaultHydroTolerancePa;
    m.aux_param = kDefaultHarmonicQFactor;
    m.stabilizer_value = 0.68f;
    m.clean_stabilizer_target = 0.70f;
    m.clean_stabilizer_half_window = 0.16f;
    m.risky_param_min = 132.0f;
    m.risky_param_max = 162.0f;
    m.risky_max_stabilizer = 0.40f;
    m.clean_energy_reward = 20.0f;
    m.clean_pollution_reduction = 14.0f;
    m.risky_energy_reward = 32.0f;
    m.risky_pollution_increase = 4.0f;
    m.dirty_energy_reward = 22.0f;
    m.dirty_pollution_increase = 22.0f;
    m.unlocked = true;
    m.allow_bypass = true;
    m.prompt_visible = false;
    return m;
}

Machine Machine::CreateDefaultThermal(Vec2 pos) noexcept {
    Machine m{};
    m.type = MachineType::ThermalExchanger;
    m.state = MachineState::Broken;
    m.approach = CalibrationApproach::None;
    m.location = LocationId::GeothermalChamber;
    m.zone = ZoneId::FumaroleCaldera;
    m.pos = pos;
    m.trigger_radius = kDefaultMachineTriggerRadius;
    m.current_param = 380.0f;
    m.target_param = 600.0f;
    m.min_param = 320.0f;
    m.max_param = 900.0f;
    m.step_param = 5.0f;
    m.tolerance_pa = kDefaultHydroTolerancePa;
    m.aux_param = kDefaultThermalToleranceK;
    m.stabilizer_value = 0.68f;
    m.clean_stabilizer_target = 0.70f;
    m.clean_stabilizer_half_window = 0.16f;
    m.risky_param_min = 680.0f;
    m.risky_param_max = 800.0f;
    m.risky_max_stabilizer = 0.40f;
    m.clean_energy_reward = 22.0f;
    m.clean_pollution_reduction = 14.0f;
    m.risky_energy_reward = 34.0f;
    m.risky_pollution_increase = 5.0f;
    m.dirty_energy_reward = 22.0f;
    m.dirty_pollution_increase = 25.0f;
    m.unlocked = true;
    m.allow_bypass = true;
    m.prompt_visible = false;
    return m;
}

Machine Machine::CreateDefaultAstrolabe(Vec2 pos) noexcept {
    Machine m{};
    m.type = MachineType::CentralAstrolabe;
    m.state = MachineState::Broken;
    m.approach = CalibrationApproach::None;
    m.location = LocationId::Overworld;
    m.zone = ZoneId::HarmonySanctuary;
    m.pos = pos;
    m.trigger_radius = 92.0f;
    m.current_param = 15.0f;
    m.target_param = 72.0f;
    m.min_param = 0.0f;
    m.max_param = 360.0f;
    m.step_param = 1.0f;
    m.tolerance_pa = kDefaultHydroTolerancePa;
    m.aux_param = 0.0f;
    m.stabilizer_value = 0.70f;
    m.clean_stabilizer_target = 0.70f;
    m.clean_stabilizer_half_window = 0.20f;
    m.risky_param_min = 90.0f;
    m.risky_param_max = 120.0f;
    m.risky_max_stabilizer = 0.35f;
    m.clean_energy_reward = 25.0f;
    m.clean_pollution_reduction = 35.0f;
    m.risky_energy_reward = 35.0f;
    m.risky_pollution_increase = 0.0f;
    m.dirty_energy_reward = 0.0f;
    m.dirty_pollution_increase = 0.0f;
    m.unlocked = false;
    m.allow_bypass = false;
    m.prompt_visible = false;
    return m;
}

float Machine::EvaluateEfficiency(float param) const noexcept {
    switch (type) {
        case MachineType::SolarArray:
            return ComputeSolarEfficiency(param, target_param);
        case MachineType::HydroTurbine:
            return ComputeHydroEfficiency(param, target_param, tolerance_pa);
        case MachineType::PendulumRegulator: {
            const float tol = (std::isfinite(aux_param) && aux_param > 0.0f)
                                  ? aux_param
                                  : kDefaultPendulumToleranceSec;
            return ComputePendulumEfficiency(param, target_param, tol);
        }
        case MachineType::HarmonicResonator: {
            const float q = (std::isfinite(aux_param) && aux_param > 0.0f)
                                ? aux_param
                                : kDefaultHarmonicQFactor;
            return ComputeHarmonicEfficiency(param, target_param, q);
        }
        case MachineType::ThermalExchanger: {
            const float tol = (std::isfinite(aux_param) && aux_param > 0.0f)
                                  ? aux_param
                                  : kDefaultThermalToleranceK;
            return ComputeThermalEfficiency(param, target_param, tol);
        }
        case MachineType::CentralAstrolabe:
            return ComputeAstrolabeEfficiency(param, target_param);
        case MachineType::Count:
            break;
    }
    return 0.0f;
}

float Machine::LiveEfficiency() const noexcept {
    return EvaluateEfficiency(current_param);
}

float Machine::Threshold() const noexcept {
    switch (type) {
        case MachineType::SolarArray:
            return kSolarEfficiencyThreshold;
        case MachineType::HydroTurbine:
            return kHydroEfficiencyThreshold;
        case MachineType::PendulumRegulator:
            return kPendulumEfficiencyThreshold;
        case MachineType::HarmonicResonator:
            return kHarmonicEfficiencyThreshold;
        case MachineType::ThermalExchanger:
            return kThermalEfficiencyThreshold;
        case MachineType::CentralAstrolabe:
            return kAstrolabeEfficiencyThreshold;
        case MachineType::Count:
            break;
    }
    return kSolarEfficiencyThreshold;
}

bool Machine::IsInHarmonicStabilizerBand(float stab, bool widened_by_relic) const noexcept {
    if (!std::isfinite(stab)) {
        return false;
    }
    const float half_win = clean_stabilizer_half_window * (widened_by_relic ? 1.30f : 1.0f);
    return std::fabs(stab - clean_stabilizer_target) <= half_win + 1e-5f;
}

bool Machine::IsInCriticalOverdriveBand(float param, float stab) const noexcept {
    if (!std::isfinite(param) || !std::isfinite(stab) || type == MachineType::CentralAstrolabe) {
        return false;
    }
    if (stab >= risky_max_stabilizer) {
        return false;
    }
    if (type == MachineType::SolarArray) {
        // Fascio concentrato/supercritico: o nella finestra 48..68 deg oppure ad alta intensita' (eta >= 0.72) con smorzatore aperto
        const bool in_focal_arc = (param >= risky_param_min && param <= risky_param_max) ||
                                  (param >= 25.0f && param <= 42.0f);
        return in_focal_arc || EvaluateEfficiency(param) >= 0.75f;
    }
    return param >= risky_param_min - 1e-4f && param <= risky_param_max + 1e-4f;
}

float Machine::EvaluateRiskyOutput(float param, float stab) const noexcept {
    if (!std::isfinite(param) || !std::isfinite(stab)) {
        return 0.0f;
    }
    const float open_factor = Clamp((risky_max_stabilizer + 0.15f - stab) / 0.45f, 0.0f, 1.0f);
    const float mid = 0.5f * (risky_param_min + risky_param_max);
    const float half = std::max(0.5f, 0.5f * (risky_param_max - risky_param_min));
    float band_fit = Clamp(1.0f - std::fabs(param - mid) / (half * 1.4f), 0.0f, 1.0f);
    if (type == MachineType::SolarArray) {
        band_fit = std::max(band_fit, EvaluateEfficiency(param));
    }
    return Clamp(band_fit * (0.55f + 0.45f * open_factor), 0.0f, 1.0f);
}

float Machine::EvaluateOperationalStress(float param, float stab) const noexcept {
    if (!std::isfinite(param) || !std::isfinite(stab)) {
        return 1.0f;
    }
    const float eff = EvaluateEfficiency(param);
    const float detune_stress = (1.0f - eff) * 0.55f;
    const float open_stress = Clamp(1.0f - stab, 0.0f, 1.0f) * 0.55f;
    float head_over = 0.0f;
    if (param > target_param && max_param > target_param) {
        head_over = Clamp((param - target_param) / (max_param - target_param), 0.0f, 1.0f) * 0.35f;
    }
    return Clamp(detune_stress + open_stress + head_over, 0.0f, 1.0f);
}

float Machine::DistanceToPlayer(const Player& player) const noexcept {
    if (!std::isfinite(player.pos.x) || !std::isfinite(player.pos.y) ||
        !std::isfinite(pos.x) || !std::isfinite(pos.y)) {
        return 1e9f;
    }
    return player.Center().DistanceTo(pos);
}

bool Machine::IsPlayerInRange(const Player& player) const noexcept {
    if (!player.active || !std::isfinite(trigger_radius) || trigger_radius <= 0.0f) {
        return false;
    }
    return DistanceToPlayer(player) <= trigger_radius;
}

bool Machine::AnyActivePlayerInRange(const PlayerManager& players) const noexcept {
    for (const Player* p : players.ActivePlayers()) {
        if (p != nullptr && IsPlayerInRange(*p)) {
            return true;
        }
    }
    return false;
}

void Machine::UpdateProximity(const PlayerManager& players) noexcept {
    prompt_visible = AnyActivePlayerInRange(players);
}

namespace {

void UndoPreviousMachineContribution(Machine& machine, EcoSystem& ecosystem) noexcept {
    if (machine.applied_clean_energy != 0.0f) {
        ecosystem.clean_energy = Clamp(
            ecosystem.clean_energy - machine.applied_clean_energy, kMinEcoValue, kMaxEcoValue);
        machine.applied_clean_energy = 0.0f;
    }
    if (machine.applied_dirty_energy != 0.0f) {
        ecosystem.dirty_energy = Clamp(
            ecosystem.dirty_energy - machine.applied_dirty_energy, kMinEcoValue, kMaxEcoValue);
        machine.applied_dirty_energy = 0.0f;
    }
    if (machine.applied_pollution_delta != 0.0f) {
        ecosystem.pollution_level = Clamp(
            ecosystem.pollution_level - machine.applied_pollution_delta, kMinEcoValue, kMaxEcoValue);
        machine.applied_pollution_delta = 0.0f;
    }
    if (ecosystem.clean_energy <= 0.0f && !ecosystem.architect_recruited) {
        ecosystem.ui_phase = UiPhase::Phase1_Minimal;
    }
}

} // namespace

void CalibrationModalState::Reset() noexcept {
    is_open = false;
    target_machine = MachineType::SolarArray;
    slider_value = 0.0f;
    stabilizer_value = 0.68f;
    preview_efficiency = 0.0f;
    preview_risky_output = 0.0f;
    preview_stress = 0.0f;
    can_confirm_clean = false;
    can_confirm_risky = false;
    precision_relic_active = false;
    piezometric_relic_active = false;
    olive_seed_bonus_active = false;
    harmonic_band_revealed = false;
    hard_physics_mode = false;
    selected_hypothesis = HypothesisKind::HarmonicEquilibrium;
    modal_stage = InquiryStage::Observe;
    trials_run = 0;
    last_trial = TrialExperimentReport{};
}

void CalibrationModalState::OpenFor(
    const Machine& machine,
    bool has_precision_relic,
    bool has_piezo_relic,
    bool has_olive_seed_bonus,
    InquiryStage current_stage,
    bool hard_mode) noexcept {
    is_open = true;
    target_machine = machine.type;
    precision_relic_active = has_precision_relic;
    piezometric_relic_active = has_piezo_relic;
    olive_seed_bonus_active = has_olive_seed_bonus;
    hard_physics_mode = hard_mode;
    trials_run = machine.trials_count;
    harmonic_band_revealed =
        !hard_physics_mode &&
        ((precision_relic_active && trials_run >= 1) || (trials_run >= 2) ||
         (machine.approach == CalibrationApproach::CleanSlow));
    modal_stage = current_stage;
    if (static_cast<int>(modal_stage) < static_cast<int>(InquiryStage::Hypothesize)) {
        modal_stage = InquiryStage::Hypothesize;
    }
    if (machine.approach == CalibrationApproach::RiskyEfficient) {
        selected_hypothesis = HypothesisKind::CriticalOverdrive;
    } else if (machine.approach == CalibrationApproach::DirtyImmediate) {
        selected_hypothesis = HypothesisKind::EmergencyBypass;
    } else {
        selected_hypothesis = HypothesisKind::HarmonicEquilibrium;
    }
    stabilizer_value = Clamp(machine.stabilizer_value, 0.0f, 1.0f);
    SetParameter(machine.current_param, machine);
}

void CalibrationModalState::RecomputePreviews(const Machine& machine) noexcept {
    preview_efficiency = machine.EvaluateEfficiency(slider_value);
    preview_risky_output = machine.EvaluateRiskyOutput(slider_value, stabilizer_value);
    preview_stress = machine.EvaluateOperationalStress(slider_value, stabilizer_value);

    const float base_thresh = machine.Threshold();
    // Dawn_FirstMeasureStele widens the clean tolerance band by +30%
    const float effective_thresh = precision_relic_active
                                       ? (1.0f - (1.0f - base_thresh) * 1.30f)
                                       : base_thresh;
    const bool stab_ok = machine.IsInHarmonicStabilizerBand(stabilizer_value, precision_relic_active);
    can_confirm_clean = (preview_efficiency >= effective_thresh) && stab_ok;
    can_confirm_risky = machine.IsInCriticalOverdriveBand(slider_value, stabilizer_value);
}

void CalibrationModalState::SetParameter(float value, const Machine& machine) noexcept {
    float safe_val = std::isfinite(value) ? value : (std::isfinite(slider_value) ? slider_value : 0.0f);
    safe_val = Clamp(safe_val, machine.min_param, machine.max_param);
    slider_value = safe_val;
    RecomputePreviews(machine);
}

void CalibrationModalState::AdjustParameter(float delta, const Machine& machine) noexcept {
    if (!is_open || !std::isfinite(delta) || delta == 0.0f) {
        return;
    }
    float scaled = delta;
    if (precision_relic_active && std::fabs(delta) <= machine.step_param * 1.25f) {
        scaled = delta * 0.25f;
    }
    SetParameter(slider_value + scaled, machine);
}

void CalibrationModalState::SetStabilizer(float value, const Machine& machine) noexcept {
    const float safe_val = std::isfinite(value) ? Clamp(value, 0.0f, 1.0f) : stabilizer_value;
    stabilizer_value = safe_val;
    RecomputePreviews(machine);
}

void CalibrationModalState::AdjustStabilizer(float delta, const Machine& machine) noexcept {
    if (!is_open || !std::isfinite(delta) || delta == 0.0f) {
        return;
    }
    const float scaled = (precision_relic_active && std::fabs(delta) <= 0.08f) ? (delta * 0.25f) : delta;
    SetStabilizer(stabilizer_value + scaled, machine);
}

void CalibrationModalState::SelectHypothesis(HypothesisKind kind) noexcept {
    selected_hypothesis = kind;
    if (static_cast<int>(modal_stage) < static_cast<int>(InquiryStage::Hypothesize)) {
        modal_stage = InquiryStage::Hypothesize;
    }
}

void CalibrationModalState::CycleHypothesis(int delta) noexcept {
    if (delta == 0) {
        return;
    }
    const int cur = static_cast<int>(selected_hypothesis);
    const int next = ((cur + delta) % 3 + 3) % 3;
    SelectHypothesis(static_cast<HypothesisKind>(next));
}

TrialExperimentReport CalibrationModalState::RunTrial(
    Machine& machine,
    EcoSystem& ecosystem) noexcept {
    TrialExperimentReport rep{};
    if (!is_open) {
        return rep;
    }
    RecomputePreviews(machine);
    ++trials_run;
    ++machine.trials_count;

    rep.executed = true;
    rep.machine_type = machine.type;
    rep.tested_hypothesis = selected_hypothesis;
    rep.tested_param = slider_value;
    rep.tested_stabilizer = stabilizer_value;
    rep.measured_efficiency = preview_efficiency;
    rep.thermal_or_hydraulic_stress = preview_stress;
    rep.harmonic_band_hit = can_confirm_clean;
    rep.critical_band_hit = can_confirm_risky;

    const float diff = slider_value - machine.target_param;
    const float deadband = std::max(machine.step_param * 1.5f, 0.35f);
    if (diff < -deadband) {
        rep.error_direction = -1;
    } else if (diff > deadband) {
        rep.error_direction = 1;
    } else {
        rep.error_direction = 0;
    }

    if (!hard_physics_mode &&
        ((precision_relic_active && trials_run >= 1) || trials_run >= 2)) {
        harmonic_band_revealed = true;
    }

    if (!rep.harmonic_band_hit) {
        const float stress_poll = rep.critical_band_hit ? 1.0f : 1.5f;
        ecosystem.ApplyPollutionDelta(stress_poll);
        rep.pollution_delta_applied = stress_poll;
    }

    if (machine.type == MachineType::SolarArray) {
        if (rep.harmonic_band_hit) {
            rep.empirical_reading =
                "Fascio in fase col coseno di Lambert: ombra nulla sullo gnomone e smorzamento termico stabile.";
            rep.world_consequence =
                "Lampo dorato nitido: le incisioni solari sulle Terrazze brillano senza scorie.";
        } else if (rep.critical_band_hit) {
            rep.empirical_reading =
                "Concentrazione focale supercritica rilevata: resa altissima ma i condotti d'ottone vibrano per il calore.";
            rep.world_consequence =
                "Vampa focalizzata: i rovi della Cresta Nord fumano e le incisioni nascoste affiorano (+1.0 stress).";
        } else if (rep.error_direction < 0) {
            rep.empirical_reading =
                "Angolo inferiore all'inclinazione solare: il raggio scivola basso sotto il ricevitore.";
            rep.world_consequence =
                "Riverbero disperso sul travertino: le incisioni svelano l'ombra residua (+1.5 smog di sfasamento).";
        } else {
            rep.empirical_reading =
                "Angolo troppo elevato o stabilizzatore fuori finestra: riflessione obliqua dissipata in calore.";
            rep.world_consequence =
                "Bagliore obliquo: bruciacchia i rovi secchi vicino al portico (+1.5 smog di sfasamento).";
        }
    } else if (machine.type == MachineType::HydroTurbine) {
        if (rep.harmonic_band_hit) {
            rep.empirical_reading =
                "Battente idrostatico in equilibrio di Stevino (~98 kPa): girante silenziosa e zero cavitazione.";
            rep.world_consequence =
                "Onda limpida nel bacino: la colonna piezometrica si stabilizza senza scuotere le volte.";
        } else if (rep.critical_band_hit) {
            rep.empirical_reading =
                "Battente supercritico (12.5..15.0 m) a valvola aperta: spinta idraulica massima con cavitazione incipiente!";
            rep.world_consequence =
                "Colpo d'ariete ad alta pressione: scuote le paratoie e forza temporaneamente lo Sfioratore Alto!";
        } else if (rep.error_direction < 0) {
            rep.empirical_reading =
                "Battente insufficiente: la pressione idrostatica non vince l'inerzia delle pale di bronzo.";
            rep.world_consequence =
                "Reflusso torbido nei canali di scolo delle Cisterne (+1.5 stress idraulico).";
        } else {
            rep.empirical_reading =
                "Sovrappressione fuori fase: vortici turbolenti contro la paratoia senza accoppiamento armonico.";
            rep.world_consequence =
                "Colpo d'ariete scomposto: spruzzi in pressione lungo i condotti (+1.5 stress idraulico).";
        }
    } else {
        if (rep.harmonic_band_hit) {
            rep.empirical_reading =
                "Risonanza pulita verificata: parametro principale e stabilizzatore sono in pieno accordo.";
            rep.world_consequence = "Impulso armonico stabile: nessun residuo rilasciato nell'aria.";
        } else if (rep.critical_band_hit) {
            rep.empirical_reading =
                "Regime supercritico attivo: massima potenza estratta con forte sollecitazione meccanica.";
            rep.world_consequence = "Vibrazione ad alta energia lungo la struttura (+1.0 stress).";
        } else {
            rep.empirical_reading = (rep.error_direction < 0)
                ? "Misura sotto la soglia di risonanza: incrementa il parametro e riprova."
                : "Misura oltre la soglia o smorzatore fuori banda: riduci lo scarto.";
            rep.world_consequence = "Impulso fuori fase dissipato in attrito (+1.5 stress ambientale).";
        }
    }

    modal_stage = InquiryStage::ConsequenceSeen;
    last_trial = rep;
    return rep;
}

bool CalibrationModalState::ConfirmClean(Machine& machine, EcoSystem& ecosystem) noexcept {
    if (!is_open) {
        return false;
    }
    RecomputePreviews(machine);
    if (!can_confirm_clean) {
        return false;
    }

    const bool already_same_clean =
        (machine.state == MachineState::CleanActive &&
         machine.approach == CalibrationApproach::CleanSlow);

    machine.current_param = slider_value;
    machine.stabilizer_value = stabilizer_value;
    machine.state = MachineState::CleanActive;
    machine.approach = CalibrationApproach::CleanSlow;
    modal_stage = InquiryStage::Resolved;
    is_open = false;

    if (!already_same_clean) {
        UndoPreviousMachineContribution(machine, ecosystem);
        const float energy = machine.clean_energy_reward;
        const float poll_red =
            olive_seed_bonus_active
                ? std::max(machine.clean_pollution_reduction, 22.0f)
                : machine.clean_pollution_reduction;
        const float clean_before = ecosystem.clean_energy;
        const float poll_before = ecosystem.pollution_level;
        ecosystem.ApplyCleanActivation(energy, poll_red);
        machine.applied_clean_energy = ecosystem.clean_energy - clean_before;
        machine.applied_pollution_delta = ecosystem.pollution_level - poll_before;
    }
    return true;
}

bool CalibrationModalState::ConfirmRisky(Machine& machine, EcoSystem& ecosystem) noexcept {
    if (!is_open) {
        return false;
    }
    RecomputePreviews(machine);
    if (!can_confirm_risky) {
        return false;
    }

    const bool already_same_risky =
        (machine.state == MachineState::CleanActive &&
         machine.approach == CalibrationApproach::RiskyEfficient);

    machine.current_param = slider_value;
    machine.stabilizer_value = stabilizer_value;
    machine.state = MachineState::CleanActive;
    machine.approach = CalibrationApproach::RiskyEfficient;
    modal_stage = InquiryStage::Resolved;
    is_open = false;

    if (!already_same_risky) {
        UndoPreviousMachineContribution(machine, ecosystem);
        const float clean_before = ecosystem.clean_energy;
        const float poll_before = ecosystem.pollution_level;
        ecosystem.ApplyRiskyActivation(machine.risky_energy_reward, machine.risky_pollution_increase);
        machine.applied_clean_energy = ecosystem.clean_energy - clean_before;
        machine.applied_pollution_delta = ecosystem.pollution_level - poll_before;
    }
    return true;
}

void CalibrationModalState::TriggerBypass(Machine& machine, EcoSystem& ecosystem) noexcept {
    if (!is_open || !machine.allow_bypass) {
        return;
    }
    const bool already_same_dirty =
        (machine.state == MachineState::BypassedDirty &&
         machine.approach == CalibrationApproach::DirtyImmediate);

    machine.state = MachineState::BypassedDirty;
    machine.approach = CalibrationApproach::DirtyImmediate;
    modal_stage = InquiryStage::Resolved;
    is_open = false;

    if (!already_same_dirty) {
        UndoPreviousMachineContribution(machine, ecosystem);
        const float dirty_before = ecosystem.dirty_energy;
        const float poll_before = ecosystem.pollution_level;
        ecosystem.ApplyDirtyBypass(
            machine.dirty_energy_reward,
            machine.dirty_pollution_increase);
        machine.applied_dirty_energy = ecosystem.dirty_energy - dirty_before;
        machine.applied_pollution_delta = ecosystem.pollution_level - poll_before;
    }
}

void CalibrationModalState::Cancel() noexcept {
    if (!is_open) {
        return;
    }
    is_open = false;
}

EmpiricalScanReading PerformEmpiricalScan(
    int player_id,
    const PlayerManager& players,
    const Machine& scene_machine,
    float max_range) noexcept {
    EmpiricalScanReading out{};
    out.active = false;
    out.has_target = false;
    out.target_type = scene_machine.type;
    out.measured_target_value = 0.0f;
    out.measured_distance = 0.0f;
    out.measured_efficiency = 0.0f;

    const Player* p = players.GetPlayer(player_id);
    if (p == nullptr || !p->active) {
        return out;
    }

    out.active = true;
    const float safe_range = (std::isfinite(max_range) && max_range > 0.0f)
                                 ? max_range
                                 : kDefaultScannerRange;
    const float dist = scene_machine.DistanceToPlayer(*p);
    if (!std::isfinite(dist) || dist > safe_range) {
        return out;
    }

    out.has_target = true;
    out.target_type = scene_machine.type;
    out.measured_target_value = scene_machine.target_param;
    out.measured_distance = dist;
    out.measured_efficiency = scene_machine.LiveEfficiency();
    return out;
}

} // namespace eco
