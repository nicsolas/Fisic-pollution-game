#include "core/npc.h"

#include <cmath>

namespace eco {

OverlayVisibility ComputeOverlayVisibility(const EcoSystem& ecosystem) noexcept {
    OverlayVisibility out{};
    out.phase = ecosystem.ui_phase;
    const bool at_least_phase2 =
        (static_cast<int>(ecosystem.ui_phase) >= static_cast<int>(UiPhase::Phase2_Telemetry));
    const bool is_phase3 = (ecosystem.ui_phase == UiPhase::Phase3_EngineOverlay);

    out.show_raw_numbers = at_least_phase2;
    out.show_velocity_vectors = is_phase3;
    out.show_aabb_hitboxes = is_phase3;
    out.show_irradiance_rays = is_phase3;
    out.show_trigger_radii = is_phase3;
    return out;
}

bool TryRequestOverlayPhase(EcoSystem& ecosystem, UiPhase requested) noexcept {
    const int req = static_cast<int>(requested);
    if (req < static_cast<int>(UiPhase::Phase1_Minimal) ||
        req > static_cast<int>(UiPhase::Phase3_EngineOverlay)) {
        return false;
    }
    const int max_unlocked = static_cast<int>(ecosystem.MaxUnlockedUiPhase());
    if (req > max_unlocked) {
        return false;
    }
    ecosystem.ui_phase = requested;
    return true;
}

void ArchitectNpc::Reset(Vec2 default_pos) {
    pos = default_pos;
    trigger_radius = kDefaultNpcTriggerRadius;
    recruited = false;
    hidden_bridge_revealed = false;
    last_dialogue = "Architetto: Dimostrami una calibrazione matematica pulita (Energia Pulita > 0).";
}

bool ArchitectNpc::IsPlayerInRange(const Player& player) const noexcept {
    if (!player.active || !std::isfinite(player.pos.x) || !std::isfinite(player.pos.y) ||
        !std::isfinite(pos.x) || !std::isfinite(pos.y) ||
        !std::isfinite(trigger_radius) || trigger_radius <= 0.0f) {
        return false;
    }
    return player.pos.DistanceTo(pos) <= trigger_radius;
}

bool ArchitectNpc::AnyActivePlayerInRange(const PlayerManager& players) const noexcept {
    for (const Player* p : players.ActivePlayers()) {
        if (p != nullptr && IsPlayerInRange(*p)) {
            return true;
        }
    }
    return false;
}

bool ArchitectNpc::TryInteract(
    int player_id,
    const PlayerManager& players,
    EcoSystem& ecosystem) {
    const Player* p = players.GetPlayer(player_id);
    if (p == nullptr || !IsPlayerInRange(*p)) {
        return recruited;
    }

    if (recruited || ecosystem.architect_recruited) {
        recruited = true;
        hidden_bridge_revealed = true;
        ecosystem.SetArchitectRecruited(true);
        last_dialogue =
            "Architetto: Ponte nascosto e Overlay Fase 3 attivi! I vettori della matrice sono visibili.";
        return true;
    }

    if (ecosystem.clean_energy > 0.0f) {
        recruited = true;
        hidden_bridge_revealed = true;
        ecosystem.SetArchitectRecruited(true);
        last_dialogue =
            "Architetto: Calibrazione matematica verificata! Mi unisco a voi: Ponte sbloccato e Fase 3 attiva.";
        return true;
    }

    if (ecosystem.dirty_energy > 0.0f) {
        last_dialogue =
            "Architetto: Rifiuto il bypass a combustione! Serve energia pulita ottenuta con la fisica.";
    } else {
        last_dialogue =
            "Architetto: L'impianto non produce ancora energia pulita. Calibra un macchinario prima!";
    }
    return false;
}

} // namespace eco
