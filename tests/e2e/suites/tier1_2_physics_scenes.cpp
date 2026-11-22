#include "e2e_adapter.hpp"
#include "e2e_harness.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace {

using eco::e2e::CameraSnapshot;
using eco::e2e::ColorRgba;
using eco::e2e::ContractConstants;
using eco::e2e::E2ETier;
using eco::e2e::EcoSnapshot;
using eco::e2e::InputFrame;
using eco::e2e::MachineKind;
using eco::e2e::MachineSnapshot;
using eco::e2e::MachineStatus;
using eco::e2e::OverlayPhase;
using eco::e2e::PlayerSnapshot;
using eco::e2e::SceneMode;
using eco::e2e::StepResult;
using eco::e2e::TileKind;
using eco::e2e::Vec2;

std::vector<std::vector<int>> MakeGrid(int rows, int cols, int fill = 0) {
    return std::vector<std::vector<int>>(
        static_cast<std::size_t>(rows),
        std::vector<int>(static_cast<std::size_t>(cols), fill));
}

std::vector<std::vector<int>> MakeBorderedGrid(int rows, int cols) {
    auto grid = MakeGrid(rows, cols, 0);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (r == 0 || r == rows - 1 || c == 0 || c == cols - 1) {
                grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = 1;
            }
        }
    }
    return grid;
}

std::vector<std::vector<int>> MakeChamberGrid(int rows, int cols, int floor_row) {
    auto grid = MakeGrid(rows, cols, 0);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (r == 0 || r >= floor_row || c == 0 || c == cols - 1) {
                grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = 1;
            }
        }
    }
    return grid;
}

std::vector<std::vector<int>> MakeLedgeGrid() {
    auto grid = MakeChamberGrid(10, 8, 9);
    grid[4][1] = 1;
    grid[4][2] = 1;
    return grid;
}

InputFrame MakeDigitalInput(
    bool left = false,
    bool right = false,
    bool up = false,
    bool down = false,
    bool jump_pressed = false,
    bool jump_held = false,
    bool interact_pressed = false) {
    InputFrame in{};
    in.active = true;
    in.use_digital_keys = true;
    in.left = left;
    in.right = right;
    in.up = up;
    in.down = down;
    in.jump_pressed = jump_pressed;
    in.jump_held = jump_held;
    in.interact_pressed = interact_pressed;
    return in;
}

InputFrame MakeAnalogInput(float move_x, float move_y) {
    InputFrame in{};
    in.active = true;
    in.use_digital_keys = false;
    in.move_x = move_x;
    in.move_y = move_y;
    return in;
}

std::array<InputFrame, 4> MakeInputsP0(const InputFrame& p0) {
    std::array<InputFrame, 4> inputs{};
    inputs[0] = p0;
    return inputs;
}

std::array<InputFrame, 4> MakeNeutralInputs(int active_count = 1) {
    std::array<InputFrame, 4> inputs{};
    for (int i = 0; i < active_count && i < 4; ++i) {
        inputs[static_cast<std::size_t>(i)].active = true;
    }
    return inputs;
}

void RecruitArchitectViaSolar(eco::e2e::IE2EAdapter& adapter, Vec2 safe_solar_pos) {
    adapter.SetMachinePosition(MachineKind::SolarArray, safe_solar_pos);
    adapter.SetPlayerPosition(
        0,
        {safe_solar_pos.x - ContractConstants::kPlayerWidth * 0.5f,
         safe_solar_pos.y - ContractConstants::kPlayerHeight * 0.5f});
    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, false, false, true)));
    E2E_ASSERT_TRUE(adapter.GetCalibrationUi().is_open);
    const MachineSnapshot solar = adapter.GetMachine(MachineKind::SolarArray);
    adapter.SetCalibrationParameter(solar.target_param);
    E2E_ASSERT_TRUE(adapter.ConfirmCalibrationClean());
    E2E_ASSERT_TRUE(adapter.GetEcoSystem().clean_energy > 0.0f);

    const auto npc_before = adapter.GetArchitectNpc();
    adapter.SetPlayerPosition(0, npc_before.pos);
    const auto npc_after = adapter.InteractArchitectNpc(0);
    E2E_ASSERT_TRUE(npc_after.recruited);
}

} // namespace

// ============================================================================
// F09: Tilemap 2D Grid (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F09_T1_01_DefaultMapsNonEmptyAndConstantTileSize, E2ETier::Tier1_Feature, "F09") {
    E2E_ASSERT_NEAR(ContractConstants::kTileSize, 48.0f, 1e-6f);
    E2E_ASSERT_EQ(static_cast<int>(TileKind::Walkable), 0);
    E2E_ASSERT_EQ(static_cast<int>(TileKind::Wall), 1);
    E2E_ASSERT_EQ(static_cast<int>(TileKind::StructurePortal), 2);
    E2E_ASSERT_EQ(static_cast<int>(TileKind::ExitPortal), 3);
    E2E_ASSERT_EQ(static_cast<int>(TileKind::HiddenBridge), 4);
    E2E_REQUIRE_LIVE(adapter);

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    for (SceneMode mode : {SceneMode::Overworld, SceneMode::Structure}) {
        const int rows = adapter.TilemapRows(mode);
        const int cols = adapter.TilemapCols(mode);
        E2E_ASSERT_GE(rows, 1);
        E2E_ASSERT_GE(cols, 1);
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                const TileKind kind = adapter.GetTileAt(mode, r, c);
                const int raw = static_cast<int>(kind);
                E2E_ASSERT_GE(raw, 0);
                E2E_ASSERT_LE(raw, 4);
                const bool solid = adapter.IsTileSolid(mode, r, c);
                if (kind == TileKind::Wall) {
                    E2E_ASSERT_TRUE(solid);
                } else if (kind == TileKind::Walkable ||
                           kind == TileKind::StructurePortal ||
                           kind == TileKind::ExitPortal) {
                    E2E_ASSERT_TRUE(!solid);
                }
            }
        }
    }
}

E2E_TEST(F09_T1_02_TileIdCategoriesAndInitialSolidity, E2ETier::Tier1_Feature, "F09") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, {{0, 1, 2, 3, 4}});

    E2E_ASSERT_EQ(adapter.GetTileAt(SceneMode::Overworld, 0, 0), TileKind::Walkable);
    E2E_ASSERT_EQ(adapter.GetTileAt(SceneMode::Overworld, 0, 1), TileKind::Wall);
    E2E_ASSERT_EQ(adapter.GetTileAt(SceneMode::Overworld, 0, 2), TileKind::StructurePortal);
    E2E_ASSERT_EQ(adapter.GetTileAt(SceneMode::Overworld, 0, 3), TileKind::ExitPortal);
    E2E_ASSERT_EQ(adapter.GetTileAt(SceneMode::Overworld, 0, 4), TileKind::HiddenBridge);

    E2E_ASSERT_TRUE(!adapter.IsTileSolid(SceneMode::Overworld, 0, 0));
    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, 0, 1));
    E2E_ASSERT_TRUE(!adapter.IsTileSolid(SceneMode::Overworld, 0, 2));
    E2E_ASSERT_TRUE(!adapter.IsTileSolid(SceneMode::Overworld, 0, 3));
    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, 0, 4));
}

E2E_TEST(F09_T1_03_HiddenBridgeUnlocksAfterArchitectRecruit, E2ETier::Tier1_Feature, "F09") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, {{0, 4, 0}});

    E2E_ASSERT_EQ(adapter.GetTileAt(SceneMode::Overworld, 0, 1), TileKind::HiddenBridge);
    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, 0, 1));

    RecruitArchitectViaSolar(adapter, {24.0f, 24.0f});

    E2E_ASSERT_TRUE(adapter.GetArchitectNpc().recruited);
    E2E_ASSERT_EQ(adapter.GetTileAt(SceneMode::Overworld, 0, 1), TileKind::HiddenBridge);
    E2E_ASSERT_TRUE(!adapter.IsTileSolid(SceneMode::Overworld, 0, 1));
}

E2E_TEST(F09_T1_04_CustomGridDimensionsAndOverwrite, E2ETier::Tier1_Feature, "F09") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);

    adapter.LoadCustomTilemap(SceneMode::Overworld, {
        {1, 1, 1, 1, 1},
        {1, 0, 2, 0, 1},
        {1, 1, 1, 1, 1}
    });
    E2E_ASSERT_EQ(adapter.TilemapRows(SceneMode::Overworld), 3);
    E2E_ASSERT_EQ(adapter.TilemapCols(SceneMode::Overworld), 5);
    E2E_ASSERT_EQ(adapter.GetTileAt(SceneMode::Overworld, 1, 2), TileKind::StructurePortal);
    E2E_ASSERT_TRUE(!adapter.IsTileSolid(SceneMode::Overworld, 1, 2));

    adapter.LoadCustomTilemap(SceneMode::Overworld, {
        {0, 0},
        {0, 1},
        {0, 0},
        {1, 1}
    });
    E2E_ASSERT_EQ(adapter.TilemapRows(SceneMode::Overworld), 4);
    E2E_ASSERT_EQ(adapter.TilemapCols(SceneMode::Overworld), 2);
    E2E_ASSERT_EQ(adapter.GetTileAt(SceneMode::Overworld, 1, 1), TileKind::Wall);
    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, 1, 1));
    E2E_ASSERT_TRUE(!adapter.IsTileSolid(SceneMode::Overworld, 2, 1));
}

E2E_TEST(F09_T1_05_IndependentOverworldAndStructureTilemaps, E2ETier::Tier1_Feature, "F09") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);

    adapter.LoadCustomTilemap(SceneMode::Overworld, {{0, 1}});
    adapter.LoadCustomTilemap(SceneMode::Structure, {{1, 0, 3}});

    E2E_ASSERT_EQ(adapter.TilemapRows(SceneMode::Overworld), 1);
    E2E_ASSERT_EQ(adapter.TilemapCols(SceneMode::Overworld), 2);
    E2E_ASSERT_EQ(adapter.GetTileAt(SceneMode::Overworld, 0, 1), TileKind::Wall);

    E2E_ASSERT_EQ(adapter.TilemapRows(SceneMode::Structure), 1);
    E2E_ASSERT_EQ(adapter.TilemapCols(SceneMode::Structure), 3);
    E2E_ASSERT_EQ(adapter.GetTileAt(SceneMode::Structure, 0, 2), TileKind::ExitPortal);
}

E2E_TEST(F09_T1_06_WorldToTile48PxGeometryMapping, E2ETier::Tier1_Feature, "F09") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, {
        {0, 1, 0},
        {0, 0, 0},
        {0, 0, 0}
    });
    adapter.SetPlayerPosition(0, {10.0f, 9.0f});
    adapter.StepFixed(15, MakeInputsP0(MakeAnalogInput(1.0f, 0.0f)));

    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x, ContractConstants::kTileSize - ContractConstants::kPlayerWidth, 0.1f);
}

E2E_TEST(F09_T2_01_OutOfBoundsQueriesAlwaysSafeAndSolid, E2ETier::Tier2_Boundary, "F09") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, {{0, 0}, {0, 0}});

    const std::array<std::pair<int, int>, 8> oob_coords = {{
        {-1, 0}, {0, -1}, {-1, -1}, {2, 0},
        {0, 2}, {2, 2}, {-100000, 100000},
        {std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}
    }};
    for (const auto& [r, c] : oob_coords) {
        E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, r, c));
        E2E_ASSERT_EQ(adapter.GetTileAt(SceneMode::Overworld, r, c), TileKind::Wall);
    }
}

E2E_TEST(F09_T2_02_EmptyZeroByZeroGridSafety, E2ETier::Tier2_Boundary, "F09") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, {});

    E2E_ASSERT_EQ(adapter.TilemapRows(SceneMode::Overworld), 0);
    E2E_ASSERT_EQ(adapter.TilemapCols(SceneMode::Overworld), 0);
    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, 0, 0));
    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, -1, -1));

    const StepResult res = adapter.StepFixed(5, MakeNeutralInputs(1));
    E2E_ASSERT_EQ(res.fixed_steps_executed, 5);
}

E2E_TEST(F09_T2_03_SingleCell1x1GridBoundaries, E2ETier::Tier2_Boundary, "F09") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, {{0}});

    E2E_ASSERT_EQ(adapter.TilemapRows(SceneMode::Overworld), 1);
    E2E_ASSERT_EQ(adapter.TilemapCols(SceneMode::Overworld), 1);
    E2E_ASSERT_TRUE(!adapter.IsTileSolid(SceneMode::Overworld, 0, 0));
    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, 0, 1));
    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, 1, 0));
    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, -1, 0));
    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, 0, -1));

    adapter.LoadCustomTilemap(SceneMode::Overworld, {{1}});
    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, 0, 0));
}

E2E_TEST(F09_T2_04_RaggedRowGridBoundsSafety, E2ETier::Tier2_Boundary, "F09") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, {
        {0, 0, 0},
        {0},
        {0, 1}
    });

    E2E_ASSERT_TRUE(!adapter.IsTileSolid(SceneMode::Overworld, 0, 2));
    E2E_ASSERT_TRUE(!adapter.IsTileSolid(SceneMode::Overworld, 1, 0));
    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, 2, 1));

    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, 1, 1));
    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, 1, 2));
    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, 2, 2));
}

E2E_TEST(F09_T2_05_UnknownTileIdValuesSafeHandling, E2ETier::Tier2_Boundary, "F09") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, {
        {-1, 5, 99, -999},
        {0, 0, 0, 0}
    });

    for (int c = 0; c < 4; ++c) {
        (void)adapter.GetTileAt(SceneMode::Overworld, 0, c);
        (void)adapter.IsTileSolid(SceneMode::Overworld, 0, c);
    }
    adapter.SetPlayerPosition(0, {10.0f, 57.0f});
    adapter.StepFixed(10, MakeInputsP0(MakeAnalogInput(1.0f, 0.0f)));
    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x, 10.0f + ContractConstants::kTopDownSpeed * (10.0f / 60.0f), 0.5f);
}

E2E_TEST(F09_T2_06_ExactTileEdgeFlushBoundary, E2ETier::Tier2_Boundary, "F09") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, {
        {0, 1, 0},
        {0, 0, 0},
        {0, 0, 0}
    });

    adapter.SetPlayerPosition(0, {18.0f, 5.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).pos.x, 18.0f, 1e-3f);

    adapter.StepFixed(10, MakeInputsP0(MakeAnalogInput(1.0f, 0.0f)));
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).pos.x, 18.0f, 0.05f);
}

// ============================================================================
// F10: Normalized Top-Down Movement (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F10_T1_01_CardinalDirectionsSpeedEquals220PxPerSec, E2ETier::Tier1_Feature, "F10") {
    E2E_ASSERT_NEAR(ContractConstants::kTopDownSpeed, 220.0f, 1e-6f);
    E2E_REQUIRE_LIVE(adapter);

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(12, 12, 0));

    const std::array<Vec2, 4> dirs = {{{1.0f, 0.0f}, {-1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, -1.0f}}};
    for (const Vec2& d : dirs) {
        adapter.SetPlayerPosition(0, {250.0f, 250.0f});
        adapter.StepFixed(60, MakeInputsP0(MakeAnalogInput(d.x, d.y)));
        const PlayerSnapshot p = adapter.GetPlayer(0);
        E2E_ASSERT_NEAR(p.pos.x - 250.0f, d.x * ContractConstants::kTopDownSpeed, 0.5f);
        E2E_ASSERT_NEAR(p.pos.y - 250.0f, d.y * ContractConstants::kTopDownSpeed, 0.5f);
    }
}

E2E_TEST(F10_T1_02_DiagonalSpeedMatchesCardinalNotSqrt2, E2ETier::Tier1_Feature, "F10") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(12, 12, 0));

    adapter.SetPlayerPosition(0, {100.0f, 100.0f});
    adapter.StepFixed(60, MakeInputsP0(MakeDigitalInput(false, true, false, true)));
    const PlayerSnapshot p = adapter.GetPlayer(0);

    const float dx = p.pos.x - 100.0f;
    const float dy = p.pos.y - 100.0f;
    const float expected_comp = ContractConstants::kTopDownSpeed / std::sqrt(2.0f);
    E2E_ASSERT_NEAR(dx, expected_comp, 0.5f);
    E2E_ASSERT_NEAR(dy, expected_comp, 0.5f);
    const float dist = std::hypot(dx, dy);
    E2E_ASSERT_NEAR(dist, ContractConstants::kTopDownSpeed, 0.5f);
    E2E_ASSERT_TRUE(dist < 250.0f);
}

E2E_TEST(F10_T1_03_AllFourDiagonalQuadrantsNormalized, E2ETier::Tier1_Feature, "F10") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(12, 12, 0));

    const float inv_sqrt2 = 1.0f / std::sqrt(2.0f);
    struct QuadCase {
        bool left;
        bool right;
        bool up;
        bool down;
        float sx;
        float sy;
    };
    const std::array<QuadCase, 4> cases = {{
        {false, true, false, true, 1.0f, 1.0f},
        {true, false, false, true, -1.0f, 1.0f},
        {true, false, true, false, -1.0f, -1.0f},
        {false, true, true, false, 1.0f, -1.0f}
    }};
    for (const auto& qc : cases) {
        const Vec2 n = adapter.NormalizeMovement({qc.sx, qc.sy});
        E2E_ASSERT_NEAR(n.x, qc.sx * inv_sqrt2, 1e-5f);
        E2E_ASSERT_NEAR(n.y, qc.sy * inv_sqrt2, 1e-5f);

        adapter.SetPlayerPosition(0, {250.0f, 250.0f});
        adapter.StepFixed(30, MakeInputsP0(MakeDigitalInput(qc.left, qc.right, qc.up, qc.down)));
        const PlayerSnapshot p = adapter.GetPlayer(0);
        const float dx = p.pos.x - 250.0f;
        const float dy = p.pos.y - 250.0f;
        E2E_ASSERT_NEAR(dx, qc.sx * 110.0f * inv_sqrt2, 0.5f);
        E2E_ASSERT_NEAR(dy, qc.sy * 110.0f * inv_sqrt2, 0.5f);
    }
}

E2E_TEST(F10_T1_04_ZeroInputYieldsZeroVelAndZeroMove, E2ETier::Tier1_Feature, "F10") {
    E2E_REQUIRE_LIVE(adapter);
    const Vec2 zero_norm = adapter.NormalizeMovement({0.0f, 0.0f});
    E2E_ASSERT_TRUE(std::isfinite(zero_norm.x));
    E2E_ASSERT_TRUE(std::isfinite(zero_norm.y));
    E2E_ASSERT_NEAR(zero_norm.x, 0.0f, 1e-6f);
    E2E_ASSERT_NEAR(zero_norm.y, 0.0f, 1e-6f);

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(10, 10, 0));
    adapter.SetPlayerPosition(0, {150.0f, 150.0f});
    adapter.StepFixed(30, MakeInputsP0(MakeAnalogInput(0.0f, 0.0f)));
    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x, 150.0f, 1e-4f);
    E2E_ASSERT_NEAR(p.pos.y, 150.0f, 1e-4f);
    E2E_ASSERT_NEAR(p.vel.x, 0.0f, 1e-4f);
    E2E_ASSERT_NEAR(p.vel.y, 0.0f, 1e-4f);
}

E2E_TEST(F10_T1_05_OpposingDigitalKeysCancelOnAxis, E2ETier::Tier1_Feature, "F10") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(12, 12, 0));

    adapter.SetPlayerPosition(0, {250.0f, 250.0f});
    adapter.StepFixed(30, MakeInputsP0(MakeDigitalInput(true, true, false, false)));
    PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x, 250.0f, 1e-4f);
    E2E_ASSERT_NEAR(p.pos.y, 250.0f, 1e-4f);

    adapter.SetPlayerPosition(0, {250.0f, 250.0f});
    adapter.StepFixed(60, MakeInputsP0(MakeDigitalInput(true, true, true, false)));
    p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x, 250.0f, 0.1f);
    E2E_ASSERT_NEAR(p.pos.y, 30.0f, 0.5f);

    adapter.SetPlayerPosition(0, {250.0f, 250.0f});
    adapter.StepFixed(60, MakeInputsP0(MakeDigitalInput(false, true, true, true)));
    p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x, 470.0f, 0.5f);
    E2E_ASSERT_NEAR(p.pos.y, 250.0f, 0.1f);
}

E2E_TEST(F10_T1_06_FourPlayersMoveIndependentlyNormalized, E2ETier::Tier1_Feature, "F10") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 4);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(16, 16, 0));

    adapter.SetPlayerPosition(0, {300.0f, 300.0f});
    adapter.SetPlayerPosition(1, {350.0f, 300.0f});
    adapter.SetPlayerPosition(2, {300.0f, 350.0f});
    adapter.SetPlayerPosition(3, {350.0f, 350.0f});

    std::array<InputFrame, 4> inputs{};
    inputs[0] = MakeAnalogInput(1.0f, 0.0f);
    inputs[1] = MakeAnalogInput(0.0f, 1.0f);
    inputs[2] = MakeAnalogInput(1.0f, 1.0f);
    inputs[3] = MakeAnalogInput(-1.0f, -1.0f);
    adapter.StepFixed(30, inputs);

    const float diag = 110.0f / std::sqrt(2.0f);
    const PlayerSnapshot p0 = adapter.GetPlayer(0);
    const PlayerSnapshot p1 = adapter.GetPlayer(1);
    const PlayerSnapshot p2 = adapter.GetPlayer(2);
    const PlayerSnapshot p3 = adapter.GetPlayer(3);

    E2E_ASSERT_NEAR(p0.pos.x - 300.0f, 110.0f, 0.5f);
    E2E_ASSERT_NEAR(p0.pos.y - 300.0f, 0.0f, 0.1f);
    E2E_ASSERT_NEAR(p1.pos.x - 350.0f, 0.0f, 0.1f);
    E2E_ASSERT_NEAR(p1.pos.y - 300.0f, 110.0f, 0.5f);
    E2E_ASSERT_NEAR(p2.pos.x - 300.0f, diag, 0.5f);
    E2E_ASSERT_NEAR(p2.pos.y - 350.0f, diag, 0.5f);
    E2E_ASSERT_NEAR(p3.pos.x - 350.0f, -diag, 0.5f);
    E2E_ASSERT_NEAR(p3.pos.y - 350.0f, -diag, 0.5f);
}

E2E_TEST(F10_T2_01_NearZeroAndNegativeZeroSafeNormalization, E2ETier::Tier2_Boundary, "F10") {
    E2E_REQUIRE_LIVE(adapter);
    const std::array<Vec2, 3> tiny = {{{1e-9f, 0.0f}, {-1e-12f, 1e-12f}, {-0.0f, -0.0f}}};
    for (const Vec2& v : tiny) {
        const Vec2 n = adapter.NormalizeMovement(v);
        E2E_ASSERT_TRUE(std::isfinite(n.x));
        E2E_ASSERT_TRUE(std::isfinite(n.y));
        E2E_ASSERT_NEAR(n.x, 0.0f, 1e-5f);
        E2E_ASSERT_NEAR(n.y, 0.0f, 1e-5f);
    }
}

E2E_TEST(F10_T2_02_LargeInputVectorNormalizationStability, E2ETier::Tier2_Boundary, "F10") {
    E2E_REQUIRE_LIVE(adapter);
    const Vec2 a = adapter.NormalizeMovement({300.0f, 400.0f});
    E2E_ASSERT_NEAR(a.x, 0.6f, 1e-5f);
    E2E_ASSERT_NEAR(a.y, 0.8f, 1e-5f);

    const Vec2 b = adapter.NormalizeMovement({1e6f, -1e6f});
    const float inv_sqrt2 = 1.0f / std::sqrt(2.0f);
    E2E_ASSERT_TRUE(std::isfinite(b.x));
    E2E_ASSERT_TRUE(std::isfinite(b.y));
    E2E_ASSERT_NEAR(b.x, inv_sqrt2, 1e-4f);
    E2E_ASSERT_NEAR(b.y, -inv_sqrt2, 1e-4f);
    E2E_ASSERT_NEAR(std::hypot(b.x, b.y), 1.0f, 1e-4f);
}

E2E_TEST(F10_T2_03_PythagoreanTriplesExactNormalization, E2ETier::Tier2_Boundary, "F10") {
    E2E_REQUIRE_LIVE(adapter);
    const Vec2 n1 = adapter.NormalizeMovement({3.0f, 4.0f});
    E2E_ASSERT_NEAR(n1.x, 0.6f, 1e-5f);
    E2E_ASSERT_NEAR(n1.y, 0.8f, 1e-5f);

    const Vec2 n2 = adapter.NormalizeMovement({-5.0f, 12.0f});
    E2E_ASSERT_NEAR(n2.x, -5.0f / 13.0f, 1e-5f);
    E2E_ASSERT_NEAR(n2.y, 12.0f / 13.0f, 1e-5f);

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(12, 12, 0));
    adapter.SetPlayerPosition(0, {100.0f, 100.0f});
    adapter.StepFixed(60, MakeInputsP0(MakeAnalogInput(3.0f, 4.0f)));
    const PlayerSnapshot p = adapter.GetPlayer(0);
    const float dx = p.pos.x - 100.0f;
    const float dy = p.pos.y - 100.0f;
    E2E_ASSERT_NEAR(dx, 132.0f, 0.5f);
    E2E_ASSERT_NEAR(dy, 176.0f, 0.5f);
    E2E_ASSERT_NEAR(std::hypot(dx, dy), ContractConstants::kTopDownSpeed, 0.5f);
}

E2E_TEST(F10_T2_04_GamepadDeadzoneBoundaryAndClampedDiagonalSpeed, E2ETier::Tier2_Boundary, "F10") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(12, 12, 0));
    adapter.SetPlayerPosition(0, {200.0f, 200.0f});

    InputFrame drift{};
    drift.active = true;
    drift.gamepad_connected = true;
    drift.gamepad_axis_x = 0.19f;
    drift.gamepad_axis_y = -0.19f;
    adapter.StepFixed(30, MakeInputsP0(drift));
    PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x, 200.0f, 1e-4f);
    E2E_ASSERT_NEAR(p.pos.y, 200.0f, 1e-4f);

    InputFrame full_diag{};
    full_diag.active = true;
    full_diag.gamepad_connected = true;
    full_diag.gamepad_axis_x = 0.95f;
    full_diag.gamepad_axis_y = 0.95f;
    adapter.StepFixed(60, MakeInputsP0(full_diag));
    p = adapter.GetPlayer(0);
    const float dist = std::hypot(p.pos.x - 200.0f, p.pos.y - 200.0f);
    E2E_ASSERT_TRUE(dist > 100.0f);
    E2E_ASSERT_LE(dist, ContractConstants::kTopDownSpeed + 0.5f);
}

E2E_TEST(F10_T2_05_SingleTickAndLongRunAccumulationPrecision, E2ETier::Tier2_Boundary, "F10") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(60, 60, 0));
    adapter.SetPlayerPosition(0, {100.0f, 100.0f});

    const auto inputs = MakeInputsP0(MakeAnalogInput(1.0f, 0.0f));
    adapter.StepFixed(1, inputs);
    PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x - 100.0f, ContractConstants::kTopDownSpeed / 60.0f, 0.05f);

    adapter.StepFixed(599, inputs);
    p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x - 100.0f, ContractConstants::kTopDownSpeed * 10.0f, 1.0f);
    E2E_ASSERT_NEAR(p.pos.y, 100.0f, 1e-3f);
}

E2E_TEST(F10_T2_06_InactivePlayerIgnoresMovementInput, E2ETier::Tier2_Boundary, "F10") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 2);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(10, 10, 0));
    adapter.SetPlayerPosition(0, {100.0f, 100.0f});
    adapter.SetPlayerPosition(1, {200.0f, 200.0f});
    adapter.SetPlayerActive(1, false);

    std::array<InputFrame, 4> inputs{};
    inputs[0] = MakeAnalogInput(1.0f, 1.0f);
    inputs[1] = MakeAnalogInput(1.0f, 1.0f);
    adapter.StepFixed(30, inputs);

    const PlayerSnapshot p0 = adapter.GetPlayer(0);
    const PlayerSnapshot p1 = adapter.GetPlayer(1);
    E2E_ASSERT_NEAR(std::hypot(p0.pos.x - 100.0f, p0.pos.y - 100.0f), 110.0f, 0.5f);
    E2E_ASSERT_NEAR(p1.pos.x, 200.0f, 1e-4f);
    E2E_ASSERT_NEAR(p1.pos.y, 200.0f, 1e-4f);
    E2E_ASSERT_NEAR(p1.vel.x, 0.0f, 1e-4f);
    E2E_ASSERT_NEAR(p1.vel.y, 0.0f, 1e-4f);
}

// ============================================================================
// F11: Custom AABB Sliding Collision (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F11_T1_01_CardinalWallStopAllFourSides, E2ETier::Tier1_Feature, "F11") {
    E2E_ASSERT_NEAR(ContractConstants::kPlayerWidth, 30.0f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kPlayerHeight, 30.0f, 1e-6f);
    E2E_REQUIRE_LIVE(adapter);

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeBorderedGrid(5, 5));

    adapter.SetPlayerPosition(0, {100.0f, 100.0f});
    adapter.StepFixed(60, MakeInputsP0(MakeAnalogInput(1.0f, 0.0f)));
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).pos.x, 162.0f, 0.1f);
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).pos.y, 100.0f, 0.1f);

    adapter.SetPlayerPosition(0, {100.0f, 100.0f});
    adapter.StepFixed(60, MakeInputsP0(MakeAnalogInput(-1.0f, 0.0f)));
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).pos.x, 48.0f, 0.1f);
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).pos.y, 100.0f, 0.1f);

    adapter.SetPlayerPosition(0, {100.0f, 100.0f});
    adapter.StepFixed(60, MakeInputsP0(MakeAnalogInput(0.0f, 1.0f)));
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).pos.x, 100.0f, 0.1f);
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).pos.y, 162.0f, 0.1f);

    adapter.SetPlayerPosition(0, {100.0f, 100.0f});
    adapter.StepFixed(60, MakeInputsP0(MakeAnalogInput(0.0f, -1.0f)));
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).pos.x, 100.0f, 0.1f);
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).pos.y, 48.0f, 0.1f);
}

E2E_TEST(F11_T1_02_DiagonalSlideAlongVerticalWall, E2ETier::Tier1_Feature, "F11") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    auto grid = MakeGrid(8, 8, 0);
    for (int r = 0; r < 8; ++r) {
        grid[static_cast<std::size_t>(r)][3] = 1;
    }
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetPlayerPosition(0, {100.0f, 60.0f});
    adapter.StepFixed(60, MakeInputsP0(MakeDigitalInput(false, true, false, true)));

    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x, 114.0f, 0.1f);
    E2E_ASSERT_NEAR(p.pos.y, 60.0f + ContractConstants::kTopDownSpeed / std::sqrt(2.0f), 1.0f);
}

E2E_TEST(F11_T1_03_DiagonalSlideAlongHorizontalWall, E2ETier::Tier1_Feature, "F11") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    auto grid = MakeGrid(8, 8, 0);
    for (int c = 0; c < 8; ++c) {
        grid[3][static_cast<std::size_t>(c)] = 1;
    }
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetPlayerPosition(0, {60.0f, 100.0f});
    adapter.StepFixed(60, MakeInputsP0(MakeDigitalInput(false, true, false, true)));

    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.y, 114.0f, 0.1f);
    E2E_ASSERT_NEAR(p.pos.x, 60.0f + ContractConstants::kTopDownSpeed / std::sqrt(2.0f), 1.0f);
}

E2E_TEST(F11_T1_04_NoSnaggingAcrossContiguousTileSeams, E2ETier::Tier1_Feature, "F11") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    auto grid = MakeGrid(8, 8, 0);
    for (int r = 0; r < 8; ++r) {
        grid[static_cast<std::size_t>(r)][3] = 1;
    }
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetPlayerPosition(0, {114.0f, 50.0f});
    adapter.StepFixed(60, MakeInputsP0(MakeDigitalInput(false, true, false, true)));

    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x, 114.0f, 0.1f);
    E2E_ASSERT_NEAR(p.pos.y, 50.0f + ContractConstants::kTopDownSpeed / std::sqrt(2.0f), 1.0f);
}

E2E_TEST(F11_T1_05_InnerLCornerSlideAndCleanStop, E2ETier::Tier1_Feature, "F11") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    auto grid = MakeGrid(6, 6, 0);
    for (int i = 0; i <= 3; ++i) {
        grid[static_cast<std::size_t>(i)][3] = 1;
        grid[3][static_cast<std::size_t>(i)] = 1;
    }
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetPlayerPosition(0, {80.0f, 80.0f});
    adapter.StepFixed(90, MakeInputsP0(MakeDigitalInput(false, true, false, true)));

    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x, 114.0f, 0.1f);
    E2E_ASSERT_NEAR(p.pos.y, 114.0f, 0.1f);
}

E2E_TEST(F11_T1_06_HiddenBridgeCollisionTogglesOnRecruit, E2ETier::Tier1_Feature, "F11") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    auto grid = MakeGrid(5, 5, 0);
    for (int r = 0; r < 5; ++r) {
        grid[static_cast<std::size_t>(r)][2] = 4;
    }
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetPlayerPosition(0, {50.0f, 100.0f});
    adapter.StepFixed(40, MakeInputsP0(MakeAnalogInput(1.0f, 0.0f)));
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).pos.x, 66.0f, 0.1f);

    RecruitArchitectViaSolar(adapter, {65.0f, 65.0f});

    adapter.SetPlayerPosition(0, {50.0f, 100.0f});
    adapter.StepFixed(40, MakeInputsP0(MakeAnalogInput(1.0f, 0.0f)));
    const PlayerSnapshot p_after = adapter.GetPlayer(0);
    E2E_ASSERT_TRUE(p_after.pos.x > 144.0f);
    E2E_ASSERT_NEAR(
        p_after.pos.x, 50.0f + ContractConstants::kTopDownSpeed * (40.0f / 60.0f), 1.0f);
}

E2E_TEST(F11_T2_01_FlushTangentCornerZeroOverlapSlidesPast, E2ETier::Tier2_Boundary, "F11") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    auto grid = MakeGrid(5, 5, 0);
    grid[1][2] = 1;
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetPlayerPosition(0, {66.0f, 96.0f});
    adapter.StepFixed(30, MakeInputsP0(MakeAnalogInput(1.0f, 0.0f)));

    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x, 176.0f, 0.5f);
    E2E_ASSERT_NEAR(p.pos.y, 96.0f, 1e-3f);
}

E2E_TEST(F11_T2_02_NarrowOneTile48PxCorridorTraversal, E2ETier::Tier2_Boundary, "F11") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    auto grid = MakeGrid(3, 8, 0);
    for (int c = 0; c < 8; ++c) {
        grid[0][static_cast<std::size_t>(c)] = 1;
        grid[2][static_cast<std::size_t>(c)] = 1;
    }
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetPlayerPosition(0, {10.0f, 57.0f});
    adapter.StepFixed(60, MakeInputsP0(MakeAnalogInput(1.0f, 0.0f)));

    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x, 230.0f, 0.5f);
    E2E_ASSERT_NEAR(p.pos.y, 57.0f, 1e-3f);
}

E2E_TEST(F11_T2_03_OuterMapEdgesClampPlayerInsideBounds, E2ETier::Tier2_Boundary, "F11") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(4, 4, 0));

    adapter.SetPlayerPosition(0, {20.0f, 20.0f});
    adapter.StepFixed(30, MakeInputsP0(MakeAnalogInput(-1.0f, -1.0f)));
    PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x, 0.0f, 0.1f);
    E2E_ASSERT_NEAR(p.pos.y, 0.0f, 0.1f);

    adapter.SetPlayerPosition(0, {150.0f, 150.0f});
    adapter.StepFixed(30, MakeInputsP0(MakeAnalogInput(1.0f, 1.0f)));
    p = adapter.GetPlayer(0);
    const float max_pos = 4.0f * ContractConstants::kTileSize - ContractConstants::kPlayerWidth;
    E2E_ASSERT_NEAR(p.pos.x, max_pos, 0.1f);
    E2E_ASSERT_NEAR(p.pos.y, max_pos, 0.1f);
}

E2E_TEST(F11_T2_04_DiagonalPinchedCornerBlocksTunneling, E2ETier::Tier2_Boundary, "F11") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    auto grid = MakeGrid(4, 4, 0);
    grid[1][2] = 1;
    grid[2][1] = 1;
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetPlayerPosition(0, {66.0f, 66.0f});
    adapter.StepFixed(30, MakeInputsP0(MakeDigitalInput(false, true, false, true)));

    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_LE(p.pos.x, 66.1f);
    E2E_ASSERT_LE(p.pos.y, 66.1f);
}

E2E_TEST(F11_T2_05_MaxFrameDeltaNoWallTunneling, E2ETier::Tier2_Boundary, "F11") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    auto grid = MakeGrid(5, 5, 0);
    for (int r = 0; r < 5; ++r) {
        grid[static_cast<std::size_t>(r)][2] = 1;
    }
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetPlayerPosition(0, {55.0f, 100.0f});
    const StepResult res = adapter.AdvanceFrame(
        ContractConstants::kMaxFrameDelta, MakeInputsP0(MakeAnalogInput(1.0f, 0.0f)));
    E2E_ASSERT_GE(res.fixed_steps_executed, 14);
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).pos.x, 66.0f, 0.1f);
}

E2E_TEST(F11_T2_06_MultiPlayerIndependentWallCollision, E2ETier::Tier2_Boundary, "F11") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 2);
    auto grid = MakeGrid(6, 6, 0);
    grid[2][2] = 1;
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);

    adapter.SetPlayerPosition(0, {50.0f, 105.0f});
    adapter.SetPlayerPosition(1, {50.0f, 200.0f});
    std::array<InputFrame, 4> inputs{};
    inputs[0] = MakeAnalogInput(1.0f, 0.0f);
    inputs[1] = MakeAnalogInput(1.0f, 0.0f);
    adapter.StepFixed(30, inputs);

    E2E_ASSERT_NEAR(adapter.GetPlayer(0).pos.x, 66.0f, 0.1f);
    E2E_ASSERT_NEAR(adapter.GetPlayer(1).pos.x, 160.0f, 0.5f);
}

// ============================================================================
// F12: Smoothed Average-Position Camera2D (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F12_T1_01_SinglePlayerCameraFollowsShift, E2ETier::Tier1_Feature, "F12") {
    E2E_ASSERT_NEAR(ContractConstants::kCameraLerpSpeed, 6.0f, 1e-6f);
    E2E_REQUIRE_LIVE(adapter);

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(20, 20, 0));
    adapter.SetPlayerPosition(0, {100.0f, 100.0f});
    adapter.StepFixed(180, MakeNeutralInputs(1));
    const CameraSnapshot cam0 = adapter.GetCamera();

    adapter.StepFixed(60, MakeInputsP0(MakeAnalogInput(1.0f, 0.0f)));
    adapter.StepFixed(180, MakeNeutralInputs(1));
    const CameraSnapshot cam1 = adapter.GetCamera();
    const PlayerSnapshot p = adapter.GetPlayer(0);

    E2E_ASSERT_NEAR(cam1.target.x - cam0.target.x, 220.0f, 1.0f);
    E2E_ASSERT_NEAR(cam1.target.y - cam0.target.y, 0.0f, 0.5f);
    E2E_ASSERT_LE(std::hypot(cam1.target.x - (p.pos.x + 15.0f),
                             cam1.target.y - (p.pos.y + 15.0f)), 16.0f);
}

E2E_TEST(F12_T1_02_TwoPlayerMidpointAveraging, E2ETier::Tier1_Feature, "F12") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 2);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(20, 20, 0));

    adapter.SetPlayerPosition(0, {100.0f, 100.0f});
    adapter.SetPlayerPosition(1, {300.0f, 300.0f});
    adapter.StepFixed(180, MakeNeutralInputs(2));
    const CameraSnapshot cam0 = adapter.GetCamera();

    adapter.SetPlayerPosition(1, {500.0f, 300.0f});
    adapter.StepFixed(180, MakeNeutralInputs(2));
    const CameraSnapshot cam1 = adapter.GetCamera();

    E2E_ASSERT_NEAR(cam1.target.x - cam0.target.x, 100.0f, 1.0f);
    E2E_ASSERT_NEAR(cam1.target.y - cam0.target.y, 0.0f, 0.5f);
}

E2E_TEST(F12_T1_03_FourPlayerSymmetricCentroidInvariant, E2ETier::Tier1_Feature, "F12") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 4);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(20, 20, 0));

    adapter.SetPlayerPosition(0, {100.0f, 100.0f});
    adapter.SetPlayerPosition(1, {500.0f, 100.0f});
    adapter.SetPlayerPosition(2, {100.0f, 500.0f});
    adapter.SetPlayerPosition(3, {500.0f, 500.0f});
    adapter.StepFixed(180, MakeNeutralInputs(4));
    const CameraSnapshot cam0 = adapter.GetCamera();

    adapter.SetPlayerPosition(0, {50.0f, 50.0f});
    adapter.SetPlayerPosition(1, {550.0f, 50.0f});
    adapter.SetPlayerPosition(2, {50.0f, 550.0f});
    adapter.SetPlayerPosition(3, {550.0f, 550.0f});
    adapter.StepFixed(180, MakeNeutralInputs(4));
    const CameraSnapshot cam1 = adapter.GetCamera();
    E2E_ASSERT_NEAR(cam1.target.x, cam0.target.x, 0.5f);
    E2E_ASSERT_NEAR(cam1.target.y, cam0.target.y, 0.5f);

    adapter.SetPlayerPosition(0, {130.0f, 10.0f});
    adapter.SetPlayerPosition(1, {630.0f, 10.0f});
    adapter.SetPlayerPosition(2, {130.0f, 510.0f});
    adapter.SetPlayerPosition(3, {630.0f, 510.0f});
    adapter.StepFixed(180, MakeNeutralInputs(4));
    const CameraSnapshot cam2 = adapter.GetCamera();
    E2E_ASSERT_NEAR(cam2.target.x - cam1.target.x, 80.0f, 1.0f);
    E2E_ASSERT_NEAR(cam2.target.y - cam1.target.y, -40.0f, 1.0f);
}

E2E_TEST(F12_T1_04_SmoothLerpPartialStepAndMonotonicApproach, E2ETier::Tier1_Feature, "F12") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(20, 20, 0));

    adapter.SetPlayerPosition(0, {100.0f, 100.0f});
    adapter.StepFixed(180, MakeNeutralInputs(1));
    const CameraSnapshot cam_start = adapter.GetCamera();

    adapter.SetPlayerPosition(0, {500.0f, 100.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));
    const CameraSnapshot cam1 = adapter.GetCamera();
    adapter.StepFixed(1, MakeNeutralInputs(1));
    const CameraSnapshot cam2 = adapter.GetCamera();
    adapter.StepFixed(1, MakeNeutralInputs(1));
    const CameraSnapshot cam3 = adapter.GetCamera();

    const float d1 = cam1.target.x - cam_start.target.x;
    E2E_ASSERT_TRUE(d1 > 20.0f && d1 < 80.0f);
    E2E_ASSERT_TRUE(cam_start.target.x < cam1.target.x);
    E2E_ASSERT_TRUE(cam1.target.x < cam2.target.x);
    E2E_ASSERT_TRUE(cam2.target.x < cam3.target.x);
    E2E_ASSERT_TRUE(cam3.target.x < cam_start.target.x + 400.0f);
}

E2E_TEST(F12_T1_05_InactivePlayersExcludedFromCameraAverage, E2ETier::Tier1_Feature, "F12") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 4);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(20, 20, 0));

    adapter.SetPlayerPosition(0, {100.0f, 100.0f});
    adapter.SetPlayerPosition(1, {200.0f, 100.0f});
    adapter.SetPlayerPosition(2, {9000.0f, 9000.0f});
    adapter.SetPlayerPosition(3, {9000.0f, 9000.0f});
    adapter.SetPlayerActive(2, false);
    adapter.SetPlayerActive(3, false);
    adapter.StepFixed(180, MakeNeutralInputs(2));

    const CameraSnapshot cam = adapter.GetCamera();
    E2E_ASSERT_GE(cam.target.x, 149.0f);
    E2E_ASSERT_LE(cam.target.x, 166.0f);
    E2E_ASSERT_GE(cam.target.y, 99.0f);
    E2E_ASSERT_LE(cam.target.y, 116.0f);
}

E2E_TEST(F12_T1_06_DynamicDropInAndDropOutRecalibratesTarget, E2ETier::Tier1_Feature, "F12") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(20, 20, 0));

    adapter.SetPlayerPosition(0, {100.0f, 100.0f});
    adapter.StepFixed(180, MakeNeutralInputs(1));
    const CameraSnapshot cam0 = adapter.GetCamera();

    adapter.SetPlayerActive(1, true);
    adapter.SetPlayerPosition(1, {500.0f, 100.0f});
    adapter.StepFixed(180, MakeNeutralInputs(2));
    const CameraSnapshot cam1 = adapter.GetCamera();
    E2E_ASSERT_NEAR(cam1.target.x - cam0.target.x, 200.0f, 1.0f);

    adapter.SetPlayerActive(1, false);
    adapter.StepFixed(180, MakeNeutralInputs(1));
    const CameraSnapshot cam2 = adapter.GetCamera();
    E2E_ASSERT_NEAR(cam2.target.x, cam0.target.x, 1.0f);
}

E2E_TEST(F12_T2_01_ZeroActivePlayersSafeNoDivisionByZero, E2ETier::Tier2_Boundary, "F12") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 0);
    adapter.StepFixed(60, MakeNeutralInputs(0));
    CameraSnapshot cam = adapter.GetCamera();
    E2E_ASSERT_TRUE(std::isfinite(cam.target.x));
    E2E_ASSERT_TRUE(std::isfinite(cam.target.y));
    E2E_ASSERT_TRUE(std::isfinite(cam.offset.x));
    E2E_ASSERT_TRUE(std::isfinite(cam.offset.y));
    E2E_ASSERT_TRUE(std::isfinite(cam.zoom));
    E2E_ASSERT_TRUE(cam.zoom > 0.0f);

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.SetPlayerPosition(0, {200.0f, 200.0f});
    adapter.StepFixed(30, MakeNeutralInputs(1));
    adapter.SetPlayerActive(0, false);
    adapter.StepFixed(60, MakeNeutralInputs(0));
    cam = adapter.GetCamera();
    E2E_ASSERT_TRUE(std::isfinite(cam.target.x));
    E2E_ASSERT_TRUE(std::isfinite(cam.target.y));
    E2E_ASSERT_TRUE(std::isfinite(cam.zoom));
    E2E_ASSERT_TRUE(cam.zoom > 0.0f);
}

E2E_TEST(F12_T2_02_ZeroFrameDtProducesZeroCameraChange, E2ETier::Tier2_Boundary, "F12") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(20, 20, 0));
    adapter.SetPlayerPosition(0, {100.0f, 100.0f});
    adapter.StepFixed(180, MakeNeutralInputs(1));

    adapter.SetPlayerPosition(0, {600.0f, 600.0f});
    const CameraSnapshot cam_before = adapter.GetCamera();
    const StepResult res = adapter.AdvanceFrame(0.0f, MakeNeutralInputs(1));
    const CameraSnapshot cam_after = adapter.GetCamera();

    E2E_ASSERT_EQ(res.fixed_steps_executed, 0);
    E2E_ASSERT_NEAR(cam_after.target.x, cam_before.target.x, 1e-5f);
    E2E_ASSERT_NEAR(cam_after.target.y, cam_before.target.y, 1e-5f);
    E2E_ASSERT_TRUE(std::isfinite(cam_after.target.x));
    E2E_ASSERT_TRUE(std::isfinite(cam_after.target.y));
}

E2E_TEST(F12_T2_03_SubFixedDtAccumulatorBoundaryCameraStep, E2ETier::Tier2_Boundary, "F12") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(20, 20, 0));
    adapter.SetPlayerPosition(0, {100.0f, 100.0f});
    adapter.StepFixed(180, MakeNeutralInputs(1));
    const CameraSnapshot cam0 = adapter.GetCamera();

    adapter.SetPlayerPosition(0, {500.0f, 100.0f});
    const StepResult res1 = adapter.AdvanceFrame(0.010f, MakeNeutralInputs(1));
    E2E_ASSERT_EQ(res1.fixed_steps_executed, 0);
    E2E_ASSERT_NEAR(adapter.GetCamera().target.x, cam0.target.x, 1e-5f);

    const StepResult res2 = adapter.AdvanceFrame(0.010f, MakeNeutralInputs(1));
    E2E_ASSERT_EQ(res2.fixed_steps_executed, 1);
    E2E_ASSERT_TRUE(adapter.GetCamera().target.x > cam0.target.x + 20.0f);
}

E2E_TEST(F12_T2_04_CoincidentOverlappingPlayersAtSameCoords, E2ETier::Tier2_Boundary, "F12") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 4);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(20, 20, 0));
    for (int i = 0; i < 4; ++i) {
        adapter.SetPlayerPosition(i, {250.0f, 250.0f});
    }
    adapter.StepFixed(180, MakeNeutralInputs(4));
    const CameraSnapshot cam4 = adapter.GetCamera();

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(20, 20, 0));
    adapter.SetPlayerPosition(0, {250.0f, 250.0f});
    adapter.StepFixed(180, MakeNeutralInputs(1));
    const CameraSnapshot cam1 = adapter.GetCamera();

    E2E_ASSERT_NEAR(cam4.target.x, cam1.target.x, 0.5f);
    E2E_ASSERT_NEAR(cam4.target.y, cam1.target.y, 0.5f);
}

E2E_TEST(F12_T2_05_ExtremeCoordinatesNoOverflowOrNaN, E2ETier::Tier2_Boundary, "F12") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 2);
    adapter.SetPlayerPosition(0, {50000.0f, -50000.0f});
    adapter.SetPlayerPosition(1, {50200.0f, -50000.0f});
    adapter.StepFixed(300, MakeNeutralInputs(2));

    const CameraSnapshot cam = adapter.GetCamera();
    E2E_ASSERT_TRUE(std::isfinite(cam.target.x));
    E2E_ASSERT_TRUE(std::isfinite(cam.target.y));
    const Vec2 mid{
        (adapter.GetPlayer(0).pos.x + adapter.GetPlayer(1).pos.x) * 0.5f,
        (adapter.GetPlayer(0).pos.y + adapter.GetPlayer(1).pos.y) * 0.5f};
    E2E_ASSERT_LE(std::hypot(cam.target.x - mid.x, cam.target.y - mid.y), 25.0f);
}

E2E_TEST(F12_T2_06_OscillatingTargetDoesNotOvershoot, E2ETier::Tier2_Boundary, "F12") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(20, 20, 0));
    adapter.SetPlayerPosition(0, {200.0f, 200.0f});
    adapter.StepFixed(180, MakeNeutralInputs(1));

    for (int cycle = 0; cycle < 10; ++cycle) {
        const float px = (cycle % 2 == 0) ? 400.0f : 200.0f;
        adapter.SetPlayerPosition(0, {px, 200.0f});
        for (int t = 0; t < 5; ++t) {
            adapter.StepFixed(1, MakeNeutralInputs(1));
            const CameraSnapshot cam = adapter.GetCamera();
            E2E_ASSERT_GE(cam.target.x, 199.0f);
            E2E_ASSERT_LE(cam.target.x, 416.0f);
            E2E_ASSERT_GE(cam.target.y, 199.0f);
            E2E_ASSERT_LE(cam.target.y, 216.0f);
        }
    }
}

// ============================================================================
// F13: OverworldState Scene (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F13_T1_01_InitialOverworldSceneAndEcoDefaults, E2ETier::Tier1_Feature, "F13") {
    E2E_ASSERT_NEAR(ContractConstants::kInitialPollution, 35.0f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kInitialCleanEnergy, 0.0f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kInitialDirtyEnergy, 0.0f, 1e-6f);
    E2E_REQUIRE_LIVE(adapter);

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Overworld);
    E2E_ASSERT_EQ(adapter.ActivePlayerCount(), 1);
    E2E_ASSERT_GE(adapter.TilemapRows(SceneMode::Overworld), 1);
    E2E_ASSERT_GE(adapter.TilemapCols(SceneMode::Overworld), 1);

    const EcoSnapshot eco = adapter.GetEcoSystem();
    E2E_ASSERT_NEAR(eco.pollution_level, 35.0f, 1e-4f);
    E2E_ASSERT_NEAR(eco.clean_energy, 0.0f, 1e-4f);
    E2E_ASSERT_NEAR(eco.dirty_energy, 0.0f, 1e-4f);
    E2E_ASSERT_EQ(eco.ui_phase, OverlayPhase::Phase1_Minimal);
    E2E_ASSERT_TRUE(!eco.architect_recruited);

    const MachineSnapshot solar = adapter.GetMachine(MachineKind::SolarArray);
    E2E_ASSERT_EQ(solar.kind, MachineKind::SolarArray);
    E2E_ASSERT_EQ(solar.status, MachineStatus::Broken);
    E2E_ASSERT_TRUE(!adapter.GetArchitectNpc().recruited);
}

E2E_TEST(F13_T1_02_FourColoredPlayersDistinctAndNonSolidSpawn, E2ETier::Tier1_Feature, "F13") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 4);
    E2E_ASSERT_EQ(adapter.ActivePlayerCount(), 4);

    std::array<PlayerSnapshot, 4> players{};
    for (int i = 0; i < 4; ++i) {
        players[static_cast<std::size_t>(i)] = adapter.GetPlayer(i);
        const auto& p = players[static_cast<std::size_t>(i)];
        E2E_ASSERT_TRUE(p.active);
        E2E_ASSERT_NEAR(p.size.x, ContractConstants::kPlayerWidth, 1e-4f);
        E2E_ASSERT_NEAR(p.size.y, ContractConstants::kPlayerHeight, 1e-4f);
        const int col = static_cast<int>(std::floor((p.pos.x + 15.0f) / ContractConstants::kTileSize));
        const int row = static_cast<int>(std::floor((p.pos.y + 15.0f) / ContractConstants::kTileSize));
        E2E_ASSERT_TRUE(!adapter.IsTileSolid(SceneMode::Overworld, row, col));
    }
    for (int i = 0; i < 4; ++i) {
        for (int j = i + 1; j < 4; ++j) {
            const ColorRgba& ci = players[static_cast<std::size_t>(i)].color;
            const ColorRgba& cj = players[static_cast<std::size_t>(j)].color;
            const bool same_rgb = (ci.r == cj.r && ci.g == cj.g && ci.b == cj.b);
            E2E_ASSERT_TRUE(!same_rgb);
        }
    }
}

E2E_TEST(F13_T1_03_TopDownYAxisActiveAndZeroGravity, E2ETier::Tier1_Feature, "F13") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(8, 8, 0));
    adapter.SetPlayerPosition(0, {200.0f, 200.0f});

    adapter.StepFixed(30, MakeInputsP0(MakeDigitalInput(false, false, true, false)));
    const PlayerSnapshot p_up = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(200.0f - p_up.pos.y, 110.0f, 0.5f);

    adapter.StepFixed(30, MakeNeutralInputs(1));
    const PlayerSnapshot p_idle = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p_idle.pos.y, p_up.pos.y, 0.01f);
    E2E_ASSERT_NEAR(p_idle.y_velocity, 0.0f, 1e-4f);
}

E2E_TEST(F13_T1_04_StructurePortalTriggersTransitionToStructure, E2ETier::Tier1_Feature, "F13") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    auto grid = MakeGrid(5, 5, 0);
    grid[2][3] = 2;
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetPlayerPosition(0, {100.0f, 105.0f});

    bool transitioned = false;
    SceneMode final_mode = SceneMode::Overworld;
    for (int t = 0; t < 15; ++t) {
        const StepResult res = adapter.StepFixed(1, MakeInputsP0(MakeAnalogInput(1.0f, 0.0f)));
        if (res.transition_occurred) {
            transitioned = true;
            final_mode = res.current_mode;
            break;
        }
    }
    E2E_ASSERT_TRUE(transitioned);
    E2E_ASSERT_EQ(final_mode, SceneMode::Structure);
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Structure);
}

E2E_TEST(F13_T1_05_SolarMachineAndArchitectInteractiveInOverworld, E2ETier::Tier1_Feature, "F13") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(20, 20, 0));
    adapter.SetMachinePosition(MachineKind::SolarArray, {100.0f, 100.0f});

    const MachineSnapshot base_solar = adapter.GetMachine(MachineKind::SolarArray);
    E2E_ASSERT_TRUE(base_solar.trigger_radius > 0.0f);

    adapter.SetPlayerPosition(0, {85.0f, 85.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));
    const MachineSnapshot near_solar = adapter.GetMachine(MachineKind::SolarArray);
    E2E_ASSERT_TRUE(near_solar.prompt_visible);
    E2E_ASSERT_TRUE(!near_solar.prompt_text.empty());

    adapter.SetPlayerPosition(0, {100.0f + base_solar.trigger_radius + 100.0f, 100.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));
    const MachineSnapshot far_solar = adapter.GetMachine(MachineKind::SolarArray);
    E2E_ASSERT_TRUE(!far_solar.prompt_visible);

    adapter.SetPlayerPosition(0, adapter.GetArchitectNpc().pos);
    const auto npc = adapter.InteractArchitectNpc(0);
    E2E_ASSERT_TRUE(!npc.recruited);
}

E2E_TEST(F13_T1_06_OverworldVisualTintReflectsEcoPollution, E2ETier::Tier1_Feature, "F13") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    const EcoSnapshot eco0 = adapter.GetEcoSystem();

    adapter.SetPollutionLevel(85.0f);
    adapter.StepFixed(5, MakeNeutralInputs(1));
    const EcoSnapshot eco1 = adapter.GetEcoSystem();

    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Overworld);
    E2E_ASSERT_NEAR(eco1.pollution_level, 85.0f, 1e-4f);
    E2E_ASSERT_TRUE(eco1.smog_alpha > eco0.smog_alpha);
    E2E_ASSERT_LE(eco1.sample_white_tint.r, eco0.sample_white_tint.r);
    const int sum0 = static_cast<int>(eco0.sample_white_tint.r) +
                     static_cast<int>(eco0.sample_white_tint.g) +
                     static_cast<int>(eco0.sample_white_tint.b);
    const int sum1 = static_cast<int>(eco1.sample_white_tint.r) +
                     static_cast<int>(eco1.sample_white_tint.g) +
                     static_cast<int>(eco1.sample_white_tint.b);
    E2E_ASSERT_TRUE(sum1 < sum0);
}

E2E_TEST(F13_T2_01_InactivePlayerOnStructurePortalIgnored, E2ETier::Tier2_Boundary, "F13") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 2);
    auto grid = MakeGrid(5, 5, 0);
    grid[2][2] = 2;
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetPlayerPosition(0, {10.0f, 10.0f});
    adapter.SetPlayerPosition(1, {100.0f, 100.0f});
    adapter.SetPlayerActive(1, false);

    const StepResult res = adapter.StepFixed(10, MakeNeutralInputs(1));
    E2E_ASSERT_TRUE(!res.transition_occurred);
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Overworld);
}

E2E_TEST(F13_T2_02_SecondActivePlayerAloneTriggersPortal, E2ETier::Tier2_Boundary, "F13") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 2);
    auto grid = MakeGrid(5, 5, 0);
    grid[2][2] = 2;
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetPlayerPosition(0, {10.0f, 10.0f});
    adapter.SetPlayerPosition(1, {60.0f, 105.0f});

    std::array<InputFrame, 4> inputs{};
    inputs[0].active = true;
    inputs[1] = MakeAnalogInput(1.0f, 0.0f);

    bool transitioned = false;
    for (int t = 0; t < 15; ++t) {
        const StepResult res = adapter.StepFixed(1, inputs);
        if (res.transition_occurred) {
            transitioned = true;
            break;
        }
    }
    E2E_ASSERT_TRUE(transitioned);
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Structure);
}

E2E_TEST(F13_T2_03_ExitPortalTileInOverworldDoesNotEnterStructure, E2ETier::Tier2_Boundary, "F13") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    auto grid = MakeGrid(5, 5, 0);
    grid[2][2] = 3;
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetPlayerPosition(0, {60.0f, 105.0f});

    for (int t = 0; t < 30; ++t) {
        const StepResult res = adapter.StepFixed(1, MakeInputsP0(MakeAnalogInput(1.0f, 0.0f)));
        E2E_ASSERT_TRUE(!res.transition_occurred);
    }
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Overworld);
}

E2E_TEST(F13_T2_04_WallBlockedPortalCannotBeTriggeredThroughWall, E2ETier::Tier2_Boundary, "F13") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    auto grid = MakeGrid(5, 5, 0);
    grid[2][2] = 1;
    grid[2][3] = 2;
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetPlayerPosition(0, {50.0f, 105.0f});

    const StepResult res = adapter.StepFixed(60, MakeInputsP0(MakeAnalogInput(1.0f, 0.0f)));
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).pos.x, 66.0f, 0.1f);
    E2E_ASSERT_TRUE(!res.transition_occurred);
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Overworld);
}

E2E_TEST(F13_T2_05_DeferredPortalTransitionSingleStepCleanliness, E2ETier::Tier2_Boundary, "F13") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    auto grid = MakeGrid(5, 5, 0);
    grid[2][2] = 2;
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetPlayerPosition(0, {64.0f, 105.0f});

    const StepResult res = adapter.StepFixed(1, MakeInputsP0(MakeAnalogInput(1.0f, 0.0f)));
    E2E_ASSERT_EQ(res.fixed_steps_executed, 1);
    E2E_ASSERT_TRUE(res.transition_occurred);
    E2E_ASSERT_EQ(res.current_mode, SceneMode::Structure);
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Structure);
}

E2E_TEST(F13_T2_06_PollutionClampingExtremesInOverworld, E2ETier::Tier2_Boundary, "F13") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 4);

    adapter.SetPollutionLevel(-25.0f);
    adapter.StepFixed(5, MakeNeutralInputs(4));
    const EcoSnapshot eco_min = adapter.GetEcoSystem();
    E2E_ASSERT_NEAR(eco_min.pollution_level, ContractConstants::kMinEcoValue, 1e-4f);
    E2E_ASSERT_GE(eco_min.smog_alpha, 0.0f);

    adapter.SetPollutionLevel(150.0f);
    adapter.StepFixed(5, MakeNeutralInputs(4));
    const EcoSnapshot eco_max = adapter.GetEcoSystem();
    E2E_ASSERT_NEAR(eco_max.pollution_level, ContractConstants::kMaxEcoValue, 1e-4f);
    E2E_ASSERT_LE(eco_max.smog_alpha, 1.0f);
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Overworld);
    E2E_ASSERT_EQ(adapter.ActivePlayerCount(), 4);
}

// ============================================================================
// F14: Side-Scrolling Vertical Physics (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F14_T1_01_YWalkInputDisabledInStructure, E2ETier::Tier1_Feature, "F14") {
    E2E_ASSERT_NEAR(ContractConstants::kSideMoveSpeed, 210.0f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kGravityAccel, 1200.0f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kJumpForce, 480.0f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kMaxFallSpeed, 900.0f, 1e-6f);
    E2E_REQUIRE_LIVE(adapter);

    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(8, 8, 5));
    adapter.SetPlayerPosition(0, {144.0f, 210.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));

    adapter.StepFixed(30, MakeInputsP0(MakeDigitalInput(false, false, true, false)));
    PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.y, 210.0f, 1e-3f);
    E2E_ASSERT_NEAR(p.vel.y, 0.0f, 1e-3f);
    E2E_ASSERT_TRUE(p.is_grounded);

    adapter.StepFixed(30, MakeInputsP0(MakeDigitalInput(false, false, false, true)));
    p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.y, 210.0f, 1e-3f);
    E2E_ASSERT_NEAR(p.vel.y, 0.0f, 1e-3f);
    E2E_ASSERT_TRUE(p.is_grounded);
}

E2E_TEST(F14_T1_02_HorizontalSideMoveSpeed, E2ETier::Tier1_Feature, "F14") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(8, 12, 5));
    adapter.SetPlayerPosition(0, {96.0f, 210.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));

    adapter.StepFixed(30, MakeInputsP0(MakeDigitalInput(false, true)));
    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x, 96.0f + ContractConstants::kSideMoveSpeed * 0.5f, 0.5f);
    E2E_ASSERT_NEAR(p.pos.y, 210.0f, 1e-3f);
    E2E_ASSERT_TRUE(p.is_grounded);
}

E2E_TEST(F14_T1_03_HorizontalSpeedUnchangedByVerticalKeys, E2ETier::Tier1_Feature, "F14") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(8, 12, 5));
    adapter.SetPlayerPosition(0, {96.0f, 210.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));
    adapter.StepFixed(30, MakeInputsP0(MakeDigitalInput(false, true, false, false)));
    const float x_horiz_only = adapter.GetPlayer(0).pos.x;

    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(8, 12, 5));
    adapter.SetPlayerPosition(0, {96.0f, 210.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));
    adapter.StepFixed(30, MakeInputsP0(MakeDigitalInput(false, true, true, false)));
    const float x_with_up = adapter.GetPlayer(0).pos.x;

    E2E_ASSERT_NEAR(x_with_up, x_horiz_only, 1e-3f);
    E2E_ASSERT_NEAR(x_with_up, 201.0f, 0.5f);
}

E2E_TEST(F14_T1_04_GravityAccelerationInMidair, E2ETier::Tier1_Feature, "F14") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(16, 8, 14));
    adapter.SetPlayerPosition(0, {144.0f, 60.0f});
    adapter.SetPlayerVelocity(0, {0.0f, 0.0f}, 0.0f);
    adapter.StepFixed(15, MakeNeutralInputs(1));

    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_TRUE(!p.is_grounded);
    E2E_ASSERT_NEAR(p.y_velocity, 300.0f, 1.0f);
    E2E_ASSERT_TRUE(p.pos.y > 60.0f && p.pos.y < 120.0f);
}

E2E_TEST(F14_T1_05_GroundedJumpImpulseAndApex, E2ETier::Tier1_Feature, "F14") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(10, 8, 8));
    adapter.SetPlayerPosition(0, {144.0f, 354.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));
    E2E_ASSERT_TRUE(adapter.GetPlayer(0).is_grounded);

    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, true)));
    PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_TRUE(!p.is_grounded);
    E2E_ASSERT_LE(p.y_velocity, -460.0f);

    float min_y = p.pos.y;
    for (int t = 0; t < 59; ++t) {
        adapter.StepFixed(1, MakeNeutralInputs(1));
        p = adapter.GetPlayer(0);
        min_y = std::min(min_y, p.pos.y);
    }
    E2E_ASSERT_NEAR(354.0f - min_y, 96.0f, 6.0f);
    E2E_ASSERT_NEAR(p.pos.y, 354.0f, 1e-2f);
    E2E_ASSERT_TRUE(p.is_grounded);
    E2E_ASSERT_NEAR(p.y_velocity, 0.0f, 1e-3f);
}

E2E_TEST(F14_T1_06_MultiplayerVerticalIsolation, E2ETier::Tier1_Feature, "F14") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 2);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(10, 8, 8));
    adapter.SetPlayerPosition(0, {96.0f, 354.0f});
    adapter.SetPlayerPosition(1, {240.0f, 354.0f});
    adapter.StepFixed(1, MakeNeutralInputs(2));

    std::array<InputFrame, 4> jump_p0{};
    jump_p0[0] = MakeDigitalInput(false, true, false, false, true, true);
    jump_p0[1].active = true;
    adapter.StepFixed(1, jump_p0);

    std::array<InputFrame, 4> move_p0{};
    move_p0[0] = MakeDigitalInput(false, true, false, false, false, false);
    move_p0[1].active = true;
    adapter.StepFixed(9, move_p0);

    const PlayerSnapshot p0 = adapter.GetPlayer(0);
    const PlayerSnapshot p1 = adapter.GetPlayer(1);
    E2E_ASSERT_TRUE(!p0.is_grounded);
    E2E_ASSERT_TRUE(p0.pos.y < 354.0f);
    E2E_ASSERT_TRUE(p0.pos.x > 96.0f);

    E2E_ASSERT_TRUE(p1.is_grounded);
    E2E_ASSERT_NEAR(p1.pos.x, 240.0f, 1e-3f);
    E2E_ASSERT_NEAR(p1.pos.y, 354.0f, 1e-3f);
    E2E_ASSERT_NEAR(p1.y_velocity, 0.0f, 1e-3f);
}

E2E_TEST(F14_T2_01_MaxFallSpeedClamp, E2ETier::Tier2_Boundary, "F14") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(26, 8, 24));
    adapter.SetPlayerPosition(0, {144.0f, 60.0f});
    adapter.StepFixed(50, MakeNeutralInputs(1));

    const PlayerSnapshot p50 = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p50.y_velocity, ContractConstants::kMaxFallSpeed, 1e-3f);
    E2E_ASSERT_LE(p50.y_velocity, ContractConstants::kMaxFallSpeed + 1e-4f);

    adapter.StepFixed(10, MakeNeutralInputs(1));
    const PlayerSnapshot p60 = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p60.y_velocity, ContractConstants::kMaxFallSpeed, 1e-3f);
    E2E_ASSERT_NEAR(p60.pos.y - p50.pos.y, 150.0f, 1e-2f);
}

E2E_TEST(F14_T2_02_PreExistingExcessFallSpeedClamped, E2ETier::Tier2_Boundary, "F14") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(26, 8, 24));
    adapter.SetPlayerPosition(0, {144.0f, 60.0f});
    adapter.SetPlayerVelocity(0, {0.0f, 0.0f}, 1500.0f);
    adapter.StepFixed(1, MakeNeutralInputs(1));

    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_LE(p.y_velocity, ContractConstants::kMaxFallSpeed + 1e-4f);
    E2E_ASSERT_TRUE(std::isfinite(p.pos.y));
}

E2E_TEST(F14_T2_03_OpposingHorizontalKeysCancel, E2ETier::Tier2_Boundary, "F14") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(8, 8, 5));
    adapter.SetPlayerPosition(0, {144.0f, 210.0f});
    adapter.StepFixed(20, MakeInputsP0(MakeDigitalInput(true, true)));

    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x, 144.0f, 1e-3f);
    E2E_ASSERT_NEAR(p.vel.x, 0.0f, 1e-3f);
}

E2E_TEST(F14_T2_04_AnalogDeadzoneBoundaryInSideScroll, E2ETier::Tier2_Boundary, "F14") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(8, 8, 5));
    adapter.SetPlayerPosition(0, {144.0f, 210.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));

    InputFrame sub_dz{};
    sub_dz.active = true;
    sub_dz.gamepad_connected = true;
    sub_dz.gamepad_axis_x = 0.19f;
    adapter.StepFixed(30, MakeInputsP0(sub_dz));
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).pos.x, 144.0f, 1e-3f);

    InputFrame over_dz{};
    over_dz.active = true;
    over_dz.gamepad_connected = true;
    over_dz.gamepad_axis_x = 0.21f;
    adapter.StepFixed(30, MakeInputsP0(over_dz));
    E2E_ASSERT_TRUE(adapter.GetPlayer(0).pos.x > 144.0f);
}

E2E_TEST(F14_T2_05_InactivePlayerIgnoresGravity, E2ETier::Tier2_Boundary, "F14") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 2);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(10, 8, 8));
    adapter.SetPlayerPosition(0, {96.0f, 60.0f});
    adapter.SetPlayerPosition(1, {192.0f, 60.0f});
    adapter.SetPlayerActive(1, false);
    adapter.StepFixed(20, MakeNeutralInputs(1));

    const PlayerSnapshot p0 = adapter.GetPlayer(0);
    const PlayerSnapshot p1 = adapter.GetPlayer(1);
    E2E_ASSERT_TRUE(p0.pos.y > 60.0f);
    E2E_ASSERT_TRUE(p0.y_velocity > 0.0f);
    E2E_ASSERT_NEAR(p1.pos.y, 60.0f, 1e-4f);
    E2E_ASSERT_NEAR(p1.y_velocity, 0.0f, 1e-4f);
}

E2E_TEST(F14_T2_06_ZeroDeltaAndClampedSpikeDelta, E2ETier::Tier2_Boundary, "F14") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(26, 8, 24));
    adapter.SetPlayerPosition(0, {144.0f, 60.0f});

    const StepResult res0 = adapter.AdvanceFrame(0.0f, MakeNeutralInputs(1));
    E2E_ASSERT_EQ(res0.fixed_steps_executed, 0);
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).pos.y, 60.0f, 1e-4f);
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).y_velocity, 0.0f, 1e-4f);

    const StepResult res1 = adapter.AdvanceFrame(0.5f, MakeNeutralInputs(1));
    E2E_ASSERT_EQ(res1.fixed_steps_executed, 15);
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).y_velocity, 300.0f, 1e-2f);
}

// ============================================================================
// F15: Sub-Stepped Vertical AABB Collision (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F15_T1_01_FloorLandingResetsVelocityAndGrounded, E2ETier::Tier1_Feature, "F15") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(8, 8, 5));
    adapter.SetPlayerPosition(0, {144.0f, 180.0f});
    adapter.StepFixed(30, MakeNeutralInputs(1));

    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.y, 210.0f, 1e-3f);
    E2E_ASSERT_NEAR(p.y_velocity, 0.0f, 1e-3f);
    E2E_ASSERT_TRUE(p.is_grounded);
}

E2E_TEST(F15_T1_02_CeilingHitClearsVelocityWithoutGrounded, E2ETier::Tier1_Feature, "F15") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    auto grid = MakeChamberGrid(6, 6, 3);
    for (int c = 0; c < 6; ++c) {
        grid[1][static_cast<std::size_t>(c)] = 1;
    }
    adapter.LoadCustomTilemap(SceneMode::Structure, grid);
    adapter.SetPlayerPosition(0, {96.0f, 114.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));
    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, true)));
    adapter.StepFixed(2, MakeNeutralInputs(1));

    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_GE(p.pos.y, 96.0f - 1e-3f);
    E2E_ASSERT_GE(p.y_velocity, 0.0f);
    E2E_ASSERT_TRUE(!p.is_grounded);
}

E2E_TEST(F15_T1_03_HighSpeedSubSteppingPreventsFloorTunneling, E2ETier::Tier1_Feature, "F15") {
    E2E_ASSERT_NEAR(ContractConstants::kMaxVerticalSubStep, 8.0f, 1e-6f);
    E2E_REQUIRE_LIVE(adapter);

    adapter.ResetSimulation(SceneMode::Structure, 1);
    auto grid = MakeGrid(8, 8, 0);
    for (int c = 0; c < 8; ++c) {
        grid[4][static_cast<std::size_t>(c)] = 1;
    }
    adapter.LoadCustomTilemap(SceneMode::Structure, grid);

    adapter.SetPlayerPosition(0, {144.0f, 152.0f});
    adapter.SetPlayerVelocity(0, {0.0f, 0.0f}, 900.0f);
    adapter.StepFixed(1, MakeNeutralInputs(1));
    PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.y, 162.0f, 1e-3f);
    E2E_ASSERT_NEAR(p.y_velocity, 0.0f, 1e-3f);
    E2E_ASSERT_TRUE(p.is_grounded);

    adapter.SetPlayerPosition(0, {144.0f, 160.0f});
    adapter.SetPlayerVelocity(0, {0.0f, 0.0f}, 5400.0f);
    adapter.StepFixed(1, MakeNeutralInputs(1));
    p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.y, 162.0f, 1e-3f);
    E2E_ASSERT_NEAR(p.y_velocity, 0.0f, 1e-3f);
    E2E_ASSERT_TRUE(p.is_grounded);
}

E2E_TEST(F15_T1_04_AxisSeparatedHorizontalWallWhileFalling, E2ETier::Tier1_Feature, "F15") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    auto grid = MakeChamberGrid(10, 8, 8);
    for (int r = 1; r <= 7; ++r) {
        grid[static_cast<std::size_t>(r)][4] = 1;
    }
    adapter.LoadCustomTilemap(SceneMode::Structure, grid);
    adapter.SetPlayerPosition(0, {162.0f, 60.0f});
    adapter.StepFixed(20, MakeInputsP0(MakeDigitalInput(false, true)));

    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x, 162.0f, 1e-3f);
    E2E_ASSERT_TRUE(p.pos.y > 110.0f);
}

E2E_TEST(F15_T1_05_WalkOffLedgeClearsGrounded, E2ETier::Tier1_Feature, "F15") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeLedgeGrid());
    adapter.SetPlayerPosition(0, {120.0f, 162.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));
    E2E_ASSERT_TRUE(adapter.GetPlayer(0).is_grounded);

    adapter.StepFixed(15, MakeInputsP0(MakeDigitalInput(false, true)));
    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_TRUE(!p.is_grounded);
    E2E_ASSERT_TRUE(p.y_velocity > 0.0f);
    E2E_ASSERT_TRUE(p.pos.y > 162.0f);
}

E2E_TEST(F15_T1_06_SingleTileShaftFallNoWallSnagging, E2ETier::Tier1_Feature, "F15") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    auto grid = MakeChamberGrid(10, 5, 9);
    for (int r = 1; r <= 8; ++r) {
        grid[static_cast<std::size_t>(r)][1] = 1;
        grid[static_cast<std::size_t>(r)][3] = 1;
    }
    adapter.LoadCustomTilemap(SceneMode::Structure, grid);
    adapter.SetPlayerPosition(0, {105.0f, 60.0f});
    adapter.StepFixed(60, MakeNeutralInputs(1));

    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x, 105.0f, 1e-3f);
    E2E_ASSERT_NEAR(p.pos.y, 402.0f, 1e-3f);
    E2E_ASSERT_TRUE(p.is_grounded);
    E2E_ASSERT_NEAR(p.y_velocity, 0.0f, 1e-3f);
}

E2E_TEST(F15_T2_01_SubStepExactBoundary8PxVs9Px, E2ETier::Tier2_Boundary, "F15") {
    E2E_REQUIRE_LIVE(adapter);

    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(8, 8, 5));
    adapter.SetPlayerPosition(0, {144.0f, 206.0f});
    adapter.SetPlayerVelocity(0, {0.0f, 0.0f}, 480.0f);
    adapter.StepFixed(1, MakeNeutralInputs(1));
    PlayerSnapshot pA = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(pA.pos.y, 210.0f, 1e-3f);
    E2E_ASSERT_NEAR(pA.y_velocity, 0.0f, 1e-3f);
    E2E_ASSERT_TRUE(pA.is_grounded);

    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(8, 8, 5));
    adapter.SetPlayerPosition(0, {144.0f, 206.0f});
    adapter.SetPlayerVelocity(0, {0.0f, 0.0f}, 540.0f);
    adapter.StepFixed(1, MakeNeutralInputs(1));
    PlayerSnapshot pB = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(pB.pos.y, 210.0f, 1e-3f);
    E2E_ASSERT_NEAR(pB.y_velocity, 0.0f, 1e-3f);
    E2E_ASSERT_TRUE(pB.is_grounded);
}

E2E_TEST(F15_T2_02_UpwardSubStepPreventsCeilingTunneling, E2ETier::Tier2_Boundary, "F15") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    auto grid = MakeGrid(8, 8, 0);
    for (int r = 0; r < 8; ++r) {
        grid[static_cast<std::size_t>(r)][0] = 1;
        grid[static_cast<std::size_t>(r)][7] = 1;
    }
    for (int c = 0; c < 8; ++c) {
        grid[2][static_cast<std::size_t>(c)] = 1;
    }
    adapter.LoadCustomTilemap(SceneMode::Structure, grid);
    adapter.SetPlayerPosition(0, {144.0f, 146.0f});
    adapter.SetPlayerVelocity(0, {0.0f, 0.0f}, -5400.0f);
    adapter.StepFixed(1, MakeNeutralInputs(1));

    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_GE(p.pos.y, 144.0f - 1e-3f);
    E2E_ASSERT_LE(p.pos.y, 145.0f);
    E2E_ASSERT_GE(p.y_velocity, 0.0f);
    E2E_ASSERT_TRUE(!p.is_grounded);
}

E2E_TEST(F15_T2_03_ExactFlushWallAndFloorCornerNoSnag, E2ETier::Tier2_Boundary, "F15") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(8, 8, 5));
    adapter.SetPlayerPosition(0, {48.0f, 210.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));
    adapter.StepFixed(10, MakeInputsP0(MakeDigitalInput(false, true)));

    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.x, 48.0f + ContractConstants::kSideMoveSpeed * (10.0f / 60.0f), 0.5f);
    E2E_ASSERT_NEAR(p.pos.y, 210.0f, 1e-3f);
    E2E_ASSERT_TRUE(p.is_grounded);
}

E2E_TEST(F15_T2_04_OnePixelFootOverlapVsZeroOverlap, E2ETier::Tier2_Boundary, "F15") {
    E2E_REQUIRE_LIVE(adapter);

    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeLedgeGrid());
    adapter.SetPlayerPosition(0, {143.0f, 162.0f});
    adapter.StepFixed(5, MakeNeutralInputs(1));
    const PlayerSnapshot pA = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(pA.pos.y, 162.0f, 1e-3f);
    E2E_ASSERT_TRUE(pA.is_grounded);

    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeLedgeGrid());
    adapter.SetPlayerPosition(0, {144.0f, 162.0f});
    adapter.StepFixed(5, MakeNeutralInputs(1));
    const PlayerSnapshot pB = adapter.GetPlayer(0);
    E2E_ASSERT_TRUE(pB.pos.y > 162.0f);
    E2E_ASSERT_TRUE(!pB.is_grounded);
}

E2E_TEST(F15_T2_05_OutOfBoundsVerticalSolidClamping, E2ETier::Tier2_Boundary, "F15") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeGrid(6, 6, 0));

    adapter.SetPlayerPosition(0, {120.0f, 250.0f});
    adapter.SetPlayerVelocity(0, {0.0f, 0.0f}, 900.0f);
    adapter.StepFixed(5, MakeNeutralInputs(1));
    PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_LE(p.pos.y, 258.0f + 1e-3f);
    E2E_ASSERT_TRUE(p.is_grounded);

    adapter.SetPlayerPosition(0, {120.0f, 4.0f});
    adapter.SetPlayerVelocity(0, {0.0f, 0.0f}, -480.0f);
    adapter.StepFixed(5, MakeNeutralInputs(1));
    p = adapter.GetPlayer(0);
    E2E_ASSERT_GE(p.pos.y, -1e-3f);
    E2E_ASSERT_TRUE(std::isfinite(p.pos.y));
}

E2E_TEST(F15_T2_06_PartialCeilingCornerOverlapDoesNotBlockClearColumn, E2ETier::Tier2_Boundary, "F15") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    auto grid = MakeChamberGrid(8, 8, 6);
    grid[3][2] = 1;
    adapter.LoadCustomTilemap(SceneMode::Structure, grid);
    adapter.SetPlayerPosition(0, {144.0f, 258.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));
    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, true)));
    adapter.StepFixed(22, MakeNeutralInputs(1));

    E2E_ASSERT_TRUE(adapter.GetPlayer(0).pos.y < 192.0f);
}

// ============================================================================
// F16: Coyote Time & Jump Buffer (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F16_T1_01_CoyoteTimeAllowsJumpAfterLedgeDrop, E2ETier::Tier1_Feature, "F16") {
    E2E_ASSERT_NEAR(ContractConstants::kCoyoteTimeDuration, 0.15f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kJumpBufferDuration, 0.12f, 1e-6f);
    E2E_REQUIRE_LIVE(adapter);

    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeLedgeGrid());
    adapter.SetPlayerPosition(0, {136.0f, 162.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));

    for (int t = 0; t < 10 && adapter.GetPlayer(0).is_grounded; ++t) {
        adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, true)));
    }
    E2E_ASSERT_TRUE(!adapter.GetPlayer(0).is_grounded);

    adapter.StepFixed(3, MakeNeutralInputs(1));
    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, true)));
    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_LE(p.y_velocity, -460.0f);
    E2E_ASSERT_TRUE(!p.is_grounded);
}

E2E_TEST(F16_T1_02_CoyoteTimeExpiredRejectsJump, E2ETier::Tier1_Feature, "F16") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeLedgeGrid());
    adapter.SetPlayerPosition(0, {136.0f, 162.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));

    for (int t = 0; t < 10 && adapter.GetPlayer(0).is_grounded; ++t) {
        adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, true)));
    }
    E2E_ASSERT_TRUE(!adapter.GetPlayer(0).is_grounded);

    adapter.StepFixed(11, MakeNeutralInputs(1));
    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, true)));
    E2E_ASSERT_TRUE(adapter.GetPlayer(0).y_velocity > 0.0f);
}

E2E_TEST(F16_T1_03_JumpBufferTriggersImmediatelyOnLanding, E2ETier::Tier1_Feature, "F16") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(10, 8, 8));
    adapter.SetPlayerPosition(0, {144.0f, 330.0f});
    adapter.SetPlayerVelocity(0, {0.0f, 0.0f}, 600.0f);

    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, true)));
    adapter.StepFixed(5, MakeNeutralInputs(1));
    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_TRUE(p.y_velocity < 0.0f);
    E2E_ASSERT_TRUE(!p.is_grounded);
    E2E_ASSERT_TRUE(p.pos.y < 354.0f);
}

E2E_TEST(F16_T1_04_JumpBufferExpiredDoesNotJump, E2ETier::Tier1_Feature, "F16") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(10, 8, 8));
    adapter.SetPlayerPosition(0, {144.0f, 200.0f});

    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, false)));
    adapter.StepFixed(45, MakeNeutralInputs(1));
    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p.pos.y, 354.0f, 1e-3f);
    E2E_ASSERT_TRUE(p.is_grounded);
    E2E_ASSERT_NEAR(p.y_velocity, 0.0f, 1e-3f);
}

E2E_TEST(F16_T1_05_SingleJumpConsumptionPreventsDoubleCoyote, E2ETier::Tier1_Feature, "F16") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeLedgeGrid());
    adapter.SetPlayerPosition(0, {136.0f, 162.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));

    for (int t = 0; t < 10 && adapter.GetPlayer(0).is_grounded; ++t) {
        adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, true)));
    }
    E2E_ASSERT_TRUE(!adapter.GetPlayer(0).is_grounded);

    adapter.StepFixed(1, MakeNeutralInputs(1));
    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, true)));
    const PlayerSnapshot p1 = adapter.GetPlayer(0);
    E2E_ASSERT_LE(p1.y_velocity, -460.0f);

    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, true)));
    const PlayerSnapshot p2 = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(p2.y_velocity, p1.y_velocity + 20.0f, 1.0f);
}

E2E_TEST(F16_T1_06_NormalJumpGrantsZeroCoyote, E2ETier::Tier1_Feature, "F16") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(10, 8, 8));
    adapter.SetPlayerPosition(0, {144.0f, 354.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));

    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, true)));
    adapter.StepFixed(2, MakeNeutralInputs(1));
    const PlayerSnapshot before_second = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(before_second.coyote_timer, 0.0f, 1e-4f);

    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, true)));
    const PlayerSnapshot after_second = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(after_second.y_velocity, before_second.y_velocity + 20.0f, 1.0f);
}

E2E_TEST(F16_T2_01_CoyoteExactTickBoundary8And9Vs10, E2ETier::Tier2_Boundary, "F16") {
    E2E_REQUIRE_LIVE(adapter);

    auto run_coyote_after_neutral = [&](int neutral_air_ticks) -> PlayerSnapshot {
        adapter.ResetSimulation(SceneMode::Structure, 1);
        adapter.LoadCustomTilemap(SceneMode::Structure, MakeLedgeGrid());
        adapter.SetPlayerPosition(0, {143.0f, 162.0f});
        adapter.StepFixed(1, MakeNeutralInputs(1));
        adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, true)));
        E2E_ASSERT_TRUE(!adapter.GetPlayer(0).is_grounded);
        if (neutral_air_ticks > 0) {
            adapter.StepFixed(neutral_air_ticks, MakeNeutralInputs(1));
        }
        adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, true)));
        return adapter.GetPlayer(0);
    };

    const PlayerSnapshot p8 = run_coyote_after_neutral(7);
    E2E_ASSERT_LE(p8.y_velocity, -460.0f);

    const PlayerSnapshot p9 = run_coyote_after_neutral(8);
    E2E_ASSERT_LE(p9.y_velocity, -460.0f);

    const PlayerSnapshot p10 = run_coyote_after_neutral(9);
    E2E_ASSERT_TRUE(p10.y_velocity > 0.0f);
}

E2E_TEST(F16_T2_02_JumpBufferExactTickBoundary7Vs9, E2ETier::Tier2_Boundary, "F16") {
    E2E_REQUIRE_LIVE(adapter);

    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(10, 8, 8));
    adapter.SetPlayerPosition(0, {144.0f, 282.0f});
    adapter.SetPlayerVelocity(0, {0.0f, 0.0f}, 600.0f);
    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, false)));
    adapter.StepFixed(6, MakeNeutralInputs(1));
    const PlayerSnapshot pA = adapter.GetPlayer(0);
    E2E_ASSERT_TRUE(pA.y_velocity < 0.0f);
    E2E_ASSERT_TRUE(!pA.is_grounded);

    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(10, 8, 8));
    adapter.SetPlayerPosition(0, {144.0f, 258.0f});
    adapter.SetPlayerVelocity(0, {0.0f, 0.0f}, 600.0f);
    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, false)));
    adapter.StepFixed(8, MakeNeutralInputs(1));
    const PlayerSnapshot pB = adapter.GetPlayer(0);
    E2E_ASSERT_NEAR(pB.pos.y, 354.0f, 1e-3f);
    E2E_ASSERT_TRUE(pB.is_grounded);
    E2E_ASSERT_NEAR(pB.y_velocity, 0.0f, 1e-3f);
}

E2E_TEST(F16_T2_03_JumpBufferSingleConsumptionNoSecondBounce, E2ETier::Tier2_Boundary, "F16") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(10, 8, 8));
    adapter.SetPlayerPosition(0, {144.0f, 340.0f});
    adapter.SetPlayerVelocity(0, {0.0f, 0.0f}, 600.0f);

    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, false)));
    float min_y = 354.0f;
    for (int t = 0; t < 60; ++t) {
        adapter.StepFixed(1, MakeNeutralInputs(1));
        min_y = std::min(min_y, adapter.GetPlayer(0).pos.y);
    }
    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_TRUE(min_y < 340.0f);
    E2E_ASSERT_NEAR(p.pos.y, 354.0f, 1e-3f);
    E2E_ASSERT_TRUE(p.is_grounded);
    E2E_ASSERT_NEAR(p.y_velocity, 0.0f, 1e-3f);
    E2E_ASSERT_NEAR(p.jump_buffer_timer, 0.0f, 1e-4f);
}

E2E_TEST(F16_T2_04_HoldingJumpHeldDoesNotAutoPogo, E2ETier::Tier2_Boundary, "F16") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(10, 8, 8));
    adapter.SetPlayerPosition(0, {144.0f, 354.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));

    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, true)));
    adapter.StepFixed(59, MakeInputsP0(MakeDigitalInput(false, false, false, false, false, true)));
    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_TRUE(p.is_grounded);
    E2E_ASSERT_NEAR(p.pos.y, 354.0f, 1e-3f);
    E2E_ASSERT_NEAR(p.y_velocity, 0.0f, 1e-3f);
}

E2E_TEST(F16_T2_05_CeilingHitGrantsZeroCoyoteAndBuffer, E2ETier::Tier2_Boundary, "F16") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    auto grid = MakeChamberGrid(6, 6, 3);
    for (int c = 0; c < 6; ++c) {
        grid[1][static_cast<std::size_t>(c)] = 1;
    }
    adapter.LoadCustomTilemap(SceneMode::Structure, grid);
    adapter.SetPlayerPosition(0, {96.0f, 114.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));

    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, true)));
    adapter.StepFixed(2, MakeNeutralInputs(1));
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).coyote_timer, 0.0f, 1e-4f);

    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, true)));
    E2E_ASSERT_TRUE(adapter.GetPlayer(0).y_velocity > 0.0f);
}

E2E_TEST(F16_T2_06_MultiplayerCoyoteAndBufferIsolation, E2ETier::Tier2_Boundary, "F16") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 2);
    auto grid = MakeChamberGrid(10, 12, 9);
    grid[4][1] = 1;
    grid[4][2] = 1;
    grid[4][6] = 1;
    grid[4][7] = 1;
    adapter.LoadCustomTilemap(SceneMode::Structure, grid);

    adapter.SetPlayerPosition(0, {143.0f, 162.0f});
    adapter.SetPlayerPosition(1, {383.0f, 162.0f});
    adapter.StepFixed(1, MakeNeutralInputs(2));

    std::array<InputFrame, 4> walk_off{};
    walk_off[0] = MakeDigitalInput(false, true);
    walk_off[1] = MakeDigitalInput(false, true);
    adapter.StepFixed(1, walk_off);
    E2E_ASSERT_TRUE(!adapter.GetPlayer(0).is_grounded);
    E2E_ASSERT_TRUE(!adapter.GetPlayer(1).is_grounded);

    adapter.StepFixed(2, MakeNeutralInputs(2));
    std::array<InputFrame, 4> p0_coyote{};
    p0_coyote[0] = MakeDigitalInput(false, false, false, false, true, true);
    p0_coyote[1].active = true;
    adapter.StepFixed(1, p0_coyote);
    E2E_ASSERT_LE(adapter.GetPlayer(0).y_velocity, -460.0f);
    E2E_ASSERT_TRUE(adapter.GetPlayer(1).y_velocity > 0.0f);

    adapter.StepFixed(8, MakeNeutralInputs(2));
    std::array<InputFrame, 4> p1_late{};
    p1_late[0].active = true;
    p1_late[1] = MakeDigitalInput(false, false, false, false, true, true);
    adapter.StepFixed(1, p1_late);
    E2E_ASSERT_TRUE(adapter.GetPlayer(1).y_velocity > 0.0f);
}

// ============================================================================
// F17: Bidirectional OverworldState <-> StructureState (6 Tier 1 + 6 Tier 2)
// ============================================================================

E2E_TEST(F17_T1_01_OverworldStructurePortalEntersStructure, E2ETier::Tier1_Feature, "F17") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    auto grid = MakeGrid(8, 8, 0);
    grid[3][4] = 2;
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetPlayerPosition(0, {150.0f, 144.0f});

    bool transitioned = false;
    for (int t = 0; t < 15; ++t) {
        const StepResult res = adapter.StepFixed(1, MakeInputsP0(MakeAnalogInput(1.0f, 0.0f)));
        if (res.transition_occurred) {
            transitioned = true;
            break;
        }
    }
    E2E_ASSERT_TRUE(transitioned);
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Structure);
    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_TRUE(std::isfinite(p.pos.x));
    E2E_ASSERT_TRUE(std::isfinite(p.pos.y));
}

E2E_TEST(F17_T1_02_StructureExitPortalReturnsToOverworld, E2ETier::Tier1_Feature, "F17") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    auto grid = MakeChamberGrid(8, 8, 5);
    grid[4][5] = 3;
    adapter.LoadCustomTilemap(SceneMode::Structure, grid);
    adapter.SetPlayerPosition(0, {192.0f, 210.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));

    bool transitioned = false;
    for (int t = 0; t < 20; ++t) {
        const StepResult res = adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, true)));
        if (res.transition_occurred) {
            transitioned = true;
            break;
        }
    }
    E2E_ASSERT_TRUE(transitioned);
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Overworld);
}

E2E_TEST(F17_T1_03_BidirectionalRoundTripAnd30FrameGuard, E2ETier::Tier1_Feature, "F17") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.RequestModeTransition(SceneMode::Structure);
    adapter.StepFixed(1, MakeNeutralInputs(1));
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Structure);

    auto struct_grid = MakeChamberGrid(8, 8, 5);
    struct_grid[4][5] = 3;
    adapter.LoadCustomTilemap(SceneMode::Structure, struct_grid);
    adapter.SetPlayerPosition(0, {192.0f, 210.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));

    bool returned = false;
    for (int t = 0; t < 20; ++t) {
        const StepResult res = adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, true)));
        if (res.transition_occurred) {
            returned = true;
            break;
        }
    }
    E2E_ASSERT_TRUE(returned);
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Overworld);

    for (int t = 0; t < 30; ++t) {
        const StepResult idle_res = adapter.StepFixed(1, MakeNeutralInputs(1));
        E2E_ASSERT_TRUE(!idle_res.transition_occurred);
        E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Overworld);
    }
}

E2E_TEST(F17_T1_04_EcoSystemPersistsAcrossRoundTrip, E2ETier::Tier1_Feature, "F17") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(10, 10, 0));
    adapter.SetMachinePosition(MachineKind::SolarArray, {100.0f, 100.0f});
    adapter.SetPlayerPosition(0, {85.0f, 85.0f});
    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, false, false, true)));
    E2E_ASSERT_TRUE(adapter.GetCalibrationUi().is_open);
    adapter.TriggerCombustionBypass();
    adapter.SetPollutionLevel(72.5f);

    const EcoSnapshot before = adapter.GetEcoSystem();
    E2E_ASSERT_TRUE(before.dirty_energy > 0.0f);

    adapter.RequestModeTransition(SceneMode::Structure);
    adapter.StepFixed(1, MakeNeutralInputs(1));
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Structure);
    const EcoSnapshot mid = adapter.GetEcoSystem();
    E2E_ASSERT_NEAR(mid.pollution_level, before.pollution_level, 1e-4f);
    E2E_ASSERT_NEAR(mid.dirty_energy, before.dirty_energy, 1e-4f);
    E2E_ASSERT_NEAR(mid.clean_energy, before.clean_energy, 1e-4f);

    adapter.RequestModeTransition(SceneMode::Overworld);
    adapter.StepFixed(1, MakeNeutralInputs(1));
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Overworld);
    const EcoSnapshot after = adapter.GetEcoSystem();
    E2E_ASSERT_NEAR(after.pollution_level, before.pollution_level, 1e-4f);
    E2E_ASSERT_NEAR(after.clean_energy, before.clean_energy, 1e-4f);
    E2E_ASSERT_NEAR(after.dirty_energy, before.dirty_energy, 1e-4f);
    E2E_ASSERT_EQ(after.ui_phase, before.ui_phase);
    E2E_ASSERT_EQ(after.architect_recruited, before.architect_recruited);
}

E2E_TEST(F17_T1_05_MultiplayerNonZeroPlayerTriggersPartyTransition, E2ETier::Tier1_Feature, "F17") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 4);
    auto grid = MakeGrid(8, 8, 0);
    grid[3][5] = 2;
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);

    adapter.SetPlayerPosition(0, {96.0f, 96.0f});
    adapter.SetPlayerPosition(1, {96.0f, 144.0f});
    adapter.SetPlayerPosition(2, {144.0f, 96.0f});
    adapter.SetPlayerPosition(3, {195.0f, 144.0f});

    auto inputs = MakeNeutralInputs(4);
    inputs[3] = MakeAnalogInput(1.0f, 0.0f);
    bool transitioned = false;
    for (int t = 0; t < 15; ++t) {
        const StepResult res = adapter.StepFixed(1, inputs);
        if (res.transition_occurred) {
            transitioned = true;
            break;
        }
    }
    E2E_ASSERT_TRUE(transitioned);
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Structure);
    E2E_ASSERT_EQ(adapter.ActivePlayerCount(), 4);
    for (int i = 0; i < 4; ++i) {
        const PlayerSnapshot p = adapter.GetPlayer(i);
        E2E_ASSERT_TRUE(std::isfinite(p.pos.x));
        E2E_ASSERT_TRUE(std::isfinite(p.pos.y));
    }
}

E2E_TEST(F17_T1_06_PhysicsModeAndCameraUpdateOnTransition, E2ETier::Tier1_Feature, "F17") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 2);
    adapter.RequestModeTransition(SceneMode::Structure);
    adapter.StepFixed(1, MakeNeutralInputs(2));
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Structure);

    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(8, 8, 5));
    adapter.SetPlayerPosition(0, {96.0f, 210.0f});
    adapter.SetPlayerPosition(1, {144.0f, 210.0f});
    adapter.StepFixed(20, MakeInputsP0(MakeDigitalInput(false, false, true, false)));
    E2E_ASSERT_NEAR(adapter.GetPlayer(0).pos.y, 210.0f, 1e-3f);
    const CameraSnapshot cam_struct = adapter.GetCamera();
    E2E_ASSERT_TRUE(std::isfinite(cam_struct.target.x));
    E2E_ASSERT_TRUE(std::isfinite(cam_struct.target.y));

    adapter.RequestModeTransition(SceneMode::Overworld);
    adapter.StepFixed(1, MakeNeutralInputs(2));
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Overworld);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeGrid(10, 10, 0));
    adapter.SetPlayerPosition(0, {200.0f, 200.0f});
    adapter.StepFixed(20, MakeInputsP0(MakeDigitalInput(false, false, true, false)));
    const PlayerSnapshot p_over = adapter.GetPlayer(0);
    E2E_ASSERT_TRUE(p_over.pos.y - 200.0f < -40.0f);
    E2E_ASSERT_NEAR(p_over.y_velocity, 0.0f, 1e-4f);
}

E2E_TEST(F17_T2_01_ZeroOverlapAdjacentToPortalDoesNotTrigger, E2ETier::Tier2_Boundary, "F17") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    auto grid = MakeGrid(8, 8, 0);
    grid[3][4] = 2;
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetPlayerPosition(0, {162.0f, 144.0f});

    const StepResult idle_res = adapter.StepFixed(10, MakeNeutralInputs(1));
    E2E_ASSERT_TRUE(!idle_res.transition_occurred);
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Overworld);

    const StepResult step_res = adapter.StepFixed(2, MakeInputsP0(MakeDigitalInput(false, true)));
    E2E_ASSERT_TRUE(step_res.transition_occurred);
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Structure);
}

E2E_TEST(F17_T2_02_InactivePlayerOnPortalDoesNotTrigger, E2ETier::Tier2_Boundary, "F17") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 2);
    auto grid = MakeGrid(8, 8, 0);
    grid[3][4] = 2;
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetPlayerActive(1, false);
    adapter.SetPlayerPosition(0, {96.0f, 96.0f});
    adapter.SetPlayerPosition(1, {195.0f, 147.0f});

    const StepResult res = adapter.StepFixed(15, MakeNeutralInputs(1));
    E2E_ASSERT_TRUE(!res.transition_occurred);
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Overworld);
}

E2E_TEST(F17_T2_03_WrongPortalTileTypeInModeIgnored, E2ETier::Tier2_Boundary, "F17") {
    E2E_REQUIRE_LIVE(adapter);

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    auto ow_grid = MakeGrid(8, 8, 0);
    ow_grid[3][4] = 3;
    adapter.LoadCustomTilemap(SceneMode::Overworld, ow_grid);
    adapter.SetPlayerPosition(0, {150.0f, 144.0f});
    for (int t = 0; t < 15; ++t) {
        const StepResult res = adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, true)));
        E2E_ASSERT_TRUE(!res.transition_occurred);
    }
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Overworld);

    adapter.ResetSimulation(SceneMode::Structure, 1);
    auto st_grid = MakeChamberGrid(8, 8, 5);
    st_grid[4][5] = 2;
    adapter.LoadCustomTilemap(SceneMode::Structure, st_grid);
    adapter.SetPlayerPosition(0, {192.0f, 210.0f});
    for (int t = 0; t < 15; ++t) {
        const StepResult res = adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, true)));
        E2E_ASSERT_TRUE(!res.transition_occurred);
    }
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Structure);
}

E2E_TEST(F17_T2_04_MapWithoutPortalsNeverTransitions, E2ETier::Tier2_Boundary, "F17") {
    E2E_REQUIRE_LIVE(adapter);

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeBorderedGrid(8, 8));
    adapter.SetPlayerPosition(0, {60.0f, 60.0f});
    for (int t = 0; t < 60; ++t) {
        const StepResult res = adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, true, false, true)));
        E2E_ASSERT_TRUE(!res.transition_occurred);
    }
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Overworld);

    adapter.ResetSimulation(SceneMode::Structure, 1);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeChamberGrid(8, 8, 5));
    adapter.SetPlayerPosition(0, {60.0f, 210.0f});
    for (int t = 0; t < 60; ++t) {
        const StepResult res = adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, true)));
        E2E_ASSERT_TRUE(!res.transition_occurred);
    }
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Structure);
}

E2E_TEST(F17_T2_05_ZeroActivePlayersSafeNoTransition, E2ETier::Tier2_Boundary, "F17") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 0);
    const StepResult r0 = adapter.StepFixed(15, MakeNeutralInputs(0));
    E2E_ASSERT_TRUE(!r0.transition_occurred);
    E2E_ASSERT_EQ(adapter.ActivePlayerCount(), 0);

    adapter.RequestModeTransition(SceneMode::Structure);
    adapter.StepFixed(15, MakeNeutralInputs(0));
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Structure);
    const CameraSnapshot c1 = adapter.GetCamera();
    E2E_ASSERT_TRUE(std::isfinite(c1.target.x));
    E2E_ASSERT_TRUE(std::isfinite(c1.target.y));

    adapter.RequestModeTransition(SceneMode::Overworld);
    adapter.StepFixed(15, MakeNeutralInputs(0));
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Overworld);
    const CameraSnapshot c2 = adapter.GetCamera();
    E2E_ASSERT_TRUE(std::isfinite(c2.target.x));
    E2E_ASSERT_TRUE(std::isfinite(c2.target.y));
}

E2E_TEST(F17_T2_06_RapidRepeatedTransitionsAndMidAirExitPortal, E2ETier::Tier2_Boundary, "F17") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);

    for (int i = 0; i < 3; ++i) {
        adapter.RequestModeTransition(SceneMode::Structure);
        adapter.StepFixed(1, MakeNeutralInputs(1));
        E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Structure);

        adapter.RequestModeTransition(SceneMode::Overworld);
        adapter.StepFixed(1, MakeNeutralInputs(1));
        E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Overworld);
    }

    adapter.ResetSimulation(SceneMode::Structure, 1);
    auto st_grid = MakeChamberGrid(8, 8, 5);
    st_grid[3][3] = 3;
    adapter.LoadCustomTilemap(SceneMode::Structure, st_grid);
    adapter.SetPlayerPosition(0, {144.0f, 210.0f});
    adapter.StepFixed(1, MakeNeutralInputs(1));
    adapter.StepFixed(1, MakeInputsP0(MakeDigitalInput(false, false, false, false, true, true)));

    bool transitioned = false;
    for (int t = 0; t < 15; ++t) {
        const StepResult res = adapter.StepFixed(1, MakeNeutralInputs(1));
        if (res.transition_occurred) {
            transitioned = true;
            break;
        }
    }
    E2E_ASSERT_TRUE(transitioned);
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Overworld);
    const PlayerSnapshot p = adapter.GetPlayer(0);
    E2E_ASSERT_TRUE(std::isfinite(p.pos.x));
    E2E_ASSERT_TRUE(std::isfinite(p.pos.y));
}
