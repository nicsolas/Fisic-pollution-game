#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "core/collectible.h"
#include "core/ecosystem.h"
#include "core/input_types.h"
#include "core/machine.h"
#include "core/math_types.h"
#include "core/npc.h"
#include "core/player.h"
#include "core/story.h"
#include "core/tilemap.h"

namespace eco {

inline constexpr float kPortalCooldownSeconds = 0.5f;
inline constexpr float kZoneBannerDuration = 3.2f;
inline constexpr float kZoneInterludeDuration = 2.6f;
inline constexpr float kZoneInterludeMinSkip = 0.35f;

struct PortalLink {
    StructureId target_structure = StructureId::HypogeanCisterns;
    ZoneId source_zone = ZoneId::CanalQuarter;
    TileCoord tile_min{0, 0};
    TileCoord tile_max{0, 0};
    Vec2 world_center{0.0f, 0.0f};
    Vec2 exit_spawn{0.0f, 0.0f};
};

struct ObjectiveTarget {
    bool valid = false;
    LocationId location = LocationId::Overworld;
    ZoneId zone = ZoneId::DawnTerraces;
    Vec2 world_pos{0.0f, 0.0f};
    const char* label = "";
    bool is_portal_step = false;
};

struct MemoryMetrics {
    std::size_t active_zone_tilemap_bytes = 0;
    std::size_t active_structure_tilemap_bytes = 0;
    std::size_t total_resident_map_bytes = 0;
    std::size_t saved_unloaded_bytes = 0;
    int zone_swaps_count = 0;
};

// ============================================================================
// 6 Meccaniche Top-Down Uniche per Zona
// ============================================================================

// 1. Terrazze dell'Alba: Specchi Solari, Fasci e Piastre Fotosensibili
struct SolarMirror {
    int id = 0;
    Vec2 pos{0.0f, 0.0f};
    int dir_index = 0; // 0=Est, 1=Sud, 2=Ovest, 3=Nord
    bool rotatable = true;
    float interact_radius = 64.0f;
};

struct SolarBeamSegment {
    Vec2 start{0.0f, 0.0f};
    Vec2 end{0.0f, 0.0f};
    bool active = true;
};

struct SolarPlate {
    int id = 0;
    Vec2 pos{0.0f, 0.0f};
    float radius = 28.0f;
    float charge = 0.0f; // 0..1
    bool lit = false;
    const char* inscription_hint = "";
};

// 2. Quartiere dei Canali: Correnti d'Acqua & Chiuse/Leve Idrauliche
struct WaterCurrentZone {
    Rect bounds{};
    Vec2 flow_dir{1.0f, 0.0f};
    float strength = 155.0f;
    int sluice_group = 0;
    bool active = true;
};

struct SluiceLever {
    int id = 0;
    Vec2 pos{0.0f, 0.0f};
    int sluice_group = 0;
    bool pulled = false;
    float interact_radius = 64.0f;
};

// 3. Altopiano dei Venti: Raffiche Periodiche & Monoliti Frangivento
struct WindState {
    float cycle_timer = 0.0f;
    float cycle_period = 6.0f;
    float gust_intensity = 0.0f; // 0..1
    Vec2 wind_dir{-1.0f, 0.15f};
    float max_push_speed = 185.0f;
    bool telegraphing = false;
    bool gust_active = false;
    bool player_sheltered = false;
};

struct WindbreakMonolith {
    Rect bounds{};
    Vec2 center{0.0f, 0.0f};
    float shelter_distance = 132.0f;
};

// 4. Giardini Sospesi: Piastre di Risonanza Armonica & Ponti di Giada
struct ResonancePad {
    int id = 0;
    Vec2 pos{0.0f, 0.0f};
    float radius = 32.0f;
    int sequence_order = 0; // 0..3
    float pitch_hz = 261.63f;
    bool activated = false;
    float pulse_timer = 0.0f;
    bool player_was_inside = false;
};

// 5. Caldera delle Fumarole: Fumarole/Geyser Termici Ciclici & Valvole di Sfogo
enum class GeyserPhase : int {
    Dormant = 0,
    Telegraphing = 1,
    Erupting = 2,
    Calmed = 3
};

struct ThermalGeyser {
    int id = 0;
    Vec2 pos{0.0f, 0.0f};
    float radius = 40.0f;
    float cycle_offset = 0.0f;
    float cycle_period = 4.6f;
    float phase_timer = 0.0f;
    float intensity = 0.0f; // 0..1
    GeyserPhase phase = GeyserPhase::Dormant;
    int valve_group = 0;
};

struct ReliefValve {
    int id = 0;
    Vec2 pos{0.0f, 0.0f};
    int valve_group = 0;
    bool open = false;
    float interact_radius = 64.0f;
};

// 6. Santuario dell'Armonia: Sigilli Elementali attorno all'Astrolabio Centrale
struct SanctuarySeal {
    int id = 0;
    ZoneId element_zone = ZoneId::DawnTerraces;
    Vec2 pos{0.0f, 0.0f};
    float radius = 38.0f;
    bool attuned = false;
    float glow = 0.0f; // 0..1
};

struct MechanicEventFlags {
    bool mirror_rotated = false;
    bool solar_gate_unlocked = false;
    bool sluice_toggled = false;
    bool wind_gust_started = false;
    bool resonance_pad_hit = false;
    int resonance_pad_index = -1;
    bool resonance_failed = false;
    bool resonance_solved = false;
    bool geyser_erupted = false;
    bool geyser_pushed_player = false;
    bool valve_opened = false;
    bool sanctuary_seal_attuned = false;
};

struct ZoneMechanicState {
    // 1. Terrazze dell'Alba
    Vec2 solar_emitter_pos{0.0f, 0.0f};
    Vec2 solar_emitter_dir{1.0f, 0.0f};
    std::vector<SolarMirror> solar_mirrors{};
    std::vector<SolarBeamSegment> solar_beams{};
    std::vector<SolarPlate> solar_plates{};
    bool solar_gate_open = false;

    // 2. Quartiere dei Canali
    std::vector<WaterCurrentZone> water_currents{};
    std::vector<SluiceLever> sluice_levers{};

    // 3. Altopiano dei Venti
    WindState wind{};
    std::vector<WindbreakMonolith> windbreaks{};

    // 4. Giardini Sospesi
    std::vector<ResonancePad> resonance_pads{};
    int resonance_next_order = 0;
    bool jade_bridge_unlocked = false;
    float jade_bridge_progress = 0.0f; // 0..1

    // 5. Caldera delle Fumarole
    std::vector<ThermalGeyser> geysers{};
    std::vector<ReliefValve> relief_valves{};

    // 6. Santuario dell'Armonia
    std::vector<SanctuarySeal> sanctuary_seals{};

    void ResetDefaults();
    void RecomputeSolarBeams(const Tilemap& dawn_map) noexcept;
    [[nodiscard]] int AttunedSealsCount() const noexcept;
    [[nodiscard]] bool AllSealsAttuned() const noexcept;
    [[nodiscard]] bool IsWindSheltered(Vec2 point) const noexcept;
};

// ============================================================================
// Intramezzo tra Zone (Transizione & Swap Pulito di Memoria)
// ============================================================================

struct ZoneInterludeState {
    bool active = false;
    ZoneId from_zone = ZoneId::DawnTerraces;
    ZoneId to_zone = ZoneId::CanalQuarter;
    TileCoord target_spawn_tile{13, 18};
    float elapsed = 0.0f;
    float duration = kZoneInterludeDuration;
    float min_skip_time = kZoneInterludeMinSkip;
    bool swap_committed = false;
    bool instant_mode = false;

    [[nodiscard]] float Progress() const noexcept {
        if (!active || duration <= 0.0f) {
            return 0.0f;
        }
        return Clamp(elapsed / duration, 0.0f, 1.0f);
    }

    [[nodiscard]] bool CanSkip() const noexcept {
        return active && (instant_mode || elapsed >= min_skip_time);
    }
};

struct OverworldStepResult {
    bool enter_structure_requested = false;
    StructureId target_structure = StructureId::HypogeanCisterns;
    bool zone_gateway_triggered = false;
    ZoneId gateway_target_zone = ZoneId::DawnTerraces;
    TileCoord gateway_arrival_tile{13, 18};
    int newly_joined_count = 0;
};

struct StructureStepResult {
    bool exit_to_overworld_requested = false;
    int newly_joined_count = 0;
};

struct WorldStepResult {
    bool enter_structure_requested = false;
    StructureId target_structure = StructureId::HypogeanCisterns;
    bool exit_to_overworld_requested = false;
    int newly_joined_count = 0;
    bool zone_changed = false;
    ZoneId new_zone = ZoneId::DawnTerraces;
    bool interlude_started = false;
    bool interlude_finished = false;
    bool collectible_picked_up = false;
    CollectibleId picked_up_id = CollectibleId::Dawn_BrassGnomon;
    MechanicEventFlags mechanics_events{};
};

struct OverworldSim {
    ZoneId active_zone = ZoneId::DawnTerraces;
    Tilemap tilemap{};
    std::vector<PortalLink> portals{};
    std::vector<ZoneGateway> active_gateways{};
    std::array<DistrictGateway, Tilemap::kOverworldGatewayCount> gateways{};
    StructureId last_used_structure = StructureId::HypogeanCisterns;
    Vec2 spawn_point{0.0f, 0.0f};
    Vec2 portal_exit_spawn{0.0f, 0.0f};
    Vec2 camera_target{0.0f, 0.0f};
    float portal_cooldown_timer = 0.0f;
    bool portal_reentry_locked = false;
    float gateway_cooldown_timer = 0.0f;
    bool gateway_reentry_locked = false;
    int zone_swaps_count = 0;
    bool initialized = false;

    void InitializeDefault();
    void LoadZone(ZoneId zone, std::optional<TileCoord> arrival_tile = std::nullopt);
    void UnloadZoneMemory() noexcept;

    void OnEnter(
        PlayerManager& players,
        bool returning_from_structure,
        bool bridge_unlocked = false,
        StructureId from_structure = StructureId::HypogeanCisterns);

    [[nodiscard]] ZoneId DistrictAt(Vec2 world_pos) const noexcept;
    [[nodiscard]] ZoneId DistrictAtTile(int row, int col) const noexcept;
    [[nodiscard]] Rect DistrictBounds(ZoneId zone) const noexcept;
    [[nodiscard]] const PortalLink* FindPortal(StructureId id) const noexcept;
    [[nodiscard]] static ZoneId PortalZone(StructureId id) noexcept;
    [[nodiscard]] Vec2 SafeSpawnInZone(ZoneId zone, bool bridge_unlocked = false) const noexcept;
    [[nodiscard]] Vec2 SafeSpawnAtTile(
        TileCoord tile,
        bool bridge_unlocked = false,
        bool solar_gate_open = false,
        bool jade_bridge_unlocked = false) const noexcept;

    [[nodiscard]] OverworldStepResult Step(
        PlayerManager& players,
        const AllPlayersInput& input,
        float dt,
        bool bridge_unlocked = false,
        bool solar_gate_open = false,
        bool jade_bridge_unlocked = false,
        Vec2 external_velocity = {0.0f, 0.0f});
};

struct StructureSim {
    StructureId active_structure = StructureId::HypogeanCisterns;
    Tilemap tilemap{};
    Vec2 spawn_point{0.0f, 0.0f};
    Vec2 camera_target{0.0f, 0.0f};
    float portal_cooldown_timer = 0.0f;
    bool portal_reentry_locked = false;
    bool initialized = false;

    void InitializeDefault();
    void SelectStructure(StructureId id);
    void UnloadMemory() noexcept;
    void OnEnter(PlayerManager& players, bool bridge_unlocked = false);
    void OnEnter(StructureId id, PlayerManager& players, bool bridge_unlocked = false);

    [[nodiscard]] StructureStepResult Step(
        PlayerManager& players,
        const AllPlayersInput& input,
        float dt,
        bool bridge_unlocked = false);
};

struct WorldContext {
    EcoSystem ecosystem;
    PlayerManager players;
    OverworldSim overworld;
    StructureSim structure;
    ZoneMechanicState zone_mechanics{};
    ZoneInterludeState interlude{};
    std::array<Machine, static_cast<std::size_t>(MachineType::Count)> machines{};
    MachineType modal_machine_type = MachineType::SolarArray;
    CalibrationModalState calibration_modal{};
    std::array<WorldNpc, static_cast<std::size_t>(NpcId::Count)> npcs{};
    CollectibleRegistry collectibles{};
    StoryDirector story{};

    ZoneId current_zone = ZoneId::DawnTerraces;
    float zone_banner_timer = 0.0f;
    ZoneId banner_zone = ZoneId::DawnTerraces;
    StructureId banner_structure = StructureId::HypogeanCisterns;
    bool banner_is_structure = false;
    bool instant_zone_transitions = false;

    bool demo_mode = false; // Campagna Completa (La Conca di Meridia) attiva di default
    EmpiricalScanReading last_scan{};
    float sim_time = 0.0f;
    float scan_pulse_timer = 0.0f;
    Vec2 scan_pulse_origin{0.0f, 0.0f};
    bool simulation_completed = false;
    GameMode current_mode = GameMode::Overworld;
    StructureId active_structure = StructureId::HypogeanCisterns;
    bool returning_from_structure = false;
    bool loaded_from_save = false;

    void Reset();
    void EnsureInitialized();

    [[nodiscard]] MemoryMetrics GetMemoryMetrics() const noexcept;

    [[nodiscard]] LocationId ActiveLocation() const noexcept;
    [[nodiscard]] const char* ActiveLocationName() const noexcept;
    [[nodiscard]] const char* ActiveLocationSubtitle() const noexcept;

    [[nodiscard]] static ZoneId MachineZone(MachineType type) noexcept;
    [[nodiscard]] Machine& GetMachine(MachineType type) noexcept;
    [[nodiscard]] const Machine& GetMachine(MachineType type) const noexcept;
    [[nodiscard]] Machine& ActiveMachine() noexcept;
    [[nodiscard]] const Machine& ActiveMachine() const noexcept;
    [[nodiscard]] Machine& ModalTargetMachine() noexcept;
    [[nodiscard]] const Machine& ModalTargetMachine() const noexcept;
    [[nodiscard]] std::vector<const Machine*> MachinesInCurrentScene() const;
    template <typename Fn>
    void ForEachMachineInScene(Fn&& fn) const {
        const LocationId loc = ActiveLocation();
        for (const auto& m : machines) {
            if (m.location != loc) {
                continue;
            }
            if (loc == LocationId::Overworld && MachineZone(m.type) != current_zone) {
                continue;
            }
            fn(m);
        }
    }

    [[nodiscard]] WorldNpc& GetNpc(NpcId id) noexcept;
    [[nodiscard]] const WorldNpc& GetNpc(NpcId id) const noexcept;
    [[nodiscard]] std::vector<const WorldNpc*> NpcsInCurrentScene() const;
    template <typename Fn>
    void ForEachNpcInScene(Fn&& fn) const {
        const LocationId loc = ActiveLocation();
        for (const auto& npc : npcs) {
            if (npc.location != loc) {
                continue;
            }
            if (loc == LocationId::Overworld && npc.zone != current_zone) {
                continue;
            }
            fn(npc);
        }
    }

    [[nodiscard]] std::vector<const CollectibleItem*> CollectiblesInCurrentScene() const;

    [[nodiscard]] ObjectiveTarget CurrentObjectiveTarget() const noexcept;

    [[nodiscard]] bool AllFiveMonumentsActivated() const noexcept;
    [[nodiscard]] int ActivatedMonumentsCount() const noexcept;

    [[nodiscard]] Tilemap& ActiveTilemap() noexcept;
    [[nodiscard]] const Tilemap& ActiveTilemap() const noexcept;
    [[nodiscard]] Vec2 ActiveCameraTarget() const noexcept;

    void EnterStructure(StructureId id);
    void ExitToOverworld();
    void TeleportToZone(ZoneId zone);
    void BeginZoneInterlude(ZoneId to_zone, TileCoord arrival_spawn_tile);
    void CommitZoneSwap();
    void SkipZoneInterlude();
    [[nodiscard]] bool IsInterludeActive() const noexcept { return interlude.active; }
    void TriggerZoneBanner(ZoneId zone) noexcept;
    void TriggerStructureBanner(StructureId id) noexcept;

    [[nodiscard]] StorySnapshot BuildStorySnapshot() const noexcept;

    void UpdateProximityPrompts() noexcept;
    EmpiricalScanReading TriggerScan(int player_id) noexcept;
    [[nodiscard]] WorldStepResult StepWorld(const AllPlayersInput& input, float dt);
};

[[nodiscard]] Vec2 FindSafeSpawnNear(
    const Tilemap& tilemap,
    Vec2 anchor_pos,
    Vec2 entity_size,
    Vec2 fallback_pos,
    bool bridge_unlocked = false,
    bool solar_gate_open = false,
    bool jade_bridge_unlocked = false) noexcept;

} // namespace eco
