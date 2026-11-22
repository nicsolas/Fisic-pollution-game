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
        default:
            return TileId::Wall;
    }
}

[[nodiscard]] bool IsValidFiniteRect(const Rect& aabb) noexcept {
    return std::isfinite(aabb.x) && std::isfinite(aabb.y) &&
           std::isfinite(aabb.width) && std::isfinite(aabb.height) &&
           aabb.width > 0.0f && aabb.height > 0.0f;
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
    // 18 rows x 28 cols (864 x 1344 px at kTileSize = 48.0f)
    // 0 = Walkable, 1 = Wall, 2 = StructurePortal, 4 = HiddenBridge
    constexpr int kRows = 18;
    constexpr int kCols = 28;
    std::vector<std::vector<int>> grid(kRows, std::vector<int>(kCols, 0));

    // Perimeter walls
    for (int c = 0; c < kCols; ++c) {
        grid[0][static_cast<std::size_t>(c)] = 1;
        grid[kRows - 1][static_cast<std::size_t>(c)] = 1;
    }
    for (int r = 0; r < kRows; ++r) {
        grid[static_cast<std::size_t>(r)][0] = 1;
        grid[static_cast<std::size_t>(r)][kCols - 1] = 1;
    }

    // Interior L-shaped ruin obstacle (rows 3..5, cols 6..7 + row 5, col 8)
    for (int r = 3; r <= 5; ++r) {
        grid[static_cast<std::size_t>(r)][6] = 1;
        grid[static_cast<std::size_t>(r)][7] = 1;
    }
    grid[5][8] = 1;

    // Vertical wall pillar east of spawn plaza (rows 7..9, col 12) for immediate X/corner slide
    for (int r = 7; r <= 9; ++r) {
        grid[static_cast<std::size_t>(r)][12] = 1;
    }

    // Southern island pillar (rows 12..13, cols 9..11) for horizontal/vertical sliding
    for (int r = 12; r <= 13; ++r) {
        for (int c = 9; c <= 11; ++c) {
            grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = 1;
        }
    }

    // StructurePortal alcove (row 3..4, cols 14..17), open walkable approach south (rows 5..7)
    for (int c = 14; c <= 17; ++c) {
        grid[3][static_cast<std::size_t>(c)] = 1;
    }
    grid[4][14] = 1;
    grid[4][15] = 2; // StructurePortal
    grid[4][16] = 2; // StructurePortal
    grid[4][17] = 1;

    // Eastern divider wall at col 20 with 2-tile HiddenBridge (rows 8..9, col 20)
    // separating the eastern courtyard (cols 21..26, rows 1..16).
    for (int r = 1; r <= kRows - 2; ++r) {
        grid[static_cast<std::size_t>(r)][20] = 1;
    }
    grid[8][20] = 4; // HiddenBridge
    grid[9][20] = 4; // HiddenBridge

    return Tilemap(std::move(grid), tile_size);
}

Tilemap Tilemap::CreateStructureMap(float tile_size) {
    // 15 rows x 28 cols (720 x 1344 px at kTileSize = 48.0f)
    // 0 = Walkable, 1 = Wall, 3 = ExitPortal
    constexpr int kRows = 15;
    constexpr int kCols = 28;
    std::vector<std::vector<int>> grid(kRows, std::vector<int>(kCols, 0));

    // Perimeter walls & ceiling
    for (int c = 0; c < kCols; ++c) {
        grid[0][static_cast<std::size_t>(c)] = 1;
        grid[kRows - 2][static_cast<std::size_t>(c)] = 1; // Floor row 13 (top at y = 624)
        grid[kRows - 1][static_cast<std::size_t>(c)] = 1;
    }
    for (int r = 0; r < kRows; ++r) {
        grid[static_cast<std::size_t>(r)][0] = 1;
        grid[static_cast<std::size_t>(r)][kCols - 1] = 1;
    }

    // ExitPortal on ground floor left side (row 12, cols 2..3)
    grid[12][2] = 3;
    grid[12][3] = 3;

    // Stepped platform tiers for jump, coyote time, jump buffer, and ceiling collision
    for (int c = 8; c <= 12; ++c) {
        grid[10][static_cast<std::size_t>(c)] = 1;
    }
    for (int c = 14; c <= 18; ++c) {
        grid[8][static_cast<std::size_t>(c)] = 1;
    }
    for (int c = 20; c <= 24; ++c) {
        grid[6][static_cast<std::size_t>(c)] = 1;
    }

    return Tilemap(std::move(grid), tile_size);
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
            return true;
        case TileId::HiddenBridge:
            return !bridge_unlocked;
        case TileId::Walkable:
        case TileId::StructurePortal:
        case TileId::ExitPortal:
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

    // Pad by 1 tile on each side so float division vs TileRect multiplication ULP
    // discrepancies never miss an actually overlapping tile, then tighten below.
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

    // Tighten padded bounds against exact TileRect boundaries so OverlappingTileRange
    // matches aabb.Overlaps(TileRect(r, c)) bit-for-bit across all fractional tile sizes.
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
