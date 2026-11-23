#include "core/npc.h"

#include <cmath>

namespace eco {

namespace {

[[nodiscard]] constexpr Vec2 TileCenterPos(int row, int col) noexcept {
    return {
        (static_cast<float>(col) + 0.5f) * kTileSize,
        (static_cast<float>(row) + 0.5f) * kTileSize
    };
}

} // namespace

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

WorldNpc WorldNpc::CreateDefault(NpcId id) {
    WorldNpc npc{};
    npc.id = id;
    npc.name = NpcDisplayName(id);
    npc.role = NpcRoleTitle(id);
    npc.trigger_radius = kDefaultNpcTriggerRadius;

    switch (id) {
        case NpcId::ClaraSereni:
            npc.location = LocationId::Overworld;
            npc.zone = ZoneId::CanalQuarter;
            npc.pos = TileCenterPos(12, 29); // (1416, 600) — Ponte degli Archi (mappa 26x36)
            npc.last_dialogue =
                "Prima di aprire il Ponte degli Archi ho bisogno di una prova pulita. Intanto usa le leve delle chiuse [E] per domare le correnti nei canali.";
            break;
        case NpcId::LorenzoAltieri:
            npc.location = LocationId::HypogeanCisterns;
            npc.zone = ZoneId::CanalQuarter;
            npc.pos = TileCenterPos(7, 28); // (1368, 360) — Galleria alta Cisterne Ipogee
            npc.trigger_radius = 96.0f;
            npc.last_dialogue =
                "Qui in alto custodisco le memorie delle quattro ere. Riattiva la Turbina idrostatica e ti apriro' gli appunti antichi.";
            break;
        case NpcId::MiriamFerro:
            npc.location = LocationId::Overworld;
            npc.zone = ZoneId::HangingGardens;
            npc.pos = TileCenterPos(15, 11); // (552, 744) — Giardini Sospesi (mappa 26x36)
            npc.last_dialogue =
                "Attiva le quattro piastre di giada in sequenza armonica per stendere il ponte verso il Risonatore, poi accordalo a 120 Hertz.";
            break;
        case NpcId::MatteoVerrini:
            npc.location = LocationId::Overworld;
            npc.zone = ZoneId::FumaroleCaldera;
            npc.pos = TileCenterPos(17, 12); // (600, 840) — Caldera delle Fumarole (mappa 26x36)
            npc.last_dialogue =
                "Chiudi pure le valvole di sfogo [E] per passare tra i geyser, ma se vuoi spegnere le nostre fornaci dimostra che Carnot basta da solo.";
            break;
        case NpcId::Count:
            break;
    }
    return npc;
}

bool WorldNpc::IsPlayerInRange(const Player& player) const noexcept {
    if (!player.active || !std::isfinite(player.pos.x) || !std::isfinite(player.pos.y) ||
        !std::isfinite(pos.x) || !std::isfinite(pos.y) ||
        !std::isfinite(trigger_radius) || trigger_radius <= 0.0f) {
        return false;
    }
    return player.Center().DistanceTo(pos) <= trigger_radius;
}

bool WorldNpc::AnyActivePlayerInRange(const PlayerManager& players) const noexcept {
    for (const Player* p : players.ActivePlayers()) {
        if (p != nullptr && IsPlayerInRange(*p)) {
            return true;
        }
    }
    return false;
}

void WorldNpc::UpdateProximity(const PlayerManager& players) noexcept {
    prompt_visible = AnyActivePlayerInRange(players);
}

void ArchitectNpc::Reset(Vec2 default_pos) {
    pos = default_pos;
    trigger_radius = kDefaultNpcTriggerRadius;
    recruited = false;
    hidden_bridge_revealed = false;
    last_dialogue =
        "Clara Sereni: Dimostrami una calibrazione fisica pulita prima di aprire il Ponte degli Archi.";
}

bool ArchitectNpc::IsPlayerInRange(const Player& player) const noexcept {
    if (!player.active || !std::isfinite(player.pos.x) || !std::isfinite(player.pos.y) ||
        !std::isfinite(pos.x) || !std::isfinite(pos.y) ||
        !std::isfinite(trigger_radius) || trigger_radius <= 0.0f) {
        return false;
    }
    return player.Center().DistanceTo(pos) <= trigger_radius;
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
            "Clara Sereni: Il Ponte degli Archi e' aperto e la Trama Geodetica e' attiva lungo tutta la Conca.";
        return true;
    }

    if (ecosystem.clean_energy > 0.0f) {
        recruited = true;
        hidden_bridge_revealed = true;
        ecosystem.SetArchitectRecruited(true);
        last_dialogue =
            "Clara Sereni: Misura verificata senza fumo. Apro il Ponte degli Archi e ti affido la Trama Geodetica.";
        return true;
    }

    if (ecosystem.dirty_energy > 0.0f) {
        last_dialogue =
            "Clara Sereni: Non apro il ponte per energia ottenuta bruciando pece. Porta una misura pulita.";
    } else {
        last_dialogue =
            "Clara Sereni: Nessun monumento produce ancora energia pulita. Calibra l'Eliostato sulle Terrazze dell'Alba.";
    }
    return false;
}

} // namespace eco
