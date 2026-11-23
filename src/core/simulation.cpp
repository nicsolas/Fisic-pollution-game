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
    bool bridge_unlocked,
    bool solar_gate_open = false,
    bool jade_bridge_unlocked = false) noexcept {
    if (!IsFiniteVec2(top_left) || !IsFiniteVec2(entity_size) ||
        entity_size.x <= 0.0f || entity_size.y <= 0.0f) {
        return false;
    }
    const Rect box{top_left.x, top_left.y, entity_size.x, entity_size.y};
    return !tilemap.OverlapsSolid(box, bridge_unlocked, solar_gate_open, jade_bridge_unlocked) &&
           !tilemap.OverlapsTileId(box, TileId::StructurePortal) &&
           !tilemap.OverlapsTileId(box, TileId::ExitPortal) &&
           !tilemap.OverlapsTileId(box, TileId::ZoneGateway);
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

[[nodiscard]] Vec2 ZoneTileCenter(int row, int col, float tile_size = kTileSize) noexcept {
    return {
        (static_cast<float>(col) + 0.5f) * tile_size,
        (static_cast<float>(row) + 0.5f) * tile_size
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

[[nodiscard]] Rect ZoneGatewayRect(const Tilemap& tilemap, const ZoneGateway& gw) noexcept {
    const Rect rmin = tilemap.TileRect(gw.tile_min.row, gw.tile_min.col);
    const Rect rmax = tilemap.TileRect(gw.tile_max.row, gw.tile_max.col);
    return {
        rmin.Left(),
        rmin.Top(),
        rmax.Right() - rmin.Left(),
        rmax.Bottom() - rmin.Top()
    };
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

[[nodiscard]] Vec2 DirectionFromIndex(int dir_index) noexcept {
    switch ((dir_index % 4 + 4) % 4) {
        case 0:
            return {1.0f, 0.0f};  // Est
        case 1:
            return {0.0f, 1.0f};  // Sud
        case 2:
            return {-1.0f, 0.0f}; // Ovest
        case 3:
            return {0.0f, -1.0f}; // Nord
        default:
            return {1.0f, 0.0f};
    }
}

[[nodiscard]] float PointToSegmentDistSq(Vec2 p, Vec2 a, Vec2 b) noexcept {
    const Vec2 ab = b - a;
    const float len_sq = ab.LengthSquared();
    if (len_sq <= kEpsilon) {
        return (p - a).LengthSquared();
    }
    const float t = Clamp((p - a).Dot(ab) / len_sq, 0.0f, 1.0f);
    const Vec2 proj = a + ab * t;
    return (p - proj).LengthSquared();
}

[[nodiscard]] Vec2 StructureSpawnFor(const Tilemap& map, StructureId id) noexcept {
    const StructureSpawnLayout layout = GetStructureLayout(id);
    return GroundedTopLeftForTile(map, layout.spawn_tile.row, layout.spawn_tile.col);
}

} // namespace

// ============================================================================
// ZoneMechanicState (6 Meccaniche Top-Down Uniche)
// ============================================================================

void ZoneMechanicState::ResetDefaults() {
    // 1. Terrazze dell'Alba: Specchi Solari e Piastre Fotosensibili
    solar_emitter_pos = ZoneTileCenter(6, 6);
    solar_emitter_dir = {1.0f, 0.0f}; // Est verso specchio 0 a (6, 14)
    solar_mirrors = {
        SolarMirror{0, ZoneTileCenter(6, 14), 0, true, 64.0f},  // ruota a Sud (1) per colpire specchio 1
        SolarMirror{1, ZoneTileCenter(14, 14), 2, true, 64.0f}, // ruota a Est (0) per colpire specchio 2
        SolarMirror{2, ZoneTileCenter(14, 24), 0, true, 64.0f}  // ruota a Nord (3) o Sud (1) per le piastre
    };
    solar_plates = {
        SolarPlate{0, ZoneTileCenter(6, 24), 30.0f, 0.0f, false, "Lastra dell'Equinozio: orienta il fascio a Nord."},
        SolarPlate{1, ZoneTileCenter(20, 24), 30.0f, 0.0f, false, "Lastra del Solstizio: completa l'arco solare."}
    };
    solar_beams.clear();
    solar_gate_open = false;

    // 2. Quartiere dei Canali: Correnti Direzionali & Chiuse/Leve
    water_currents = {
        WaterCurrentZone{
            Rect{4.0f * kTileSize, 11.0f * kTileSize, 11.0f * kTileSize, 3.0f * kTileSize},
            {-1.0f, 0.0f},
            155.0f,
            0,
            true
        },
        WaterCurrentZone{
            Rect{20.0f * kTileSize, 11.0f * kTileSize, 8.0f * kTileSize, 3.0f * kTileSize},
            {1.0f, 0.0f},
            145.0f,
            0,
            true
        },
        WaterCurrentZone{
            Rect{10.0f * kTileSize, 18.0f * kTileSize, 18.0f * kTileSize, 3.0f * kTileSize},
            {-1.0f, 0.0f},
            165.0f,
            1,
            true
        }
    };
    sluice_levers = {
        SluiceLever{0, ZoneTileCenter(10, 13), 0, false, 64.0f},
        SluiceLever{1, ZoneTileCenter(17, 23), 1, false, 64.0f}
    };

    // 3. Altopiano dei Venti: Raffiche Periodiche & 5 Monoliti Frangivento
    wind = WindState{};
    wind.cycle_timer = 0.0f;
    wind.cycle_period = 6.0f;
    wind.wind_dir = Vec2{-1.0f, 0.12f}.Normalized();
    wind.max_push_speed = 185.0f;

    auto make_monolith = [](int r0, int r1, int col) noexcept -> WindbreakMonolith {
        Rect r{
            static_cast<float>(col) * kTileSize,
            static_cast<float>(r0) * kTileSize,
            kTileSize,
            static_cast<float>(r1 - r0 + 1) * kTileSize
        };
        return WindbreakMonolith{r, r.Center(), 144.0f};
    };
    windbreaks = {
        make_monolith(18, 20, 21),
        make_monolith(15, 17, 13),
        make_monolith(11, 13, 22),
        make_monolith(8, 10, 12),
        make_monolith(11, 13, 27)
    };

    // 4. Giardini Sospesi: 4 Piastre di Risonanza Armonica (Accordo di Do Maggiore)
    resonance_pads = {
        ResonancePad{0, ZoneTileCenter(9, 12), 34.0f, 0, 261.63f, false, 0.0f, false},
        ResonancePad{1, ZoneTileCenter(9, 16), 34.0f, 1, 329.63f, false, 0.0f, false},
        ResonancePad{2, ZoneTileCenter(16, 16), 34.0f, 2, 392.00f, false, 0.0f, false},
        ResonancePad{3, ZoneTileCenter(16, 12), 34.0f, 3, 523.25f, false, 0.0f, false}
    };
    resonance_next_order = 0;
    jade_bridge_unlocked = false;
    jade_bridge_progress = 0.0f;

    // 5. Caldera delle Fumarole: 6 Geyser Termici Ciclici & 2 Valvole di Sfogo
    geysers = {
        ThermalGeyser{0, ZoneTileCenter(18, 16), 42.0f, 0.0f, 4.6f, 0.0f, 0.0f, GeyserPhase::Dormant, 0},
        ThermalGeyser{1, ZoneTileCenter(15, 14), 42.0f, 1.5f, 4.6f, 0.0f, 0.0f, GeyserPhase::Dormant, 0},
        ThermalGeyser{2, ZoneTileCenter(15, 20), 42.0f, 2.8f, 4.6f, 0.0f, 0.0f, GeyserPhase::Dormant, 0},
        ThermalGeyser{3, ZoneTileCenter(11, 17), 42.0f, 0.8f, 4.6f, 0.0f, 0.0f, GeyserPhase::Dormant, 1},
        ThermalGeyser{4, ZoneTileCenter(10, 23), 42.0f, 2.1f, 4.6f, 0.0f, 0.0f, GeyserPhase::Dormant, 1},
        ThermalGeyser{5, ZoneTileCenter(9, 13), 42.0f, 3.4f, 4.6f, 0.0f, 0.0f, GeyserPhase::Dormant, 1}
    };
    relief_valves = {
        ReliefValve{0, ZoneTileCenter(15, 8), 0, false, 64.0f},
        ReliefValve{1, ZoneTileCenter(11, 27), 1, false, 64.0f}
    };

    // 6. Santuario dell'Armonia: 5 Sigilli Elementali disposti a pentagono attorno all'Astrolabio (13, 18)
    const Vec2 center = ZoneTileCenter(13, 18);
    constexpr float kSealRadius = 216.0f;
    static constexpr std::array<ZoneId, 5> kSealZones{{
        ZoneId::DawnTerraces,
        ZoneId::CanalQuarter,
        ZoneId::WindPlateau,
        ZoneId::HangingGardens,
        ZoneId::FumaroleCaldera
    }};
    sanctuary_seals.clear();
    sanctuary_seals.reserve(5);
    for (int i = 0; i < 5; ++i) {
        const float angle = -kPi * 0.5f + static_cast<float>(i) * (2.0f * kPi / 5.0f);
        const Vec2 pos{
            center.x + std::cos(angle) * kSealRadius,
            center.y + std::sin(angle) * kSealRadius
        };
        sanctuary_seals.push_back(SanctuarySeal{i, kSealZones[static_cast<std::size_t>(i)], pos, 40.0f, false, 0.0f});
    }
}

void ZoneMechanicState::RecomputeSolarBeams(const Tilemap& dawn_map) noexcept {
    solar_beams.clear();
    if (dawn_map.Empty()) {
        return;
    }

    for (auto& plate : solar_plates) {
        plate.lit = false;
    }

    Vec2 origin = solar_emitter_pos;
    Vec2 dir = solar_emitter_dir.Normalized();
    if (dir.LengthSquared() <= kEpsilon) {
        dir = {1.0f, 0.0f};
    }

    constexpr int kMaxBounces = 6;
    constexpr float kStepPx = 8.0f;
    constexpr float kMaxDistPx = 1800.0f;
    constexpr float kMirrorHitRadiusSq = 22.0f * 22.0f;

    int last_mirror_id = -1;
    for (int bounce = 0; bounce < kMaxBounces; ++bounce) {
        Vec2 cur = origin;
        bool hit_mirror = false;
        int hit_mirror_idx = -1;
        float traveled = 0.0f;

        while (traveled < kMaxDistPx) {
            cur += dir * kStepPx;
            traveled += kStepPx;

            const TileCoord tc = dawn_map.WorldToTile(cur);
            if (!dawn_map.InBounds(tc.row, tc.col)) {
                break;
            }
            const TileId tid = dawn_map.GetTile(tc.row, tc.col);
            if (tid == TileId::Wall || tid == TileId::RuinsWall) {
                break;
            }

            // Controlla impatto con uno specchio (diverso dall'ultimo rimbalzo)
            for (std::size_t i = 0; i < solar_mirrors.size(); ++i) {
                if (solar_mirrors[i].id == last_mirror_id) {
                    continue;
                }
                if ((cur - solar_mirrors[i].pos).LengthSquared() <= kMirrorHitRadiusSq) {
                    cur = solar_mirrors[i].pos;
                    hit_mirror = true;
                    hit_mirror_idx = static_cast<int>(i);
                    break;
                }
            }
            if (hit_mirror) {
                break;
            }
        }

        solar_beams.push_back(SolarBeamSegment{origin, cur, true});

        // Verifica piastre fotosensibili illuminate da questo segmento
        for (auto& plate : solar_plates) {
            const float r_sq = plate.radius * plate.radius;
            if (PointToSegmentDistSq(plate.pos, origin, cur) <= r_sq) {
                plate.lit = true;
                plate.charge = 1.0f;
            }
        }

        if (!hit_mirror || hit_mirror_idx < 0) {
            break;
        }
        const SolarMirror& m = solar_mirrors[static_cast<std::size_t>(hit_mirror_idx)];
        last_mirror_id = m.id;
        origin = m.pos;
        dir = DirectionFromIndex(m.dir_index);
    }

    // Se almeno una piastra e' illuminata e tutte hanno charge >= 1.0 (o almeno tutte le piastre sono state caricate)
    bool all_charged = !solar_plates.empty();
    for (const auto& plate : solar_plates) {
        if (plate.charge < 0.99f) {
            all_charged = false;
            break;
        }
    }
    if (all_charged) {
        solar_gate_open = true;
    }
}

int ZoneMechanicState::AttunedSealsCount() const noexcept {
    int count = 0;
    for (const auto& seal : sanctuary_seals) {
        if (seal.attuned) {
            ++count;
        }
    }
    return count;
}

bool ZoneMechanicState::AllSealsAttuned() const noexcept {
    return !sanctuary_seals.empty() &&
           AttunedSealsCount() == static_cast<int>(sanctuary_seals.size());
}

bool ZoneMechanicState::IsWindSheltered(Vec2 point) const noexcept {
    const Vec2 dir = wind.wind_dir.Normalized();
    if (dir.LengthSquared() <= kEpsilon) {
        return true;
    }
    const Vec2 perp{-dir.y, dir.x};
    for (const auto& m : windbreaks) {
        const Vec2 rel = point - m.center;
        const float along = rel.Dot(dir);
        const float across = std::fabs(rel.Dot(perp));
        const float half_span = std::max(m.bounds.width, m.bounds.height) * 0.55f + 16.0f;
        if (along >= 0.0f && along <= m.shelter_distance && across <= half_span) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// FindSafeSpawnNear
// ============================================================================

Vec2 FindSafeSpawnNear(
    const Tilemap& tilemap,
    Vec2 anchor_pos,
    Vec2 entity_size,
    Vec2 fallback_pos,
    bool bridge_unlocked,
    bool solar_gate_open,
    bool jade_bridge_unlocked) noexcept {
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
            if (IsSafeSpawnBox(tilemap, candidate, entity_size, bridge_unlocked, solar_gate_open, jade_bridge_unlocked)) {
                return candidate;
            }
        }
    }

    if (IsFiniteVec2(fallback_pos)) {
        for (const Vec2& offset : kOffsets) {
            const Vec2 candidate{fallback_pos.x + offset.x, fallback_pos.y + offset.y};
            if (IsSafeSpawnBox(tilemap, candidate, entity_size, bridge_unlocked, solar_gate_open, jade_bridge_unlocked)) {
                return candidate;
            }
        }
    }

    if (IsFiniteVec2(anchor_pos)) {
        return anchor_pos;
    }
    return IsFiniteVec2(fallback_pos) ? fallback_pos : Vec2{0.0f, 0.0f};
}

// ============================================================================
// OverworldSim (Singola Zona Residente in RAM + Swap Pulito)
// ============================================================================

ZoneId OverworldSim::PortalZone(StructureId id) noexcept {
    switch (id) {
        case StructureId::HypogeanCisterns:
            return ZoneId::CanalQuarter;
        case StructureId::PendulumTower:
            return ZoneId::WindPlateau;
        case StructureId::GeothermalChamber:
            return ZoneId::FumaroleCaldera;
        case StructureId::Count:
            break;
    }
    return ZoneId::CanalQuarter;
}

void OverworldSim::InitializeDefault() {
    gateways = Tilemap::OverworldGateways(kTileSize);
    zone_swaps_count = 0;

    // Catalogo portali (coordinate locali nella rispettiva zona 26x36)
    portals.clear();
    portals.reserve(static_cast<std::size_t>(StructureId::Count));
    {
        PortalLink link{};
        link.target_structure = StructureId::HypogeanCisterns;
        link.source_zone = ZoneId::CanalQuarter;
        link.tile_min = {6, 7};
        link.tile_max = {6, 8};
        link.world_center = {(7.5f + 0.5f) * kTileSize, 6.5f * kTileSize};
        link.exit_spawn = {
            ZoneTileCenter(8, 7).x - kPlayerWidth * 0.5f,
            ZoneTileCenter(8, 7).y - kPlayerHeight * 0.5f
        };
        portals.push_back(link);
    }
    {
        PortalLink link{};
        link.target_structure = StructureId::PendulumTower;
        link.source_zone = ZoneId::WindPlateau;
        link.tile_min = {5, 17};
        link.tile_max = {5, 18};
        link.world_center = {18.0f * kTileSize, 5.5f * kTileSize};
        link.exit_spawn = {
            ZoneTileCenter(7, 17).x - kPlayerWidth * 0.5f,
            ZoneTileCenter(7, 17).y - kPlayerHeight * 0.5f
        };
        portals.push_back(link);
    }
    {
        PortalLink link{};
        link.target_structure = StructureId::GeothermalChamber;
        link.source_zone = ZoneId::FumaroleCaldera;
        link.tile_min = {5, 17};
        link.tile_max = {5, 18};
        link.world_center = {18.0f * kTileSize, 5.5f * kTileSize};
        link.exit_spawn = {
            ZoneTileCenter(7, 17).x - kPlayerWidth * 0.5f,
            ZoneTileCenter(7, 17).y - kPlayerHeight * 0.5f
        };
        portals.push_back(link);
    }

    last_used_structure = StructureId::HypogeanCisterns;
    portal_exit_spawn = portals[0].exit_spawn;
    portal_cooldown_timer = 0.0f;
    portal_reentry_locked = false;
    gateway_cooldown_timer = 0.0f;
    gateway_reentry_locked = false;

    // Carica SOLO la zona iniziale (DawnTerraces) in RAM
    active_zone = ZoneId::DawnTerraces;
    tilemap = Tilemap::CreateZoneMap(active_zone, kTileSize);
    active_gateways = Tilemap::ZoneGateways(active_zone, kTileSize);
    const TileCoord def_spawn = Tilemap::DefaultZoneSpawnTile(active_zone);
    spawn_point = CenteredTopLeftForTile(tilemap, def_spawn.row, def_spawn.col);
    camera_target = {spawn_point.x + kPlayerWidth * 0.5f, spawn_point.y + kPlayerHeight * 0.5f};
    initialized = true;
}

void OverworldSim::UnloadZoneMemory() noexcept {
    tilemap.ReleaseMemory();
    active_gateways.clear();
    active_gateways.shrink_to_fit();
}

void OverworldSim::LoadZone(ZoneId zone, std::optional<TileCoord> arrival_tile) {
    const bool changing_zone = (!tilemap.Empty() && zone != active_zone);
    // Libera fisicamente la memoria della zona precedente prima di allocare la nuova
    tilemap.ReleaseMemory();
    active_zone = zone;
    tilemap = Tilemap::CreateZoneMap(zone, kTileSize);
    active_gateways = Tilemap::ZoneGateways(zone, kTileSize);

    const TileCoord target_tile = arrival_tile.value_or(Tilemap::DefaultZoneSpawnTile(zone));
    spawn_point = CenteredTopLeftForTile(tilemap, target_tile.row, target_tile.col);
    camera_target = {spawn_point.x + kPlayerWidth * 0.5f, spawn_point.y + kPlayerHeight * 0.5f};
    gateway_cooldown_timer = kPortalCooldownSeconds;
    gateway_reentry_locked = true;

    if (changing_zone) {
        ++zone_swaps_count;
    }
    initialized = true;
}

ZoneId OverworldSim::DistrictAt(Vec2 world_pos) const noexcept {
    (void)world_pos;
    return active_zone;
}

ZoneId OverworldSim::DistrictAtTile(int row, int col) const noexcept {
    (void)row;
    (void)col;
    return active_zone;
}

Rect OverworldSim::DistrictBounds(ZoneId zone) const noexcept {
    (void)zone;
    return {0.0f, 0.0f, tilemap.WorldWidth(), tilemap.WorldHeight()};
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
    const TileCoord target = Tilemap::DefaultZoneSpawnTile(zone);
    if (zone == active_zone && !tilemap.Empty()) {
        const Vec2 anchor = CenteredTopLeftForTile(tilemap, target.row, target.col);
        return FindSafeSpawnNear(tilemap, anchor, {kPlayerWidth, kPlayerHeight}, spawn_point, bridge_unlocked);
    }
    const Vec2 center = ZoneTileCenter(target.row, target.col, kTileSize);
    return {center.x - kPlayerWidth * 0.5f, center.y - kPlayerHeight * 0.5f};
}

Vec2 OverworldSim::SafeSpawnAtTile(
    TileCoord tile,
    bool bridge_unlocked,
    bool solar_gate_open,
    bool jade_bridge_unlocked) const noexcept {
    const Vec2 anchor = CenteredTopLeftForTile(tilemap, tile.row, tile.col);
    return FindSafeSpawnNear(
        tilemap,
        anchor,
        {kPlayerWidth, kPlayerHeight},
        spawn_point,
        bridge_unlocked,
        solar_gate_open,
        jade_bridge_unlocked);
}

void OverworldSim::OnEnter(
    PlayerManager& players,
    bool returning_from_structure,
    bool bridge_unlocked,
    StructureId from_structure) {
    if (!initialized) {
        InitializeDefault();
    }

    if (returning_from_structure) {
        const ZoneId target_zone = PortalZone(from_structure);
        if (tilemap.Empty() || active_zone != target_zone) {
            LoadZone(target_zone);
        }
    } else if (tilemap.Empty()) {
        LoadZone(active_zone);
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

    static constexpr std::array<Vec2, kMaxPlayers> kFormationOffsets{{
        {0.0f, 0.0f},
        {36.0f, 0.0f},
        {-36.0f, 0.0f},
        {0.0f, 36.0f}
    }};

    int slot_idx = 0;
    for (Player* p : players.ActivePlayers()) {
        const Vec2 offset = kFormationOffsets[static_cast<std::size_t>(slot_idx % kMaxPlayers)];
        const Vec2 desired{anchor.x + offset.x, anchor.y + offset.y};
        p->pos = FindSafeSpawnNear(tilemap, desired, p->size, anchor, bridge_unlocked);
        p->vel = {0.0f, 0.0f};
        p->ResetVerticalState();
        ++slot_idx;
    }

    portal_cooldown_timer = returning_from_structure ? kPortalCooldownSeconds : 0.0f;
    portal_reentry_locked = returning_from_structure;
    gateway_cooldown_timer = kPortalCooldownSeconds;
    gateway_reentry_locked = false;

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
    bool bridge_unlocked,
    bool solar_gate_open,
    bool jade_bridge_unlocked,
    Vec2 external_velocity,
    bool forward_gateway_unlocked) {
    if (!initialized || tilemap.Empty()) {
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
        p->pos = FindSafeSpawnNear(
            tilemap, drop_anchor, p->size, spawn_point,
            bridge_unlocked, solar_gate_open, jade_bridge_unlocked);
        p->vel = {0.0f, 0.0f};
        p->ResetVerticalState();
    }

    const float safe_dt = (std::isfinite(dt) && dt > 0.0f) ? dt : 0.0f;

    for (Player* p : players.ActivePlayers()) {
        const PlayerInput& in = input.Get(p->player_id);
        MovePlayerTopDown(
            *p,
            {in.move_x, in.move_y},
            safe_dt,
            tilemap,
            bridge_unlocked,
            kTopDownSpeed,
            external_velocity,
            solar_gate_open,
            jade_bridge_unlocked);
    }

    camera_target = UpdateAverageCameraTarget(camera_target, players, safe_dt, kCameraLerpSpeed);

    if (portal_cooldown_timer > 0.0f) {
        portal_cooldown_timer = std::max(0.0f, portal_cooldown_timer - safe_dt);
    }
    if (gateway_cooldown_timer > 0.0f) {
        gateway_cooldown_timer = std::max(0.0f, gateway_cooldown_timer - safe_dt);
    }

    // 1. Verifica portali verso strutture interne nella zona corrente
    bool any_on_portal = false;
    StructureId matched_structure = last_used_structure;
    for (const Player* p : players.ActivePlayers()) {
        const Rect bounds = p->Bounds();
        if (!tilemap.OverlapsTileId(bounds, TileId::StructurePortal)) {
            continue;
        }
        any_on_portal = true;
        for (const auto& link : portals) {
            if (link.source_zone == active_zone && bounds.Overlaps(PortalLinkRect(tilemap, link))) {
                matched_structure = link.target_structure;
                break;
            }
        }
        break;
    }

    if (portal_reentry_locked) {
        if (portal_cooldown_timer <= 0.0f && !any_on_portal) {
            portal_reentry_locked = false;
        }
    } else if (portal_cooldown_timer <= 0.0f && any_on_portal) {
        result.enter_structure_requested = true;
        result.target_structure = matched_structure;
        last_used_structure = matched_structure;
        if (const PortalLink* link = FindPortal(matched_structure); link != nullptr) {
            portal_exit_spawn = link->exit_spawn;
        }
        return result;
    }

    // 2. Verifica varchi verso zone adiacenti (ZoneGateway)
    bool any_on_gateway = false;
    const ZoneGateway* matched_gw = nullptr;
    for (const Player* p : players.ActivePlayers()) {
        const Rect bounds = p->Bounds();
        if (!tilemap.OverlapsTileId(bounds, TileId::ZoneGateway)) {
            continue;
        }
        for (const auto& gw : active_gateways) {
            const bool is_forward = static_cast<int>(gw.to_zone) > static_cast<int>(gw.from_zone);
            const bool gw_locked = (gw.requires_bridge_unlock && !bridge_unlocked) ||
                                   (is_forward && !forward_gateway_unlocked);
            if (bounds.Overlaps(ZoneGatewayRect(tilemap, gw))) {
                if (gw_locked) {
                    result.blocked_gateway_touched = true;
                    result.blocked_gateway_target = gw.to_zone;
                    continue;
                }
                any_on_gateway = true;
                matched_gw = &gw;
                break;
            }
        }
        if (any_on_gateway) {
            break;
        }
    }

    if (gateway_reentry_locked) {
        if (gateway_cooldown_timer <= 0.0f && !any_on_gateway) {
            gateway_reentry_locked = false;
        }
    } else if (gateway_cooldown_timer <= 0.0f && any_on_gateway && matched_gw != nullptr) {
        result.zone_gateway_triggered = true;
        result.gateway_target_zone = matched_gw->to_zone;
        result.gateway_arrival_tile = matched_gw->arrival_spawn_tile;
    }

    return result;
}

// ============================================================================
// StructureSim (Caricamento On-Demand Singola Struttura)
// ============================================================================

void StructureSim::InitializeDefault() {
    active_structure = StructureId::HypogeanCisterns;
    tilemap.ReleaseMemory();
    for (std::size_t i = 0; i < saved_puzzles.size(); ++i) {
        saved_puzzles[i].ResetForStructure(static_cast<StructureId>(i), kTileSize);
    }
    puzzle = saved_puzzles[0];
    spawn_point = {0.0f, 0.0f};
    camera_target = {0.0f, 0.0f};
    portal_cooldown_timer = 0.0f;
    portal_reentry_locked = false;
    initialized = true;
}

void StructureSim::SyncActivePuzzleToSaved() noexcept {
    if (!initialized || puzzle.switches.empty()) {
        return;
    }
    const int idx = static_cast<int>(puzzle.structure_id);
    if (idx >= 0 && idx < static_cast<int>(saved_puzzles.size())) {
        saved_puzzles[static_cast<std::size_t>(idx)] = puzzle;
    }
}

void StructureSim::SelectStructure(StructureId id) {
    if (!initialized) {
        InitializeDefault();
    } else {
        SyncActivePuzzleToSaved();
    }
    tilemap.ReleaseMemory();
    active_structure = id;
    tilemap = Tilemap::CreateStructureMap(id, kTileSize);

    const int idx = std::clamp(
        static_cast<int>(id),
        0,
        static_cast<int>(saved_puzzles.size()) - 1);
    StructurePuzzleState& stored = saved_puzzles[static_cast<std::size_t>(idx)];
    if (stored.switches.empty() || stored.structure_id != id) {
        stored.ResetForStructure(id, kTileSize);
    }
    puzzle = stored;
    puzzle.EvaluateGates(tilemap);
    stored = puzzle;

    spawn_point = StructureSpawnFor(tilemap, id);
    camera_target = {spawn_point.x + kPlayerWidth * 0.5f, spawn_point.y + kPlayerHeight * 0.5f};
}

void StructureSim::UnloadMemory() noexcept {
    SyncActivePuzzleToSaved();
    tilemap.ReleaseMemory();
}

void StructureSim::OnEnter(PlayerManager& players, bool bridge_unlocked) {
    OnEnter(active_structure, players, bridge_unlocked);
}

void StructureSim::OnEnter(StructureId id, PlayerManager& players, bool bridge_unlocked) {
    if (!initialized) {
        InitializeDefault();
    }
    SelectStructure(id);

    Vec2 anchor = spawn_point;
    if (!IsSafeSpawnBox(tilemap, anchor, {kPlayerWidth, kPlayerHeight}, bridge_unlocked)) {
        anchor = FindSafeSpawnNear(
            tilemap, anchor, {kPlayerWidth, kPlayerHeight}, spawn_point, bridge_unlocked);
    }

    players.EnsureActive(0, anchor);

    static constexpr std::array<float, kMaxPlayers> kHorizontalOffsets{{0.0f, 36.0f, -36.0f, 72.0f}};
    int slot_idx = 0;
    for (Player* p : players.ActivePlayers()) {
        Vec2 chosen = anchor;
        bool found = false;
        for (int attempt = 0; attempt < kMaxPlayers; ++attempt) {
            const float dx = kHorizontalOffsets[static_cast<std::size_t>((slot_idx + attempt) % kMaxPlayers)];
            const Vec2 candidate{anchor.x + dx, anchor.y};
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
    if (!initialized || tilemap.Empty()) {
        SelectStructure(active_structure);
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

// ============================================================================
// WorldContext
// ============================================================================

ZoneId WorldContext::MachineZone(MachineType type) noexcept {
    switch (type) {
        case MachineType::SolarArray:
            return ZoneId::DawnTerraces;
        case MachineType::HydroTurbine:
            return ZoneId::CanalQuarter;
        case MachineType::PendulumRegulator:
            return ZoneId::WindPlateau;
        case MachineType::HarmonicResonator:
            return ZoneId::HangingGardens;
        case MachineType::ThermalExchanger:
            return ZoneId::FumaroleCaldera;
        case MachineType::CentralAstrolabe:
            return ZoneId::HarmonySanctuary;
        case MachineType::Count:
            break;
    }
    return ZoneId::DawnTerraces;
}

void WorldContext::Reset() {
    const bool keep_demo = demo_mode;
    const bool keep_instant = instant_zone_transitions;
    ecosystem.Reset();
    players = PlayerManager{};
    overworld = OverworldSim{};
    structure = StructureSim{};
    overworld.InitializeDefault();
    structure.InitializeDefault();

    zone_mechanics.ResetDefaults();
    zone_mechanics.RecomputeSolarBeams(overworld.tilemap);
    interlude = ZoneInterludeState{};
    interlude.instant_mode = keep_instant;

    // 6 Monumenti (coordinate locali nella rispettiva zona 26x36 o struttura)
    const StructureSpawnLayout cistern_layout = GetStructureLayout(StructureId::HypogeanCisterns);
    const StructureSpawnLayout tower_layout = GetStructureLayout(StructureId::PendulumTower);
    const StructureSpawnLayout geo_layout = GetStructureLayout(StructureId::GeothermalChamber);

    machines[MachineIndex(MachineType::SolarArray)] =
        Machine::CreateDefaultSolar(ZoneTileCenter(13, 18));
    machines[MachineIndex(MachineType::HydroTurbine)] =
        Machine::CreateDefaultHydro(ZoneTileCenter(cistern_layout.machine_tile.row, cistern_layout.machine_tile.col));
    machines[MachineIndex(MachineType::PendulumRegulator)] =
        Machine::CreateDefaultPendulum(ZoneTileCenter(tower_layout.machine_tile.row, tower_layout.machine_tile.col));
    machines[MachineIndex(MachineType::HarmonicResonator)] =
        Machine::CreateDefaultHarmonic(ZoneTileCenter(12, 26));
    machines[MachineIndex(MachineType::ThermalExchanger)] =
        Machine::CreateDefaultThermal(ZoneTileCenter(geo_layout.machine_tile.row, geo_layout.machine_tile.col));
    machines[MachineIndex(MachineType::CentralAstrolabe)] =
        Machine::CreateDefaultAstrolabe(ZoneTileCenter(13, 18));

    modal_machine_type = MachineType::SolarArray;
    calibration_modal.Reset();

    // 4 NPC (coordinate locali nelle mappe 26x36 per gli NPC overworld)
    npcs[NpcIndex(NpcId::ClaraSereni)] = WorldNpc::CreateDefault(NpcId::ClaraSereni);
    npcs[NpcIndex(NpcId::ClaraSereni)].pos = ZoneTileCenter(12, 29);
    npcs[NpcIndex(NpcId::LorenzoAltieri)] = WorldNpc::CreateDefault(NpcId::LorenzoAltieri);
    npcs[NpcIndex(NpcId::LorenzoAltieri)].pos =
        ZoneTileCenter(cistern_layout.npc_tile.row, cistern_layout.npc_tile.col);
    npcs[NpcIndex(NpcId::MiriamFerro)] = WorldNpc::CreateDefault(NpcId::MiriamFerro);
    npcs[NpcIndex(NpcId::MiriamFerro)].pos = ZoneTileCenter(15, 11);
    npcs[NpcIndex(NpcId::MatteoVerrini)] = WorldNpc::CreateDefault(NpcId::MatteoVerrini);
    npcs[NpcIndex(NpcId::MatteoVerrini)].pos = ZoneTileCenter(17, 12);

    // 16 Reliquie (coordinate autorevoli in CollectibleRegistry::ResetDefault)
    collectibles.ResetDefault();

    story.Reset();

    current_zone = ZoneId::DawnTerraces;
    zone_banner_timer = 0.0f;
    locked_gateway_hint_timer = 0.0f;
    banner_zone = ZoneId::DawnTerraces;
    banner_structure = StructureId::HypogeanCisterns;
    banner_is_structure = false;

    demo_mode = keep_demo;
    instant_zone_transitions = keep_instant;
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

MemoryMetrics WorldContext::GetMemoryMetrics() const noexcept {
    MemoryMetrics m{};
    m.active_zone_tilemap_bytes = overworld.tilemap.MemoryBytes();
    m.active_structure_tilemap_bytes = structure.tilemap.MemoryBytes();
    m.total_resident_map_bytes = m.active_zone_tilemap_bytes + m.active_structure_tilemap_bytes;
    constexpr std::size_t kFullSixZonesBytes =
        static_cast<std::size_t>(ZoneId::Count) *
        static_cast<std::size_t>(Tilemap::kZoneRows) *
        static_cast<std::size_t>(Tilemap::kZoneCols);
    constexpr std::size_t kAllThreeStructuresBytes = (20 * 40) + (22 * 38) + (20 * 42);
    const std::size_t unoptimized_total = kFullSixZonesBytes + kAllThreeStructuresBytes;
    m.saved_unloaded_bytes =
        (unoptimized_total > m.total_resident_map_bytes)
            ? (unoptimized_total - m.total_resident_map_bytes)
            : 0;
    m.zone_swaps_count = overworld.zone_swaps_count;
    return m;
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
        if (loc == LocationId::Overworld && MachineZone(m.type) != current_zone) {
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
        if (loc == LocationId::Overworld && MachineZone(m.type) != current_zone) {
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
        if (m.location != loc) {
            continue;
        }
        if (loc == LocationId::Overworld && MachineZone(m.type) != current_zone) {
            continue;
        }
        out.push_back(&m);
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
    out.reserve(npcs.size());
    const LocationId loc = ActiveLocation();
    for (const auto& npc : npcs) {
        if (npc.location != loc) {
            continue;
        }
        if (loc == LocationId::Overworld && npc.zone != current_zone) {
            continue;
        }
        out.push_back(&npc);
    }
    return out;
}

std::vector<const CollectibleItem*> WorldContext::CollectiblesInCurrentScene() const {
    std::vector<const CollectibleItem*> out;
    const LocationId loc = ActiveLocation();
    for (const auto& item : collectibles.Items()) {
        if (item.location != loc) {
            continue;
        }
        if (loc == LocationId::Overworld && item.zone != current_zone) {
            continue;
        }
        out.push_back(&item);
    }
    return out;
}

ObjectiveTarget WorldContext::CurrentObjectiveTarget() const noexcept {
    ObjectiveTarget out{};
    out.location = ActiveLocation();
    out.zone = current_zone;

    if (simulation_completed ||
        GetMachine(MachineType::CentralAstrolabe).state != MachineState::Broken) {
        out.valid = false;
        return out;
    }

    auto find_exit_portal_center = [](const Tilemap& map) noexcept -> Vec2 {
        float sum_x = 0.0f;
        float sum_y = 0.0f;
        int count = 0;
        const int rows = map.Rows();
        const int cols = map.Cols();
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (map.GetTile(r, c) == TileId::ExitPortal) {
                    const Vec2 center = map.TileCenter(r, c);
                    sum_x += center.x;
                    sum_y += center.y;
                    ++count;
                }
            }
        }
        if (count > 0) {
            const float inv = 1.0f / static_cast<float>(count);
            return {sum_x * inv, sum_y * inv};
        }
        return map.TileCenter(std::max(0, rows - 3), 2);
    };

    // Trova il varco nella zona corrente verso la zona target (catena lineare 0 <-> 1 <-> 2 <-> 3 <-> 4 <-> 5)
    auto next_hop_zone = [](ZoneId from, ZoneId to) noexcept -> ZoneId {
        if (from == to) {
            return to;
        }
        const int cur = static_cast<int>(from);
        const int dst = static_cast<int>(to);
        return static_cast<ZoneId>(cur + (dst > cur ? 1 : -1));
    };

    auto guide_overworld = [&](ZoneId target_zone, Vec2 target_pos, const char* label) noexcept -> ObjectiveTarget {
        if (current_mode != GameMode::Overworld) {
            return ObjectiveTarget{
                true,
                ActiveLocation(),
                current_zone,
                find_exit_portal_center(structure.tilemap),
                "Uscita verso Meridia",
                true
            };
        }
        if (target_zone == current_zone) {
            return ObjectiveTarget{true, LocationId::Overworld, target_zone, target_pos, label, false};
        }
        const ZoneId hop = next_hop_zone(current_zone, target_zone);
        for (const auto& gw : overworld.active_gateways) {
            if (gw.to_zone == hop) {
                return ObjectiveTarget{true, LocationId::Overworld, target_zone, gw.world_center, label, true};
            }
        }
        return ObjectiveTarget{true, LocationId::Overworld, target_zone, target_pos, label, false};
    };

    auto guide_structure = [&](
        StructureId target_struct,
        Vec2 interior_pos,
        const char* interior_label,
        const char* portal_label) noexcept -> ObjectiveTarget {
        const PortalLink* link = overworld.FindPortal(target_struct);
        const ZoneId portal_zone = (link != nullptr) ? link->source_zone : OverworldSim::PortalZone(target_struct);
        if (current_mode == GameMode::Structure && active_structure == target_struct) {
            return ObjectiveTarget{
                true,
                LocationFromStructure(target_struct),
                portal_zone,
                interior_pos,
                interior_label,
                false
            };
        }
        if (current_mode == GameMode::Overworld) {
            const Vec2 p_pos = (link != nullptr) ? link->world_center : overworld.spawn_point;
            return guide_overworld(portal_zone, p_pos, portal_label);
        }
        return ObjectiveTarget{
            true,
            ActiveLocation(),
            current_zone,
            find_exit_portal_center(structure.tilemap),
            "Uscita verso Meridia",
            true
        };
    };

    const Machine& solar = GetMachine(MachineType::SolarArray);
    if (solar.state == MachineState::Broken) {
        return guide_overworld(ZoneId::DawnTerraces, solar.pos, "Eliostato Solare");
    }

    const Machine& hydro = GetMachine(MachineType::HydroTurbine);
    if (hydro.state == MachineState::Broken) {
        return guide_structure(
            StructureId::HypogeanCisterns,
            hydro.pos,
            "Turbina Idrostatica",
            "Cisterne Ipogee");
    }

    const WorldNpc& clara = GetNpc(NpcId::ClaraSereni);
    if (!ecosystem.architect_recruited && !clara.recruited) {
        return guide_overworld(ZoneId::CanalQuarter, clara.pos, "Clara Sereni (Ponte)");
    }

    const Machine& pendulum = GetMachine(MachineType::PendulumRegulator);
    if (pendulum.state == MachineState::Broken) {
        return guide_structure(
            StructureId::PendulumTower,
            pendulum.pos,
            "Pendolo Isochronos",
            "Torre del Pendolo");
    }

    const Machine& harmonic = GetMachine(MachineType::HarmonicResonator);
    if (harmonic.state == MachineState::Broken) {
        return guide_overworld(ZoneId::HangingGardens, harmonic.pos, "Risonatore Armonico");
    }

    const Machine& thermal = GetMachine(MachineType::ThermalExchanger);
    if (thermal.state == MachineState::Broken) {
        return guide_structure(
            StructureId::GeothermalChamber,
            thermal.pos,
            "Scambiatore di Carnot",
            "Camera Geotermica");
    }

    const Machine& astrolabe = GetMachine(MachineType::CentralAstrolabe);
    return guide_overworld(ZoneId::HarmonySanctuary, astrolabe.pos, "Astrolabio Centrale");
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

bool WorldContext::IsZoneMainTaskCompleted(ZoneId zone) const noexcept {
    switch (zone) {
        case ZoneId::DawnTerraces:
            return GetMachine(MachineType::SolarArray).state != MachineState::Broken;
        case ZoneId::CanalQuarter:
            return GetMachine(MachineType::HydroTurbine).state != MachineState::Broken &&
                   ecosystem.architect_recruited;
        case ZoneId::WindPlateau:
            return GetMachine(MachineType::PendulumRegulator).state != MachineState::Broken;
        case ZoneId::HangingGardens:
            return GetMachine(MachineType::HarmonicResonator).state != MachineState::Broken;
        case ZoneId::FumaroleCaldera:
            return GetMachine(MachineType::ThermalExchanger).state != MachineState::Broken;
        case ZoneId::HarmonySanctuary:
            return GetMachine(MachineType::CentralAstrolabe).state != MachineState::Broken;
        case ZoneId::Count:
            break;
    }
    return false;
}

bool WorldContext::IsZoneGatewayUnlocked(const ZoneGateway& gw) const noexcept {
    if (static_cast<int>(gw.to_zone) <= static_cast<int>(gw.from_zone)) {
        return true;
    }
    if (gw.requires_bridge_unlock && !ecosystem.architect_recruited) {
        return false;
    }
    return IsZoneMainTaskCompleted(gw.from_zone);
}

const char* WorldContext::ZoneGatewayLockReason(const ZoneGateway& gw) const noexcept {
    switch (gw.from_zone) {
        case ZoneId::DawnTerraces:
            return "Varco sigillato: attiva prima l'Eliostato Solare sulle Terrazze dell'Alba.";
        case ZoneId::CanalQuarter: {
            const bool hydro_broken =
                GetMachine(MachineType::HydroTurbine).state == MachineState::Broken;
            if (hydro_broken && !ecosystem.architect_recruited) {
                return "Varco sigillato: riattiva la Turbina nelle Cisterne Ipogee e parla con Clara Sereni.";
            }
            if (hydro_broken) {
                return "Varco sigillato: riattiva prima la Turbina Idrostatica nelle Cisterne Ipogee.";
            }
            return "Ponte sigillato: parla con Clara Sereni per aprire il Ponte degli Archi.";
        }
        case ZoneId::WindPlateau:
            return "Varco sigillato: calibra prima il Pendolo Isochronos nella Torre del Pendolo.";
        case ZoneId::HangingGardens:
            return "Varco sigillato: accorda prima il Risonatore Armonico nei Giardini Sospesi.";
        case ZoneId::FumaroleCaldera:
            return "Varco sigillato: calibra prima lo Scambiatore di Carnot nella Camera Geotermica.";
        case ZoneId::HarmonySanctuary:
        case ZoneId::Count:
            break;
    }
    return "Varco sigillato: completa prima il monumento di questa zona.";
}

const char* WorldContext::ZoneGatewayLockedPlaque(const ZoneGateway& gw) const noexcept {
    switch (gw.from_zone) {
        case ZoneId::DawnTerraces:
            return "SIGILLATO: ELIOSTATO";
        case ZoneId::CanalQuarter: {
            const bool hydro_broken =
                GetMachine(MachineType::HydroTurbine).state == MachineState::Broken;
            if (hydro_broken) {
                return "SIGILLATO: TURBINA";
            }
            return "PONTE SIGILLATO";
        }
        case ZoneId::WindPlateau:
            return "SIGILLATO: PENDOLO";
        case ZoneId::HangingGardens:
            return "SIGILLATO: RISONATORE";
        case ZoneId::FumaroleCaldera:
            return "SIGILLATO: CARNOT";
        case ZoneId::HarmonySanctuary:
        case ZoneId::Count:
            break;
    }
    return "VARCO SIGILLATO";
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
    interlude.active = false;
    active_structure = id;
    current_mode = GameMode::Structure;
    returning_from_structure = false;
    overworld.last_used_structure = id;
    // Libera la tilemap overworld mentre siamo dentro la struttura (1 sola mappa in RAM)
    overworld.UnloadZoneMemory();
    structure.OnEnter(id, players, ecosystem.architect_recruited);
    TriggerStructureBanner(id);
    UpdateProximityPrompts();
    story.Observe(BuildStorySnapshot());
}

void WorldContext::ExitToOverworld() {
    EnsureInitialized();
    interlude.active = false;
    const StructureId from_id = active_structure;
    current_mode = GameMode::Overworld;
    returning_from_structure = true;
    // Libera la tilemap della struttura prima di ricaricare la zona overworld
    structure.UnloadMemory();
    overworld.OnEnter(players, true, ecosystem.architect_recruited, from_id);
    current_zone = overworld.active_zone;
    if (current_zone == ZoneId::DawnTerraces) {
        zone_mechanics.RecomputeSolarBeams(overworld.tilemap);
    }
    TriggerZoneBanner(current_zone);
    UpdateProximityPrompts();
    story.Observe(BuildStorySnapshot());
}

void WorldContext::TeleportToZone(ZoneId zone) {
    EnsureInitialized();
    interlude.active = false;
    current_mode = GameMode::Overworld;
    returning_from_structure = false;
    structure.UnloadMemory();
    overworld.LoadZone(zone);
    const Vec2 spawn = overworld.SafeSpawnInZone(zone, ecosystem.architect_recruited);
    players.EnsureActive(0, spawn);
    for (Player* p : players.ActivePlayers()) {
        p->pos = spawn;
        p->vel = {0.0f, 0.0f};
        p->ResetVerticalState();
    }
    overworld.camera_target = players.AverageActivePosition();
    current_zone = zone;
    if (current_zone == ZoneId::DawnTerraces) {
        zone_mechanics.RecomputeSolarBeams(overworld.tilemap);
    }
    TriggerZoneBanner(zone);
    UpdateProximityPrompts();
    story.Observe(BuildStorySnapshot());
}

void WorldContext::BeginZoneInterlude(ZoneId to_zone, TileCoord arrival_spawn_tile) {
    EnsureInitialized();
    interlude.active = true;
    interlude.from_zone = current_zone;
    interlude.to_zone = to_zone;
    interlude.target_spawn_tile = arrival_spawn_tile;
    interlude.elapsed = 0.0f;
    interlude.duration = kZoneInterludeDuration;
    interlude.min_skip_time = kZoneInterludeMinSkip;
    interlude.swap_committed = false;
    interlude.instant_mode = instant_zone_transitions;

    if (instant_zone_transitions || interlude.instant_mode) {
        SkipZoneInterlude();
    }
}

void WorldContext::CommitZoneSwap() {
    if (interlude.swap_committed) {
        return;
    }
    overworld.LoadZone(interlude.to_zone, interlude.target_spawn_tile);
    const Vec2 spawn = overworld.SafeSpawnAtTile(
        interlude.target_spawn_tile,
        ecosystem.architect_recruited,
        zone_mechanics.solar_gate_open,
        zone_mechanics.jade_bridge_unlocked);
    players.EnsureActive(0, spawn);
    static constexpr std::array<Vec2, kMaxPlayers> kOffsets{{
        {0.0f, 0.0f}, {34.0f, 0.0f}, {-34.0f, 0.0f}, {0.0f, 34.0f}
    }};
    int idx = 0;
    for (Player* p : players.ActivePlayers()) {
        const Vec2 desired = spawn + kOffsets[static_cast<std::size_t>(idx % kMaxPlayers)];
        p->pos = FindSafeSpawnNear(
            overworld.tilemap,
            desired,
            p->size,
            spawn,
            ecosystem.architect_recruited,
            zone_mechanics.solar_gate_open,
            zone_mechanics.jade_bridge_unlocked);
        p->vel = {0.0f, 0.0f};
        p->ResetVerticalState();
        ++idx;
    }
    overworld.camera_target = players.AverageActivePosition();
    current_zone = interlude.to_zone;
    if (current_zone == ZoneId::DawnTerraces) {
        zone_mechanics.RecomputeSolarBeams(overworld.tilemap);
    }
    interlude.swap_committed = true;
    UpdateProximityPrompts();
    story.Observe(BuildStorySnapshot());
}

void WorldContext::SkipZoneInterlude() {
    if (!interlude.active) {
        return;
    }
    if (!interlude.swap_committed) {
        CommitZoneSwap();
    }
    interlude.elapsed = interlude.duration;
    interlude.active = false;
    TriggerZoneBanner(current_zone);
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
        const bool in_scene =
            (m.location == loc) &&
            (loc != LocationId::Overworld || MachineZone(m.type) == current_zone);
        if (in_scene) {
            m.UpdateProximity(players);
        } else {
            m.prompt_visible = false;
        }
    }
    for (auto& npc : npcs) {
        const bool in_scene =
            (npc.location == loc) &&
            (loc != LocationId::Overworld || npc.zone == current_zone);
        if (in_scene) {
            npc.UpdateProximity(players);
        } else {
            npc.prompt_visible = false;
        }
    }
    // Aggiorna prossimita' reliquie filtrando anche per zona in Overworld
    for (auto& item : collectibles.Items()) {
        const bool in_scene =
            !item.collected &&
            item.location == loc &&
            (loc != LocationId::Overworld || item.zone == current_zone);
        bool near_interact = false;
        if (in_scene) {
            for (const Player* p : players.ActivePlayers()) {
                if (p != nullptr && p->Center().DistanceTo(item.pos) <= item.interact_radius) {
                    near_interact = true;
                    break;
                }
            }
        }
        item.prompt_visible = near_interact;
    }
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
        if (loc == LocationId::Overworld && MachineZone(m.type) != current_zone) {
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
    if (locked_gateway_hint_timer > 0.0f) {
        locked_gateway_hint_timer = std::max(0.0f, locked_gateway_hint_timer - safe_dt);
    }
    collectibles.Tick(safe_dt);

    // 0. Gestione Intramezzo tra Zone (congela fisica, esegue swap pulito di memoria a 50% o su skip)
    if (interlude.active) {
        interlude.elapsed += safe_dt;
        if (!interlude.swap_committed && interlude.Progress() >= 0.5f) {
            CommitZoneSwap();
            out.zone_changed = true;
            out.new_zone = current_zone;
        }
        if (interlude.elapsed >= interlude.duration) {
            if (!interlude.swap_committed) {
                CommitZoneSwap();
                out.zone_changed = true;
                out.new_zone = current_zone;
            }
            interlude.active = false;
            out.interlude_finished = true;
            TriggerZoneBanner(current_zone);
        }
        story.Tick(safe_dt);
        return out;
    }

    if (input.advance_dialogue_pressed) {
        story.AdvanceDialogue();
    }

    // 1. UI Phase cycling
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

    // 4. Calcolo forze ambientali e aggiornamento delle 6 meccaniche top-down in Overworld
    Vec2 external_velocity{0.0f, 0.0f};
    if (current_mode == GameMode::Overworld) {
        const Player* p0 = players.GetPlayer(0);
        const Vec2 p0_center = (p0 != nullptr && p0->active) ? p0->Center() : overworld.camera_target;

        // 4a. Quartiere dei Canali: Correnti d'acqua direzionali
        if (current_zone == ZoneId::CanalQuarter) {
            for (const auto& cur : zone_mechanics.water_currents) {
                if (!cur.active) {
                    continue;
                }
                if (p0_center.x >= cur.bounds.Left() && p0_center.x <= cur.bounds.Right() &&
                    p0_center.y >= cur.bounds.Top() && p0_center.y <= cur.bounds.Bottom()) {
                    external_velocity += cur.flow_dir.Normalized() * cur.strength;
                }
            }
        }

        // 4b. Altopiano dei Venti: Raffiche cicliche & riparo dietro i monoliti
        if (current_zone == ZoneId::WindPlateau) {
            auto& w = zone_mechanics.wind;
            const bool was_gust = w.gust_active;
            w.cycle_timer += safe_dt;
            if (w.cycle_timer >= w.cycle_period) {
                w.cycle_timer = std::fmod(w.cycle_timer, w.cycle_period);
            }
            // 0..2.8s calma, 2.8..3.8s telegraphing, 3.8..6.0s raffica forte
            if (w.cycle_timer < 2.8f) {
                w.telegraphing = false;
                w.gust_active = false;
                w.gust_intensity = std::max(0.0f, w.gust_intensity - safe_dt * 3.0f);
            } else if (w.cycle_timer < 3.8f) {
                w.telegraphing = true;
                w.gust_active = false;
                const float t = (w.cycle_timer - 2.8f) / 1.0f;
                w.gust_intensity = 0.25f * t;
            } else {
                w.telegraphing = false;
                w.gust_active = true;
                const float t = (w.cycle_timer - 3.8f) / 2.2f;
                w.gust_intensity = std::sin(Clamp(t, 0.0f, 1.0f) * kPi);
                w.gust_intensity = std::max(0.35f, w.gust_intensity);
            }
            if (!was_gust && w.gust_active) {
                out.mechanics_events.wind_gust_started = true;
            }
            w.player_sheltered = zone_mechanics.IsWindSheltered(p0_center);
            if (w.gust_active && !w.player_sheltered) {
                external_velocity += w.wind_dir.Normalized() * (w.max_push_speed * w.gust_intensity);
            }
        }

        // 4c. Giardini Sospesi: Piastre di Risonanza Armonica & apertura Ponte di Giada
        if (current_zone == ZoneId::HangingGardens) {
            for (auto& pad : zone_mechanics.resonance_pads) {
                if (pad.pulse_timer > 0.0f) {
                    pad.pulse_timer = std::max(0.0f, pad.pulse_timer - safe_dt * 1.4f);
                }
                const bool inside = (p0 != nullptr && p0->active) &&
                                    (p0_center.DistanceTo(pad.pos) <= pad.radius);
                if (inside && !pad.player_was_inside && !zone_mechanics.jade_bridge_unlocked) {
                    if (pad.sequence_order == zone_mechanics.resonance_next_order) {
                        pad.activated = true;
                        pad.pulse_timer = 1.0f;
                        ++zone_mechanics.resonance_next_order;
                        out.mechanics_events.resonance_pad_hit = true;
                        out.mechanics_events.resonance_pad_index = pad.sequence_order;
                        if (zone_mechanics.resonance_next_order >=
                            static_cast<int>(zone_mechanics.resonance_pads.size())) {
                            zone_mechanics.jade_bridge_unlocked = true;
                            out.mechanics_events.resonance_solved = true;
                        }
                    } else if (!pad.activated) {
                        // Ordine errato: resetta la sequenza
                        for (auto& other : zone_mechanics.resonance_pads) {
                            other.activated = false;
                            other.pulse_timer = 0.0f;
                        }
                        pad.pulse_timer = 0.6f;
                        zone_mechanics.resonance_next_order = 0;
                        out.mechanics_events.resonance_failed = true;
                    }
                }
                pad.player_was_inside = inside;
            }
            if (zone_mechanics.jade_bridge_unlocked && zone_mechanics.jade_bridge_progress < 1.0f) {
                zone_mechanics.jade_bridge_progress =
                    std::min(1.0f, zone_mechanics.jade_bridge_progress + safe_dt * 1.8f);
            }
        }

        // 4d. Caldera delle Fumarole: Geyser Termici Ciclici (con telegraphing e spinta sicura)
        if (current_zone == ZoneId::FumaroleCaldera) {
            for (auto& g : zone_mechanics.geysers) {
                if (g.phase == GeyserPhase::Calmed) {
                    g.intensity = std::max(0.0f, g.intensity - safe_dt * 2.5f);
                    continue;
                }
                const GeyserPhase prev_phase = g.phase;
                g.phase_timer = std::fmod(sim_time + g.cycle_offset, g.cycle_period);
                if (g.phase_timer < 2.0f) {
                    g.phase = GeyserPhase::Dormant;
                    g.intensity = 0.0f;
                } else if (g.phase_timer < 3.1f) {
                    g.phase = GeyserPhase::Telegraphing;
                    g.intensity = (g.phase_timer - 2.0f) / 1.1f * 0.45f;
                } else {
                    g.phase = GeyserPhase::Erupting;
                    g.intensity = 1.0f;
                }
                if (prev_phase != GeyserPhase::Erupting && g.phase == GeyserPhase::Erupting) {
                    out.mechanics_events.geyser_erupted = true;
                }
                if (g.phase == GeyserPhase::Erupting && p0 != nullptr && p0->active) {
                    const Vec2 delta = p0_center - g.pos;
                    const float dist = delta.Length();
                    if (dist <= g.radius) {
                        const Vec2 push_dir = (dist > kEpsilon) ? (delta / dist) : Vec2{0.0f, 1.0f};
                        external_velocity += push_dir * 340.0f;
                        out.mechanics_events.geyser_pushed_player = true;
                    }
                }
            }
        }

        // 4e. Santuario dell'Armonia: 5 Sigilli Elementali attorno all'Astrolabio
        if (current_zone == ZoneId::HarmonySanctuary) {
            for (auto& seal : zone_mechanics.sanctuary_seals) {
                if (!seal.attuned && p0 != nullptr && p0->active &&
                    p0_center.DistanceTo(seal.pos) <= seal.radius) {
                    seal.attuned = true;
                    out.mechanics_events.sanctuary_seal_attuned = true;
                }
                if (seal.attuned && seal.glow < 1.0f) {
                    seal.glow = std::min(1.0f, seal.glow + safe_dt * 2.2f);
                }
            }
        }
    }

    // 5. Step fisica scena attiva
    if (current_mode == GameMode::Overworld) {
        const OverworldStepResult ow_res = overworld.Step(
            players,
            input,
            dt,
            ecosystem.architect_recruited,
            zone_mechanics.solar_gate_open,
            zone_mechanics.jade_bridge_unlocked,
            external_velocity,
            IsZoneMainTaskCompleted(current_zone));
        out.enter_structure_requested = ow_res.enter_structure_requested;
        out.target_structure = ow_res.target_structure;
        out.newly_joined_count = ow_res.newly_joined_count;
        if (ow_res.enter_structure_requested) {
            active_structure = ow_res.target_structure;
        }
        if (ow_res.blocked_gateway_touched && locked_gateway_hint_timer <= 0.0f) {
            for (const auto& gw : overworld.active_gateways) {
                if (gw.to_zone == ow_res.blocked_gateway_target) {
                    story.PushAdHocLine(
                        "Elena Valli",
                        ZoneGatewayLockReason(gw),
                        current_zone,
                        ActiveLocationName());
                    locked_gateway_hint_timer = 3.5f;
                    break;
                }
            }
        }
        if (ow_res.zone_gateway_triggered) {
            BeginZoneInterlude(ow_res.gateway_target_zone, ow_res.gateway_arrival_tile);
            if (interlude.active) {
                out.interlude_started = true;
            } else {
                // Modalita' istantanea: swap gia' completato
                out.zone_changed = true;
                out.new_zone = current_zone;
                out.interlude_finished = true;
            }
            return out;
        }
    } else {
        const StructureStepResult st_res =
            structure.Step(players, input, dt, ecosystem.architect_recruited);
        out.exit_to_overworld_requested = st_res.exit_to_overworld_requested;
        out.newly_joined_count = st_res.newly_joined_count;
    }

    // 6. Update proximity prompts
    UpdateProximityPrompts();

    // 7. Check if any active player pressed [E] interact
    bool any_interact = false;
    for (const Player* p : players.ActivePlayers()) {
        if (p != nullptr && input.Get(p->player_id).interact_pressed) {
            any_interact = true;
            break;
        }
    }

    // 8. Collectibles pickup (filtrato per LocationId e ZoneId in Overworld!)
    const LocationId loc = ActiveLocation();
    for (auto& item : collectibles.Items()) {
        if (item.collected || item.location != loc) {
            continue;
        }
        if (loc == LocationId::Overworld && item.zone != current_zone) {
            continue;
        }
        bool touch = false;
        bool interact_hit = false;
        for (const Player* p : players.ActivePlayers()) {
            if (p == nullptr) {
                continue;
            }
            const float dist = p->Center().DistanceTo(item.pos);
            if (dist <= item.auto_pickup_radius) {
                touch = true;
            }
            if (any_interact && dist <= item.interact_radius) {
                interact_hit = true;
            }
        }
        if (touch || interact_hit) {
            (void)collectibles.Collect(item.id);
            item.prompt_visible = false;
            out.collectible_picked_up = true;
            out.picked_up_id = item.id;
            const CollectibleMeta& meta = CollectibleMetadata(item.id);
            if (!meta.pickup_line.empty()) {
                story.PushAdHocLine(
                    meta.pickup_speaker.empty() ? "Elena Valli" : meta.pickup_speaker,
                    meta.pickup_line,
                    current_zone,
                    ActiveLocationName());
            }
            break;
        }
    }

    // 9. Unlock Central Astrolabe once all 5 district/structure monuments are activated
    if (AllFiveMonumentsActivated()) {
        GetMachine(MachineType::CentralAstrolabe).unlocked = true;
    }

    // 10. Handle [E] interactions with Zone Mechanics, Machines & NPCs
    for (const Player* p : players.ActivePlayers()) {
        if (p == nullptr || !input.Get(p->player_id).interact_pressed) {
            continue;
        }

        bool handled = false;
        const Vec2 p_center = p->Center();

        // 10a. Interazioni con i puzzle interni o con le meccaniche top-down della zona corrente
        if (current_mode == GameMode::Structure) {
            if (const StructureSwitch* sw = structure.puzzle.FindNearestSwitch(p_center)) {
                const StructurePuzzleEventFlags pev =
                    structure.puzzle.ToggleSwitchById(sw->id, structure.tilemap);
                if (pev.Any()) {
                    structure.SyncActivePuzzleToSaved();
                    out.structure_puzzle_events = pev;
                    handled = true;
                }
            }
        } else if (current_mode == GameMode::Overworld) {
            if (current_zone == ZoneId::DawnTerraces) {
                for (auto& mirror : zone_mechanics.solar_mirrors) {
                    if (mirror.rotatable && p_center.DistanceTo(mirror.pos) <= mirror.interact_radius) {
                        const bool was_open = zone_mechanics.solar_gate_open;
                        mirror.dir_index = (mirror.dir_index + 1) % 4;
                        zone_mechanics.RecomputeSolarBeams(overworld.tilemap);
                        out.mechanics_events.mirror_rotated = true;
                        if (!was_open && zone_mechanics.solar_gate_open) {
                            out.mechanics_events.solar_gate_unlocked = true;
                        }
                        handled = true;
                        break;
                    }
                }
            } else if (current_zone == ZoneId::CanalQuarter) {
                for (auto& lever : zone_mechanics.sluice_levers) {
                    if (p_center.DistanceTo(lever.pos) <= lever.interact_radius) {
                        lever.pulled = !lever.pulled;
                        for (auto& cur : zone_mechanics.water_currents) {
                            if (cur.sluice_group == lever.sluice_group) {
                                cur.active = !lever.pulled;
                            }
                        }
                        out.mechanics_events.sluice_toggled = true;
                        handled = true;
                        break;
                    }
                }
            } else if (current_zone == ZoneId::FumaroleCaldera) {
                for (auto& valve : zone_mechanics.relief_valves) {
                    if (!valve.open && p_center.DistanceTo(valve.pos) <= valve.interact_radius) {
                        valve.open = true;
                        for (auto& g : zone_mechanics.geysers) {
                            if (g.valve_group == valve.valve_group) {
                                g.phase = GeyserPhase::Calmed;
                                g.intensity = 0.0f;
                            }
                        }
                        out.mechanics_events.valve_opened = true;
                        handled = true;
                        break;
                    }
                }
            } else if (current_zone == ZoneId::HarmonySanctuary) {
                for (auto& seal : zone_mechanics.sanctuary_seals) {
                    if (!seal.attuned && p_center.DistanceTo(seal.pos) <= seal.radius + 24.0f) {
                        seal.attuned = true;
                        out.mechanics_events.sanctuary_seal_attuned = true;
                        handled = true;
                        break;
                    }
                }
            }
        }
        if (handled) {
            break;
        }

        // 10b. Machines in current scene
        for (auto& m : machines) {
            if (m.location != loc) {
                continue;
            }
            if (loc == LocationId::Overworld && MachineZone(m.type) != current_zone) {
                continue;
            }
            if (!m.IsPlayerInRange(*p)) {
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

        // 10c. NPCs in current scene
        for (auto& npc : npcs) {
            if (npc.location != loc) {
                continue;
            }
            if (loc == LocationId::Overworld && npc.zone != current_zone) {
                continue;
            }
            if (!npc.IsPlayerInRange(*p)) {
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
                        "Riavvia prima la Turbina Idrostatica qui sotto: solo allora ti consegnero' le mappe dell'archivio.";
                    story.PushAdHocLine(npc.name, npc.last_dialogue, current_zone, ActiveLocationName());
                }
            } else if (npc.id == NpcId::MiriamFerro) {
                if (GetMachine(MachineType::HarmonicResonator).state != MachineState::Broken) {
                    npc.recruited = true;
                    npc.last_dialogue =
                        "Senti il tremito della giada? I semi di Meridia stanno gia' rompendo la crosta di cenere.";
                } else {
                    npc.last_dialogue =
                        "Risveglia le quattro piastre armoniche per tendere il Ponte di Giada, poi accorda il Risonatore a 120 Hz.";
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
                        "Apri le valvole di sfogo per calmare le fumarole, poi dimostra nella Camera Geotermica che Carnot regge l'inverno.";
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
