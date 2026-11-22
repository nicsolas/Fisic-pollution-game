#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "app/game_state.h"
#include "app/game_state_manager.h"
#include "core/ecosystem.h"
#include "core/input_types.h"
#include "core/machine.h"
#include "core/math_types.h"
#include "core/npc.h"
#include "core/physics2d.h"
#include "core/player.h"
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
    CHECK(clamped_steps <= 15);

    stepper.Reset();
    const float nan_dt = std::numeric_limits<float>::quiet_NaN();
    const float inf_dt = std::numeric_limits<float>::infinity();
    CHECK(stepper.Accumulate(-0.1f) == 0);
    CHECK(stepper.Accumulate(nan_dt) == 0);
    CHECK_NEAR(stepper.accumulator, 0.0f, 1e-6f);

    // M1-CF3: max_frame_delta <= 0 / NaN / +Inf guards
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
    CHECK_NEAR(diag_with_drift.x, 0.70710678f, 1e-4f);
    CHECK_NEAR(diag_with_drift.y, 0.70710678f, 1e-4f);

    const eco::Vec2 doubled = eco::CombineKeyboardAndGamepadAxes({1.0f, 0.0f}, {1.0f, 0.0f});
    CHECK_NEAR(doubled.x, 1.0f, 1e-6f);
    CHECK_NEAR(doubled.y, 0.0f, 1e-6f);

    const eco::Vec2 partial_gp = eco::CombineKeyboardAndGamepadAxes({0.0f, 0.0f}, {0.5f, 0.0f});
    CHECK_NEAR(partial_gp.x, 0.5f, 1e-6f);
    CHECK_NEAR(partial_gp.y, 0.0f, 1e-6f);

    eco::AllPlayersInput all_in{};
    CHECK(!all_in.Get(-1).HasAnyAction());
    CHECK(!all_in.Get(4).HasAnyAction());
    CHECK(all_in.TryMutable(-1) == nullptr);
    CHECK(all_in.TryMutable(4) == nullptr);

    // Edge latch / consume behavior across 0-step and 2-step frames
    eco::AllPlayersInput pending{};
    eco::AllPlayersInput frame1{};
    frame1.players[0].move_x = 1.0f;
    frame1.players[0].jump_pressed = true;
    frame1.players[0].jump_held = true;
    frame1.toggle_state_pressed = true;
    eco::LatchFrameInput(pending, frame1);

    eco::AllPlayersInput frame2{};
    frame2.players[0].move_x = 0.5f;
    frame2.players[0].jump_pressed = false;
    frame2.players[0].jump_held = true;
    frame2.toggle_state_pressed = false;
    eco::LatchFrameInput(pending, frame2);

    // Latched edges survive frame2 (e.g. if frame1 had 0 fixed steps)
    CHECK(pending.players[0].jump_pressed);
    CHECK(pending.toggle_state_pressed);
    CHECK_NEAR(pending.players[0].move_x, 0.5f, 1e-6f);

    eco::ConsumePressedEdges(pending);
    CHECK(!pending.players[0].jump_pressed);
    CHECK(!pending.toggle_state_pressed);
    CHECK(pending.players[0].jump_held);
    CHECK_NEAR(pending.players[0].move_x, 0.5f, 1e-6f);
}

void TestPlayerAndManager() {
    const eco::Player p_low(-5);
    CHECK(p_low.player_id == 0);
    const eco::Player p_high(99);
    CHECK(p_high.player_id == 3);

    CHECK_NEAR(p_low.size.x, 30.0f, 1e-6f);
    CHECK_NEAR(p_low.size.y, 30.0f, 1e-6f);
    CHECK_NEAR(p_low.gravity_accel, 1200.0f, 1e-6f);
    CHECK_NEAR(p_low.jump_force, 480.0f, 1e-6f);
    CHECK(!p_low.isGrounded);
    CHECK_NEAR(p_low.coyote_timer, 0.0f, 1e-6f);
    CHECK_NEAR(p_low.jump_buffer_timer, 0.0f, 1e-6f);

    for (int i = 0; i < eco::kMaxPlayers; ++i) {
        const eco::ColorRgba ci = eco::ColorForPlayerId(i);
        CHECK(ci.a == 255);
        for (int j = i + 1; j < eco::kMaxPlayers; ++j) {
            CHECK(!(ci == eco::ColorForPlayerId(j)));
        }
    }

    eco::PlayerManager mgr;
    CHECK(mgr.ActiveCount() == 1);
    CHECK(mgr.GetPlayer(0)->active);
    CHECK(!mgr.GetPlayer(1)->active);
    CHECK(mgr.GetPlayer(-1) == nullptr);
    CHECK(mgr.GetPlayer(4) == nullptr);

    CHECK(!mgr.JoinPlayer(-1, {0.0f, 0.0f}));
    CHECK(!mgr.JoinPlayer(4, {0.0f, 0.0f}));
    CHECK(!mgr.JoinPlayer(0, {0.0f, 0.0f})); // Already active
    CHECK(mgr.JoinPlayer(1, {100.0f, 200.0f}));
    CHECK(!mgr.JoinPlayer(1, {100.0f, 200.0f})); // Duplicate join
    CHECK(mgr.ActiveCount() == 2);

    eco::AllPlayersInput drop_in{};
    drop_in.players[2].move_x = 1.0f;
    drop_in.players[3].join_pressed = true;
    mgr.ProcessDropIn(drop_in, {50.0f, 50.0f});
    CHECK(mgr.ActiveCount() == 4);

    CHECK(mgr.LeavePlayer(2));
    CHECK(mgr.LeavePlayer(3));
    CHECK(!mgr.LeavePlayer(2));
    CHECK(mgr.ActiveCount() == 2);

    mgr.GetPlayer(0)->pos = {0.0f, 0.0f};
    mgr.GetPlayer(1)->pos = {100.0f, 200.0f};
    const eco::Vec2 mid = mgr.AverageActivePosition();
    CHECK_NEAR(mid.x, 65.0f, 1e-4f);  // Center of {0,0} (15,15) and {100,200} (115,215)
    CHECK_NEAR(mid.y, 115.0f, 1e-4f);

    CHECK(mgr.LeavePlayer(0));
    CHECK(mgr.LeavePlayer(1));
    CHECK(mgr.ActiveCount() == 0);
    const eco::Vec2 zero_mid = mgr.AverageActivePosition();
    CHECK_NEAR(zero_mid.x, 0.0f, 1e-6f);
    CHECK_NEAR(zero_mid.y, 0.0f, 1e-6f);

    // M1-CF3: EnsureActive coverage
    eco::PlayerManager ensure_mgr;
    ensure_mgr.EnsureActive(2, {12.0f, 34.0f});
    CHECK(ensure_mgr.GetPlayer(2)->active);
    CHECK_NEAR(ensure_mgr.GetPlayer(2)->pos.x, 12.0f, 1e-6f);
    CHECK_NEAR(ensure_mgr.GetPlayer(2)->pos.y, 34.0f, 1e-6f);
    ensure_mgr.GetPlayer(2)->pos = {99.0f, 88.0f};
    ensure_mgr.EnsureActive(2, {0.0f, 0.0f}); // Already active: must not move
    CHECK_NEAR(ensure_mgr.GetPlayer(2)->pos.x, 99.0f, 1e-6f);
    CHECK_NEAR(ensure_mgr.GetPlayer(2)->pos.y, 88.0f, 1e-6f);
    ensure_mgr.EnsureActive(-1, {1.0f, 1.0f});
    ensure_mgr.EnsureActive(4, {1.0f, 1.0f});
    CHECK(ensure_mgr.ActiveCount() == 2);

    // M1-CF1: ProcessDropIn uses P0's pos instead of midpoint between separated P0/P1
    ensure_mgr.GetPlayer(0)->pos = {100.0f, 100.0f};
    ensure_mgr.EnsureActive(1, {500.0f, 100.0f});
    eco::AllPlayersInput p3_join{};
    p3_join.players[3].join_pressed = true;
    ensure_mgr.ProcessDropIn(p3_join, {0.0f, 0.0f});
    CHECK(ensure_mgr.GetPlayer(3)->active);
    CHECK_NEAR(ensure_mgr.GetPlayer(3)->pos.x, 100.0f, 1e-6f);
    CHECK_NEAR(ensure_mgr.GetPlayer(3)->pos.y, 100.0f, 1e-6f);
}

void TestEcoSystem() {
    eco::EcoSystem eco;
    CHECK_NEAR(eco.pollution_level, 35.0f, 1e-6f);
    CHECK_NEAR(eco.clean_energy, 0.0f, 1e-6f);
    CHECK_NEAR(eco.dirty_energy, 0.0f, 1e-6f);
    CHECK(eco.ui_phase == eco::UiPhase::Phase1_Minimal);
    CHECK(!eco.architect_recruited);

    eco.ApplyCleanActivation(30.0f, 15.0f);
    CHECK_NEAR(eco.clean_energy, 30.0f, 1e-6f);
    CHECK_NEAR(eco.pollution_level, 20.0f, 1e-6f);
    CHECK(eco.ui_phase == eco::UiPhase::Phase2_Telemetry);

    eco.ApplyCleanActivation(500.0f, 500.0f);
    CHECK_NEAR(eco.clean_energy, 100.0f, 1e-6f);
    CHECK_NEAR(eco.pollution_level, 0.0f, 1e-6f);

    eco.Reset();
    eco.ApplyDirtyBypass(40.0f, 25.0f);
    CHECK_NEAR(eco.dirty_energy, 40.0f, 1e-6f);
    CHECK_NEAR(eco.clean_energy, 0.0f, 1e-6f);
    CHECK_NEAR(eco.pollution_level, 60.0f, 1e-6f);
    CHECK(eco.ui_phase == eco::UiPhase::Phase1_Minimal);
    CHECK(!eco.architect_recruited);

    eco.ApplyDirtyBypass(500.0f, 500.0f);
    CHECK_NEAR(eco.dirty_energy, 100.0f, 1e-6f);
    CHECK_NEAR(eco.pollution_level, 100.0f, 1e-6f);

    const float nan_val = std::numeric_limits<float>::quiet_NaN();
    eco.ApplyCleanActivation(-10.0f, nan_val);
    eco.ApplyDirtyBypass(nan_val, -20.0f);
    CHECK_NEAR(eco.clean_energy, 0.0f, 1e-6f);
    CHECK_NEAR(eco.pollution_level, 100.0f, 1e-6f);

    eco.SetArchitectRecruited(true);
    CHECK(eco.architect_recruited);
    CHECK(eco.ui_phase == eco::UiPhase::Phase3_EngineOverlay);

    // M1-CF3: MaxUnlockedUiPhase, CycleUiPhase, SetArchitectRecruited(false), TotalEnergy
    eco.Reset();
    CHECK(eco.MaxUnlockedUiPhase() == eco::UiPhase::Phase1_Minimal);
    eco.CycleUiPhase();
    CHECK(eco.ui_phase == eco::UiPhase::Phase1_Minimal);
    CHECK_NEAR(eco.TotalEnergy(), 0.0f, 1e-6f);

    eco.ApplyCleanActivation(25.0f, 10.0f);
    eco.ApplyDirtyBypass(15.0f, 5.0f);
    CHECK_NEAR(eco.TotalEnergy(), 40.0f, 1e-6f);
    CHECK(eco.MaxUnlockedUiPhase() == eco::UiPhase::Phase2_Telemetry);
    CHECK(eco.ui_phase == eco::UiPhase::Phase2_Telemetry);
    eco.CycleUiPhase();
    CHECK(eco.ui_phase == eco::UiPhase::Phase1_Minimal);
    eco.CycleUiPhase();
    CHECK(eco.ui_phase == eco::UiPhase::Phase2_Telemetry);

    eco.SetArchitectRecruited(true);
    CHECK(eco.MaxUnlockedUiPhase() == eco::UiPhase::Phase3_EngineOverlay);
    eco.CycleUiPhase();
    CHECK(eco.ui_phase == eco::UiPhase::Phase1_Minimal);
    eco.CycleUiPhase();
    CHECK(eco.ui_phase == eco::UiPhase::Phase2_Telemetry);
    eco.CycleUiPhase();
    CHECK(eco.ui_phase == eco::UiPhase::Phase3_EngineOverlay);

    eco.SetArchitectRecruited(false);
    CHECK(!eco.architect_recruited);
    CHECK(eco.MaxUnlockedUiPhase() == eco::UiPhase::Phase2_Telemetry);
    CHECK(eco.ui_phase == eco::UiPhase::Phase2_Telemetry);

    eco.Reset();
    eco.SetArchitectRecruited(true);
    CHECK(eco.ui_phase == eco::UiPhase::Phase3_EngineOverlay);
    eco.SetArchitectRecruited(false);
    CHECK(eco.MaxUnlockedUiPhase() == eco::UiPhase::Phase1_Minimal);
    CHECK(eco.ui_phase == eco::UiPhase::Phase1_Minimal);

    // Visual tinting and smog alpha checks
    eco.Reset();
    const eco::ColorRgba base{60, 200, 100, 210};

    eco.pollution_level = 0.0f;
    CHECK(eco.TintColor(base) == base);
    CHECK_NEAR(eco.SmogAlpha(), 0.0f, 1e-6f);

    eco.pollution_level = 35.0f;
    const eco::ColorRgba t35 = eco.TintColor(base);
    CHECK(t35.a == 210);
    CHECK_NEAR(eco.SmogAlpha(), 0.1575f, 1e-5f);

    eco.pollution_level = 70.0f;
    const eco::ColorRgba t70 = eco.TintColor(base);

    eco.pollution_level = 100.0f;
    const eco::ColorRgba t100 = eco.TintColor(base);
    CHECK(t100.a == 210);
    CHECK_NEAR(eco.SmogAlpha(), 0.45f, 1e-5f);

    auto luma = [](eco::ColorRgba c) {
        return 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
    };
    auto chroma = [](eco::ColorRgba c) {
        const int mx = std::max({c.r, c.g, c.b});
        const int mn = std::min({c.r, c.g, c.b});
        return mx - mn;
    };

    CHECK(luma(base) > luma(t35));
    CHECK(luma(t35) > luma(t70));
    CHECK(luma(t70) > luma(t100));

    CHECK(chroma(base) > chroma(t35));
    CHECK(chroma(t35) > chroma(t70));
    CHECK(chroma(t70) > chroma(t100));
}

class MockStateB final : public eco::app::GameState {
public:
    MockStateB(std::vector<std::string>& log,
               eco::app::GameStateManager* mgr = nullptr,
               bool chain_on_enter = false)
        : log_(log), mgr_(mgr), chain_on_enter_(chain_on_enter) {}

    ~MockStateB() override { log_.push_back("~StateB"); }

    void OnEnter() override {
        log_.push_back("B::OnEnter");
        if (chain_on_enter_ && mgr_ != nullptr) {
            mgr_->RequestTransition([this]() {
                return std::make_unique<MockStateB>(log_, nullptr, false);
            });
        }
    }
    void OnExit() override { log_.push_back("B::OnExit"); }
    void Update(float) override {}
    void Draw() override {}
    [[nodiscard]] const char* Name() const noexcept override { return "MockStateB"; }

private:
    std::vector<std::string>& log_;
    eco::app::GameStateManager* mgr_;
    bool chain_on_enter_;
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
        // Critical UAF guard: *this must still be alive after RequestTransition!
        post_request_counter_ += 1;
        if (post_request_counter_ == 1) {
            log_.push_back("A::Update_end");
        }
    }

    void Draw() override {}
    [[nodiscard]] const char* Name() const noexcept override { return "MockStateA"; }

private:
    eco::app::GameStateManager& mgr_;
    std::vector<std::string>& log_;
    int post_request_counter_ = 0;
};

void TestGameStateManager() {
    eco::app::GameStateManager empty_mgr;
    empty_mgr.Update(eco::kFixedDt);
    empty_mgr.Draw();
    CHECK(!empty_mgr.ApplyPendingTransition());
    CHECK(!empty_mgr.HasActiveState());

    // UAF-safe deferred transition test
    {
        std::vector<std::string> log;
        eco::app::GameStateManager mgr;
        mgr.RequestTransition(std::make_unique<MockStateA>(mgr, log));
        CHECK(mgr.HasPendingTransition());
        CHECK(mgr.ApplyPendingTransition());
        CHECK(mgr.HasActiveState());
        CHECK(std::string(mgr.CurrentState()->Name()) == "MockStateA");

        mgr.Update(eco::kFixedDt);
        CHECK(std::string(mgr.CurrentState()->Name()) == "MockStateB");

        const std::vector<std::string> expected{
            "A::OnEnter", "A::Update_end", "A::OnExit", "~StateA", "B::OnEnter"
        };
        CHECK(log == expected);
    }

    // Null state / empty factory / null-returning factory guards
    {
        std::vector<std::string> log;
        eco::app::GameStateManager mgr;
        mgr.RequestTransition(std::make_unique<MockStateB>(log));
        CHECK(mgr.ApplyPendingTransition());
        auto* original_ptr = mgr.CurrentState();

        mgr.RequestTransition(std::unique_ptr<eco::app::GameState>{nullptr});
        CHECK(!mgr.HasPendingTransition());

        mgr.RequestTransition(eco::app::GameStateManager::StateFactory{});
        CHECK(!mgr.HasPendingTransition());

        mgr.RequestTransition([]() -> std::unique_ptr<eco::app::GameState> { return nullptr; });
        CHECK(!mgr.ApplyPendingTransition());
        CHECK(mgr.CurrentState() == original_ptr);
    }

    // Re-entrant transition requested inside OnEnter()
    {
        std::vector<std::string> log;
        {
            eco::app::GameStateManager mgr;
            mgr.RequestTransition(std::make_unique<MockStateB>(log, &mgr, true));
            CHECK(mgr.ApplyPendingTransition());
            // Chained transition must wait until next ApplyPendingTransition() / Update()
            CHECK(mgr.HasPendingTransition());
            CHECK(mgr.ApplyPendingTransition());
            CHECK(!mgr.HasPendingTransition());
            const std::vector<std::string> expected_mid{
                "B::OnEnter", "B::OnExit", "~StateB", "B::OnEnter"};
            CHECK(log == expected_mid);
        }
        // ~GameStateManager() calls OnExit() and destroys final state
        const std::vector<std::string> expected_end{
            "B::OnEnter", "B::OnExit", "~StateB", "B::OnEnter", "B::OnExit", "~StateB"};
        CHECK(log == expected_end);
    }
}

void TestTilemap() {
    const eco::Tilemap map = eco::Tilemap::CreateOverworldMap();
    CHECK(map.Rows() == 18);
    CHECK(map.Cols() == 28);
    CHECK_NEAR(map.TileSize(), 48.0f, 1e-6f);
    CHECK_NEAR(map.WorldWidth(), 28.0f * 48.0f, 1e-4f);
    CHECK_NEAR(map.WorldHeight(), 18.0f * 48.0f, 1e-4f);

    // Perimeter walls & OOB fallback
    CHECK(map.GetTile(0, 0) == eco::TileId::Wall);
    CHECK(map.GetTile(17, 27) == eco::TileId::Wall);
    CHECK(map.GetTile(-1, 0) == eco::TileId::Wall);
    CHECK(map.GetTile(0, -1) == eco::TileId::Wall);
    CHECK(map.GetTile(18, 0) == eco::TileId::Wall);
    CHECK(map.GetTile(0, 28) == eco::TileId::Wall);
    CHECK(map.IsSolid(-1, -1));
    CHECK(map.IsSolid(18, 28));

    // Portals and HiddenBridge exist in Overworld map
    const auto portals = map.FindTiles(eco::TileId::StructurePortal);
    const auto bridges = map.FindTiles(eco::TileId::HiddenBridge);
    CHECK(!portals.empty());
    CHECK(!bridges.empty());

    for (const auto& br : bridges) {
        CHECK(map.IsSolid(br.row, br.col, false));
        CHECK(!map.IsSolid(br.row, br.col, true));
    }
    for (const auto& pt : portals) {
        CHECK(!map.IsSolid(pt.row, pt.col, false));
    }

    // WorldToTile with negative coords, boundaries, and non-finite coords
    CHECK((map.WorldToTile({0.0f, 0.0f}) == eco::TileCoord{0, 0}));
    CHECK((map.WorldToTile({48.0f, 48.0f}) == eco::TileCoord{1, 1}));
    CHECK((map.WorldToTile({-1.0f, -1.0f}) == eco::TileCoord{-1, -1}));
    const float nan_val = std::numeric_limits<float>::quiet_NaN();
    const float inf_val = std::numeric_limits<float>::infinity();
    CHECK((map.WorldToTile({nan_val, 0.0f}) == eco::TileCoord{-1, -1}));
    CHECK((map.WorldToTile({0.0f, inf_val}) == eco::TileCoord{-1, -1}));

    // Custom tilemap: SetTile, ExitPortal, unknown int -> Wall, OverlapsSolid strict edge
    eco::Tilemap custom({
        {0, 0, 1},
        {2, 3, 99},
        {0, 4, 0}
    }, 48.0f);
    CHECK(custom.GetTile(1, 0) == eco::TileId::StructurePortal);
    CHECK(custom.GetTile(1, 1) == eco::TileId::ExitPortal);
    CHECK(custom.GetTile(1, 2) == eco::TileId::Wall); // unknown int -> Wall
    CHECK(!custom.IsSolid(1, 0));
    CHECK(!custom.IsSolid(1, 1));
    CHECK(custom.IsSolid(1, 2));
    CHECK(custom.IsSolid(2, 1, false));
    CHECK(!custom.IsSolid(2, 1, true));
    CHECK(!custom.SetTile(-1, 0, eco::TileId::Walkable));
    CHECK(custom.SetTile(0, 2, eco::TileId::Walkable));
    CHECK(!custom.IsSolid(0, 2));
    CHECK(custom.SetTile(0, 2, eco::TileId::Wall));

    // Strict edge contact with wall at (row 0, col 2) -> x in [96, 144)
    const eco::Rect flush_left{66.0f, 0.0f, 30.0f, 30.0f}; // Right == 96.0f
    const eco::Rect overlap_left{67.0f, 0.0f, 30.0f, 30.0f};
    const eco::Rect oob_left{-1.0f, 0.0f, 30.0f, 30.0f};
    CHECK(!custom.OverlapsSolid(flush_left));
    CHECK(custom.OverlapsSolid(overlap_left));
    CHECK(custom.OverlapsSolid(oob_left));
    CHECK(custom.OverlapsTileId({0.0f, 40.0f, 30.0f, 30.0f}, eco::TileId::StructurePortal));
    CHECK(!custom.OverlapsTileId({0.0f, 0.0f, 30.0f, 30.0f}, eco::TileId::StructurePortal));
}

void TestPhysics2D() {
    const float nan_val = std::numeric_limits<float>::quiet_NaN();
    const float inf_val = std::numeric_limits<float>::infinity();

    // Normalization zero/epsilon/NaN/Inf/overflow safety
    CHECK((eco::NormalizeVector({0.0f, 0.0f}) == eco::Vec2{0.0f, 0.0f}));
    CHECK((eco::NormalizeVector({1e-9f, -1e-9f}) == eco::Vec2{0.0f, 0.0f}));
    CHECK((eco::NormalizeVector({nan_val, 1.0f}) == eco::Vec2{0.0f, 0.0f}));
    CHECK((eco::NormalizeVector({inf_val, 1.0f}) == eco::Vec2{0.0f, 0.0f}));

    const eco::Vec2 huge = eco::NormalizeVector({1e30f, 1e30f});
    CHECK_NEAR(huge.Length(), 1.0f, 1e-4f);

    const eco::Vec2 diag_vel = eco::ComputeTopDownVelocity({1.0f, 1.0f});
    const eco::Vec2 card_vel = eco::ComputeTopDownVelocity({1.0f, 0.0f});
    CHECK_NEAR(diag_vel.Length(), eco::kTopDownSpeed, 1e-4f);
    CHECK_NEAR(card_vel.Length(), eco::kTopDownSpeed, 1e-4f);

    // Open-arena cardinal vs diagonal travel distance equality
    eco::Tilemap open_arena({
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    }, 48.0f);

    eco::Player p_card(0);
    p_card.pos = {50.0f, 50.0f};
    eco::Player p_diag(1);
    p_diag.pos = {50.0f, 50.0f};

    for (int i = 0; i < 10; ++i) {
        eco::MovePlayerTopDown(p_card, {1.0f, 0.0f}, eco::kFixedDt, open_arena);
        eco::MovePlayerTopDown(p_diag, {1.0f, 1.0f}, eco::kFixedDt, open_arena);
    }
    const float expected_dist = 10.0f * eco::kTopDownSpeed * eco::kFixedDt;
    CHECK_NEAR((p_card.pos - eco::Vec2{50.0f, 50.0f}).Length(), expected_dist, 1e-3f);
    CHECK_NEAR((p_diag.pos - eco::Vec2{50.0f, 50.0f}).Length(), expected_dist, 1e-3f);

    // Axis-separated AABB slide collision + multi-tile corner snag resistance
    eco::Tilemap wall_map({
        {1, 1, 1, 1, 1, 1},
        {1, 0, 0, 1, 0, 1},
        {1, 0, 0, 1, 0, 1},
        {1, 0, 0, 1, 0, 1},
        {1, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1}
    }, 48.0f);

    // Vertical wall at col 3 (x = 144.0f), rows 1..3.
    // Slide down-right across the row 1->2 and 2->3 tile boundaries with fractional player size.
    eco::Player p_slide(0);
    p_slide.size = {29.3f, 29.3f};
    p_slide.pos = {144.0f - p_slide.size.x - 1.0f, 52.0f};
    const float y_before = p_slide.pos.y;
    for (int i = 0; i < 25; ++i) {
        eco::MovePlayerTopDown(p_slide, {1.0f, 1.0f}, eco::kFixedDt, wall_map);
        CHECK(!wall_map.OverlapsSolid(p_slide.Bounds()));
    }
    CHECK_NEAR(p_slide.vel.x, 0.0f, 1e-5f);
    CHECK(p_slide.vel.y > 0.0f);
    CHECK(p_slide.pos.y > y_before + 48.0f); // Crossed tile seam without snagging

    // Horizontal wall impact: Y stops flush and vel.y == 0 while X slides
    eco::Player p_hslide(0);
    p_hslide.pos = {52.0f, 49.0f};
    const eco::SlideMoveResult h_res =
        eco::MovePlayerTopDown(p_hslide, {1.0f, -1.0f}, eco::kFixedDt, wall_map);
    CHECK(h_res.hit_y);
    CHECK(!h_res.hit_x);
    CHECK_NEAR(p_hslide.pos.y, 48.0f, 1e-5f);
    CHECK_NEAR(p_hslide.vel.y, 0.0f, 1e-5f);
    CHECK(p_hslide.vel.x > 0.0f);
    CHECK(!wall_map.OverlapsSolid(p_hslide.Bounds()));

    // 1-tile (48px) corridor traversal for 30x30 player
    eco::Tilemap corridor({
        {1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1},
        {1, 1, 1, 1, 1}
    }, 48.0f);
    eco::Player p_corr(0);
    p_corr.pos = {52.0f, 57.0f};
    for (int i = 0; i < 15; ++i) {
        eco::MovePlayerTopDown(p_corr, {1.0f, 0.0f}, eco::kFixedDt, corridor);
        CHECK(!corridor.OverlapsSolid(p_corr.Bounds()));
    }
    CHECK(p_corr.pos.x > 100.0f);

    // Sub-stepping tunnel safety: high dt must not pass through 1-tile wall
    const eco::SlideMoveResult tunnel_res = eco::MoveAndSlideTopDown(
        {52.0f, 57.0f}, {30.0f, 30.0f}, {600.0f, 0.0f}, 0.5f, wall_map);
    CHECK(tunnel_res.hit_x);
    CHECK(tunnel_res.position.x <= 144.0f - 30.0f + 1e-4f);
    CHECK(!wall_map.OverlapsSolid({tunnel_res.position.x, tunnel_res.position.y, 30.0f, 30.0f}));

    // dt <= 0 and NaN dt leave position unchanged and finite
    const eco::Vec2 frozen_pos{70.0f, 70.0f};
    const eco::SlideMoveResult dt0 = eco::MoveAndSlideTopDown(
        frozen_pos, {30.0f, 30.0f}, {220.0f, 220.0f}, 0.0f, wall_map);
    const eco::SlideMoveResult dt_nan = eco::MoveAndSlideTopDown(
        frozen_pos, {30.0f, 30.0f}, {220.0f, 220.0f}, nan_val, wall_map);
    CHECK(dt0.position == frozen_pos);
    CHECK(dt_nan.position == frozen_pos);
}

void TestAverageCameraController() {
    eco::PlayerManager mgr;
    CHECK(mgr.LeavePlayer(0));
    CHECK(mgr.ActiveCount() == 0);

    // 0 active players: preserves non-origin current_target (never snaps to {0,0})
    const eco::Vec2 cam_init{350.0f, 240.0f};
    const eco::Vec2 cam_zero = eco::UpdateAverageCameraTarget(cam_init, mgr, eco::kFixedDt);
    CHECK(cam_zero == cam_init);

    // Non-finite / non-positive dt & lerp_speed guards
    const float nan_val = std::numeric_limits<float>::quiet_NaN();
    CHECK(eco::UpdateAverageCameraTarget(cam_init, mgr, nan_val) == cam_init);
    CHECK(eco::LerpCameraTarget(cam_init, {500.0f, 500.0f}, 0.0f) == cam_init);
    CHECK(eco::LerpCameraTarget(cam_init, {500.0f, 500.0f}, eco::kFixedDt, -1.0f) == cam_init);
    CHECK((eco::LerpCameraTarget({nan_val, 0.0f}, {500.0f, 400.0f}, eco::kFixedDt) ==
           eco::Vec2{500.0f, 400.0f}));

    // 1 active player monotonic convergence
    mgr.EnsureActive(0, {400.0f, 300.0f});
    eco::Vec2 cam{100.0f, 100.0f};
    const eco::Vec2 target1 = mgr.AverageActivePosition();
    float prev_dist = cam.DistanceTo(target1);
    for (int i = 0; i < 20; ++i) {
        cam = eco::UpdateAverageCameraTarget(cam, mgr, eco::kFixedDt);
        const float dist = cam.DistanceTo(target1);
        CHECK(dist < prev_dist);
        prev_dist = dist;
    }

    // 2..4 active players midpoint + smooth drop-in shift
    mgr.EnsureActive(2, {800.0f, 500.0f});
    const eco::Vec2 mid2 = mgr.AverageActivePosition();
    const float before_drop = cam.DistanceTo(mid2);
    const eco::Vec2 after_one = eco::UpdateAverageCameraTarget(cam, mgr, eco::kFixedDt);
    const float after_drop = after_one.DistanceTo(mid2);
    CHECK(after_drop > 0.0f);
    CHECK(after_drop < before_drop);
}

void TestOverworldSim() {
    eco::WorldContext world;
    world.overworld.OnEnter(world.players, false, world.ecosystem.architect_recruited);

    const eco::Player* p0 = world.players.GetPlayer(0);
    CHECK(p0 != nullptr && p0->active);
    CHECK(!world.overworld.tilemap.OverlapsSolid(p0->Bounds(), false));
    CHECK(!world.overworld.tilemap.OverlapsTileId(p0->Bounds(), eco::TileId::StructurePortal));
    CHECK_NEAR(world.overworld.camera_target.x, p0->Center().x, 1e-4f);
    CHECK_NEAR(world.overworld.camera_target.y, p0->Center().y, 1e-4f);

    // Move P0 away from spawn, then mid-game drop-in P1/P2 must NOT teleport P0 (M1-CF1)
    eco::AllPlayersInput move_in{};
    move_in.players[0].move_x = 1.0f;
    for (int i = 0; i < 15; ++i) {
        const auto res = world.overworld.Step(world.players, move_in, eco::kFixedDt, false);
        CHECK(!res.enter_structure_requested);
    }
    const eco::Vec2 p0_moved = p0->pos;
    CHECK(p0_moved.x > world.overworld.spawn_point.x + 20.0f);

    eco::AllPlayersInput join_in{};
    join_in.players[1].join_pressed = true;
    join_in.players[2].move_y = 1.0f;
    const auto join_res = world.overworld.Step(world.players, join_in, eco::kFixedDt, false);
    CHECK(join_res.newly_joined_count == 2);
    CHECK(world.players.ActiveCount() == 3);
    CHECK_NEAR(p0->pos.x, p0_moved.x, 1e-5f);
    CHECK_NEAR(p0->pos.y, p0_moved.y, 1e-5f);
    CHECK(!world.overworld.tilemap.OverlapsSolid(world.players.GetPlayer(1)->Bounds(), false));
    CHECK(!world.overworld.tilemap.OverlapsSolid(world.players.GetPlayer(2)->Bounds(), false));

    // HiddenBridge gating: blocked when bridge_unlocked == false, passable when true
    const auto bridges = world.overworld.tilemap.FindTiles(eco::TileId::HiddenBridge);
    CHECK(!bridges.empty());
    const eco::Vec2 bridge_center =
        world.overworld.tilemap.TileCenter(bridges[0].row, bridges[0].col);
    eco::Player* mut_p0 = world.players.GetPlayer(0);
    mut_p0->pos = {
        bridge_center.x - eco::kTileSize,
        bridge_center.y - mut_p0->size.y * 0.5f
    };
    eco::AllPlayersInput right_in{};
    right_in.players[0].move_x = 1.0f;
    for (int i = 0; i < 20; ++i) {
        (void)world.overworld.Step(world.players, right_in, eco::kFixedDt, false);
    }
    CHECK(mut_p0->pos.x + mut_p0->size.x <=
          world.overworld.tilemap.TileRect(bridges[0].row, bridges[0].col).Left() + 1e-4f);

    for (int i = 0; i < 30; ++i) {
        (void)world.overworld.Step(world.players, right_in, eco::kFixedDt, true);
    }
    CHECK(mut_p0->pos.x > bridge_center.x);

    // Portal entry trigger
    const auto portals = world.overworld.tilemap.FindTiles(eco::TileId::StructurePortal);
    CHECK(!portals.empty());
    const eco::Vec2 portal_center =
        world.overworld.tilemap.TileCenter(portals[0].row, portals[0].col);
    mut_p0->pos = {
        portal_center.x - mut_p0->size.x * 0.5f,
        portal_center.y - mut_p0->size.y * 0.5f
    };
    const auto portal_res = world.overworld.Step(world.players, eco::AllPlayersInput{}, eco::kFixedDt, false);
    CHECK(portal_res.enter_structure_requested);

    // Returning from StructureState places players at portal_exit_spawn & arms loop guard
    world.overworld.OnEnter(world.players, true, false);
    CHECK(!world.overworld.tilemap.OverlapsTileId(p0->Bounds(), eco::TileId::StructurePortal));
    CHECK(world.overworld.portal_cooldown_timer > 0.0f);
    CHECK(world.overworld.portal_reentry_locked);

    // Even if forced onto portal while cooldown active, loop guard suppresses re-entry
    mut_p0->pos = {
        portal_center.x - mut_p0->size.x * 0.5f,
        portal_center.y - mut_p0->size.y * 0.5f
    };
    const auto locked_res = world.overworld.Step(world.players, eco::AllPlayersInput{}, eco::kFixedDt, false);
    CHECK(!locked_res.enter_structure_requested);

    // After cooldown expires AND player leaves portal, portal works again
    mut_p0->pos = world.overworld.portal_exit_spawn;
    for (int i = 0; i < 40; ++i) {
        const auto wait_res = world.overworld.Step(world.players, eco::AllPlayersInput{}, eco::kFixedDt, false);
        CHECK(!wait_res.enter_structure_requested);
    }
    CHECK(!world.overworld.portal_reentry_locked);
    mut_p0->pos = {
        portal_center.x - mut_p0->size.x * 0.5f,
        portal_center.y - mut_p0->size.y * 0.5f
    };
    const auto unlocked_res = world.overworld.Step(world.players, eco::AllPlayersInput{}, eco::kFixedDt, false);
    CHECK(unlocked_res.enter_structure_requested);

    world.Reset();
    CHECK(!world.overworld.initialized);
    CHECK(world.players.ActiveCount() == 1);
}

void TestSideScrollingPhysicsM3() {
    eco::Tilemap chamber = eco::Tilemap::CreateStructureMap();
    CHECK(chamber.Rows() == 15);
    CHECK(chamber.Cols() == 28);
    CHECK(!chamber.FindTiles(eco::TileId::ExitPortal).empty());

    // Floor at row 13 (top = 13 * 48 = 624.0f -> player stands at y = 594.0f)
    eco::Player p(0);
    p.pos = {300.0f, 550.0f};
    p.y_velocity = 900.0f;
    for (int i = 0; i < 10; ++i) {
        eco::MovePlayerSideScrolling(p, 0.0f, false, false, eco::kFixedDt, chamber);
    }
    CHECK(p.isGrounded);
    CHECK_NEAR(p.pos.y, 594.0f, 1e-3f);
    CHECK_NEAR(p.y_velocity, 0.0f, 1e-4f);

    // High-speed sub-stepped vertical sweep prevents floor tunneling
    p.pos = {300.0f, 590.0f};
    p.y_velocity = 5400.0f;
    p.isGrounded = false;
    eco::MovePlayerSideScrolling(p, 0.0f, false, false, eco::kFixedDt, chamber);
    CHECK(p.isGrounded);
    CHECK_NEAR(p.pos.y, 594.0f, 1e-3f);
    CHECK_NEAR(p.y_velocity, 0.0f, 1e-4f);

    // Coyote time jump after walking off ledge
    eco::Tilemap ledge_map({
        {1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 1},
        {1, 1, 1, 0, 0, 1},
        {1, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1}
    }, 48.0f);
    p.ResetVerticalState();
    p.pos = {143.0f, 66.0f}; // 1px overlap on ledge at row 2, cols 1..2 (top = 96.0f)
    eco::MovePlayerSideScrolling(p, 0.0f, false, false, eco::kFixedDt, ledge_map);
    CHECK(p.isGrounded);
    eco::MovePlayerSideScrolling(p, 1.0f, false, false, eco::kFixedDt, ledge_map);
    CHECK(!p.isGrounded);
    CHECK(p.coyote_timer > 0.0f);
    for (int i = 0; i < 3; ++i) {
        eco::MovePlayerSideScrolling(p, 0.0f, false, false, eco::kFixedDt, ledge_map);
    }
    eco::MovePlayerSideScrolling(p, 0.0f, true, true, eco::kFixedDt, ledge_map);
    CHECK(p.y_velocity <= -460.0f);
    CHECK_NEAR(p.coyote_timer, 0.0f, 1e-5f);

    // Jump buffer triggers immediately upon landing
    p.ResetVerticalState();
    p.pos = {160.0f, 140.0f}; // Falling toward row 4 floor (top = 192.0f -> player y = 162.0f)
    p.y_velocity = 600.0f;
    eco::MovePlayerSideScrolling(p, 0.0f, true, false, eco::kFixedDt, ledge_map);
    for (int i = 0; i < 4; ++i) {
        eco::MovePlayerSideScrolling(p, 0.0f, false, false, eco::kFixedDt, ledge_map);
    }
    CHECK(p.y_velocity < 0.0f);
    CHECK(!p.isGrounded);
}

void TestMachinesAndExploratoryM4() {
    // Cosine solar & hydro turbine pressure math
    CHECK_NEAR(eco::ComputeSolarEfficiency(45.0f, 45.0f), 1.0f, 1e-5f);
    CHECK(eco::ComputeSolarEfficiency(65.0f, 45.0f) > eco::kSolarEfficiencyThreshold);
    CHECK(eco::ComputeSolarEfficiency(90.0f, 45.0f) <= eco::kSolarEfficiencyThreshold);
    CHECK_NEAR(eco::ComputeSolarEfficiency(135.0f, 0.0f), 0.0f, 1e-5f);

    CHECK_NEAR(eco::ComputeHydroEfficiency(10.0f, 10.0f, 20000.0f), 1.0f, 1e-5f);
    CHECK(eco::ComputeHydroEfficiency(10.3f, 10.0f, 20000.0f) > eco::kHydroEfficiencyThreshold);
    CHECK(eco::ComputeHydroEfficiency(11.0f, 10.0f, 20000.0f) <= eco::kHydroEfficiencyThreshold);

    // Full WorldContext M4 workflow: Empirical Scan -> Clean Solar Calibration -> Phase 2 -> Architect NPC -> Phase 3
    eco::WorldContext world;
    world.EnsureInitialized();
    world.overworld.OnEnter(world.players, false, false);

    eco::Player* p0 = world.players.GetPlayer(0);
    CHECK(p0 != nullptr);
    p0->pos = world.solar_machine.pos;

    const eco::EmpiricalScanReading scan = world.TriggerScan(0);
    CHECK(scan.active && scan.has_target);
    CHECK_NEAR(scan.measured_target_value, world.solar_machine.target_param, 1e-4f);

    // Reject Phase 2 and Architect NPC before clean energy
    CHECK(!eco::TryRequestOverlayPhase(world.ecosystem, eco::UiPhase::Phase2_Telemetry));
    p0->pos = world.architect_npc.pos;
    CHECK(!world.architect_npc.TryInteract(0, world.players, world.ecosystem));

    // Calibrate Solar Array cleanly
    p0->pos = world.solar_machine.pos;
    world.calibration_modal.OpenFor(world.solar_machine);
    CHECK(world.calibration_modal.is_open);
    world.calibration_modal.SetParameter(scan.measured_target_value, world.solar_machine);
    CHECK(world.calibration_modal.can_confirm_clean);
    CHECK(world.calibration_modal.ConfirmClean(world.solar_machine, world.ecosystem));
    CHECK(!world.calibration_modal.is_open);
    CHECK(world.solar_machine.state == eco::MachineState::CleanActive);
    CHECK(world.ecosystem.clean_energy > 0.0f);
    CHECK(world.ecosystem.ui_phase == eco::UiPhase::Phase2_Telemetry);

    // Recruit Architect NPC -> unlocks Phase 3 Engine Overlays and HiddenBridge
    p0->pos = world.architect_npc.pos;
    CHECK(world.architect_npc.TryInteract(0, world.players, world.ecosystem));
    CHECK(world.architect_npc.recruited);
    CHECK(world.ecosystem.architect_recruited);
    CHECK(world.ecosystem.ui_phase == eco::UiPhase::Phase3_EngineOverlay);
    const eco::OverlayVisibility ov = eco::ComputeOverlayVisibility(world.ecosystem);
    CHECK(ov.show_raw_numbers && ov.show_velocity_vectors && ov.show_aabb_hitboxes &&
          ov.show_irradiance_rays && ov.show_trigger_radii);
}

void TestStoryDirector() {
    // 1. ChapterFor branches
    eco::StorySnapshot s{};
    CHECK(eco::ChapterFor(s) == eco::Chapter::Prologo);

    s.has_scanned = true;
    CHECK(eco::ChapterFor(s) == eco::Chapter::Atto1_Luce);

    s.has_scanned = false;
    s.solar = eco::MachineState::CleanActive;
    CHECK(eco::ChapterFor(s) == eco::Chapter::Atto1_Luce);

    s.architect_recruited = true;
    CHECK(eco::ChapterFor(s) == eco::Chapter::Atto2_Acqua);

    s.hydro = eco::MachineState::CleanActive;
    CHECK(eco::ChapterFor(s) == eco::Chapter::Atto3_Codice);

    // Bypassed hydro also advances to Atto3_Codice so Grey Ending is reachable
    s.hydro = eco::MachineState::BypassedDirty;
    CHECK(eco::ChapterFor(s) == eco::Chapter::Atto3_Codice);

    s.simulation_completed = true;
    CHECK(eco::ChapterFor(s) == eco::Chapter::Epilogo);

    // 2. ChapterTitle & ObjectiveFor non-empty across all chapters
    const std::array<eco::Chapter, 5> chapters = {
        eco::Chapter::Prologo,
        eco::Chapter::Atto1_Luce,
        eco::Chapter::Atto2_Acqua,
        eco::Chapter::Atto3_Codice,
        eco::Chapter::Epilogo
    };
    for (eco::Chapter c : chapters) {
        const char* title = eco::ChapterTitle(c);
        CHECK(title != nullptr && std::string(title).size() > 0);
        const eco::Objective obj = eco::ObjectiveFor(c, s);
        CHECK(!obj.title.empty());
    }
    CHECK(!eco::IntroCrawl().empty());
    CHECK(!eco::EndingText(true).empty());
    CHECK(!eco::EndingText(false).empty());

    // 3. StoryDirector one-shot events, queue, Tick, AdvanceDialogue, CleanPath
    eco::StoryDirector dir;
    dir.Reset();
    CHECK(dir.CleanPath());
    CHECK(!dir.HasActiveDialogue());

    eco::StorySnapshot snap{};
    dir.Observe(snap);
    CHECK(dir.EventFired(eco::StoryEvent::GameStarted));
    CHECK(dir.HasActiveDialogue());
    const eco::DialogueLine* first_line = dir.CurrentLine();
    CHECK(first_line != nullptr && !first_line->text.empty());

    // Re-observing same state does not re-fire GameStarted
    dir.AdvanceDialogue();
    while (dir.HasActiveDialogue()) {
        dir.AdvanceDialogue();
    }
    CHECK(!dir.HasActiveDialogue());
    dir.Observe(snap);
    CHECK(!dir.HasActiveDialogue());

    // FirstScan & SolarClean
    snap.has_scanned = true;
    dir.Observe(snap);
    CHECK(dir.EventFired(eco::StoryEvent::FirstScan));
    CHECK(dir.HasActiveDialogue());

    // Tick advances elapsed and eventually drains queue
    dir.Tick(0.5f);
    CHECK(dir.LineElapsed() > 0.0f);
    dir.Tick(500.0f);
    CHECK(!dir.HasActiveDialogue());

    // CleanPath stays true on CleanActive
    snap.solar = eco::MachineState::CleanActive;
    dir.Observe(snap);
    CHECK(dir.EventFired(eco::StoryEvent::SolarClean));
    CHECK(dir.CleanPath());

    // CleanPath becomes false on any *Bypassed event
    snap.hydro = eco::MachineState::BypassedDirty;
    dir.Observe(snap);
    CHECK(dir.EventFired(eco::StoryEvent::HydroBypassed));
    CHECK(!dir.CleanPath());

    // SmogCritical fires at pollution >= 70
    CHECK(!dir.EventFired(eco::StoryEvent::SmogCritical));
    snap.pollution_level = 72.0f;
    dir.Observe(snap);
    CHECK(dir.EventFired(eco::StoryEvent::SmogCritical));
}

} // namespace

int main() {
    TestMathTypes();
    TestFixedTimestepStepper();
    TestInputHelpers();
    TestPlayerAndManager();
    TestEcoSystem();
    TestGameStateManager();
    TestTilemap();
    TestPhysics2D();
    TestAverageCameraController();
    TestOverworldSim();
    TestSideScrollingPhysicsM3();
    TestMachinesAndExploratoryM4();
    TestStoryDirector();

    if (g_failures != 0) {
        std::cerr << g_failures << " test check(s) failed.\n";
        return 1;
    }

    std::cout << "All M1-M4 unit & integration tests passed.\n";
    return 0;
}
