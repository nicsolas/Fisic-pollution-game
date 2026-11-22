#pragma once

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

struct OverworldStepResult {
    bool enter_structure_requested = false;
    int newly_joined_count = 0;
};

struct StructureStepResult {
    bool exit_to_overworld_requested = false;
    int newly_joined_count = 0;
};

struct WorldStepResult {
    bool enter_structure_requested = false;
    bool exit_to_overworld_requested = false;
    int newly_joined_count = 0;
};

struct OverworldSim {
    Tilemap tilemap{};
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
        bool bridge_unlocked = false);

    [[nodiscard]] OverworldStepResult Step(
        PlayerManager& players,
        const AllPlayersInput& input,
        float dt,
        bool bridge_unlocked = false);
};

struct StructureSim {
    Tilemap tilemap{};
    Vec2 spawn_point{0.0f, 0.0f};
    Vec2 camera_target{0.0f, 0.0f};
    float portal_cooldown_timer = 0.0f;
    bool portal_reentry_locked = false;
    bool initialized = false;

    void InitializeDefault();
    void OnEnter(PlayerManager& players, bool bridge_unlocked = false);

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
    Machine solar_machine = Machine::CreateDefaultSolar();
    Machine hydro_machine = Machine::CreateDefaultHydro();
    Machine purifier_machine = Machine::CreateDefaultSolar({1128.0f, 432.0f});
    Machine thermal_machine = Machine::CreateDefaultSolar({1128.0f, 216.0f});
    CalibrationModalState calibration_modal{};
    bool calibrating_purifier = false;
    bool calibrating_thermal = false;
    ArchitectNpc architect_npc{};
    ArchitectNpc rosa_npc{};
    Vec2 gaia_terminal_pos{1128.0f, 648.0f};
    bool gaia_terminal_unlocked = false;
    bool demo_mode = true; // true = Demo Atti I-III; false = Campagna Completa Atti I-V
    EmpiricalScanReading last_scan{};
    float sim_time = 0.0f;
    float scan_pulse_timer = 0.0f;
    Vec2 scan_pulse_origin{0.0f, 0.0f};
    bool simulation_completed = false;
    GameMode current_mode = GameMode::Overworld;
    bool returning_from_structure = false;
    StoryDirector story{};

    void Reset();
    void EnsureInitialized();

    [[nodiscard]] Machine& ActiveMachine() noexcept;
    [[nodiscard]] const Machine& ActiveMachine() const noexcept;
    [[nodiscard]] Machine& ModalTargetMachine() noexcept;
    [[nodiscard]] const Machine& ModalTargetMachine() const noexcept;
    [[nodiscard]] Machine& GetMachine(MachineType type) noexcept;
    [[nodiscard]] const Machine& GetMachine(MachineType type) const noexcept;

    [[nodiscard]] Tilemap& ActiveTilemap() noexcept;
    [[nodiscard]] const Tilemap& ActiveTilemap() const noexcept;
    [[nodiscard]] Vec2 ActiveCameraTarget() const noexcept;

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
