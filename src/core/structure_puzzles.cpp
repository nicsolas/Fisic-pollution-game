#include "core/structure_puzzles.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace eco {

namespace {

void FillRect(
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
        const std::size_t row_off =
            static_cast<std::size_t>(r) * static_cast<std::size_t>(cols);
        for (int c = c_start; c <= c_end; ++c) {
            cells[row_off + static_cast<std::size_t>(c)] = val;
        }
    }
}

void SetCell(
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
        static_cast<std::size_t>(r) * static_cast<std::size_t>(cols) +
        static_cast<std::size_t>(c);
    cells[idx] = static_cast<std::uint8_t>(id);
}

void BuildOuterShell(
    std::vector<std::uint8_t>& cells,
    int rows,
    int cols) noexcept {
    // 2-tile thick ceiling, floor, and side walls
    FillRect(cells, rows, cols, 0, 1, 0, cols - 1, TileId::Wall);
    FillRect(cells, rows, cols, rows - 2, rows - 1, 0, cols - 1, TileId::Wall);
    FillRect(cells, rows, cols, 0, rows - 1, 0, 1, TileId::Wall);
    FillRect(cells, rows, cols, 0, rows - 1, cols - 2, cols - 1, TileId::Wall);
}

[[nodiscard]] Vec2 TileCenterPos(TileCoord coord, float tile_size) noexcept {
    return {
        (static_cast<float>(coord.col) + 0.5f) * tile_size,
        (static_cast<float>(coord.row) + 0.5f) * tile_size
    };
}

[[nodiscard]] StructureSwitch MakeSwitch(
    int id,
    TileCoord tile,
    const char* symbol,
    float tile_size,
    float interact_radius = 64.0f) noexcept {
    StructureSwitch sw{};
    sw.id = id;
    sw.tile = tile;
    sw.pos = TileCenterPos(tile, tile_size);
    sw.interact_radius = interact_radius;
    sw.active = false;
    sw.is_momentary = false;
    sw.symbol = symbol;
    return sw;
}

[[nodiscard]] std::vector<TileCoord> MakeHorizontalSpan(int row, int c0, int c1) {
    std::vector<TileCoord> out;
    if (c1 < c0) {
        return out;
    }
    out.reserve(static_cast<std::size_t>(c1 - c0 + 1));
    for (int c = c0; c <= c1; ++c) {
        out.push_back({row, c});
    }
    return out;
}

[[nodiscard]] std::vector<TileCoord> MakeVerticalSpan(int r0, int r1, int col) {
    std::vector<TileCoord> out;
    if (r1 < r0) {
        return out;
    }
    out.reserve(static_cast<std::size_t>(r1 - r0 + 1));
    for (int r = r0; r <= r1; ++r) {
        out.push_back({r, col});
    }
    return out;
}

// ============================================================================
// 1. HypogeanCisterns (Cisterne Ipogee) - 20x40 - "Equilibrio delle Paratoie"
// ============================================================================
[[nodiscard]] Tilemap BuildHypogeanCisternsMap(float tile_size) {
    const StructureSpawnLayout layout = GetStructureLayout(StructureId::HypogeanCisterns);
    const int kR = layout.rows; // 20
    const int kC = layout.cols; // 40
    std::vector<std::uint8_t> cells(
        static_cast<std::size_t>(kR * kC),
        static_cast<std::uint8_t>(TileId::Walkable));

    BuildOuterShell(cells, kR, kC);

    // Exit portal on west ground floor (row 17, cols 2..3)
    for (const TileCoord& pt : layout.exit_portal_tiles) {
        SetCell(cells, kR, kC, pt.row, pt.col, TileId::ExitPortal);
    }

    // West Entry Hall ledges climbing to Switch A (13, 5)
    FillRect(cells, kR, kC, 16, 16, 8, 11, TileId::Wall); // climb 18->16 (2 tiles)
    FillRect(cells, kR, kC, 14, 14, 4, 7, TileId::Wall);  // climb 16->14 (2 tiles)

    // Upper-West Aqueduct holding Switch C (9, 6) and hidden Relic 2 (9, 3), unreachable from below (14->10 = 4 tiles)
    FillRect(cells, kR, kC, 10, 10, 2, 10, TileId::Wall);
    // Decorative ceiling arch overhang (rows 2..6, col 4); rows 7..9 (3 tiles) stay wide open to walk to (9, 3)
    FillRect(cells, kR, kC, 2, 6, 4, 4, TileId::Wall);

    // Central Sluice Divider Wall (col 16):
    // - Upper ceiling masonry rows 2..6
    // - Open aqueduct archway rows 7..9 (3 tiles tall) above Bridge 1 (row 10, cols 12..16)
    // - Mid masonry rows 11..14
    // - Gate 1 occupies rows 15..17 at col 16
    FillRect(cells, kR, kC, 2, 6, 16, 16, TileId::Wall);
    FillRect(cells, kR, kC, 11, 14, 16, 16, TileId::Wall);

    // Central Cistern platforms (Switch B at (15, 22), climbing up to Bridge 1 at row 10)
    FillRect(cells, kR, kC, 16, 16, 20, 23, TileId::Wall); // climb 18->16 (2 tiles, clean headroom under row 14)
    FillRect(cells, kR, kC, 14, 14, 24, 27, TileId::Wall); // climb 16->14 (2 tiles, 0 gap)
    FillRect(cells, kR, kC, 12, 12, 20, 23, TileId::Wall); // climb 14->12 (2 tiles)
    FillRect(cells, kR, kC, 10, 10, 17, 19, TileId::Wall); // climb 12->10 (2 tiles)

    // East Turbine Vault Divider Wall (col 28):
    // - Solid masonry rows 2..14
    // - Gate 2 occupies rows 15..17 at col 28
    FillRect(cells, kR, kC, 2, 14, 28, 28, TileId::Wall);

    // Elevated dais inside Turbine Vault (cols 33..37, floor at row 17 -> standing row 16)
    // Lorenzo stands at (17, 30); HydroTurbine at (16, 36).
    FillRect(cells, kR, kC, 17, 17, 33, 37, TileId::Wall);

    Tilemap map(kR, kC, std::move(cells), tile_size);

    StructurePuzzleState initial_state{};
    initial_state.ResetForStructure(StructureId::HypogeanCisterns, tile_size);
    initial_state.ApplyGatesToTilemap(map);
    return map;
}

// ============================================================================
// 2. PendulumTower (Torre del Pendolo) - 22x38 - "Sequenza Isocrona"
// ============================================================================
[[nodiscard]] Tilemap BuildPendulumTowerMap(float tile_size) {
    const StructureSpawnLayout layout = GetStructureLayout(StructureId::PendulumTower);
    const int kR = layout.rows; // 22
    const int kC = layout.cols; // 38
    std::vector<std::uint8_t> cells(
        static_cast<std::size_t>(kR * kC),
        static_cast<std::uint8_t>(TileId::Walkable));

    BuildOuterShell(cells, kR, kC);

    // Exit portal on ground floor (row 19, cols 2..3)
    for (const TileCoord& pt : layout.exit_portal_tiles) {
        SetCell(cells, kR, kC, pt.row, pt.col, TileId::ExitPortal);
    }

    // Ground-Right Vault housing PendulumRegulator at (19, 33)
    // Vault roof at row 15 (cols 24..35) also serves as Mid-Right Balcony for Chime I (14, 31)
    FillRect(cells, kR, kC, 15, 15, 24, 35, TileId::Wall);
    FillRect(cells, kR, kC, 16, 16, 28, 28, TileId::Wall); // Lintel above Gate 3 (rows 17..19, col 28)

    // Tier 1 climb: Ground (row 20) -> Mid-Right Balcony (row 15)
    FillRect(cells, kR, kC, 18, 18, 12, 16, TileId::Wall); // climb 20->18 (2 tiles)
    FillRect(cells, kR, kC, 16, 16, 18, 22, TileId::Wall); // climb 18->16 (2 tiles, 1-tile gap)
    // (16, 22) -> (15, 24) is 1-tile climb, 1-tile gap

    // Tier 2 climb: Mid-Right Balcony (row 15) -> Mid-Left Gallery (row 11) for Chime II (10, 5)
    FillRect(cells, kR, kC, 13, 13, 20, 23, TileId::Wall); // climb 15->13 (2 tiles, 0 gap)
    FillRect(cells, kR, kC, 12, 12, 12, 17, TileId::Wall); // climb 13->12 (1 tile, 2-tile gap)
    FillRect(cells, kR, kC, 11, 11, 3, 10, TileId::Wall);  // climb 12->11 (1 tile, 1-tile gap)

    // Tier 3 approach: Mid-Left Gallery (row 11) -> Bridge 1 (row 9, cols 15..21) -> Upper-Right Belfry (row 7)
    FillRect(cells, kR, kC, 9, 9, 11, 13, TileId::Wall);   // climb 11->9 (2 tiles, 0 gap, clean headroom)
    FillRect(cells, kR, kC, 9, 9, 23, 26, TileId::Wall);   // landing after Bridge 1 (1-tile gap each side)
    FillRect(cells, kR, kC, 7, 7, 27, 35, TileId::Wall);   // Upper-Right Belfry: Chime III (6, 30), Relic 1 (6, 34)

    // Tier 4 spire: Upper-Right Belfry (row 7) -> Bridge 2 (row 6, cols 18..25) -> Upper-Left Spire (row 5)
    FillRect(cells, kR, kC, 5, 5, 2, 16, TileId::Wall);    // Upper-Left Spire: Chime IV (4, 9), hidden Relic 2 (4, 3)
    SetCell(cells, kR, kC, 2, 5, TileId::Wall);            // Decorative ceiling tooth; rows 3..4 (2 tiles) stay open to (4, 3)

    Tilemap map(kR, kC, std::move(cells), tile_size);

    StructurePuzzleState initial_state{};
    initial_state.ResetForStructure(StructureId::PendulumTower, tile_size);
    initial_state.ApplyGatesToTilemap(map);
    return map;
}

// ============================================================================
// 3. GeothermalChamber (Camera Geotermica) - 20x42 - "Ciclo di Carnot"
// ============================================================================
[[nodiscard]] Tilemap BuildGeothermalChamberMap(float tile_size) {
    const StructureSpawnLayout layout = GetStructureLayout(StructureId::GeothermalChamber);
    const int kR = layout.rows; // 20
    const int kC = layout.cols; // 42
    std::vector<std::uint8_t> cells(
        static_cast<std::size_t>(kR * kC),
        static_cast<std::uint8_t>(TileId::Walkable));

    BuildOuterShell(cells, kR, kC);

    // Exit portal on west ground floor (row 17, cols 2..3)
    for (const TileCoord& pt : layout.exit_portal_tiles) {
        SetCell(cells, kR, kC, pt.row, pt.col, TileId::ExitPortal);
    }

    // West Basalt Gallery climb (V1 at (15, 7), V2 at ground (17, 18))
    FillRect(cells, kR, kC, 16, 16, 5, 9, TileId::Wall);   // climb 18->16 (2 tiles)
    FillRect(cells, kR, kC, 14, 14, 2, 5, TileId::Wall);   // climb 16->14 (2 tiles)
    FillRect(cells, kR, kC, 12, 12, 6, 8, TileId::Wall);   // climb 14->12 (2 tiles)
    FillRect(cells, kR, kC, 10, 10, 9, 11, TileId::Wall);  // Bridge 1 launch (Bridge 1: row 10, cols 13..17)

    // Upper-Center Ridge (V3 at (9, 21), hidden Collectible 1 at (9, 25))
    FillRect(cells, kR, kC, 10, 10, 19, 25, TileId::Wall);
    // Sealing right wall of Upper-Center Ridge so player cannot drop into East Vent bypassing Gate 1
    FillRect(cells, kR, kC, 2, 10, 26, 26, TileId::Wall);

    // Central Conduit Wall (col 22): masonry rows 11..14, Gate 1 at rows 15..17
    FillRect(cells, kR, kC, 11, 14, 22, 22, TileId::Wall);

    // East Geothermal Vent climb (V4 at ground (17, 37)) toward Bridge 2 (row 10, cols 29..33)
    FillRect(cells, kR, kC, 16, 16, 35, 38, TileId::Wall); // Step E1: climb 18->16 (2 tiles)
    FillRect(cells, kR, kC, 14, 14, 30, 34, TileId::Wall); // Step E2: climb 16->14 (2 tiles, 0 gap)
    FillRect(cells, kR, kC, 12, 12, 27, 28, TileId::Wall); // Step E3: climb 14->12 (2 tiles, 1-tile gap)
    // From Step E3 (12, 28) -> Bridge 2 (10, 29..33): 2-tile climb, 0 gap.
    // Without Bridge 2, gap from col 28 to Vault floor col 34 is 5 tiles (impassable).

    // Upper Carnot Vault (floor row 10, cols 34..39; standing row 9)
    // Vault wall col 34 rows 2..6; Gate 2 at col 34 rows 7..9
    // Inside: Collectible 2 at (9, 36), ThermalExchanger at (9, 38)
    FillRect(cells, kR, kC, 10, 10, 34, 39, TileId::Wall);
    FillRect(cells, kR, kC, 2, 6, 34, 34, TileId::Wall);

    Tilemap map(kR, kC, std::move(cells), tile_size);

    StructurePuzzleState initial_state{};
    initial_state.ResetForStructure(StructureId::GeothermalChamber, tile_size);
    initial_state.ApplyGatesToTilemap(map);
    return map;
}

} // namespace

StructureSpawnLayout GetStructureLayout(StructureId id) noexcept {
    switch (id) {
        case StructureId::HypogeanCisterns:
            return StructureSpawnLayout{
                StructureId::HypogeanCisterns,
                20,
                40,
                TileCoord{17, 5},
                {TileCoord{17, 2}, TileCoord{17, 3}},
                TileCoord{16, 36},
                true,
                TileCoord{17, 30},
                {TileCoord{17, 25}, TileCoord{9, 3}}
            };
        case StructureId::PendulumTower:
            return StructureSpawnLayout{
                StructureId::PendulumTower,
                22,
                38,
                TileCoord{19, 5},
                {TileCoord{19, 2}, TileCoord{19, 3}},
                TileCoord{19, 33},
                false,
                TileCoord{0, 0},
                {TileCoord{6, 34}, TileCoord{4, 3}}
            };
        case StructureId::GeothermalChamber:
            return StructureSpawnLayout{
                StructureId::GeothermalChamber,
                20,
                42,
                TileCoord{17, 5},
                {TileCoord{17, 2}, TileCoord{17, 3}},
                TileCoord{9, 38},
                false,
                TileCoord{0, 0},
                {TileCoord{9, 25}, TileCoord{9, 36}}
            };
        case StructureId::Count:
            break;
    }
    return GetStructureLayout(StructureId::HypogeanCisterns);
}

Tilemap BuildStructurePuzzleMap(StructureId id, float tile_size) {
    switch (id) {
        case StructureId::HypogeanCisterns:
            return BuildHypogeanCisternsMap(tile_size);
        case StructureId::PendulumTower:
            return BuildPendulumTowerMap(tile_size);
        case StructureId::GeothermalChamber:
            return BuildGeothermalChamberMap(tile_size);
        case StructureId::Count:
            break;
    }
    return BuildHypogeanCisternsMap(tile_size);
}

void StructurePuzzleState::ResetForStructure(StructureId id, float tile_sz) {
    structure_id = id;
    tile_size = (std::isfinite(tile_sz) && tile_sz > 0.0f) ? tile_sz : kTileSize;
    switches.clear();
    gates.clear();
    expected_sequence.clear();
    current_sequence.clear();
    solved = false;

    switch (id) {
        case StructureId::HypogeanCisterns: {
            title = "Equilibrio delle Paratoie";
            hint_text = "Paratoia I: A o C | Ponte: A e B | Chiusa Turbina: A spento, B e C accesi";

            switches.push_back(MakeSwitch(0, {13, 5}, "A", tile_size));
            switches.push_back(MakeSwitch(1, {15, 22}, "B", tile_size));
            switches.push_back(MakeSwitch(2, {9, 6}, "C", tile_size));

            StructureGate gate1{};
            gate1.id = 0;
            gate1.is_bridge = false;
            gate1.open = false;
            gate1.rule = StructureLogicRule::AnyActive;
            gate1.tiles = MakeVerticalSpan(15, 17, 16);
            gate1.required_switches = {0, 2}; // A || C
            gate1.label = "Paratoia Centrale";
            gates.push_back(std::move(gate1));

            StructureGate bridge1{};
            bridge1.id = 1;
            bridge1.is_bridge = true;
            bridge1.open = false;
            bridge1.rule = StructureLogicRule::AllActive;
            bridge1.tiles = MakeHorizontalSpan(10, 12, 16);
            bridge1.required_switches = {0, 1}; // A && B
            bridge1.label = "Ponte Acquedotto";
            gates.push_back(std::move(bridge1));

            StructureGate gate2{};
            gate2.id = 2;
            gate2.is_bridge = false;
            gate2.open = false;
            gate2.rule = StructureLogicRule::ExactMask;
            gate2.tiles = MakeVerticalSpan(15, 17, 28);
            gate2.required_switches = {0, 1, 2};
            gate2.required_states = {false, true, true}; // !A && B && C
            gate2.label = "Chiusa della Turbina";
            gates.push_back(std::move(gate2));
            break;
        }
        case StructureId::PendulumTower: {
            title = "Sequenza Isocrona";
            hint_text = "Percuoti le campane armoniche in ordine crescente: I -> II -> III -> IV";
            expected_sequence = {0, 1, 2, 3};

            switches.push_back(MakeSwitch(0, {14, 31}, "I", tile_size));
            switches.push_back(MakeSwitch(1, {10, 5}, "II", tile_size));
            switches.push_back(MakeSwitch(2, {6, 30}, "III", tile_size));
            switches.push_back(MakeSwitch(3, {4, 9}, "IV", tile_size));

            StructureGate bridge1{};
            bridge1.id = 0;
            bridge1.is_bridge = true;
            bridge1.open = false;
            bridge1.rule = StructureLogicRule::SequenceOrder;
            bridge1.tiles = MakeHorizontalSpan(9, 15, 21);
            bridge1.sequence_target_len = 2; // I -> II
            bridge1.label = "Ponte Armonico I";
            gates.push_back(std::move(bridge1));

            StructureGate bridge2{};
            bridge2.id = 1;
            bridge2.is_bridge = true;
            bridge2.open = false;
            bridge2.rule = StructureLogicRule::SequenceOrder;
            bridge2.tiles = MakeHorizontalSpan(6, 18, 25);
            bridge2.sequence_target_len = 3; // I -> II -> III
            bridge2.label = "Ponte Armonico II";
            gates.push_back(std::move(bridge2));

            StructureGate vault_gate{};
            vault_gate.id = 2;
            vault_gate.is_bridge = false;
            vault_gate.open = false;
            vault_gate.rule = StructureLogicRule::SequenceOrder;
            vault_gate.tiles = MakeVerticalSpan(17, 19, 28);
            vault_gate.sequence_target_len = 4; // I -> II -> III -> IV
            vault_gate.label = "Portale Isocrono";
            gates.push_back(std::move(vault_gate));
            break;
        }
        case StructureId::GeothermalChamber: {
            title = "Ciclo di Carnot";
            hint_text = "Ponte Ovest: V1 acceso e V2 spento | Condotto: V2 | Camera: tutte le valvole accese";

            switches.push_back(MakeSwitch(0, {15, 7}, "V1", tile_size));
            switches.push_back(MakeSwitch(1, {17, 18}, "V2", tile_size));
            switches.push_back(MakeSwitch(2, {9, 21}, "V3", tile_size));
            switches.push_back(MakeSwitch(3, {17, 37}, "V4", tile_size));

            StructureGate bridge1{};
            bridge1.id = 0;
            bridge1.is_bridge = true;
            bridge1.open = false;
            bridge1.rule = StructureLogicRule::ExactMask;
            bridge1.tiles = MakeHorizontalSpan(10, 13, 17);
            bridge1.required_switches = {0, 1};
            bridge1.required_states = {true, false}; // V1 && !V2
            bridge1.label = "Ponte di Basalto";
            gates.push_back(std::move(bridge1));

            StructureGate gate1{};
            gate1.id = 1;
            gate1.is_bridge = false;
            gate1.open = false;
            gate1.rule = StructureLogicRule::AllActive;
            gate1.tiles = MakeVerticalSpan(15, 17, 22);
            gate1.required_switches = {1}; // V2
            gate1.label = "Valvola del Condotto";
            gates.push_back(std::move(gate1));

            StructureGate bridge2{};
            bridge2.id = 2;
            bridge2.is_bridge = true;
            bridge2.open = false;
            bridge2.rule = StructureLogicRule::AllActive;
            bridge2.tiles = MakeHorizontalSpan(10, 29, 33);
            bridge2.required_switches = {2, 3}; // V3 && V4
            bridge2.label = "Passerella Termica";
            gates.push_back(std::move(bridge2));

            StructureGate gate2{};
            gate2.id = 3;
            gate2.is_bridge = false;
            gate2.open = false;
            gate2.rule = StructureLogicRule::AllActive;
            gate2.tiles = MakeVerticalSpan(7, 9, 34);
            gate2.required_switches = {0, 1, 2, 3}; // V1 && V2 && V3 && V4
            gate2.label = "Camera di Carnot";
            gates.push_back(std::move(gate2));
            break;
        }
        case StructureId::Count:
            ResetForStructure(StructureId::HypogeanCisterns, tile_sz);
            break;
    }
}

void StructurePuzzleState::ApplyGatesToTilemap(Tilemap& tilemap) const noexcept {
    for (const StructureGate& gate : gates) {
        const TileId tile_id = gate.is_bridge
            ? (gate.open ? TileId::RuinsWall : TileId::Walkable)
            : (gate.open ? TileId::Walkable : TileId::SolarGate);
        for (const TileCoord& coord : gate.tiles) {
            (void)tilemap.SetTile(coord.row, coord.col, tile_id);
        }
    }
}

bool StructurePuzzleState::EvaluateGateCondition(const StructureGate& gate) const noexcept {
    switch (gate.rule) {
        case StructureLogicRule::AllActive: {
            if (gate.required_switches.empty()) {
                return false;
            }
            for (const int sw_id : gate.required_switches) {
                const StructureSwitch* sw = GetSwitch(sw_id);
                if (sw == nullptr || !sw->active) {
                    return false;
                }
            }
            return true;
        }
        case StructureLogicRule::AnyActive: {
            for (const int sw_id : gate.required_switches) {
                const StructureSwitch* sw = GetSwitch(sw_id);
                if (sw != nullptr && sw->active) {
                    return true;
                }
            }
            return false;
        }
        case StructureLogicRule::ExactMask: {
            if (gate.required_switches.empty() ||
                gate.required_switches.size() != gate.required_states.size()) {
                return false;
            }
            for (std::size_t i = 0; i < gate.required_switches.size(); ++i) {
                const StructureSwitch* sw = GetSwitch(gate.required_switches[i]);
                if (sw == nullptr || sw->active != gate.required_states[i]) {
                    return false;
                }
            }
            return true;
        }
        case StructureLogicRule::SequenceOrder: {
            if (gate.sequence_target_len <= 0) {
                return false;
            }
            return static_cast<int>(current_sequence.size()) >= gate.sequence_target_len;
        }
    }
    return false;
}

void StructurePuzzleState::EvaluateGates(
    Tilemap& tilemap,
    StructurePuzzleEventFlags* events) noexcept {
    for (StructureGate& gate : gates) {
        const bool should_open = EvaluateGateCondition(gate);
        if (should_open && !gate.open) {
            gate.open = true;
            if (events != nullptr) {
                events->gate_opened = true;
            }
        } else if (!should_open && gate.open) {
            gate.open = false;
            if (events != nullptr) {
                events->gate_closed = true;
            }
        }
    }

    ApplyGatesToTilemap(tilemap);

    const bool was_solved = solved;
    solved = !gates.empty() && gates.back().open;
    if (!was_solved && solved && events != nullptr) {
        events->puzzle_solved = true;
    }
}

StructurePuzzleEventFlags StructurePuzzleState::ToggleSwitchById(
    int switch_id,
    Tilemap& tilemap) noexcept {
    StructurePuzzleEventFlags events{};

    StructureSwitch* target = nullptr;
    for (StructureSwitch& sw : switches) {
        if (sw.id == switch_id) {
            target = &sw;
            break;
        }
    }
    if (target == nullptr) {
        return events;
    }

    if (!expected_sequence.empty()) {
        // Ordered sequence puzzle: striking an already-active chime in a valid prefix is a no-op.
        if (target->active) {
            return events;
        }
        const std::size_t next_idx = current_sequence.size();
        if (next_idx < expected_sequence.size() && expected_sequence[next_idx] == switch_id) {
            target->active = true;
            current_sequence.push_back(switch_id);
            events.switch_toggled = true;
        } else {
            for (StructureSwitch& sw : switches) {
                sw.active = false;
            }
            current_sequence.clear();
            events.sequence_failed = true;
        }
    } else {
        target->active = !target->active;
        events.switch_toggled = true;
    }

    EvaluateGates(tilemap, &events);
    return events;
}

StructurePuzzleEventFlags StructurePuzzleState::TryInteract(
    const PlayerManager& players,
    Tilemap& tilemap) noexcept {
    int best_switch_id = -1;
    float best_dist_sq = std::numeric_limits<float>::max();

    for (const Player& player : players.Slots()) {
        if (!player.active) {
            continue;
        }
        const Vec2 center = player.Center();
        for (const StructureSwitch& sw : switches) {
            const float radius = std::max(0.0f, sw.interact_radius);
            const float dist_sq = (sw.pos - center).LengthSquared();
            if (dist_sq <= radius * radius && dist_sq < best_dist_sq) {
                best_dist_sq = dist_sq;
                best_switch_id = sw.id;
            }
        }
    }

    if (best_switch_id < 0) {
        return {};
    }
    return ToggleSwitchById(best_switch_id, tilemap);
}

const StructureSwitch* StructurePuzzleState::FindNearestSwitch(
    Vec2 world_pos,
    float max_radius) const noexcept {
    const StructureSwitch* best = nullptr;
    float best_dist_sq = std::numeric_limits<float>::max();

    for (const StructureSwitch& sw : switches) {
        const float limit = (max_radius >= 0.0f) ? max_radius : sw.interact_radius;
        const float dist_sq = (sw.pos - world_pos).LengthSquared();
        if (dist_sq <= limit * limit && dist_sq < best_dist_sq) {
            best_dist_sq = dist_sq;
            best = &sw;
        }
    }
    return best;
}

const StructureSwitch* StructurePuzzleState::GetSwitch(int switch_id) const noexcept {
    for (const StructureSwitch& sw : switches) {
        if (sw.id == switch_id) {
            return &sw;
        }
    }
    return nullptr;
}

const StructureGate* StructurePuzzleState::GetGate(int gate_id) const noexcept {
    for (const StructureGate& gate : gates) {
        if (gate.id == gate_id) {
            return &gate;
        }
    }
    return nullptr;
}

} // namespace eco
