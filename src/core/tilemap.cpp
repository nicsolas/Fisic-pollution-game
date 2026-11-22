#include "core/tilemap.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace eco {

namespace {

[[nodiscard]] TileId IntToTileId(int raw) noexcept {
    switch (raw) {
        case 0:
            return TileId::Walkable;
        case 1:
            return TileId::Wall;
        case 2:
            return TileId::StructurePortal;
        case 3:
            return TileId::ExitPortal;
        case 4:
            return TileId::HiddenBridge;
        case 5:
            return TileId::Water;
        case 6:
            return TileId::PavedPath;
        case 7:
            return TileId::SanctuaryFloor;
        case 8:
            return TileId::RuinsWall;
        case 9:
            return TileId::GardenFloor;
        default:
            return TileId::Wall;
    }
}

[[nodiscard]] bool IsValidFiniteRect(const Rect& aabb) noexcept {
    return std::isfinite(aabb.x) && std::isfinite(aabb.y) &&
           std::isfinite(aabb.width) && std::isfinite(aabb.height) &&
           aabb.width > 0.0f && aabb.height > 0.0f;
}

void FillRect(
    std::vector<std::vector<int>>& grid,
    int r0,
    int r1,
    int c0,
    int c1,
    TileId id) {
    const int rows = static_cast<int>(grid.size());
    for (int r = std::max(0, r0); r <= std::min(rows - 1, r1); ++r) {
        const int cols = static_cast<int>(grid[static_cast<std::size_t>(r)].size());
        for (int c = std::max(0, c0); c <= std::min(cols - 1, c1); ++c) {
            grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = static_cast<int>(id);
        }
    }
}

void SetCell(std::vector<std::vector<int>>& grid, int r, int c, TileId id) {
    if (r < 0 || r >= static_cast<int>(grid.size())) {
        return;
    }
    auto& row = grid[static_cast<std::size_t>(r)];
    if (c < 0 || c >= static_cast<int>(row.size())) {
        return;
    }
    row[static_cast<std::size_t>(c)] = static_cast<int>(id);
}

[[nodiscard]] Tilemap CreateHypogeanCisternsMap(float tile_size) {
    // 16 rows x 34 cols (768 x 1632 px) — Cisterne Ipogee
    constexpr int kRows = 16;
    constexpr int kCols = 34;
    std::vector<std::vector<int>> grid(kRows, std::vector<int>(kCols, 0));

    FillRect(grid, 0, 0, 0, kCols - 1, TileId::Wall);
    FillRect(grid, kRows - 2, kRows - 1, 0, kCols - 1, TileId::Wall);
    FillRect(grid, 0, kRows - 1, 0, 0, TileId::Wall);
    FillRect(grid, 0, kRows - 1, kCols - 1, kCols - 1, TileId::Wall);

    // ExitPortal on ground floor left side (row 13, cols 2..3)
    SetCell(grid, 13, 2, TileId::ExitPortal);
    SetCell(grid, 13, 3, TileId::ExitPortal);

    // Ascending aqueduct stone ledges (every vertical jump step <= 2 tiles = 96 px)
    FillRect(grid, 12, 12, 16, 20, TileId::Wall); // Tier 1
    FillRect(grid, 10, 10, 22, 26, TileId::Wall); // Tier 2 East
    FillRect(grid, 8, 8, 24, 31, TileId::Wall);   // Tier 3 Archivist Gallery (Lorenzo + Relic)
    FillRect(grid, 10, 10, 8, 14, TileId::Wall);  // Tier 2 West Cistern Ledge (Relic)

    return Tilemap(std::move(grid), tile_size);
}

[[nodiscard]] Tilemap CreatePendulumTowerMap(float tile_size) {
    // 18 rows x 30 cols (864 x 1440 px) — Torre del Pendolo
    constexpr int kRows = 18;
    constexpr int kCols = 30;
    std::vector<std::vector<int>> grid(kRows, std::vector<int>(kCols, 0));

    FillRect(grid, 0, 0, 0, kCols - 1, TileId::Wall);
    FillRect(grid, kRows - 2, kRows - 1, 0, kCols - 1, TileId::Wall);
    FillRect(grid, 0, kRows - 1, 0, 0, TileId::Wall);
    FillRect(grid, 0, kRows - 1, kCols - 1, kCols - 1, TileId::Wall);

    // ExitPortal on ground floor left side (row 15, cols 2..3)
    SetCell(grid, 15, 2, TileId::ExitPortal);
    SetCell(grid, 15, 3, TileId::ExitPortal);

    // Ascending spiral clocktower galleries (every vertical step = 2 tiles)
    FillRect(grid, 14, 14, 10, 15, TileId::Wall); // Tier 1
    FillRect(grid, 12, 12, 17, 23, TileId::Wall); // Tier 2
    FillRect(grid, 10, 10, 11, 18, TileId::Wall); // Tier 3
    FillRect(grid, 8, 8, 10, 20, TileId::Wall);   // Tier 4 Central Pendulum Gallery
    FillRect(grid, 6, 6, 3, 9, TileId::Wall);     // Tier 5 West Belfry (Relic)
    FillRect(grid, 6, 6, 21, 27, TileId::Wall);   // Tier 5 East Belfry (Relic)

    return Tilemap(std::move(grid), tile_size);
}

[[nodiscard]] Tilemap CreateGeothermalChamberMap(float tile_size) {
    // 16 rows x 34 cols (768 x 1632 px) — Camera Geotermica
    constexpr int kRows = 16;
    constexpr int kCols = 34;
    std::vector<std::vector<int>> grid(kRows, std::vector<int>(kCols, 0));

    FillRect(grid, 0, 0, 0, kCols - 1, TileId::Wall);
    FillRect(grid, kRows - 2, kRows - 1, 0, kCols - 1, TileId::Wall);
    FillRect(grid, 0, kRows - 1, 0, 0, TileId::Wall);
    FillRect(grid, 0, kRows - 1, kCols - 1, kCols - 1, TileId::Wall);

    // ExitPortal on ground floor left side (row 13, cols 2..3)
    SetCell(grid, 13, 2, TileId::ExitPortal);
    SetCell(grid, 13, 3, TileId::ExitPortal);

    // Basalt geothermal bridges & steam catwalks (every vertical step <= 2 tiles)
    FillRect(grid, 12, 12, 10, 15, TileId::Wall); // Tier 1
    FillRect(grid, 10, 10, 17, 22, TileId::Wall); // Tier 2
    FillRect(grid, 10, 10, 24, 30, TileId::Wall); // Tier 3 Carnot Exchanger Deck
    FillRect(grid, 8, 8, 12, 18, TileId::Wall);   // Tier 4 Upper Basalt Catwalk
    FillRect(grid, 6, 6, 4, 10, TileId::Wall);    // Tier 5 Upper West Alcove (Relic)

    return Tilemap(std::move(grid), tile_size);
}

} // namespace

Tilemap::Tilemap(std::vector<std::vector<int>> grid, float tile_size)
    : grid_(std::move(grid)),
      tile_size_((std::isfinite(tile_size) && tile_size > 0.0f) ? tile_size : kTileSize) {
    int max_c = 0;
    for (const auto& row : grid_) {
        max_c = std::max(max_c, static_cast<int>(row.size()));
    }
    max_cols_ = max_c;
}

Tilemap Tilemap::CreateOverworldMap(float tile_size) {
    // 36 rows x 50 cols (1728 x 2400 px at kTileSize = 48.0f) — La Conca di Meridia
    constexpr int kRows = kOverworldRows;
    constexpr int kCols = kOverworldCols;
    std::vector<std::vector<int>> grid(kRows, std::vector<int>(kCols, 0));

    // 1. District ground layers (walkable decorative surfaces)
    // Giardini Sospesi (Centro-Est) base flora
    FillRect(grid, 13, 22, 26, 47, TileId::GardenFloor);
    // Terrazze dell'Alba (SO) garden alcoves & sundial court
    FillRect(grid, 25, 26, 14, 21, TileId::GardenFloor);
    FillRect(grid, 32, 34, 2, 8, TileId::GardenFloor);
    FillRect(grid, 27, 31, 9, 13, TileId::SanctuaryFloor);
    // Quartiere dei Canali olive courtyard
    FillRect(grid, 20, 22, 3, 5, TileId::GardenFloor);
    // Santuario dell'Armonia (SE) astronomical court & flanking gardens
    FillRect(grid, 25, 34, 26, 29, TileId::GardenFloor);
    FillRect(grid, 25, 34, 45, 48, TileId::GardenFloor);
    FillRect(grid, 26, 33, 30, 44, TileId::SanctuaryFloor);

    // 2. Paved stone avenues connecting all 6 districts smoothly
    FillRect(grid, 4, 31, 10, 11, TileId::PavedPath);  // West North-South spine
    FillRect(grid, 4, 31, 36, 37, TileId::PavedPath);  // East North-South spine
    FillRect(grid, 16, 18, 5, 23, TileId::PavedPath);  // West-East avenue (West side)
    FillRect(grid, 16, 18, 25, 44, TileId::PavedPath); // West-East avenue (East side)
    FillRect(grid, 29, 30, 4, 9, TileId::PavedPath);   // Dawn spawn to sundial
    FillRect(grid, 29, 30, 13, 18, TileId::PavedPath); // Dawn east terrace walk
    FillRect(grid, 8, 8, 12, 21, TileId::PavedPath);   // WindPlateau east overlook
    FillRect(grid, 8, 8, 31, 40, TileId::PavedPath);   // FumaroleCaldera furnace square
    FillRect(grid, 15, 19, 34, 38, TileId::SanctuaryFloor); // HangingGardens resonator pavilion

    // 3. Central Canal & Ponte degli Archi at col 24
    FillRect(grid, 1, 4, kBridgeDividerCol, kBridgeDividerCol, TileId::RuinsWall);
    FillRect(grid, 5, 15, kBridgeDividerCol, kBridgeDividerCol, TileId::Water);
    FillRect(grid, 16, 18, kBridgeDividerCol, kBridgeDividerCol, TileId::HiddenBridge);
    FillRect(grid, 19, 30, kBridgeDividerCol, kBridgeDividerCol, TileId::Water);
    FillRect(grid, 31, 34, kBridgeDividerCol, kBridgeDividerCol, TileId::RuinsWall);

    // 4. Water canals & reflecting basins
    // Quartiere dei Canali flanking aqueduct pools
    FillRect(grid, 13, 14, 18, 22, TileId::Water);
    FillRect(grid, 20, 21, 18, 22, TileId::Water);
    // Giardini Sospesi reflecting pools
    FillRect(grid, 14, 15, 28, 30, TileId::Water);
    FillRect(grid, 20, 21, 28, 30, TileId::Water);
    // Santuario dell'Armonia side basins
    FillRect(grid, 27, 28, 27, 27, TileId::Water);
    FillRect(grid, 27, 28, 47, 47, TileId::Water);

    // 5. Ancient Ruins, Colonnades & Basalt Outcrops (solid RuinsWall / Wall)
    // DawnTerraces ruins
    FillRect(grid, 25, 26, 4, 6, TileId::RuinsWall);
    FillRect(grid, 32, 33, 15, 17, TileId::RuinsWall);
    // CanalQuarter aqueduct piers & west fountain wall
    SetCell(grid, 15, 19, TileId::RuinsWall);
    SetCell(grid, 15, 21, TileId::RuinsWall);
    SetCell(grid, 19, 19, TileId::RuinsWall);
    SetCell(grid, 19, 21, TileId::RuinsWall);
    FillRect(grid, 19, 20, 7, 8, TileId::RuinsWall);
    // WindPlateau windbreak cliffs
    FillRect(grid, 4, 5, 4, 5, TileId::RuinsWall);
    FillRect(grid, 7, 8, 15, 17, TileId::RuinsWall);
    FillRect(grid, 3, 4, 19, 20, TileId::RuinsWall);
    // HangingGardens pergola pillars
    SetCell(grid, 13, 41, TileId::RuinsWall);
    SetCell(grid, 13, 45, TileId::RuinsWall);
    SetCell(grid, 21, 41, TileId::RuinsWall);
    SetCell(grid, 21, 45, TileId::RuinsWall);
    // FumaroleCaldera broken furnace stacks
    FillRect(grid, 4, 5, 28, 30, TileId::RuinsWall);
    FillRect(grid, 7, 8, 42, 44, TileId::RuinsWall);
    FillRect(grid, 2, 2, 43, 46, TileId::Wall);
    // HarmonySanctuary symmetrical colonnade pillars
    SetCell(grid, 26, 29, TileId::RuinsWall);
    SetCell(grid, 30, 29, TileId::RuinsWall);
    SetCell(grid, 26, 45, TileId::RuinsWall);
    SetCell(grid, 30, 45, TileId::RuinsWall);

    // 6. Three Dedicated Structure Portals (framed stone alcoves opening south)
    // Portal 1: Cisterne Ipogee (Quartiere dei Canali, row 14, cols 5..6)
    FillRect(grid, 13, 13, 4, 7, TileId::RuinsWall);
    SetCell(grid, 14, 4, TileId::RuinsWall);
    SetCell(grid, 14, 5, TileId::StructurePortal);
    SetCell(grid, 14, 6, TileId::StructurePortal);
    SetCell(grid, 14, 7, TileId::RuinsWall);

    // Portal 2: Torre del Pendolo (Altopiano dei Venti, row 3, cols 11..12)
    FillRect(grid, 2, 2, 10, 13, TileId::RuinsWall);
    SetCell(grid, 3, 10, TileId::RuinsWall);
    SetCell(grid, 3, 11, TileId::StructurePortal);
    SetCell(grid, 3, 12, TileId::StructurePortal);
    SetCell(grid, 3, 13, TileId::RuinsWall);

    // Portal 3: Camera Geotermica (Caldera delle Fumarole, row 3, cols 37..38)
    FillRect(grid, 2, 2, 36, 39, TileId::Wall);
    SetCell(grid, 3, 36, TileId::Wall);
    SetCell(grid, 3, 37, TileId::StructurePortal);
    SetCell(grid, 3, 38, TileId::StructurePortal);
    SetCell(grid, 3, 39, TileId::Wall);

    // 7. Outer perimeter walls
    FillRect(grid, 0, 0, 0, kCols - 1, TileId::Wall);
    FillRect(grid, kRows - 1, kRows - 1, 0, kCols - 1, TileId::Wall);
    FillRect(grid, 0, kRows - 1, 0, 0, TileId::Wall);
    FillRect(grid, 0, kRows - 1, kCols - 1, kCols - 1, TileId::Wall);

    return Tilemap(std::move(grid), tile_size);
}

Tilemap Tilemap::CreateStructureMap(StructureId structure_id, float tile_size) {
    switch (structure_id) {
        case StructureId::HypogeanCisterns:
            return CreateHypogeanCisternsMap(tile_size);
        case StructureId::PendulumTower:
            return CreatePendulumTowerMap(tile_size);
        case StructureId::GeothermalChamber:
            return CreateGeothermalChamberMap(tile_size);
        case StructureId::Count:
            break;
    }
    return CreateHypogeanCisternsMap(tile_size);
}

Tilemap Tilemap::CreateStructureMap(float tile_size) {
    return CreateStructureMap(StructureId::HypogeanCisterns, tile_size);
}

ZoneId Tilemap::DistrictAtTile(int row, int col) noexcept {
    const bool east = (col > kBridgeDividerCol);
    if (row < 12) {
        return east ? ZoneId::FumaroleCaldera : ZoneId::WindPlateau;
    }
    if (row < 24) {
        return east ? ZoneId::HangingGardens : ZoneId::CanalQuarter;
    }
    return east ? ZoneId::HarmonySanctuary : ZoneId::DawnTerraces;
}

ZoneId Tilemap::DistrictAt(Vec2 world_pos) const noexcept {
    const TileCoord tc = WorldToTile(world_pos);
    if (tc.row < 0 || tc.col < 0) {
        return ZoneId::DawnTerraces;
    }
    return DistrictAtTile(tc.row, tc.col);
}

Rect Tilemap::DistrictBounds(ZoneId zone) const noexcept {
    const float ts = tile_size_;
    const int west_cols = kBridgeDividerCol + 1;
    const int east_start = kBridgeDividerCol + 1;
    const int east_cols = std::max(1, max_cols_ - east_start);
    const int band_rows = 12;

    switch (zone) {
        case ZoneId::WindPlateau:
            return {0.0f, 0.0f, static_cast<float>(west_cols) * ts, static_cast<float>(band_rows) * ts};
        case ZoneId::CanalQuarter:
            return {0.0f, static_cast<float>(band_rows) * ts, static_cast<float>(west_cols) * ts, static_cast<float>(band_rows) * ts};
        case ZoneId::DawnTerraces:
            return {0.0f, static_cast<float>(2 * band_rows) * ts, static_cast<float>(west_cols) * ts, static_cast<float>(std::max(1, Rows() - 2 * band_rows)) * ts};
        case ZoneId::FumaroleCaldera:
            return {static_cast<float>(east_start) * ts, 0.0f, static_cast<float>(east_cols) * ts, static_cast<float>(band_rows) * ts};
        case ZoneId::HangingGardens:
            return {static_cast<float>(east_start) * ts, static_cast<float>(band_rows) * ts, static_cast<float>(east_cols) * ts, static_cast<float>(band_rows) * ts};
        case ZoneId::HarmonySanctuary:
            return {static_cast<float>(east_start) * ts, static_cast<float>(2 * band_rows) * ts, static_cast<float>(east_cols) * ts, static_cast<float>(std::max(1, Rows() - 2 * band_rows)) * ts};
        case ZoneId::Count:
            break;
    }
    return {0.0f, 0.0f, WorldWidth(), WorldHeight()};
}

int Tilemap::Rows() const noexcept {
    return static_cast<int>(grid_.size());
}

int Tilemap::Cols() const noexcept {
    return max_cols_;
}

int Tilemap::RowCols(int row) const noexcept {
    if (row < 0 || row >= Rows()) {
        return 0;
    }
    return static_cast<int>(grid_[static_cast<std::size_t>(row)].size());
}

float Tilemap::TileSize() const noexcept {
    return tile_size_;
}

float Tilemap::WorldWidth() const noexcept {
    return static_cast<float>(max_cols_) * tile_size_;
}

float Tilemap::WorldHeight() const noexcept {
    return static_cast<float>(Rows()) * tile_size_;
}

bool Tilemap::InBounds(int row, int col) const noexcept {
    return row >= 0 && row < Rows() && col >= 0 && col < RowCols(row);
}

TileId Tilemap::GetTile(int row, int col) const noexcept {
    if (!InBounds(row, col)) {
        return TileId::Wall;
    }
    const int raw = grid_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
    return IntToTileId(raw);
}

bool Tilemap::SetTile(int row, int col, TileId id) noexcept {
    if (!InBounds(row, col)) {
        return false;
    }
    grid_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = static_cast<int>(id);
    return true;
}

bool Tilemap::IsSolid(int row, int col, bool bridge_unlocked) const noexcept {
    switch (GetTile(row, col)) {
        case TileId::Wall:
        case TileId::Water:
        case TileId::RuinsWall:
            return true;
        case TileId::HiddenBridge:
            return !bridge_unlocked;
        case TileId::Walkable:
        case TileId::StructurePortal:
        case TileId::ExitPortal:
        case TileId::PavedPath:
        case TileId::SanctuaryFloor:
        case TileId::GardenFloor:
            return false;
    }
    return true;
}

TileCoord Tilemap::WorldToTile(Vec2 world_pos) const noexcept {
    if (!std::isfinite(world_pos.x) || !std::isfinite(world_pos.y) || tile_size_ <= 0.0f) {
        return {-1, -1};
    }
    const float row_f = std::floor(world_pos.y / tile_size_);
    const float col_f = std::floor(world_pos.x / tile_size_);
    constexpr float kMaxSafeCoord = 1e9f;
    if (!std::isfinite(row_f) || !std::isfinite(col_f) ||
        std::abs(row_f) > kMaxSafeCoord || std::abs(col_f) > kMaxSafeCoord) {
        return {-1, -1};
    }
    return {static_cast<int>(row_f), static_cast<int>(col_f)};
}

Rect Tilemap::TileRect(int row, int col) const noexcept {
    return {
        static_cast<float>(col) * tile_size_,
        static_cast<float>(row) * tile_size_,
        tile_size_,
        tile_size_
    };
}

Vec2 Tilemap::TileCenter(int row, int col) const noexcept {
    return {
        (static_cast<float>(col) + 0.5f) * tile_size_,
        (static_cast<float>(row) + 0.5f) * tile_size_
    };
}

TileRange Tilemap::OverlappingTileRange(const Rect& aabb, bool include_oob_border) const noexcept {
    if (Rows() == 0 || max_cols_ == 0 || tile_size_ <= 0.0f || !IsValidFiniteRect(aabb)) {
        return {};
    }

    const float left = aabb.Left();
    const float right = aabb.Right();
    const float top = aabb.Top();
    const float bottom = aabb.Bottom();

    const float min_col_f = std::floor(left / tile_size_) - 1.0f;
    const float max_col_f = std::ceil(right / tile_size_);
    const float min_row_f = std::floor(top / tile_size_) - 1.0f;
    const float max_row_f = std::ceil(bottom / tile_size_);

    if (std::isnan(min_col_f) || std::isnan(max_col_f) ||
        std::isnan(min_row_f) || std::isnan(max_row_f)) {
        return {};
    }

    const int lo_i = include_oob_border ? -1 : 0;
    const int hi_row_i = include_oob_border ? Rows() : (Rows() - 1);
    const int hi_col_i = include_oob_border ? max_cols_ : (max_cols_ - 1);

    const float lo_f = static_cast<float>(lo_i);
    const float hi_row_f = static_cast<float>(hi_row_i);
    const float hi_col_f = static_cast<float>(hi_col_i);

    if (max_row_f < lo_f || min_row_f > hi_row_f || max_col_f < lo_f || min_col_f > hi_col_f) {
        return {};
    }

    int min_row = static_cast<int>(std::clamp(min_row_f, lo_f, hi_row_f));
    int max_row = static_cast<int>(std::clamp(max_row_f, lo_f, hi_row_f));
    int min_col = static_cast<int>(std::clamp(min_col_f, lo_f, hi_col_f));
    int max_col = static_cast<int>(std::clamp(max_col_f, lo_f, hi_col_f));

    while (min_col <= max_col && TileRect(0, min_col).Right() <= left) {
        ++min_col;
    }
    while (max_col >= min_col && TileRect(0, max_col).Left() >= right) {
        --max_col;
    }
    while (min_row <= max_row && TileRect(min_row, 0).Bottom() <= top) {
        ++min_row;
    }
    while (max_row >= min_row && TileRect(max_row, 0).Top() >= bottom) {
        --max_row;
    }

    if (min_row > max_row || min_col > max_col) {
        return {};
    }

    return {min_row, max_row, min_col, max_col};
}

std::vector<TileCoord> Tilemap::OverlappingTiles(const Rect& aabb) const {
    std::vector<TileCoord> result;
    const TileRange range = OverlappingTileRange(aabb, false);
    if (range.Empty()) {
        return result;
    }
    for (int r = range.min_row; r <= range.max_row; ++r) {
        for (int c = range.min_col; c <= range.max_col; ++c) {
            if (InBounds(r, c) && aabb.Overlaps(TileRect(r, c))) {
                result.push_back({r, c});
            }
        }
    }
    return result;
}

bool Tilemap::OverlapsSolid(const Rect& aabb, bool bridge_unlocked) const noexcept {
    if (Rows() == 0 || max_cols_ == 0 || !IsValidFiniteRect(aabb)) {
        return false;
    }
    if (aabb.Left() < 0.0f || aabb.Top() < 0.0f ||
        aabb.Right() > WorldWidth() || aabb.Bottom() > WorldHeight()) {
        return true;
    }
    const TileRange range = OverlappingTileRange(aabb, true);
    if (range.Empty()) {
        return false;
    }
    for (int r = range.min_row; r <= range.max_row; ++r) {
        for (int c = range.min_col; c <= range.max_col; ++c) {
            if (IsSolid(r, c, bridge_unlocked) && aabb.Overlaps(TileRect(r, c))) {
                return true;
            }
        }
    }
    return false;
}

bool Tilemap::OverlapsTileId(const Rect& aabb, TileId tile_id) const noexcept {
    const TileRange range = OverlappingTileRange(aabb, false);
    if (range.Empty()) {
        return false;
    }
    for (int r = range.min_row; r <= range.max_row; ++r) {
        for (int c = range.min_col; c <= range.max_col; ++c) {
            if (InBounds(r, c) && GetTile(r, c) == tile_id && aabb.Overlaps(TileRect(r, c))) {
                return true;
            }
        }
    }
    return false;
}

std::vector<TileCoord> Tilemap::FindTiles(TileId tile_id) const {
    std::vector<TileCoord> matches;
    for (int r = 0; r < Rows(); ++r) {
        const int cols = RowCols(r);
        for (int c = 0; c < cols; ++c) {
            if (GetTile(r, c) == tile_id) {
                matches.push_back({r, c});
            }
        }
    }
    return matches;
}

} // namespace eco
