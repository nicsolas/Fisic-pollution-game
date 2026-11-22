#include "core/simulation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "core/physics2d.h"

namespace eco {

namespace {

[[nodiscard]] bool IsFiniteVec2(Vec2 v) noexcept {
    return std::isfinite(v.x) && std::isfinite(v.y);
}

[[nodiscard]] bool IsSafeSpawnBox(
    const Tilemap& tilemap,
    Vec2 top_left,
    Vec2 entity_size,
    bool bridge_unlocked) noexcept {
    if (!IsFiniteVec2(top_left) || !IsFiniteVec2(entity_size) ||
        entity_size.x <= 0.0f || entity_size.y <= 0.0f) {
        return false;
    }
    const Rect box{top_left.x, top_left.y, entity_size.x, entity_size.y};
    return !tilemap.OverlapsSolid(box, bridge_unlocked) &&
           !tilemap.OverlapsTileId(box, TileId::StructurePortal) &&
           !tilemap.OverlapsTileId(box, TileId::ExitPortal);
}

[[nodiscard]] Vec2 CenteredTopLeftForTile(const Tilemap& tilemap, int row, int col) noexcept {
    const Vec2 center = tilemap.TileCenter(row, col);
    return {center.x - kPlayerWidth * 0.5f, center.y - kPlayerHeight * 0.5f};
}

[[nodiscard]] Vec2 GroundedTopLeftForTile(const Tilemap& tilemap, int row, int col) noexcept {
    const Rect tr = tilemap.TileRect(row, col);
    return {
        tr.x + (tr.width - kPlayerWidth) * 0.5f,
        tr.Bottom() - kPlayerHeight
    };
}

} // namespace

Vec2 FindSafeSpawnNear(
    const Tilemap& tilemap,
    Vec2 anchor_pos,
    Vec2 entity_size,
    Vec2 fallback_pos,
    bool bridge_unlocked) noexcept {
    static constexpr std::array<Vec2, 13> kOffsets{{
        {0.0f, 0.0f},
        {36.0f, 0.0f},
        {-36.0f, 0.0f},
        {0.0f, 36.0f},
        {0.0f, -36.0f},
        {36.0f, 36.0f},
        {-36.0f, 36.0f},
        {36.0f, -36.0f},
        {-36.0f, -36.0f},
        {48.0f, 0.0f},
        {-48.0f, 0.0f},
        {0.0f, 48.0f},
        {0.0f, -48.0f}
    }};

    if (IsFiniteVec2(anchor_pos)) {
        for (const Vec2& offset : kOffsets) {
            const Vec2 candidate{anchor_pos.x + offset.x, anchor_pos.y + offset.y};
            if (IsSafeSpawnBox(tilemap, candidate, entity_size, bridge_unlocked)) {
                return candidate;
            }
        }
    }

    if (IsFiniteVec2(fallback_pos)) {
        for (const Vec2& offset : kOffsets) {
            const Vec2 candidate{fallback_pos.x + offset.x, fallback_pos.y + offset.y};
            if (IsSafeSpawnBox(tilemap, candidate, entity_size, bridge_unlocked)) {
                return candidate;
            }
        }
    }

    if (IsFiniteVec2(anchor_pos)) {
        return anchor_pos;
    }
    return IsFiniteVec2(fallback_pos) ? fallback_pos : Vec2{0.0f, 0.0f};
}

void OverworldSim::InitializeDefault() {
    tilemap = Tilemap::CreateOverworldMap(kTileSize);
    // Open spawn plaza around (row 8, col 4)
    spawn_point = CenteredTopLeftForTile(tilemap, 8, 4);
    // Two tiles south of StructurePortal at (row 4, cols 15..16)
    portal_exit_spawn = CenteredTopLeftForTile(tilemap, 6, 15);
    camera_target = {spawn_point.x + kPlayerWidth * 0.5f, spawn_point.y + kPlayerHeight * 0.5f};
    portal_cooldown_timer = 0.0f;
    portal_reentry_locked = false;
    initialized = true;
}

void OverworldSim::OnEnter(
    PlayerManager& players,
    bool returning_from_structure,
    bool bridge_unlocked) {
    if (!initialized) {
        InitializeDefault();
    }

    Vec2 anchor = returning_from_structure ? portal_exit_spawn : spawn_point;
    const auto portals = tilemap.FindTiles(TileId::StructurePortal);
    if (returning_from_structure && !portals.empty()) {
        const Vec2 p_tl = CenteredTopLeftForTile(tilemap, portals[0].row, portals[0].col);
        if (!IsSafeSpawnBox(tilemap, anchor, {kPlayerWidth, kPlayerHeight}, bridge_unlocked)) {
            anchor = FindSafeSpawnNear(
                tilemap,
                {p_tl.x, p_tl.y + kTileSize * 1.5f},
                {kPlayerWidth, kPlayerHeight},
                spawn_point,
                bridge_unlocked);
        }
    }

    players.EnsureActive(0, anchor);

    static constexpr std::array<Vec2, 8> kEntryOffsets{{
        {0.0f, 0.0f},
        {36.0f, 0.0f},
        {-36.0f, 0.0f},
        {0.0f, 36.0f},
        {0.0f, -36.0f},
        {36.0f, 36.0f},
        {-36.0f, 36.0f},
        {36.0f, -36.0f}
    }};

    int slot_idx = 0;
    for (Player* p : players.ActivePlayers()) {
        Vec2 chosen = anchor;
        bool found = false;
        for (int attempt = 0; attempt < static_cast<int>(kEntryOffsets.size()); ++attempt) {
            const int idx = (slot_idx + attempt) % static_cast<int>(kEntryOffsets.size());
            const Vec2 candidate{
                anchor.x + kEntryOffsets[static_cast<std::size_t>(idx)].x,
                anchor.y + kEntryOffsets[static_cast<std::size_t>(idx)].y
            };
            if (IsSafeSpawnBox(tilemap, candidate, p->size, bridge_unlocked)) {
                chosen = candidate;
                found = true;
                break;
            }
        }
        if (!found) {
            chosen = FindSafeSpawnNear(tilemap, anchor, p->size, spawn_point, bridge_unlocked);
        }
        p->pos = chosen;
        p->vel = {0.0f, 0.0f};
        p->ResetVerticalState();
        ++slot_idx;
    }

    if (returning_from_structure) {
        portal_cooldown_timer = kPortalCooldownSeconds;
        portal_reentry_locked = true;
    } else {
        portal_cooldown_timer = 0.0f;
        portal_reentry_locked = false;
    }

    if (players.ActiveCount() > 0) {
        camera_target = players.AverageActivePosition();
    } else {
        camera_target = {anchor.x + kPlayerWidth * 0.5f, anchor.y + kPlayerHeight * 0.5f};
    }
}

OverworldStepResult OverworldSim::Step(
    PlayerManager& players,
    const AllPlayersInput& input,
    float dt,
    bool bridge_unlocked) {
    if (!initialized) {
        InitializeDefault();
    }

    OverworldStepResult result{};

    std::array<bool, kMaxPlayers> was_active{};
    Vec2 drop_anchor = spawn_point;
    bool found_anchor = false;
    if (const Player* p0 = players.GetPlayer(0); p0 != nullptr && p0->active) {
        drop_anchor = p0->pos;
        found_anchor = true;
    }
    for (int id = 0; id < kMaxPlayers; ++id) {
        const Player* p = players.GetPlayer(id);
        was_active[static_cast<std::size_t>(id)] = (p != nullptr && p->active);
        if (!found_anchor && p != nullptr && p->active) {
            drop_anchor = p->pos;
            found_anchor = true;
        }
    }

    players.ProcessDropIn(input, drop_anchor);

    for (int id = 0; id < kMaxPlayers; ++id) {
        if (was_active[static_cast<std::size_t>(id)]) {
            continue;
        }
        Player* p = players.GetPlayer(id);
        if (p == nullptr || !p->active) {
            continue;
        }
        ++result.newly_joined_count;
        static constexpr std::array<Vec2, 5> kJoinOffsets{{
            {36.0f, 0.0f}, {-36.0f, 0.0f}, {0.0f, 36.0f}, {0.0f, -36.0f}, {0.0f, 0.0f}
        }};
        const std::size_t join_idx = static_cast<std::size_t>(result.newly_joined_count - 1);
        bool placed = false;
        if (found_anchor) {
            for (std::size_t attempt = 0; attempt < kJoinOffsets.size(); ++attempt) {
                const Vec2& off = kJoinOffsets[(join_idx + attempt) % kJoinOffsets.size()];
                const Vec2 cand{drop_anchor.x + off.x, drop_anchor.y + off.y};
                if (IsSafeSpawnBox(tilemap, cand, p->size, bridge_unlocked)) {
                    p->pos = cand;
                    placed = true;
                    break;
                }
            }
        }
        if (!placed) {
            p->pos = FindSafeSpawnNear(tilemap, drop_anchor, p->size, spawn_point, bridge_unlocked);
        }
        p->vel = {0.0f, 0.0f};
        p->ResetVerticalState();
    }

    const float safe_dt = (std::isfinite(dt) && dt > 0.0f) ? dt : 0.0f;

    for (Player* p : players.ActivePlayers()) {
        const Vec2 move_axis = input.Get(p->player_id).MoveAxis();
        MovePlayerTopDown(*p, move_axis, safe_dt, tilemap, bridge_unlocked, kTopDownSpeed);
    }

    camera_target = UpdateAverageCameraTarget(camera_target, players, safe_dt, kCameraLerpSpeed);

    if (portal_cooldown_timer > 0.0f) {
        portal_cooldown_timer = std::max(0.0f, portal_cooldown_timer - safe_dt);
    }

    bool any_on_portal = false;
    for (const Player* p : players.ActivePlayers()) {
        if (tilemap.OverlapsTileId(p->Bounds(), TileId::StructurePortal)) {
            any_on_portal = true;
            break;
        }
    }

    if (portal_reentry_locked) {
        if (portal_cooldown_timer <= 0.0f && !any_on_portal) {
            portal_reentry_locked = false;
        }
    } else if (portal_cooldown_timer <= 0.0f && any_on_portal) {
        result.enter_structure_requested = true;
    }

    return result;
}

void StructureSim::InitializeDefault() {
    tilemap = Tilemap::CreateStructureMap(kTileSize);
    // Ground floor row 12, col 6 (away from ExitPortal at cols 2..3)
    spawn_point = GroundedTopLeftForTile(tilemap, 12, 6);
    camera_target = {spawn_point.x + kPlayerWidth * 0.5f, spawn_point.y + kPlayerHeight * 0.5f};
    portal_cooldown_timer = 0.0f;
    portal_reentry_locked = false;
    initialized = true;
}

void StructureSim::OnEnter(PlayerManager& players, bool bridge_unlocked) {
    if (!initialized) {
        InitializeDefault();
    }

    Vec2 anchor = spawn_point;
    if (!IsSafeSpawnBox(tilemap, anchor, {kPlayerWidth, kPlayerHeight}, bridge_unlocked)) {
        // Search for any non-solid, non-portal cell in current tilemap
        bool found_cell = false;
        for (int r = tilemap.Rows() - 1; r >= 0 && !found_cell; --r) {
            for (int c = 0; c < tilemap.RowCols(r); ++c) {
                const Vec2 cand = GroundedTopLeftForTile(tilemap, r, c);
                if (IsSafeSpawnBox(tilemap, cand, {kPlayerWidth, kPlayerHeight}, bridge_unlocked)) {
                    anchor = cand;
                    found_cell = true;
                    break;
                }
            }
        }
    }

    static constexpr std::array<Vec2, 8> kHorizOffsets{{
        {0.0f, 0.0f},
        {38.0f, 0.0f},
        {76.0f, 0.0f},
        {114.0f, 0.0f},
        {-38.0f, 0.0f},
        {0.0f, -48.0f},
        {38.0f, -48.0f},
        {-38.0f, -48.0f}
    }};

    int slot_idx = 0;
    for (Player* p : players.ActivePlayers()) {
        Vec2 chosen = anchor;
        bool found = false;
        for (int attempt = 0; attempt < static_cast<int>(kHorizOffsets.size()); ++attempt) {
            const int idx = (slot_idx + attempt) % static_cast<int>(kHorizOffsets.size());
            const Vec2 cand{
                anchor.x + kHorizOffsets[static_cast<std::size_t>(idx)].x,
                anchor.y + kHorizOffsets[static_cast<std::size_t>(idx)].y
            };
            if (IsSafeSpawnBox(tilemap, cand, p->size, bridge_unlocked)) {
                chosen = cand;
                found = true;
                break;
            }
        }
        if (!found) {
            chosen = FindSafeSpawnNear(tilemap, anchor, p->size, spawn_point, bridge_unlocked);
        }
        p->pos = chosen;
        p->vel = {0.0f, 0.0f};
        p->ResetVerticalState();
        if (IsSupportedByFloor(p->pos, p->size, tilemap, bridge_unlocked)) {
            p->isGrounded = true;
            p->coyote_timer = kCoyoteTimeDuration;
        }
        ++slot_idx;
    }

    portal_cooldown_timer = 0.0f;
    portal_reentry_locked = false;

    if (players.ActiveCount() > 0) {
        camera_target = players.AverageActivePosition();
    } else {
        camera_target = {anchor.x + kPlayerWidth * 0.5f, anchor.y + kPlayerHeight * 0.5f};
    }
}

StructureStepResult StructureSim::Step(
    PlayerManager& players,
    const AllPlayersInput& input,
    float dt,
    bool bridge_unlocked) {
    if (!initialized) {
        InitializeDefault();
    }

    StructureStepResult result{};

    std::array<bool, kMaxPlayers> was_active{};
    Vec2 drop_anchor = spawn_point;
    bool found_anchor = false;
    if (const Player* p0 = players.GetPlayer(0); p0 != nullptr && p0->active) {
        drop_anchor = p0->pos;
        found_anchor = true;
    }
    for (int id = 0; id < kMaxPlayers; ++id) {
        const Player* p = players.GetPlayer(id);
        was_active[static_cast<std::size_t>(id)] = (p != nullptr && p->active);
        if (!found_anchor && p != nullptr && p->active) {
            drop_anchor = p->pos;
            found_anchor = true;
        }
    }

    players.ProcessDropIn(input, drop_anchor);

    for (int id = 0; id < kMaxPlayers; ++id) {
        if (was_active[static_cast<std::size_t>(id)]) {
            continue;
        }
        Player* p = players.GetPlayer(id);
        if (p == nullptr || !p->active) {
            continue;
        }
        ++result.newly_joined_count;
        p->pos = FindSafeSpawnNear(tilemap, drop_anchor, p->size, spawn_point, bridge_unlocked);
        p->vel = {0.0f, 0.0f};
        p->ResetVerticalState();
        if (IsSupportedByFloor(p->pos, p->size, tilemap, bridge_unlocked)) {
            p->isGrounded = true;
            p->coyote_timer = kCoyoteTimeDuration;
        }
    }

    const float safe_dt = (std::isfinite(dt) && dt > 0.0f) ? dt : 0.0f;

    for (Player* p : players.ActivePlayers()) {
        const PlayerInput& in = input.Get(p->player_id);
        MovePlayerSideScrolling(
            *p,
            in.move_x,
            in.jump_pressed,
            in.jump_held,
            safe_dt,
            tilemap,
            bridge_unlocked,
            kSideMoveSpeed);
    }

    camera_target = UpdateAverageCameraTarget(camera_target, players, safe_dt, kCameraLerpSpeed);

    if (portal_cooldown_timer > 0.0f) {
        portal_cooldown_timer = std::max(0.0f, portal_cooldown_timer - safe_dt);
    }

    bool any_on_exit = false;
    for (const Player* p : players.ActivePlayers()) {
        if (tilemap.OverlapsTileId(p->Bounds(), TileId::ExitPortal)) {
            any_on_exit = true;
            break;
        }
    }

    if (portal_reentry_locked) {
        if (portal_cooldown_timer <= 0.0f && !any_on_exit) {
            portal_reentry_locked = false;
        }
    } else if (portal_cooldown_timer <= 0.0f && any_on_exit) {
        result.exit_to_overworld_requested = true;
    }

    return result;
}

void WorldContext::Reset() {
    const bool keep_demo = demo_mode;
    ecosystem.Reset();
    players = PlayerManager{};
    overworld = OverworldSim{};
    structure = StructureSim{};
    solar_machine = Machine::CreateDefaultSolar();
    hydro_machine = Machine::CreateDefaultHydro();
    purifier_machine = Machine::CreateDefaultSolar({1128.0f, 432.0f});
    purifier_machine.target_param = 120.0f;
    purifier_machine.current_param = 30.0f;
    purifier_machine.clean_energy_reward = 30.0f;
    purifier_machine.clean_pollution_reduction = 100.0f;
    purifier_machine.dirty_energy_reward = 30.0f;
    purifier_machine.dirty_pollution_increase = 35.0f;
    thermal_machine = Machine::CreateDefaultSolar({1128.0f, 216.0f});
    thermal_machine.target_param = 60.0f;
    thermal_machine.current_param = 15.0f;
    thermal_machine.clean_energy_reward = 25.0f;
    thermal_machine.clean_pollution_reduction = 35.0f;
    thermal_machine.dirty_energy_reward = 25.0f;
    thermal_machine.dirty_pollution_increase = 30.0f;
    calibration_modal.Reset();
    calibrating_purifier = false;
    calibrating_thermal = false;
    architect_npc.Reset();
    rosa_npc.Reset({1056.0f, 274.0f});
    rosa_npc.last_dialogue = "Nonna Rosa: Sali qui sull'impalcatura della Diga quando avrai riattivato la Turbina!";
    gaia_terminal_pos = {1128.0f, 648.0f};
    gaia_terminal_unlocked = false;
    demo_mode = keep_demo;
    last_scan = EmpiricalScanReading{};
    sim_time = 0.0f;
    scan_pulse_timer = 0.0f;
    scan_pulse_origin = {0.0f, 0.0f};
    simulation_completed = false;
    current_mode = GameMode::Overworld;
    returning_from_structure = false;
    story.Reset();
}

void WorldContext::EnsureInitialized() {
    if (!overworld.initialized) {
        overworld.InitializeDefault();
        purifier_machine.pos = {1128.0f, 432.0f};
        purifier_machine.target_param = 120.0f;
        if (purifier_machine.state == MachineState::Broken &&
            purifier_machine.current_param == 0.0f) {
            purifier_machine.current_param = 30.0f;
            purifier_machine.clean_energy_reward = 30.0f;
            purifier_machine.clean_pollution_reduction = 100.0f;
            purifier_machine.dirty_energy_reward = 30.0f;
            purifier_machine.dirty_pollution_increase = 35.0f;
        }
        thermal_machine.pos = {1128.0f, 216.0f};
        thermal_machine.target_param = 60.0f;
        if (thermal_machine.state == MachineState::Broken &&
            thermal_machine.current_param == 0.0f) {
            thermal_machine.current_param = 15.0f;
            thermal_machine.clean_energy_reward = 25.0f;
            thermal_machine.clean_pollution_reduction = 35.0f;
            thermal_machine.dirty_energy_reward = 25.0f;
            thermal_machine.dirty_pollution_increase = 30.0f;
        }
    }
    if (!structure.initialized) {
        structure.InitializeDefault();
    }
    if (architect_npc.last_dialogue.empty()) {
        architect_npc.Reset(architect_npc.pos);
    }
    if (rosa_npc.last_dialogue.empty()) {
        rosa_npc.Reset({1056.0f, 274.0f});
        rosa_npc.last_dialogue = "Nonna Rosa: Sali qui sull'impalcatura della Diga quando avrai riattivato la Turbina!";
    }
}

Machine& WorldContext::ActiveMachine() noexcept {
    return (current_mode == GameMode::Overworld) ? solar_machine : hydro_machine;
}

const Machine& WorldContext::ActiveMachine() const noexcept {
    return (current_mode == GameMode::Overworld) ? solar_machine : hydro_machine;
}

Machine& WorldContext::ModalTargetMachine() noexcept {
    if (current_mode == GameMode::Overworld) {
        if (calibrating_thermal) {
            return thermal_machine;
        }
        if (calibrating_purifier) {
            return purifier_machine;
        }
    }
    return ActiveMachine();
}

const Machine& WorldContext::ModalTargetMachine() const noexcept {
    if (current_mode == GameMode::Overworld) {
        if (calibrating_thermal) {
            return thermal_machine;
        }
        if (calibrating_purifier) {
            return purifier_machine;
        }
    }
    return ActiveMachine();
}

Machine& WorldContext::GetMachine(MachineType type) noexcept {
    return (type == MachineType::SolarArray) ? solar_machine : hydro_machine;
}

const Machine& WorldContext::GetMachine(MachineType type) const noexcept {
    return (type == MachineType::SolarArray) ? solar_machine : hydro_machine;
}

Tilemap& WorldContext::ActiveTilemap() noexcept {
    EnsureInitialized();
    return (current_mode == GameMode::Overworld) ? overworld.tilemap : structure.tilemap;
}

const Tilemap& WorldContext::ActiveTilemap() const noexcept {
    return (current_mode == GameMode::Overworld) ? overworld.tilemap : structure.tilemap;
}

Vec2 WorldContext::ActiveCameraTarget() const noexcept {
    return (current_mode == GameMode::Overworld)
               ? overworld.camera_target
               : structure.camera_target;
}

void WorldContext::UpdateProximityPrompts() noexcept {
    if (current_mode == GameMode::Overworld) {
        solar_machine.UpdateProximity(players);
        purifier_machine.UpdateProximity(players);
        if (!demo_mode && rosa_npc.recruited) {
            thermal_machine.UpdateProximity(players);
        } else {
            thermal_machine.prompt_visible = false;
        }
        hydro_machine.prompt_visible = false;
    } else {
        hydro_machine.UpdateProximity(players);
        solar_machine.prompt_visible = false;
        purifier_machine.prompt_visible = false;
        thermal_machine.prompt_visible = false;
    }
}

EmpiricalScanReading WorldContext::TriggerScan(int player_id) noexcept {
    if (const Player* p = players.GetPlayer(player_id); p != nullptr && p->active) {
        scan_pulse_timer = 0.85f;
        scan_pulse_origin = p->Center();
        if (current_mode == GameMode::Overworld && ecosystem.architect_recruited) {
            const float d_solar = solar_machine.DistanceToPlayer(*p);
            const float d_purifier = purifier_machine.DistanceToPlayer(*p);
            const float d_thermal = thermal_machine.DistanceToPlayer(*p);
            if (!demo_mode && rosa_npc.recruited &&
                d_thermal <= kDefaultScannerRange &&
                d_thermal < d_solar && d_thermal <= d_purifier) {
                last_scan = PerformEmpiricalScan(player_id, players, thermal_machine);
                return last_scan;
            }
            if (d_purifier < d_solar && d_purifier <= kDefaultScannerRange) {
                last_scan = PerformEmpiricalScan(player_id, players, purifier_machine);
                return last_scan;
            }
        }
    }
    last_scan = PerformEmpiricalScan(player_id, players, ActiveMachine());
    return last_scan;
}

WorldStepResult WorldContext::StepWorld(const AllPlayersInput& input, float dt) {
    EnsureInitialized();
    WorldStepResult out{};

    const float safe_dt = (std::isfinite(dt) && dt > 0.0f) ? dt : 0.0f;
    sim_time += safe_dt;
    if (scan_pulse_timer > 0.0f) {
        scan_pulse_timer = std::max(0.0f, scan_pulse_timer - safe_dt);
    }

    auto make_story_snapshot = [this]() -> StorySnapshot {
        StorySnapshot snap{};
        snap.mode = current_mode;
        snap.has_scanned = last_scan.active && last_scan.has_target;
        snap.solar = solar_machine.state;
        snap.hydro = hydro_machine.state;
        snap.purifier = purifier_machine.state;
        snap.architect_recruited = ecosystem.architect_recruited;
        snap.pollution_level = ecosystem.pollution_level;
        snap.simulation_completed = simulation_completed;
        snap.rosa_recruited = rosa_npc.recruited;
        snap.thermal = thermal_machine.state;
        snap.gaia_terminal_unlocked = gaia_terminal_unlocked;
        snap.demo_mode = demo_mode;
        return snap;
    };

    if (input.advance_dialogue_pressed) {
        story.AdvanceDialogue();
    }

    // 1. UI Phase cycling (respects MaxUnlockedUiPhase gates)
    bool wants_cycle_phase = input.toggle_ui_phase_pressed;
    for (const Player* p : players.ActivePlayers()) {
        if (p != nullptr && input.Get(p->player_id).toggle_overlay_pressed) {
            wants_cycle_phase = true;
            break;
        }
    }
    if (wants_cycle_phase) {
        ecosystem.CycleUiPhase();
    }

    // 2. Empirical scanner trigger
    for (const Player* p : players.ActivePlayers()) {
        if (p != nullptr && input.Get(p->player_id).scan_pressed) {
            (void)TriggerScan(p->player_id);
        }
    }

    // 3. If modal calibration popup is open, freeze player movement and handle modal controls
    if (calibration_modal.is_open) {
        Machine& machine = ModalTargetMachine();
        for (const Player* p : players.ActivePlayers()) {
            if (p == nullptr) {
                continue;
            }
            const PlayerInput& in = input.Get(p->player_id);
            if (std::isfinite(in.slider_delta) && in.slider_delta != 0.0f) {
                calibration_modal.AdjustParameter(in.slider_delta, machine);
            }
            if (in.cancel_pressed) {
                calibration_modal.Cancel();
                calibrating_purifier = false;
                calibrating_thermal = false;
                break;
            }
            if (in.bypass_pressed) {
                calibration_modal.TriggerBypass(machine, ecosystem);
                if (calibrating_purifier && demo_mode) {
                    simulation_completed = true;
                }
                calibrating_purifier = false;
                calibrating_thermal = false;
                break;
            }
            if (in.confirm_pressed) {
                if (calibration_modal.ConfirmClean(machine, ecosystem)) {
                    if (calibrating_purifier) {
                        if (demo_mode) {
                            simulation_completed = true;
                        }
                        architect_npc.last_dialogue =
                            "MATRICE PURIFICATA (0% Smog): L'ecosistema e le leggi fisiche sono in equilibrio perfetto!";
                    }
                    calibrating_purifier = false;
                    calibrating_thermal = false;
                }
                break;
            }
        }
        UpdateProximityPrompts();
        story.Observe(make_story_snapshot());
        story.Tick(safe_dt);
        return out;
    }

    // 4. Step physics & scene simulation
    if (current_mode == GameMode::Overworld) {
        const OverworldStepResult ow_res =
            overworld.Step(players, input, dt, ecosystem.architect_recruited);
        out.enter_structure_requested = ow_res.enter_structure_requested;
        out.newly_joined_count = ow_res.newly_joined_count;
    } else {
        const StructureStepResult st_res =
            structure.Step(players, input, dt, ecosystem.architect_recruited);
        out.exit_to_overworld_requested = st_res.exit_to_overworld_requested;
        out.newly_joined_count = st_res.newly_joined_count;
    }

    // 5. Update machine proximity prompts for active scene
    UpdateProximityPrompts();

    // 6. Handle interaction presses (Machine modal, Purifier modal, Thermal modal, Rosa NPC, GAIA Terminal, Architect NPC)
    Machine& machine = ActiveMachine();
    for (const Player* p : players.ActivePlayers()) {
        if (p == nullptr) {
            continue;
        }
        const PlayerInput& in = input.Get(p->player_id);
        if (!in.interact_pressed) {
            continue;
        }
        if (machine.IsPlayerInRange(*p)) {
            calibrating_purifier = false;
            calibrating_thermal = false;
            calibration_modal.OpenFor(machine);
            break;
        }
        if (current_mode == GameMode::Overworld &&
            ecosystem.architect_recruited &&
            purifier_machine.IsPlayerInRange(*p)) {
            calibrating_purifier = true;
            calibrating_thermal = false;
            calibration_modal.OpenFor(purifier_machine);
            break;
        }
        if (current_mode == GameMode::Overworld &&
            !demo_mode &&
            rosa_npc.recruited &&
            thermal_machine.IsPlayerInRange(*p)) {
            calibrating_thermal = true;
            calibrating_purifier = false;
            calibration_modal.OpenFor(thermal_machine);
            break;
        }
        if (current_mode == GameMode::Overworld &&
            !demo_mode &&
            rosa_npc.recruited &&
            thermal_machine.state != MachineState::Broken &&
            (p->Center() - gaia_terminal_pos).Length() <= 78.0f) {
            gaia_terminal_unlocked = true;
            simulation_completed = true;
            break;
        }
        if (current_mode == GameMode::Structure &&
            !demo_mode &&
            rosa_npc.IsPlayerInRange(*p)) {
            if (hydro_machine.state != MachineState::Broken) {
                rosa_npc.recruited = true;
                rosa_npc.last_dialogue =
                    "Nonna Rosa: Ecco le chiavi dello Scambiatore Geotermico nel Settore Est!";
            } else {
                rosa_npc.last_dialogue =
                    "Nonna Rosa: Prima riattiva la Turbina qui sotto, poi ti daro' i codici geotermici!";
            }
            break;
        }
        if (current_mode == GameMode::Overworld && architect_npc.IsPlayerInRange(*p)) {
            (void)architect_npc.TryInteract(p->player_id, players, ecosystem);
        }
    }

    if (demo_mode) {
        if ((solar_machine.state == MachineState::CleanActive &&
             hydro_machine.state == MachineState::CleanActive &&
             purifier_machine.state == MachineState::CleanActive &&
             ecosystem.pollution_level <= 0.01f) ||
            (solar_machine.state != MachineState::Broken &&
             hydro_machine.state != MachineState::Broken &&
             purifier_machine.state != MachineState::Broken)) {
            simulation_completed = true;
        }
    } else if (gaia_terminal_unlocked) {
        simulation_completed = true;
    }

    story.Observe(make_story_snapshot());
    story.Tick(safe_dt);

    return out;
}

} // namespace eco
