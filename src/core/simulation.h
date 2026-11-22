#pragma once

#include <array>
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

struct PortalLink {
    StructureId target_structure = StructureId::HypogeanCisterns;
    ZoneId source_zone = ZoneId::CanalQuarter;
    TileCoord tile_min{0, 0};
    TileCoord tile_max{0, 0};
    Vec2 world_center{0.0f, 0.0f};
    Vec2 exit_spawn{0.0f, 0.0f};
};

struct OverworldStepResult {
    bool enter_structure_requested = false;
    StructureId target_structure = StructureId::HypogeanCisterns;
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
    bool collectible_picked_up = false;
    CollectibleId picked_up_id = CollectibleId::Dawn_BrassGnomon;
};

struct OverworldSim {
    Tilemap tilemap{};
    std::vector<PortalLink> portals{};
    StructureId last_used_structure = StructureId::HypogeanCisterns;
    Vec2 spawn_point{0.0f, 0.0f};
    Vec2 portal_exit_spawn{0.0f, 0.0f};
    Vec2 camera_target{0.0f, 0.0f};
    float portal_cooldown_timer = 0.0f;
    bool portal_reentry_locked = false;
    bool initialized = false;

    void InitializeDefault();
    void OnEnter(
        PlayerManager& players,
        bool returning_from_structure,
        bool bridge_unlocked = false,
        StructureId from_structure = StructureId::HypogeanCisterns);

    [[nodiscard]] ZoneId DistrictAt(Vec2 world_pos) const noexcept;
    [[nodiscard]] ZoneId DistrictAtTile(int row, int col) const noexcept;
    [[nodiscard]] Rect DistrictBounds(ZoneId zone) const noexcept;
    [[nodiscard]] const PortalLink* FindPortal(StructureId id) const noexcept;
    [[nodiscard]] Vec2 SafeSpawnInZone(ZoneId zone, bool bridge_unlocked = false) const noexcept;

    [[nodiscard]] OverworldStepResult Step(
        PlayerManager& players,
        const AllPlayersInput& input,
        float dt,
        bool bridge_unlocked = false);
};

struct StructureSim {
    StructureId active_structure = StructureId::HypogeanCisterns;
    std::array<Tilemap, static_cast<std::size_t>(StructureId::Count)> tilemaps{};
    std::array<Vec2, static_cast<std::size_t>(StructureId::Count)> spawn_points{};
    Tilemap tilemap{};
    Vec2 spawn_point{0.0f, 0.0f};
    Vec2 camera_target{0.0f, 0.0f};
    float portal_cooldown_timer = 0.0f;
    bool portal_reentry_locked = false;
    bool initialized = false;

    void InitializeDefault();
    void SelectStructure(StructureId id);
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

    [[nodiscard]] LocationId ActiveLocation() const noexcept;
    [[nodiscard]] const char* ActiveLocationName() const noexcept;
    [[nodiscard]] const char* ActiveLocationSubtitle() const noexcept;

    [[nodiscard]] Machine& GetMachine(MachineType type) noexcept;
    [[nodiscard]] const Machine& GetMachine(MachineType type) const noexcept;
    [[nodiscard]] Machine& ActiveMachine() noexcept;
    [[nodiscard]] const Machine& ActiveMachine() const noexcept;
    [[nodiscard]] Machine& ModalTargetMachine() noexcept;
    [[nodiscard]] const Machine& ModalTargetMachine() const noexcept;
    [[nodiscard]] std::vector<const Machine*> MachinesInCurrentScene() const;

    [[nodiscard]] WorldNpc& GetNpc(NpcId id) noexcept;
    [[nodiscard]] const WorldNpc& GetNpc(NpcId id) const noexcept;
    [[nodiscard]] std::vector<const WorldNpc*> NpcsInCurrentScene() const;

    [[nodiscard]] bool AllFiveMonumentsActivated() const noexcept;
    [[nodiscard]] int ActivatedMonumentsCount() const noexcept;

    [[nodiscard]] Tilemap& ActiveTilemap() noexcept;
    [[nodiscard]] const Tilemap& ActiveTilemap() const noexcept;
    [[nodiscard]] Vec2 ActiveCameraTarget() const noexcept;

    void EnterStructure(StructureId id);
    void ExitToOverworld();
    void TeleportToZone(ZoneId zone);
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
    bool bridge_unlocked = false) noexcept;

} // namespace eco
