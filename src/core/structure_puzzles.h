#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "core/math_types.h"
#include "core/player.h"
#include "core/tilemap.h"

namespace eco {

// Authoritative spawn & entity tile coordinates for each internal structure chamber.
struct StructureSpawnLayout {
    StructureId structure_id = StructureId::HypogeanCisterns;
    int rows = 20;
    int cols = 40;
    TileCoord spawn_tile{17, 5};
    std::array<TileCoord, 2> exit_portal_tiles{{{17, 2}, {17, 3}}};
    TileCoord machine_tile{16, 36};
    bool has_npc = false;
    TileCoord npc_tile{0, 0};
    std::array<TileCoord, 2> collectible_tiles{{{17, 25}, {16, 33}}};
};

// Logical activation rules for barriers and retractable bridges.
enum class StructureLogicRule : std::uint8_t {
    AllActive = 0,     // Logical AND of all switches in required_switches
    AnyActive = 1,     // Logical OR of switches in required_switches
    ExactMask = 2,     // Exact ON/OFF match between required_switches and required_states
    SequenceOrder = 3  // Current valid prefix length >= sequence_target_len
};

// Interactive switch, lever, chime, or thermal valve inside a side-scrolling structure.
struct StructureSwitch {
    int id = 0;
    TileCoord tile{0, 0};
    Vec2 pos{0.0f, 0.0f};
    float interact_radius = 64.0f;
    bool active = false;
    bool is_momentary = false;
    const char* symbol = "A"; // Pure ASCII symbol shown in HUD/world
};

// Dynamic barrier (is_bridge == false) or retractable bridge/platform (is_bridge == true).
struct StructureGate {
    int id = 0;
    bool is_bridge = false;
    bool open = false;
    StructureLogicRule rule = StructureLogicRule::AllActive;
    std::vector<TileCoord> tiles{};
    std::vector<int> required_switches{};
    std::vector<bool> required_states{};
    int sequence_target_len = 0;
    const char* label = ""; // Pure ASCII label
};

// Diegetic observation station for empirical field readings ([E] inspect or [Q] scan).
struct ObservationNode {
    int id = 0;
    LocationId location = LocationId::Overworld;
    ZoneId zone = ZoneId::DawnTerraces;
    TileCoord tile{0, 0};
    Vec2 pos{0.0f, 0.0f};
    float interact_radius = 68.0f;
    bool inspected = false;
    bool scanned = false;
    bool prompt_visible = false;
    const char* title = "";
    const char* reading_text = "";
    const char* hypothesis_hint = "";
};

// Event flags emitted when a player interacts with a puzzle switch.
struct StructurePuzzleEventFlags {
    bool switch_toggled = false;
    bool gate_opened = false;
    bool gate_closed = false;
    bool sequence_failed = false;
    bool puzzle_solved = false;
    bool observation_inspected = false;
    bool aux_valve_toggled = false;

    [[nodiscard]] constexpr bool Any() const noexcept {
        return switch_toggled || gate_opened || gate_closed || sequence_failed ||
               puzzle_solved || observation_inspected || aux_valve_toggled;
    }
};

// Returns exact spawn, portal, machine, NPC, and collectible tile coordinates.
[[nodiscard]] StructureSpawnLayout GetStructureLayout(StructureId id) noexcept;

// Builds the redesigned multi-tier side-scrolling tilemap for the given structure,
// with initial gate/bridge states already applied.
[[nodiscard]] Tilemap BuildStructurePuzzleMap(StructureId id, float tile_size = kTileSize);

// Self-contained logic puzzle state and simulation for a single structure chamber.
struct StructurePuzzleState {
    StructureId structure_id = StructureId::HypogeanCisterns;
    float tile_size = kTileSize;
    std::vector<StructureSwitch> switches{};
    std::vector<StructureGate> gates{};
    std::vector<ObservationNode> observation_nodes{};
    std::vector<int> expected_sequence{};
    std::vector<int> current_sequence{};
    TileCoord aux_stevino_valve_tile{17, 24};
    Vec2 aux_stevino_valve_pos{0.0f, 0.0f};
    bool aux_stevino_valve_unlocked = false; // Unlocked by Waters_SluiceStele
    bool aux_stevino_valve_open = false;     // Dry auxiliary equilibrium path active
    bool aux_valve_prompt_visible = false;
    bool high_spillway_open = false;         // Opened by HydroTurbine RiskyEfficient or trial pulse
    float trial_water_hammer_timer = 0.0f;   // Temporary sluice surge from RunTrial
    bool archivist_niche_open = false;       // Opened by Lorenzo in FlourishingTrust
    const char* title = "";
    const char* hint_text = "";
    bool solved = false;

    void ResetForStructure(StructureId id, float tile_sz = kTileSize);
    void ApplyGatesToTilemap(Tilemap& tilemap) const noexcept;
    void EvaluateGates(
        Tilemap& tilemap,
        StructurePuzzleEventFlags* events = nullptr) noexcept;

    [[nodiscard]] StructurePuzzleEventFlags TryInteract(
        const PlayerManager& players,
        Tilemap& tilemap) noexcept;

    [[nodiscard]] StructurePuzzleEventFlags ToggleSwitchById(
        int switch_id,
        Tilemap& tilemap) noexcept;

    [[nodiscard]] const StructureSwitch* FindNearestSwitch(
        Vec2 world_pos,
        float max_radius = -1.0f) const noexcept;

    [[nodiscard]] const StructureSwitch* GetSwitch(int switch_id) const noexcept;
    [[nodiscard]] const StructureGate* GetGate(int gate_id) const noexcept;

private:
    [[nodiscard]] bool EvaluateGateCondition(const StructureGate& gate) const noexcept;
};

} // namespace eco
