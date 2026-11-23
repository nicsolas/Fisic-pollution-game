#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <queue>
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
    // Verifica tutte e 6 le mappe dedicate di zona (26x36)
    for (int z = 0; z < static_cast<int>(eco::ZoneId::Count); ++z) {
        const auto zid = static_cast<eco::ZoneId>(z);
        const eco::Tilemap map = eco::Tilemap::CreateZoneMap(zid);
        CHECK(map.Rows() == eco::Tilemap::kZoneRows);
        CHECK(map.Cols() == eco::Tilemap::kZoneCols);
        CHECK(map.Rows() == 26);
        CHECK(map.Cols() == 36);
        CHECK(map.GetTile(0, 0) == eco::TileId::Wall);
        CHECK(map.GetTile(25, 35) == eco::TileId::Wall);
        CHECK(map.IsSolid(-1, -1));
        CHECK(map.IsSolid(26, 36));
        CHECK(!map.FindTiles(eco::TileId::ZoneGateway).empty());
        CHECK(!eco::Tilemap::ZoneGateways(zid).empty());
    }

    // Verifica nuovi TileId specifici per zona e loro solidita'
    const eco::Tilemap dawn = eco::Tilemap::CreateZoneMap(eco::ZoneId::DawnTerraces);
    CHECK(!dawn.FindTiles(eco::TileId::SolarGate).empty());
    for (const auto& sg : dawn.FindTiles(eco::TileId::SolarGate)) {
        CHECK(dawn.IsSolid(sg.row, sg.col, false, false, false));
        CHECK(!dawn.IsSolid(sg.row, sg.col, false, true, false));
    }

    const eco::Tilemap canal = eco::Tilemap::CreateZoneMap(eco::ZoneId::CanalQuarter);
    CHECK(!canal.FindTiles(eco::TileId::ShallowChannel).empty());
    CHECK(!canal.FindTiles(eco::TileId::HiddenBridge).empty());
    CHECK(!canal.FindTiles(eco::TileId::StructurePortal).empty());
    for (const auto& br : canal.FindTiles(eco::TileId::HiddenBridge)) {
        CHECK(canal.IsSolid(br.row, br.col, false));
        CHECK(!canal.IsSolid(br.row, br.col, true));
    }
    for (const auto& sc : canal.FindTiles(eco::TileId::ShallowChannel)) {
        CHECK(!canal.IsSolid(sc.row, sc.col, false));
    }

    const eco::Tilemap gardens = eco::Tilemap::CreateZoneMap(eco::ZoneId::HangingGardens);
    CHECK(!gardens.FindTiles(eco::TileId::JadeBridge).empty());
    CHECK(!gardens.FindTiles(eco::TileId::GardenFloor).empty());
    for (const auto& jb : gardens.FindTiles(eco::TileId::JadeBridge)) {
        CHECK(gardens.IsSolid(jb.row, jb.col, true, true, false));
        CHECK(!gardens.IsSolid(jb.row, jb.col, false, false, true));
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
            world.TeleportToZone(eco::WorldContext::MachineZone(mt));
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
    world.TeleportToZone(eco::ZoneId::CanalQuarter);
    p0 = world.players.GetPlayer(0);
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
    src.zone_mechanics.solar_gate_open = true;
    src.zone_mechanics.jade_bridge_unlocked = true;
    if (!src.zone_mechanics.sluice_levers.empty()) {
        src.zone_mechanics.sluice_levers[0].pulled = true;
    }
    if (!src.zone_mechanics.relief_valves.empty()) {
        src.zone_mechanics.relief_valves[0].open = true;
    }
    if (!src.zone_mechanics.sanctuary_seals.empty()) {
        src.zone_mechanics.sanctuary_seals[0].attuned = true;
    }
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
    CHECK(dst.zone_mechanics.solar_gate_open);
    CHECK(dst.zone_mechanics.jade_bridge_unlocked);
    CHECK(!dst.zone_mechanics.sluice_levers.empty() && dst.zone_mechanics.sluice_levers[0].pulled);
    CHECK(!dst.zone_mechanics.relief_valves.empty() && dst.zone_mechanics.relief_valves[0].open);
    CHECK(!dst.zone_mechanics.sanctuary_seals.empty() && dst.zone_mechanics.sanctuary_seals[0].attuned);

    // Also verify saving/loading inside PendulumTower with partial chime sequence I -> II (Bridge 0 extended)
    src.EnterStructure(eco::StructureId::PendulumTower);
    (void)src.structure.puzzle.ToggleSwitchById(0, src.structure.tilemap); // Chime I
    (void)src.structure.puzzle.ToggleSwitchById(1, src.structure.tilemap); // Chime II -> Bridge 0 extends!
    src.structure.SyncActivePuzzleToSaved();
    CHECK(src.structure.puzzle.GetGate(0)->open);
    {
        auto* p0 = src.players.GetPlayer(0);
        const eco::Rect bridge_tile = src.structure.tilemap.TileRect(8, 17);
        p0->pos = {bridge_tile.x + 9.0f, bridge_tile.Bottom() - p0->size.y};
    }
    CHECK(eco::SaveWorldToFile(src, test_save_path, "2026-11-22 19:41"));
    const eco::SaveSummary struct_summary = eco::ReadSaveSummary(test_save_path);
    CHECK(struct_summary.exists);
    CHECK(struct_summary.mode == eco::GameMode::Structure);
    CHECK(struct_summary.structure == eco::StructureId::PendulumTower);

    eco::WorldContext dst_struct;
    CHECK(eco::LoadWorldFromFile(dst_struct, test_save_path));
    CHECK(dst_struct.current_mode == eco::GameMode::Structure);
    CHECK(dst_struct.active_structure == eco::StructureId::PendulumTower);
    CHECK(dst_struct.structure.puzzle.current_sequence.size() == 2);
    CHECK(dst_struct.structure.puzzle.GetSwitch(0)->active);
    CHECK(dst_struct.structure.puzzle.GetSwitch(1)->active);
    CHECK(!dst_struct.structure.puzzle.GetSwitch(2)->active);
    CHECK(dst_struct.structure.puzzle.GetGate(0)->open);
    CHECK(dst_struct.structure.tilemap.IsSolid(9, 17)); // Bridge 0 restored as solid floor!
    CHECK(dst_struct.players.GetPlayer(0)->isGrounded);

    // Verify saving/loading inside HypogeanCisterns past open gates (A=OFF, B=ON, C=ON -> no softlock!)
    src.EnterStructure(eco::StructureId::HypogeanCisterns);
    (void)src.structure.puzzle.ToggleSwitchById(0, src.structure.tilemap); // A ON
    (void)src.structure.puzzle.ToggleSwitchById(1, src.structure.tilemap); // B ON
    (void)src.structure.puzzle.ToggleSwitchById(2, src.structure.tilemap); // C ON
    (void)src.structure.puzzle.ToggleSwitchById(0, src.structure.tilemap); // A OFF -> Gate 0 & Gate 2 open!
    src.structure.SyncActivePuzzleToSaved();
    CHECK(src.structure.puzzle.solved);
    {
        auto* p0 = src.players.GetPlayer(0);
        const eco::Rect vault_tile = src.structure.tilemap.TileRect(17, 30);
        p0->pos = {vault_tile.x + 9.0f, vault_tile.Bottom() - p0->size.y};
    }
    CHECK(eco::SaveWorldToFile(src, test_save_path, "2026-11-22 19:42"));

    eco::WorldContext dst_cisterns;
    CHECK(eco::LoadWorldFromFile(dst_cisterns, test_save_path));
    CHECK(dst_cisterns.current_mode == eco::GameMode::Structure);
    CHECK(dst_cisterns.active_structure == eco::StructureId::HypogeanCisterns);
    CHECK(!dst_cisterns.structure.puzzle.GetSwitch(0)->active); // A OFF
    CHECK(dst_cisterns.structure.puzzle.GetSwitch(1)->active);  // B ON
    CHECK(dst_cisterns.structure.puzzle.GetSwitch(2)->active);  // C ON
    CHECK(dst_cisterns.structure.puzzle.GetGate(0)->open);
    CHECK(!dst_cisterns.structure.puzzle.GetGate(1)->open);
    CHECK(dst_cisterns.structure.puzzle.GetGate(2)->open);
    CHECK(dst_cisterns.structure.puzzle.solved);
    CHECK(!dst_cisterns.structure.tilemap.IsSolid(16, 16)); // Gate 0 passable!
    CHECK(!dst_cisterns.structure.tilemap.IsSolid(16, 28)); // Gate 2 passable!
    // Player is NOT trapped: can walk straight left back to ExitPortal through open Gate 2 and Gate 0
    dst_cisterns.structure.portal_cooldown_timer = 0.0f;
    dst_cisterns.structure.portal_reentry_locked = false;
    bool escaped_vault = false;
    for (int t = 0; t < 600 && !escaped_vault; ++t) {
        eco::AllPlayersInput in{};
        in.players[0].move_x = -1.0f;
        if (dst_cisterns.StepWorld(in, eco::kFixedDt).exit_to_overworld_requested) {
            escaped_vault = true;
        }
    }
    CHECK(escaped_vault);

    // Also verify PendulumTower's saved levers persisted alongside HypogeanCisterns!
    dst_cisterns.EnterStructure(eco::StructureId::PendulumTower);
    CHECK(dst_cisterns.structure.puzzle.current_sequence.size() == 2);
    CHECK(dst_cisterns.structure.puzzle.GetGate(0)->open);

    (void)eco::DeleteSaveFile(test_save_path);
}

void TestZoneBordersFlatMemoryAndGuidance() {
    eco::WorldContext world;
    world.EnsureInitialized();

    // 1. Ottimizzazione memoria reale: solo la zona attiva residente in RAM
    const std::size_t expected_zone_bytes =
        static_cast<std::size_t>(eco::Tilemap::kZoneRows * eco::Tilemap::kZoneCols);
    const eco::MemoryMetrics mem0 = world.GetMemoryMetrics();
    CHECK(mem0.active_zone_tilemap_bytes == expected_zone_bytes);
    CHECK(mem0.active_structure_tilemap_bytes == 0);
    CHECK(mem0.total_resident_map_bytes == expected_zone_bytes);
    CHECK(mem0.saved_unloaded_bytes > expected_zone_bytes * 4);
    CHECK(mem0.zone_swaps_count == 0);

    // Entrando in una struttura, l'overworld viene scaricato e resta in RAM solo la struttura
    world.EnterStructure(eco::StructureId::HypogeanCisterns);
    const eco::MemoryMetrics mem_struct = world.GetMemoryMetrics();
    CHECK(mem_struct.active_zone_tilemap_bytes == 0);
    CHECK(mem_struct.active_structure_tilemap_bytes > 0);
    CHECK(mem_struct.total_resident_map_bytes == mem_struct.active_structure_tilemap_bytes);

    world.ExitToOverworld();
    const eco::MemoryMetrics mem_back = world.GetMemoryMetrics();
    CHECK(mem_back.active_zone_tilemap_bytes == expected_zone_bytes);
    CHECK(mem_back.active_structure_tilemap_bytes == 0);

    // 2. BFS Reachability per ciascuna delle 6 zone separate (entita', portali, varchi, reliquie)
    auto run_bfs = [](const eco::Tilemap& map, eco::Vec2 start_pos,
                      bool bridge_unlocked, bool solar_open, bool jade_open) -> std::vector<bool> {
        const int rows = map.Rows();
        const int cols = map.Cols();
        std::vector<bool> visited(static_cast<std::size_t>(rows * cols), false);
        const eco::TileCoord start = map.WorldToTile(start_pos);
        if (!map.InBounds(start.row, start.col) ||
            map.IsSolid(start.row, start.col, bridge_unlocked, solar_open, jade_open)) {
            return visited;
        }
        std::queue<eco::TileCoord> q;
        visited[static_cast<std::size_t>(start.row * cols + start.col)] = true;
        q.push(start);
        static constexpr int kDr[4] = {-1, 1, 0, 0};
        static constexpr int kDc[4] = {0, 0, -1, 1};
        while (!q.empty()) {
            const eco::TileCoord cur = q.front();
            q.pop();
            for (int d = 0; d < 4; ++d) {
                const int nr = cur.row + kDr[d];
                const int nc = cur.col + kDc[d];
                if (!map.InBounds(nr, nc) ||
                    map.IsSolid(nr, nc, bridge_unlocked, solar_open, jade_open)) {
                    continue;
                }
                const std::size_t idx = static_cast<std::size_t>(nr * cols + nc);
                if (!visited[idx]) {
                    visited[idx] = true;
                    q.push({nr, nc});
                }
            }
        }
        return visited;
    };

    auto is_reached = [](const eco::Tilemap& map, const std::vector<bool>& vis, eco::Vec2 pos) -> bool {
        const eco::TileCoord tc = map.WorldToTile(pos);
        if (!map.InBounds(tc.row, tc.col)) {
            return false;
        }
        return vis[static_cast<std::size_t>(tc.row * map.Cols() + tc.col)];
    };

    for (int z = 0; z < static_cast<int>(eco::ZoneId::Count); ++z) {
        const auto zid = static_cast<eco::ZoneId>(z);
        world.TeleportToZone(zid);
        const eco::Tilemap& map = world.overworld.tilemap;
        const eco::TileCoord sp_tile = eco::Tilemap::DefaultZoneSpawnTile(zid);
        const std::vector<bool> vis_all = run_bfs(map, map.TileCenter(sp_tile.row, sp_tile.col), true, true, true);

        // Tutti i varchi della zona devono essere raggiungibili
        CHECK(!world.overworld.active_gateways.empty());
        for (const auto& gw : world.overworld.active_gateways) {
            CHECK(gw.signpost_label != nullptr && std::string_view(gw.signpost_label).size() > 0);
            CHECK(IsPureAsciiPrintable(gw.signpost_label));
            CHECK(is_reached(map, vis_all, gw.world_center));
        }

        // Tutti i monumenti, NPC, portali e reliquie di questa zona devono essere raggiungibili
        for (const eco::Machine* m : world.MachinesInCurrentScene()) {
            CHECK(is_reached(map, vis_all, m->pos));
        }
        for (const eco::WorldNpc* npc : world.NpcsInCurrentScene()) {
            CHECK(is_reached(map, vis_all, npc->pos));
        }
        for (const auto& portal : world.overworld.portals) {
            if (portal.source_zone == zid) {
                CHECK(is_reached(map, vis_all, portal.world_center));
            }
        }
        for (const eco::CollectibleItem* item : world.CollectiblesInCurrentScene()) {
            CHECK(is_reached(map, vis_all, item->pos));
        }
    }

    // 3. Reachability interna delle 3 strutture: bloccata prima del puzzle, aperta dopo la soluzione
    for (int s = 0; s < static_cast<int>(eco::StructureId::Count); ++s) {
        const auto sid = static_cast<eco::StructureId>(s);
        world.EnterStructure(sid);
        const eco::Tilemap& smap = world.structure.tilemap;
        const eco::Vec2 sp = {
            world.structure.spawn_point.x + eco::kPlayerWidth * 0.5f,
            world.structure.spawn_point.y + eco::kPlayerHeight * 0.5f
        };
        const std::vector<bool> vis_locked = run_bfs(smap, sp, false, false, false);
        for (const eco::Machine* m : world.MachinesInCurrentScene()) {
            CHECK(!is_reached(smap, vis_locked, m->pos));
        }
        const std::vector<bool> vis_open = run_bfs(smap, sp, true, true, true);
        for (const eco::Machine* m : world.MachinesInCurrentScene()) {
            CHECK(is_reached(smap, vis_open, m->pos));
        }
    }

    // 4. CurrentObjectiveTarget() guida verso il varco se il target e' in altra zona
    world.Reset();
    world.EnsureInitialized();
    eco::ObjectiveTarget t = world.CurrentObjectiveTarget();
    CHECK(t.valid && !t.is_portal_step && t.zone == eco::ZoneId::DawnTerraces);
    CHECK(t.world_pos == world.GetMachine(eco::MachineType::SolarArray).pos);

    world.GetMachine(eco::MachineType::SolarArray).state = eco::MachineState::CleanActive;
    t = world.CurrentObjectiveTarget();
    CHECK(t.valid && t.is_portal_step && t.zone == eco::ZoneId::CanalQuarter);

    world.EnterStructure(eco::StructureId::HypogeanCisterns);
    t = world.CurrentObjectiveTarget();
    CHECK(t.valid && !t.is_portal_step && t.location == eco::LocationId::HypogeanCisterns);

    world.EnterStructure(eco::StructureId::PendulumTower);
    t = world.CurrentObjectiveTarget();
    CHECK(t.valid && t.is_portal_step && t.location == eco::LocationId::PendulumTower);
}

void TestSixZoneTopDownMechanics() {
    eco::WorldContext world;
    world.EnsureInitialized();
    world.overworld.OnEnter(world.players, false, false);

    // 1. Terrazze dell'Alba: Specchi Solari -> Fasci -> Piastre Fotosensibili -> SolarGate
    CHECK(world.current_zone == eco::ZoneId::DawnTerraces);
    CHECK(!world.zone_mechanics.solar_gate_open);
    CHECK(!world.zone_mechanics.solar_beams.empty());
    // Orienta specchio 0 a Sud (1), specchio 1 a Est (0), specchio 2 prima a Nord (3) poi a Sud (1)
    auto rotate_mirror_to = [&](int mirror_idx, int target_dir) {
        auto* p0 = world.players.GetPlayer(0);
        p0->pos = world.zone_mechanics.solar_mirrors[static_cast<std::size_t>(mirror_idx)].pos;
        for (int guard = 0; guard < 4; ++guard) {
            if (world.zone_mechanics.solar_mirrors[static_cast<std::size_t>(mirror_idx)].dir_index == target_dir) {
                break;
            }
            eco::AllPlayersInput in{};
            in.players[0].interact_pressed = true;
            const auto res = world.StepWorld(in, eco::kFixedDt);
            CHECK(res.mechanics_events.mirror_rotated);
        }
        CHECK(world.zone_mechanics.solar_mirrors[static_cast<std::size_t>(mirror_idx)].dir_index == target_dir);
    };
    rotate_mirror_to(0, 1); // Sud -> colpisce specchio 1
    rotate_mirror_to(1, 0); // Est -> colpisce specchio 2
    rotate_mirror_to(2, 3); // Nord -> carica piastra 0
    CHECK(world.zone_mechanics.solar_plates[0].charge >= 0.99f);
    rotate_mirror_to(2, 1); // Sud -> carica piastra 1 e apre SolarGate!
    CHECK(world.zone_mechanics.solar_plates[1].charge >= 0.99f);
    CHECK(world.zone_mechanics.solar_gate_open);

    // 2. Quartiere dei Canali: Correnti d'acqua fisiche + Leve Chiuse
    world.TeleportToZone(eco::ZoneId::CanalQuarter);
    auto* p0 = world.players.GetPlayer(0);
    p0->pos = {
        world.zone_mechanics.water_currents[0].bounds.Center().x - eco::kPlayerWidth * 0.5f,
        world.zone_mechanics.water_currents[0].bounds.Center().y - eco::kPlayerHeight * 0.5f
    };
    const float x_before_current = p0->pos.x;
    {
        eco::AllPlayersInput in{};
        for (int i = 0; i < 10; ++i) {
            (void)world.StepWorld(in, eco::kFixedDt);
        }
    }
    CHECK(p0->pos.x < x_before_current - 10.0f); // spinto dalla corrente verso Ovest!
    // Tira la leva 0 -> disattiva corrente 0
    p0->pos = world.zone_mechanics.sluice_levers[0].pos;
    {
        eco::AllPlayersInput in{};
        in.players[0].interact_pressed = true;
        const auto res = world.StepWorld(in, eco::kFixedDt);
        CHECK(res.mechanics_events.sluice_toggled);
    }
    CHECK(world.zone_mechanics.sluice_levers[0].pulled);
    CHECK(!world.zone_mechanics.water_currents[0].active);

    // 3. Altopiano dei Venti: Raffiche cicliche & riparo dietro Monoliti Frangivento
    world.TeleportToZone(eco::ZoneId::WindPlateau);
    p0 = world.players.GetPlayer(0);
    // Mettiamo il player allo scoperto sul viale centrale e portiamo il ciclo in raffica attiva
    p0->pos = world.overworld.tilemap.TileCenter(20, 17);
    world.zone_mechanics.wind.cycle_timer = 4.2f;
    const float x_exposed_before = p0->pos.x;
    {
        eco::AllPlayersInput in{};
        for (int i = 0; i < 12; ++i) {
            (void)world.StepWorld(in, eco::kFixedDt);
        }
    }
    CHECK(world.zone_mechanics.wind.gust_active);
    CHECK(!world.zone_mechanics.wind.player_sheltered);
    CHECK(p0->pos.x < x_exposed_before - 10.0f); // spinto dal vento!
    // Ripariamoci sottovento dietro il monolite 0 (wind_dir punta a Ovest -> ovest del monolite!)
    const auto& mono0 = world.zone_mechanics.windbreaks[0];
    p0->pos = {
        mono0.center.x - 64.0f - eco::kPlayerWidth * 0.5f,
        mono0.center.y - eco::kPlayerHeight * 0.5f
    };
    world.zone_mechanics.wind.cycle_timer = 4.2f;
    const float x_sheltered_before = p0->pos.x;
    {
        eco::AllPlayersInput in{};
        for (int i = 0; i < 10; ++i) {
            (void)world.StepWorld(in, eco::kFixedDt);
        }
    }
    CHECK(world.zone_mechanics.wind.player_sheltered);
    CHECK_NEAR(p0->pos.x, x_sheltered_before, 1.0f); // protetto dal monolite!

    // 4. Giardini Sospesi: Piastre di Risonanza Armonica -> Ponte di Giada
    world.TeleportToZone(eco::ZoneId::HangingGardens);
    p0 = world.players.GetPlayer(0);
    CHECK(!world.zone_mechanics.jade_bridge_unlocked);
    // Passo errato (pad 2 per primo) -> resonance_failed
    p0->pos = {
        world.zone_mechanics.resonance_pads[2].pos.x - eco::kPlayerWidth * 0.5f,
        world.zone_mechanics.resonance_pads[2].pos.y - eco::kPlayerHeight * 0.5f
    };
    {
        eco::AllPlayersInput in{};
        const auto res = world.StepWorld(in, eco::kFixedDt);
        CHECK(res.mechanics_events.resonance_failed);
        CHECK(world.zone_mechanics.resonance_next_order == 0);
    }
    // Sequenza corretta 0 -> 1 -> 2 -> 3
    for (int step = 0; step < 4; ++step) {
        const auto& pad = world.zone_mechanics.resonance_pads[static_cast<std::size_t>(step)];
        p0->pos = {pad.pos.x - eco::kPlayerWidth * 0.5f, pad.pos.y - eco::kPlayerHeight * 0.5f};
        eco::AllPlayersInput in{};
        const auto res = world.StepWorld(in, eco::kFixedDt);
        CHECK(res.mechanics_events.resonance_pad_hit);
        if (step == 3) {
            CHECK(res.mechanics_events.resonance_solved);
        }
    }
    CHECK(world.zone_mechanics.jade_bridge_unlocked);

    // 5. Caldera delle Fumarole: Geyser eruttivi + Valvole di Sfogo
    world.TeleportToZone(eco::ZoneId::FumaroleCaldera);
    p0 = world.players.GetPlayer(0);
    // Porta sim_time in fase Erupting per geyser 0 (cycle_offset=0, period=4.6 -> erupting in 3.1..4.6)
    world.sim_time = 3.5f;
    p0->pos = {
        world.zone_mechanics.geysers[0].pos.x + 8.0f - eco::kPlayerWidth * 0.5f,
        world.zone_mechanics.geysers[0].pos.y - eco::kPlayerHeight * 0.5f
    };
    {
        eco::AllPlayersInput in{};
        const auto res = world.StepWorld(in, eco::kFixedDt);
        CHECK(world.zone_mechanics.geysers[0].phase == eco::GeyserPhase::Erupting);
        CHECK(res.mechanics_events.geyser_pushed_player);
    }
    // Apri valvola 0 -> calma il gruppo 0
    p0->pos = world.zone_mechanics.relief_valves[0].pos;
    {
        eco::AllPlayersInput in{};
        in.players[0].interact_pressed = true;
        const auto res = world.StepWorld(in, eco::kFixedDt);
        CHECK(res.mechanics_events.valve_opened);
    }
    CHECK(world.zone_mechanics.relief_valves[0].open);
    CHECK(world.zone_mechanics.geysers[0].phase == eco::GeyserPhase::Calmed);

    // 6. Santuario dell'Armonia: 5 Sigilli Elementali
    world.TeleportToZone(eco::ZoneId::HarmonySanctuary);
    p0 = world.players.GetPlayer(0);
    CHECK(world.zone_mechanics.AttunedSealsCount() == 0);
    for (const auto& seal : world.zone_mechanics.sanctuary_seals) {
        p0->pos = {seal.pos.x - eco::kPlayerWidth * 0.5f, seal.pos.y - eco::kPlayerHeight * 0.5f};
        eco::AllPlayersInput in{};
        const auto res = world.StepWorld(in, eco::kFixedDt);
        CHECK(res.mechanics_events.sanctuary_seal_attuned);
    }
    CHECK(world.zone_mechanics.AllSealsAttuned());
}

void TestZoneInterludesAndMemorySwap() {
    eco::WorldContext world;
    world.EnsureInitialized();
    world.overworld.OnEnter(world.players, false, false);
    CHECK(world.current_zone == eco::ZoneId::DawnTerraces);
    CHECK(!world.IsInterludeActive());

    // 1. Verifica che il varco Nord di DawnTerraces sia SIGILLATO prima dell'attivazione dell'Eliostato
    world.overworld.gateway_cooldown_timer = 0.0f;
    world.overworld.gateway_reentry_locked = false;
    auto* p0 = world.players.GetPlayer(0);
    const auto& north_gw = world.overworld.active_gateways[0];
    CHECK(!world.IsZoneMainTaskCompleted(eco::ZoneId::DawnTerraces));
    CHECK(!world.IsZoneGatewayUnlocked(north_gw));
    p0->pos = {
        north_gw.world_center.x - eco::kPlayerWidth * 0.5f,
        north_gw.world_center.y - eco::kPlayerHeight * 0.5f
    };

    eco::AllPlayersInput in{};
    const auto blocked_res = world.StepWorld(in, eco::kFixedDt);
    CHECK(!blocked_res.interlude_started);
    CHECK(!world.IsInterludeActive());
    CHECK(world.current_zone == eco::ZoneId::DawnTerraces);
    CHECK(world.locked_gateway_hint_timer > 0.0f);

    // 2. Attiva l'Eliostato Solare -> il varco Nord si sblocca e avvia l'intramezzo verso CanalQuarter
    world.GetMachine(eco::MachineType::SolarArray).state = eco::MachineState::CleanActive;
    CHECK(world.IsZoneMainTaskCompleted(eco::ZoneId::DawnTerraces));
    CHECK(world.IsZoneGatewayUnlocked(north_gw));

    const auto start_res = world.StepWorld(in, eco::kFixedDt);
    CHECK(start_res.interlude_started);
    CHECK(world.IsInterludeActive());
    CHECK(world.interlude.from_zone == eco::ZoneId::DawnTerraces);
    CHECK(world.interlude.to_zone == eco::ZoneId::CanalQuarter);
    CHECK(!world.interlude.swap_committed);

    // Avanza fino al completamento dell'intramezzo: verifica swap memoria e cambio zona
    bool saw_zone_changed = false;
    bool saw_finished = false;
    for (int i = 0; i < 200 && world.IsInterludeActive(); ++i) {
        const auto r = world.StepWorld(in, eco::kFixedDt);
        if (r.zone_changed) {
            saw_zone_changed = true;
        }
        if (r.interlude_finished) {
            saw_finished = true;
        }
    }
    CHECK(saw_zone_changed);
    CHECK(saw_finished);
    CHECK(!world.IsInterludeActive());
    CHECK(world.current_zone == eco::ZoneId::CanalQuarter);
    CHECK(world.overworld.zone_swaps_count >= 1);

    // 3. Verifica la catena completa di sblocco dei varchi avanti (task principale) e ritorno sempre aperto
    auto check_zone_gateways = [&](eco::ZoneId zone, bool expect_forward_open) {
        world.TeleportToZone(zone);
        for (const auto& gw : world.overworld.active_gateways) {
            const bool is_forward = static_cast<int>(gw.to_zone) > static_cast<int>(gw.from_zone);
            if (is_forward) {
                CHECK(world.IsZoneGatewayUnlocked(gw) == expect_forward_open);
            } else {
                CHECK(world.IsZoneGatewayUnlocked(gw)); // Ritorno sempre libero!
            }
        }
    };

    // CanalQuarter -> WindPlateau: richiede HydroTurbine + Clara Sereni
    check_zone_gateways(eco::ZoneId::CanalQuarter, false);
    world.ecosystem.SetArchitectRecruited(true);
    check_zone_gateways(eco::ZoneId::CanalQuarter, false); // Turbina ancora guasta!
    world.GetMachine(eco::MachineType::HydroTurbine).state = eco::MachineState::CleanActive;
    check_zone_gateways(eco::ZoneId::CanalQuarter, true);

    // WindPlateau -> HangingGardens: richiede PendulumRegulator
    check_zone_gateways(eco::ZoneId::WindPlateau, false);
    world.GetMachine(eco::MachineType::PendulumRegulator).state = eco::MachineState::CleanActive;
    check_zone_gateways(eco::ZoneId::WindPlateau, true);

    // HangingGardens -> FumaroleCaldera: richiede HarmonicResonator
    check_zone_gateways(eco::ZoneId::HangingGardens, false);
    world.GetMachine(eco::MachineType::HarmonicResonator).state = eco::MachineState::CleanActive;
    check_zone_gateways(eco::ZoneId::HangingGardens, true);

    // FumaroleCaldera -> HarmonySanctuary: richiede ThermalExchanger
    check_zone_gateways(eco::ZoneId::FumaroleCaldera, false);
    world.GetMachine(eco::MachineType::ThermalExchanger).state = eco::MachineState::CleanActive;
    check_zone_gateways(eco::ZoneId::FumaroleCaldera, true);

    // HarmonySanctuary: solo varco di ritorno sempre aperto
    check_zone_gateways(eco::ZoneId::HarmonySanctuary, true);

    // Verifica SkipZoneInterlude() immediato
    world.BeginZoneInterlude(eco::ZoneId::WindPlateau, {12, 3});
    CHECK(world.IsInterludeActive());
    world.SkipZoneInterlude();
    CHECK(!world.IsInterludeActive());
    CHECK(world.current_zone == eco::ZoneId::WindPlateau);
    CHECK(world.overworld.zone_swaps_count >= 2);
}

void TestRedesignedPortalLogicLevelsPhysics() {
    // Helper BFS on any tilemap
    auto run_bfs = [](const eco::Tilemap& map, eco::Vec2 start_pos,
                      bool bridge_unlocked, bool solar_open, bool jade_open) -> std::vector<bool> {
        const int rows = map.Rows();
        const int cols = map.Cols();
        std::vector<bool> visited(static_cast<std::size_t>(rows * cols), false);
        const eco::TileCoord start = map.WorldToTile(start_pos);
        if (!map.InBounds(start.row, start.col) ||
            map.IsSolid(start.row, start.col, bridge_unlocked, solar_open, jade_open)) {
            return visited;
        }
        std::queue<eco::TileCoord> q;
        visited[static_cast<std::size_t>(start.row * cols + start.col)] = true;
        q.push(start);
        static constexpr int kDr[4] = {-1, 1, 0, 0};
        static constexpr int kDc[4] = {0, 0, -1, 1};
        while (!q.empty()) {
            const eco::TileCoord cur = q.front();
            q.pop();
            for (int d = 0; d < 4; ++d) {
                const int nr = cur.row + kDr[d];
                const int nc = cur.col + kDc[d];
                if (!map.InBounds(nr, nc) ||
                    map.IsSolid(nr, nc, bridge_unlocked, solar_open, jade_open)) {
                    continue;
                }
                const std::size_t idx = static_cast<std::size_t>(nr * cols + nc);
                if (!visited[idx]) {
                    visited[idx] = true;
                    q.push({nr, nc});
                }
            }
        }
        return visited;
    };

    auto is_reached = [](const eco::Tilemap& map, const std::vector<bool>& vis, eco::Vec2 pos) -> bool {
        const eco::TileCoord tc = map.WorldToTile(pos);
        if (!map.InBounds(tc.row, tc.col)) {
            return false;
        }
        return vis[static_cast<std::size_t>(tc.row * map.Cols() + tc.col)];
    };

    // ------------------------------------------------------------------------
    // 1. DawnTerraces SolarGate wall sealing: (1,28) and (24,28) solid;
    //    Dawn_FirstMeasureStele unreachable when closed, reachable when open.
    // ------------------------------------------------------------------------
    {
        const eco::Tilemap dawn = eco::Tilemap::CreateZoneMap(eco::ZoneId::DawnTerraces);
        CHECK(dawn.IsSolid(1, 28, true, false, true));
        CHECK(dawn.IsSolid(24, 28, true, false, true));

        eco::CollectibleRegistry reg;
        reg.ResetDefault();
        const auto* stele = reg.Find(eco::CollectibleId::Dawn_FirstMeasureStele);
        CHECK(stele != nullptr);
        CHECK(stele->zone == eco::ZoneId::DawnTerraces);

        const eco::TileCoord sp = eco::Tilemap::DefaultZoneSpawnTile(eco::ZoneId::DawnTerraces);
        const eco::Vec2 sp_pos = dawn.TileCenter(sp.row, sp.col);
        const std::vector<bool> vis_closed = run_bfs(dawn, sp_pos, true, false, true);
        const std::vector<bool> vis_open = run_bfs(dawn, sp_pos, true, true, true);
        CHECK(!is_reached(dawn, vis_closed, stele->pos));
        CHECK(is_reached(dawn, vis_open, stele->pos));
    }

    // ------------------------------------------------------------------------
    // 2. Strict 6-zone sequential topology, valid walkable arrival tiles,
    //    sequential CurrentObjectiveTarget() routing, and 16 collectibles non-solid.
    // ------------------------------------------------------------------------
    {
        auto has_target = [](const std::vector<eco::ZoneGateway>& gws, eco::ZoneId target) -> bool {
            for (const auto& gw : gws) {
                if (gw.to_zone == target) {
                    return true;
                }
            }
            return false;
        };

        const auto gw_dawn = eco::Tilemap::ZoneGateways(eco::ZoneId::DawnTerraces);
        CHECK(gw_dawn.size() == 1);
        CHECK(has_target(gw_dawn, eco::ZoneId::CanalQuarter));

        const auto gw_canal = eco::Tilemap::ZoneGateways(eco::ZoneId::CanalQuarter);
        CHECK(gw_canal.size() == 2);
        CHECK(has_target(gw_canal, eco::ZoneId::DawnTerraces));
        CHECK(has_target(gw_canal, eco::ZoneId::WindPlateau));

        const auto gw_wind = eco::Tilemap::ZoneGateways(eco::ZoneId::WindPlateau);
        CHECK(gw_wind.size() == 2);
        CHECK(has_target(gw_wind, eco::ZoneId::CanalQuarter));
        CHECK(has_target(gw_wind, eco::ZoneId::HangingGardens));

        const auto gw_gardens = eco::Tilemap::ZoneGateways(eco::ZoneId::HangingGardens);
        CHECK(gw_gardens.size() == 2);
        CHECK(has_target(gw_gardens, eco::ZoneId::WindPlateau));
        CHECK(has_target(gw_gardens, eco::ZoneId::FumaroleCaldera));

        const auto gw_caldera = eco::Tilemap::ZoneGateways(eco::ZoneId::FumaroleCaldera);
        CHECK(gw_caldera.size() == 2);
        CHECK(has_target(gw_caldera, eco::ZoneId::HangingGardens));
        CHECK(has_target(gw_caldera, eco::ZoneId::HarmonySanctuary));

        const auto gw_sanctuary = eco::Tilemap::ZoneGateways(eco::ZoneId::HarmonySanctuary);
        CHECK(gw_sanctuary.size() == 1);
        CHECK(has_target(gw_sanctuary, eco::ZoneId::FumaroleCaldera));

        // Verify every gateway arrival tile is walkable in its destination zone
        for (int z = 0; z < static_cast<int>(eco::ZoneId::Count); ++z) {
            const auto zid = static_cast<eco::ZoneId>(z);
            for (const auto& gw : eco::Tilemap::ZoneGateways(zid)) {
                const eco::Tilemap dest_map = eco::Tilemap::CreateZoneMap(gw.to_zone);
                CHECK(dest_map.InBounds(gw.arrival_spawn_tile.row, gw.arrival_spawn_tile.col));
                CHECK(!dest_map.IsSolid(gw.arrival_spawn_tile.row, gw.arrival_spawn_tile.col, true, false, false));
            }
        }

        // Verify sequential CurrentObjectiveTarget() routing across chain
        eco::WorldContext routing_world;
        routing_world.EnsureInitialized();
        routing_world.GetMachine(eco::MachineType::SolarArray).state = eco::MachineState::CleanActive;
        routing_world.GetNpc(eco::NpcId::ClaraSereni).recruited = true;
        routing_world.ecosystem.SetArchitectRecruited(true);
        routing_world.GetMachine(eco::MachineType::HydroTurbine).state = eco::MachineState::CleanActive;
        routing_world.GetMachine(eco::MachineType::PendulumRegulator).state = eco::MachineState::CleanActive;
        // Next objective is HarmonicResonator in HangingGardens; from DawnTerraces it must route Dawn -> Canal -> Wind -> Gardens
        auto find_gw_center = [](eco::ZoneId from, eco::ZoneId to) -> eco::Vec2 {
            for (const auto& gw : eco::Tilemap::ZoneGateways(from)) {
                if (gw.to_zone == to) {
                    return gw.world_center;
                }
            }
            return {-1.0f, -1.0f};
        };
        routing_world.TeleportToZone(eco::ZoneId::DawnTerraces);
        eco::ObjectiveTarget step1 = routing_world.CurrentObjectiveTarget();
        CHECK(step1.valid && step1.is_portal_step && step1.zone == eco::ZoneId::HangingGardens);
        CHECK(step1.world_pos == find_gw_center(eco::ZoneId::DawnTerraces, eco::ZoneId::CanalQuarter));
        routing_world.TeleportToZone(eco::ZoneId::CanalQuarter);
        eco::ObjectiveTarget step2 = routing_world.CurrentObjectiveTarget();
        CHECK(step2.valid && step2.is_portal_step && step2.zone == eco::ZoneId::HangingGardens);
        CHECK(step2.world_pos == find_gw_center(eco::ZoneId::CanalQuarter, eco::ZoneId::WindPlateau));
        routing_world.TeleportToZone(eco::ZoneId::WindPlateau);
        eco::ObjectiveTarget step3 = routing_world.CurrentObjectiveTarget();
        CHECK(step3.valid && step3.is_portal_step && step3.zone == eco::ZoneId::HangingGardens);
        CHECK(step3.world_pos == find_gw_center(eco::ZoneId::WindPlateau, eco::ZoneId::HangingGardens));
        routing_world.TeleportToZone(eco::ZoneId::HangingGardens);
        eco::ObjectiveTarget step4 = routing_world.CurrentObjectiveTarget();
        CHECK(step4.valid && !step4.is_portal_step && step4.zone == eco::ZoneId::HangingGardens);

        // Verify all 16 collectibles sit on non-solid tiles in their respective maps
        eco::CollectibleRegistry reg;
        reg.ResetDefault();
        CHECK(reg.Items().size() == 16);
        for (const auto& item : reg.Items()) {
            const eco::Tilemap map = (item.location == eco::LocationId::Overworld)
                ? eco::Tilemap::CreateZoneMap(item.zone)
                : eco::Tilemap::CreateStructureMap(eco::StructureFromLocation(item.location));
            const eco::TileCoord tc = map.WorldToTile(item.pos);
            CHECK(map.InBounds(tc.row, tc.col));
            CHECK(!map.IsSolid(tc.row, tc.col, true, true, true));
        }
    }

    // ------------------------------------------------------------------------
    // Real 60Hz side-scrolling physics driver helpers (NO teleporting!)
    // ------------------------------------------------------------------------
    auto tile_x = [](float col) -> float {
        return (col + 0.5f) * eco::kTileSize;
    };

    auto standing_row = [](const eco::Player& p) -> int {
        return static_cast<int>(std::floor((p.pos.y + p.size.y - 1.0f) / eco::kTileSize));
    };

    auto settle_grounded = [](eco::WorldContext& world, int max_ticks = 90) -> bool {
        eco::AllPlayersInput in{};
        for (int t = 0; t < max_ticks; ++t) {
            (void)world.StepWorld(in, eco::kFixedDt);
            if (world.players.GetPlayer(0)->isGrounded) {
                return true;
            }
        }
        return world.players.GetPlayer(0)->isGrounded;
    };

    auto walk_to_x = [&](eco::WorldContext& world, float target_x, int max_ticks = 420) -> bool {
        for (int t = 0; t < max_ticks; ++t) {
            const eco::Player* p0 = world.players.GetPlayer(0);
            const float dx = target_x - p0->Center().x;
            if (std::fabs(dx) <= 5.0f && p0->isGrounded) {
                return true;
            }
            eco::AllPlayersInput in{};
            if (std::fabs(dx) > 3.0f) {
                in.players[0].move_x = (dx > 0.0f) ? 1.0f : -1.0f;
            }
            (void)world.StepWorld(in, eco::kFixedDt);
        }
        const eco::Player* p0 = world.players.GetPlayer(0);
        return std::fabs(target_x - p0->Center().x) <= 10.0f && p0->isGrounded;
    };

    auto jump_to_ledge = [&](eco::WorldContext& world,
                             float takeoff_x,
                             float land_x,
                             int target_stand_row,
                             int max_ticks = 220) -> bool {
        if (!walk_to_x(world, takeoff_x, 420)) {
            return false;
        }
        const float dir = (land_x >= takeoff_x) ? 1.0f : -1.0f;
        bool jumped = false;
        for (int t = 0; t < max_ticks; ++t) {
            const eco::Player* p0 = world.players.GetPlayer(0);
            eco::AllPlayersInput in{};
            if (!jumped) {
                in.players[0].jump_pressed = true;
                in.players[0].jump_held = true;
                in.players[0].move_x = dir;
                jumped = true;
            } else {
                const float dx = land_x - p0->Center().x;
                if (std::fabs(dx) > 4.0f) {
                    in.players[0].move_x = (dx > 0.0f) ? 1.0f : -1.0f;
                }
                in.players[0].jump_held = (p0->y_velocity < 0.0f);
            }
            (void)world.StepWorld(in, eco::kFixedDt);
            p0 = world.players.GetPlayer(0);
            if (jumped && t >= 6 && p0->isGrounded) {
                if (standing_row(*p0) == target_stand_row &&
                    std::fabs(p0->Center().x - land_x) <= 96.0f) {
                    return true;
                }
                return false;
            }
        }
        return false;
    };

    auto drop_to_row = [&](eco::WorldContext& world,
                           float target_x,
                           int target_stand_row,
                           int max_ticks = 300) -> bool {
        for (int t = 0; t < max_ticks; ++t) {
            const eco::Player* p0 = world.players.GetPlayer(0);
            if (p0->isGrounded && standing_row(*p0) == target_stand_row &&
                std::fabs(p0->Center().x - target_x) <= 24.0f) {
                return true;
            }
            eco::AllPlayersInput in{};
            const float dx = target_x - p0->Center().x;
            if (std::fabs(dx) > 4.0f) {
                in.players[0].move_x = (dx > 0.0f) ? 1.0f : -1.0f;
            }
            (void)world.StepWorld(in, eco::kFixedDt);
        }
        const eco::Player* p0 = world.players.GetPlayer(0);
        return p0->isGrounded && standing_row(*p0) == target_stand_row;
    };

    auto press_interact = [](eco::WorldContext& world) -> eco::WorldStepResult {
        eco::AllPlayersInput in{};
        in.players[0].interact_pressed = true;
        return world.StepWorld(in, eco::kFixedDt);
    };

    // ------------------------------------------------------------------------
    // 3. REAL 60Hz PHYSICS PLAYTHROUGH: HypogeanCisterns (Cisterne Ipogee)
    // ------------------------------------------------------------------------
    {
        eco::WorldContext world;
        world.EnsureInitialized();
        world.EnterStructure(eco::StructureId::HypogeanCisterns);
        CHECK(settle_grounded(world));

        // Confirm HydroTurbine and LorenzoAltieri are NOT BFS-reachable initially
        {
            const eco::Tilemap& smap = world.structure.tilemap;
            const std::vector<bool> vis_init = run_bfs(
                smap, world.players.GetPlayer(0)->Center(), false, false, false);
            CHECK(!is_reached(smap, vis_init, world.GetMachine(eco::MachineType::HydroTurbine).pos));
            CHECK(!is_reached(smap, vis_init, world.GetNpc(eco::NpcId::LorenzoAltieri).pos));
        }

        // Spawn (17, 5) -> climb to Switch A (13, 5)
        CHECK(jump_to_ledge(world, tile_x(6.8f), tile_x(8.5f), 15));
        CHECK(jump_to_ledge(world, tile_x(8.3f), tile_x(6.2f), 13));
        CHECK(walk_to_x(world, tile_x(5.0f)));
        {
            const auto res = press_interact(world);
            CHECK(res.structure_puzzle_events.switch_toggled);
            CHECK(res.structure_puzzle_events.gate_opened);
            CHECK(world.structure.puzzle.GetSwitch(0)->active);
            CHECK(world.structure.puzzle.GetGate(0)->open);
            CHECK(!world.structure.puzzle.GetGate(1)->open);
            CHECK(!world.structure.puzzle.GetGate(2)->open);
        }

        // Drop to floor, walk through Gate 0, jump to Switch B (15, 22)
        CHECK(drop_to_row(world, tile_x(12.5f), 17));
        CHECK(walk_to_x(world, tile_x(18.8f)));
        CHECK(jump_to_ledge(world, tile_x(18.8f), tile_x(21.0f), 15));
        CHECK(walk_to_x(world, tile_x(22.0f)));
        {
            const auto res = press_interact(world);
            CHECK(res.structure_puzzle_events.switch_toggled);
            CHECK(world.structure.puzzle.GetSwitch(1)->active);
            CHECK(world.structure.puzzle.GetGate(1)->open); // Bridge 1 extends!
        }

        // Drop right to pick up Waters_SluiceStele at (17, 25)
        CHECK(drop_to_row(world, tile_x(25.0f), 17));
        CHECK(world.collectibles.IsCollected(eco::CollectibleId::Waters_SluiceStele));

        // Climb back up to Bridge 1 (row 10) and cross to Switch C (9, 6)
        CHECK(jump_to_ledge(world, tile_x(24.5f), tile_x(22.8f), 15));
        CHECK(jump_to_ledge(world, tile_x(22.9f), tile_x(24.8f), 13));
        CHECK(jump_to_ledge(world, tile_x(24.2f), tile_x(22.5f), 11));
        CHECK(jump_to_ledge(world, tile_x(20.2f), tile_x(18.5f), 9));
        CHECK(walk_to_x(world, tile_x(12.3f))); // Walk across Bridge 1 through archway
        CHECK(jump_to_ledge(world, tile_x(12.3f), tile_x(9.5f), 9)); // Jump 1-tile gap col 11
        CHECK(walk_to_x(world, tile_x(6.0f)));
        {
            const auto res = press_interact(world);
            CHECK(res.structure_puzzle_events.switch_toggled);
            CHECK(world.structure.puzzle.GetSwitch(2)->active);
            // With A=ON, B=ON, C=ON, Gate 2 MUST STILL BE CLOSED!
            CHECK(!world.structure.puzzle.GetGate(2)->open);
            CHECK(!world.structure.puzzle.solved);
        }

        // Walk left under upper-west ceiling arch to hidden Harmony_FarewellPrism at (9, 3)
        CHECK(walk_to_x(world, tile_x(3.0f)));
        CHECK(world.collectibles.IsCollected(eco::CollectibleId::Harmony_FarewellPrism));

        // Drop down via col 11 gap directly onto west ledge (row 15), jump to Switch A (13, 5), turn A OFF (A=OFF, B=ON, C=ON)
        CHECK(walk_to_x(world, tile_x(10.0f)));
        CHECK(drop_to_row(world, tile_x(11.0f), 15));
        CHECK(jump_to_ledge(world, tile_x(8.3f), tile_x(6.2f), 13));
        CHECK(walk_to_x(world, tile_x(5.0f)));
        {
            const auto res = press_interact(world);
            CHECK(res.structure_puzzle_events.switch_toggled);
            CHECK(!world.structure.puzzle.GetSwitch(0)->active);
            CHECK(world.structure.puzzle.GetGate(0)->open);  // Gate 0 stays open via C!
            CHECK(!world.structure.puzzle.GetGate(1)->open); // Bridge 1 retracts
            CHECK(world.structure.puzzle.GetGate(2)->open);  // Gate 2 opens!
            CHECK(res.structure_puzzle_events.puzzle_solved);
            CHECK(world.structure.puzzle.solved);
        }

        // Drive through Gate 0 and Gate 2 -> Lorenzo -> HydroTurbine -> ExitPortal
        CHECK(drop_to_row(world, tile_x(12.5f), 17));
        CHECK(walk_to_x(world, tile_x(30.0f)));
        CHECK(world.players.GetPlayer(0)->Center().DistanceTo(
                  world.GetNpc(eco::NpcId::LorenzoAltieri).pos) <= 48.0f);
        CHECK(jump_to_ledge(world, tile_x(31.8f), tile_x(33.5f), 16));
        CHECK(walk_to_x(world, tile_x(36.0f)));
        CHECK(world.players.GetPlayer(0)->Center().DistanceTo(
                  world.GetMachine(eco::MachineType::HydroTurbine).pos) <= 48.0f);

        // Return to ExitPortal at (17, 2..3)
        CHECK(drop_to_row(world, tile_x(31.0f), 17));
        world.structure.portal_cooldown_timer = 0.0f;
        world.structure.portal_reentry_locked = false;
        bool exited = false;
        for (int t = 0; t < 600 && !exited; ++t) {
            eco::AllPlayersInput in{};
            in.players[0].move_x = -1.0f;
            const auto res = world.StepWorld(in, eco::kFixedDt);
            if (res.exit_to_overworld_requested) {
                exited = true;
            }
        }
        CHECK(exited);
    }

    // ------------------------------------------------------------------------
    // 4. REAL 60Hz PHYSICS PLAYTHROUGH: PendulumTower (Torre del Pendolo)
    // ------------------------------------------------------------------------
    {
        eco::WorldContext world;
        world.EnsureInitialized();
        world.EnterStructure(eco::StructureId::PendulumTower);
        CHECK(settle_grounded(world));

        // Confirm PendulumRegulator is NOT BFS-reachable initially
        {
            const eco::Tilemap& smap = world.structure.tilemap;
            const std::vector<bool> vis_init = run_bfs(
                smap, world.players.GetPlayer(0)->Center(), false, false, false);
            CHECK(!is_reached(smap, vis_init, world.GetMachine(eco::MachineType::PendulumRegulator).pos));
        }

        // Climb Tier 1 -> Mid-Right Balcony (row 15) -> Tier 2 -> Mid-Left Gallery (row 11)
        // First test wrong-order logic: strike Chime II before Chime I!
        CHECK(jump_to_ledge(world, tile_x(10.8f), tile_x(13.0f), 17));
        CHECK(jump_to_ledge(world, tile_x(15.8f), tile_x(18.5f), 15));
        CHECK(jump_to_ledge(world, tile_x(21.8f), tile_x(24.5f), 14));
        // Skip Chime I for now and climb to Chime II (10, 5)
        CHECK(jump_to_ledge(world, tile_x(24.3f), tile_x(22.5f), 12));
        CHECK(jump_to_ledge(world, tile_x(20.2f), tile_x(16.8f), 11));
        CHECK(jump_to_ledge(world, tile_x(12.2f), tile_x(9.5f), 10));
        CHECK(walk_to_x(world, tile_x(5.0f)));
        {
            const auto res = press_interact(world);
            CHECK(res.structure_puzzle_events.sequence_failed);
            CHECK(world.structure.puzzle.current_sequence.empty());
        }

        // Now traverse back to Mid-Right Balcony and play the correct sequence: I -> II -> III -> IV
        CHECK(drop_to_row(world, tile_x(13.0f), 11));
        CHECK(jump_to_ledge(world, tile_x(16.8f), tile_x(20.5f), 12));
        CHECK(drop_to_row(world, tile_x(25.0f), 14));
        CHECK(walk_to_x(world, tile_x(31.0f)));
        {
            const auto res = press_interact(world); // Chime I
            CHECK(res.structure_puzzle_events.switch_toggled);
            CHECK(world.structure.puzzle.current_sequence.size() == 1);
        }

        // Climb to Chime II (10, 5)
        CHECK(jump_to_ledge(world, tile_x(24.3f), tile_x(22.5f), 12));
        CHECK(jump_to_ledge(world, tile_x(20.2f), tile_x(16.8f), 11));
        CHECK(jump_to_ledge(world, tile_x(12.2f), tile_x(9.5f), 10));
        CHECK(walk_to_x(world, tile_x(5.0f)));
        {
            const auto res = press_interact(world); // Chime II -> Bridge 0 extends!
            CHECK(res.structure_puzzle_events.switch_toggled);
            CHECK(res.structure_puzzle_events.gate_opened);
            CHECK(world.structure.puzzle.GetGate(0)->open);
            CHECK(!world.structure.puzzle.GetGate(1)->open);
        }

        // Climb to Step 5 (row 9, cols 11..13), cross Bridge 0 (row 9, cols 15..21),
        // land on (row 9, cols 23..26), climb to Upper-Right Belfry (row 7, cols 27..35)
        CHECK(jump_to_ledge(world, tile_x(9.9f), tile_x(11.8f), 8));
        CHECK(jump_to_ledge(world, tile_x(12.8f), tile_x(15.5f), 8));
        CHECK(jump_to_ledge(world, tile_x(20.8f), tile_x(23.5f), 8));
        CHECK(jump_to_ledge(world, tile_x(25.8f), tile_x(28.0f), 6));
        CHECK(walk_to_x(world, tile_x(30.0f)));
        {
            const auto res = press_interact(world); // Chime III -> Bridge 1 extends!
            CHECK(res.structure_puzzle_events.switch_toggled);
            CHECK(res.structure_puzzle_events.gate_opened);
            CHECK(world.structure.puzzle.GetGate(1)->open);
            CHECK(!world.structure.puzzle.GetGate(2)->open);
        }
        // Pick up Waters_ThreadChronometer at (6, 34)
        CHECK(walk_to_x(world, tile_x(34.0f)));
        CHECK(world.collectibles.IsCollected(eco::CollectibleId::Waters_ThreadChronometer));

        // Jump onto Bridge 1 (row 6, cols 18..25) -> Upper-Left Spire (row 5, cols 3..16)
        CHECK(jump_to_ledge(world, tile_x(27.2f), tile_x(24.5f), 5));
        CHECK(jump_to_ledge(world, tile_x(18.2f), tile_x(15.5f), 4));
        CHECK(walk_to_x(world, tile_x(9.0f)));
        {
            const auto res = press_interact(world); // Chime IV -> Gate 2 opens, puzzle_solved!
            CHECK(res.structure_puzzle_events.switch_toggled);
            CHECK(res.structure_puzzle_events.gate_opened);
            CHECK(res.structure_puzzle_events.puzzle_solved);
            CHECK(world.structure.puzzle.GetGate(2)->open);
            CHECK(world.structure.puzzle.solved);
        }
        // Pick up hidden Harmony_CurrentsPrism at (4, 3) under spire arch
        CHECK(walk_to_x(world, tile_x(3.0f)));
        CHECK(world.collectibles.IsCollected(eco::CollectibleId::Harmony_CurrentsPrism));

        // Drop down to ground floor (row 19) via Bridge 0 -> Step 4 -> shaft at col 11,
        // then walk through Gate 2 to PendulumRegulator (19, 33)
        CHECK(drop_to_row(world, tile_x(17.0f), 8, 300));
        CHECK(drop_to_row(world, tile_x(14.0f), 11, 300));
        CHECK(drop_to_row(world, tile_x(11.0f), 19, 300));
        CHECK(walk_to_x(world, tile_x(33.0f), 500));
        CHECK(world.players.GetPlayer(0)->Center().DistanceTo(
                  world.GetMachine(eco::MachineType::PendulumRegulator).pos) <= 48.0f);

        // Return to ExitPortal at (19, 2..3)
        world.structure.portal_cooldown_timer = 0.0f;
        world.structure.portal_reentry_locked = false;
        bool exited = false;
        for (int t = 0; t < 600 && !exited; ++t) {
            eco::AllPlayersInput in{};
            in.players[0].move_x = -1.0f;
            const auto res = world.StepWorld(in, eco::kFixedDt);
            if (res.exit_to_overworld_requested) {
                exited = true;
            }
        }
        CHECK(exited);
    }

    // ------------------------------------------------------------------------
    // 5. REAL 60Hz PHYSICS PLAYTHROUGH: GeothermalChamber (Camera Geotermica)
    // ------------------------------------------------------------------------
    {
        eco::WorldContext world;
        world.EnsureInitialized();
        world.EnterStructure(eco::StructureId::GeothermalChamber);
        CHECK(settle_grounded(world));

        // Confirm ThermalExchanger is NOT BFS-reachable initially
        {
            const eco::Tilemap& smap = world.structure.tilemap;
            const std::vector<bool> vis_init = run_bfs(
                smap, world.players.GetPlayer(0)->Center(), false, false, false);
            CHECK(!is_reached(smap, vis_init, world.GetMachine(eco::MachineType::ThermalExchanger).pos));
        }

        // Spawn (17, 5) -> jump to V1 (15, 7) -> [E] (Bridge 0 extends)
        CHECK(jump_to_ledge(world, tile_x(3.8f), tile_x(6.0f), 15));
        CHECK(walk_to_x(world, tile_x(7.0f)));
        {
            const auto res = press_interact(world); // V1 ON
            CHECK(res.structure_puzzle_events.switch_toggled);
            CHECK(res.structure_puzzle_events.gate_opened);
            CHECK(world.structure.puzzle.GetGate(0)->open); // Bridge 0 extends!
            CHECK(!world.structure.puzzle.GetGate(1)->open);
        }

        // Climb west ledges & cross Bridge 0 to V3 (9, 21) and Pitch_EmberStele (9, 24)
        CHECK(jump_to_ledge(world, tile_x(6.2f), tile_x(4.0f), 13));
        CHECK(jump_to_ledge(world, tile_x(4.8f), tile_x(6.8f), 11));
        CHECK(jump_to_ledge(world, tile_x(7.8f), tile_x(9.8f), 9));
        CHECK(jump_to_ledge(world, tile_x(10.8f), tile_x(13.5f), 9)); // onto Bridge 0
        CHECK(jump_to_ledge(world, tile_x(16.8f), tile_x(19.5f), 9)); // onto Upper-Center Ridge
        CHECK(walk_to_x(world, tile_x(21.0f)));
        {
            const auto res = press_interact(world); // V3 ON
            CHECK(res.structure_puzzle_events.switch_toggled);
            CHECK(world.structure.puzzle.GetSwitch(2)->active);
        }
        // Pick up hidden Pitch_EmberStele tucked against obsidian wall at (9, 25)
        CHECK(walk_to_x(world, tile_x(25.0f)));
        CHECK(world.collectibles.IsCollected(eco::CollectibleId::Pitch_EmberStele));

        // Drop to ground floor -> walk to V2 (17, 18) -> [E] (Bridge 0 retracts, Gate 1 opens)
        CHECK(drop_to_row(world, tile_x(18.0f), 17, 360));
        CHECK(walk_to_x(world, tile_x(18.0f)));
        {
            const auto res = press_interact(world); // V2 ON
            CHECK(res.structure_puzzle_events.switch_toggled);
            CHECK(!world.structure.puzzle.GetGate(0)->open); // Bridge 0 retracts
            CHECK(world.structure.puzzle.GetGate(1)->open);  // Gate 1 opens!
        }

        // Walk through Gate 1 to V4 (17, 37) -> [E] (Bridge 2 extends, Gate 3 opens, puzzle_solved!)
        CHECK(walk_to_x(world, tile_x(37.0f), 500));
        {
            const auto res = press_interact(world); // V4 ON
            CHECK(res.structure_puzzle_events.switch_toggled);
            CHECK(res.structure_puzzle_events.gate_opened);
            CHECK(res.structure_puzzle_events.puzzle_solved);
            CHECK(world.structure.puzzle.GetGate(2)->open); // Bridge 2 extends!
            CHECK(world.structure.puzzle.GetGate(3)->open); // Gate 3 opens!
            CHECK(world.structure.puzzle.solved);
        }

        // Climb east ledges & Bridge 2 through Gate 3 -> Pitch_BasaltThermometer (9, 36) & ThermalExchanger (9, 38)
        CHECK(jump_to_ledge(world, tile_x(33.8f), tile_x(36.0f), 15));
        CHECK(jump_to_ledge(world, tile_x(35.2f), tile_x(33.2f), 13));
        CHECK(jump_to_ledge(world, tile_x(30.2f), tile_x(27.8f), 11));
        CHECK(jump_to_ledge(world, tile_x(27.8f), tile_x(29.8f), 9));
        CHECK(walk_to_x(world, tile_x(36.0f)));
        CHECK(world.collectibles.IsCollected(eco::CollectibleId::Pitch_BasaltThermometer));
        CHECK(walk_to_x(world, tile_x(38.0f)));
        CHECK(world.players.GetPlayer(0)->Center().DistanceTo(
                  world.GetMachine(eco::MachineType::ThermalExchanger).pos) <= 48.0f);

        // Drop down and return to ExitPortal at (17, 2..3)
        CHECK(drop_to_row(world, tile_x(26.0f), 17, 400));
        world.structure.portal_cooldown_timer = 0.0f;
        world.structure.portal_reentry_locked = false;
        bool exited = false;
        for (int t = 0; t < 700 && !exited; ++t) {
            eco::AllPlayersInput in{};
            in.players[0].move_x = -1.0f;
            const auto res = world.StepWorld(in, eco::kFixedDt);
            if (res.exit_to_overworld_requested) {
                exited = true;
            }
        }
        CHECK(exited);
    }
}

void TestSprintMovement() {
    eco::WorldContext world;
    world.instant_zone_transitions = true;
    world.EnsureInitialized();
    world.overworld.OnEnter(
        world.players,
        false,
        world.ecosystem.architect_recruited,
        world.active_structure);

    const eco::Player* p0 = world.players.GetPlayer(0);
    CHECK(p0 != nullptr);
    const eco::Vec2 start_pos = p0->pos;

    eco::AllPlayersInput walk_in{};
    walk_in.players[0].move_x = 1.0f;
    (void)world.StepWorld(walk_in, eco::kFixedDt);
    const float walk_dx = p0->pos.x - start_pos.x;
    CHECK(std::abs(walk_dx - eco::kTopDownSpeed * eco::kFixedDt) < 1e-3f);

    const eco::Vec2 sprint_start = p0->pos;
    eco::AllPlayersInput sprint_in{};
    sprint_in.players[0].move_x = 1.0f;
    sprint_in.players[0].sprint_held = true;
    eco::AllPlayersInput latched{};
    eco::LatchFrameInput(latched, sprint_in);
    CHECK(latched.players[0].sprint_held);

    (void)world.StepWorld(latched, eco::kFixedDt);
    const float sprint_dx = p0->pos.x - sprint_start.x;
    CHECK(std::abs(sprint_dx - eco::kTopDownSpeed * eco::kSprintSpeedMultiplier * eco::kFixedDt) < 1e-3f);
    CHECK(sprint_dx > walk_dx);
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
    TestZoneBordersFlatMemoryAndGuidance();
    TestSixZoneTopDownMechanics();
    TestZoneInterludesAndMemorySwap();
    TestRedesignedPortalLogicLevelsPhysics();
    TestSprintMovement();

    if (g_failures != 0) {
        std::cerr << g_failures << " test check(s) failed.\n";
        return 1;
    }

    std::cout << "All La Conca di Meridia unit & integration tests passed.\n";
    return 0;
}
