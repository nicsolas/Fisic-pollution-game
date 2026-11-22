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

[[nodiscard]] Rect PortalLinkRect(const Tilemap& tilemap, const PortalLink& link) noexcept {
    const Rect rmin = tilemap.TileRect(link.tile_min.row, link.tile_min.col);
    const Rect rmax = tilemap.TileRect(link.tile_max.row, link.tile_max.col);
    return {
        rmin.Left(),
        rmin.Top(),
        rmax.Right() - rmin.Left(),
        rmax.Bottom() - rmin.Top()
    };
}

[[nodiscard]] std::size_t StructureIndex(StructureId id) noexcept {
    const int idx = static_cast<int>(id);
    if (idx < 0 || idx >= static_cast<int>(StructureId::Count)) {
        return 0;
    }
    return static_cast<std::size_t>(idx);
}

[[nodiscard]] std::size_t MachineIndex(MachineType type) noexcept {
    const int idx = static_cast<int>(type);
    if (idx < 0 || idx >= static_cast<int>(MachineType::Count)) {
        return 0;
    }
    return static_cast<std::size_t>(idx);
}

[[nodiscard]] std::size_t NpcIndex(NpcId id) noexcept {
    const int idx = static_cast<int>(id);
    if (idx < 0 || idx >= static_cast<int>(NpcId::Count)) {
        return 0;
    }
    return static_cast<std::size_t>(idx);
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
    // Initial spawn in Terrazze dell'Alba (row 30, col 5)
    spawn_point = CenteredTopLeftForTile(tilemap, 30, 5);

    portals.clear();
    // Portal 1: Cisterne Ipogee (Quartiere dei Canali, row 14, cols 5..6)
    {
        PortalLink link{};
        link.target_structure = StructureId::HypogeanCisterns;
        link.source_zone = ZoneId::CanalQuarter;
        link.tile_min = {14, 5};
        link.tile_max = {14, 6};
        link.world_center = PortalLinkRect(tilemap, link).Center();
        link.exit_spawn = CenteredTopLeftForTile(tilemap, 16, 5);
        portals.push_back(link);
    }
    // Portal 2: Torre del Pendolo (Altopiano dei Venti, row 3, cols 11..12)
    {
        PortalLink link{};
        link.target_structure = StructureId::PendulumTower;
        link.source_zone = ZoneId::WindPlateau;
        link.tile_min = {3, 11};
        link.tile_max = {3, 12};
        link.world_center = PortalLinkRect(tilemap, link).Center();
        link.exit_spawn = CenteredTopLeftForTile(tilemap, 5, 11);
        portals.push_back(link);
    }
    // Portal 3: Camera Geotermica (Caldera delle Fumarole, row 3, cols 37..38)
    {
        PortalLink link{};
        link.target_structure = StructureId::GeothermalChamber;
        link.source_zone = ZoneId::FumaroleCaldera;
        link.tile_min = {3, 37};
        link.tile_max = {3, 38};
        link.world_center = PortalLinkRect(tilemap, link).Center();
        link.exit_spawn = CenteredTopLeftForTile(tilemap, 5, 37);
        portals.push_back(link);
    }

    last_used_structure = StructureId::HypogeanCisterns;
    portal_exit_spawn = portals[0].exit_spawn;
    camera_target = {spawn_point.x + kPlayerWidth * 0.5f, spawn_point.y + kPlayerHeight * 0.5f};
    portal_cooldown_timer = 0.0f;
    portal_reentry_locked = false;
    initialized = true;
}

ZoneId OverworldSim::DistrictAt(Vec2 world_pos) const noexcept {
    return tilemap.DistrictAt(world_pos);
}

ZoneId OverworldSim::DistrictAtTile(int row, int col) const noexcept {
    return Tilemap::DistrictAtTile(row, col);
}

Rect OverworldSim::DistrictBounds(ZoneId zone) const noexcept {
    return tilemap.DistrictBounds(zone);
}

const PortalLink* OverworldSim::FindPortal(StructureId id) const noexcept {
    for (const auto& link : portals) {
        if (link.target_structure == id) {
            return &link;
        }
    }
    return nullptr;
}

Vec2 OverworldSim::SafeSpawnInZone(ZoneId zone, bool bridge_unlocked) const noexcept {
    TileCoord target{30, 5};
    switch (zone) {
        case ZoneId::DawnTerraces:
            target = {30, 5};
            break;
        case ZoneId::CanalQuarter:
            target = {17, 11};
            break;
        case ZoneId::WindPlateau:
            target = {7, 11};
            break;
        case ZoneId::HangingGardens:
            target = {17, 32};
            break;
        case ZoneId::FumaroleCaldera:
            target = {8, 36};
            break;
        case ZoneId::HarmonySanctuary:
            target = {29, 32};
            break;
        case ZoneId::Count:
            break;
    }
    const Vec2 anchor = CenteredTopLeftForTile(tilemap, target.row, target.col);
    return FindSafeSpawnNear(tilemap, anchor, {kPlayerWidth, kPlayerHeight}, spawn_point, bridge_unlocked);
}

void OverworldSim::OnEnter(
    PlayerManager& players,
    bool returning_from_structure,
    bool bridge_unlocked,
    StructureId from_structure) {
    if (!initialized) {
        InitializeDefault();
    }

    Vec2 anchor = spawn_point;
    if (returning_from_structure) {
        if (const PortalLink* link = FindPortal(from_structure); link != nullptr) {
            anchor = link->exit_spawn;
            portal_exit_spawn = link->exit_spawn;
            last_used_structure = from_structure;
        } else {
            anchor = portal_exit_spawn;
        }
        if (!IsSafeSpawnBox(tilemap, anchor, {kPlayerWidth, kPlayerHeight}, bridge_unlocked)) {
            anchor = FindSafeSpawnNear(
                tilemap,
                anchor,
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
    StructureId hit_structure = last_used_structure;
    for (const Player* p : players.ActivePlayers()) {
        if (tilemap.OverlapsTileId(p->Bounds(), TileId::StructurePortal)) {
            any_on_portal = true;
            // Match the exact portal link
            for (const auto& link : portals) {
                if (p->Bounds().Overlaps(PortalLinkRect(tilemap, link))) {
                    hit_structure = link.target_structure;
                    break;
                }
            }
            break;
        }
    }

    if (portal_reentry_locked) {
        if (portal_cooldown_timer <= 0.0f && !any_on_portal) {
            portal_reentry_locked = false;
        }
    } else if (portal_cooldown_timer <= 0.0f && any_on_portal) {
        result.enter_structure_requested = true;
        result.target_structure = hit_structure;
        last_used_structure = hit_structure;
    }

    return result;
}

void StructureSim::InitializeDefault() {
    tilemaps[StructureIndex(StructureId::HypogeanCisterns)] =
        Tilemap::CreateStructureMap(StructureId::HypogeanCisterns, kTileSize);
    tilemaps[StructureIndex(StructureId::PendulumTower)] =
        Tilemap::CreateStructureMap(StructureId::PendulumTower, kTileSize);
    tilemaps[StructureIndex(StructureId::GeothermalChamber)] =
        Tilemap::CreateStructureMap(StructureId::GeothermalChamber, kTileSize);

    spawn_points[StructureIndex(StructureId::HypogeanCisterns)] =
        GroundedTopLeftForTile(tilemaps[StructureIndex(StructureId::HypogeanCisterns)], 13, 6);
    spawn_points[StructureIndex(StructureId::PendulumTower)] =
        GroundedTopLeftForTile(tilemaps[StructureIndex(StructureId::PendulumTower)], 15, 6);
    spawn_points[StructureIndex(StructureId::GeothermalChamber)] =
        GroundedTopLeftForTile(tilemaps[StructureIndex(StructureId::GeothermalChamber)], 13, 6);

    active_structure = StructureId::HypogeanCisterns;
    tilemap = tilemaps[0];
    spawn_point = spawn_points[0];
    camera_target = {spawn_point.x + kPlayerWidth * 0.5f, spawn_point.y + kPlayerHeight * 0.5f};
    portal_cooldown_timer = 0.0f;
    portal_reentry_locked = false;
    initialized = true;
}

void StructureSim::SelectStructure(StructureId id) {
    if (!initialized) {
        InitializeDefault();
    }
    const std::size_t idx = StructureIndex(id);
    active_structure = static_cast<StructureId>(idx);
    tilemap = tilemaps[idx];
    spawn_point = spawn_points[idx];
}

void StructureSim::OnEnter(StructureId id, PlayerManager& players, bool bridge_unlocked) {
    if (!initialized) {
        InitializeDefault();
    }
    SelectStructure(id);
    OnEnter(players, bridge_unlocked);
}

void StructureSim::OnEnter(PlayerManager& players, bool bridge_unlocked) {
    if (!initialized) {
        InitializeDefault();
    }

    Vec2 anchor = spawn_point;
    if (!IsSafeSpawnBox(tilemap, anchor, {kPlayerWidth, kPlayerHeight}, bridge_unlocked)) {
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

    players.EnsureActive(0, anchor);

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
    overworld.InitializeDefault();
    structure.InitializeDefault();

    machines[MachineIndex(MachineType::SolarArray)] =
        Machine::CreateDefaultSolar(overworld.tilemap.TileCenter(29, 11));
    machines[MachineIndex(MachineType::HydroTurbine)] =
        Machine::CreateDefaultHydro(structure.tilemaps[StructureIndex(StructureId::HypogeanCisterns)].TileCenter(13, 13));
    machines[MachineIndex(MachineType::PendulumRegulator)] =
        Machine::CreateDefaultPendulum(structure.tilemaps[StructureIndex(StructureId::PendulumTower)].TileCenter(7, 15));
    machines[MachineIndex(MachineType::HarmonicResonator)] =
        Machine::CreateDefaultHarmonic(overworld.tilemap.TileCenter(17, 36));
    machines[MachineIndex(MachineType::ThermalExchanger)] =
        Machine::CreateDefaultThermal(structure.tilemaps[StructureIndex(StructureId::GeothermalChamber)].TileCenter(9, 26));
    machines[MachineIndex(MachineType::CentralAstrolabe)] =
        Machine::CreateDefaultAstrolabe(overworld.tilemap.TileCenter(29, 37));

    modal_machine_type = MachineType::SolarArray;
    calibration_modal.Reset();

    npcs[NpcIndex(NpcId::ClaraSereni)] = WorldNpc::CreateDefault(NpcId::ClaraSereni);
    npcs[NpcIndex(NpcId::LorenzoAltieri)] = WorldNpc::CreateDefault(NpcId::LorenzoAltieri);
    npcs[NpcIndex(NpcId::MiriamFerro)] = WorldNpc::CreateDefault(NpcId::MiriamFerro);
    npcs[NpcIndex(NpcId::MatteoVerrini)] = WorldNpc::CreateDefault(NpcId::MatteoVerrini);

    collectibles.ResetDefault();
    story.Reset();

    current_zone = ZoneId::DawnTerraces;
    zone_banner_timer = 0.0f;
    banner_zone = ZoneId::DawnTerraces;
    banner_structure = StructureId::HypogeanCisterns;
    banner_is_structure = false;

    demo_mode = keep_demo;
    last_scan = EmpiricalScanReading{};
    sim_time = 0.0f;
    scan_pulse_timer = 0.0f;
    scan_pulse_origin = {0.0f, 0.0f};
    simulation_completed = false;
    current_mode = GameMode::Overworld;
    active_structure = StructureId::HypogeanCisterns;
    returning_from_structure = false;
}

void WorldContext::EnsureInitialized() {
    if (!overworld.initialized || !structure.initialized || collectibles.TotalCount() == 0) {
        Reset();
    }
}

LocationId WorldContext::ActiveLocation() const noexcept {
    if (current_mode == GameMode::Overworld) {
        return LocationId::Overworld;
    }
    return LocationFromStructure(active_structure);
}

const char* WorldContext::ActiveLocationName() const noexcept {
    return LocationName(ActiveLocation(), current_zone);
}

const char* WorldContext::ActiveLocationSubtitle() const noexcept {
    if (current_mode == GameMode::Overworld) {
        return ZoneSubtitle(current_zone);
    }
    return StructureSubtitle(active_structure);
}

Machine& WorldContext::GetMachine(MachineType type) noexcept {
    return machines[MachineIndex(type)];
}

const Machine& WorldContext::GetMachine(MachineType type) const noexcept {
    return machines[MachineIndex(type)];
}

Machine& WorldContext::ModalTargetMachine() noexcept {
    return GetMachine(modal_machine_type);
}

const Machine& WorldContext::ModalTargetMachine() const noexcept {
    return GetMachine(modal_machine_type);
}

Machine& WorldContext::ActiveMachine() noexcept {
    const LocationId loc = ActiveLocation();
    const Player* p0 = players.GetPlayer(0);
    Machine* best = nullptr;
    float best_dist = 1e9f;
    for (auto& m : machines) {
        if (m.location != loc) {
            continue;
        }
        if (p0 != nullptr && p0->active) {
            const float d = m.DistanceToPlayer(*p0);
            if (best == nullptr || d < best_dist) {
                best = &m;
                best_dist = d;
            }
        } else if (best == nullptr) {
            best = &m;
        }
    }
    return (best != nullptr) ? *best : machines[0];
}

const Machine& WorldContext::ActiveMachine() const noexcept {
    const LocationId loc = ActiveLocation();
    const Player* p0 = players.GetPlayer(0);
    const Machine* best = nullptr;
    float best_dist = 1e9f;
    for (const auto& m : machines) {
        if (m.location != loc) {
            continue;
        }
        if (p0 != nullptr && p0->active) {
            const float d = m.DistanceToPlayer(*p0);
            if (best == nullptr || d < best_dist) {
                best = &m;
                best_dist = d;
            }
        } else if (best == nullptr) {
            best = &m;
        }
    }
    return (best != nullptr) ? *best : machines[0];
}

std::vector<const Machine*> WorldContext::MachinesInCurrentScene() const {
    std::vector<const Machine*> out;
    const LocationId loc = ActiveLocation();
    for (const auto& m : machines) {
        if (m.location == loc) {
            out.push_back(&m);
        }
    }
    return out;
}

WorldNpc& WorldContext::GetNpc(NpcId id) noexcept {
    return npcs[NpcIndex(id)];
}

const WorldNpc& WorldContext::GetNpc(NpcId id) const noexcept {
    return npcs[NpcIndex(id)];
}

std::vector<const WorldNpc*> WorldContext::NpcsInCurrentScene() const {
    std::vector<const WorldNpc*> out;
    const LocationId loc = ActiveLocation();
    for (const auto& npc : npcs) {
        if (npc.location == loc) {
            out.push_back(&npc);
        }
    }
    return out;
}

bool WorldContext::AllFiveMonumentsActivated() const noexcept {
    return GetMachine(MachineType::SolarArray).state != MachineState::Broken &&
           GetMachine(MachineType::HydroTurbine).state != MachineState::Broken &&
           GetMachine(MachineType::PendulumRegulator).state != MachineState::Broken &&
           GetMachine(MachineType::HarmonicResonator).state != MachineState::Broken &&
           GetMachine(MachineType::ThermalExchanger).state != MachineState::Broken;
}

int WorldContext::ActivatedMonumentsCount() const noexcept {
    int count = 0;
    for (const auto& m : machines) {
        if (m.state != MachineState::Broken) {
            ++count;
        }
    }
    return count;
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

void WorldContext::TriggerZoneBanner(ZoneId zone) noexcept {
    banner_zone = zone;
    banner_is_structure = false;
    zone_banner_timer = kZoneBannerDuration;
}

void WorldContext::TriggerStructureBanner(StructureId id) noexcept {
    banner_structure = id;
    banner_is_structure = true;
    zone_banner_timer = kZoneBannerDuration;
}

void WorldContext::EnterStructure(StructureId id) {
    EnsureInitialized();
    active_structure = id;
    current_mode = GameMode::Structure;
    returning_from_structure = false;
    overworld.last_used_structure = id;
    structure.OnEnter(id, players, ecosystem.architect_recruited);
    TriggerStructureBanner(id);
    UpdateProximityPrompts();
    story.Observe(BuildStorySnapshot());
}

void WorldContext::ExitToOverworld() {
    EnsureInitialized();
    const StructureId from_id = active_structure;
    current_mode = GameMode::Overworld;
    returning_from_structure = true;
    overworld.OnEnter(players, true, ecosystem.architect_recruited, from_id);
    if (const Player* p0 = players.GetPlayer(0); p0 != nullptr && p0->active) {
        current_zone = overworld.DistrictAt(p0->Center());
    }
    TriggerZoneBanner(current_zone);
    UpdateProximityPrompts();
    story.Observe(BuildStorySnapshot());
}

void WorldContext::TeleportToZone(ZoneId zone) {
    EnsureInitialized();
    current_mode = GameMode::Overworld;
    returning_from_structure = false;
    const Vec2 spawn = overworld.SafeSpawnInZone(zone, ecosystem.architect_recruited);
    players.EnsureActive(0, spawn);
    for (Player* p : players.ActivePlayers()) {
        p->pos = spawn;
        p->vel = {0.0f, 0.0f};
        p->ResetVerticalState();
    }
    overworld.camera_target = players.AverageActivePosition();
    current_zone = zone;
    TriggerZoneBanner(zone);
    UpdateProximityPrompts();
    story.Observe(BuildStorySnapshot());
}

StorySnapshot WorldContext::BuildStorySnapshot() const noexcept {
    StorySnapshot snap{};
    snap.mode = current_mode;
    snap.location = ActiveLocation();
    snap.zone = current_zone;
    snap.has_scanned = last_scan.active && last_scan.has_target;
    snap.solar = GetMachine(MachineType::SolarArray).state;
    snap.hydro = GetMachine(MachineType::HydroTurbine).state;
    snap.pendulum = GetMachine(MachineType::PendulumRegulator).state;
    snap.harmonic = GetMachine(MachineType::HarmonicResonator).state;
    snap.thermal = GetMachine(MachineType::ThermalExchanger).state;
    snap.astrolabe = GetMachine(MachineType::CentralAstrolabe).state;
    snap.clara_recruited = GetNpc(NpcId::ClaraSereni).recruited || ecosystem.architect_recruited;
    snap.lorenzo_recruited = GetNpc(NpcId::LorenzoAltieri).recruited;
    snap.miriam_met = GetNpc(NpcId::MiriamFerro).met;
    snap.matteo_confronted = GetNpc(NpcId::MatteoVerrini).met;
    snap.astrolabe_unlocked = GetMachine(MachineType::CentralAstrolabe).unlocked;
    snap.relics_collected = collectibles.CollectedCount();
    snap.total_relics = collectibles.TotalCount();
    snap.pollution_level = ecosystem.pollution_level;
    snap.simulation_completed = simulation_completed;
    return snap;
}

void WorldContext::UpdateProximityPrompts() noexcept {
    const LocationId loc = ActiveLocation();
    for (auto& m : machines) {
        if (m.location == loc) {
            m.UpdateProximity(players);
        } else {
            m.prompt_visible = false;
        }
    }
    for (auto& npc : npcs) {
        if (npc.location == loc) {
            npc.UpdateProximity(players);
        } else {
            npc.prompt_visible = false;
        }
    }
    collectibles.UpdateProximity(loc, players);
}

EmpiricalScanReading WorldContext::TriggerScan(int player_id) noexcept {
    const Player* p = players.GetPlayer(player_id);
    if (p == nullptr || !p->active) {
        last_scan = EmpiricalScanReading{};
        return last_scan;
    }

    scan_pulse_timer = 0.85f;
    scan_pulse_origin = p->Center();

    const LocationId loc = ActiveLocation();
    const Machine* best = nullptr;
    float best_dist = 1e9f;
    for (const auto& m : machines) {
        if (m.location != loc) {
            continue;
        }
        const float d = m.DistanceToPlayer(*p);
        if (d <= kDefaultScannerRange && d < best_dist) {
            best = &m;
            best_dist = d;
        }
    }

    if (best != nullptr) {
        last_scan = PerformEmpiricalScan(player_id, players, *best, kDefaultScannerRange);
    } else {
        last_scan = PerformEmpiricalScan(player_id, players, ActiveMachine(), kDefaultScannerRange);
    }
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
    if (zone_banner_timer > 0.0f) {
        zone_banner_timer = std::max(0.0f, zone_banner_timer - safe_dt);
    }
    collectibles.Tick(safe_dt);

    if (input.advance_dialogue_pressed) {
        story.AdvanceDialogue();
    }

    // 1. UI Phase cycling (Sguardo Empirico -> Rilievo Metrico -> Trama Geodetica)
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

    // 3. Calibration modal (freezes movement while open)
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
                break;
            }
            if (in.bypass_pressed && machine.allow_bypass) {
                calibration_modal.TriggerBypass(machine, ecosystem);
                break;
            }
            if (in.confirm_pressed) {
                if (calibration_modal.ConfirmClean(machine, ecosystem)) {
                    if (machine.type == MachineType::CentralAstrolabe) {
                        simulation_completed = true;
                    }
                }
                break;
            }
        }
        if (AllFiveMonumentsActivated()) {
            GetMachine(MachineType::CentralAstrolabe).unlocked = true;
        }
        UpdateProximityPrompts();
        story.Observe(BuildStorySnapshot());
        story.Tick(safe_dt);
        return out;
    }

    // 4. Step active scene physics
    if (current_mode == GameMode::Overworld) {
        const OverworldStepResult ow_res =
            overworld.Step(players, input, dt, ecosystem.architect_recruited);
        out.enter_structure_requested = ow_res.enter_structure_requested;
        out.target_structure = ow_res.target_structure;
        out.newly_joined_count = ow_res.newly_joined_count;
        if (ow_res.enter_structure_requested) {
            active_structure = ow_res.target_structure;
        }
        // Detect Overworld district transitions
        if (const Player* p0 = players.GetPlayer(0); p0 != nullptr && p0->active) {
            const ZoneId next_zone = overworld.DistrictAt(p0->Center());
            if (next_zone != current_zone) {
                current_zone = next_zone;
                out.zone_changed = true;
                out.new_zone = next_zone;
                TriggerZoneBanner(next_zone);
            }
        }
    } else {
        const StructureStepResult st_res =
            structure.Step(players, input, dt, ecosystem.architect_recruited);
        out.exit_to_overworld_requested = st_res.exit_to_overworld_requested;
        out.newly_joined_count = st_res.newly_joined_count;
    }

    // 5. Update proximity prompts
    UpdateProximityPrompts();

    // 6. Check if any active player pressed [E] interact
    bool any_interact = false;
    for (const Player* p : players.ActivePlayers()) {
        if (p != nullptr && input.Get(p->player_id).interact_pressed) {
            any_interact = true;
            break;
        }
    }

    // 7. Collectibles pickup (auto-pickup on close touch OR [E] within interact radius)
    if (const auto picked = collectibles.TryPickupNear(ActiveLocation(), players, any_interact);
        picked.has_value()) {
        out.collectible_picked_up = true;
        out.picked_up_id = *picked;
        const CollectibleMeta& meta = CollectibleMetadata(*picked);
        if (!meta.pickup_line.empty()) {
            story.PushAdHocLine(
                meta.pickup_speaker.empty() ? "Elena Valli" : meta.pickup_speaker,
                meta.pickup_line,
                current_zone,
                ActiveLocationName());
        }
    }

    // 8. Unlock Central Astrolabe once all 5 district/structure monuments are activated
    if (AllFiveMonumentsActivated()) {
        GetMachine(MachineType::CentralAstrolabe).unlocked = true;
    }

    // 9. Handle [E] interactions with Machines & NPCs
    const LocationId loc = ActiveLocation();
    for (const Player* p : players.ActivePlayers()) {
        if (p == nullptr || !input.Get(p->player_id).interact_pressed) {
            continue;
        }

        // 9a. Machines in current scene
        bool handled = false;
        for (auto& m : machines) {
            if (m.location != loc || !m.IsPlayerInRange(*p)) {
                continue;
            }
            if (!m.unlocked) {
                story.PushAdHocLine(
                    "Elena Valli",
                    "L'Astrolabio Centrale e' ancora sigillato: occorre riattivare prima i cinque monumenti della Conca.",
                    current_zone,
                    ActiveLocationName());
                handled = true;
                break;
            }
            modal_machine_type = m.type;
            calibration_modal.OpenFor(m);
            handled = true;
            break;
        }
        if (handled) {
            break;
        }

        // 9b. NPCs in current scene
        for (auto& npc : npcs) {
            if (npc.location != loc || !npc.IsPlayerInRange(*p)) {
                continue;
            }
            npc.met = true;
            if (npc.id == NpcId::ClaraSereni) {
                if (npc.recruited || ecosystem.architect_recruited) {
                    npc.recruited = true;
                    npc.hidden_bridge_revealed = true;
                    ecosystem.SetArchitectRecruited(true);
                    npc.last_dialogue =
                        "Il Ponte degli Archi e' libero e la Trama Geodetica mostra le linee di forza della Conca.";
                    story.PushAdHocLine(npc.name, npc.last_dialogue, current_zone, ActiveLocationName());
                } else if (ecosystem.clean_energy > 0.0f) {
                    npc.recruited = true;
                    npc.hidden_bridge_revealed = true;
                    ecosystem.SetArchitectRecruited(true);
                    npc.last_dialogue =
                        "Calibrazione verificata senza pece. Apro il Ponte degli Archi e condivido con te la Trama Geodetica.";
                } else if (ecosystem.dirty_energy > 0.0f) {
                    npc.last_dialogue =
                        "Non apro il Ponte degli Archi per fumo di fornace. Riaccendi un monumento con la fisica pulita.";
                    story.PushAdHocLine(npc.name, npc.last_dialogue, current_zone, ActiveLocationName());
                } else {
                    npc.last_dialogue =
                        "Prima di aprire il Ponte degli Archi calibra l'Eliostato sulle Terrazze dell'Alba.";
                    story.PushAdHocLine(npc.name, npc.last_dialogue, current_zone, ActiveLocationName());
                }
            } else if (npc.id == NpcId::LorenzoAltieri) {
                if (GetMachine(MachineType::HydroTurbine).state != MachineState::Broken) {
                    const bool was_recruited = npc.recruited;
                    npc.recruited = true;
                    npc.last_dialogue =
                        "L'acqua torna a cantare sotto le volte. Cerca le reliquie nelle quattro ere: la Conca ricorda tutto.";
                    if (was_recruited) {
                        story.PushAdHocLine(npc.name, npc.last_dialogue, current_zone, ActiveLocationName());
                    }
                } else {
                    npc.last_dialogue =
                        "Riavvia prima la Turbina Idrostatica qui sotto: solo allora ti consegniero' le mappe dell'archivio.";
                    story.PushAdHocLine(npc.name, npc.last_dialogue, current_zone, ActiveLocationName());
                }
            } else if (npc.id == NpcId::MiriamFerro) {
                if (GetMachine(MachineType::HarmonicResonator).state != MachineState::Broken) {
                    npc.recruited = true;
                    npc.last_dialogue =
                        "Senti il tremito della giada? I semi di Meridia stanno gia' rompendo la crosta di cenere.";
                } else {
                    npc.last_dialogue =
                        "Il Risonatore Armonico qui vicino attende i suoi 120 Hz. Accordalo e i Giardini Sospesi rinasceranno.";
                }
                if (story.EventFired(StoryEvent::MiriamMet)) {
                    story.PushAdHocLine(npc.name, npc.last_dialogue, current_zone, ActiveLocationName());
                }
            } else if (npc.id == NpcId::MatteoVerrini) {
                if (GetMachine(MachineType::ThermalExchanger).state == MachineState::CleanActive) {
                    npc.recruited = true;
                    npc.last_dialogue =
                        "Hai tenuto calda la valle senza bruciare una goccia di pece. Forse avevo torto a temere il tempo.";
                } else {
                    npc.last_dialogue =
                        "La Camera Geotermica e' qui dietro. Dimostra che Carnot regge il carico invernale e spegnero' le fornaci.";
                }
                if (story.EventFired(StoryEvent::MatteoConfronted)) {
                    story.PushAdHocLine(npc.name, npc.last_dialogue, current_zone, ActiveLocationName());
                }
            }
            handled = true;
            break;
        }
        if (handled) {
            break;
        }
    }

    if (GetMachine(MachineType::CentralAstrolabe).state != MachineState::Broken) {
        simulation_completed = true;
    }

    story.Observe(BuildStorySnapshot());
    story.Tick(safe_dt);

    return out;
}

} // namespace eco
