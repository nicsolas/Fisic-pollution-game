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

void FillRectFlat(
    std::vector<std::uint8_t>& cells,
    int rows,
    int cols,
    int r0,
    int r1,
    int c0,
    int c1,
    TileId id) noexcept {
    const auto val = static_cast<std::uint8_t>(id);
    const int r_start = std::max(0, r0);
    const int r_end = std::min(rows - 1, r1);
    const int c_start = std::max(0, c0);
    const int c_end = std::min(cols - 1, c1);
    for (int r = r_start; r <= r_end; ++r) {
        const std::size_t row_off = static_cast<std::size_t>(r) * static_cast<std::size_t>(cols);
        for (int c = c_start; c <= c_end; ++c) {
            cells[row_off + static_cast<std::size_t>(c)] = val;
        }
    }
}

void SetCellFlat(
    std::vector<std::uint8_t>& cells,
    int rows,
    int cols,
    int r,
    int c,
    TileId id) noexcept {
    if (r < 0 || r >= rows || c < 0 || c >= cols) {
        return;
    }
    const std::size_t idx =
        static_cast<std::size_t>(r) * static_cast<std::size_t>(cols) + static_cast<std::size_t>(c);
    cells[idx] = static_cast<std::uint8_t>(id);
}

[[nodiscard]] Tilemap CreateHypogeanCisternsMap(float tile_size) {
    // 16 rows x 34 cols (768 x 1632 px) — Cisterne Ipogee
    constexpr int kRows = 16;
    constexpr int kCols = 34;
    std::vector<std::uint8_t> cells(static_cast<std::size_t>(kRows * kCols), 0);

    FillRectFlat(cells, kRows, kCols, 0, 0, 0, kCols - 1, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, kRows - 2, kRows - 1, 0, kCols - 1, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 0, kRows - 1, 0, 0, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 0, kRows - 1, kCols - 1, kCols - 1, TileId::Wall);

    // ExitPortal on ground floor left side (row 13, cols 2..3)
    SetCellFlat(cells, kRows, kCols, 13, 2, TileId::ExitPortal);
    SetCellFlat(cells, kRows, kCols, 13, 3, TileId::ExitPortal);

    // Ascending aqueduct stone ledges (every vertical jump step <= 2 tiles = 96 px)
    FillRectFlat(cells, kRows, kCols, 12, 12, 16, 20, TileId::Wall); // Tier 1
    FillRectFlat(cells, kRows, kCols, 10, 10, 22, 26, TileId::Wall); // Tier 2 East
    FillRectFlat(cells, kRows, kCols, 8, 8, 24, 31, TileId::Wall);   // Tier 3 Archivist Gallery (Lorenzo + Relic)
    FillRectFlat(cells, kRows, kCols, 10, 10, 8, 14, TileId::Wall);  // Tier 2 West Cistern Ledge (Relic)

    return Tilemap(kRows, kCols, std::move(cells), tile_size);
}

[[nodiscard]] Tilemap CreatePendulumTowerMap(float tile_size) {
    // 18 rows x 30 cols (864 x 1440 px) — Torre del Pendolo
    constexpr int kRows = 18;
    constexpr int kCols = 30;
    std::vector<std::uint8_t> cells(static_cast<std::size_t>(kRows * kCols), 0);

    FillRectFlat(cells, kRows, kCols, 0, 0, 0, kCols - 1, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, kRows - 2, kRows - 1, 0, kCols - 1, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 0, kRows - 1, 0, 0, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 0, kRows - 1, kCols - 1, kCols - 1, TileId::Wall);

    // ExitPortal on ground floor left side (row 15, cols 2..3)
    SetCellFlat(cells, kRows, kCols, 15, 2, TileId::ExitPortal);
    SetCellFlat(cells, kRows, kCols, 15, 3, TileId::ExitPortal);

    // Percorso principale continuo a scalini larghi. Ogni gradino ha ampio margine
    // orizzontale e sovrapposizione con il successivo per rendere la salita leggibile.
    FillRectFlat(cells, kRows, kCols, 15, 15, 5, 9, TileId::Wall);   // Gradino d'ingresso
    FillRectFlat(cells, kRows, kCols, 14, 14, 8, 12, TileId::Wall);  // Primo ripiano
    FillRectFlat(cells, kRows, kCols, 13, 13, 12, 16, TileId::Wall); // Svolta centrale
    FillRectFlat(cells, kRows, kCols, 12, 12, 16, 20, TileId::Wall); // Ripiano centrale
    FillRectFlat(cells, kRows, kCols, 11, 11, 19, 24, TileId::Wall); // Svolta verso Est
    FillRectFlat(cells, kRows, kCols, 10, 10, 18, 24, TileId::Wall); // Corridoio superiore
    FillRectFlat(cells, kRows, kCols, 9, 9, 17, 22, TileId::Wall);   // Avvicinamento al nucleo
    FillRectFlat(cells, kRows, kCols, 8, 8, 10, 20, TileId::Wall);   // Galleria del Pendolo
    FillRectFlat(cells, kRows, kCols, 7, 7, 3, 9, TileId::Wall);     // Gradino reliquia Ovest
    FillRectFlat(cells, kRows, kCols, 7, 7, 21, 27, TileId::Wall);   // Gradino reliquia Est
    FillRectFlat(cells, kRows, kCols, 6, 6, 3, 9, TileId::Wall);     // Tier 5 West Belfry (Relic)
    FillRectFlat(cells, kRows, kCols, 6, 6, 21, 27, TileId::Wall);   // Tier 5 East Belfry (Relic)

    return Tilemap(kRows, kCols, std::move(cells), tile_size);
}

[[nodiscard]] Tilemap CreateGeothermalChamberMap(float tile_size) {
    // 16 rows x 34 cols (768 x 1632 px) — Camera Geotermica
    constexpr int kRows = 16;
    constexpr int kCols = 34;
    std::vector<std::uint8_t> cells(static_cast<std::size_t>(kRows * kCols), 0);

    FillRectFlat(cells, kRows, kCols, 0, 0, 0, kCols - 1, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, kRows - 2, kRows - 1, 0, kCols - 1, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 0, kRows - 1, 0, 0, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 0, kRows - 1, kCols - 1, kCols - 1, TileId::Wall);

    // ExitPortal on ground floor left side (row 13, cols 2..3)
    SetCellFlat(cells, kRows, kCols, 13, 2, TileId::ExitPortal);
    SetCellFlat(cells, kRows, kCols, 13, 3, TileId::ExitPortal);

    // Basalt geothermal bridges & steam catwalks (every vertical step <= 2 tiles)
    FillRectFlat(cells, kRows, kCols, 12, 12, 10, 15, TileId::Wall); // Tier 1
    FillRectFlat(cells, kRows, kCols, 10, 10, 17, 22, TileId::Wall); // Tier 2
    FillRectFlat(cells, kRows, kCols, 10, 10, 24, 30, TileId::Wall); // Tier 3 Carnot Exchanger Deck
    FillRectFlat(cells, kRows, kCols, 8, 8, 12, 18, TileId::Wall);   // Tier 4 Upper Basalt Catwalk
    FillRectFlat(cells, kRows, kCols, 6, 6, 4, 10, TileId::Wall);    // Tier 5 Upper West Alcove (Relic)

    return Tilemap(kRows, kCols, std::move(cells), tile_size);
}

} // namespace

Tilemap::Tilemap(std::vector<std::vector<int>> grid, float tile_size)
    : tile_size_((std::isfinite(tile_size) && tile_size > 0.0f) ? tile_size : kTileSize) {
    rows_ = static_cast<int>(grid.size());
    int max_c = 0;
    for (const auto& row : grid) {
        max_c = std::max(max_c, static_cast<int>(row.size()));
    }
    cols_ = max_c;
    if (rows_ <= 0 || cols_ <= 0) {
        rows_ = 0;
        cols_ = 0;
        return;
    }
    cells_.assign(
        static_cast<std::size_t>(rows_) * static_cast<std::size_t>(cols_),
        static_cast<std::uint8_t>(TileId::Wall));
    for (int r = 0; r < rows_; ++r) {
        const auto& src_row = grid[static_cast<std::size_t>(r)];
        const std::size_t row_off = static_cast<std::size_t>(r) * static_cast<std::size_t>(cols_);
        const int row_len = static_cast<int>(src_row.size());
        for (int c = 0; c < row_len; ++c) {
            cells_[row_off + static_cast<std::size_t>(c)] =
                static_cast<std::uint8_t>(IntToTileId(src_row[static_cast<std::size_t>(c)]));
        }
    }
}

Tilemap::Tilemap(int rows, int cols, std::vector<std::uint8_t> cells, float tile_size)
    : cells_(std::move(cells)),
      rows_(std::max(0, rows)),
      cols_(std::max(0, cols)),
      tile_size_((std::isfinite(tile_size) && tile_size > 0.0f) ? tile_size : kTileSize) {
    const std::size_t expected =
        static_cast<std::size_t>(rows_) * static_cast<std::size_t>(cols_);
    if (expected == 0) {
        rows_ = 0;
        cols_ = 0;
        cells_.clear();
    } else if (cells_.size() != expected) {
        cells_.resize(expected, static_cast<std::uint8_t>(TileId::Wall));
    }
}

std::array<DistrictGateway, Tilemap::kOverworldGatewayCount> Tilemap::OverworldGateways(
    float tile_size) noexcept {
    const float ts = (std::isfinite(tile_size) && tile_size > 0.0f) ? tile_size : kTileSize;
    return {{
        // 0: Terrazze dell'Alba (SO) <-> Quartiere dei Canali (CO)
        DistrictGateway{
            GatewayOrientation::Horizontal,
            {23, 10},
            {24, 12},
            {11.5f * ts, 24.0f * ts},
            ZoneId::DawnTerraces,
            ZoneId::CanalQuarter,
            "N -> CANALI",
            "S -> TERRAZZE",
            false
        },
        // 1: Quartiere dei Canali (CO) <-> Altopiano dei Venti (NO)
        DistrictGateway{
            GatewayOrientation::Horizontal,
            {11, 10},
            {12, 12},
            {11.5f * ts, 12.0f * ts},
            ZoneId::CanalQuarter,
            ZoneId::WindPlateau,
            "N -> VENTO",
            "S -> CANALI",
            false
        },
        // 2: Ponte degli Archi — Quartiere dei Canali (CO) <-> Giardini Sospesi (CE)
        DistrictGateway{
            GatewayOrientation::Vertical,
            {16, kBridgeDividerCol},
            {18, kBridgeDividerCol},
            {24.5f * ts, 17.5f * ts},
            ZoneId::CanalQuarter,
            ZoneId::HangingGardens,
            "E -> GIARDINI",
            "O -> CANALI",
            true
        },
        // 3: Giardini Sospesi (CE) <-> Caldera delle Fumarole (NE)
        DistrictGateway{
            GatewayOrientation::Horizontal,
            {11, 35},
            {12, 37},
            {36.5f * ts, 12.0f * ts},
            ZoneId::HangingGardens,
            ZoneId::FumaroleCaldera,
            "N -> CALDERA",
            "S -> GIARDINI",
            false
        },
        // 4: Giardini Sospesi (CE) <-> Santuario dell'Armonia (SE)
        DistrictGateway{
            GatewayOrientation::Horizontal,
            {23, 35},
            {24, 37},
            {36.5f * ts, 24.0f * ts},
            ZoneId::HangingGardens,
            ZoneId::HarmonySanctuary,
            "S -> SANTUARIO",
            "N -> GIARDINI",
            false
        }
    }};
}

Tilemap Tilemap::CreateOverworldMap(float tile_size) {
    // 36 rows x 50 cols (1728 x 2400 px at kTileSize = 48.0f) — La Conca di Meridia
    constexpr int kRows = kOverworldRows;
    constexpr int kCols = kOverworldCols;
    std::vector<std::uint8_t> cells(static_cast<std::size_t>(kRows * kCols), 0);

    // 1. District ground layers (walkable decorative surfaces)
    // Giardini Sospesi (Centro-Est) base flora
    FillRectFlat(cells, kRows, kCols, 13, 22, 26, 47, TileId::GardenFloor);
    // Terrazze dell'Alba (SO) garden alcoves & sundial court
    FillRectFlat(cells, kRows, kCols, 25, 26, 14, 21, TileId::GardenFloor);
    FillRectFlat(cells, kRows, kCols, 32, 34, 2, 8, TileId::GardenFloor);
    FillRectFlat(cells, kRows, kCols, 27, 31, 9, 13, TileId::SanctuaryFloor);
    // Quartiere dei Canali olive courtyard
    FillRectFlat(cells, kRows, kCols, 20, 22, 3, 5, TileId::GardenFloor);
    // Santuario dell'Armonia (SE) astronomical court & flanking gardens
    FillRectFlat(cells, kRows, kCols, 25, 34, 26, 29, TileId::GardenFloor);
    FillRectFlat(cells, kRows, kCols, 25, 34, 45, 48, TileId::GardenFloor);
    FillRectFlat(cells, kRows, kCols, 26, 33, 30, 44, TileId::SanctuaryFloor);

    // 2. Paved stone avenues connecting all 6 districts along the guided progression
    FillRectFlat(cells, kRows, kCols, 4, 31, 10, 12, TileId::PavedPath);  // West North-South spine
    FillRectFlat(cells, kRows, kCols, 4, 31, 35, 37, TileId::PavedPath);  // East North-South spine
    FillRectFlat(cells, kRows, kCols, 16, 18, 5, 23, TileId::PavedPath);  // West-East avenue (West side)
    FillRectFlat(cells, kRows, kCols, 15, 16, 5, 6, TileId::PavedPath);   // Spur to Cisterne Ipogee portal
    FillRectFlat(cells, kRows, kCols, 16, 18, 25, 44, TileId::PavedPath); // West-East avenue (East side)
    FillRectFlat(cells, kRows, kCols, 29, 30, 4, 9, TileId::PavedPath);   // Dawn spawn to sundial
    FillRectFlat(cells, kRows, kCols, 29, 30, 13, 18, TileId::PavedPath); // Dawn east terrace walk
    FillRectFlat(cells, kRows, kCols, 8, 8, 12, 21, TileId::PavedPath);   // WindPlateau east overlook
    FillRectFlat(cells, kRows, kCols, 8, 8, 31, 40, TileId::PavedPath);   // FumaroleCaldera furnace square
    FillRectFlat(cells, kRows, kCols, 15, 19, 34, 38, TileId::SanctuaryFloor); // HangingGardens resonator pavilion

    // 3. Horizontal District Border Walls (2 tiles thick: rows 11..12 and rows 23..24)
    // North/Mid ridge (rows 11..12): separates WindPlateau/CanalQuarter and FumaroleCaldera/HangingGardens
    FillRectFlat(cells, kRows, kCols, 11, 12, 1, 23, TileId::RuinsWall);
    FillRectFlat(cells, kRows, kCols, 11, 12, 1, 3, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 11, 12, 20, 23, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 11, 12, 25, 48, TileId::RuinsWall);
    FillRectFlat(cells, kRows, kCols, 11, 12, 25, 27, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 11, 12, 44, 48, TileId::Wall);

    // Mid/South ridge (rows 23..24): separates CanalQuarter/DawnTerraces and HangingGardens/HarmonySanctuary
    FillRectFlat(cells, kRows, kCols, 23, 24, 1, 23, TileId::RuinsWall);
    FillRectFlat(cells, kRows, kCols, 23, 24, 1, 3, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 23, 24, 20, 23, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 23, 24, 25, 48, TileId::RuinsWall);
    FillRectFlat(cells, kRows, kCols, 23, 24, 25, 27, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 23, 24, 46, 48, TileId::Wall);

    // Carve the 4 horizontal-wall District Gateways (each 3 tiles wide, paved)
    FillRectFlat(cells, kRows, kCols, 22, 25, 10, 12, TileId::PavedPath); // DawnTerraces <-> CanalQuarter
    FillRectFlat(cells, kRows, kCols, 10, 13, 10, 12, TileId::PavedPath); // CanalQuarter <-> WindPlateau
    FillRectFlat(cells, kRows, kCols, 10, 13, 35, 37, TileId::PavedPath); // HangingGardens <-> FumaroleCaldera
    FillRectFlat(cells, kRows, kCols, 22, 25, 35, 37, TileId::PavedPath); // HangingGardens <-> HarmonySanctuary

    // 4. Central Canal & Ponte degli Archi at col 24
    FillRectFlat(cells, kRows, kCols, 1, 4, kBridgeDividerCol, kBridgeDividerCol, TileId::RuinsWall);
    FillRectFlat(cells, kRows, kCols, 5, 15, kBridgeDividerCol, kBridgeDividerCol, TileId::Water);
    FillRectFlat(cells, kRows, kCols, 16, 18, kBridgeDividerCol, kBridgeDividerCol, TileId::HiddenBridge);
    FillRectFlat(cells, kRows, kCols, 19, 30, kBridgeDividerCol, kBridgeDividerCol, TileId::Water);
    FillRectFlat(cells, kRows, kCols, 31, 34, kBridgeDividerCol, kBridgeDividerCol, TileId::RuinsWall);

    // 5. Water canals & reflecting basins
    // Quartiere dei Canali flanking aqueduct pools
    FillRectFlat(cells, kRows, kCols, 13, 14, 18, 22, TileId::Water);
    FillRectFlat(cells, kRows, kCols, 20, 21, 18, 22, TileId::Water);
    // Giardini Sospesi reflecting pools
    FillRectFlat(cells, kRows, kCols, 14, 15, 28, 30, TileId::Water);
    FillRectFlat(cells, kRows, kCols, 20, 21, 28, 30, TileId::Water);
    // Santuario dell'Armonia side basins
    FillRectFlat(cells, kRows, kCols, 27, 28, 27, 27, TileId::Water);
    FillRectFlat(cells, kRows, kCols, 27, 28, 47, 47, TileId::Water);

    // 6. Ancient Ruins, Colonnades & Basalt Outcrops (solid RuinsWall / Wall)
    // DawnTerraces ruins
    FillRectFlat(cells, kRows, kCols, 25, 26, 4, 6, TileId::RuinsWall);
    FillRectFlat(cells, kRows, kCols, 32, 33, 15, 17, TileId::RuinsWall);
    // CanalQuarter aqueduct piers & west fountain wall
    SetCellFlat(cells, kRows, kCols, 15, 19, TileId::RuinsWall);
    SetCellFlat(cells, kRows, kCols, 15, 21, TileId::RuinsWall);
    SetCellFlat(cells, kRows, kCols, 19, 19, TileId::RuinsWall);
    SetCellFlat(cells, kRows, kCols, 19, 21, TileId::RuinsWall);
    FillRectFlat(cells, kRows, kCols, 19, 20, 7, 8, TileId::RuinsWall);
    // WindPlateau windbreak cliffs
    FillRectFlat(cells, kRows, kCols, 4, 5, 4, 5, TileId::RuinsWall);
    FillRectFlat(cells, kRows, kCols, 7, 8, 15, 17, TileId::RuinsWall);
    FillRectFlat(cells, kRows, kCols, 3, 4, 19, 20, TileId::RuinsWall);
    // HangingGardens pergola pillars
    SetCellFlat(cells, kRows, kCols, 13, 41, TileId::RuinsWall);
    SetCellFlat(cells, kRows, kCols, 13, 45, TileId::RuinsWall);
    SetCellFlat(cells, kRows, kCols, 21, 41, TileId::RuinsWall);
    SetCellFlat(cells, kRows, kCols, 21, 45, TileId::RuinsWall);
    // FumaroleCaldera broken furnace stacks
    FillRectFlat(cells, kRows, kCols, 4, 5, 28, 30, TileId::RuinsWall);
    FillRectFlat(cells, kRows, kCols, 7, 8, 42, 44, TileId::RuinsWall);
    FillRectFlat(cells, kRows, kCols, 2, 2, 43, 46, TileId::Wall);
    // HarmonySanctuary symmetrical colonnade pillars
    SetCellFlat(cells, kRows, kCols, 26, 29, TileId::RuinsWall);
    SetCellFlat(cells, kRows, kCols, 30, 29, TileId::RuinsWall);
    SetCellFlat(cells, kRows, kCols, 26, 45, TileId::RuinsWall);
    SetCellFlat(cells, kRows, kCols, 30, 45, TileId::RuinsWall);

    // 7. Three Dedicated Structure Portals (framed stone alcoves opening south)
    // Portal 1: Cisterne Ipogee (Quartiere dei Canali, row 14, cols 5..6)
    FillRectFlat(cells, kRows, kCols, 13, 13, 4, 7, TileId::RuinsWall);
    SetCellFlat(cells, kRows, kCols, 14, 4, TileId::RuinsWall);
    SetCellFlat(cells, kRows, kCols, 14, 5, TileId::StructurePortal);
    SetCellFlat(cells, kRows, kCols, 14, 6, TileId::StructurePortal);
    SetCellFlat(cells, kRows, kCols, 14, 7, TileId::RuinsWall);

    // Portal 2: Torre del Pendolo (Altopiano dei Venti, row 3, cols 11..12)
    FillRectFlat(cells, kRows, kCols, 2, 2, 10, 13, TileId::RuinsWall);
    SetCellFlat(cells, kRows, kCols, 3, 10, TileId::RuinsWall);
    SetCellFlat(cells, kRows, kCols, 3, 11, TileId::StructurePortal);
    SetCellFlat(cells, kRows, kCols, 3, 12, TileId::StructurePortal);
    SetCellFlat(cells, kRows, kCols, 3, 13, TileId::RuinsWall);

    // Portal 3: Camera Geotermica (Caldera delle Fumarole, row 3, cols 37..38)
    FillRectFlat(cells, kRows, kCols, 2, 2, 36, 39, TileId::Wall);
    SetCellFlat(cells, kRows, kCols, 3, 36, TileId::Wall);
    SetCellFlat(cells, kRows, kCols, 3, 37, TileId::StructurePortal);
    SetCellFlat(cells, kRows, kCols, 3, 38, TileId::StructurePortal);
    SetCellFlat(cells, kRows, kCols, 3, 39, TileId::Wall);

    // 8. Outer perimeter walls
    FillRectFlat(cells, kRows, kCols, 0, 0, 0, kCols - 1, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, kRows - 1, kRows - 1, 0, kCols - 1, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 0, kRows - 1, 0, 0, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 0, kRows - 1, kCols - 1, kCols - 1, TileId::Wall);

    return Tilemap(kRows, kCols, std::move(cells), tile_size);
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
    const int east_cols = std::max(1, cols_ - east_start);
    const int band_rows = 12;

    switch (zone) {
        case ZoneId::WindPlateau:
            return {0.0f, 0.0f, static_cast<float>(west_cols) * ts, static_cast<float>(band_rows) * ts};
        case ZoneId::CanalQuarter:
            return {0.0f, static_cast<float>(band_rows) * ts, static_cast<float>(west_cols) * ts, static_cast<float>(band_rows) * ts};
        case ZoneId::DawnTerraces:
            return {0.0f, static_cast<float>(2 * band_rows) * ts, static_cast<float>(west_cols) * ts, static_cast<float>(std::max(1, rows_ - 2 * band_rows)) * ts};
        case ZoneId::FumaroleCaldera:
            return {static_cast<float>(east_start) * ts, 0.0f, static_cast<float>(east_cols) * ts, static_cast<float>(band_rows) * ts};
        case ZoneId::HangingGardens:
            return {static_cast<float>(east_start) * ts, static_cast<float>(band_rows) * ts, static_cast<float>(east_cols) * ts, static_cast<float>(band_rows) * ts};
        case ZoneId::HarmonySanctuary:
            return {static_cast<float>(east_start) * ts, static_cast<float>(2 * band_rows) * ts, static_cast<float>(east_cols) * ts, static_cast<float>(std::max(1, rows_ - 2 * band_rows)) * ts};
        case ZoneId::Count:
            break;
    }
    return {0.0f, 0.0f, WorldWidth(), WorldHeight()};
}

int Tilemap::Rows() const noexcept {
    return rows_;
}

int Tilemap::Cols() const noexcept {
    return cols_;
}

int Tilemap::RowCols(int row) const noexcept {
    if (row < 0 || row >= rows_) {
        return 0;
    }
    return cols_;
}

float Tilemap::TileSize() const noexcept {
    return tile_size_;
}

float Tilemap::WorldWidth() const noexcept {
    return static_cast<float>(cols_) * tile_size_;
}

float Tilemap::WorldHeight() const noexcept {
    return static_cast<float>(rows_) * tile_size_;
}

bool Tilemap::InBounds(int row, int col) const noexcept {
    return row >= 0 && row < rows_ && col >= 0 && col < cols_;
}

TileId Tilemap::GetTile(int row, int col) const noexcept {
    if (!InBounds(row, col)) {
        return TileId::Wall;
    }
    const std::size_t idx =
        static_cast<std::size_t>(row) * static_cast<std::size_t>(cols_) + static_cast<std::size_t>(col);
    return IntToTileId(static_cast<int>(cells_[idx]));
}

bool Tilemap::SetTile(int row, int col, TileId id) noexcept {
    if (!InBounds(row, col)) {
        return false;
    }
    const std::size_t idx =
        static_cast<std::size_t>(row) * static_cast<std::size_t>(cols_) + static_cast<std::size_t>(col);
    cells_[idx] = static_cast<std::uint8_t>(id);
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
    if (rows_ == 0 || cols_ == 0 || tile_size_ <= 0.0f || !IsValidFiniteRect(aabb)) {
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
    const int hi_row_i = include_oob_border ? rows_ : (rows_ - 1);
    const int hi_col_i = include_oob_border ? cols_ : (cols_ - 1);

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
    result.reserve(
        static_cast<std::size_t>(range.max_row - range.min_row + 1) *
        static_cast<std::size_t>(range.max_col - range.min_col + 1));
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
    if (rows_ == 0 || cols_ == 0 || !IsValidFiniteRect(aabb)) {
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
    const auto target = static_cast<std::uint8_t>(tile_id);
    for (int r = 0; r < rows_; ++r) {
        const std::size_t row_off = static_cast<std::size_t>(r) * static_cast<std::size_t>(cols_);
        for (int c = 0; c < cols_; ++c) {
            if (cells_[row_off + static_cast<std::size_t>(c)] == target) {
                matches.push_back({r, c});
            }
        }
    }
    return matches;
}

} // namespace eco
