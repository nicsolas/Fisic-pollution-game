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
        case 10:
            return TileId::ZoneGateway;
        case 11:
            return TileId::ShallowChannel;
        case 12:
            return TileId::JadeBridge;
        case 13:
            return TileId::SolarGate;
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

void AddZoneBorder(std::vector<std::uint8_t>& cells, int rows, int cols, TileId border_id) noexcept {
    FillRectFlat(cells, rows, cols, 0, 0, 0, cols - 1, border_id);
    FillRectFlat(cells, rows, cols, rows - 1, rows - 1, 0, cols - 1, border_id);
    FillRectFlat(cells, rows, cols, 0, rows - 1, 0, 0, border_id);
    FillRectFlat(cells, rows, cols, 0, rows - 1, cols - 1, cols - 1, border_id);
}

// ============================================================================
// 6 Mappe Dedicate delle Zone (26 righe x 36 colonne = 1248 x 1728 px)
// ============================================================================

[[nodiscard]] Tilemap CreateDawnTerracesMap(float tile_size) {
    constexpr int kR = Tilemap::kZoneRows;
    constexpr int kC = Tilemap::kZoneCols;
    std::vector<std::uint8_t> cells(static_cast<std::size_t>(kR * kC), static_cast<std::uint8_t>(TileId::Walkable));

    AddZoneBorder(cells, kR, kC, TileId::Wall);

    // Varco Nord verso Quartiere dei Canali (r=1, c=16..19)
    FillRectFlat(cells, kR, kC, 1, 1, 16, 19, TileId::ZoneGateway);
    FillRectFlat(cells, kR, kC, 2, 11, 17, 18, TileId::PavedPath);

    // Piazza circolare/ottagonale della Meridiana e dell'Eliostato Solare (centro r=13, c=18)
    FillRectFlat(cells, kR, kC, 10, 16, 15, 21, TileId::SanctuaryFloor);
    FillRectFlat(cells, kR, kC, 12, 14, 6, 27, TileId::PavedPath);
    FillRectFlat(cells, kR, kC, 16, 21, 5, 18, TileId::PavedPath);

    // Terrazze e corridoi per i fasci solari (emettitore a (6,6) -> specchi (6,14), (14,14), (14,24) -> piastre (6,24), (20,24))
    FillRectFlat(cells, kR, kC, 5, 7, 5, 25, TileId::PavedPath);
    FillRectFlat(cells, kR, kC, 6, 21, 13, 15, TileId::PavedPath);
    FillRectFlat(cells, kR, kC, 6, 21, 23, 25, TileId::PavedPath);

    // Rovine architettoniche che incorniciano i fasci senza bloccarli
    FillRectFlat(cells, kR, kC, 3, 4, 8, 12, TileId::RuinsWall);
    FillRectFlat(cells, kR, kC, 8, 9, 8, 11, TileId::RuinsWall);
    FillRectFlat(cells, kR, kC, 18, 21, 19, 20, TileId::RuinsWall);

    // Vasca riflettente a sud-ovest
    FillRectFlat(cells, kR, kC, 21, 23, 2, 7, TileId::Water);

    // Alcova Orientale dell'Iscrizione & Reliquia protetta dal SolarGate (col 28)
    FillRectFlat(cells, kR, kC, 2, 10, 28, 28, TileId::RuinsWall);
    FillRectFlat(cells, kR, kC, 15, 23, 28, 28, TileId::RuinsWall);
    FillRectFlat(cells, kR, kC, 11, 14, 28, 28, TileId::SolarGate);
    FillRectFlat(cells, kR, kC, 9, 17, 29, 33, TileId::SanctuaryFloor);

    return Tilemap(kR, kC, std::move(cells), tile_size);
}

[[nodiscard]] Tilemap CreateCanalQuarterMap(float tile_size) {
    constexpr int kR = Tilemap::kZoneRows;
    constexpr int kC = Tilemap::kZoneCols;
    std::vector<std::uint8_t> cells(static_cast<std::size_t>(kR * kC), static_cast<std::uint8_t>(TileId::Walkable));

    AddZoneBorder(cells, kR, kC, TileId::Wall);

    // Varchi: Sud -> DawnTerraces, Nord -> WindPlateau, Est -> HangingGardens (Ponte degli Archi)
    FillRectFlat(cells, kR, kC, 24, 24, 16, 19, TileId::ZoneGateway);
    FillRectFlat(cells, kR, kC, 1, 1, 16, 19, TileId::ZoneGateway);

    // Gran Canale Orientale (separa il Ponte degli Archi a col 32..34)
    FillRectFlat(cells, kR, kC, 1, 24, 32, 33, TileId::Water);
    // Ponte degli Archi (sbloccabile con Clara Sereni = HiddenBridge) + Varco Est
    FillRectFlat(cells, kR, kC, 11, 14, 32, 33, TileId::HiddenBridge);
    FillRectFlat(cells, kR, kC, 11, 14, 34, 34, TileId::ZoneGateway);

    // Due canali guadabili con correnti direzionali (ShallowChannel)
    // Canale Ovest-Centro (r=11..13, c=4..27) interrotto da passerelle
    FillRectFlat(cells, kR, kC, 11, 13, 4, 14, TileId::ShallowChannel);
    FillRectFlat(cells, kR, kC, 11, 13, 20, 27, TileId::ShallowChannel);
    // Canale Sud-Est (r=18..20, c=10..27)
    FillRectFlat(cells, kR, kC, 18, 20, 10, 27, TileId::ShallowChannel);

    // Viale centrale lastricato N-S e banchina verso Clara Sereni e Cisterne Ipogee
    FillRectFlat(cells, kR, kC, 2, 23, 16, 19, TileId::PavedPath);
    FillRectFlat(cells, kR, kC, 11, 14, 16, 31, TileId::PavedPath);
    FillRectFlat(cells, kR, kC, 6, 11, 6, 16, TileId::PavedPath);

    // Archi e pilastri d'acquedotto
    FillRectFlat(cells, kR, kC, 3, 4, 12, 14, TileId::RuinsWall);
    FillRectFlat(cells, kR, kC, 3, 4, 21, 23, TileId::RuinsWall);
    FillRectFlat(cells, kR, kC, 15, 16, 6, 8, TileId::RuinsWall);
    FillRectFlat(cells, kR, kC, 21, 22, 24, 26, TileId::RuinsWall);

    // Portale Cisterne Ipogee (r=6, c=7..8)
    SetCellFlat(cells, kR, kC, 6, 7, TileId::StructurePortal);
    SetCellFlat(cells, kR, kC, 6, 8, TileId::StructurePortal);

    return Tilemap(kR, kC, std::move(cells), tile_size);
}

[[nodiscard]] Tilemap CreateWindPlateauMap(float tile_size) {
    constexpr int kR = Tilemap::kZoneRows;
    constexpr int kC = Tilemap::kZoneCols;
    std::vector<std::uint8_t> cells(static_cast<std::size_t>(kR * kC), static_cast<std::uint8_t>(TileId::Walkable));

    AddZoneBorder(cells, kR, kC, TileId::Wall);

    // Varco Sud -> Quartiere dei Canali
    FillRectFlat(cells, kR, kC, 24, 24, 16, 19, TileId::ZoneGateway);

    // Sentiero esposto alle raffiche che sale verso la Torre del Pendolo
    FillRectFlat(cells, kR, kC, 5, 23, 16, 19, TileId::PavedPath);
    FillRectFlat(cells, kR, kC, 10, 14, 8, 30, TileId::PavedPath);
    FillRectFlat(cells, kR, kC, 4, 9, 14, 21, TileId::SanctuaryFloor);

    // 5 Monoliti frangivento in basalto (RuinsWall solidi, allineati con ZoneMechanicState::windbreaks)
    FillRectFlat(cells, kR, kC, 18, 20, 21, 21, TileId::RuinsWall); // Monolite 1
    FillRectFlat(cells, kR, kC, 15, 17, 13, 13, TileId::RuinsWall); // Monolite 2
    FillRectFlat(cells, kR, kC, 11, 13, 22, 22, TileId::RuinsWall); // Monolite 3
    FillRectFlat(cells, kR, kC, 8, 10, 12, 12, TileId::RuinsWall);  // Monolite 4
    FillRectFlat(cells, kR, kC, 11, 13, 27, 27, TileId::RuinsWall); // Monolite 5 (riparo verso reliquia Est)

    // Bordi scoscesi laterali
    FillRectFlat(cells, kR, kC, 2, 5, 2, 6, TileId::Wall);
    FillRectFlat(cells, kR, kC, 2, 5, 29, 33, TileId::Wall);
    FillRectFlat(cells, kR, kC, 20, 23, 2, 5, TileId::Water);

    // Portale Torre del Pendolo (r=5, c=17..18)
    SetCellFlat(cells, kR, kC, 5, 17, TileId::StructurePortal);
    SetCellFlat(cells, kR, kC, 5, 18, TileId::StructurePortal);

    return Tilemap(kR, kC, std::move(cells), tile_size);
}

[[nodiscard]] Tilemap CreateHangingGardensMap(float tile_size) {
    constexpr int kR = Tilemap::kZoneRows;
    constexpr int kC = Tilemap::kZoneCols;
    std::vector<std::uint8_t> cells(static_cast<std::size_t>(kR * kC), static_cast<std::uint8_t>(TileId::GardenFloor));

    AddZoneBorder(cells, kR, kC, TileId::Wall);

    // Varchi: Ovest -> CanalQuarter, Nord -> FumaroleCaldera, Sud -> HarmonySanctuary
    FillRectFlat(cells, kR, kC, 11, 14, 1, 1, TileId::ZoneGateway);
    FillRectFlat(cells, kR, kC, 1, 1, 16, 19, TileId::ZoneGateway);
    FillRectFlat(cells, kR, kC, 24, 24, 16, 19, TileId::ZoneGateway);

    // Viali principali N-S e Ovest -> terrazza delle piastre armoniche
    FillRectFlat(cells, kR, kC, 2, 23, 16, 19, TileId::PavedPath);
    FillRectFlat(cells, kR, kC, 11, 14, 2, 21, TileId::PavedPath);

    // Specchio d'acqua che separa la terrazza orientale del Risonatore Armonico (col 21..22)
    FillRectFlat(cells, kR, kC, 4, 21, 21, 22, TileId::Water);
    // Ponte di Giada centrale sbloccabile con la sequenza di risonanza armonica (r=11..14, c=21..22)
    FillRectFlat(cells, kR, kC, 11, 14, 21, 22, TileId::JadeBridge);
    // Passaggio botanico nord/sud sempre percorribile per esplorazione libera e compatibilita'
    FillRectFlat(cells, kR, kC, 2, 3, 20, 26, TileId::PavedPath);
    FillRectFlat(cells, kR, kC, 22, 23, 20, 26, TileId::PavedPath);

    // Terrazza orientale del Risonatore Armonico (r=9..16, c=24..30)
    FillRectFlat(cells, kR, kC, 9, 16, 24, 30, TileId::SanctuaryFloor);
    FillRectFlat(cells, kR, kC, 4, 21, 26, 28, TileId::PavedPath);

    // Vasca centrale decorativa a ovest delle piastre
    FillRectFlat(cells, kR, kC, 6, 8, 6, 9, TileId::Water);
    FillRectFlat(cells, kR, kC, 17, 19, 6, 9, TileId::Water);

    return Tilemap(kR, kC, std::move(cells), tile_size);
}

[[nodiscard]] Tilemap CreateFumaroleCalderaMap(float tile_size) {
    constexpr int kR = Tilemap::kZoneRows;
    constexpr int kC = Tilemap::kZoneCols;
    std::vector<std::uint8_t> cells(static_cast<std::size_t>(kR * kC), static_cast<std::uint8_t>(TileId::Walkable));

    AddZoneBorder(cells, kR, kC, TileId::Wall);

    // Varco Sud -> Giardini Sospesi
    FillRectFlat(cells, kR, kC, 24, 24, 16, 19, TileId::ZoneGateway);

    // Sentieri di basalto fra le bocche fumarole verso la Camera Geotermica (r=5, c=17..18)
    FillRectFlat(cells, kR, kC, 6, 23, 16, 19, TileId::PavedPath);
    FillRectFlat(cells, kR, kC, 14, 16, 7, 28, TileId::PavedPath);
    FillRectFlat(cells, kR, kC, 9, 11, 10, 31, TileId::PavedPath);
    FillRectFlat(cells, kR, kC, 4, 8, 14, 21, TileId::SanctuaryFloor);

    // Colonne di basalto e rovine delle fornaci
    FillRectFlat(cells, kR, kC, 4, 6, 5, 8, TileId::RuinsWall);
    FillRectFlat(cells, kR, kC, 4, 6, 27, 30, TileId::RuinsWall);
    FillRectFlat(cells, kR, kC, 19, 21, 5, 9, TileId::RuinsWall);
    FillRectFlat(cells, kR, kC, 19, 21, 26, 30, TileId::RuinsWall);

    // Bacini termali ai lati
    FillRectFlat(cells, kR, kC, 11, 13, 3, 5, TileId::Water);
    FillRectFlat(cells, kR, kC, 14, 17, 31, 33, TileId::Water);

    // Portale Camera Geotermica (r=5, c=17..18)
    SetCellFlat(cells, kR, kC, 5, 17, TileId::StructurePortal);
    SetCellFlat(cells, kR, kC, 5, 18, TileId::StructurePortal);

    return Tilemap(kR, kC, std::move(cells), tile_size);
}

[[nodiscard]] Tilemap CreateHarmonySanctuaryMap(float tile_size) {
    constexpr int kR = Tilemap::kZoneRows;
    constexpr int kC = Tilemap::kZoneCols;
    std::vector<std::uint8_t> cells(static_cast<std::size_t>(kR * kC), static_cast<std::uint8_t>(TileId::SanctuaryFloor));

    AddZoneBorder(cells, kR, kC, TileId::Wall);

    // Varco Nord -> Giardini Sospesi
    FillRectFlat(cells, kR, kC, 1, 1, 16, 19, TileId::ZoneGateway);
    FillRectFlat(cells, kR, kC, 2, 7, 16, 19, TileId::PavedPath);

    // Specchi d'acqua sacri ai quattro angoli del Santuario
    FillRectFlat(cells, kR, kC, 3, 6, 3, 7, TileId::Water);
    FillRectFlat(cells, kR, kC, 3, 6, 28, 32, TileId::Water);
    FillRectFlat(cells, kR, kC, 20, 23, 3, 7, TileId::Water);
    FillRectFlat(cells, kR, kC, 20, 23, 28, 32, TileId::Water);

    // Colonnato simmetrico attorno al mosaico astronomico dell'Astrolabio Centrale (13, 18)
    SetCellFlat(cells, kR, kC, 6, 11, TileId::RuinsWall);
    SetCellFlat(cells, kR, kC, 6, 24, TileId::RuinsWall);
    SetCellFlat(cells, kR, kC, 19, 11, TileId::RuinsWall);
    SetCellFlat(cells, kR, kC, 19, 24, TileId::RuinsWall);

    // Aiuole dei semi risvegliati
    FillRectFlat(cells, kR, kC, 11, 15, 3, 6, TileId::GardenFloor);
    FillRectFlat(cells, kR, kC, 11, 15, 29, 32, TileId::GardenFloor);

    return Tilemap(kR, kC, std::move(cells), tile_size);
}

// ============================================================================
// 3 Mappe delle Strutture Side-Scrolling (invariate)
// ============================================================================

[[nodiscard]] Tilemap CreateHypogeanCisternsMap(float tile_size) {
    constexpr int kRows = 16;
    constexpr int kCols = 34;
    std::vector<std::uint8_t> cells(static_cast<std::size_t>(kRows * kCols), 0);

    FillRectFlat(cells, kRows, kCols, 0, 0, 0, kCols - 1, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, kRows - 2, kRows - 1, 0, kCols - 1, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 0, kRows - 1, 0, 0, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 0, kRows - 1, kCols - 1, kCols - 1, TileId::Wall);

    SetCellFlat(cells, kRows, kCols, 13, 2, TileId::ExitPortal);
    SetCellFlat(cells, kRows, kCols, 13, 3, TileId::ExitPortal);

    FillRectFlat(cells, kRows, kCols, 12, 12, 16, 20, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 10, 10, 22, 26, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 8, 8, 24, 31, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 10, 10, 8, 14, TileId::Wall);

    return Tilemap(kRows, kCols, std::move(cells), tile_size);
}

[[nodiscard]] Tilemap CreatePendulumTowerMap(float tile_size) {
    constexpr int kRows = 18;
    constexpr int kCols = 30;
    std::vector<std::uint8_t> cells(static_cast<std::size_t>(kRows * kCols), 0);

    FillRectFlat(cells, kRows, kCols, 0, 0, 0, kCols - 1, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, kRows - 2, kRows - 1, 0, kCols - 1, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 0, kRows - 1, 0, 0, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 0, kRows - 1, kCols - 1, kCols - 1, TileId::Wall);

    SetCellFlat(cells, kRows, kCols, 15, 2, TileId::ExitPortal);
    SetCellFlat(cells, kRows, kCols, 15, 3, TileId::ExitPortal);

    FillRectFlat(cells, kRows, kCols, 15, 15, 7, 12, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 14, 14, 10, 17, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 13, 13, 10, 20, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 6, 6, 3, 9, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 6, 6, 21, 27, TileId::Wall);

    return Tilemap(kRows, kCols, std::move(cells), tile_size);
}

[[nodiscard]] Tilemap CreateGeothermalChamberMap(float tile_size) {
    constexpr int kRows = 16;
    constexpr int kCols = 34;
    std::vector<std::uint8_t> cells(static_cast<std::size_t>(kRows * kCols), 0);

    FillRectFlat(cells, kRows, kCols, 0, 0, 0, kCols - 1, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, kRows - 2, kRows - 1, 0, kCols - 1, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 0, kRows - 1, 0, 0, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 0, kRows - 1, kCols - 1, kCols - 1, TileId::Wall);

    SetCellFlat(cells, kRows, kCols, 13, 2, TileId::ExitPortal);
    SetCellFlat(cells, kRows, kCols, 13, 3, TileId::ExitPortal);

    FillRectFlat(cells, kRows, kCols, 12, 12, 10, 15, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 10, 10, 17, 22, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 10, 10, 24, 30, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 8, 8, 12, 18, TileId::Wall);
    FillRectFlat(cells, kRows, kCols, 6, 6, 4, 10, TileId::Wall);

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
        cells_.shrink_to_fit();
    } else if (cells_.size() != expected) {
        cells_.resize(expected, static_cast<std::uint8_t>(TileId::Wall));
    }
}

Tilemap Tilemap::CreateZoneMap(ZoneId zone, float tile_size) {
    switch (zone) {
        case ZoneId::DawnTerraces:
            return CreateDawnTerracesMap(tile_size);
        case ZoneId::CanalQuarter:
            return CreateCanalQuarterMap(tile_size);
        case ZoneId::WindPlateau:
            return CreateWindPlateauMap(tile_size);
        case ZoneId::HangingGardens:
            return CreateHangingGardensMap(tile_size);
        case ZoneId::FumaroleCaldera:
            return CreateFumaroleCalderaMap(tile_size);
        case ZoneId::HarmonySanctuary:
            return CreateHarmonySanctuaryMap(tile_size);
        case ZoneId::Count:
            break;
    }
    return CreateDawnTerracesMap(tile_size);
}

std::vector<ZoneGateway> Tilemap::ZoneGateways(ZoneId zone, float tile_size) {
    const float ts = (std::isfinite(tile_size) && tile_size > 0.0f) ? tile_size : kTileSize;
    std::vector<ZoneGateway> out;
    switch (zone) {
        case ZoneId::DawnTerraces:
            out.push_back(ZoneGateway{
                GatewayOrientation::Horizontal,
                {1, 16},
                {1, 19},
                {18.0f * ts, 1.5f * ts},
                ZoneId::DawnTerraces,
                ZoneId::CanalQuarter,
                {22, 17},
                "N -> CANALI",
                false
            });
            break;
        case ZoneId::CanalQuarter:
            out.push_back(ZoneGateway{
                GatewayOrientation::Horizontal,
                {24, 16},
                {24, 19},
                {18.0f * ts, 24.5f * ts},
                ZoneId::CanalQuarter,
                ZoneId::DawnTerraces,
                {3, 17},
                "S -> TERRAZZE",
                false
            });
            out.push_back(ZoneGateway{
                GatewayOrientation::Horizontal,
                {1, 16},
                {1, 19},
                {18.0f * ts, 1.5f * ts},
                ZoneId::CanalQuarter,
                ZoneId::WindPlateau,
                {22, 17},
                "N -> VENTO",
                false
            });
            out.push_back(ZoneGateway{
                GatewayOrientation::Vertical,
                {11, 34},
                {14, 34},
                {34.5f * ts, 13.0f * ts},
                ZoneId::CanalQuarter,
                ZoneId::HangingGardens,
                {12, 3},
                "E -> GIARDINI",
                true
            });
            break;
        case ZoneId::WindPlateau:
            out.push_back(ZoneGateway{
                GatewayOrientation::Horizontal,
                {24, 16},
                {24, 19},
                {18.0f * ts, 24.5f * ts},
                ZoneId::WindPlateau,
                ZoneId::CanalQuarter,
                {3, 17},
                "S -> CANALI",
                false
            });
            break;
        case ZoneId::HangingGardens:
            out.push_back(ZoneGateway{
                GatewayOrientation::Vertical,
                {11, 1},
                {14, 1},
                {1.5f * ts, 13.0f * ts},
                ZoneId::HangingGardens,
                ZoneId::CanalQuarter,
                {12, 30},
                "O -> CANALI",
                false
            });
            out.push_back(ZoneGateway{
                GatewayOrientation::Horizontal,
                {1, 16},
                {1, 19},
                {18.0f * ts, 1.5f * ts},
                ZoneId::HangingGardens,
                ZoneId::FumaroleCaldera,
                {22, 17},
                "N -> CALDERA",
                false
            });
            out.push_back(ZoneGateway{
                GatewayOrientation::Horizontal,
                {24, 16},
                {24, 19},
                {18.0f * ts, 24.5f * ts},
                ZoneId::HangingGardens,
                ZoneId::HarmonySanctuary,
                {3, 17},
                "S -> SANTUARIO",
                false
            });
            break;
        case ZoneId::FumaroleCaldera:
            out.push_back(ZoneGateway{
                GatewayOrientation::Horizontal,
                {24, 16},
                {24, 19},
                {18.0f * ts, 24.5f * ts},
                ZoneId::FumaroleCaldera,
                ZoneId::HangingGardens,
                {3, 17},
                "S -> GIARDINI",
                false
            });
            break;
        case ZoneId::HarmonySanctuary:
            out.push_back(ZoneGateway{
                GatewayOrientation::Horizontal,
                {1, 16},
                {1, 19},
                {18.0f * ts, 1.5f * ts},
                ZoneId::HarmonySanctuary,
                ZoneId::HangingGardens,
                {22, 17},
                "N -> GIARDINI",
                false
            });
            break;
        case ZoneId::Count:
            break;
    }
    return out;
}

TileCoord Tilemap::DefaultZoneSpawnTile(ZoneId zone) noexcept {
    switch (zone) {
        case ZoneId::DawnTerraces:
            return {18, 6};
        case ZoneId::CanalQuarter:
            return {17, 17};
        case ZoneId::WindPlateau:
            return {21, 17};
        case ZoneId::HangingGardens:
            return {12, 6};
        case ZoneId::FumaroleCaldera:
            return {21, 17};
        case ZoneId::HarmonySanctuary:
            return {6, 17};
        case ZoneId::Count:
            break;
    }
    return {18, 6};
}

Tilemap Tilemap::CreateOverworldMap(float tile_size) {
    return CreateZoneMap(ZoneId::DawnTerraces, tile_size);
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

std::array<DistrictGateway, Tilemap::kOverworldGatewayCount> Tilemap::OverworldGateways(
    float tile_size) noexcept {
    const float ts = (std::isfinite(tile_size) && tile_size > 0.0f) ? tile_size : kTileSize;
    return {{
        DistrictGateway{
            GatewayOrientation::Horizontal,
            {1, 16},
            {1, 19},
            {18.0f * ts, 1.5f * ts},
            ZoneId::DawnTerraces,
            ZoneId::CanalQuarter,
            "N -> CANALI",
            "S -> TERRAZZE",
            false
        },
        DistrictGateway{
            GatewayOrientation::Horizontal,
            {1, 16},
            {1, 19},
            {18.0f * ts, 1.5f * ts},
            ZoneId::CanalQuarter,
            ZoneId::WindPlateau,
            "N -> VENTO",
            "S -> CANALI",
            false
        },
        DistrictGateway{
            GatewayOrientation::Vertical,
            {11, 34},
            {14, 34},
            {34.5f * ts, 13.0f * ts},
            ZoneId::CanalQuarter,
            ZoneId::HangingGardens,
            "E -> GIARDINI",
            "O -> CANALI",
            true
        },
        DistrictGateway{
            GatewayOrientation::Horizontal,
            {1, 16},
            {1, 19},
            {18.0f * ts, 1.5f * ts},
            ZoneId::HangingGardens,
            ZoneId::FumaroleCaldera,
            "N -> CALDERA",
            "S -> GIARDINI",
            false
        },
        DistrictGateway{
            GatewayOrientation::Horizontal,
            {24, 16},
            {24, 19},
            {18.0f * ts, 24.5f * ts},
            ZoneId::HangingGardens,
            ZoneId::HarmonySanctuary,
            "S -> SANTUARIO",
            "N -> GIARDINI",
            false
        }
    }};
}

ZoneId Tilemap::DistrictAtTile(int row, int col) noexcept {
    (void)row;
    (void)col;
    return ZoneId::DawnTerraces;
}

ZoneId Tilemap::DistrictAt(Vec2 world_pos) const noexcept {
    (void)world_pos;
    return ZoneId::DawnTerraces;
}

Rect Tilemap::DistrictBounds(ZoneId zone) const noexcept {
    (void)zone;
    return {0.0f, 0.0f, WorldWidth(), WorldHeight()};
}

void Tilemap::ReleaseMemory() noexcept {
    cells_.clear();
    cells_.shrink_to_fit();
    rows_ = 0;
    cols_ = 0;
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

bool Tilemap::IsSolid(
    int row,
    int col,
    bool bridge_unlocked,
    bool solar_gate_open,
    bool jade_bridge_unlocked) const noexcept {
    switch (GetTile(row, col)) {
        case TileId::Wall:
        case TileId::Water:
        case TileId::RuinsWall:
            return true;
        case TileId::HiddenBridge:
            return !bridge_unlocked;
        case TileId::SolarGate:
            return !solar_gate_open;
        case TileId::JadeBridge:
            return !jade_bridge_unlocked;
        case TileId::Walkable:
        case TileId::StructurePortal:
        case TileId::ExitPortal:
        case TileId::PavedPath:
        case TileId::SanctuaryFloor:
        case TileId::GardenFloor:
        case TileId::ZoneGateway:
        case TileId::ShallowChannel:
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

bool Tilemap::OverlapsSolid(
    const Rect& aabb,
    bool bridge_unlocked,
    bool solar_gate_open,
    bool jade_bridge_unlocked) const noexcept {
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
            if (IsSolid(r, c, bridge_unlocked, solar_gate_open, jade_bridge_unlocked) &&
                aabb.Overlaps(TileRect(r, c))) {
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
