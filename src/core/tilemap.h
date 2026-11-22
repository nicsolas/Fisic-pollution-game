#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
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

enum class GatewayOrientation : std::uint8_t {
    Horizontal = 0, // Varco N-S in una scarpata/muro orizzontale
    Vertical = 1    // Varco E-O nel canale verticale (Ponte degli Archi)
};

struct DistrictGateway {
    GatewayOrientation orientation = GatewayOrientation::Horizontal;
    TileCoord tile_min{0, 0};
    TileCoord tile_max{0, 0};
    Vec2 world_center{0.0f, 0.0f};
    ZoneId from_zone = ZoneId::DawnTerraces;
    ZoneId to_zone = ZoneId::CanalQuarter;
    const char* signpost_label = "";
    const char* reverse_label = "";
    bool requires_bridge_unlock = false;

    [[nodiscard]] constexpr bool IsHorizontalWall() const noexcept {
        return orientation == GatewayOrientation::Horizontal;
    }
};

class Tilemap {
public:
    static constexpr int kOverworldRows = 36;
    static constexpr int kOverworldCols = 50;
    static constexpr int kBridgeDividerCol = 24;
    static constexpr int kNorthDividerRow = 11; // rows 11..12
    static constexpr int kSouthDividerRow = 23; // rows 23..24
    static constexpr std::size_t kOverworldGatewayCount = 5;

    Tilemap() = default;
    explicit Tilemap(std::vector<std::vector<int>> grid, float tile_size = kTileSize);
    Tilemap(int rows, int cols, std::vector<std::uint8_t> cells, float tile_size = kTileSize);

    [[nodiscard]] static Tilemap CreateOverworldMap(float tile_size = kTileSize);
    [[nodiscard]] static Tilemap CreateStructureMap(
        StructureId structure_id, float tile_size = kTileSize);
    [[nodiscard]] static Tilemap CreateStructureMap(float tile_size = kTileSize);
    [[nodiscard]] static std::array<DistrictGateway, kOverworldGatewayCount> OverworldGateways(
        float tile_size = kTileSize) noexcept;

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
    [[nodiscard]] std::span<const std::uint8_t> RawData() const noexcept { return cells_; }
    [[nodiscard]] std::size_t MemoryBytes() const noexcept {
        return cells_.capacity() * sizeof(std::uint8_t);
    }

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
    std::vector<std::uint8_t> cells_{};
    int rows_ = 0;
    int cols_ = 0;
    float tile_size_ = kTileSize;
};

} // namespace eco
