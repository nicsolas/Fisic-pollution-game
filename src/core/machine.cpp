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
    m.clean_energy_reward = 20.0f;
    m.clean_pollution_reduction = 10.0f;
    m.dirty_energy_reward = 20.0f;
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
    m.clean_energy_reward = 20.0f;
    m.clean_pollution_reduction = 12.0f;
    m.dirty_energy_reward = 20.0f;
    m.dirty_pollution_increase = 24.0f;
    m.unlocked = true;
    m.allow_bypass = true;
    m.prompt_visible = false;
    return m;
}

Machine Machine::CreateDefaultPendulum(Vec2 pos) noexcept {
    Machine m{};
    m.type = MachineType::PendulumRegulator;
    m.state = MachineState::Broken;
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
    m.clean_energy_reward = 18.0f;
    m.clean_pollution_reduction = 10.0f;
    m.dirty_energy_reward = 18.0f;
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
    m.clean_energy_reward = 20.0f;
    m.clean_pollution_reduction = 14.0f;
    m.dirty_energy_reward = 20.0f;
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
    m.clean_energy_reward = 22.0f;
    m.clean_pollution_reduction = 14.0f;
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
    m.clean_energy_reward = 25.0f;
    m.clean_pollution_reduction = 35.0f;
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

void CalibrationModalState::Reset() noexcept {
    is_open = false;
    target_machine = MachineType::SolarArray;
    slider_value = 0.0f;
    preview_efficiency = 0.0f;
    can_confirm_clean = false;
}

void CalibrationModalState::OpenFor(const Machine& machine) noexcept {
    is_open = true;
    target_machine = machine.type;
    SetParameter(machine.current_param, machine);
}

void CalibrationModalState::SetParameter(float value, const Machine& machine) noexcept {
    float safe_val = std::isfinite(value) ? value : (std::isfinite(slider_value) ? slider_value : 0.0f);
    safe_val = Clamp(safe_val, machine.min_param, machine.max_param);
    slider_value = safe_val;
    preview_efficiency = machine.EvaluateEfficiency(slider_value);
    can_confirm_clean = (preview_efficiency >= machine.Threshold());
}

void CalibrationModalState::AdjustParameter(float delta, const Machine& machine) noexcept {
    if (!is_open || !std::isfinite(delta) || delta == 0.0f) {
        return;
    }
    SetParameter(slider_value + delta, machine);
}

bool CalibrationModalState::ConfirmClean(Machine& machine, EcoSystem& ecosystem) noexcept {
    if (!is_open) {
        return false;
    }
    preview_efficiency = machine.EvaluateEfficiency(slider_value);
    can_confirm_clean = (preview_efficiency >= machine.Threshold());
    if (!can_confirm_clean) {
        return false;
    }

    const bool was_clean = (machine.state == MachineState::CleanActive);
    machine.current_param = slider_value;
    machine.state = MachineState::CleanActive;
    is_open = false;

    if (!was_clean) {
        ecosystem.ApplyCleanActivation(
            machine.clean_energy_reward,
            machine.clean_pollution_reduction);
    }
    return true;
}

void CalibrationModalState::TriggerBypass(Machine& machine, EcoSystem& ecosystem) noexcept {
    if (!is_open || !machine.allow_bypass) {
        return;
    }
    const bool already_activated = (machine.state != MachineState::Broken);
    if (machine.state != MachineState::CleanActive) {
        machine.state = MachineState::BypassedDirty;
    }
    is_open = false;

    if (!already_activated) {
        ecosystem.ApplyDirtyBypass(
            machine.dirty_energy_reward,
            machine.dirty_pollution_increase);
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
