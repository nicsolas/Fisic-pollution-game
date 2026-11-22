#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "app/game_state.h"
#include "app/game_state_manager.h"
#include "core/collectible.h"
#include "core/ecosystem.h"
#include "core/input_types.h"
#include "core/machine.h"
#include "core/math_types.h"
#include "core/npc.h"
#include "core/physics2d.h"
#include "core/player.h"
#include "core/save_data.h"
#include "core/simulation.h"
#include "core/story.h"
#include "core/tilemap.h"

namespace {

int g_failures = 0;

void ReportFailure(const char* expr, const char* file, int line, const std::string& detail = "") {
    ++g_failures;
    std::cerr << "[FAIL] " << file << ":" << line << " (" << expr << ")";
    if (!detail.empty()) {
        std::cerr << " -> " << detail;
    }
    std::cerr << "\n";
}

#define CHECK(expr)                                                   \
    do {                                                              \
        if (!(expr)) {                                                \
            ReportFailure(#expr, __FILE__, __LINE__);                 \
        }                                                             \
    } while (false)

#define CHECK_NEAR(a, b, eps)                                         \
    do {                                                              \
        const float _va = static_cast<float>(a);                      \
        const float _vb = static_cast<float>(b);                      \
        const float _ve = static_cast<float>(eps);                    \
        if (!std::isfinite(_va) || !std::isfinite(_vb) ||             \
            std::fabs(_va - _vb) > _ve) {                             \
            ReportFailure(#a " ~= " #b, __FILE__, __LINE__,           \
                          "got " + std::to_string(_va) +              \
                              " vs " + std::to_string(_vb));          \
        }                                                             \
    } while (false)

bool IsPureAsciiPrintable(std::string_view text) noexcept {
    for (unsigned char ch : text) {
        if (ch == '\n' || ch == '\t') {
            continue;
        }
        if (ch < 32 || ch > 126) {
            return false;
        }
    }
    return true;
}

bool ContainsForbiddenLegacyToken(std::string_view text) noexcept {
    std::string upper;
    upper.reserve(text.size());
    for (unsigned char ch : text) {
        upper.push_back(static_cast<char>(std::toupper(ch)));
    }
    constexpr std::array<std::string_view, 11> kForbidden = {
        "CALABRIA",
        "CRATI",
        "UNICAL",
        "PONTE BUCCI",
        "// GAIA",
        "[SYS]",
        "AABB",
        "CICLO 404",
        "CONSORZIO BRACE",
        "BRUNO CENERI",
        "NONNA ROSA"
    };
    for (std::string_view tok : kForbidden) {
        if (upper.find(tok) != std::string::npos) {
            return true;
        }
    }
    return false;
}

void TestMathTypes() {
    CHECK_NEAR(eco::deg2rad(0.0f), 0.0f, 1e-6f);
    CHECK_NEAR(eco::deg2rad(90.0f), eco::kPi * 0.5f, 1e-5f);
    CHECK_NEAR(eco::deg2rad(180.0f), eco::kPi, 1e-5f);
    CHECK_NEAR(eco::deg2rad(-90.0f), -eco::kPi * 0.5f, 1e-5f);
    CHECK_NEAR(eco::rad2deg(eco::deg2rad(137.5f)), 137.5f, 1e-4f);

    const float nan_val = std::numeric_limits<float>::quiet_NaN();
    CHECK_NEAR(eco::Clamp(-5.0f, 0.0f, 10.0f), 0.0f, 1e-6f);
    CHECK_NEAR(eco::Clamp(25.0f, 0.0f, 10.0f), 10.0f, 1e-6f);
    CHECK_NEAR(eco::Clamp(4.25f, 0.0f, 10.0f), 4.25f, 1e-6f);
    CHECK_NEAR(eco::Clamp(nan_val, 2.0f, 10.0f), 2.0f, 1e-6f);
    CHECK_NEAR(eco::Clamp(5.0f, 10.0f, 0.0f), 5.0f, 1e-6f);

    CHECK_NEAR(eco::Lerp(10.0f, 20.0f, 0.0f), 10.0f, 1e-6f);
    CHECK_NEAR(eco::Lerp(10.0f, 20.0f, 0.5f), 15.0f, 1e-6f);
    CHECK_NEAR(eco::Lerp(10.0f, 20.0f, 1.0f), 20.0f, 1e-6f);

    const eco::Vec2 zero_norm = eco::Vec2{0.0f, 0.0f}.Normalized();
    CHECK(zero_norm.x == 0.0f && zero_norm.y == 0.0f);
    const eco::Vec2 tiny_norm = eco::Vec2{1e-9f, -1e-9f}.Normalized();
    CHECK(tiny_norm.x == 0.0f && tiny_norm.y == 0.0f);
    const eco::Vec2 nan_norm = eco::Vec2{nan_val, 3.0f}.Normalized();
    CHECK(nan_norm.x == 0.0f && nan_norm.y == 0.0f);

    const eco::Vec2 v34 = eco::Vec2{3.0f, 4.0f}.Normalized();
    CHECK_NEAR(v34.x, 0.6f, 1e-5f);
    CHECK_NEAR(v34.y, 0.8f, 1e-5f);

    const eco::Rect r1{0.0f, 0.0f, 30.0f, 30.0f};
    const eco::Rect r2{29.0f, 10.0f, 20.0f, 20.0f};
    const eco::Rect edge_right{30.0f, 0.0f, 20.0f, 30.0f};
    const eco::Rect edge_bottom{0.0f, 30.0f, 30.0f, 20.0f};
    CHECK(r1.Overlaps(r2));
    CHECK(!r1.Overlaps(edge_right));
    CHECK(!r1.Overlaps(edge_bottom));

    // Meridia naming helpers: ASCII-safe and zero forbidden tokens
    for (int z = 0; z < static_cast<int>(eco::ZoneId::Count); ++z) {
        const auto zid = static_cast<eco::ZoneId>(z);
        CHECK(IsPureAsciiPrintable(eco::ZoneName(zid)));
        CHECK(IsPureAsciiPrintable(eco::ZoneSubtitle(zid)));
        CHECK(!ContainsForbiddenLegacyToken(eco::ZoneName(zid)));
        CHECK(!ContainsForbiddenLegacyToken(eco::ZoneSubtitle(zid)));
    }
    for (int s = 0; s < static_cast<int>(eco::StructureId::Count); ++s) {
        const auto sid = static_cast<eco::StructureId>(s);
        CHECK(IsPureAsciiPrintable(eco::StructureName(sid)));
        CHECK(IsPureAsciiPrintable(eco::StructureSubtitle(sid)));
        CHECK(!ContainsForbiddenLegacyToken(eco::StructureName(sid)));
    }
    CHECK(std::string_view(eco::UiPhaseName(eco::UiPhase::Phase3_EngineOverlay)) == "Trama Geodetica");
}

void TestFixedTimestepStepper() {
    eco::FixedTimestepStepper stepper;
    CHECK(stepper.Accumulate(eco::kFixedDt) == 1);

    stepper.Reset();
    CHECK(stepper.Accumulate(0.005f) == 0);
    CHECK(stepper.Accumulate(0.005f) == 0);
    CHECK(stepper.Accumulate(0.005f) == 0);
    CHECK(stepper.Accumulate(0.005f) == 1);
    CHECK(stepper.Alpha() >= 0.0f && stepper.Alpha() <= 1.0f);

    stepper.Reset();
    const int clamped_steps = stepper.Accumulate(5.0f);
    CHECK(clamped_steps == 15);

    stepper.Reset();
    const float nan_dt = std::numeric_limits<float>::quiet_NaN();
    const float inf_dt = std::numeric_limits<float>::infinity();
    CHECK(stepper.Accumulate(-0.1f) == 0);
    CHECK(stepper.Accumulate(nan_dt) == 0);
    CHECK_NEAR(stepper.accumulator, 0.0f, 1e-6f);

    for (float bad_max : {0.0f, -0.25f, nan_dt, inf_dt}) {
        eco::FixedTimestepStepper bad_stepper;
        bad_stepper.max_frame_delta = bad_max;
        CHECK(bad_stepper.Accumulate(0.05f) == 0);
        CHECK_NEAR(bad_stepper.accumulator, 0.0f, 1e-6f);
    }
}

void TestInputHelpers() {
    CHECK_NEAR(eco::CancelOpposingKeys(false, false), 0.0f, 1e-6f);
    CHECK_NEAR(eco::CancelOpposingKeys(true, false), -1.0f, 1e-6f);
    CHECK_NEAR(eco::CancelOpposingKeys(false, true), 1.0f, 1e-6f);
    CHECK_NEAR(eco::CancelOpposingKeys(true, true), 0.0f, 1e-6f);

    const float nan_val = std::numeric_limits<float>::quiet_NaN();
    CHECK_NEAR(eco::ApplyAnalogDeadzone(0.0f), 0.0f, 1e-6f);
    CHECK_NEAR(eco::ApplyAnalogDeadzone(0.19f), 0.0f, 1e-6f);
    CHECK_NEAR(eco::ApplyAnalogDeadzone(-0.19f), 0.0f, 1e-6f);
    CHECK_NEAR(eco::ApplyAnalogDeadzone(nan_val), 0.0f, 1e-6f);
    CHECK_NEAR(eco::ApplyAnalogDeadzone(0.5f), 0.5f, 1e-6f);
    CHECK_NEAR(eco::ApplyAnalogDeadzone(1.5f), 1.0f, 1e-6f);
    CHECK_NEAR(eco::ApplyAnalogDeadzone(-1.5f), -1.0f, 1e-6f);

    const eco::Vec2 diag_with_drift =
        eco::CombineKeyboardAndGamepadAxes({1.0f, 1.0f}, {0.1f, -0.15f});
    CHECK_NEAR(diag_with_drift.Length(), 1.0f, 1e-5f);

    eco::AllPlayersInput all_in{};
    CHECK(!all_in.Get(-1).HasAnyAction());
    CHECK(!all_in.Get(4).HasAnyAction());
    CHECK(all_in.TryMutable(-1) == nullptr);
    CHECK(all_in.TryMutable(4) == nullptr);

    // Edge latch / consume behavior including Journal [H]/[J] toggles
    eco::AllPlayersInput pending{};
    eco::AllPlayersInput frame1{};
    frame1.players[0].move_x = 1.0f;
    frame1.players[0].jump_pressed = true;
    frame1.players[0].jump_held = true;
    frame1.toggle_state_pressed = true;
    frame1.toggle_history_pressed = true;
    frame1.toggle_codex_pressed = true;
    eco::LatchFrameInput(pending, frame1);

    eco::AllPlayersInput frame2{};
    frame2.players[0].move_x = 0.5f;
    frame2.players[0].jump_held = true;
    eco::LatchFrameInput(pending, frame2);

    CHECK(pending.players[0].jump_pressed);
    CHECK(pending.toggle_state_pressed);
    CHECK(pending.toggle_history_pressed);
    CHECK(pending.toggle_codex_pressed);
    CHECK_NEAR(pending.players[0].move_x, 0.5f, 1e-6f);

    eco::ConsumePressedEdges(pending);
    CHECK(!pending.players[0].jump_pressed);
    CHECK(!pending.toggle_state_pressed);
    CHECK(!pending.toggle_history_pressed);
    CHECK(!pending.toggle_codex_pressed);
    CHECK(pending.players[0].jump_held);
}

void TestPlayerAndManager() {
    const eco::Player p_low(-5);
    CHECK(p_low.player_id == 0);
    const eco::Player p_high(99);
    CHECK(p_high.player_id == 3);

    for (int i = 0; i < eco::kMaxPlayers; ++i) {
        const eco::ColorRgba ci = eco::ColorForPlayerId(i);
        CHECK(ci.a == 255);
        for (int j = i + 1; j < eco::kMaxPlayers; ++j) {
            CHECK(!(ci == eco::ColorForPlayerId(j)));
        }
    }

    eco::PlayerManager mgr;
    CHECK(mgr.ActiveCount() == 1);
    CHECK(mgr.JoinPlayer(1, {100.0f, 200.0f}));
    CHECK(!mgr.JoinPlayer(1, {100.0f, 200.0f}));
    CHECK(mgr.ActiveCount() == 2);

    eco::AllPlayersInput drop_in{};
    drop_in.players[2].move_x = 1.0f;
    drop_in.players[3].join_pressed = true;
    mgr.ProcessDropIn(drop_in, {50.0f, 50.0f});
    CHECK(mgr.ActiveCount() == 4);

    CHECK(mgr.LeavePlayer(2));
    CHECK(mgr.LeavePlayer(3));
    CHECK(mgr.ActiveCount() == 2);
}

void TestEcoSystem() {
    eco::EcoSystem eco;
    CHECK_NEAR(eco.pollution_level, 35.0f, 1e-6f);
    CHECK(eco.ui_phase == eco::UiPhase::Phase1_Minimal);

    eco.ApplyCleanActivation(30.0f, 15.0f);
    CHECK_NEAR(eco.clean_energy, 30.0f, 1e-6f);
    CHECK_NEAR(eco.pollution_level, 20.0f, 1e-6f);
    CHECK(eco.ui_phase == eco::UiPhase::Phase2_Telemetry);

    eco.SetArchitectRecruited(true);
    CHECK(eco.architect_recruited);
    CHECK(eco.ui_phase == eco::UiPhase::Phase3_EngineOverlay);

    eco.Reset();
    eco.ApplyDirtyBypass(40.0f, 25.0f);
    CHECK_NEAR(eco.dirty_energy, 40.0f, 1e-6f);
    CHECK_NEAR(eco.pollution_level, 60.0f, 1e-6f);

    const eco::ColorRgba base{60, 200, 100, 210};
    eco.pollution_level = 0.0f;
    CHECK(eco.TintColor(base) == base);
    eco.pollution_level = 100.0f;
    CHECK(!(eco.TintColor(base) == base));
}

class MockStateB final : public eco::app::GameState {
public:
    MockStateB(std::vector<std::string>& log) : log_(log) {}
    ~MockStateB() override { log_.push_back("~StateB"); }
    void OnEnter() override { log_.push_back("B::OnEnter"); }
    void OnExit() override { log_.push_back("B::OnExit"); }
    void Update(float) override {}
    void Draw() override {}
    [[nodiscard]] const char* Name() const noexcept override { return "MockStateB"; }

private:
    std::vector<std::string>& log_;
};

class MockStateA final : public eco::app::GameState {
public:
    MockStateA(eco::app::GameStateManager& mgr, std::vector<std::string>& log)
        : mgr_(mgr), log_(log) {}
    ~MockStateA() override { log_.push_back("~StateA"); }
    void OnEnter() override { log_.push_back("A::OnEnter"); }
    void OnExit() override { log_.push_back("A::OnExit"); }
    void Update(float) override {
        mgr_.RequestTransition(std::make_unique<MockStateB>(log_));
        log_.push_back("A::Update_end");
    }
    void Draw() override {}
    [[nodiscard]] const char* Name() const noexcept override { return "MockStateA"; }

private:
    eco::app::GameStateManager& mgr_;
    std::vector<std::string>& log_;
};

void TestGameStateManager() {
    std::vector<std::string> log;
    {
        eco::app::GameStateManager mgr;
        mgr.RequestTransition(std::make_unique<MockStateA>(mgr, log));
        CHECK(mgr.ApplyPendingTransition());
        mgr.Update(eco::kFixedDt);
        CHECK(std::string_view(mgr.CurrentState()->Name()) == "MockStateB");
    }
    const std::vector<std::string> expected{
        "A::OnEnter", "A::Update_end", "A::OnExit", "~StateA",
        "B::OnEnter", "B::OnExit", "~StateB"
    };
    CHECK(log == expected);
}

void TestTilemapMultiZoneAndStructures() {
    const eco::Tilemap map = eco::Tilemap::CreateOverworldMap();
    CHECK(map.Rows() == eco::Tilemap::kOverworldRows);
    CHECK(map.Cols() == eco::Tilemap::kOverworldCols);
    CHECK(map.Rows() == 36);
    CHECK(map.Cols() == 50);

    // Perimeter walls & OOB solidity
    CHECK(map.GetTile(0, 0) == eco::TileId::Wall);
    CHECK(map.GetTile(35, 49) == eco::TileId::Wall);
    CHECK(map.IsSolid(-1, -1));
    CHECK(map.IsSolid(36, 50));

    // Verify all 6 districts are reachable via DistrictAtTile and have non-empty DistrictBounds
    CHECK(eco::Tilemap::DistrictAtTile(30, 10) == eco::ZoneId::DawnTerraces);
    CHECK(eco::Tilemap::DistrictAtTile(18, 10) == eco::ZoneId::CanalQuarter);
    CHECK(eco::Tilemap::DistrictAtTile(5, 10) == eco::ZoneId::WindPlateau);
    CHECK(eco::Tilemap::DistrictAtTile(18, 35) == eco::ZoneId::HangingGardens);
    CHECK(eco::Tilemap::DistrictAtTile(5, 35) == eco::ZoneId::FumaroleCaldera);
    CHECK(eco::Tilemap::DistrictAtTile(30, 35) == eco::ZoneId::HarmonySanctuary);

    for (int z = 0; z < static_cast<int>(eco::ZoneId::Count); ++z) {
        const eco::Rect b = map.DistrictBounds(static_cast<eco::ZoneId>(z));
        CHECK(b.width > 200.0f && b.height > 200.0f);
        CHECK(map.DistrictAt(b.Center()) == static_cast<eco::ZoneId>(z));
    }

    // Verify enriched TileIds exist & have right solidity
    CHECK(!map.FindTiles(eco::TileId::StructurePortal).empty());
    CHECK(!map.FindTiles(eco::TileId::HiddenBridge).empty());
    CHECK(!map.FindTiles(eco::TileId::Water).empty());
    CHECK(!map.FindTiles(eco::TileId::PavedPath).empty());
    CHECK(!map.FindTiles(eco::TileId::SanctuaryFloor).empty());
    CHECK(!map.FindTiles(eco::TileId::GardenFloor).empty());

    for (const auto& w : map.FindTiles(eco::TileId::Water)) {
        CHECK(map.IsSolid(w.row, w.col, false));
        CHECK(map.IsSolid(w.row, w.col, true));
    }
    for (const auto& br : map.FindTiles(eco::TileId::HiddenBridge)) {
        CHECK(map.IsSolid(br.row, br.col, false));
        CHECK(!map.IsSolid(br.row, br.col, true));
    }

    // Verify all 3 internal structures have valid dimensions and ExitPortal tiles
    for (int s = 0; s < static_cast<int>(eco::StructureId::Count); ++s) {
        const auto sid = static_cast<eco::StructureId>(s);
        const eco::Tilemap smap = eco::Tilemap::CreateStructureMap(sid);
        CHECK(smap.Rows() >= 14 && smap.Cols() >= 28);
        CHECK(!smap.FindTiles(eco::TileId::ExitPortal).empty());
    }
}

void TestPhysics2D() {
    eco::Tilemap wall_map({
        {1, 1, 1, 1, 1, 1},
        {1, 0, 0, 1, 0, 1},
        {1, 0, 0, 1, 0, 1},
        {1, 0, 0, 1, 0, 1},
        {1, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1}
    }, 48.0f);

    eco::Player p_slide(0);
    p_slide.pos = {144.0f - p_slide.size.x - 1.0f, 52.0f};
    const float y_before = p_slide.pos.y;
    for (int i = 0; i < 25; ++i) {
        eco::MovePlayerTopDown(p_slide, {1.0f, 1.0f}, eco::kFixedDt, wall_map);
        CHECK(!wall_map.OverlapsSolid(p_slide.Bounds()));
    }
    CHECK_NEAR(p_slide.vel.x, 0.0f, 1e-5f);
    CHECK(p_slide.pos.y > y_before + 48.0f);

    // Side-scrolling coyote time & jump buffer
    eco::Tilemap ledge_map({
        {1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 1},
        {1, 1, 1, 0, 0, 1},
        {1, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1}
    }, 48.0f);
    eco::Player p(0);
    p.pos = {143.0f, 66.0f};
    eco::MovePlayerSideScrolling(p, 0.0f, false, false, eco::kFixedDt, ledge_map);
    CHECK(p.isGrounded);
    eco::MovePlayerSideScrolling(p, 1.0f, false, false, eco::kFixedDt, ledge_map);
    CHECK(!p.isGrounded && p.coyote_timer > 0.0f);
    eco::MovePlayerSideScrolling(p, 0.0f, true, true, eco::kFixedDt, ledge_map);
    CHECK(p.y_velocity <= -460.0f);
}

void TestSixPhysicalLaws() {
    // 1. Lambert Optical Law (SolarArray)
    CHECK_NEAR(eco::ComputeSolarEfficiency(45.0f, 45.0f), 1.0f, 1e-5f);
    CHECK(eco::ComputeSolarEfficiency(60.0f, 45.0f) > eco::kSolarEfficiencyThreshold);
    CHECK(eco::ComputeSolarEfficiency(90.0f, 45.0f) <= eco::kSolarEfficiencyThreshold);

    // 2. Stevino Hydrostatic Law (HydroTurbine)
    CHECK_NEAR(eco::ComputeHydroEfficiency(10.0f, 10.0f, 20000.0f), 1.0f, 1e-5f);
    CHECK(eco::ComputeHydroEfficiency(10.3f, 10.0f, 20000.0f) > eco::kHydroEfficiencyThreshold);
    CHECK(eco::ComputeHydroEfficiency(13.0f, 10.0f, 20000.0f) <= eco::kHydroEfficiencyThreshold);

    // 3. Galileo Isochronous Pendulum (PendulumRegulator): T = 2*pi*sqrt(L/g)
    const float t_2m = eco::ComputePendulumPeriod(2.0f);
    CHECK_NEAR(t_2m, 2.8369f, 0.02f);
    CHECK_NEAR(eco::ComputePendulumEfficiency(2.0f, 2.0f), 1.0f, 1e-5f);
    CHECK(eco::ComputePendulumEfficiency(2.05f, 2.0f) > eco::kPendulumEfficiencyThreshold);
    CHECK(eco::ComputePendulumEfficiency(4.5f, 2.0f) <= eco::kPendulumEfficiencyThreshold);

    // 4. Harmonic Resonance Curve (HarmonicResonator): f0 = 120 Hz, Q = 3.0
    CHECK_NEAR(eco::ComputeHarmonicEfficiency(120.0f, 120.0f, 3.0f), 1.0f, 1e-5f);
    CHECK(eco::ComputeHarmonicEfficiency(126.0f, 120.0f, 3.0f) > eco::kHarmonicEfficiencyThreshold);
    CHECK(eco::ComputeHarmonicEfficiency(200.0f, 120.0f, 3.0f) <= eco::kHarmonicEfficiencyThreshold);

    // 5. Carnot Thermodynamics (ThermalExchanger): Tc=300 K, Th=600 K -> eta = 0.50
    CHECK_NEAR(eco::ComputeCarnotEta(600.0f, 300.0f), 0.50f, 1e-4f);
    CHECK_NEAR(eco::ComputeThermalEfficiency(600.0f, 600.0f), 1.0f, 1e-5f);
    CHECK(eco::ComputeThermalEfficiency(615.0f, 600.0f) > eco::kThermalEfficiencyThreshold);
    CHECK(eco::ComputeThermalEfficiency(850.0f, 600.0f) <= eco::kThermalEfficiencyThreshold);

    // 6. Central Astrolabe Phase Interference: I = cos^2((phi - phi0)/2)
    CHECK_NEAR(eco::ComputeAstrolabeEfficiency(180.0f, 180.0f), 1.0f, 1e-5f);
    CHECK(eco::ComputeAstrolabeEfficiency(190.0f, 180.0f) > eco::kAstrolabeEfficiencyThreshold);
    CHECK(eco::ComputeAstrolabeEfficiency(0.0f, 180.0f) <= 1e-4f);

    // Check machine labels are ASCII & clean
    for (int m = 0; m < static_cast<int>(eco::MachineType::Count); ++m) {
        const auto mt = static_cast<eco::MachineType>(m);
        CHECK(IsPureAsciiPrintable(eco::MachineName(mt)));
        CHECK(IsPureAsciiPrintable(eco::MachineLawName(mt)));
        CHECK(IsPureAsciiPrintable(eco::MachineFormulaLabel(mt)));
        CHECK(!ContainsForbiddenLegacyToken(eco::MachineName(mt)));
        CHECK(!ContainsForbiddenLegacyToken(eco::MachineLawName(mt)));
    }
}

void TestCollectiblesAndCodex() {
    eco::CollectibleRegistry reg;
    reg.ResetDefault();

    CHECK(reg.TotalCount() == eco::kTotalCollectibles);
    CHECK(reg.TotalCount() == 16);
    CHECK(reg.CollectedCount() == 0);
    CHECK(!reg.HasRecentPickup());

    // Verify 4 relics per Era
    for (int e = 0; e < static_cast<int>(eco::CollectibleEra::Count); ++e) {
        const auto era = static_cast<eco::CollectibleEra>(e);
        CHECK(reg.TotalInEra(era) == eco::kCollectiblesPerEra);
        CHECK(reg.CollectedInEra(era) == 0);
        CHECK(IsPureAsciiPrintable(eco::EraName(era)));
    }

    // Verify 10 in Overworld and 2 in each of the 3 Structures
    CHECK(reg.TotalInLocation(eco::LocationId::Overworld) == 10);
    CHECK(reg.TotalInLocation(eco::LocationId::HypogeanCisterns) == 2);
    CHECK(reg.TotalInLocation(eco::LocationId::PendulumTower) == 2);
    CHECK(reg.TotalInLocation(eco::LocationId::GeothermalChamber) == 2);

    // Audit every single relic's metadata (ASCII 32..126, non-empty, zero Calabria/script)
    for (int i = 0; i < eco::kTotalCollectibles; ++i) {
        const auto id = static_cast<eco::CollectibleId>(i);
        const auto& meta = eco::CollectibleMetadata(id);
        CHECK(meta.id == id);
        CHECK(!meta.code.empty());
        CHECK(!meta.title.empty());
        CHECK(!meta.pickup_speaker.empty());
        CHECK(!meta.pickup_line.empty());
        CHECK(!meta.codex_paragraphs.empty());

        CHECK(IsPureAsciiPrintable(meta.code));
        CHECK(IsPureAsciiPrintable(meta.title));
        CHECK(IsPureAsciiPrintable(meta.era_name));
        CHECK(IsPureAsciiPrintable(meta.zone_name));
        CHECK(IsPureAsciiPrintable(meta.pickup_speaker));
        CHECK(IsPureAsciiPrintable(meta.pickup_line));
        CHECK(!ContainsForbiddenLegacyToken(meta.title));
        CHECK(!ContainsForbiddenLegacyToken(meta.zone_name));
        CHECK(!ContainsForbiddenLegacyToken(meta.pickup_line));

        for (const auto& para : meta.codex_paragraphs) {
            CHECK(!para.empty());
            CHECK(IsPureAsciiPrintable(para));
            CHECK(!ContainsForbiddenLegacyToken(para));
        }
    }

    // Verify proximity auto-pickup & banner timer
    eco::PlayerManager players;
    const auto* item0 = reg.Find(eco::CollectibleId::Dawn_BrassGnomon);
    CHECK(item0 != nullptr);
    players.GetPlayer(0)->pos = item0->pos;
    const auto picked = reg.TryPickupNear(eco::LocationId::Overworld, players, false);
    CHECK(picked.has_value() && *picked == eco::CollectibleId::Dawn_BrassGnomon);
    CHECK(reg.IsCollected(eco::CollectibleId::Dawn_BrassGnomon));
    CHECK(reg.CollectedCount() == 1);
    CHECK(reg.HasRecentPickup());
    CHECK(reg.LastCollectedId() == eco::CollectibleId::Dawn_BrassGnomon);

    reg.Tick(eco::kRelicBannerDuration + 0.5f);
    CHECK(!reg.HasRecentPickup());

    reg.CollectAll();
    CHECK(reg.CollectedCount() == 16);
}

void TestStoryHistoryAndFullCampaign() {
    // 1. Audit all 7 chapters, IntroCrawl, all 32 StoryEvents, and all 3 Endings
    const eco::StorySnapshot base_snap{};
    for (int c = 0; c <= static_cast<int>(eco::Chapter::Epilogo); ++c) {
        const auto chap = static_cast<eco::Chapter>(c);
        const char* title = eco::ChapterTitle(chap);
        CHECK(title != nullptr && std::string_view(title).size() > 0);
        CHECK(IsPureAsciiPrintable(title));
        CHECK(!ContainsForbiddenLegacyToken(title));

        const eco::Objective obj = eco::ObjectiveFor(chap, base_snap);
        CHECK(!obj.title.empty());
        CHECK(IsPureAsciiPrintable(obj.title));
        CHECK(IsPureAsciiPrintable(obj.hint));
        CHECK(!ContainsForbiddenLegacyToken(obj.title));
        CHECK(!ContainsForbiddenLegacyToken(obj.hint));
    }

    const auto intro = eco::IntroCrawl();
    CHECK(!intro.empty());
    for (const auto& line : intro) {
        CHECK(IsPureAsciiPrintable(line));
        CHECK(!ContainsForbiddenLegacyToken(line));
    }

    for (int e = 0; e < static_cast<int>(eco::StoryEvent::Count); ++e) {
        const auto ev = static_cast<eco::StoryEvent>(e);
        const auto lines = eco::DialogueForEvent(ev, base_snap);
        CHECK(!lines.empty());
        for (const auto& dl : lines) {
            CHECK(!dl.speaker.empty() && !dl.text.empty());
            CHECK(IsPureAsciiPrintable(dl.speaker));
            CHECK(IsPureAsciiPrintable(dl.text));
            CHECK(!ContainsForbiddenLegacyToken(dl.speaker));
            CHECK(!ContainsForbiddenLegacyToken(dl.text));
        }
    }

    // 3 Endings: True Ending (>=12 clean), Clean Ending (<12 clean), Grey/Pitch Ending (dirty)
    const auto ending_true = eco::EndingText(true, 14, 16);
    const auto ending_clean = eco::EndingText(true, 5, 16);
    const auto ending_dirty = eco::EndingText(false, 16, 16);
    CHECK(!ending_true.empty() && !ending_clean.empty() && !ending_dirty.empty());
    CHECK(ending_true != ending_clean);
    CHECK(ending_clean != ending_dirty);
    for (const auto* vec : {&ending_true, &ending_clean, &ending_dirty}) {
        for (const auto& l : *vec) {
            CHECK(IsPureAsciiPrintable(l));
            CHECK(!ContainsForbiddenLegacyToken(l));
        }
    }

    // 2. StoryDirector Dialogue History & PushAdHocLine
    eco::StoryDirector dir;
    dir.Reset();
    CHECK(dir.History().empty());
    CHECK(dir.CleanPath());

    eco::StorySnapshot snap{};
    dir.Observe(snap);
    CHECK(dir.EventFired(eco::StoryEvent::GameStarted));
    CHECK(!dir.History().empty());

    const std::size_t after_first = dir.History().size();
    while (dir.HasActiveDialogue()) {
        dir.AdvanceDialogue();
    }
    CHECK(dir.History().size() >= after_first);
    // Sequence IDs strictly increasing
    for (std::size_t i = 1; i < dir.History().size(); ++i) {
        CHECK(dir.History()[i].sequence_id > dir.History()[i - 1].sequence_id);
    }

    const std::size_t before_adhoc = dir.History().size();
    dir.PushAdHocLine("Elena Valli", "Un frammento inciso di ottone.", eco::ZoneId::DawnTerraces, "Terrazze dell'Alba");
    while (dir.HasActiveDialogue()) {
        dir.AdvanceDialogue();
    }
    CHECK(dir.History().size() == before_adhoc + 1);
    CHECK(dir.History().back().is_ad_hoc);
    CHECK(dir.History().back().speaker == "Elena Valli");

    // Bypass sets CleanPath() to false
    snap.hydro = eco::MachineState::BypassedDirty;
    dir.Observe(snap);
    CHECK(dir.EventFired(eco::StoryEvent::HydroBypassed));
    CHECK(!dir.CleanPath());

    // 3. Full WorldContext Campaign E2E (Default Full Campaign, 6 districts, 3 structures, 4 NPCs, 16 relics)
    eco::WorldContext world;
    CHECK(!world.demo_mode); // Campagna Completa attiva di default!
    world.EnsureInitialized();
    world.overworld.OnEnter(world.players, false, false);

    // Verify TeleportToZone updates current_zone and arms zone banner
    world.TeleportToZone(eco::ZoneId::WindPlateau);
    CHECK(world.current_zone == eco::ZoneId::WindPlateau);
    CHECK(world.zone_banner_timer > 0.0f);

    // Verify all 4 NPCs exist and have clean Meridia names
    for (int n = 0; n < static_cast<int>(eco::NpcId::Count); ++n) {
        const auto nid = static_cast<eco::NpcId>(n);
        const auto& npc = world.GetNpc(nid);
        CHECK(!npc.name.empty() && !npc.role.empty());
        CHECK(IsPureAsciiPrintable(npc.name));
        CHECK(!ContainsForbiddenLegacyToken(npc.name));
    }

    // Complete all 5 monuments cleanly + recruit Clara + unlock Astrolabe
    auto* p0 = world.players.GetPlayer(0);
    auto calibrate_clean = [&](eco::MachineType mt) {
        auto& m = world.GetMachine(mt);
        if (m.location == eco::LocationId::Overworld) {
            world.current_mode = eco::GameMode::Overworld;
        } else {
            world.EnterStructure(eco::StructureFromLocation(m.location));
        }
        p0 = world.players.GetPlayer(0);
        p0->pos = m.pos;
        eco::AllPlayersInput in{};
        in.players[0].interact_pressed = true;
        (void)world.StepWorld(in, eco::kFixedDt);
        CHECK(world.calibration_modal.is_open);
        world.calibration_modal.SetParameter(m.target_param, m);
        in = {};
        in.players[0].confirm_pressed = true;
        (void)world.StepWorld(in, eco::kFixedDt);
        CHECK(m.state == eco::MachineState::CleanActive);
    };

    calibrate_clean(eco::MachineType::SolarArray);

    // Recruit Clara Sereni -> unlocks Ponte degli Archi
    world.current_mode = eco::GameMode::Overworld;
    p0->pos = world.GetNpc(eco::NpcId::ClaraSereni).pos;
    {
        eco::AllPlayersInput in{};
        in.players[0].interact_pressed = true;
        (void)world.StepWorld(in, eco::kFixedDt);
    }
    CHECK(world.GetNpc(eco::NpcId::ClaraSereni).recruited);
    CHECK(world.ecosystem.architect_recruited);

    calibrate_clean(eco::MachineType::HydroTurbine);
    calibrate_clean(eco::MachineType::PendulumRegulator);
    calibrate_clean(eco::MachineType::HarmonicResonator);
    calibrate_clean(eco::MachineType::ThermalExchanger);

    CHECK(world.AllFiveMonumentsActivated());
    CHECK(world.GetMachine(eco::MachineType::CentralAstrolabe).unlocked);
    CHECK(!world.simulation_completed);

    // Collect 12+ relics and activate CentralAstrolabe -> Epilogo & True Ending
    world.collectibles.CollectAll();
    calibrate_clean(eco::MachineType::CentralAstrolabe);
    CHECK(world.simulation_completed);
    CHECK(world.story.CurrentChapter() == eco::Chapter::Epilogo);
    CHECK(world.story.CleanPath());

    // Drain active dialogue queue so every triggered dialogue is shown & recorded in History
    while (world.story.HasActiveDialogue()) {
        world.story.AdvanceDialogue();
    }
    CHECK(world.story.History().size() >= 10);
}

void TestSaveAndLoadPersistence() {
    const std::string test_save_path = "test_tmp_meridia_save.sav";
    (void)eco::DeleteSaveFile(test_save_path);

    eco::WorldContext src;
    src.EnsureInitialized();
    src.overworld.OnEnter(src.players, false, false);
    src.GetMachine(eco::MachineType::SolarArray).state = eco::MachineState::CleanActive;
    src.GetMachine(eco::MachineType::SolarArray).current_param = 45.0f;
    src.GetNpc(eco::NpcId::ClaraSereni).met = true;
    src.GetNpc(eco::NpcId::ClaraSereni).recruited = true;
    src.ecosystem.ApplyCleanActivation(30.0f, 15.0f);
    src.ecosystem.SetArchitectRecruited(true);
    (void)src.collectibles.Collect(eco::CollectibleId::Dawn_BrassGnomon);
    (void)src.collectibles.Collect(eco::CollectibleId::Waters_JadeTuningFork);
    src.TeleportToZone(eco::ZoneId::HangingGardens);
    src.story.Observe(src.BuildStorySnapshot());
    while (src.story.HasActiveDialogue()) {
        src.story.AdvanceDialogue();
    }
    const std::size_t saved_hist_count = src.story.History().size();
    CHECK(saved_hist_count > 0);

    CHECK(eco::SaveWorldToFile(src, test_save_path, "2026-11-22 19:40"));

    const eco::SaveSummary summary = eco::ReadSaveSummary(test_save_path);
    CHECK(summary.exists);
    CHECK(summary.mode == eco::GameMode::Overworld);
    CHECK(summary.zone == eco::ZoneId::HangingGardens);
    CHECK(summary.monuments_active == 1);
    CHECK(summary.relics_found == 2);
    CHECK(summary.timestamp == "2026-11-22 19:40");

    eco::WorldContext dst;
    CHECK(eco::LoadWorldFromFile(dst, test_save_path));
    CHECK(dst.loaded_from_save);
    CHECK(dst.current_mode == eco::GameMode::Overworld);
    CHECK(dst.current_zone == eco::ZoneId::HangingGardens);
    CHECK(dst.GetMachine(eco::MachineType::SolarArray).state == eco::MachineState::CleanActive);
    CHECK(dst.ecosystem.architect_recruited);
    CHECK(dst.collectibles.CollectedCount() == 2);
    CHECK(dst.collectibles.IsCollected(eco::CollectibleId::Dawn_BrassGnomon));
    CHECK(dst.collectibles.IsCollected(eco::CollectibleId::Waters_JadeTuningFork));
    CHECK(dst.story.History().size() == saved_hist_count);

    // Also verify saving/loading inside an internal Structure
    src.EnterStructure(eco::StructureId::PendulumTower);
    CHECK(eco::SaveWorldToFile(src, test_save_path, "2026-11-22 19:41"));
    const eco::SaveSummary struct_summary = eco::ReadSaveSummary(test_save_path);
    CHECK(struct_summary.exists);
    CHECK(struct_summary.mode == eco::GameMode::Structure);
    CHECK(struct_summary.structure == eco::StructureId::PendulumTower);

    eco::WorldContext dst_struct;
    CHECK(eco::LoadWorldFromFile(dst_struct, test_save_path));
    CHECK(dst_struct.current_mode == eco::GameMode::Structure);
    CHECK(dst_struct.active_structure == eco::StructureId::PendulumTower);

    (void)eco::DeleteSaveFile(test_save_path);
}

} // namespace

int main() {
    TestMathTypes();
    TestFixedTimestepStepper();
    TestInputHelpers();
    TestPlayerAndManager();
    TestEcoSystem();
    TestGameStateManager();
    TestTilemapMultiZoneAndStructures();
    TestPhysics2D();
    TestSixPhysicalLaws();
    TestCollectiblesAndCodex();
    TestStoryHistoryAndFullCampaign();
    TestSaveAndLoadPersistence();

    if (g_failures != 0) {
        std::cerr << g_failures << " test check(s) failed.\n";
        return 1;
    }

    std::cout << "All La Conca di Meridia unit & integration tests passed.\n";
    return 0;
}
