#include "e2e_adapter.hpp"
#include "e2e_harness.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif

namespace {

using eco::e2e::CommandResult;
using eco::e2e::ContractConstants;
using eco::e2e::E2ETier;
using eco::e2e::FilteredInput;
using eco::e2e::FixedLoopTrace;
using eco::e2e::IE2EAdapter;
using eco::e2e::ModeId;
using eco::e2e::PlayerSnapshot;
using eco::e2e::RawGamepadState;
using eco::e2e::RawInputSnapshot;
using eco::e2e::RawKeyboardState;
using eco::e2e::SimSnapshot;
using eco::e2e::StateTransitionReport;

class TempDirGuard {
public:
    explicit TempDirGuard(std::string prefix) {
        const auto now_ns = std::chrono::steady_clock::now().time_since_epoch().count();
        long pid = 0;
#if defined(__unix__) || defined(__APPLE__)
        pid = static_cast<long>(::getpid());
#endif
        path_ = std::filesystem::temp_directory_path() /
                (prefix + "_" + std::to_string(pid) + "_" + std::to_string(now_ns));
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    ~TempDirGuard() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    TempDirGuard(const TempDirGuard&) = delete;
    TempDirGuard& operator=(const TempDirGuard&) = delete;

    [[nodiscard]] std::string Path() const { return path_.string(); }

private:
    std::filesystem::path path_;
};

bool ContainsCaseInsensitive(const std::string& haystack, const std::string& needle) {
    if (needle.empty() || haystack.size() < needle.size()) {
        return false;
    }
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char a, char b) {
            return std::tolower(static_cast<unsigned char>(a)) ==
                   std::tolower(static_cast<unsigned char>(b));
        });
    return it != haystack.end();
}

} // namespace

// ============================================================================
// F01: Portable CMake Build (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F01_T1_01_RootCMakeAndCpp20, E2ETier::Tier1_Feature, "F01") {
    E2E_REQUIRE_LIVE(adapter);
    E2E_ASSERT_TRUE(adapter.FileExists("CMakeLists.txt"));
    const std::string cmake = adapter.ReadProjectFile("CMakeLists.txt");
    E2E_ASSERT_TRUE(cmake.find("CMAKE_CXX_STANDARD 20") != std::string::npos ||
                    cmake.find("cxx_std_20") != std::string::npos);
    E2E_ASSERT_TRUE(cmake.find("game_core") != std::string::npos);
    E2E_ASSERT_TRUE(cmake.find("eco_proto") != std::string::npos);
    E2E_ASSERT_TRUE(cmake.find("eco_tests") != std::string::npos);
    E2E_ASSERT_TRUE(cmake.find("/home/") == std::string::npos);
    E2E_ASSERT_TRUE(cmake.find("C:\\") == std::string::npos);
}

E2E_TEST(F01_T1_02_OutOfSourceConfigure, E2ETier::Tier1_Feature, "F01") {
    E2E_REQUIRE_LIVE(adapter);
    TempDirGuard tmp_build("eco_e2e_f01_t1_02");
    const CommandResult res = adapter.RunCMakeConfigure(tmp_build.Path());
    E2E_ASSERT_EQ(res.exit_code, 0);
    E2E_ASSERT_TRUE(std::filesystem::exists(
        std::filesystem::path(tmp_build.Path()) / "CMakeCache.txt"));
}

E2E_TEST(F01_T1_03_TargetsBuiltAndArtifactsPresent, E2ETier::Tier1_Feature, "F01") {
    E2E_REQUIRE_LIVE(adapter);
    const auto rep = adapter.InspectBuildAndArtifacts();
    E2E_ASSERT_TRUE(rep.cmake_file_exists);
    E2E_ASSERT_TRUE(rep.has_game_core_target);
    E2E_ASSERT_TRUE(rep.has_eco_proto_target);
    E2E_ASSERT_TRUE(rep.has_eco_tests_target);
    E2E_ASSERT_TRUE(rep.lib_game_core_exists);
    E2E_ASSERT_TRUE(rep.eco_proto_exists);

    const std::filesystem::path build(adapter.BuildDir());
    const std::filesystem::path eco_tests_bin = build / "eco_tests";
    E2E_ASSERT_TRUE(std::filesystem::exists(eco_tests_bin));
    E2E_ASSERT_GE(static_cast<std::uintmax_t>(std::filesystem::file_size(eco_tests_bin)),
                  static_cast<std::uintmax_t>(1));
}

E2E_TEST(F01_T1_04_CTestUnitSuitePasses, E2ETier::Tier1_Feature, "F01") {
    E2E_REQUIRE_LIVE(adapter);
    const CommandResult res = adapter.RunCTest(adapter.BuildDir(), 1);
    E2E_ASSERT_EQ(res.exit_code, 0);
    E2E_ASSERT_TRUE(res.stdout_text.find("100% tests passed") != std::string::npos ||
                    res.stdout_text.find("Passed") != std::string::npos);
}

E2E_TEST(F01_T1_05_NoExternalPhysicsLibraries, E2ETier::Tier1_Feature, "F01") {
    E2E_REQUIRE_LIVE(adapter);
    const auto rep = adapter.InspectBuildAndArtifacts();
    E2E_ASSERT_TRUE(rep.no_external_physics_libs);
    const std::string cmake = adapter.ReadProjectFile("CMakeLists.txt");
    static constexpr std::array<const char*, 6> kBanned = {
        "box2d", "bullet", "chipmunk", "physx", "jolt", "ode"
    };
    for (const char* lib : kBanned) {
        E2E_ASSERT_TRUE(!ContainsCaseInsensitive(cmake, lib));
    }
}

E2E_TEST(F01_T1_06_PortableLinuxWindowsGuards, E2ETier::Tier1_Feature, "F01") {
    E2E_REQUIRE_LIVE(adapter);
    const std::string cmake = adapter.ReadProjectFile("CMakeLists.txt");
    E2E_ASSERT_TRUE(cmake.find("MSVC") != std::string::npos ||
                    cmake.find("WIN32") != std::string::npos);
    E2E_ASSERT_TRUE(cmake.find("UNIX") != std::string::npos ||
                    cmake.find("GNU") != std::string::npos);
    E2E_ASSERT_TRUE(cmake.find("raylib") != std::string::npos);
}

E2E_TEST(F01_T2_01_IdempotentReconfigure, E2ETier::Tier2_Boundary, "F01") {
    E2E_REQUIRE_LIVE(adapter);
    TempDirGuard tmp_build("eco_e2e_f01_t2_01");
    const CommandResult first = adapter.RunCMakeConfigure(tmp_build.Path());
    E2E_ASSERT_EQ(first.exit_code, 0);
    const CommandResult second = adapter.RunCMakeConfigure(tmp_build.Path());
    E2E_ASSERT_EQ(second.exit_code, 0);
}

E2E_TEST(F01_T2_02_IncrementalNoOpBuild, E2ETier::Tier2_Boundary, "F01") {
    E2E_REQUIRE_LIVE(adapter);
    const CommandResult res = adapter.RunCMakeBuild(adapter.BuildDir());
    E2E_ASSERT_EQ(res.exit_code, 0);
}

E2E_TEST(F01_T2_03_NonExistentTargetFails, E2ETier::Tier2_Boundary, "F01") {
    E2E_REQUIRE_LIVE(adapter);
    const CommandResult res =
        adapter.RunCMakeBuild(adapter.BuildDir(), "nonexistent_target_e2e_xyz");
    E2E_ASSERT_TRUE(res.exit_code != 0);
}

E2E_TEST(F01_T2_04_MissingSourceDirFails, E2ETier::Tier2_Boundary, "F01") {
    E2E_REQUIRE_LIVE(adapter);
    TempDirGuard tmp_bad("eco_e2e_f01_t2_04");
    const CommandResult res = adapter.RunProcess(
        "cmake", {"-S", "/tmp/nonexistent_src_e2e_xyz_999", "-B", tmp_bad.Path()});
    E2E_ASSERT_TRUE(res.exit_code != 0);
}

E2E_TEST(F01_T2_05_StrictWarningsCompileE2E, E2ETier::Tier2_Boundary, "F01") {
    // Verified by compilation of this suite under -std=c++20 -Wall -Wextra -Werror -Wpedantic
    E2E_ASSERT_EQ(ContractConstants::kMaxPlayers, 4);
    E2E_REQUIRE_LIVE(adapter);
    E2E_ASSERT_TRUE(adapter.IsLiveBackend());
    E2E_ASSERT_TRUE(adapter.FileExists("tests/e2e/CMakeLists.txt"));
    const std::string e2e_cmake = adapter.ReadProjectFile("tests/e2e/CMakeLists.txt");
    E2E_ASSERT_TRUE(e2e_cmake.find("-Wall") != std::string::npos);
    E2E_ASSERT_TRUE(e2e_cmake.find("-Wextra") != std::string::npos);
    E2E_ASSERT_TRUE(e2e_cmake.find("-Werror") != std::string::npos);
}

E2E_TEST(F01_T2_06_CTestParallelMultiJob, E2ETier::Tier2_Boundary, "F01") {
    E2E_REQUIRE_LIVE(adapter);
    const CommandResult res = adapter.RunCTest(adapter.BuildDir(), 4);
    E2E_ASSERT_EQ(res.exit_code, 0);
    E2E_ASSERT_TRUE(res.stdout_text.find("100% tests passed") != std::string::npos);
}

// ============================================================================
// F02: Raylib Window, MSAA & CLI Modes (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F02_T1_01_WindowAndMsaaContract, E2ETier::Tier1_Feature, "F02") {
    E2E_ASSERT_EQ(ContractConstants::kWindowWidth, 1280);
    E2E_ASSERT_EQ(ContractConstants::kWindowHeight, 720);
    E2E_ASSERT_EQ(ContractConstants::kTargetFps, 60);
    E2E_REQUIRE_LIVE(adapter);
    const auto rep = adapter.InspectBuildAndArtifacts();
    E2E_ASSERT_TRUE(rep.window_config_1280x720_60fps_msaa);
}

E2E_TEST(F02_T1_02_HeadlessE2EExitsZero, E2ETier::Tier1_Feature, "F02") {
    E2E_REQUIRE_LIVE(adapter);
    const CommandResult res = adapter.RunEcoProtoHeadlessE2E(false);
    E2E_ASSERT_EQ(res.exit_code, 0);
    E2E_ASSERT_TRUE(!res.signaled);
    E2E_ASSERT_TRUE(!res.stdout_text.empty());
}

E2E_TEST(F02_T1_03_HeadlessE2EWithoutDisplayEnv, E2ETier::Tier1_Feature, "F02") {
    E2E_REQUIRE_LIVE(adapter);
    const CommandResult res = adapter.RunEcoProtoHeadlessE2E(true);
    E2E_ASSERT_EQ(res.exit_code, 0);
    E2E_ASSERT_TRUE(!res.signaled);
    E2E_ASSERT_TRUE(!res.stdout_text.empty());
}

E2E_TEST(F02_T1_04_SmokeFramesLiveWindowed, E2ETier::Tier1_Feature, "F02") {
    E2E_REQUIRE_LIVE(adapter);
    if (adapter.HasDisplayServer()) {
        const CommandResult res = adapter.RunEcoProtoSmokeFrames(5);
        E2E_ASSERT_EQ(res.exit_code, 0);
        E2E_ASSERT_TRUE(!res.signaled);
    } else {
        // Headless sandbox without reachable display socket: confirm --headless-e2e succeeds
        const CommandResult res = adapter.RunEcoProtoHeadlessE2E(true);
        E2E_ASSERT_EQ(res.exit_code, 0);
    }
}

E2E_TEST(F02_T1_05_UnknownCliFlagRejected, E2ETier::Tier1_Feature, "F02") {
    E2E_REQUIRE_LIVE(adapter);
    const CommandResult res = adapter.RunEcoProtoArgs({"--unknown-flag-e2e-xyz"}, true);
    E2E_ASSERT_TRUE(res.exit_code != 0);
    E2E_ASSERT_TRUE(!res.signaled);
}

E2E_TEST(F02_T1_06_HeadlessE2EDeterministicOutput, E2ETier::Tier1_Feature, "F02") {
    E2E_REQUIRE_LIVE(adapter);
    const CommandResult run1 = adapter.RunEcoProtoHeadlessE2E(true);
    const CommandResult run2 = adapter.RunEcoProtoHeadlessE2E(true);
    E2E_ASSERT_EQ(run1.exit_code, 0);
    E2E_ASSERT_EQ(run2.exit_code, 0);
    E2E_ASSERT_EQ(run1.stdout_text, run2.stdout_text);
}

E2E_TEST(F02_T2_01_SmokeFramesZeroBoundary, E2ETier::Tier2_Boundary, "F02") {
    E2E_REQUIRE_LIVE(adapter);
    const CommandResult res = adapter.RunEcoProtoSmokeFrames(0);
    E2E_ASSERT_TRUE(!res.signaled);
    E2E_ASSERT_TRUE(res.exit_code != 0);
}

E2E_TEST(F02_T2_02_SmokeFramesOneBoundary, E2ETier::Tier2_Boundary, "F02") {
    E2E_REQUIRE_LIVE(adapter);
    if (adapter.HasDisplayServer()) {
        const CommandResult res = adapter.RunEcoProtoSmokeFrames(1);
        E2E_ASSERT_EQ(res.exit_code, 0);
        E2E_ASSERT_TRUE(!res.signaled);
    } else {
        // Headless fallback: verify --smoke-frames 1 parses cleanly at positive boundary
        // alongside --headless-e2e even under poisoned display env, while --smoke-frames 0
        // is rejected before --headless-e2e dispatch.
        const std::vector<std::pair<std::string, std::string>> poisoned_env = {
            {"DISPLAY", ":999"},
            {"WAYLAND_DISPLAY", "wayland-nonexistent"},
            {"XDG_RUNTIME_DIR", "/tmp/nonexistent_runtime_dir_e2e"}
        };
        const CommandResult res_one = adapter.RunEcoProtoArgs(
            {"--smoke-frames", "1", "--headless-e2e"}, false, poisoned_env);
        E2E_ASSERT_EQ(res_one.exit_code, 0);
        E2E_ASSERT_TRUE(!res_one.signaled);
        E2E_ASSERT_TRUE(!res_one.stdout_text.empty());

        const CommandResult res_zero = adapter.RunEcoProtoArgs(
            {"--smoke-frames", "0", "--headless-e2e"}, false, poisoned_env);
        E2E_ASSERT_TRUE(res_zero.exit_code != 0);
        E2E_ASSERT_TRUE(!res_zero.signaled);

        const std::string main_cpp = adapter.ReadProjectFile("src/app/main.cpp");
        E2E_ASSERT_TRUE(main_cpp.find("parsed <= 0") != std::string::npos);
        E2E_ASSERT_TRUE(main_cpp.find("frame_count < smoke_frames") != std::string::npos);
    }
}

E2E_TEST(F02_T2_03_SmokeFramesNegativeBoundary, E2ETier::Tier2_Boundary, "F02") {
    E2E_REQUIRE_LIVE(adapter);
    const CommandResult res = adapter.RunEcoProtoArgs({"--smoke-frames", "-1"}, true);
    E2E_ASSERT_TRUE(res.exit_code != 0);
    E2E_ASSERT_TRUE(!res.signaled);
}

E2E_TEST(F02_T2_04_SmokeFramesMissingValue, E2ETier::Tier2_Boundary, "F02") {
    E2E_REQUIRE_LIVE(adapter);
    const CommandResult res = adapter.RunEcoProtoArgs({"--smoke-frames"}, true);
    E2E_ASSERT_TRUE(res.exit_code != 0);
    E2E_ASSERT_TRUE(!res.signaled);
}

E2E_TEST(F02_T2_05_SmokeFramesNonNumericValue, E2ETier::Tier2_Boundary, "F02") {
    E2E_REQUIRE_LIVE(adapter);
    const CommandResult res = adapter.RunEcoProtoArgs({"--smoke-frames", "abc"}, true);
    E2E_ASSERT_TRUE(res.exit_code != 0);
    E2E_ASSERT_TRUE(!res.signaled);
}

E2E_TEST(F02_T2_06_SmokeFramesOverflowValue, E2ETier::Tier2_Boundary, "F02") {
    E2E_REQUIRE_LIVE(adapter);
    const CommandResult res =
        adapter.RunEcoProtoArgs({"--smoke-frames", "99999999999999999"}, true);
    E2E_ASSERT_TRUE(res.exit_code != 0);
    E2E_ASSERT_TRUE(!res.signaled);
}

// ============================================================================
// F03: Fixed-Timestep Accumulator Loop (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F03_T1_01_ExactSingleFixedStep, E2ETier::Tier1_Feature, "F03") {
    E2E_ASSERT_NEAR(ContractConstants::kFixedDt, 1.0f / 60.0f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kMaxFrameDelta, 0.25f, 1e-6f);
    E2E_REQUIRE_LIVE(adapter);
    const FixedLoopTrace tr = adapter.StepFixedLoop({ContractConstants::kFixedDt});
    E2E_ASSERT_EQ(tr.total_steps, 1);
    E2E_ASSERT_EQ(static_cast<int>(tr.step_dts.size()), 1);
    E2E_ASSERT_NEAR(tr.step_dts[0], ContractConstants::kFixedDt, 1e-6f);
    E2E_ASSERT_NEAR(tr.remaining_accumulator, 0.0f, 1e-6f);
}

E2E_TEST(F03_T1_02_SubStepAccumulationAcrossFrames, E2ETier::Tier1_Feature, "F03") {
    E2E_REQUIRE_LIVE(adapter);
    const FixedLoopTrace tr1 = adapter.StepFixedLoop({0.010f});
    E2E_ASSERT_EQ(tr1.total_steps, 0);
    E2E_ASSERT_NEAR(tr1.remaining_accumulator, 0.010f, 1e-5f);

    const FixedLoopTrace tr2 = adapter.StepFixedLoop({0.010f, 0.010f});
    E2E_ASSERT_EQ(tr2.total_steps, 1);
    E2E_ASSERT_NEAR(tr2.remaining_accumulator, 0.020f - ContractConstants::kFixedDt, 1e-5f);
}

E2E_TEST(F03_T1_03_MultiStepLowFramerate, E2ETier::Tier1_Feature, "F03") {
    E2E_REQUIRE_LIVE(adapter);
    const FixedLoopTrace tr = adapter.StepFixedLoop({2.0f * ContractConstants::kFixedDt});
    E2E_ASSERT_EQ(tr.total_steps, 2);
    E2E_ASSERT_EQ(static_cast<int>(tr.step_dts.size()), 2);
    E2E_ASSERT_NEAR(tr.step_dts[0], ContractConstants::kFixedDt, 1e-6f);
    E2E_ASSERT_NEAR(tr.step_dts[1], ContractConstants::kFixedDt, 1e-6f);
}

E2E_TEST(F03_T1_04_StepDtAlwaysConstant, E2ETier::Tier1_Feature, "F03") {
    E2E_REQUIRE_LIVE(adapter);
    const FixedLoopTrace tr = adapter.StepFixedLoop({0.005f, 0.022f, 0.041f, 0.012f});
    E2E_ASSERT_GE(tr.total_steps, 4);
    for (float dt : tr.step_dts) {
        E2E_ASSERT_NEAR(dt, ContractConstants::kFixedDt, 1e-6f);
    }
}

E2E_TEST(F03_T1_05_SpiralOfDeathClamp, E2ETier::Tier1_Feature, "F03") {
    E2E_REQUIRE_LIVE(adapter);
    const FixedLoopTrace tr = adapter.StepFixedLoop({2.0f});
    E2E_ASSERT_GE(tr.total_steps, 1);
    E2E_ASSERT_LE(tr.total_steps, 15);
}

E2E_TEST(F03_T1_06_SimulationTimeEquivalence, E2ETier::Tier1_Feature, "F03") {
    E2E_REQUIRE_LIVE(adapter);
    std::vector<float> deltas_60hz(30, ContractConstants::kFixedDt);
    std::vector<float> deltas_30hz(15, 2.0f * ContractConstants::kFixedDt);
    const FixedLoopTrace tr_60 = adapter.StepFixedLoop(deltas_60hz);
    const FixedLoopTrace tr_30 = adapter.StepFixedLoop(deltas_30hz);
    E2E_ASSERT_EQ(tr_60.total_steps, 30);
    E2E_ASSERT_EQ(tr_30.total_steps, 30);
}

E2E_TEST(F03_T2_01_ZeroFrameDelta, E2ETier::Tier2_Boundary, "F03") {
    E2E_REQUIRE_LIVE(adapter);
    const FixedLoopTrace tr = adapter.StepFixedLoop({0.0f, 0.0f});
    E2E_ASSERT_EQ(tr.total_steps, 0);
    E2E_ASSERT_NEAR(tr.remaining_accumulator, 0.0f, 1e-6f);
}

E2E_TEST(F03_T2_02_NegativeFrameDelta, E2ETier::Tier2_Boundary, "F03") {
    E2E_REQUIRE_LIVE(adapter);
    const FixedLoopTrace tr = adapter.StepFixedLoop({-0.05f, -1.0f});
    E2E_ASSERT_EQ(tr.total_steps, 0);
    E2E_ASSERT_NEAR(tr.remaining_accumulator, 0.0f, 1e-6f);
}

E2E_TEST(F03_T2_03_JustBelowFixedDt, E2ETier::Tier2_Boundary, "F03") {
    E2E_REQUIRE_LIVE(adapter);
    const FixedLoopTrace tr =
        adapter.StepFixedLoop({ContractConstants::kFixedDt - 1e-4f});
    E2E_ASSERT_EQ(tr.total_steps, 0);
    E2E_ASSERT_GE(tr.remaining_accumulator, 0.016f);
}

E2E_TEST(F03_T2_04_AtMaxFrameDelta, E2ETier::Tier2_Boundary, "F03") {
    E2E_REQUIRE_LIVE(adapter);
    const FixedLoopTrace tr = adapter.StepFixedLoop({ContractConstants::kMaxFrameDelta});
    E2E_ASSERT_GE(tr.total_steps, 14);
    E2E_ASSERT_LE(tr.total_steps, 15);
    E2E_ASSERT_GE(tr.remaining_accumulator, 0.0f);
    E2E_ASSERT_TRUE(tr.remaining_accumulator < ContractConstants::kFixedDt);
}

E2E_TEST(F03_T2_05_JustAboveMaxFrameDelta, E2ETier::Tier2_Boundary, "F03") {
    E2E_REQUIRE_LIVE(adapter);
    const FixedLoopTrace tr_a = adapter.StepFixedLoop({0.30f});
    const FixedLoopTrace tr_b = adapter.StepFixedLoop({100.0f});
    E2E_ASSERT_EQ(tr_a.total_steps, tr_b.total_steps);
    E2E_ASSERT_GE(tr_a.total_steps, 14);
    E2E_ASSERT_LE(tr_a.total_steps, 15);
    E2E_ASSERT_NEAR(tr_a.remaining_accumulator, tr_b.remaining_accumulator, 1e-6f);
}

E2E_TEST(F03_T2_06_NonFiniteFrameDeltaNaNAndInf, E2ETier::Tier2_Boundary, "F03") {
    E2E_REQUIRE_LIVE(adapter);
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float pinf = std::numeric_limits<float>::infinity();
    const FixedLoopTrace tr =
        adapter.StepFixedLoop({qnan, pinf, ContractConstants::kFixedDt});
    E2E_ASSERT_TRUE(!std::isnan(tr.remaining_accumulator));
    E2E_ASSERT_TRUE(std::isfinite(tr.remaining_accumulator));
    E2E_ASSERT_GE(tr.total_steps, 1);
}

// ============================================================================
// F04: GameState & GameStateManager (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F04_T1_01_GameStateInterfacePolymorphism, E2ETier::Tier1_Feature, "F04") {
    E2E_REQUIRE_LIVE(adapter);
    E2E_ASSERT_TRUE(adapter.IsGameStateAbstractAndPolymorphic());
}

E2E_TEST(F04_T1_02_InitialStateEnterUpdateDraw, E2ETier::Tier1_Feature, "F04") {
    E2E_REQUIRE_LIVE(adapter);
    const StateTransitionReport rep =
        adapter.RunStateLifecycleProbe(ModeId::Overworld, {}, 2);
    E2E_ASSERT_EQ(rep.enter_count, 1);
    E2E_ASSERT_EQ(rep.exit_count, 0);
    E2E_ASSERT_EQ(rep.updates_executed, 3);
    E2E_ASSERT_EQ(rep.draws_executed, 3);
    E2E_ASSERT_EQ(rep.mode_after_tick, ModeId::Overworld);
}

E2E_TEST(F04_T1_03_DeferredTransitionMidUpdateSafety, E2ETier::Tier1_Feature, "F04") {
    E2E_REQUIRE_LIVE(adapter);
    const StateTransitionReport rep =
        adapter.RunStateLifecycleProbe(ModeId::Overworld, {ModeId::Structure}, 0);
    E2E_ASSERT_TRUE(!rep.old_destroyed_mid_update);
    E2E_ASSERT_EQ(rep.mode_after_request_mid_update, ModeId::Overworld);
    E2E_ASSERT_EQ(rep.mode_after_tick, ModeId::Structure);
    E2E_ASSERT_EQ(rep.exit_count, 1);
    E2E_ASSERT_EQ(rep.enter_count, 2);
}

E2E_TEST(F04_T1_04_BidirectionalRoundTrip, E2ETier::Tier1_Feature, "F04") {
    E2E_REQUIRE_LIVE(adapter);
    const StateTransitionReport rep = adapter.RunRapidTransitionSequence({
        ModeId::Overworld,
        ModeId::Structure,
        ModeId::Overworld
    });
    E2E_ASSERT_EQ(rep.mode_after_tick, ModeId::Overworld);
    E2E_ASSERT_EQ(rep.enter_count, 3);
    E2E_ASSERT_EQ(rep.exit_count, 2);
}

E2E_TEST(F04_T1_05_WorldContextPersistsAcrossTransition, E2ETier::Tier1_Feature, "F04") {
    E2E_REQUIRE_LIVE(adapter);
    const StateTransitionReport rep = adapter.RunStateLifecycleProbe(
        ModeId::Overworld, {ModeId::Structure}, 2, ContractConstants::kFixedDt, true);
    E2E_ASSERT_EQ(rep.mode_after_tick, ModeId::Structure);
    E2E_ASSERT_NEAR(rep.persisted_clean_energy, 25.0f, 1e-4f);
    E2E_ASSERT_NEAR(rep.persisted_pollution, 25.0f, 1e-4f);
}

E2E_TEST(F04_T1_06_EmptyManagerSafeNoOp, E2ETier::Tier1_Feature, "F04") {
    E2E_REQUIRE_LIVE(adapter);
    const StateTransitionReport rep = adapter.RunEmptyManagerProbe(5);
    E2E_ASSERT_EQ(rep.mode_after_tick, ModeId::None);
}

E2E_TEST(F04_T2_01_MultipleTransitionRequestsSameTick, E2ETier::Tier2_Boundary, "F04") {
    E2E_REQUIRE_LIVE(adapter);
    const StateTransitionReport rep = adapter.RunStateLifecycleProbe(
        ModeId::Overworld, {ModeId::Structure, ModeId::Overworld}, 1);
    E2E_ASSERT_TRUE(!rep.old_destroyed_mid_update);
    E2E_ASSERT_EQ(rep.mode_after_tick, ModeId::Overworld);
    E2E_ASSERT_EQ(rep.exit_count, 1);
    E2E_ASSERT_EQ(rep.enter_count, 2);
}

E2E_TEST(F04_T2_02_SelfTransitionSameMode, E2ETier::Tier2_Boundary, "F04") {
    E2E_REQUIRE_LIVE(adapter);
    const StateTransitionReport rep =
        adapter.RunStateLifecycleProbe(ModeId::Overworld, {ModeId::Overworld}, 1);
    E2E_ASSERT_TRUE(!rep.old_destroyed_mid_update);
    E2E_ASSERT_EQ(rep.mode_after_tick, ModeId::Overworld);
}

E2E_TEST(F04_T2_03_NullTransitionRequestIgnored, E2ETier::Tier2_Boundary, "F04") {
    E2E_REQUIRE_LIVE(adapter);
    const StateTransitionReport rep =
        adapter.RunStateLifecycleProbe(ModeId::Overworld, {ModeId::None}, 1);
    E2E_ASSERT_EQ(rep.mode_after_tick, ModeId::Overworld);
    E2E_ASSERT_EQ(rep.exit_count, 0);
    E2E_ASSERT_EQ(rep.enter_count, 1);
}

E2E_TEST(F04_T2_04_ApplyPendingWithoutRequest, E2ETier::Tier2_Boundary, "F04") {
    E2E_REQUIRE_LIVE(adapter);
    const StateTransitionReport rep =
        adapter.RunStateLifecycleProbe(ModeId::Structure, {}, 3);
    E2E_ASSERT_EQ(rep.exit_count, 0);
    E2E_ASSERT_EQ(rep.enter_count, 1);
    E2E_ASSERT_EQ(rep.mode_after_tick, ModeId::Structure);
}

E2E_TEST(F04_T2_05_RapidStressPingPongTransitions, E2ETier::Tier2_Boundary, "F04") {
    E2E_REQUIRE_LIVE(adapter);
    std::vector<ModeId> seq;
    seq.reserve(100);
    for (int i = 0; i < 100; ++i) {
        seq.push_back((i % 2 == 0) ? ModeId::Overworld : ModeId::Structure);
    }
    const StateTransitionReport rep = adapter.RunRapidTransitionSequence(seq);
    E2E_ASSERT_EQ(rep.mode_after_tick, ModeId::Structure);
    E2E_ASSERT_EQ(rep.enter_count, 100);
    E2E_ASSERT_EQ(rep.exit_count, 99);
}

E2E_TEST(F04_T2_06_ZeroDtUpdateWithPendingTransition, E2ETier::Tier2_Boundary, "F04") {
    E2E_REQUIRE_LIVE(adapter);
    const StateTransitionReport rep =
        adapter.RunStateLifecycleProbe(ModeId::Overworld, {ModeId::Structure}, 0, 0.0f);
    E2E_ASSERT_EQ(rep.mode_after_tick, ModeId::Structure);
    E2E_ASSERT_EQ(rep.exit_count, 1);
    E2E_ASSERT_EQ(rep.enter_count, 2);
}

// ============================================================================
// F05: Headless Logic / Render Separation (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F05_T1_01_CoreHeadersZeroRaylibIncludes, E2ETier::Tier1_Feature, "F05") {
    E2E_REQUIRE_LIVE(adapter);
    E2E_ASSERT_TRUE(adapter.CoreSourcesHaveZeroRaylibOrGlIncludes());
}

E2E_TEST(F05_T1_02_CoreTargetsZeroDisplayLinkage, E2ETier::Tier1_Feature, "F05") {
    E2E_REQUIRE_LIVE(adapter);
    E2E_ASSERT_TRUE(adapter.CoreBinaryHasZeroDisplayLibs("eco_tests"));
    E2E_ASSERT_TRUE(adapter.CoreBinaryHasZeroDisplayLibs("eco_e2e_runner"));
}

E2E_TEST(F05_T1_03_HeadlessSimulationStepWithoutWindow, E2ETier::Tier1_Feature, "F05") {
    E2E_REQUIRE_LIVE(adapter);
    std::array<RawInputSnapshot, 4> inputs{};
    inputs[0].kb.d = true;
    const SimSnapshot snap =
        adapter.RunHeadlessSimulationSteps(60, ContractConstants::kFixedDt, inputs, false);
    E2E_ASSERT_GE(snap.active_player_count, 1);
    E2E_ASSERT_TRUE(std::isfinite(snap.player_x[0]));
    E2E_ASSERT_TRUE(snap.player_x[0] > 0.0f);
}

E2E_TEST(F05_T1_04_UpdateVsReadSnapshotDeterminism, E2ETier::Tier1_Feature, "F05") {
    E2E_REQUIRE_LIVE(adapter);
    std::array<RawInputSnapshot, 4> inputs{};
    inputs[0].kb.d = true;
    inputs[0].kb.s = true;
    const SimSnapshot s1 =
        adapter.RunHeadlessSimulationSteps(120, ContractConstants::kFixedDt, inputs, false);
    const SimSnapshot s2 =
        adapter.RunHeadlessSimulationSteps(120, ContractConstants::kFixedDt, inputs, true);
    E2E_ASSERT_NEAR(s1.player_x[0], s2.player_x[0], 1e-6f);
    E2E_ASSERT_NEAR(s1.player_y[0], s2.player_y[0], 1e-6f);
    E2E_ASSERT_NEAR(s1.pollution, s2.pollution, 1e-6f);
    E2E_ASSERT_NEAR(s1.clean_energy, s2.clean_energy, 1e-6f);
}

E2E_TEST(F05_T1_05_ReadSnapshotIsNonMutating, E2ETier::Tier1_Feature, "F05") {
    E2E_REQUIRE_LIVE(adapter);
    std::array<RawInputSnapshot, 4> inputs{};
    inputs[0].kb.d = true;
    const SimSnapshot s_once =
        adapter.RunHeadlessSimulationSteps(30, ContractConstants::kFixedDt, inputs, false, 0);
    const SimSnapshot s_50x =
        adapter.RunHeadlessSimulationSteps(30, ContractConstants::kFixedDt, inputs, false, 50);
    E2E_ASSERT_NEAR(s_once.player_x[0], s_50x.player_x[0], 1e-6f);
    E2E_ASSERT_NEAR(s_once.player_y[0], s_50x.player_y[0], 1e-6f);
    E2E_ASSERT_NEAR(s_once.pollution, s_50x.pollution, 1e-6f);
}

E2E_TEST(F05_T1_06_HeadlessCLIIsDisplayIndependent, E2ETier::Tier1_Feature, "F05") {
    E2E_REQUIRE_LIVE(adapter);
    const CommandResult res = adapter.RunEcoProtoHeadlessE2E(true);
    E2E_ASSERT_EQ(res.exit_code, 0);
    E2E_ASSERT_TRUE(!res.signaled);
}

E2E_TEST(F05_T2_01_ZeroStepSimulationSnapshot, E2ETier::Tier2_Boundary, "F05") {
    E2E_ASSERT_NEAR(ContractConstants::kInitialPollution, 35.0f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kInitialCleanEnergy, 0.0f, 1e-6f);
    E2E_REQUIRE_LIVE(adapter);
    std::array<RawInputSnapshot, 4> inputs{};
    const SimSnapshot snap =
        adapter.RunHeadlessSimulationSteps(0, ContractConstants::kFixedDt, inputs, false);
    E2E_ASSERT_NEAR(snap.pollution, ContractConstants::kInitialPollution, 1e-5f);
    E2E_ASSERT_NEAR(snap.clean_energy, ContractConstants::kInitialCleanEnergy, 1e-5f);
    E2E_ASSERT_GE(snap.active_player_count, 1);
    E2E_ASSERT_TRUE(std::isfinite(snap.player_x[0]));
    E2E_ASSERT_TRUE(std::isfinite(snap.player_y[0]));
}

E2E_TEST(F05_T2_02_SingleStepSimulation, E2ETier::Tier2_Boundary, "F05") {
    E2E_REQUIRE_LIVE(adapter);
    std::array<RawInputSnapshot, 4> inputs{};
    inputs[0].kb.d = true;
    const SimSnapshot snap =
        adapter.RunHeadlessSimulationSteps(1, ContractConstants::kFixedDt, inputs, false);
    E2E_ASSERT_TRUE(std::isfinite(snap.player_x[0]));
    E2E_ASSERT_TRUE(snap.player_x[0] > 0.0f);
}

E2E_TEST(F05_T2_03_LongHeadlessRunStability, E2ETier::Tier2_Boundary, "F05") {
    E2E_REQUIRE_LIVE(adapter);
    std::array<RawInputSnapshot, 4> inputs{};
    inputs[0].kb.d = true;
    inputs[0].kb.w = true;
    const SimSnapshot snap =
        adapter.RunHeadlessSimulationSteps(3600, ContractConstants::kFixedDt, inputs, false);
    E2E_ASSERT_TRUE(std::isfinite(snap.player_x[0]));
    E2E_ASSERT_TRUE(std::isfinite(snap.player_y[0]));
    E2E_ASSERT_GE(snap.pollution, 0.0f);
    E2E_ASSERT_LE(snap.pollution, 100.0f);
    E2E_ASSERT_GE(snap.clean_energy, 0.0f);
    E2E_ASSERT_LE(snap.clean_energy, 100.0f);
}

E2E_TEST(F05_T2_04_ZeroDtStepNonMutating, E2ETier::Tier2_Boundary, "F05") {
    E2E_REQUIRE_LIVE(adapter);
    std::array<RawInputSnapshot, 4> inputs{};
    inputs[0].kb.d = true;
    inputs[0].kb.s = true;
    const SimSnapshot snap = adapter.RunHeadlessSimulationSteps(10, 0.0f, inputs, false);
    E2E_ASSERT_NEAR(snap.player_x[0], 0.0f, 1e-6f);
    E2E_ASSERT_NEAR(snap.player_y[0], 0.0f, 1e-6f);
}

E2E_TEST(F05_T2_05_FourPlayersFullHeadlessLoad, E2ETier::Tier2_Boundary, "F05") {
    E2E_REQUIRE_LIVE(adapter);
    std::array<RawInputSnapshot, 4> inputs{};
    inputs[0].kb.d = true;
    inputs[1].kb.left = true;
    inputs[2].pads[2].available = true;
    inputs[2].pads[2].axis_y = -0.8f;
    inputs[3].pads[3].available = true;
    inputs[3].pads[3].axis_y = 0.8f;
    const SimSnapshot snap =
        adapter.RunHeadlessSimulationSteps(60, ContractConstants::kFixedDt, inputs, false);
    E2E_ASSERT_EQ(snap.active_player_count, 4);
    for (int i = 0; i < 4; ++i) {
        E2E_ASSERT_TRUE(std::isfinite(snap.player_x[static_cast<std::size_t>(i)]));
        E2E_ASSERT_TRUE(std::isfinite(snap.player_y[static_cast<std::size_t>(i)]));
    }
    E2E_ASSERT_TRUE(snap.player_x[0] > 0.0f);
    E2E_ASSERT_TRUE(snap.player_x[1] < 0.0f);
    E2E_ASSERT_TRUE(snap.player_y[2] < 0.0f);
    E2E_ASSERT_TRUE(snap.player_y[3] > 0.0f);
}

E2E_TEST(F05_T2_06_PoisonedDisplayEnvHeadlessCLI, E2ETier::Tier2_Boundary, "F05") {
    E2E_REQUIRE_LIVE(adapter);
    const CommandResult res = adapter.RunEcoProtoArgs(
        {"--headless-e2e"},
        false,
        {{"DISPLAY", ":999"},
         {"WAYLAND_DISPLAY", "nonexistent_wayland_socket"},
         {"XDG_RUNTIME_DIR", "/tmp/nonexistent_runtime_dir_e2e"}});
    E2E_ASSERT_EQ(res.exit_code, 0);
    E2E_ASSERT_TRUE(!res.signaled);
}

// ============================================================================
// F06: Player Entity & 1-4 Player Manager (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F06_T1_01_ConstructValidPlayerIds, E2ETier::Tier1_Feature, "F06") {
    E2E_ASSERT_NEAR(ContractConstants::kPlayerWidth, 30.0f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kPlayerHeight, 30.0f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kGravityAccel, 1200.0f, 1e-6f);
    E2E_ASSERT_NEAR(ContractConstants::kJumpForce, 480.0f, 1e-6f);
    E2E_REQUIRE_LIVE(adapter);
    for (int id = 0; id < 4; ++id) {
        const PlayerSnapshot p = adapter.ConstructPlayerWithId(id);
        E2E_ASSERT_EQ(p.player_id, id);
        E2E_ASSERT_NEAR(p.size.x, ContractConstants::kPlayerWidth, 1e-5f);
        E2E_ASSERT_NEAR(p.size.y, ContractConstants::kPlayerHeight, 1e-5f);
        E2E_ASSERT_NEAR(p.gravity_accel, ContractConstants::kGravityAccel, 1e-5f);
        E2E_ASSERT_NEAR(p.jump_force, ContractConstants::kJumpForce, 1e-5f);
    }
}

E2E_TEST(F06_T1_02_DistinctColorsForAllFourPlayers, E2ETier::Tier1_Feature, "F06") {
    E2E_REQUIRE_LIVE(adapter);
    std::array<PlayerSnapshot, 4> players{};
    for (int i = 0; i < 4; ++i) {
        players[static_cast<std::size_t>(i)] = adapter.ConstructPlayerWithId(i);
        E2E_ASSERT_GE(static_cast<int>(players[static_cast<std::size_t>(i)].color.a), 1);
    }
    for (int i = 0; i < 4; ++i) {
        for (int j = i + 1; j < 4; ++j) {
            const auto& ci = players[static_cast<std::size_t>(i)].color;
            const auto& cj = players[static_cast<std::size_t>(j)].color;
            const bool same_rgb = (ci.r == cj.r && ci.g == cj.g && ci.b == cj.b);
            E2E_ASSERT_TRUE(!same_rgb);
        }
    }
}

E2E_TEST(F06_T1_03_DropInJoinActivation, E2ETier::Tier1_Feature, "F06") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetPlayerManager(1);
    E2E_ASSERT_EQ(adapter.ManagedActiveCount(), 1);
    E2E_ASSERT_TRUE(adapter.TryJoinManagedPlayer(1));
    E2E_ASSERT_EQ(adapter.ManagedActiveCount(), 2);
    E2E_ASSERT_TRUE(adapter.TryJoinManagedPlayer(2));
    E2E_ASSERT_EQ(adapter.ManagedActiveCount(), 3);
    E2E_ASSERT_TRUE(adapter.TryJoinManagedPlayer(3));
    E2E_ASSERT_EQ(adapter.ManagedActiveCount(), 4);
    const auto players = adapter.GetManagedPlayers();
    E2E_ASSERT_EQ(static_cast<int>(players.size()), 4);
    for (const auto& p : players) {
        E2E_ASSERT_TRUE(p.active);
    }
}

E2E_TEST(F06_T1_04_PerPlayerInputIsolation, E2ETier::Tier1_Feature, "F06") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetPlayerManager(4);
    std::array<RawInputSnapshot, 4> inputs{};
    inputs[2].pads[2].available = true;
    inputs[2].pads[2].axis_x = 1.0f;
    for (int t = 0; t < 20; ++t) {
        adapter.StepManagedPlayers(inputs, ContractConstants::kFixedDt);
    }
    const auto players = adapter.GetManagedPlayers();
    E2E_ASSERT_NEAR(players[0].pos.x, 0.0f, 1e-6f);
    E2E_ASSERT_NEAR(players[1].pos.x, 0.0f, 1e-6f);
    E2E_ASSERT_TRUE(players[2].pos.x > 0.0f);
    E2E_ASSERT_NEAR(players[3].pos.x, 0.0f, 1e-6f);
}

E2E_TEST(F06_T1_05_DeactivateAndRejoinPlayer, E2ETier::Tier1_Feature, "F06") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetPlayerManager(1);
    E2E_ASSERT_TRUE(adapter.TryJoinManagedPlayer(1));
    E2E_ASSERT_EQ(adapter.ManagedActiveCount(), 2);
    E2E_ASSERT_TRUE(adapter.SetManagedPlayerActive(1, false));
    E2E_ASSERT_EQ(adapter.ManagedActiveCount(), 1);

    std::array<RawInputSnapshot, 4> inputs{};
    inputs[1].kb.right = true;
    adapter.StepManagedPlayers(inputs, ContractConstants::kFixedDt);
    auto players = adapter.GetManagedPlayers();
    E2E_ASSERT_NEAR(players[1].pos.x, 0.0f, 1e-6f);

    E2E_ASSERT_TRUE(adapter.SetManagedPlayerActive(1, true));
    E2E_ASSERT_EQ(adapter.ManagedActiveCount(), 2);
    adapter.StepManagedPlayers(inputs, ContractConstants::kFixedDt);
    players = adapter.GetManagedPlayers();
    E2E_ASSERT_TRUE(players[1].pos.x > 0.0f);
}

E2E_TEST(F06_T1_06_PlayerHeaderAndSourceExist, E2ETier::Tier1_Feature, "F06") {
    E2E_REQUIRE_LIVE(adapter);
    E2E_ASSERT_TRUE(adapter.FileExists("src/core/player.h"));
    E2E_ASSERT_TRUE(adapter.FileExists("src/core/player.cpp"));
    E2E_ASSERT_TRUE(!adapter.ReadProjectFile("src/core/player.h").empty());
    E2E_ASSERT_TRUE(!adapter.ReadProjectFile("src/core/player.cpp").empty());
}

E2E_TEST(F06_T2_01_PlayerIdNegativeClamp, E2ETier::Tier2_Boundary, "F06") {
    E2E_REQUIRE_LIVE(adapter);
    const PlayerSnapshot p0 = adapter.ConstructPlayerWithId(0);
    for (int raw : {-1, -100, std::numeric_limits<int>::min()}) {
        const PlayerSnapshot p = adapter.ConstructPlayerWithId(raw);
        E2E_ASSERT_EQ(p.player_id, 0);
        E2E_ASSERT_EQ(p.color, p0.color);
    }
}

E2E_TEST(F06_T2_02_PlayerIdAboveMaxClamp, E2ETier::Tier2_Boundary, "F06") {
    E2E_REQUIRE_LIVE(adapter);
    const PlayerSnapshot p3 = adapter.ConstructPlayerWithId(3);
    for (int raw : {4, 99, std::numeric_limits<int>::max()}) {
        const PlayerSnapshot p = adapter.ConstructPlayerWithId(raw);
        E2E_ASSERT_EQ(p.player_id, 3);
        E2E_ASSERT_EQ(p.color, p3.color);
    }
}

E2E_TEST(F06_T2_03_PlayerIdExactBounds, E2ETier::Tier2_Boundary, "F06") {
    E2E_REQUIRE_LIVE(adapter);
    const PlayerSnapshot p0 = adapter.ConstructPlayerWithId(0);
    const PlayerSnapshot p3 = adapter.ConstructPlayerWithId(3);
    E2E_ASSERT_EQ(p0.player_id, 0);
    E2E_ASSERT_EQ(p3.player_id, 3);
}

E2E_TEST(F06_T2_04_MaxCapacityFourPlayersGuard, E2ETier::Tier2_Boundary, "F06") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetPlayerManager(4);
    E2E_ASSERT_EQ(adapter.ManagedActiveCount(), 4);
    E2E_ASSERT_TRUE(!adapter.TryJoinManagedPlayer(4));
    E2E_ASSERT_TRUE(!adapter.TryJoinManagedPlayer(5));
    E2E_ASSERT_TRUE(!adapter.TryJoinManagedPlayer(-1));
    E2E_ASSERT_EQ(adapter.ManagedActiveCount(), 4);
}

E2E_TEST(F06_T2_05_DuplicateJoinIdempotent, E2ETier::Tier2_Boundary, "F06") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetPlayerManager(2);
    std::array<RawInputSnapshot, 4> inputs{};
    inputs[1].kb.right = true;
    for (int t = 0; t < 10; ++t) {
        adapter.StepManagedPlayers(inputs, ContractConstants::kFixedDt);
    }
    const float moved_x = adapter.GetManagedPlayers()[1].pos.x;
    E2E_ASSERT_TRUE(moved_x > 0.0f);
    E2E_ASSERT_TRUE(!adapter.TryJoinManagedPlayer(1));
    E2E_ASSERT_EQ(adapter.ManagedActiveCount(), 2);
    E2E_ASSERT_NEAR(adapter.GetManagedPlayers()[1].pos.x, moved_x, 1e-6f);
}

E2E_TEST(F06_T2_06_ZeroActivePlayersSafe, E2ETier::Tier2_Boundary, "F06") {
    E2E_REQUIRE_LIVE(adapter);
    adapter.ResetPlayerManager(0);
    E2E_ASSERT_EQ(adapter.ManagedActiveCount(), 0);
    std::array<RawInputSnapshot, 4> inputs{};
    inputs[0].kb.d = true;
    for (int t = 0; t < 10; ++t) {
        adapter.StepManagedPlayers(inputs, ContractConstants::kFixedDt);
    }
    E2E_ASSERT_EQ(adapter.ManagedActiveCount(), 0);
}

// ============================================================================
// F07: Keyboard & Gamepad Input + Deadzone (6 Tier 1 + 6 Tier 2 = 12 tests)
// ============================================================================

E2E_TEST(F07_T1_01_Player0QwertyWASD, E2ETier::Tier1_Feature, "F07") {
    E2E_REQUIRE_LIVE(adapter);
    std::array<RawGamepadState, 4> pads{};
    {
        RawKeyboardState kb{}; kb.w = true;
        const FilteredInput fi = adapter.PollPlayerInput(0, kb, pads);
        E2E_ASSERT_NEAR(fi.move_x, 0.0f, 1e-6f);
        E2E_ASSERT_NEAR(fi.move_y, -1.0f, 1e-6f);
    }
    {
        RawKeyboardState kb{}; kb.s = true;
        const FilteredInput fi = adapter.PollPlayerInput(0, kb, pads);
        E2E_ASSERT_NEAR(fi.move_x, 0.0f, 1e-6f);
        E2E_ASSERT_NEAR(fi.move_y, 1.0f, 1e-6f);
    }
    {
        RawKeyboardState kb{}; kb.a = true;
        const FilteredInput fi = adapter.PollPlayerInput(0, kb, pads);
        E2E_ASSERT_NEAR(fi.move_x, -1.0f, 1e-6f);
        E2E_ASSERT_NEAR(fi.move_y, 0.0f, 1e-6f);
    }
    {
        RawKeyboardState kb{}; kb.d = true;
        const FilteredInput fi = adapter.PollPlayerInput(0, kb, pads);
        E2E_ASSERT_NEAR(fi.move_x, 1.0f, 1e-6f);
        E2E_ASSERT_NEAR(fi.move_y, 0.0f, 1e-6f);
    }
}

E2E_TEST(F07_T1_02_Player0AzertyZQSD, E2ETier::Tier1_Feature, "F07") {
    E2E_REQUIRE_LIVE(adapter);
    std::array<RawGamepadState, 4> pads{};
    {
        RawKeyboardState kb{}; kb.z = true;
        const FilteredInput fi = adapter.PollPlayerInput(0, kb, pads);
        E2E_ASSERT_NEAR(fi.move_x, 0.0f, 1e-6f);
        E2E_ASSERT_NEAR(fi.move_y, -1.0f, 1e-6f);
    }
    {
        RawKeyboardState kb{}; kb.s = true;
        const FilteredInput fi = adapter.PollPlayerInput(0, kb, pads);
        E2E_ASSERT_NEAR(fi.move_x, 0.0f, 1e-6f);
        E2E_ASSERT_NEAR(fi.move_y, 1.0f, 1e-6f);
    }
    {
        RawKeyboardState kb{}; kb.q = true;
        const FilteredInput fi = adapter.PollPlayerInput(0, kb, pads);
        E2E_ASSERT_NEAR(fi.move_x, -1.0f, 1e-6f);
        E2E_ASSERT_NEAR(fi.move_y, 0.0f, 1e-6f);
    }
    {
        RawKeyboardState kb{}; kb.d = true;
        const FilteredInput fi = adapter.PollPlayerInput(0, kb, pads);
        E2E_ASSERT_NEAR(fi.move_x, 1.0f, 1e-6f);
        E2E_ASSERT_NEAR(fi.move_y, 0.0f, 1e-6f);
    }
}

E2E_TEST(F07_T1_03_Player1ArrowKeys, E2ETier::Tier1_Feature, "F07") {
    E2E_REQUIRE_LIVE(adapter);
    std::array<RawGamepadState, 4> pads{};
    RawKeyboardState kb{};
    kb.up = true;
    const FilteredInput p1_up = adapter.PollPlayerInput(1, kb, pads);
    const FilteredInput p0_up = adapter.PollPlayerInput(0, kb, pads);
    E2E_ASSERT_NEAR(p1_up.move_y, -1.0f, 1e-6f);
    E2E_ASSERT_NEAR(p0_up.move_y, 0.0f, 1e-6f);

    kb = RawKeyboardState{}; kb.down = true;
    E2E_ASSERT_NEAR(adapter.PollPlayerInput(1, kb, pads).move_y, 1.0f, 1e-6f);
    kb = RawKeyboardState{}; kb.left = true;
    E2E_ASSERT_NEAR(adapter.PollPlayerInput(1, kb, pads).move_x, -1.0f, 1e-6f);
    kb = RawKeyboardState{}; kb.right = true;
    E2E_ASSERT_NEAR(adapter.PollPlayerInput(1, kb, pads).move_x, 1.0f, 1e-6f);
}

E2E_TEST(F07_T1_04_OpposingKeysCancel, E2ETier::Tier1_Feature, "F07") {
    E2E_REQUIRE_LIVE(adapter);
    std::array<RawGamepadState, 4> pads{};
    {
        RawKeyboardState kb{}; kb.a = true; kb.d = true;
        const FilteredInput fi = adapter.PollPlayerInput(0, kb, pads);
        E2E_ASSERT_NEAR(fi.move_x, 0.0f, 1e-6f);
        E2E_ASSERT_NEAR(fi.move_y, 0.0f, 1e-6f);
    }
    {
        RawKeyboardState kb{}; kb.w = true; kb.s = true;
        const FilteredInput fi = adapter.PollPlayerInput(0, kb, pads);
        E2E_ASSERT_NEAR(fi.move_y, 0.0f, 1e-6f);
    }
    {
        RawKeyboardState kb{}; kb.a = true; kb.d = true; kb.w = true;
        const FilteredInput fi = adapter.PollPlayerInput(0, kb, pads);
        E2E_ASSERT_NEAR(fi.move_x, 0.0f, 1e-6f);
        E2E_ASSERT_NEAR(fi.move_y, -1.0f, 1e-6f);
    }
    {
        RawKeyboardState kb{}; kb.left = true; kb.right = true; kb.up = true; kb.down = true;
        const FilteredInput fi = adapter.PollPlayerInput(1, kb, pads);
        E2E_ASSERT_NEAR(fi.move_x, 0.0f, 1e-6f);
        E2E_ASSERT_NEAR(fi.move_y, 0.0f, 1e-6f);
    }
}

E2E_TEST(F07_T1_05_GamepadAllFourPlayers, E2ETier::Tier1_Feature, "F07") {
    E2E_REQUIRE_LIVE(adapter);
    RawKeyboardState kb{};
    std::array<RawGamepadState, 4> pads{};
    pads[0].available = true; pads[0].axis_x = 0.8f;
    pads[1].available = true; pads[1].axis_y = -0.7f;
    pads[2].available = true; pads[2].axis_x = -0.6f;
    pads[3].available = true; pads[3].axis_y = 0.9f;

    E2E_ASSERT_TRUE(adapter.PollPlayerInput(0, kb, pads).move_x > 0.0f);
    E2E_ASSERT_TRUE(adapter.PollPlayerInput(1, kb, pads).move_y < 0.0f);
    E2E_ASSERT_TRUE(adapter.PollPlayerInput(2, kb, pads).move_x < 0.0f);
    E2E_ASSERT_TRUE(adapter.PollPlayerInput(3, kb, pads).move_y > 0.0f);
}

E2E_TEST(F07_T1_06_UnavailableGamepadIgnoredSafely, E2ETier::Tier1_Feature, "F07") {
    E2E_REQUIRE_LIVE(adapter);
    RawKeyboardState kb{};
    kb.d = true;
    std::array<RawGamepadState, 4> pads{};
    for (auto& pad : pads) {
        pad.available = false;
        pad.axis_x = 0.9f;
        pad.axis_y = -0.9f;
    }
    const FilteredInput p0 = adapter.PollPlayerInput(0, kb, pads);
    E2E_ASSERT_NEAR(p0.move_x, 1.0f, 1e-6f);
    E2E_ASSERT_NEAR(p0.move_y, 0.0f, 1e-6f);
    for (int id = 1; id < 4; ++id) {
        const FilteredInput p = adapter.PollPlayerInput(id, kb, pads);
        E2E_ASSERT_NEAR(p.move_x, 0.0f, 1e-6f);
        E2E_ASSERT_NEAR(p.move_y, 0.0f, 1e-6f);
    }
}

E2E_TEST(F07_T2_01_DeadzoneZeroAndInteriorDrift, E2ETier::Tier2_Boundary, "F07") {
    E2E_ASSERT_NEAR(ContractConstants::kGamepadDeadzone, 0.2f, 1e-6f);
    E2E_REQUIRE_LIVE(adapter);
    for (float v : {0.0f, 0.10f, -0.10f, 0.199f, -0.199f}) {
        E2E_ASSERT_NEAR(adapter.FilterDeadzone(v), 0.0f, 1e-6f);
    }
    const auto dz2 = adapter.ApplyGamepadDeadzone(0.14f, -0.14f);
    E2E_ASSERT_NEAR(dz2.x, 0.0f, 1e-6f);
    E2E_ASSERT_NEAR(dz2.y, 0.0f, 1e-6f);
}

E2E_TEST(F07_T2_02_DeadzoneJustOutsidePosAndNeg, E2ETier::Tier2_Boundary, "F07") {
    E2E_REQUIRE_LIVE(adapter);
    for (float v : {0.201f, 0.25f}) {
        const float out = adapter.FilterDeadzone(v);
        E2E_ASSERT_TRUE(out > 0.0f && out <= 1.0f);
    }
    for (float v : {-0.201f, -0.25f}) {
        const float out = adapter.FilterDeadzone(v);
        E2E_ASSERT_TRUE(out < 0.0f && out >= -1.0f);
    }
}

E2E_TEST(F07_T2_03_DeadzoneExactThreshold, E2ETier::Tier2_Boundary, "F07") {
    E2E_REQUIRE_LIVE(adapter);
    const float pos = adapter.FilterDeadzone(0.2f);
    const float neg = adapter.FilterDeadzone(-0.2f);
    E2E_ASSERT_GE(pos, 0.0f);
    E2E_ASSERT_LE(pos, 0.2f + 1e-6f);
    E2E_ASSERT_LE(neg, 0.0f);
    E2E_ASSERT_GE(neg, -0.2f - 1e-6f);
}

E2E_TEST(F07_T2_04_GamepadAxisExtremesAndOutOfRange, E2ETier::Tier2_Boundary, "F07") {
    E2E_REQUIRE_LIVE(adapter);
    E2E_ASSERT_NEAR(adapter.FilterDeadzone(1.0f), 1.0f, 1e-6f);
    E2E_ASSERT_NEAR(adapter.FilterDeadzone(-1.0f), -1.0f, 1e-6f);
    const float over_pos = adapter.FilterDeadzone(1.5f);
    const float over_neg = adapter.FilterDeadzone(-1.5f);
    E2E_ASSERT_GE(over_pos, -1.0f);
    E2E_ASSERT_LE(over_pos, 1.0f);
    E2E_ASSERT_GE(over_neg, -1.0f);
    E2E_ASSERT_LE(over_neg, 1.0f);
}

E2E_TEST(F07_T2_05_CombinedKeyboardPlusGamepadClamp, E2ETier::Tier2_Boundary, "F07") {
    E2E_REQUIRE_LIVE(adapter);
    std::array<RawGamepadState, 4> pads{};
    pads[0].available = true;
    pads[0].axis_x = 1.0f;
    {
        RawKeyboardState kb{};
        kb.d = true;
        const FilteredInput fi = adapter.PollPlayerInput(0, kb, pads);
        E2E_ASSERT_LE(fi.move_x, 1.0f + 1e-6f);
        E2E_ASSERT_GE(fi.move_x, 0.5f);
    }
    {
        RawKeyboardState kb{};
        kb.a = true;
        const FilteredInput fi = adapter.PollPlayerInput(0, kb, pads);
        E2E_ASSERT_NEAR(fi.move_x, 0.0f, 1e-5f);
    }
}

E2E_TEST(F07_T2_06_Players2And3IgnoreKeyboard, E2ETier::Tier2_Boundary, "F07") {
    E2E_REQUIRE_LIVE(adapter);
    RawKeyboardState kb{};
    kb.w = true; kb.a = true; kb.s = true; kb.d = true;
    kb.z = true; kb.q = true;
    kb.up = true; kb.down = true; kb.left = true; kb.right = true;
    kb.space = true; kb.e = true; kb.f = true; kb.r = true;
    kb.right_shift = true; kb.enter = true; kb.right_ctrl = true; kb.period = true;
    std::array<RawGamepadState, 4> pads{};

    for (int id : {2, 3}) {
        const FilteredInput fi = adapter.PollPlayerInput(id, kb, pads);
        E2E_ASSERT_NEAR(fi.move_x, 0.0f, 1e-6f);
        E2E_ASSERT_NEAR(fi.move_y, 0.0f, 1e-6f);
        E2E_ASSERT_TRUE(!fi.jump_pressed);
        E2E_ASSERT_TRUE(!fi.interact_pressed);
        E2E_ASSERT_TRUE(!fi.scan_pressed);
    }
}
