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
#include "core/structure_puzzles.h"
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

// ============================================================================
// Vertical Slice: Ecologia Sistemica, Percorsi Dinamici, Vasche & Fauna Viva
// ============================================================================

enum class PassageKind : int {
    PitchBramble = 0,     // Bloccato se inquinato/Dirty; lastricato fiorito se guarito/BioPlanter
    ScorchedCrest = 1,    // Scorciatoia della Cresta Arsa (SolarArray RiskyEfficient)
    LaminarFord = 2,      // Guado degli Idraulici (Waters_HydraulicDiary + chiuse laminari)
    ArtisanArchway = 3,   // Passaggio degli Artigiani (Clara FlourishingTrust)
    HighSpillway = 4,     // Sfioratore del Prisma (HydroTurbine RiskyEfficient / colpo d'ariete)
    ArchivistNiche = 5    // Nicchia Sigillata d'Archivio (Lorenzo FlourishingTrust)
};

struct DynamicPassage {
    int id = 0;
    PassageKind kind = PassageKind::PitchBramble;
    LocationId location = LocationId::Overworld;
    ZoneId zone = ZoneId::DawnTerraces;
    Rect bounds{};
    std::vector<TileCoord> tiles{};
    bool open = false;
    const char* label = "";
    const char* state_hint = "";
};

struct BioPlanter {
    int id = 0;
    ZoneId zone = ZoneId::DawnTerraces;
    TileCoord tile{0, 0};
    Vec2 pos{0.0f, 0.0f};
    float interact_radius = 68.0f;
    float aura_radius = 220.0f;
    bool planted = false;
    bool prompt_visible = false;
    int clears_passage_id = -1;
    const char* label = "";
};

enum class CritterKind : int {
    Swallow = 0,      // Rondini radenti (Terrazze dell'Alba)
    SilverCarp = 1,   // Carpe argentate (Quartiere dei Canali)
    Dragonfly = 2,    // Libellule turchesi (Quartiere dei Canali)
    GlowNewt = 3,     // Tritoni bioluminescenti (Cisterne Ipogee)
    CaveFirefly = 4   // Lucciole di grotta (Cisterne Ipogee)
};

inline constexpr std::size_t kMaxAmbientCritters = 12;

struct AmbientCritter {
    bool active = false;
    CritterKind kind = CritterKind::Swallow;
    Vec2 pos{0.0f, 0.0f};
    Vec2 anchor{0.0f, 0.0f};
    Vec2 velocity{0.0f, 0.0f};
    float orbit_radius = 36.0f;
    float phase = 0.0f;
    float speed = 1.4f;
};

enum class RemoteTargetKind : int {
    None = 0,
    SolarMirror = 1,
    SluiceLever = 2,
    StructureSwitch = 3
};

inline constexpr float kFarewellPrismRemoteRange = 220.0f;

struct RemotePrismTarget {
    bool valid = false;
    RemoteTargetKind kind = RemoteTargetKind::None;
    int target_id = -1;
    Vec2 world_pos{0.0f, 0.0f};
    float distance = 0.0f;
    const char* label = "";
};

struct GuidanceDisplayState {
    bool show_compass = false;
    bool show_distance_meters = false;
    bool show_edge_arrow = false;
    bool show_world_beacon = false;
    bool is_scan_pulse_reveal = false;
    const char* field_note = "";
};

struct VerticalSliceSummary {
    bool terraces_resolved = false;
    bool canals_explored = false;
    bool cisterns_resolved = false;
    bool clara_confronted = false;
    bool lorenzo_confronted = false;
    bool slice_complete = false;
    CalibrationApproach solar_approach = CalibrationApproach::None;
    CalibrationApproach hydro_approach = CalibrationApproach::None;
    InquiryStage highest_inquiry_stage = InquiryStage::Observe;
    int observations_count = 0;
    int trials_run_total = 0;
    int slice_relics_collected = 0;
    int planters_seeded = 0;
    bool laminar_ford_unlocked = false;
    bool scorched_shortcut_open = false;
    bool high_spillway_open = false;
    EcoHealthBand eco_band = EcoHealthBand::Polluted;
    float vitality_score = 0.5f;
    const char* synthesis_headline = "";
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
    bool observation_inspected = false;
    int observation_id = -1;
    bool planter_seeded = false;
    bool laminar_ford_opened = false;
    bool remote_prism_triggered = false;
    bool trial_pulse_triggered = false;
    bool calibration_confirmed = false;
    CalibrationApproach confirmed_approach = CalibrationApproach::None;
};

struct ZoneMechanicState {
    // 1. Terrazze dell'Alba
    Vec2 solar_emitter_pos{0.0f, 0.0f};
    Vec2 solar_emitter_dir{1.0f, 0.0f};
    std::vector<SolarMirror> solar_mirrors{};
    std::vector<SolarBeamSegment> solar_beams{};
    std::vector<SolarPlate> solar_plates{};
    bool solar_gate_open = false;
    bool scorched_crest_open = false;
    bool hidden_sundial_glyphs_revealed = false;
    float solar_trial_flash_timer = 0.0f;

    // 2. Quartiere dei Canali
    std::vector<WaterCurrentZone> water_currents{};
    std::vector<SluiceLever> sluice_levers{};
    bool laminar_flow_active = false;
    bool laminar_ford_open = false;

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

    // 7. Vertical Slice Systemic Layer (zero-heap critter pool + stations/passages)
    std::vector<ObservationNode> observation_nodes{};
    std::vector<BioPlanter> bio_planters{};
    std::vector<DynamicPassage> dynamic_passages{};
    std::array<AmbientCritter, kMaxAmbientCritters> ambient_critters{};
    int active_critter_count = 0;

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
    bool blocked_gateway_touched = false;
    ZoneId blocked_gateway_target = ZoneId::DawnTerraces;
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
    StructurePuzzleEventFlags structure_puzzle_events{};
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
        Vec2 external_velocity = {0.0f, 0.0f},
        bool forward_gateway_unlocked = true);
};

struct StructureSim {
    StructureId active_structure = StructureId::HypogeanCisterns;
    Tilemap tilemap{};
    StructurePuzzleState puzzle{};
    std::array<StructurePuzzleState, static_cast<std::size_t>(StructureId::Count)> saved_puzzles{};
    Vec2 spawn_point{0.0f, 0.0f};
    Vec2 camera_target{0.0f, 0.0f};
    float portal_cooldown_timer = 0.0f;
    bool portal_reentry_locked = false;
    bool initialized = false;

    void InitializeDefault();
    void SyncActivePuzzleToSaved() noexcept;
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
    float locked_gateway_hint_timer = 0.0f;
    ZoneId banner_zone = ZoneId::DawnTerraces;
    StructureId banner_structure = StructureId::HypogeanCisterns;
    bool banner_is_structure = false;
    bool instant_zone_transitions = false;

    bool demo_mode = false; // Campagna Completa (La Conca di Meridia) attiva di default
    bool hard_physics_mode = false; // Modalita' Fisica Pura (Hard): calcolo manuale senza aiuti live
    EmpiricalScanReading last_scan{};
    float sim_time = 0.0f;
    float scan_pulse_timer = 0.0f;
    Vec2 scan_pulse_origin{0.0f, 0.0f};
    GuidanceAssistMode guidance_mode = GuidanceAssistMode::Empirical;
    float empirical_compass_timer = 0.0f;

    // Ciclo d'indagine Vertical Slice (0=DawnTerraces, 1=CanalQuarter, 2=HypogeanCisterns)
    std::array<InquiryStage, 3> slice_inquiry_stage{{
        InquiryStage::Observe,
        InquiryStage::Observe,
        InquiryStage::Observe
    }};
    int total_trials_run = 0;
    int observations_inspected_count = 0;
    int planters_seeded_count = 0;
    int prism_remote_toggles = 0;
    bool vertical_slice_synthesis_done = false;

    // Timer e coordinate live per VFX/SFX procedurali (zero allocazioni)
    float trial_pulse_timer = 0.0f;
    Vec2 trial_pulse_origin{0.0f, 0.0f};
    MachineType last_trial_machine = MachineType::SolarArray;
    float prism_pulse_timer = 0.0f;
    Vec2 prism_pulse_from{0.0f, 0.0f};
    Vec2 prism_pulse_to{0.0f, 0.0f};
    float planter_bloom_timer = 0.0f;
    Vec2 planter_bloom_pos{0.0f, 0.0f};

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
    [[nodiscard]] GuidanceDisplayState ComputeGuidanceDisplay() const noexcept;
    [[nodiscard]] InquiryStage CurrentAreaInquiryStage() const noexcept;
    void AdvanceInquiryStageForArea(LocationId loc, ZoneId zone, InquiryStage min_stage) noexcept;

    [[nodiscard]] ZoneEcoStatus ComputeZoneEcoStatus(ZoneId zone) const noexcept;
    [[nodiscard]] ZoneEcoStatus GetZoneEcoStatus(ZoneId zone) const noexcept {
        return ComputeZoneEcoStatus(zone);
    }
    [[nodiscard]] ZoneEcoStatus ActiveSceneEcoStatus() const noexcept;
    [[nodiscard]] VerticalSliceSummary ComputeVerticalSliceSummary() const noexcept;

    [[nodiscard]] RemotePrismTarget FindRemotePrismTarget(
        Vec2 origin,
        float max_range = kFarewellPrismRemoteRange) const noexcept;
    [[nodiscard]] std::vector<SolarBeamSegment> PredictGnomonBeams() const;

    // Helpers richiamabili sia da StepWorld (tastiera/gamepad) sia da calibration_ui.cpp (click mouse)
    void OpenCalibrationModalFor(MachineType type) noexcept;
    TrialExperimentReport ExecuteModalTrial() noexcept;
    bool ExecuteModalConfirmClean() noexcept;
    bool ExecuteModalConfirmRisky() noexcept;
    bool ExecuteModalBypass() noexcept;
    void RefreshSystemicWorldState() noexcept;
    void TickAmbientCritters(float dt) noexcept;

    [[nodiscard]] bool AllFiveMonumentsActivated() const noexcept;
    [[nodiscard]] int ActivatedMonumentsCount() const noexcept;

    [[nodiscard]] bool IsZoneMainTaskCompleted(ZoneId zone) const noexcept;
    [[nodiscard]] bool IsZoneGatewayUnlocked(const ZoneGateway& gw) const noexcept;
    [[nodiscard]] const char* ZoneGatewayLockReason(const ZoneGateway& gw) const noexcept;
    [[nodiscard]] const char* ZoneGatewayLockedPlaque(const ZoneGateway& gw) const noexcept;

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
