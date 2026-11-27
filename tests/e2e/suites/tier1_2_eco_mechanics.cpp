#include "e2e_adapter.hpp"
#include "e2e_harness.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {

using eco::e2e::CalibrationUiSnapshot;
using eco::e2e::ColorRgba;
using eco::e2e::ContractConstants;
using eco::e2e::E2ETier;
using eco::e2e::EcoSnapshot;
using eco::e2e::FilteredInput;
using eco::e2e::IE2EAdapter;
using eco::e2e::InputFrame;
using eco::e2e::MachineKind;
using eco::e2e::MachineSnapshot;
using eco::e2e::MachineStatus;
using eco::e2e::NpcSnapshot;
using eco::e2e::OverlayPhase;
using eco::e2e::OverlayRenderData;
using eco::e2e::PlayerSnapshot;
using eco::e2e::RawGamepadState;
using eco::e2e::RawKeyboardState;
using eco::e2e::ScannerSnapshot;
using eco::e2e::SceneMode;
using eco::e2e::TileKind;
using eco::e2e::Vec2;

constexpr const char* kMachinePrompt = "Premi [E] per calibrare il monumento";
constexpr const char* kBypassLabel = "Usa Fornace a Pece di Emergenza (Bypass)";

std::array<InputFrame, 4> MakeIdleInputs(const IE2EAdapter& adapter) {
    std::array<InputFrame, 4> inputs{};
    for (int i = 0; i < ContractConstants::kMaxPlayers; ++i) {
        inputs[static_cast<std::size_t>(i)].active = adapter.GetPlayer(i).active;
    }
    return inputs;
}

std::vector<std::vector<int>> MakeUniformGrid(int rows, int cols, int tile_id = 0) {
    return std::vector<std::vector<int>>(
        static_cast<std::size_t>(rows),
        std::vector<int>(static_cast<std::size_t>(cols), tile_id));
}

float FarOffSolarAngle(float target_deg) {
    return (target_deg >= 90.0f) ? (target_deg - 45.0f) : (target_deg + 45.0f);
}

float NearPassSolarAngle(float target_deg) {
    return (target_deg >= 90.0f) ? (target_deg - 35.0f) : (target_deg + 35.0f);
}

float NearFailSolarAngle(float target_deg) {
    return (target_deg >= 90.0f) ? (target_deg - 37.5f) : (target_deg + 37.5f);
}

bool OpenMachineCalibrationUi(IE2EAdapter& adapter, MachineKind kind, int player_id = 0) {
    const MachineSnapshot m = adapter.GetMachine(kind);
    adapter.SetPlayerPosition(player_id, m.pos);
    auto inputs = MakeIdleInputs(adapter);
    inputs[static_cast<std::size_t>(player_id)].active = true;
    inputs[static_cast<std::size_t>(player_id)].interact_pressed = true;
    adapter.StepFixed(1, inputs);
    const CalibrationUiSnapshot ui = adapter.GetCalibrationUi();
    return ui.is_open && ui.target_machine == kind;
}

void CalibrateMachineClean(IE2EAdapter& adapter, MachineKind kind, int player_id = 0) {
    const bool opened = OpenMachineCalibrationUi(adapter, kind, player_id);
    E2E_ASSERT_TRUE(opened);
    const MachineSnapshot m = adapter.GetMachine(kind);
    adapter.SetCalibrationParameter(m.target_param);
    const bool ok = adapter.ConfirmCalibrationClean();
    E2E_ASSERT_TRUE(ok);
}

void BypassMachineDirty(IE2EAdapter& adapter, MachineKind kind, int player_id = 0) {
    const bool opened = OpenMachineCalibrationUi(adapter, kind, player_id);
    E2E_ASSERT_TRUE(opened);
    adapter.TriggerCombustionBypass();
}

void TransitionToScene(IE2EAdapter& adapter, SceneMode target) {
    if (adapter.CurrentMode() == target) {
        return;
    }
    adapter.RequestModeTransition(target);
    const auto idle = MakeIdleInputs(adapter);
    adapter.StepFixed(1, idle);
    E2E_ASSERT_EQ(adapter.CurrentMode(), target);
}

int ColorLuminanceSum(const ColorRgba& c) {
    return static_cast<int>(c.r) + static_cast<int>(c.g) + static_cast<int>(c.b);
}

int ColorChromaSpread(const ColorRgba& c) {
    const int hi = std::max({static_cast<int>(c.r), static_cast<int>(c.g), static_cast<int>(c.b)});
    const int lo = std::min({static_cast<int>(c.r), static_cast<int>(c.g), static_cast<int>(c.b)});
    return hi - lo;
}

} // namespace

// ============================================================================
// F08: Global EcoSystem State & Tint Math (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F08_T1_01_InitialEcoSystemDefaults, E2ETier::Tier1_Feature, "F08") {
    E2E_ASSERT_NEAR(ContractConstants::kInitialPollution, 35.0f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kInitialCleanEnergy, 0.0f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kInitialDirtyEnergy, 0.0f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kMinEcoValue, 0.0f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kMaxEcoValue, 100.0f, 1e-6f);
    E2E_REQUIRE_LIVE(adapter);

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    const EcoSnapshot eco = adapter.GetEcoSystem();
    E2E_ASSERT_NEAR(eco.pollution_level, ContractConstants::kInitialPollution, 1e-4f);
    E2E_ASSERT_NEAR(eco.clean_energy, ContractConstants::kInitialCleanEnergy, 1e-4f);
    E2E_ASSERT_NEAR(eco.dirty_energy, ContractConstants::kInitialDirtyEnergy, 1e-4f);
    E2E_ASSERT_EQ(eco.ui_phase, OverlayPhase::Phase1_Minimal);
    E2E_ASSERT_TRUE(!eco.architect_recruited);
}

E2E_TEST(F08_T1_02_CleanActivationMutatesCleanAndReducesPollution, E2ETier::Tier1_Feature, "F08") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    const EcoSnapshot before = adapter.GetEcoSystem();

    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    const EcoSnapshot after = adapter.GetEcoSystem();

    E2E_ASSERT_TRUE(after.clean_energy > before.clean_energy);
    E2E_ASSERT_TRUE(after.pollution_level < before.pollution_level);
    E2E_ASSERT_NEAR(after.dirty_energy, 0.0f, 1e-4f);
}

E2E_TEST(F08_T1_03_DirtyBypassMutatesDirtyAndIncreasesPollution, E2ETier::Tier1_Feature, "F08") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    const EcoSnapshot before = adapter.GetEcoSystem();

    BypassMachineDirty(adapter, MachineKind::SolarArray, 0);
    const EcoSnapshot after = adapter.GetEcoSystem();

    E2E_ASSERT_TRUE(after.dirty_energy > before.dirty_energy);
    E2E_ASSERT_TRUE(after.pollution_level > before.pollution_level);
    E2E_ASSERT_NEAR(after.clean_energy, 0.0f, 1e-4f);
}

E2E_TEST(F08_T1_04_TintColorMonotonicDarkening, E2ETier::Tier1_Feature, "F08") {
    E2E_REQUIRE_LIVE(adapter);
    const ColorRgba white{255, 255, 255, 255};
    const ColorRgba c0 = adapter.ComputeTintColor(white, 0.0f);
    const ColorRgba c35 = adapter.ComputeTintColor(white, 35.0f);
    const ColorRgba c70 = adapter.ComputeTintColor(white, 70.0f);
    const ColorRgba c100 = adapter.ComputeTintColor(white, 100.0f);

    E2E_ASSERT_EQ(c0.a, static_cast<std::uint8_t>(255));
    E2E_ASSERT_EQ(c35.a, static_cast<std::uint8_t>(255));
    E2E_ASSERT_EQ(c70.a, static_cast<std::uint8_t>(255));
    E2E_ASSERT_EQ(c100.a, static_cast<std::uint8_t>(255));

    E2E_ASSERT_GE(c0.r, c35.r);
    E2E_ASSERT_GE(c35.r, c70.r);
    E2E_ASSERT_GE(c70.r, c100.r);
    E2E_ASSERT_GE(c0.g, c35.g);
    E2E_ASSERT_GE(c35.g, c70.g);
    E2E_ASSERT_GE(c70.g, c100.g);
    E2E_ASSERT_GE(c0.b, c35.b);
    E2E_ASSERT_GE(c35.b, c70.b);
    E2E_ASSERT_GE(c70.b, c100.b);
    E2E_ASSERT_TRUE(ColorLuminanceSum(c0) > ColorLuminanceSum(c100));
}

E2E_TEST(F08_T1_05_SmogAlphaMonotonicScaling, E2ETier::Tier1_Feature, "F08") {
    E2E_REQUIRE_LIVE(adapter);
    const float a0 = adapter.ComputeSmogAlpha(0.0f);
    const float a35 = adapter.ComputeSmogAlpha(35.0f);
    const float a70 = adapter.ComputeSmogAlpha(70.0f);
    const float a100 = adapter.ComputeSmogAlpha(100.0f);

    E2E_ASSERT_NEAR(a0, 0.0f, 1e-5f);
    E2E_ASSERT_TRUE(a35 > 0.0f);
    E2E_ASSERT_TRUE(a35 < a70);
    E2E_ASSERT_TRUE(a70 < a100);
    E2E_ASSERT_LE(a100, 1.0f);
}

E2E_TEST(F08_T1_06_PersistenceAcrossSceneTransitions, E2ETier::Tier1_Feature, "F08") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    const EcoSnapshot in_overworld = adapter.GetEcoSystem();

    TransitionToScene(adapter, SceneMode::Structure);
    const EcoSnapshot in_structure = adapter.GetEcoSystem();
    E2E_ASSERT_NEAR(in_structure.pollution_level, in_overworld.pollution_level, 1e-5f);
    E2E_ASSERT_NEAR(in_structure.clean_energy, in_overworld.clean_energy, 1e-5f);
    E2E_ASSERT_NEAR(in_structure.dirty_energy, in_overworld.dirty_energy, 1e-5f);
    E2E_ASSERT_EQ(in_structure.ui_phase, in_overworld.ui_phase);
    E2E_ASSERT_EQ(in_structure.architect_recruited, in_overworld.architect_recruited);

    TransitionToScene(adapter, SceneMode::Overworld);
    const EcoSnapshot back_overworld = adapter.GetEcoSystem();
    E2E_ASSERT_NEAR(back_overworld.pollution_level, in_overworld.pollution_level, 1e-5f);
    E2E_ASSERT_NEAR(back_overworld.clean_energy, in_overworld.clean_energy, 1e-5f);
    E2E_ASSERT_NEAR(back_overworld.dirty_energy, in_overworld.dirty_energy, 1e-5f);
    E2E_ASSERT_EQ(back_overworld.ui_phase, in_overworld.ui_phase);
}

E2E_TEST(F08_T2_01_PollutionLowerBoundClampZero, E2ETier::Tier2_Boundary, "F08") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.SetPollutionLevel(-50.0f);
    E2E_ASSERT_NEAR(adapter.GetEcoSystem().pollution_level, 0.0f, 1e-5f);

    adapter.SetPollutionLevel(1.0f);
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    const float p = adapter.GetEcoSystem().pollution_level;
    E2E_ASSERT_GE(p, ContractConstants::kMinEcoValue);
    E2E_ASSERT_LE(p, 1.0f);
}

E2E_TEST(F08_T2_02_PollutionUpperBoundClampHundred, E2ETier::Tier2_Boundary, "F08") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.SetPollutionLevel(250.0f);
    E2E_ASSERT_NEAR(adapter.GetEcoSystem().pollution_level, 100.0f, 1e-5f);

    adapter.SetPollutionLevel(99.0f);
    BypassMachineDirty(adapter, MachineKind::SolarArray, 0);
    const float p = adapter.GetEcoSystem().pollution_level;
    E2E_ASSERT_GE(p, 99.0f);
    E2E_ASSERT_LE(p, ContractConstants::kMaxEcoValue);
}

E2E_TEST(F08_T2_03_EnergyUpperBoundsNeverExceedHundred, E2ETier::Tier2_Boundary, "F08") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    TransitionToScene(adapter, SceneMode::Structure);
    CalibrateMachineClean(adapter, MachineKind::HydroTurbine, 0);

    const EcoSnapshot clean_eco = adapter.GetEcoSystem();
    E2E_ASSERT_GE(clean_eco.clean_energy, ContractConstants::kMinEcoValue);
    E2E_ASSERT_LE(clean_eco.clean_energy, ContractConstants::kMaxEcoValue);

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    BypassMachineDirty(adapter, MachineKind::SolarArray, 0);
    TransitionToScene(adapter, SceneMode::Structure);
    BypassMachineDirty(adapter, MachineKind::HydroTurbine, 0);

    const EcoSnapshot dirty_eco = adapter.GetEcoSystem();
    E2E_ASSERT_GE(dirty_eco.dirty_energy, ContractConstants::kMinEcoValue);
    E2E_ASSERT_LE(dirty_eco.dirty_energy, ContractConstants::kMaxEcoValue);
}

E2E_TEST(F08_T2_04_TintColorBlackAndZeroAlphaEdge, E2ETier::Tier2_Boundary, "F08") {
    E2E_REQUIRE_LIVE(adapter);
    const ColorRgba black_tinted = adapter.ComputeTintColor(ColorRgba{0, 0, 0, 255}, 100.0f);
    E2E_ASSERT_EQ(black_tinted.r, static_cast<std::uint8_t>(0));
    E2E_ASSERT_EQ(black_tinted.g, static_cast<std::uint8_t>(0));
    E2E_ASSERT_EQ(black_tinted.b, static_cast<std::uint8_t>(0));
    E2E_ASSERT_EQ(black_tinted.a, static_cast<std::uint8_t>(255));

    const ColorRgba zero_alpha = adapter.ComputeTintColor(ColorRgba{200, 100, 50, 0}, 50.0f);
    E2E_ASSERT_EQ(zero_alpha.a, static_cast<std::uint8_t>(0));
}

E2E_TEST(F08_T2_05_NanAndInfinitySanitization, E2ETier::Tier2_Boundary, "F08") {
    E2E_REQUIRE_LIVE(adapter);
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float pinf = std::numeric_limits<float>::infinity();
    const float ninf = -std::numeric_limits<float>::infinity();

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.SetPollutionLevel(qnan);
    const float p_nan = adapter.GetEcoSystem().pollution_level;
    E2E_ASSERT_TRUE(std::isfinite(p_nan));
    E2E_ASSERT_GE(p_nan, ContractConstants::kMinEcoValue);
    E2E_ASSERT_LE(p_nan, ContractConstants::kMaxEcoValue);

    const float s_nan = adapter.ComputeSmogAlpha(qnan);
    const float s_pinf = adapter.ComputeSmogAlpha(pinf);
    const float s_ninf = adapter.ComputeSmogAlpha(ninf);
    E2E_ASSERT_TRUE(std::isfinite(s_nan));
    E2E_ASSERT_TRUE(std::isfinite(s_pinf));
    E2E_ASSERT_TRUE(std::isfinite(s_ninf));
    E2E_ASSERT_GE(s_nan, 0.0f);
    E2E_ASSERT_LE(s_nan, 1.0f);
    E2E_ASSERT_GE(s_pinf, 0.0f);
    E2E_ASSERT_LE(s_pinf, 1.0f);
    E2E_ASSERT_GE(s_ninf, 0.0f);
    E2E_ASSERT_LE(s_ninf, 1.0f);
}

E2E_TEST(F08_T2_06_PureRedDesaturationSpreadShrinks, E2ETier::Tier2_Boundary, "F08") {
    E2E_REQUIRE_LIVE(adapter);
    const ColorRgba pure_red{255, 0, 0, 255};
    const ColorRgba at_zero = adapter.ComputeTintColor(pure_red, 0.0f);
    const ColorRgba at_max = adapter.ComputeTintColor(pure_red, 100.0f);
    E2E_ASSERT_TRUE(ColorChromaSpread(at_max) < ColorChromaSpread(at_zero));
}

// ============================================================================
// F18: Machine Entities & Trigger Radius (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F18_T1_01_InitialMachinesBrokenAndPromptHiddenFar, E2ETier::Tier1_Feature, "F18") {
    E2E_ASSERT_NEAR(ContractConstants::kPlayerWidth, 30.0f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kPlayerHeight, 30.0f, 1e-6f);
    E2E_REQUIRE_LIVE(adapter);

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeUniformGrid(50, 50, 0));
    adapter.SetMachinePosition(MachineKind::SolarArray, Vec2{144.0f, 144.0f});

    const MachineSnapshot solar = adapter.GetMachine(MachineKind::SolarArray);
    const MachineSnapshot hydro = adapter.GetMachine(MachineKind::HydroTurbine);
    E2E_ASSERT_EQ(solar.status, MachineStatus::Broken);
    E2E_ASSERT_EQ(hydro.status, MachineStatus::Broken);
    E2E_ASSERT_TRUE(solar.trigger_radius > 0.0f);
    E2E_ASSERT_TRUE(hydro.trigger_radius > 0.0f);

    adapter.SetPlayerPosition(0, Vec2{144.0f + solar.trigger_radius + 300.0f, 144.0f});
    adapter.StepFixed(1, MakeIdleInputs(adapter));
    E2E_ASSERT_TRUE(!adapter.GetMachine(MachineKind::SolarArray).prompt_visible);
}

E2E_TEST(F18_T1_02_SolarPromptVisibleInsideRadius, E2ETier::Tier1_Feature, "F18") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    const MachineSnapshot m = adapter.GetMachine(MachineKind::SolarArray);
    adapter.SetPlayerPosition(0, m.pos);
    adapter.StepFixed(1, MakeIdleInputs(adapter));

    const MachineSnapshot updated = adapter.GetMachine(MachineKind::SolarArray);
    E2E_ASSERT_TRUE(updated.prompt_visible);
    E2E_ASSERT_EQ(updated.prompt_text, std::string(kMachinePrompt));
}

E2E_TEST(F18_T1_03_PromptClearsWhenLeavingRadius, E2ETier::Tier1_Feature, "F18") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeUniformGrid(50, 50, 0));
    adapter.SetMachinePosition(MachineKind::SolarArray, Vec2{300.0f, 300.0f});
    const MachineSnapshot m = adapter.GetMachine(MachineKind::SolarArray);

    adapter.SetPlayerPosition(0, m.pos);
    adapter.StepFixed(1, MakeIdleInputs(adapter));
    E2E_ASSERT_TRUE(adapter.GetMachine(MachineKind::SolarArray).prompt_visible);

    adapter.SetPlayerPosition(0, Vec2{300.0f + m.trigger_radius + 100.0f, 300.0f});
    adapter.StepFixed(1, MakeIdleInputs(adapter));
    E2E_ASSERT_TRUE(!adapter.GetMachine(MachineKind::SolarArray).prompt_visible);
}

E2E_TEST(F18_T1_04_HydroPromptVisibleInStructure, E2ETier::Tier1_Feature, "F18") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    const MachineSnapshot hydro = adapter.GetMachine(MachineKind::HydroTurbine);
    adapter.SetPlayerPosition(0, hydro.pos);
    adapter.StepFixed(1, MakeIdleInputs(adapter));

    const MachineSnapshot updated = adapter.GetMachine(MachineKind::HydroTurbine);
    E2E_ASSERT_TRUE(updated.prompt_visible);
    E2E_ASSERT_EQ(updated.prompt_text, std::string(kMachinePrompt));
}

E2E_TEST(F18_T1_05_FourPlayersAnyTriggersSinglePrompt, E2ETier::Tier1_Feature, "F18") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 4);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeUniformGrid(50, 50, 0));
    adapter.SetMachinePosition(MachineKind::SolarArray, Vec2{300.0f, 300.0f});
    const MachineSnapshot m = adapter.GetMachine(MachineKind::SolarArray);
    const Vec2 far_pos{300.0f + m.trigger_radius + 400.0f, 300.0f};

    adapter.SetPlayerPosition(0, far_pos);
    adapter.SetPlayerPosition(1, far_pos);
    adapter.SetPlayerPosition(2, m.pos);
    adapter.SetPlayerPosition(3, far_pos);
    adapter.StepFixed(1, MakeIdleInputs(adapter));

    MachineSnapshot snap = adapter.GetMachine(MachineKind::SolarArray);
    E2E_ASSERT_TRUE(snap.prompt_visible);
    E2E_ASSERT_EQ(snap.prompt_text, std::string(kMachinePrompt));

    adapter.SetPlayerPosition(0, m.pos);
    adapter.SetPlayerPosition(1, m.pos);
    adapter.SetPlayerPosition(3, m.pos);
    adapter.StepFixed(1, MakeIdleInputs(adapter));

    snap = adapter.GetMachine(MachineKind::SolarArray);
    E2E_ASSERT_TRUE(snap.prompt_visible);
    E2E_ASSERT_EQ(snap.prompt_text, std::string(kMachinePrompt));
}

E2E_TEST(F18_T1_06_InteractOpensPopupOnlyInsideRadius, E2ETier::Tier1_Feature, "F18") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeUniformGrid(50, 50, 0));
    adapter.SetMachinePosition(MachineKind::SolarArray, Vec2{300.0f, 300.0f});
    const MachineSnapshot m = adapter.GetMachine(MachineKind::SolarArray);

    adapter.SetPlayerPosition(0, Vec2{300.0f + m.trigger_radius + 200.0f, 300.0f});
    auto inputs = MakeIdleInputs(adapter);
    inputs[0].interact_pressed = true;
    adapter.StepFixed(1, inputs);
    E2E_ASSERT_TRUE(!adapter.GetCalibrationUi().is_open);

    adapter.SetPlayerPosition(0, m.pos);
    adapter.StepFixed(1, inputs);
    const CalibrationUiSnapshot ui = adapter.GetCalibrationUi();
    E2E_ASSERT_TRUE(ui.is_open);
    E2E_ASSERT_EQ(ui.target_machine, MachineKind::SolarArray);
}

E2E_TEST(F18_T2_01_EuclideanDiagonalOutsideCircle, E2ETier::Tier2_Boundary, "F18") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeUniformGrid(50, 50, 0));
    adapter.SetMachinePosition(MachineKind::SolarArray, Vec2{500.0f, 500.0f});
    const float r = adapter.GetMachine(MachineKind::SolarArray).trigger_radius;
    const float d = 0.82f * r;

    // 0.82*R < R on both X and Y, yet sqrt(2) * 0.82*R > R for both top-left and center
    adapter.SetPlayerPosition(0, Vec2{500.0f + d, 500.0f + d});
    adapter.StepFixed(1, MakeIdleInputs(adapter));
    E2E_ASSERT_TRUE(!adapter.GetMachine(MachineKind::SolarArray).prompt_visible);
}

E2E_TEST(F18_T2_02_RadiusBoundaryInsideVsOutside, E2ETier::Tier2_Boundary, "F18") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeUniformGrid(50, 50, 0));
    adapter.SetMachinePosition(MachineKind::SolarArray, Vec2{500.0f, 500.0f});
    const float r = adapter.GetMachine(MachineKind::SolarArray).trigger_radius;

    bool saw_inside = false;
    bool saw_outside = false;
    bool monotonic = true;
    float transition_offset = -1.0f;

    const int max_step = static_cast<int>(std::ceil(r)) + 30;
    for (int px = 0; px <= max_step; ++px) {
        const float offset = static_cast<float>(px);
        adapter.SetPlayerPosition(0, Vec2{500.0f + offset, 500.0f});
        adapter.StepFixed(1, MakeIdleInputs(adapter));
        const bool vis = adapter.GetMachine(MachineKind::SolarArray).prompt_visible;
        if (vis) {
            if (saw_outside) {
                monotonic = false;
            }
            saw_inside = true;
        } else {
            if (!saw_outside) {
                transition_offset = offset;
            }
            saw_outside = true;
        }
    }

    E2E_ASSERT_TRUE(saw_inside);
    E2E_ASSERT_TRUE(saw_outside);
    E2E_ASSERT_TRUE(monotonic);
    const float min_expected_offset = std::max(0.0f, r - 25.0f);
    E2E_ASSERT_GE(transition_offset, min_expected_offset);
    E2E_ASSERT_LE(transition_offset, r + 25.0f);
}

E2E_TEST(F18_T2_03_InactivePlayerInsideRadiusIgnored, E2ETier::Tier2_Boundary, "F18") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 2);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeUniformGrid(50, 50, 0));
    adapter.SetMachinePosition(MachineKind::SolarArray, Vec2{300.0f, 300.0f});
    const MachineSnapshot m = adapter.GetMachine(MachineKind::SolarArray);

    adapter.SetPlayerPosition(0, Vec2{300.0f + m.trigger_radius + 400.0f, 300.0f});
    adapter.SetPlayerPosition(1, m.pos);
    adapter.SetPlayerActive(1, false);

    auto inputs = MakeIdleInputs(adapter);
    inputs[1].active = true;
    inputs[1].interact_pressed = true;
    adapter.StepFixed(1, inputs);

    E2E_ASSERT_TRUE(!adapter.GetMachine(MachineKind::SolarArray).prompt_visible);
    E2E_ASSERT_TRUE(!adapter.GetCalibrationUi().is_open);
}

E2E_TEST(F18_T2_04_SceneIsolationBlocksCrossMachineTrigger, E2ETier::Tier2_Boundary, "F18") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeUniformGrid(60, 60, 0));
    const MachineSnapshot hydro = adapter.GetMachine(MachineKind::HydroTurbine);
    adapter.SetMachinePosition(MachineKind::SolarArray, Vec2{2200.0f, 2200.0f});
    adapter.SetPlayerPosition(0, hydro.pos);

    auto inputs = MakeIdleInputs(adapter);
    inputs[0].interact_pressed = true;
    adapter.StepFixed(1, inputs);

    E2E_ASSERT_TRUE(!adapter.GetMachine(MachineKind::HydroTurbine).prompt_visible);
    E2E_ASSERT_TRUE(!adapter.GetCalibrationUi().is_open);
}

E2E_TEST(F18_T2_05_NegativeAndLargeCoordinatesSafe, E2ETier::Tier2_Boundary, "F18") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.SetMachinePosition(MachineKind::SolarArray, Vec2{-1000.0f, -1000.0f});
    adapter.SetPlayerPosition(0, Vec2{-1000.0f, -1000.0f});
    adapter.StepFixed(1, MakeIdleInputs(adapter));
    E2E_ASSERT_TRUE(adapter.GetMachine(MachineKind::SolarArray).prompt_visible);

    adapter.SetPlayerPosition(0, Vec2{5000.0f, 5000.0f});
    adapter.StepFixed(1, MakeIdleInputs(adapter));
    E2E_ASSERT_TRUE(!adapter.GetMachine(MachineKind::SolarArray).prompt_visible);
}

E2E_TEST(F18_T2_06_AlreadyActivatedMachineReinteractSafe, E2ETier::Tier2_Boundary, "F18") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    const float clean_before = adapter.GetEcoSystem().clean_energy;

    const MachineSnapshot m = adapter.GetMachine(MachineKind::SolarArray);
    adapter.SetPlayerPosition(0, m.pos);
    auto inputs = MakeIdleInputs(adapter);
    inputs[0].interact_pressed = true;
    adapter.StepFixed(1, inputs);

    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::SolarArray).status, MachineStatus::CleanActive);
    E2E_ASSERT_GE(adapter.GetEcoSystem().clean_energy, clean_before);
    E2E_ASSERT_LE(adapter.GetEcoSystem().clean_energy, ContractConstants::kMaxEcoValue);
}

// ============================================================================
// F19: Raylib Calibration Pop-up UI (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F19_T1_01_OpenSetsValidModalState, E2ETier::Tier1_Feature, "F19") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::SolarArray, 0));

    const CalibrationUiSnapshot ui = adapter.GetCalibrationUi();
    E2E_ASSERT_TRUE(ui.is_open);
    E2E_ASSERT_EQ(ui.target_machine, MachineKind::SolarArray);
    E2E_ASSERT_TRUE(std::isfinite(ui.slider_value));
    E2E_ASSERT_TRUE(std::isfinite(ui.preview_efficiency));
    E2E_ASSERT_GE(ui.preview_efficiency, 0.0f);
    E2E_ASSERT_LE(ui.preview_efficiency, 1.0f);
    E2E_ASSERT_TRUE(!ui.bypass_label.empty());
}

E2E_TEST(F19_T1_02_SliderDeltaUpdatesLivePreview, E2ETier::Tier1_Feature, "F19") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::SolarArray, 0));

    const float target = adapter.GetMachine(MachineKind::SolarArray).target_param;
    const float far_angle = FarOffSolarAngle(target);

    adapter.SetCalibrationParameter(far_angle);
    const CalibrationUiSnapshot ui_far = adapter.GetCalibrationUi();
    E2E_ASSERT_LE(ui_far.preview_efficiency, ContractConstants::kSolarEfficiencyThreshold);
    E2E_ASSERT_TRUE(!ui_far.can_confirm_clean);

    auto inputs = MakeIdleInputs(adapter);
    inputs[0].slider_delta = 5.0f;
    adapter.StepFixed(3, inputs);
    const float after_plus = adapter.GetCalibrationUi().slider_value;
    if (std::fabs(after_plus - ui_far.slider_value) < 1e-4f) {
        inputs[0].slider_delta = -5.0f;
        adapter.StepFixed(3, inputs);
    }
    const float after_step = adapter.GetCalibrationUi().slider_value;
    E2E_ASSERT_TRUE(std::fabs(after_step - ui_far.slider_value) > 1e-4f);

    adapter.SetCalibrationParameter(target);
    const CalibrationUiSnapshot ui_opt = adapter.GetCalibrationUi();
    E2E_ASSERT_TRUE(ui_opt.preview_efficiency > ContractConstants::kSolarEfficiencyThreshold);
    E2E_ASSERT_TRUE(ui_opt.can_confirm_clean);
}

E2E_TEST(F19_T1_03_ConfirmSuccessClosesAndActivates, E2ETier::Tier1_Feature, "F19") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::SolarArray, 0));
    adapter.SetCalibrationParameter(adapter.GetMachine(MachineKind::SolarArray).target_param);
    E2E_ASSERT_TRUE(adapter.ConfirmCalibrationClean());
    E2E_ASSERT_TRUE(!adapter.GetCalibrationUi().is_open);
    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::SolarArray).status, MachineStatus::CleanActive);

    TransitionToScene(adapter, SceneMode::Structure);
    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::HydroTurbine, 0));
    adapter.SetCalibrationParameter(adapter.GetMachine(MachineKind::HydroTurbine).target_param);
    auto inputs = MakeIdleInputs(adapter);
    inputs[0].confirm_pressed = true;
    adapter.StepFixed(1, inputs);
    E2E_ASSERT_TRUE(!adapter.GetCalibrationUi().is_open);
    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::HydroTurbine).status, MachineStatus::CleanActive);
}

E2E_TEST(F19_T1_04_CancelClosesWithoutSideEffects, E2ETier::Tier1_Feature, "F19") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    const EcoSnapshot before = adapter.GetEcoSystem();

    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::SolarArray, 0));
    adapter.SetCalibrationParameter(adapter.GetMachine(MachineKind::SolarArray).target_param);
    adapter.CancelCalibration();

    E2E_ASSERT_TRUE(!adapter.GetCalibrationUi().is_open);
    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::SolarArray).status, MachineStatus::Broken);
    const EcoSnapshot after = adapter.GetEcoSystem();
    E2E_ASSERT_NEAR(after.clean_energy, before.clean_energy, 1e-5f);
    E2E_ASSERT_NEAR(after.dirty_energy, before.dirty_energy, 1e-5f);
    E2E_ASSERT_NEAR(after.pollution_level, before.pollution_level, 1e-5f);

    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::SolarArray, 0));
    auto inputs = MakeIdleInputs(adapter);
    inputs[0].cancel_pressed = true;
    adapter.StepFixed(1, inputs);
    E2E_ASSERT_TRUE(!adapter.GetCalibrationUi().is_open);
    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::SolarArray).status, MachineStatus::Broken);
}

E2E_TEST(F19_T1_05_BypassClosesAndActivatesDirty, E2ETier::Tier1_Feature, "F19") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::SolarArray, 0));
    adapter.SetCalibrationParameter(
        FarOffSolarAngle(adapter.GetMachine(MachineKind::SolarArray).target_param));
    adapter.TriggerCombustionBypass();

    E2E_ASSERT_TRUE(!adapter.GetCalibrationUi().is_open);
    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::SolarArray).status, MachineStatus::BypassedDirty);

    TransitionToScene(adapter, SceneMode::Structure);
    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::HydroTurbine, 0));
    auto inputs = MakeIdleInputs(adapter);
    inputs[0].bypass_pressed = true;
    adapter.StepFixed(1, inputs);
    E2E_ASSERT_TRUE(!adapter.GetCalibrationUi().is_open);
    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::HydroTurbine).status, MachineStatus::BypassedDirty);
}

E2E_TEST(F19_T1_06_FreezesPlayerMotionWhileOpenThenRestores, E2ETier::Tier1_Feature, "F19") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeUniformGrid(30, 30, 0));
    adapter.SetMachinePosition(MachineKind::SolarArray, Vec2{300.0f, 300.0f});

    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::SolarArray, 0));
    const Vec2 frozen_pos = adapter.GetPlayer(0).pos;

    auto move_inputs = MakeIdleInputs(adapter);
    move_inputs[0].use_digital_keys = true;
    move_inputs[0].right = true;
    move_inputs[0].up = true;
    adapter.StepFixed(20, move_inputs);

    const Vec2 while_open_pos = adapter.GetPlayer(0).pos;
    E2E_ASSERT_NEAR(while_open_pos.x, frozen_pos.x, 1e-5f);
    E2E_ASSERT_NEAR(while_open_pos.y, frozen_pos.y, 1e-5f);

    adapter.CancelCalibration();
    E2E_ASSERT_TRUE(!adapter.GetCalibrationUi().is_open);

    auto right_only = MakeIdleInputs(adapter);
    right_only[0].use_digital_keys = true;
    right_only[0].right = true;
    adapter.StepFixed(20, right_only);
    E2E_ASSERT_TRUE(adapter.GetPlayer(0).pos.x > frozen_pos.x + 1.0f);
}

E2E_TEST(F19_T2_01_ConfirmFailsAtOrBelowThreshold, E2ETier::Tier2_Boundary, "F19") {
    E2E_ASSERT_NEAR(ContractConstants::kSolarEfficiencyThreshold, 0.80f, 1e-6f);
    E2E_REQUIRE_LIVE(adapter);

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::SolarArray, 0));
    const float target = adapter.GetMachine(MachineKind::SolarArray).target_param;
    adapter.SetCalibrationParameter(NearFailSolarAngle(target));

    const CalibrationUiSnapshot ui = adapter.GetCalibrationUi();
    E2E_ASSERT_LE(ui.preview_efficiency, ContractConstants::kSolarEfficiencyThreshold);
    E2E_ASSERT_TRUE(!ui.can_confirm_clean);
    E2E_ASSERT_TRUE(!adapter.ConfirmCalibrationClean());
    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::SolarArray).status, MachineStatus::Broken);
    E2E_ASSERT_NEAR(adapter.GetEcoSystem().clean_energy, 0.0f, 1e-5f);
}

E2E_TEST(F19_T2_02_ConfirmSucceedsJustAboveThreshold, E2ETier::Tier2_Boundary, "F19") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::SolarArray, 0));
    const float target = adapter.GetMachine(MachineKind::SolarArray).target_param;
    adapter.SetCalibrationParameter(NearPassSolarAngle(target));

    const CalibrationUiSnapshot ui = adapter.GetCalibrationUi();
    E2E_ASSERT_TRUE(ui.preview_efficiency > ContractConstants::kSolarEfficiencyThreshold);
    E2E_ASSERT_TRUE(ui.can_confirm_clean);
    E2E_ASSERT_TRUE(adapter.ConfirmCalibrationClean());
    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::SolarArray).status, MachineStatus::CleanActive);
}

E2E_TEST(F19_T2_03_CancelDoesNotCommitSliderOnReopen, E2ETier::Tier2_Boundary, "F19") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::SolarArray, 0));
    adapter.SetCalibrationParameter(adapter.GetMachine(MachineKind::SolarArray).target_param);
    adapter.CancelCalibration();

    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::SolarArray).status, MachineStatus::Broken);
    E2E_ASSERT_NEAR(adapter.GetEcoSystem().clean_energy, 0.0f, 1e-5f);
    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::SolarArray, 0));
    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::SolarArray).status, MachineStatus::Broken);
}

E2E_TEST(F19_T2_04_FreezesMultiPlayerAndJumpInStructure, E2ETier::Tier2_Boundary, "F19") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 2);
    adapter.LoadCustomTilemap(SceneMode::Structure, MakeUniformGrid(20, 20, 0));
    adapter.SetMachinePosition(MachineKind::HydroTurbine, Vec2{300.0f, 300.0f});
    adapter.SetPlayerPosition(1, Vec2{400.0f, 300.0f});

    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::HydroTurbine, 0));
    const Vec2 p0_before = adapter.GetPlayer(0).pos;
    const Vec2 p1_before = adapter.GetPlayer(1).pos;

    auto inputs = MakeIdleInputs(adapter);
    inputs[0].use_digital_keys = true;
    inputs[0].right = true;
    inputs[0].jump_pressed = true;
    inputs[0].jump_held = true;
    inputs[1].use_digital_keys = true;
    inputs[1].left = true;
    adapter.StepFixed(15, inputs);

    const Vec2 p0_after = adapter.GetPlayer(0).pos;
    const Vec2 p1_after = adapter.GetPlayer(1).pos;
    E2E_ASSERT_NEAR(p0_after.x, p0_before.x, 1e-5f);
    E2E_ASSERT_NEAR(p0_after.y, p0_before.y, 1e-5f);
    E2E_ASSERT_NEAR(p1_after.x, p1_before.x, 1e-5f);
    E2E_ASSERT_NEAR(p1_after.y, p1_before.y, 1e-5f);
}

E2E_TEST(F19_T2_05_ModalActionsWhenClosedAreNoOp, E2ETier::Tier2_Boundary, "F19") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    E2E_ASSERT_TRUE(!adapter.GetCalibrationUi().is_open);

    E2E_ASSERT_TRUE(!adapter.ConfirmCalibrationClean());
    adapter.TriggerCombustionBypass();
    adapter.CancelCalibration();

    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::SolarArray).status, MachineStatus::Broken);
    const EcoSnapshot eco = adapter.GetEcoSystem();
    E2E_ASSERT_NEAR(eco.clean_energy, 0.0f, 1e-5f);
    E2E_ASSERT_NEAR(eco.dirty_energy, 0.0f, 1e-5f);
    E2E_ASSERT_NEAR(eco.pollution_level, ContractConstants::kInitialPollution, 1e-5f);
}

E2E_TEST(F19_T2_06_ExtremeAndNanSliderParametersSafe, E2ETier::Tier2_Boundary, "F19") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::SolarArray, 0));

    const float qnan = std::numeric_limits<float>::quiet_NaN();
    for (float val : {-10000.0f, 10000.0f, qnan}) {
        adapter.SetCalibrationParameter(val);
        const CalibrationUiSnapshot ui = adapter.GetCalibrationUi();
        E2E_ASSERT_TRUE(std::isfinite(ui.slider_value));
        E2E_ASSERT_TRUE(std::isfinite(ui.preview_efficiency));
        E2E_ASSERT_GE(ui.preview_efficiency, 0.0f);
        E2E_ASSERT_LE(ui.preview_efficiency, 1.0f);
    }
}

// ============================================================================
// F20: Cosine Solar Efficiency & Pressure Math (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F20_T1_01_SolarExactAlignmentIsOne, E2ETier::Tier1_Feature, "F20") {
    E2E_ASSERT_NEAR(ContractConstants::kSolarEfficiencyThreshold, 0.80f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kWaterDensity, 1000.0f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kGravityWater, 9.81f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kHydroEfficiencyThreshold, 0.80f, 1e-6f);
    E2E_REQUIRE_LIVE(adapter);

    const float eff = adapter.ComputeSolarEfficiency(42.0f, 42.0f);
    E2E_ASSERT_NEAR(eff, 1.0f, 1e-5f);
    E2E_ASSERT_TRUE(eff > ContractConstants::kSolarEfficiencyThreshold);
}

E2E_TEST(F20_T1_02_SolarTwentyDegreeOffsetMatchesCosine, E2ETier::Tier1_Feature, "F20") {
    E2E_REQUIRE_LIVE(adapter);
    constexpr float kPi = 3.14159265358979323846f;
    const float expected = std::cos(20.0f * kPi / 180.0f);
    const float eff = adapter.ComputeSolarEfficiency(65.0f, 45.0f);
    E2E_ASSERT_NEAR(eff, expected, 1e-4f);
    E2E_ASSERT_TRUE(eff > ContractConstants::kSolarEfficiencyThreshold);
}

E2E_TEST(F20_T1_03_SolarFortyFiveDegreeOffsetFailsThreshold, E2ETier::Tier1_Feature, "F20") {
    E2E_REQUIRE_LIVE(adapter);
    constexpr float kPi = 3.14159265358979323846f;
    const float expected = std::cos(45.0f * kPi / 180.0f);
    const float eff = adapter.ComputeSolarEfficiency(90.0f, 45.0f);
    E2E_ASSERT_NEAR(eff, expected, 1e-4f);
    E2E_ASSERT_LE(eff, ContractConstants::kSolarEfficiencyThreshold);
}

E2E_TEST(F20_T1_04_HydroExactTargetHeadIsOne, E2ETier::Tier1_Feature, "F20") {
    E2E_REQUIRE_LIVE(adapter);
    const float eff = adapter.ComputeHydroEfficiency(10.0f, 10.0f, 20000.0f);
    E2E_ASSERT_NEAR(eff, 1.0f, 1e-5f);
    E2E_ASSERT_TRUE(eff > ContractConstants::kHydroEfficiencyThreshold);
}

E2E_TEST(F20_T1_05_HydroSmallHeadDeviationFormula, E2ETier::Tier1_Feature, "F20") {
    E2E_REQUIRE_LIVE(adapter);
    const float dp =
        ContractConstants::kWaterDensity * ContractConstants::kGravityWater * 0.3f;
    const float expected = 1.0f - dp / 20000.0f;
    const float eff = adapter.ComputeHydroEfficiency(10.3f, 10.0f, 20000.0f);
    E2E_ASSERT_NEAR(eff, expected, 1e-4f);
    E2E_ASSERT_TRUE(eff > ContractConstants::kHydroEfficiencyThreshold);
}

E2E_TEST(F20_T1_06_HydroLargeHeadDeviationBelowThreshold, E2ETier::Tier1_Feature, "F20") {
    E2E_REQUIRE_LIVE(adapter);
    const float dp =
        ContractConstants::kWaterDensity * ContractConstants::kGravityWater * 1.0f;
    const float expected = 1.0f - dp / 20000.0f;
    const float eff = adapter.ComputeHydroEfficiency(11.0f, 10.0f, 20000.0f);
    E2E_ASSERT_NEAR(eff, expected, 1e-4f);
    E2E_ASSERT_LE(eff, ContractConstants::kHydroEfficiencyThreshold);
}

E2E_TEST(F20_T2_01_SolarThresholdBoundaryAt36Point87Deg, E2ETier::Tier2_Boundary, "F20") {
    E2E_REQUIRE_LIVE(adapter);
    const float pass_eff = adapter.ComputeSolarEfficiency(36.80f, 0.0f);
    const float fail_eff = adapter.ComputeSolarEfficiency(36.90f, 0.0f);
    E2E_ASSERT_TRUE(pass_eff > ContractConstants::kSolarEfficiencyThreshold);
    E2E_ASSERT_LE(fail_eff, ContractConstants::kSolarEfficiencyThreshold);
}

E2E_TEST(F20_T2_02_SolarNinetyAndObtuseClampZero, E2ETier::Tier2_Boundary, "F20") {
    E2E_REQUIRE_LIVE(adapter);
    for (float deg : {90.0f, 135.0f, 180.0f, 240.0f}) {
        const float eff = adapter.ComputeSolarEfficiency(deg, 0.0f);
        E2E_ASSERT_NEAR(eff, 0.0f, 1e-5f);
    }
}

E2E_TEST(F20_T2_03_SolarSymmetryAnd360Periodicity, E2ETier::Tier2_Boundary, "F20") {
    E2E_REQUIRE_LIVE(adapter);
    const float sun = 50.0f;
    const float left = adapter.ComputeSolarEfficiency(sun - 25.0f, sun);
    const float right = adapter.ComputeSolarEfficiency(sun + 25.0f, sun);
    E2E_ASSERT_NEAR(left, right, 1e-5f);

    const float wrapped = adapter.ComputeSolarEfficiency(sun + 360.0f, sun);
    E2E_ASSERT_NEAR(wrapped, 1.0f, 1e-4f);
}

E2E_TEST(F20_T2_04_HydroExactThresholdBoundary, E2ETier::Tier2_Boundary, "F20") {
    E2E_REQUIRE_LIVE(adapter);
    const float tol_pa = ContractConstants::kWaterDensity * ContractConstants::kGravityWater;
    const float at_boundary = adapter.ComputeHydroEfficiency(5.20f, 5.00f, tol_pa);
    const float just_above = adapter.ComputeHydroEfficiency(5.19f, 5.00f, tol_pa);

    E2E_ASSERT_NEAR(at_boundary, 0.80f, 1e-4f);
    E2E_ASSERT_TRUE(just_above > ContractConstants::kHydroEfficiencyThreshold);
}

E2E_TEST(F20_T2_05_HydroExcessErrorClampsToZero, E2ETier::Tier2_Boundary, "F20") {
    E2E_REQUIRE_LIVE(adapter);
    E2E_ASSERT_NEAR(adapter.ComputeHydroEfficiency(0.0f, 10.0f, 20000.0f), 0.0f, 1e-5f);
    E2E_ASSERT_NEAR(adapter.ComputeHydroEfficiency(100.0f, 10.0f, 20000.0f), 0.0f, 1e-5f);
}

E2E_TEST(F20_T2_06_ZeroToleranceAndNanInputsSafe, E2ETier::Tier2_Boundary, "F20") {
    E2E_REQUIRE_LIVE(adapter);
    const float qnan = std::numeric_limits<float>::quiet_NaN();

    for (float eff : {
             adapter.ComputeHydroEfficiency(10.0f, 10.0f, 0.0f),
             adapter.ComputeHydroEfficiency(10.5f, 10.0f, 0.0f),
             adapter.ComputeHydroEfficiency(10.0f, 10.0f, -500.0f),
             adapter.ComputeHydroEfficiency(qnan, 10.0f, 20000.0f),
             adapter.ComputeHydroEfficiency(10.0f, qnan, 20000.0f),
             adapter.ComputeHydroEfficiency(10.0f, 10.0f, qnan),
             adapter.ComputeSolarEfficiency(qnan, 0.0f),
             adapter.ComputeSolarEfficiency(0.0f, qnan),
         }) {
        E2E_ASSERT_TRUE(std::isfinite(eff));
        E2E_ASSERT_GE(eff, 0.0f);
        E2E_ASSERT_LE(eff, 1.0f);
    }
}

// ============================================================================
// F21: Polluting Combustion Bypass & Purification (6 T1 + 6 T2 = 12 tests)
// ============================================================================

E2E_TEST(F21_T1_01_BypassButtonExactItalianLabel, E2ETier::Tier1_Feature, "F21") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::SolarArray, 0));
    E2E_ASSERT_EQ(adapter.GetCalibrationUi().bypass_label, std::string(kBypassLabel));
}

E2E_TEST(F21_T1_02_SolarBypassWithoutMath, E2ETier::Tier1_Feature, "F21") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::SolarArray, 0));
    adapter.SetCalibrationParameter(
        FarOffSolarAngle(adapter.GetMachine(MachineKind::SolarArray).target_param));
    adapter.TriggerCombustionBypass();

    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::SolarArray).status, MachineStatus::BypassedDirty);
    const EcoSnapshot eco = adapter.GetEcoSystem();
    E2E_ASSERT_TRUE(eco.dirty_energy > 0.0f);
    E2E_ASSERT_TRUE(eco.pollution_level > ContractConstants::kInitialPollution);
    E2E_ASSERT_NEAR(eco.clean_energy, 0.0f, 1e-5f);
}

E2E_TEST(F21_T1_03_HydroBypassInStructure, E2ETier::Tier1_Feature, "F21") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    BypassMachineDirty(adapter, MachineKind::HydroTurbine, 0);

    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::HydroTurbine).status, MachineStatus::BypassedDirty);
    const EcoSnapshot eco = adapter.GetEcoSystem();
    E2E_ASSERT_TRUE(eco.dirty_energy > 0.0f);
    E2E_ASSERT_TRUE(eco.pollution_level > ContractConstants::kInitialPollution);
    E2E_ASSERT_NEAR(eco.clean_energy, 0.0f, 1e-5f);
}

E2E_TEST(F21_T1_04_SolarCleanPurification, E2ETier::Tier1_Feature, "F21") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);

    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::SolarArray).status, MachineStatus::CleanActive);
    const EcoSnapshot eco = adapter.GetEcoSystem();
    E2E_ASSERT_TRUE(eco.clean_energy > 0.0f);
    E2E_ASSERT_TRUE(eco.pollution_level < ContractConstants::kInitialPollution);
    E2E_ASSERT_NEAR(eco.dirty_energy, 0.0f, 1e-5f);
}

E2E_TEST(F21_T1_05_HydroCleanPurification, E2ETier::Tier1_Feature, "F21") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    CalibrateMachineClean(adapter, MachineKind::HydroTurbine, 0);

    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::HydroTurbine).status, MachineStatus::CleanActive);
    const EcoSnapshot eco = adapter.GetEcoSystem();
    E2E_ASSERT_TRUE(eco.clean_energy > 0.0f);
    E2E_ASSERT_TRUE(eco.pollution_level < ContractConstants::kInitialPollution);
    E2E_ASSERT_NEAR(eco.dirty_energy, 0.0f, 1e-5f);
}

E2E_TEST(F21_T1_06_BypassThenCleanRedemptionArc, E2ETier::Tier1_Feature, "F21") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    BypassMachineDirty(adapter, MachineKind::SolarArray, 0);
    const float p_dirty = adapter.GetEcoSystem().pollution_level;
    E2E_ASSERT_TRUE(p_dirty > ContractConstants::kInitialPollution);

    TransitionToScene(adapter, SceneMode::Structure);
    CalibrateMachineClean(adapter, MachineKind::HydroTurbine, 0);

    const EcoSnapshot eco = adapter.GetEcoSystem();
    E2E_ASSERT_TRUE(eco.clean_energy > 0.0f);
    E2E_ASSERT_TRUE(eco.pollution_level < p_dirty);
}

E2E_TEST(F21_T2_01_BypassAtMaxPollutionClamps, E2ETier::Tier2_Boundary, "F21") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.SetPollutionLevel(100.0f);
    BypassMachineDirty(adapter, MachineKind::SolarArray, 0);

    const EcoSnapshot eco = adapter.GetEcoSystem();
    E2E_ASSERT_NEAR(eco.pollution_level, 100.0f, 1e-5f);
    E2E_ASSERT_LE(eco.dirty_energy, ContractConstants::kMaxEcoValue);
}

E2E_TEST(F21_T2_02_CleanAtZeroPollutionClamps, E2ETier::Tier2_Boundary, "F21") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.SetPollutionLevel(0.0f);
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);

    const EcoSnapshot eco = adapter.GetEcoSystem();
    E2E_ASSERT_NEAR(eco.pollution_level, 0.0f, 1e-5f);
    E2E_ASSERT_TRUE(eco.clean_energy > 0.0f);
}

E2E_TEST(F21_T2_03_BypassDoesNotUnlockPhase2OrArchitect, E2ETier::Tier2_Boundary, "F21") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    BypassMachineDirty(adapter, MachineKind::SolarArray, 0);

    E2E_ASSERT_EQ(adapter.GetEcoSystem().ui_phase, OverlayPhase::Phase1_Minimal);
    adapter.SetPlayerPosition(0, adapter.GetArchitectNpc().pos);
    const NpcSnapshot npc = adapter.InteractArchitectNpc(0);
    E2E_ASSERT_TRUE(!npc.recruited);
    E2E_ASSERT_TRUE(!adapter.GetEcoSystem().architect_recruited);
}

E2E_TEST(F21_T2_04_DualBypassCumulativePollutionClamped, E2ETier::Tier2_Boundary, "F21") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    BypassMachineDirty(adapter, MachineKind::SolarArray, 0);
    const EcoSnapshot after_solar = adapter.GetEcoSystem();

    TransitionToScene(adapter, SceneMode::Structure);
    BypassMachineDirty(adapter, MachineKind::HydroTurbine, 0);
    const EcoSnapshot after_both = adapter.GetEcoSystem();

    E2E_ASSERT_TRUE(after_both.dirty_energy > after_solar.dirty_energy);
    E2E_ASSERT_LE(after_both.dirty_energy, ContractConstants::kMaxEcoValue);
    E2E_ASSERT_TRUE(after_both.pollution_level >= after_solar.pollution_level);
    E2E_ASSERT_LE(after_both.pollution_level, ContractConstants::kMaxEcoValue);
    E2E_ASSERT_NEAR(after_both.clean_energy, 0.0f, 1e-5f);
}

E2E_TEST(F21_T2_05_FailedCleanFollowedByBypass, E2ETier::Tier2_Boundary, "F21") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::SolarArray, 0));
    const float target = adapter.GetMachine(MachineKind::SolarArray).target_param;
    adapter.SetCalibrationParameter(FarOffSolarAngle(target));
    E2E_ASSERT_TRUE(!adapter.ConfirmCalibrationClean());

    if (!adapter.GetCalibrationUi().is_open) {
        E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::SolarArray, 0));
    }
    adapter.TriggerCombustionBypass();

    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::SolarArray).status, MachineStatus::BypassedDirty);
    E2E_ASSERT_TRUE(adapter.GetEcoSystem().dirty_energy > 0.0f);
    E2E_ASSERT_NEAR(adapter.GetEcoSystem().clean_energy, 0.0f, 1e-5f);
}

E2E_TEST(F21_T2_06_RepeatBypassDoesNotInfinitelyFarm, E2ETier::Tier2_Boundary, "F21") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    BypassMachineDirty(adapter, MachineKind::SolarArray, 0);
    const EcoSnapshot after_first = adapter.GetEcoSystem();

    const MachineSnapshot m = adapter.GetMachine(MachineKind::SolarArray);
    adapter.SetPlayerPosition(0, m.pos);
    auto inputs = MakeIdleInputs(adapter);
    inputs[0].interact_pressed = true;
    for (int i = 0; i < 5; ++i) {
        adapter.StepFixed(1, inputs);
        if (adapter.GetCalibrationUi().is_open) {
            adapter.TriggerCombustionBypass();
        }
    }

    const EcoSnapshot after_repeat = adapter.GetEcoSystem();
    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::SolarArray).status, MachineStatus::BypassedDirty);
    E2E_ASSERT_NEAR(after_repeat.dirty_energy, after_first.dirty_energy, 1e-4f);
    E2E_ASSERT_NEAR(after_repeat.pollution_level, after_first.pollution_level, 1e-4f);
    E2E_ASSERT_LE(after_repeat.dirty_energy, ContractConstants::kMaxEcoValue);
}

// ============================================================================
// F22: Pollution Visual Tinting Across Both States (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F22_T1_01_InitialSmogAndTintActive, E2ETier::Tier1_Feature, "F22") {
    E2E_ASSERT_NEAR(ContractConstants::kInitialPollution, 35.0f, 1e-6f);
    E2E_REQUIRE_LIVE(adapter);

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    const EcoSnapshot eco = adapter.GetEcoSystem();
    E2E_ASSERT_TRUE(eco.sample_white_tint.r < 255 ||
                    eco.sample_white_tint.g < 255 ||
                    eco.sample_white_tint.b < 255);
    E2E_ASSERT_TRUE(eco.smog_alpha > 0.0f);
    E2E_ASSERT_LE(eco.smog_alpha, 1.0f);
}

E2E_TEST(F22_T1_02_CleanActivationBrightensAndClearsSmog, E2ETier::Tier1_Feature, "F22") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    const EcoSnapshot before = adapter.GetEcoSystem();

    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    const EcoSnapshot after = adapter.GetEcoSystem();

    E2E_ASSERT_TRUE(ColorLuminanceSum(after.sample_white_tint) >
                    ColorLuminanceSum(before.sample_white_tint));
    E2E_ASSERT_TRUE(after.smog_alpha < before.smog_alpha);
}

E2E_TEST(F22_T1_03_BypassDarkensAndThickensSmog, E2ETier::Tier1_Feature, "F22") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    const EcoSnapshot before = adapter.GetEcoSystem();

    BypassMachineDirty(adapter, MachineKind::SolarArray, 0);
    const EcoSnapshot after = adapter.GetEcoSystem();

    E2E_ASSERT_TRUE(ColorLuminanceSum(after.sample_white_tint) <
                    ColorLuminanceSum(before.sample_white_tint));
    E2E_ASSERT_TRUE(after.smog_alpha > before.smog_alpha);
}

E2E_TEST(F22_T1_04_OverworldPollutionPersistsIntoStructureTint, E2ETier::Tier1_Feature, "F22") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    BypassMachineDirty(adapter, MachineKind::SolarArray, 0);
    const EcoSnapshot in_overworld = adapter.GetEcoSystem();

    TransitionToScene(adapter, SceneMode::Structure);
    const EcoSnapshot in_structure = adapter.GetEcoSystem();

    E2E_ASSERT_EQ(in_structure.sample_white_tint.r, in_overworld.sample_white_tint.r);
    E2E_ASSERT_EQ(in_structure.sample_white_tint.g, in_overworld.sample_white_tint.g);
    E2E_ASSERT_EQ(in_structure.sample_white_tint.b, in_overworld.sample_white_tint.b);
    E2E_ASSERT_EQ(in_structure.sample_white_tint.a, in_overworld.sample_white_tint.a);
    E2E_ASSERT_NEAR(in_structure.smog_alpha, in_overworld.smog_alpha, 1e-5f);
}

E2E_TEST(F22_T1_05_StructurePurificationPersistsIntoOverworldTint, E2ETier::Tier1_Feature, "F22") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    CalibrateMachineClean(adapter, MachineKind::HydroTurbine, 0);
    const EcoSnapshot in_structure = adapter.GetEcoSystem();

    TransitionToScene(adapter, SceneMode::Overworld);
    const EcoSnapshot in_overworld = adapter.GetEcoSystem();

    E2E_ASSERT_EQ(in_overworld.sample_white_tint.r, in_structure.sample_white_tint.r);
    E2E_ASSERT_EQ(in_overworld.sample_white_tint.g, in_structure.sample_white_tint.g);
    E2E_ASSERT_EQ(in_overworld.sample_white_tint.b, in_structure.sample_white_tint.b);
    E2E_ASSERT_NEAR(in_overworld.smog_alpha, in_structure.smog_alpha, 1e-5f);
}

E2E_TEST(F22_T1_06_ZeroPollutionRestoresFullWhiteAndZeroSmog, E2ETier::Tier1_Feature, "F22") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.SetPollutionLevel(0.0f);
    const ColorRgba c = adapter.ComputeTintColor(ColorRgba{255, 255, 255, 255}, 0.0f);
    E2E_ASSERT_EQ(c.r, static_cast<std::uint8_t>(255));
    E2E_ASSERT_EQ(c.g, static_cast<std::uint8_t>(255));
    E2E_ASSERT_EQ(c.b, static_cast<std::uint8_t>(255));
    E2E_ASSERT_EQ(c.a, static_cast<std::uint8_t>(255));
    E2E_ASSERT_NEAR(adapter.ComputeSmogAlpha(0.0f), 0.0f, 1e-5f);
}

E2E_TEST(F22_T2_01_MaxPollutionAt100ValidBounds, E2ETier::Tier2_Boundary, "F22") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    const EcoSnapshot at_init = adapter.GetEcoSystem();
    adapter.SetPollutionLevel(100.0f);
    const EcoSnapshot at_max = adapter.GetEcoSystem();

    E2E_ASSERT_TRUE(ColorLuminanceSum(at_max.sample_white_tint) <
                    ColorLuminanceSum(at_init.sample_white_tint));
    E2E_ASSERT_TRUE(at_max.smog_alpha > at_init.smog_alpha);
    E2E_ASSERT_LE(at_max.smog_alpha, 1.0f);
}

E2E_TEST(F22_T2_02_AlphaChannelPreservedAcrossLevels, E2ETier::Tier2_Boundary, "F22") {
    E2E_REQUIRE_LIVE(adapter);
    for (std::uint8_t alpha : {static_cast<std::uint8_t>(0),
                               static_cast<std::uint8_t>(64),
                               static_cast<std::uint8_t>(128),
                               static_cast<std::uint8_t>(255)}) {
        for (float p : {0.0f, 35.0f, 100.0f}) {
            const ColorRgba out = adapter.ComputeTintColor(ColorRgba{120, 180, 240, alpha}, p);
            E2E_ASSERT_EQ(out.a, alpha);
        }
    }
}

E2E_TEST(F22_T2_03_OutOfRangePollutionClampedInVisualHelpers, E2ETier::Tier2_Boundary, "F22") {
    E2E_REQUIRE_LIVE(adapter);
    const ColorRgba white{255, 255, 255, 255};
    const ColorRgba c_neg = adapter.ComputeTintColor(white, -25.0f);
    const ColorRgba c_zero = adapter.ComputeTintColor(white, 0.0f);
    const ColorRgba c_over = adapter.ComputeTintColor(white, 150.0f);
    const ColorRgba c_max = adapter.ComputeTintColor(white, 100.0f);

    E2E_ASSERT_EQ(c_neg.r, c_zero.r);
    E2E_ASSERT_EQ(c_neg.g, c_zero.g);
    E2E_ASSERT_EQ(c_neg.b, c_zero.b);
    E2E_ASSERT_EQ(c_over.r, c_max.r);
    E2E_ASSERT_EQ(c_over.g, c_max.g);
    E2E_ASSERT_EQ(c_over.b, c_max.b);

    E2E_ASSERT_NEAR(adapter.ComputeSmogAlpha(-25.0f), adapter.ComputeSmogAlpha(0.0f), 1e-5f);
    E2E_ASSERT_NEAR(adapter.ComputeSmogAlpha(150.0f), adapter.ComputeSmogAlpha(100.0f), 1e-5f);
}

E2E_TEST(F22_T2_04_NanPollutionSafeInTintAndSmog, E2ETier::Tier2_Boundary, "F22") {
    E2E_REQUIRE_LIVE(adapter);
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const ColorRgba tint = adapter.ComputeTintColor(ColorRgba{255, 255, 255, 200}, qnan);
    E2E_ASSERT_EQ(tint.a, static_cast<std::uint8_t>(200));

    const float smog = adapter.ComputeSmogAlpha(qnan);
    E2E_ASSERT_TRUE(std::isfinite(smog));
    E2E_ASSERT_GE(smog, 0.0f);
    E2E_ASSERT_LE(smog, 1.0f);
}

E2E_TEST(F22_T2_05_ChromaDesaturationUnderMaxPollution, E2ETier::Tier2_Boundary, "F22") {
    E2E_REQUIRE_LIVE(adapter);
    for (const ColorRgba& primary : {ColorRgba{255, 0, 0, 255},
                                     ColorRgba{0, 255, 0, 255},
                                     ColorRgba{0, 0, 255, 255}}) {
        const ColorRgba clean = adapter.ComputeTintColor(primary, 0.0f);
        const ColorRgba polluted = adapter.ComputeTintColor(primary, 100.0f);
        E2E_ASSERT_TRUE(ColorChromaSpread(polluted) < ColorChromaSpread(clean));
    }
}

E2E_TEST(F22_T2_06_TenStepMonotonicity, E2ETier::Tier2_Boundary, "F22") {
    E2E_REQUIRE_LIVE(adapter);
    const ColorRgba white{255, 255, 255, 255};
    int prev_lum = ColorLuminanceSum(adapter.ComputeTintColor(white, 0.0f));
    float prev_smog = adapter.ComputeSmogAlpha(0.0f);

    for (int step = 10; step <= 100; step += 10) {
        const float p = static_cast<float>(step);
        const int lum = ColorLuminanceSum(adapter.ComputeTintColor(white, p));
        const float smog = adapter.ComputeSmogAlpha(p);
        E2E_ASSERT_LE(lum, prev_lum);
        E2E_ASSERT_GE(smog, prev_smog);
        prev_lum = lum;
        prev_smog = smog;
    }
}

// ============================================================================
// F23: Empirical Measurement Tool (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F23_T1_01_ScanSolarArrayInOverworld, E2ETier::Tier1_Feature, "F23") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    const MachineSnapshot solar = adapter.GetMachine(MachineKind::SolarArray);
    adapter.SetPlayerPosition(0, solar.pos);

    const ScannerSnapshot scan = adapter.TriggerEmpiricalScan(0);
    E2E_ASSERT_TRUE(scan.active);
    E2E_ASSERT_TRUE(scan.has_target);
    E2E_ASSERT_EQ(scan.target_kind, MachineKind::SolarArray);
    E2E_ASSERT_NEAR(scan.measured_target_value, solar.target_param, 1e-3f);
    E2E_ASSERT_GE(scan.measured_distance, 0.0f);
    E2E_ASSERT_NEAR(scan.measured_efficiency, solar.live_efficiency, 1e-3f);
}

E2E_TEST(F23_T1_02_ScanHydroTurbineInStructure, E2ETier::Tier1_Feature, "F23") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Structure, 1);
    const MachineSnapshot hydro = adapter.GetMachine(MachineKind::HydroTurbine);
    adapter.SetPlayerPosition(0, hydro.pos);

    const ScannerSnapshot scan = adapter.TriggerEmpiricalScan(0);
    E2E_ASSERT_TRUE(scan.active);
    E2E_ASSERT_TRUE(scan.has_target);
    E2E_ASSERT_EQ(scan.target_kind, MachineKind::HydroTurbine);
    E2E_ASSERT_NEAR(scan.measured_target_value, hydro.target_param, 1e-3f);
    E2E_ASSERT_GE(scan.measured_distance, 0.0f);
    E2E_ASSERT_NEAR(scan.measured_efficiency, hydro.live_efficiency, 1e-3f);
}

E2E_TEST(F23_T1_03_ScanViaInputFrameAndGamepadChannel, E2ETier::Tier1_Feature, "F23") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.SetPlayerPosition(0, adapter.GetMachine(MachineKind::SolarArray).pos);

    auto inputs = MakeIdleInputs(adapter);
    inputs[0].scan_pressed = true;
    adapter.StepFixed(1, inputs);
    E2E_ASSERT_TRUE(!adapter.GetCalibrationUi().is_open);

    RawKeyboardState kb{};
    std::array<RawGamepadState, 4> pads{};
    pads[0].available = true;
    pads[0].scan_pressed = true;
    const FilteredInput fi = adapter.PollPlayerInput(0, kb, pads);
    E2E_ASSERT_TRUE(fi.scan_pressed);
}

E2E_TEST(F23_T1_04_ScannedTargetCalibratesToUnityEfficiency, E2ETier::Tier1_Feature, "F23") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.SetPlayerPosition(0, adapter.GetMachine(MachineKind::SolarArray).pos);
    const ScannerSnapshot solar_scan = adapter.TriggerEmpiricalScan(0);
    E2E_ASSERT_TRUE(solar_scan.has_target);

    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::SolarArray, 0));
    adapter.SetCalibrationParameter(solar_scan.measured_target_value);
    E2E_ASSERT_TRUE(adapter.ConfirmCalibrationClean());
    const MachineSnapshot solar_after = adapter.GetMachine(MachineKind::SolarArray);
    E2E_ASSERT_EQ(solar_after.status, MachineStatus::CleanActive);
    E2E_ASSERT_NEAR(solar_after.live_efficiency, 1.0f, 1e-3f);

    TransitionToScene(adapter, SceneMode::Structure);
    adapter.SetPlayerPosition(0, adapter.GetMachine(MachineKind::HydroTurbine).pos);
    const ScannerSnapshot hydro_scan = adapter.TriggerEmpiricalScan(0);
    E2E_ASSERT_TRUE(hydro_scan.has_target);

    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::HydroTurbine, 0));
    adapter.SetCalibrationParameter(hydro_scan.measured_target_value);
    E2E_ASSERT_TRUE(adapter.ConfirmCalibrationClean());
    const MachineSnapshot hydro_after = adapter.GetMachine(MachineKind::HydroTurbine);
    E2E_ASSERT_EQ(hydro_after.status, MachineStatus::CleanActive);
    E2E_ASSERT_NEAR(hydro_after.live_efficiency, 1.0f, 1e-3f);
}

E2E_TEST(F23_T1_05_ScanWorksInPhase1MinimalWithoutUnlockingPhase3, E2ETier::Tier1_Feature, "F23") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    E2E_ASSERT_EQ(adapter.GetEcoSystem().ui_phase, OverlayPhase::Phase1_Minimal);

    adapter.SetPlayerPosition(0, adapter.GetMachine(MachineKind::SolarArray).pos);
    const ScannerSnapshot scan = adapter.TriggerEmpiricalScan(0);
    E2E_ASSERT_TRUE(scan.has_target);
    E2E_ASSERT_EQ(adapter.GetEcoSystem().ui_phase, OverlayPhase::Phase1_Minimal);
    E2E_ASSERT_TRUE(!adapter.GetOverlayData().show_velocity_vectors);
}

E2E_TEST(F23_T1_06_ScanIsReadOnlyOnEcoSystemAndTint, E2ETier::Tier1_Feature, "F23") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.SetPlayerPosition(0, adapter.GetMachine(MachineKind::SolarArray).pos);

    const EcoSnapshot before_eco = adapter.GetEcoSystem();
    const MachineSnapshot before_machine = adapter.GetMachine(MachineKind::SolarArray);

    for (int i = 0; i < 5; ++i) {
        const ScannerSnapshot scan = adapter.TriggerEmpiricalScan(0);
        E2E_ASSERT_TRUE(scan.has_target);
    }

    const EcoSnapshot after_eco = adapter.GetEcoSystem();
    const MachineSnapshot after_machine = adapter.GetMachine(MachineKind::SolarArray);
    E2E_ASSERT_NEAR(after_eco.pollution_level, before_eco.pollution_level, 1e-5f);
    E2E_ASSERT_NEAR(after_eco.clean_energy, before_eco.clean_energy, 1e-5f);
    E2E_ASSERT_NEAR(after_eco.dirty_energy, before_eco.dirty_energy, 1e-5f);
    E2E_ASSERT_EQ(after_eco.ui_phase, before_eco.ui_phase);
    E2E_ASSERT_EQ(after_eco.architect_recruited, before_eco.architect_recruited);
    E2E_ASSERT_EQ(after_eco.sample_white_tint.r, before_eco.sample_white_tint.r);
    E2E_ASSERT_NEAR(after_eco.smog_alpha, before_eco.smog_alpha, 1e-5f);
    E2E_ASSERT_EQ(after_machine.status, before_machine.status);
}

E2E_TEST(F23_T2_01_ScanOutOfRangeReturnsNoTargetSafely, E2ETier::Tier2_Boundary, "F23") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeUniformGrid(60, 60, 0));
    adapter.SetMachinePosition(MachineKind::SolarArray, Vec2{144.0f, 144.0f});
    adapter.SetPlayerPosition(0, Vec2{2500.0f, 2500.0f});

    const ScannerSnapshot scan = adapter.TriggerEmpiricalScan(0);
    E2E_ASSERT_TRUE(!scan.has_target);
    E2E_ASSERT_TRUE(std::isfinite(scan.measured_target_value));
    E2E_ASSERT_TRUE(std::isfinite(scan.measured_distance));
    E2E_ASSERT_TRUE(std::isfinite(scan.measured_efficiency));
}

E2E_TEST(F23_T2_02_ScanInvalidPlayerIdsSafe, E2ETier::Tier2_Boundary, "F23") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.SetPlayerPosition(0, adapter.GetMachine(MachineKind::SolarArray).pos);

    for (int bad_id : {-1, 4, 100}) {
        const ScannerSnapshot scan = adapter.TriggerEmpiricalScan(bad_id);
        E2E_ASSERT_TRUE(!scan.has_target);
    }
}

E2E_TEST(F23_T2_03_ScanInactivePlayerSlotIgnored, E2ETier::Tier2_Boundary, "F23") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    const MachineSnapshot solar = adapter.GetMachine(MachineKind::SolarArray);
    adapter.SetPlayerPosition(1, solar.pos);
    E2E_ASSERT_TRUE(!adapter.GetPlayer(1).active);

    const ScannerSnapshot scan = adapter.TriggerEmpiricalScan(1);
    E2E_ASSERT_TRUE(!scan.has_target);
}

E2E_TEST(F23_T2_04_MultiplayerSelectiveProximityScan, E2ETier::Tier2_Boundary, "F23") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 2);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeUniformGrid(60, 60, 0));
    adapter.SetMachinePosition(MachineKind::SolarArray, Vec2{144.0f, 144.0f});
    adapter.SetPlayerPosition(0, Vec2{2500.0f, 2500.0f});
    adapter.SetPlayerPosition(1, Vec2{144.0f + 30.0f, 144.0f + 40.0f});

    const ScannerSnapshot s0 = adapter.TriggerEmpiricalScan(0);
    const ScannerSnapshot s1 = adapter.TriggerEmpiricalScan(1);
    E2E_ASSERT_TRUE(!s0.has_target);
    E2E_ASSERT_TRUE(s1.has_target);
    E2E_ASSERT_EQ(s1.target_kind, MachineKind::SolarArray);
    E2E_ASSERT_GE(s1.measured_distance, 20.0f);
    E2E_ASSERT_LE(s1.measured_distance, 90.0f);
}

E2E_TEST(F23_T2_05_ScanAcrossMachineStatesBrokenCleanBypassed, E2ETier::Tier2_Boundary, "F23") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    const Vec2 solar_pos = adapter.GetMachine(MachineKind::SolarArray).pos;
    adapter.SetPlayerPosition(0, solar_pos);

    const ScannerSnapshot scan_broken = adapter.TriggerEmpiricalScan(0);
    E2E_ASSERT_TRUE(scan_broken.has_target);

    BypassMachineDirty(adapter, MachineKind::SolarArray, 0);
    const ScannerSnapshot scan_dirty = adapter.TriggerEmpiricalScan(0);
    E2E_ASSERT_TRUE(scan_dirty.has_target);
    E2E_ASSERT_EQ(adapter.GetMachine(MachineKind::SolarArray).status, MachineStatus::BypassedDirty);

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    adapter.SetPlayerPosition(0, adapter.GetMachine(MachineKind::SolarArray).pos);
    const ScannerSnapshot scan_clean = adapter.TriggerEmpiricalScan(0);
    E2E_ASSERT_TRUE(scan_clean.has_target);
    E2E_ASSERT_TRUE(scan_clean.measured_efficiency > ContractConstants::kSolarEfficiencyThreshold);
}

E2E_TEST(F23_T2_06_ScanZeroDistanceAtMachineCenter, E2ETier::Tier2_Boundary, "F23") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    const Vec2 mpos = adapter.GetMachine(MachineKind::SolarArray).pos;

    adapter.SetPlayerPosition(0, mpos);
    const ScannerSnapshot s_topleft = adapter.TriggerEmpiricalScan(0);
    adapter.SetPlayerPosition(0, Vec2{mpos.x - 15.0f, mpos.y - 15.0f});
    const ScannerSnapshot s_center = adapter.TriggerEmpiricalScan(0);

    E2E_ASSERT_TRUE(s_topleft.has_target);
    E2E_ASSERT_TRUE(s_center.has_target);
    E2E_ASSERT_TRUE(std::isfinite(s_topleft.measured_distance));
    E2E_ASSERT_TRUE(std::isfinite(s_center.measured_distance));
    E2E_ASSERT_GE(s_topleft.measured_distance, 0.0f);
    E2E_ASSERT_GE(s_center.measured_distance, 0.0f);
    const float min_dist = std::min(s_topleft.measured_distance, s_center.measured_distance);
    E2E_ASSERT_LE(min_dist, 1e-2f);
}

// ============================================================================
// F24: 3-Phase Progressive UI & Engine Overlays (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F24_T1_01_InitialPhase1MinimalAndAllFlagsFalse, E2ETier::Tier1_Feature, "F24") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    const EcoSnapshot eco = adapter.GetEcoSystem();
    const OverlayRenderData ov = adapter.GetOverlayData();

    E2E_ASSERT_EQ(eco.ui_phase, OverlayPhase::Phase1_Minimal);
    E2E_ASSERT_EQ(ov.phase, OverlayPhase::Phase1_Minimal);
    E2E_ASSERT_TRUE(!ov.show_raw_numbers);
    E2E_ASSERT_TRUE(!ov.show_velocity_vectors);
    E2E_ASSERT_TRUE(!ov.show_aabb_hitboxes);
    E2E_ASSERT_TRUE(!ov.show_irradiance_rays);
    E2E_ASSERT_TRUE(!ov.show_trigger_radii);
}

E2E_TEST(F24_T1_02_CleanEnergyUnlocksPhase2TelemetryFlags, E2ETier::Tier1_Feature, "F24") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);

    E2E_ASSERT_TRUE(adapter.GetEcoSystem().clean_energy > 0.0f);
    E2E_ASSERT_TRUE(adapter.RequestOverlayPhase(OverlayPhase::Phase2_Telemetry));
    const OverlayRenderData ov = adapter.GetOverlayData();
    E2E_ASSERT_EQ(ov.phase, OverlayPhase::Phase2_Telemetry);
    E2E_ASSERT_TRUE(ov.show_raw_numbers);
    E2E_ASSERT_TRUE(!ov.show_velocity_vectors);
    E2E_ASSERT_TRUE(!ov.show_aabb_hitboxes);
    E2E_ASSERT_TRUE(!ov.show_irradiance_rays);
    E2E_ASSERT_TRUE(!ov.show_trigger_radii);
}

E2E_TEST(F24_T1_03_ArchitectRecruitmentUnlocksPhase3EngineFlags, E2ETier::Tier1_Feature, "F24") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    adapter.SetPlayerPosition(0, adapter.GetArchitectNpc().pos);
    const NpcSnapshot npc = adapter.InteractArchitectNpc(0);
    E2E_ASSERT_TRUE(npc.recruited);

    E2E_ASSERT_TRUE(adapter.GetEcoSystem().architect_recruited);
    E2E_ASSERT_TRUE(adapter.RequestOverlayPhase(OverlayPhase::Phase3_EngineOverlay));
    const OverlayRenderData ov = adapter.GetOverlayData();
    E2E_ASSERT_EQ(ov.phase, OverlayPhase::Phase3_EngineOverlay);
    E2E_ASSERT_TRUE(ov.show_raw_numbers);
    E2E_ASSERT_TRUE(ov.show_velocity_vectors);
    E2E_ASSERT_TRUE(ov.show_aabb_hitboxes);
    E2E_ASSERT_TRUE(ov.show_irradiance_rays);
    E2E_ASSERT_TRUE(ov.show_trigger_radii);
}

E2E_TEST(F24_T1_04_RequestOverlayPhaseSwitchesFreelyWhenUnlocked, E2ETier::Tier1_Feature, "F24") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    adapter.SetPlayerPosition(0, adapter.GetArchitectNpc().pos);
    E2E_ASSERT_TRUE(adapter.InteractArchitectNpc(0).recruited);

    E2E_ASSERT_TRUE(adapter.RequestOverlayPhase(OverlayPhase::Phase1_Minimal));
    OverlayRenderData ov = adapter.GetOverlayData();
    E2E_ASSERT_EQ(ov.phase, OverlayPhase::Phase1_Minimal);
    E2E_ASSERT_TRUE(!ov.show_raw_numbers);
    E2E_ASSERT_TRUE(!ov.show_velocity_vectors);

    E2E_ASSERT_TRUE(adapter.RequestOverlayPhase(OverlayPhase::Phase2_Telemetry));
    ov = adapter.GetOverlayData();
    E2E_ASSERT_EQ(ov.phase, OverlayPhase::Phase2_Telemetry);
    E2E_ASSERT_TRUE(ov.show_raw_numbers);
    E2E_ASSERT_TRUE(!ov.show_velocity_vectors);

    E2E_ASSERT_TRUE(adapter.RequestOverlayPhase(OverlayPhase::Phase3_EngineOverlay));
    ov = adapter.GetOverlayData();
    E2E_ASSERT_EQ(ov.phase, OverlayPhase::Phase3_EngineOverlay);
    E2E_ASSERT_TRUE(ov.show_raw_numbers);
    E2E_ASSERT_TRUE(ov.show_velocity_vectors);
    E2E_ASSERT_TRUE(ov.show_aabb_hitboxes);
    E2E_ASSERT_TRUE(ov.show_irradiance_rays);
    E2E_ASSERT_TRUE(ov.show_trigger_radii);
}

E2E_TEST(F24_T1_05_OverlayPhasePersistsAcrossSceneTransitions, E2ETier::Tier1_Feature, "F24") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    E2E_ASSERT_TRUE(adapter.RequestOverlayPhase(OverlayPhase::Phase2_Telemetry));

    TransitionToScene(adapter, SceneMode::Structure);
    E2E_ASSERT_EQ(adapter.GetEcoSystem().ui_phase, OverlayPhase::Phase2_Telemetry);
    E2E_ASSERT_EQ(adapter.GetOverlayData().phase, OverlayPhase::Phase2_Telemetry);
    E2E_ASSERT_TRUE(adapter.GetOverlayData().show_raw_numbers);

    TransitionToScene(adapter, SceneMode::Overworld);
    adapter.SetPlayerPosition(0, adapter.GetArchitectNpc().pos);
    E2E_ASSERT_TRUE(adapter.InteractArchitectNpc(0).recruited);
    E2E_ASSERT_TRUE(adapter.RequestOverlayPhase(OverlayPhase::Phase3_EngineOverlay));

    TransitionToScene(adapter, SceneMode::Structure);
    const OverlayRenderData ov_struct = adapter.GetOverlayData();
    E2E_ASSERT_EQ(ov_struct.phase, OverlayPhase::Phase3_EngineOverlay);
    E2E_ASSERT_TRUE(ov_struct.show_velocity_vectors);
    E2E_ASSERT_TRUE(ov_struct.show_aabb_hitboxes);
}

E2E_TEST(F24_T1_06_Phase3ActiveUnderMotionAndPollutionTint, E2ETier::Tier1_Feature, "F24") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeUniformGrid(30, 30, 0));
    adapter.SetMachinePosition(MachineKind::SolarArray, Vec2{144.0f, 144.0f});
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    adapter.SetPlayerPosition(0, adapter.GetArchitectNpc().pos);
    E2E_ASSERT_TRUE(adapter.InteractArchitectNpc(0).recruited);
    E2E_ASSERT_TRUE(adapter.RequestOverlayPhase(OverlayPhase::Phase3_EngineOverlay));
    adapter.SetPollutionLevel(60.0f);

    adapter.SetPlayerPosition(0, Vec2{144.0f, 144.0f});
    auto inputs = MakeIdleInputs(adapter);
    inputs[0].use_digital_keys = true;
    inputs[0].right = true;
    inputs[0].down = true;
    adapter.StepFixed(20, inputs);

    E2E_ASSERT_TRUE(adapter.GetPlayer(0).pos.x > 144.0f);
    E2E_ASSERT_TRUE(adapter.GetPlayer(0).pos.y > 144.0f);
    const OverlayRenderData ov = adapter.GetOverlayData();
    E2E_ASSERT_EQ(ov.phase, OverlayPhase::Phase3_EngineOverlay);
    E2E_ASSERT_TRUE(ov.show_velocity_vectors);
    E2E_ASSERT_TRUE(ov.show_aabb_hitboxes);
    E2E_ASSERT_TRUE(ov.show_irradiance_rays);
    E2E_ASSERT_TRUE(ov.show_trigger_radii);
    E2E_ASSERT_NEAR(adapter.GetEcoSystem().pollution_level, 60.0f, 1e-4f);
}

E2E_TEST(F24_T2_01_RejectPhase2WhenCleanEnergyZero, E2ETier::Tier2_Boundary, "F24") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    E2E_ASSERT_TRUE(!adapter.RequestOverlayPhase(OverlayPhase::Phase2_Telemetry));
    E2E_ASSERT_EQ(adapter.GetEcoSystem().ui_phase, OverlayPhase::Phase1_Minimal);
    E2E_ASSERT_TRUE(!adapter.GetOverlayData().show_raw_numbers);
}

E2E_TEST(F24_T2_02_RejectPhase3WhenArchitectLockedEvenInPhase2, E2ETier::Tier2_Boundary, "F24") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    E2E_ASSERT_TRUE(!adapter.RequestOverlayPhase(OverlayPhase::Phase3_EngineOverlay));
    E2E_ASSERT_EQ(adapter.GetEcoSystem().ui_phase, OverlayPhase::Phase1_Minimal);

    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    E2E_ASSERT_TRUE(adapter.RequestOverlayPhase(OverlayPhase::Phase2_Telemetry));
    E2E_ASSERT_TRUE(!adapter.RequestOverlayPhase(OverlayPhase::Phase3_EngineOverlay));
    E2E_ASSERT_EQ(adapter.GetEcoSystem().ui_phase, OverlayPhase::Phase2_Telemetry);

    const OverlayRenderData ov = adapter.GetOverlayData();
    E2E_ASSERT_TRUE(ov.show_raw_numbers);
    E2E_ASSERT_TRUE(!ov.show_velocity_vectors);
    E2E_ASSERT_TRUE(!ov.show_aabb_hitboxes);
    E2E_ASSERT_TRUE(!ov.show_irradiance_rays);
    E2E_ASSERT_TRUE(!ov.show_trigger_radii);
}

E2E_TEST(F24_T2_03_DirtyBypassNeverUnlocksPhase2OrPhase3, E2ETier::Tier2_Boundary, "F24") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    BypassMachineDirty(adapter, MachineKind::SolarArray, 0);
    E2E_ASSERT_TRUE(adapter.GetEcoSystem().dirty_energy > 0.0f);
    E2E_ASSERT_NEAR(adapter.GetEcoSystem().clean_energy, 0.0f, 1e-5f);

    E2E_ASSERT_TRUE(!adapter.RequestOverlayPhase(OverlayPhase::Phase2_Telemetry));
    E2E_ASSERT_TRUE(!adapter.RequestOverlayPhase(OverlayPhase::Phase3_EngineOverlay));
    E2E_ASSERT_EQ(adapter.GetEcoSystem().ui_phase, OverlayPhase::Phase1_Minimal);
    E2E_ASSERT_TRUE(!adapter.GetOverlayData().show_raw_numbers);
    E2E_ASSERT_TRUE(!adapter.GetOverlayData().show_velocity_vectors);
}

E2E_TEST(F24_T2_04_SubthresholdCalibrationDoesNotUnlockPhase2, E2ETier::Tier2_Boundary, "F24") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    E2E_ASSERT_TRUE(OpenMachineCalibrationUi(adapter, MachineKind::SolarArray, 0));
    const float target = adapter.GetMachine(MachineKind::SolarArray).target_param;
    adapter.SetCalibrationParameter(NearFailSolarAngle(target));
    E2E_ASSERT_TRUE(!adapter.ConfirmCalibrationClean());

    E2E_ASSERT_NEAR(adapter.GetEcoSystem().clean_energy, 0.0f, 1e-5f);
    E2E_ASSERT_EQ(adapter.GetEcoSystem().ui_phase, OverlayPhase::Phase1_Minimal);
    E2E_ASSERT_TRUE(!adapter.RequestOverlayPhase(OverlayPhase::Phase2_Telemetry));
}

E2E_TEST(F24_T2_05_OverlayToggleInputRespectsUnlockGates, E2ETier::Tier2_Boundary, "F24") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);

    for (int i = 0; i < 4; ++i) {
        auto toggle = MakeIdleInputs(adapter);
        toggle[0].toggle_overlay_phase = true;
        adapter.StepFixed(1, toggle);
        adapter.StepFixed(1, MakeIdleInputs(adapter));
        E2E_ASSERT_EQ(adapter.GetEcoSystem().ui_phase, OverlayPhase::Phase1_Minimal);
    }

    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    for (int i = 0; i < 4; ++i) {
        auto toggle = MakeIdleInputs(adapter);
        toggle[0].toggle_overlay_phase = true;
        adapter.StepFixed(1, toggle);
        adapter.StepFixed(1, MakeIdleInputs(adapter));
        E2E_ASSERT_TRUE(adapter.GetEcoSystem().ui_phase != OverlayPhase::Phase3_EngineOverlay);
    }
}

E2E_TEST(F24_T2_06_ResetSimulationClearsUnlockedOverlays, E2ETier::Tier2_Boundary, "F24") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    adapter.SetPlayerPosition(0, adapter.GetArchitectNpc().pos);
    E2E_ASSERT_TRUE(adapter.InteractArchitectNpc(0).recruited);
    E2E_ASSERT_EQ(adapter.GetEcoSystem().ui_phase, OverlayPhase::Phase3_EngineOverlay);

    adapter.ResetSimulation(SceneMode::Overworld, 1);
    const EcoSnapshot eco = adapter.GetEcoSystem();
    const OverlayRenderData ov = adapter.GetOverlayData();
    E2E_ASSERT_EQ(eco.ui_phase, OverlayPhase::Phase1_Minimal);
    E2E_ASSERT_TRUE(!eco.architect_recruited);
    E2E_ASSERT_NEAR(eco.clean_energy, 0.0f, 1e-5f);
    E2E_ASSERT_TRUE(!ov.show_raw_numbers);
    E2E_ASSERT_TRUE(!ov.show_velocity_vectors);
    E2E_ASSERT_TRUE(!ov.show_aabb_hitboxes);
    E2E_ASSERT_TRUE(!ov.show_irradiance_rays);
    E2E_ASSERT_TRUE(!ov.show_trigger_radii);
}

// ============================================================================
// F25: Mathematical Architect NPC Recruitment & HiddenBridge (6 T1 + 6 T2 = 12 tests)
// ============================================================================

E2E_TEST(F25_T1_01_InitialArchitectUnrecruitedAndBridgeSolid, E2ETier::Tier1_Feature, "F25") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    const NpcSnapshot npc = adapter.GetArchitectNpc();
    const EcoSnapshot eco = adapter.GetEcoSystem();
    E2E_ASSERT_TRUE(!npc.recruited);
    E2E_ASSERT_TRUE(!npc.hidden_bridge_revealed);
    E2E_ASSERT_TRUE(!eco.architect_recruited);

    auto grid = MakeUniformGrid(12, 12, 0);
    grid[3][5] = static_cast<int>(TileKind::HiddenBridge);
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    E2E_ASSERT_EQ(adapter.GetTileAt(SceneMode::Overworld, 3, 5), TileKind::HiddenBridge);
    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, 3, 5));
}

E2E_TEST(F25_T1_02_RecruitArchitectWithCleanSolarProof, E2ETier::Tier1_Feature, "F25") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);

    adapter.SetPlayerPosition(0, adapter.GetArchitectNpc().pos);
    const NpcSnapshot npc = adapter.InteractArchitectNpc(0);
    E2E_ASSERT_TRUE(npc.recruited);
    E2E_ASSERT_TRUE(npc.hidden_bridge_revealed);
    E2E_ASSERT_TRUE(!npc.last_dialogue.empty());

    const EcoSnapshot eco = adapter.GetEcoSystem();
    E2E_ASSERT_TRUE(eco.architect_recruited);
    E2E_ASSERT_EQ(eco.ui_phase, OverlayPhase::Phase3_EngineOverlay);
}

E2E_TEST(F25_T1_03_RecruitArchitectAfterDirtyBypassPlusCleanHydroRedemption,
         E2ETier::Tier1_Feature,
         "F25") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    BypassMachineDirty(adapter, MachineKind::SolarArray, 0);

    adapter.SetPlayerPosition(0, adapter.GetArchitectNpc().pos);
    const NpcSnapshot rejected = adapter.InteractArchitectNpc(0);
    E2E_ASSERT_TRUE(!rejected.recruited);

    TransitionToScene(adapter, SceneMode::Structure);
    CalibrateMachineClean(adapter, MachineKind::HydroTurbine, 0);

    TransitionToScene(adapter, SceneMode::Overworld);
    adapter.SetPlayerPosition(0, adapter.GetArchitectNpc().pos);
    const NpcSnapshot accepted = adapter.InteractArchitectNpc(0);
    E2E_ASSERT_TRUE(accepted.recruited);
    E2E_ASSERT_TRUE(accepted.hidden_bridge_revealed);
    E2E_ASSERT_TRUE(adapter.GetEcoSystem().architect_recruited);
    E2E_ASSERT_EQ(adapter.GetEcoSystem().ui_phase, OverlayPhase::Phase3_EngineOverlay);
}

E2E_TEST(F25_T1_04_HiddenBridgeBlocksBeforeAndWalkableAfterRecruitment,
         E2ETier::Tier1_Feature,
         "F25") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    const Vec2 npc_pos = adapter.GetArchitectNpc().pos;
    const int npc_row = static_cast<int>(npc_pos.y / ContractConstants::kTileSize);
    const int npc_col = static_cast<int>(npc_pos.x / ContractConstants::kTileSize);

    const int rows = std::max(16, npc_row + 6);
    const int cols = std::max(16, npc_col + 6);
    const int test_row = (npc_row != 3) ? 3 : 8;
    const int test_col = (npc_col >= 5 && npc_col <= 7) ? 10 : 6;

    auto grid = MakeUniformGrid(rows, cols, 0);
    grid[static_cast<std::size_t>(test_row - 1)][static_cast<std::size_t>(test_col)] =
        static_cast<int>(TileKind::Wall);
    grid[static_cast<std::size_t>(test_row)][static_cast<std::size_t>(test_col)] =
        static_cast<int>(TileKind::HiddenBridge);
    grid[static_cast<std::size_t>(test_row + 1)][static_cast<std::size_t>(test_col)] =
        static_cast<int>(TileKind::Wall);
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetMachinePosition(MachineKind::SolarArray, Vec2{48.0f, 48.0f});

    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, test_row, test_col));

    const float start_x = static_cast<float>(test_col - 1) * ContractConstants::kTileSize;
    const float start_y = static_cast<float>(test_row) * ContractConstants::kTileSize + 8.0f;
    adapter.SetPlayerPosition(0, Vec2{start_x, start_y});

    auto walk_right = MakeIdleInputs(adapter);
    walk_right[0].use_digital_keys = true;
    walk_right[0].right = true;
    adapter.StepFixed(45, walk_right);

    const PlayerSnapshot blocked = adapter.GetPlayer(0);
    E2E_ASSERT_LE(blocked.pos.x + blocked.size.x,
                  static_cast<float>(test_col) * ContractConstants::kTileSize + 0.5f);

    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    adapter.SetPlayerPosition(0, npc_pos);
    E2E_ASSERT_TRUE(adapter.InteractArchitectNpc(0).recruited);
    E2E_ASSERT_TRUE(!adapter.IsTileSolid(SceneMode::Overworld, test_row, test_col));

    adapter.SetPlayerPosition(0, Vec2{start_x, start_y});
    adapter.StepFixed(45, walk_right);
    E2E_ASSERT_TRUE(adapter.GetPlayer(0).pos.x >
                    static_cast<float>(test_col) * ContractConstants::kTileSize);
}

E2E_TEST(F25_T1_05_RecruitmentAndBridgeWalkabilityPersistAcrossScenes,
         E2ETier::Tier1_Feature,
         "F25") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    adapter.SetPlayerPosition(0, adapter.GetArchitectNpc().pos);
    E2E_ASSERT_TRUE(adapter.InteractArchitectNpc(0).recruited);

    TransitionToScene(adapter, SceneMode::Structure);
    E2E_ASSERT_TRUE(adapter.GetEcoSystem().architect_recruited);

    TransitionToScene(adapter, SceneMode::Overworld);
    auto grid = MakeUniformGrid(12, 12, 0);
    grid[4][4] = static_cast<int>(TileKind::HiddenBridge);
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);

    const NpcSnapshot npc = adapter.GetArchitectNpc();
    E2E_ASSERT_TRUE(npc.recruited);
    E2E_ASSERT_TRUE(npc.hidden_bridge_revealed);
    E2E_ASSERT_EQ(adapter.GetEcoSystem().ui_phase, OverlayPhase::Phase3_EngineOverlay);
    E2E_ASSERT_TRUE(!adapter.IsTileSolid(SceneMode::Overworld, 4, 4));
}

E2E_TEST(F25_T1_06_CoopPlayerRecruitsGloballyForAllPlayers, E2ETier::Tier1_Feature, "F25") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 4);
    const Vec2 npc_pos = adapter.GetArchitectNpc().pos;
    const int npc_row = static_cast<int>(npc_pos.y / ContractConstants::kTileSize);
    const int npc_col = static_cast<int>(npc_pos.x / ContractConstants::kTileSize);

    const int rows = std::max(16, npc_row + 6);
    const int cols = std::max(16, npc_col + 6);
    const int test_row = (npc_row != 3) ? 3 : 8;
    const int test_col = (npc_col >= 5 && npc_col <= 7) ? 10 : 6;

    auto grid = MakeUniformGrid(rows, cols, 0);
    grid[static_cast<std::size_t>(test_row)][static_cast<std::size_t>(test_col)] =
        static_cast<int>(TileKind::HiddenBridge);
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);
    adapter.SetMachinePosition(MachineKind::SolarArray, Vec2{48.0f, 48.0f});

    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    adapter.SetPlayerPosition(2, npc_pos);
    const NpcSnapshot npc = adapter.InteractArchitectNpc(2);
    E2E_ASSERT_TRUE(npc.recruited);
    E2E_ASSERT_EQ(adapter.GetEcoSystem().ui_phase, OverlayPhase::Phase3_EngineOverlay);

    const float start_x = static_cast<float>(test_col - 1) * ContractConstants::kTileSize;
    const float start_y = static_cast<float>(test_row) * ContractConstants::kTileSize + 8.0f;
    adapter.SetPlayerPosition(3, Vec2{start_x, start_y});

    auto walk_p3 = MakeIdleInputs(adapter);
    walk_p3[3].use_digital_keys = true;
    walk_p3[3].right = true;
    adapter.StepFixed(45, walk_p3);
    E2E_ASSERT_TRUE(adapter.GetPlayer(3).pos.x >
                    static_cast<float>(test_col) * ContractConstants::kTileSize);
}

E2E_TEST(F25_T2_01_RejectRecruitmentAtZeroCleanAndDirtyEnergy, E2ETier::Tier2_Boundary, "F25") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    auto grid = MakeUniformGrid(12, 12, 0);
    grid[3][3] = static_cast<int>(TileKind::HiddenBridge);
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);

    adapter.SetPlayerPosition(0, adapter.GetArchitectNpc().pos);
    const NpcSnapshot npc = adapter.InteractArchitectNpc(0);
    E2E_ASSERT_TRUE(!npc.recruited);
    E2E_ASSERT_TRUE(!npc.hidden_bridge_revealed);
    E2E_ASSERT_TRUE(!npc.last_dialogue.empty());
    E2E_ASSERT_TRUE(!adapter.GetEcoSystem().architect_recruited);
    E2E_ASSERT_EQ(adapter.GetEcoSystem().ui_phase, OverlayPhase::Phase1_Minimal);
    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, 3, 3));
}

E2E_TEST(F25_T2_02_RejectRecruitmentWithDirtyEnergyOnly, E2ETier::Tier2_Boundary, "F25") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    BypassMachineDirty(adapter, MachineKind::SolarArray, 0);
    auto grid = MakeUniformGrid(12, 12, 0);
    grid[3][3] = static_cast<int>(TileKind::HiddenBridge);
    adapter.LoadCustomTilemap(SceneMode::Overworld, grid);

    adapter.SetPlayerPosition(0, adapter.GetArchitectNpc().pos);
    const NpcSnapshot npc = adapter.InteractArchitectNpc(0);
    E2E_ASSERT_TRUE(!npc.recruited);
    E2E_ASSERT_TRUE(!npc.hidden_bridge_revealed);
    E2E_ASSERT_TRUE(!npc.last_dialogue.empty());
    E2E_ASSERT_TRUE(!adapter.GetEcoSystem().architect_recruited);
    E2E_ASSERT_EQ(adapter.GetEcoSystem().ui_phase, OverlayPhase::Phase1_Minimal);
    E2E_ASSERT_TRUE(adapter.IsTileSolid(SceneMode::Overworld, 3, 3));
}

E2E_TEST(F25_T2_03_ReinteractingPostRecruitmentIsIdempotent, E2ETier::Tier2_Boundary, "F25") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    adapter.SetPlayerPosition(0, adapter.GetArchitectNpc().pos);
    E2E_ASSERT_TRUE(adapter.InteractArchitectNpc(0).recruited);
    const EcoSnapshot before = adapter.GetEcoSystem();

    for (int i = 0; i < 3; ++i) {
        const NpcSnapshot npc = adapter.InteractArchitectNpc(0);
        E2E_ASSERT_TRUE(npc.recruited);
        E2E_ASSERT_TRUE(npc.hidden_bridge_revealed);
    }

    const EcoSnapshot after = adapter.GetEcoSystem();
    E2E_ASSERT_NEAR(after.clean_energy, before.clean_energy, 1e-5f);
    E2E_ASSERT_NEAR(after.dirty_energy, before.dirty_energy, 1e-5f);
    E2E_ASSERT_NEAR(after.pollution_level, before.pollution_level, 1e-5f);
    E2E_ASSERT_EQ(after.ui_phase, before.ui_phase);
}

E2E_TEST(F25_T2_04_InvalidAndInactivePlayerIdsRejectedSafely, E2ETier::Tier2_Boundary, "F25") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    adapter.SetPlayerPosition(1, adapter.GetArchitectNpc().pos);
    E2E_ASSERT_TRUE(!adapter.GetPlayer(1).active);

    for (int bad_id : {-1, 4, 1}) {
        const NpcSnapshot npc = adapter.InteractArchitectNpc(bad_id);
        E2E_ASSERT_TRUE(!npc.recruited);
    }
    E2E_ASSERT_TRUE(!adapter.GetArchitectNpc().recruited);
    E2E_ASSERT_TRUE(!adapter.GetEcoSystem().architect_recruited);
}

E2E_TEST(F25_T2_05_OutOfRangeAndStructureSceneInteractSafe, E2ETier::Tier2_Boundary, "F25") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    adapter.LoadCustomTilemap(SceneMode::Overworld, MakeUniformGrid(80, 80, 0));
    adapter.SetMachinePosition(MachineKind::SolarArray, Vec2{96.0f, 96.0f});
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);

    const Vec2 npc_pos = adapter.GetArchitectNpc().pos;
    adapter.SetPlayerPosition(0, Vec2{npc_pos.x + 1500.0f, npc_pos.y + 1500.0f});
    auto inputs = MakeIdleInputs(adapter);
    inputs[0].interact_pressed = true;
    adapter.StepFixed(1, inputs);
    E2E_ASSERT_TRUE(!adapter.GetArchitectNpc().recruited);
    E2E_ASSERT_TRUE(!adapter.GetEcoSystem().architect_recruited);

    TransitionToScene(adapter, SceneMode::Structure);
    adapter.StepFixed(1, inputs);
    const NpcSnapshot in_struct = adapter.InteractArchitectNpc(0);
    E2E_ASSERT_TRUE(!in_struct.recruited);
    E2E_ASSERT_EQ(adapter.CurrentMode(), SceneMode::Structure);
}

E2E_TEST(F25_T2_06_MaxClampedPollutionWithCleanEnergyStillRecruits,
         E2ETier::Tier2_Boundary,
         "F25") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetSimulation(SceneMode::Overworld, 1);
    CalibrateMachineClean(adapter, MachineKind::SolarArray, 0);
    adapter.SetPollutionLevel(100.0f);

    adapter.SetPlayerPosition(0, adapter.GetArchitectNpc().pos);
    const NpcSnapshot npc = adapter.InteractArchitectNpc(0);
    E2E_ASSERT_TRUE(npc.recruited);
    E2E_ASSERT_TRUE(npc.hidden_bridge_revealed);

    const EcoSnapshot eco = adapter.GetEcoSystem();
    E2E_ASSERT_EQ(eco.ui_phase, OverlayPhase::Phase3_EngineOverlay);
    E2E_ASSERT_NEAR(eco.pollution_level, 100.0f, 1e-5f);
}
