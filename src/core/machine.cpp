#include "core/machine.h"

#include <algorithm>
#include <cmath>

namespace eco {

float ComputeSolarEfficiency(float panel_angle_deg, float sun_angle_deg) noexcept {
    if (!std::isfinite(panel_angle_deg) || !std::isfinite(sun_angle_deg)) {
        return 0.0f;
    }
    // Wrap angle difference into [-180, 180] so large or 360-periodic angles preserve accuracy
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

Machine Machine::CreateDefaultSolar(Vec2 pos) noexcept {
    Machine m;
    m.type = MachineType::SolarArray;
    m.state = MachineState::Broken;
    m.pos = pos;
    m.trigger_radius = kDefaultMachineTriggerRadius;
    m.current_param = 0.0f;
    m.target_param = 45.0f;
    m.tolerance_pa = kDefaultHydroTolerancePa;
    m.clean_energy_reward = 35.0f;
    m.clean_pollution_reduction = 15.0f;
    m.dirty_energy_reward = 35.0f;
    m.dirty_pollution_increase = 25.0f;
    m.prompt_visible = false;
    return m;
}

Machine Machine::CreateDefaultHydro(Vec2 pos) noexcept {
    Machine m;
    m.type = MachineType::HydroTurbine;
    m.state = MachineState::Broken;
    m.pos = pos;
    m.trigger_radius = kDefaultMachineTriggerRadius;
    m.current_param = 7.0f;
    m.target_param = 10.0f;
    m.tolerance_pa = kDefaultHydroTolerancePa;
    m.clean_energy_reward = 35.0f;
    m.clean_pollution_reduction = 15.0f;
    m.dirty_energy_reward = 35.0f;
    m.dirty_pollution_increase = 25.0f;
    m.prompt_visible = false;
    return m;
}

float Machine::EvaluateEfficiency(float param) const noexcept {
    if (type == MachineType::SolarArray) {
        return ComputeSolarEfficiency(param, target_param);
    }
    return ComputeHydroEfficiency(param, target_param, tolerance_pa);
}

float Machine::LiveEfficiency() const noexcept {
    return EvaluateEfficiency(current_param);
}

float Machine::Threshold() const noexcept {
    return (type == MachineType::SolarArray)
               ? kSolarEfficiencyThreshold
               : kHydroEfficiencyThreshold;
}

float Machine::DistanceToPlayer(const Player& player) const noexcept {
    if (!std::isfinite(player.pos.x) || !std::isfinite(player.pos.y) ||
        !std::isfinite(pos.x) || !std::isfinite(pos.y)) {
        return 1e9f;
    }
    return player.pos.DistanceTo(pos);
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
    if (machine.type == MachineType::SolarArray) {
        safe_val = Clamp(safe_val, -360.0f, 360.0f);
    } else {
        safe_val = Clamp(safe_val, -50.0f, 100.0f);
    }
    slider_value = safe_val;
    preview_efficiency = machine.EvaluateEfficiency(slider_value);
    can_confirm_clean = (preview_efficiency > machine.Threshold());
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
    can_confirm_clean = (preview_efficiency > machine.Threshold());
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
    if (!is_open) {
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
