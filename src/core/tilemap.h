#pragma once

#include <vector>

#include "core/math_types.h"

namespace eco {

struct TileCoord {
    int row = 0;
    int col = 0;

    [[nodiscard]] constexpr bool operator==(const TileCoord&) const noexcept = default;
};

struct TileRange {
    int min_row = 0;
    int max_row = -1;
    int min_col = 0;
    int max_col = -1;

    [[nodiscard]] constexpr bool Empty() const noexcept {
        return max_row < min_row || max_col < min_col;
    }
};

class Tilemap {
public:
    static constexpr int kOverworldRows = 36;
    static constexpr int kOverworldCols = 50;
    static constexpr int kBridgeDividerCol = 24;

    Tilemap() = default;
    explicit Tilemap(std::vector<std::vector<int>> grid, float tile_size = kTileSize);

    [[nodiscard]] static Tilemap CreateOverworldMap(float tile_size = kTileSize);
    [[nodiscard]] static Tilemap CreateStructureMap(
        StructureId structure_id, float tile_size = kTileSize);
    [[nodiscard]] static Tilemap CreateStructureMap(float tile_size = kTileSize);

    [[nodiscard]] static ZoneId DistrictAtTile(int row, int col) noexcept;
    [[nodiscard]] ZoneId DistrictAt(Vec2 world_pos) const noexcept;
    [[nodiscard]] Rect DistrictBounds(ZoneId zone) const noexcept;

    [[nodiscard]] int Rows() const noexcept;
    [[nodiscard]] int Cols() const noexcept;
    [[nodiscard]] int RowCols(int row) const noexcept;
    [[nodiscard]] float TileSize() const noexcept;
    [[nodiscard]] float WorldWidth() const noexcept;
    [[nodiscard]] float WorldHeight() const noexcept;
    [[nodiscard]] bool InBounds(int row, int col) const noexcept;
    [[nodiscard]] const std::vector<std::vector<int>>& Grid() const noexcept { return grid_; }

    [[nodiscard]] TileId GetTile(int row, int col) const noexcept;
    bool SetTile(int row, int col, TileId id) noexcept;
    [[nodiscard]] bool IsSolid(int row, int col, bool bridge_unlocked = false) const noexcept;

    [[nodiscard]] TileCoord WorldToTile(Vec2 world_pos) const noexcept;
    [[nodiscard]] Rect TileRect(int row, int col) const noexcept;
    [[nodiscard]] Vec2 TileCenter(int row, int col) const noexcept;

    [[nodiscard]] TileRange OverlappingTileRange(
        const Rect& aabb, bool include_oob_border = false) const noexcept;

    [[nodiscard]] std::vector<TileCoord> OverlappingTiles(const Rect& aabb) const;
    [[nodiscard]] bool OverlapsSolid(const Rect& aabb, bool bridge_unlocked = false) const noexcept;
    [[nodiscard]] bool OverlapsTileId(const Rect& aabb, TileId tile_id) const noexcept;
    [[nodiscard]] std::vector<TileCoord> FindTiles(TileId tile_id) const;

private:
    std::vector<std::vector<int>> grid_{};
    float tile_size_ = kTileSize;
    int max_cols_ = 0;
};

} // namespace eco
