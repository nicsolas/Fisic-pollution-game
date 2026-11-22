#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace eco::e2e {

// Authoritative Interface Contract Constants (PROJECT.md § Interface Contracts)
struct ContractConstants {
    static constexpr int kWindowWidth = 1280;
    static constexpr int kWindowHeight = 720;
    static constexpr int kTargetFps = 60;

    static constexpr float kFixedDt = 1.0f / 60.0f;
    static constexpr float kMaxFrameDelta = 0.25f;

    static constexpr float kTileSize = 48.0f;
    static constexpr float kPlayerWidth = 30.0f;
    static constexpr float kPlayerHeight = 30.0f;

    static constexpr float kTopDownSpeed = 220.0f;
    static constexpr float kSideMoveSpeed = 210.0f;
    static constexpr float kGravityAccel = 1200.0f;
    static constexpr float kJumpForce = 480.0f;
    static constexpr float kMaxFallSpeed = 900.0f;
    static constexpr float kMaxVerticalSubStep = 8.0f;
    static constexpr float kCoyoteTimeDuration = 0.15f;
    static constexpr float kJumpBufferDuration = 0.12f;

    static constexpr float kGamepadDeadzone = 0.2f;
    static constexpr int kMaxPlayers = 4;

    static constexpr float kCameraLerpSpeed = 6.0f;

    static constexpr float kInitialPollution = 35.0f;
    static constexpr float kInitialCleanEnergy = 0.0f;
    static constexpr float kInitialDirtyEnergy = 0.0f;
    static constexpr float kMinEcoValue = 0.0f;
    static constexpr float kMaxEcoValue = 100.0f;

    static constexpr float kSolarEfficiencyThreshold = 0.80f;
    static constexpr float kWaterDensity = 1000.0f;
    static constexpr float kGravityWater = 9.81f;
    static constexpr float kHydroEfficiencyThreshold = 0.80f;
};

// Opaque-Box Primitives & Enums (NO dependency on src/core/*.h or raylib.h)
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
    bool operator==(const Vec2&) const = default;
};

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    bool operator==(const Rect&) const = default;
};

struct ColorRgba {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
    bool operator==(const ColorRgba&) const = default;
};

enum class TileKind : int {
    Walkable = 0,
    Wall = 1,
    StructurePortal = 2,
    ExitPortal = 3,
    HiddenBridge = 4
};

enum class SceneMode { Overworld, Structure };

enum class ModeId { None = 0, Overworld = 1, Structure = 2 };

enum class MachineKind { SolarArray, HydroTurbine };

enum class MachineStatus { Broken, CleanActive, BypassedDirty };

enum class OverlayPhase {
    Phase1_Minimal = 1,
    Phase2_Telemetry = 2,
    Phase3_EngineOverlay = 3
};

// Input & Probe Structs
struct InputFrame {
    bool active = false;
    bool use_digital_keys = false;
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    float move_x = 0.0f;
    float move_y = 0.0f;
    bool gamepad_connected = false;
    float gamepad_axis_x = 0.0f;
    float gamepad_axis_y = 0.0f;
    bool jump_pressed = false;
    bool jump_held = false;
    bool interact_pressed = false;
    bool scan_pressed = false;
    bool confirm_pressed = false;
    bool cancel_pressed = false;
    bool bypass_pressed = false;
    float slider_delta = 0.0f;
    bool toggle_overlay_phase = false;
};

struct RawKeyboardState {
    bool w = false;
    bool a = false;
    bool s = false;
    bool d = false;
    bool z = false;
    bool q = false;
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool space = false;
    bool e = false;
    bool f = false;
    bool r = false;
    bool right_shift = false;
    bool enter = false;
    bool right_ctrl = false;
    bool period = false;
    bool tab = false;
};

struct RawGamepadState {
    bool available = false;
    float axis_x = 0.0f;
    float axis_y = 0.0f;
    bool jump_pressed = false;
    bool interact_pressed = false;
    bool scan_pressed = false;
};

struct RawInputSnapshot {
    RawKeyboardState kb{};
    std::array<RawGamepadState, 4> pads{};
};

struct FilteredInput {
    float move_x = 0.0f;
    float move_y = 0.0f;
    bool jump_down = false;
    bool jump_pressed = false;
    bool interact_pressed = false;
    bool scan_pressed = false;
};

// Process & Build Inspection Results
struct CommandResult {
    int exit_code = -1;
    bool signaled = false;
    std::string stdout_text;
    std::string stderr_text;
};

struct ProcessRunResult {
    int exit_code = -1;
    bool signaled = false;
    std::string stdout_text;
    std::string stderr_text;
    bool timed_out = false;
};

struct BuildContractReport {
    bool cmake_file_exists = false;
    bool has_game_core_target = false;
    bool has_eco_proto_target = false;
    bool has_eco_tests_target = false;
    bool core_free_of_raylib_windowing = false;
    bool no_external_physics_libs = false;
    bool lib_game_core_exists = false;
    bool eco_proto_exists = false;
    bool window_config_1280x720_60fps_msaa = false;
};

// F03–F06 Probe Snapshots
struct FixedLoopTrace {
    int total_steps = 0;
    float remaining_accumulator = 0.0f;
    std::vector<float> step_dts;
};

struct StateTransitionReport {
    ModeId mode_after_request_mid_update = ModeId::None;
    ModeId mode_after_tick = ModeId::None;
    bool old_destroyed_mid_update = false;
    int exit_count = 0;
    int enter_count = 0;
    int updates_executed = 0;
    int draws_executed = 0;
    float persisted_pollution = 0.0f;
    float persisted_clean_energy = 0.0f;
};

struct SimSnapshot {
    ModeId mode = ModeId::Overworld;
    int active_player_count = 0;
    std::vector<float> player_x;
    std::vector<float> player_y;
    float pollution = 0.0f;
    float clean_energy = 0.0f;
};

// F06–F25 Unified Simulation Snapshots
struct PlayerSnapshot {
    int player_id = 0;
    bool active = false;
    Vec2 pos{};
    Vec2 size{};
    Vec2 vel{};
    float y_velocity = 0.0f;
    float gravity_accel = 0.0f;
    float jump_force = 0.0f;
    bool is_grounded = false;
    float coyote_timer = 0.0f;
    float jump_buffer_timer = 0.0f;
    ColorRgba color{};
};

struct CameraSnapshot {
    Vec2 target{};
    Vec2 offset{};
    float zoom = 1.0f;
};

struct StepResult {
    int fixed_steps_executed = 0;
    float remaining_accumulator = 0.0f;
    SceneMode current_mode = SceneMode::Overworld;
    bool transition_occurred = false;
};

struct EcoSnapshot {
    float pollution_level = 0.0f;
    float clean_energy = 0.0f;
    float dirty_energy = 0.0f;
    OverlayPhase ui_phase = OverlayPhase::Phase1_Minimal;
    bool architect_recruited = false;
    ColorRgba sample_white_tint{};
    float smog_alpha = 0.0f;
};

struct MachineSnapshot {
    MachineKind kind = MachineKind::SolarArray;
    MachineStatus status = MachineStatus::Broken;
    Vec2 pos{};
    float trigger_radius = 0.0f;
    bool prompt_visible = false;
    std::string prompt_text;
    float current_param = 0.0f;
    float target_param = 0.0f;
    float live_efficiency = 0.0f;
};

struct CalibrationUiSnapshot {
    bool is_open = false;
    MachineKind target_machine = MachineKind::SolarArray;
    float slider_value = 0.0f;
    float preview_efficiency = 0.0f;
    bool can_confirm_clean = false;
    std::string bypass_label;
};

struct ScannerSnapshot {
    bool active = false;
    bool has_target = false;
    MachineKind target_kind = MachineKind::SolarArray;
    float measured_target_value = 0.0f;
    float measured_distance = 0.0f;
    float measured_efficiency = 0.0f;
};

struct OverlayRenderData {
    OverlayPhase phase = OverlayPhase::Phase1_Minimal;
    bool show_raw_numbers = false;
    bool show_velocity_vectors = false;
    bool show_aabb_hitboxes = false;
    bool show_irradiance_rays = false;
    bool show_trigger_radii = false;
};

struct NpcSnapshot {
    Vec2 pos{};
    bool recruited = false;
    bool hidden_bridge_revealed = false;
    std::string last_dialogue;
};

class IE2EAdapter {
public:
    virtual ~IE2EAdapter() = default;

    // Backend identification
    [[nodiscard]] virtual bool IsLiveBackend() const = 0;
    [[nodiscard]] virtual bool IsContractCheckOnly() const { return !IsLiveBackend(); }

    // Paths, files, processes & build inspection (F01, F02, F05)
    [[nodiscard]] virtual std::string ProjectRoot() const = 0;
    [[nodiscard]] virtual std::string BuildDir() const = 0;
    [[nodiscard]] virtual bool FileExists(const std::string& rel_path) const = 0;
    [[nodiscard]] virtual std::string ReadProjectFile(const std::string& rel_path) const = 0;
    [[nodiscard]] virtual bool HasDisplayServer() const = 0;
    [[nodiscard]] virtual CommandResult RunProcess(
        const std::string& exe,
        const std::vector<std::string>& args,
        const std::vector<std::string>& unset_env = {},
        const std::vector<std::pair<std::string, std::string>>& set_env = {},
        int timeout_ms = 30000) const = 0;
    [[nodiscard]] virtual CommandResult RunCMakeConfigure(const std::string& build_dir) const = 0;
    [[nodiscard]] virtual CommandResult RunCMakeBuild(
        const std::string& build_dir, const std::string& target = "") const = 0;
    [[nodiscard]] virtual CommandResult RunCTest(
        const std::string& build_dir, int jobs = 1) const = 0;
    [[nodiscard]] virtual CommandResult RunEcoProtoHeadlessE2E(
        bool strip_display_env = true) const = 0;
    [[nodiscard]] virtual CommandResult RunEcoProtoSmokeFrames(int frames) const = 0;
    [[nodiscard]] virtual CommandResult RunEcoProtoArgs(
        const std::vector<std::string>& args,
        bool strip_display_env = false,
        const std::vector<std::pair<std::string, std::string>>& set_env = {}) const = 0;
    [[nodiscard]] virtual ProcessRunResult RunEcoProto(
        const std::vector<std::string>& args, int timeout_ms = 5000) const = 0;
    [[nodiscard]] virtual bool CoreSourcesHaveZeroRaylibOrGlIncludes() const = 0;
    [[nodiscard]] virtual bool CoreBinaryHasZeroDisplayLibs(
        const std::string& target_rel_path) const = 0;
    [[nodiscard]] virtual BuildContractReport InspectBuildAndArtifacts() const = 0;

    // F03 Fixed-timestep probe
    [[nodiscard]] virtual FixedLoopTrace StepFixedLoop(
        const std::vector<float>& frame_deltas) const = 0;

    // F04 GameState & GameStateManager probes
    [[nodiscard]] virtual bool IsGameStateAbstractAndPolymorphic() const = 0;
    [[nodiscard]] virtual StateTransitionReport RunStateLifecycleProbe(
        ModeId initial,
        const std::vector<ModeId>& mid_update_requests,
        int extra_ticks,
        float dt = ContractConstants::kFixedDt,
        bool mutate_eco = false) const = 0;
    [[nodiscard]] virtual StateTransitionReport RunEmptyManagerProbe(int ticks) const = 0;
    [[nodiscard]] virtual StateTransitionReport RunRapidTransitionSequence(
        const std::vector<ModeId>& sequence) const = 0;

    // F05 Headless simulation probe
    [[nodiscard]] virtual SimSnapshot RunHeadlessSimulationSteps(
        int steps,
        float dt,
        const std::array<RawInputSnapshot, 4>& inputs,
        bool query_snapshot_each_step,
        int read_only_repeat_at_end = 0) const = 0;

    // F06 Player & PlayerManager probes
    [[nodiscard]] virtual PlayerSnapshot ConstructPlayerWithId(int raw_id) const = 0;
    virtual void ResetPlayerManager(int active_count) = 0;
    virtual bool TryJoinManagedPlayer(int id) = 0;
    virtual bool SetManagedPlayerActive(int id, bool active) = 0;
    [[nodiscard]] virtual int ManagedActiveCount() const = 0;
    [[nodiscard]] virtual std::vector<PlayerSnapshot> GetManagedPlayers() const = 0;
    virtual void StepManagedPlayers(
        const std::array<RawInputSnapshot, 4>& inputs, float dt) = 0;

    // F07 Input & deadzone probes
    [[nodiscard]] virtual float FilterDeadzone(
        float raw_axis, float deadzone = ContractConstants::kGamepadDeadzone) const = 0;
    [[nodiscard]] virtual FilteredInput PollPlayerInput(
        int player_id,
        const RawKeyboardState& kb,
        const std::array<RawGamepadState, 4>& pads) const = 0;
    [[nodiscard]] virtual Vec2 ApplyGamepadDeadzone(
        float raw_x,
        float raw_y,
        float deadzone = ContractConstants::kGamepadDeadzone) const = 0;
    [[nodiscard]] virtual Vec2 MapDigitalDirections(
        bool left, bool right, bool up, bool down) const = 0;

    // Unified simulation interface (F03-F25: used by E2E-M2, E2E-M3, E2E-M4)
    virtual void ResetSimulation(
        SceneMode initial_mode = SceneMode::Overworld, int active_players = 1) = 0;
    virtual StepResult AdvanceFrame(
        float raw_frame_dt, const std::array<InputFrame, 4>& inputs) = 0;
    virtual StepResult StepFixed(int steps, const std::array<InputFrame, 4>& inputs) = 0;
    virtual void RequestModeTransition(SceneMode target) = 0;
    [[nodiscard]] virtual SceneMode CurrentMode() const = 0;

    [[nodiscard]] virtual PlayerSnapshot CreateStandalonePlayer(int raw_player_id) const = 0;
    virtual void SetPlayerActive(int player_id, bool active) = 0;
    virtual void SetPlayerPosition(int player_id, Vec2 pos) = 0;
    virtual void SetPlayerVelocity(int player_id, Vec2 vel, float y_velocity = 0.0f) = 0;
    [[nodiscard]] virtual PlayerSnapshot GetPlayer(int player_id) const = 0;
    [[nodiscard]] virtual int ActivePlayerCount() const = 0;

    // Tilemap, top-down, camera, side-scroll & portals (F09-F17)
    virtual void LoadCustomTilemap(
        SceneMode mode, const std::vector<std::vector<int>>& grid) = 0;
    [[nodiscard]] virtual TileKind GetTileAt(SceneMode mode, int row, int col) const = 0;
    [[nodiscard]] virtual int TilemapRows(SceneMode mode) const = 0;
    [[nodiscard]] virtual int TilemapCols(SceneMode mode) const = 0;
    [[nodiscard]] virtual bool IsTileSolid(SceneMode mode, int row, int col) const = 0;
    [[nodiscard]] virtual CameraSnapshot GetCamera() const = 0;
    [[nodiscard]] virtual Vec2 NormalizeMovement(Vec2 raw) const = 0;

    // EcoSystem, Machines, Calibration, Math, Bypass, Tint, Scanner, Overlays, NPC (F08, F18-F25)
    [[nodiscard]] virtual EcoSnapshot GetEcoSystem() const = 0;
    virtual void SetPollutionLevel(float value) = 0;
    [[nodiscard]] virtual ColorRgba ComputeTintColor(
        ColorRgba base, float pollution_level) const = 0;
    [[nodiscard]] virtual float ComputeSmogAlpha(float pollution_level) const = 0;
    [[nodiscard]] virtual MachineSnapshot GetMachine(MachineKind kind) const = 0;
    virtual void SetMachinePosition(MachineKind kind, Vec2 pos) = 0;
    [[nodiscard]] virtual CalibrationUiSnapshot GetCalibrationUi() const = 0;
    virtual void SetCalibrationParameter(float value) = 0;
    virtual bool ConfirmCalibrationClean() = 0;
    virtual void TriggerCombustionBypass() = 0;
    virtual void CancelCalibration() = 0;
    [[nodiscard]] virtual float ComputeSolarEfficiency(
        float panel_deg, float sun_deg) const = 0;
    [[nodiscard]] virtual float ComputeHydroEfficiency(
        float head_m, float target_head_m, float tolerance_pa) const = 0;
    virtual ScannerSnapshot TriggerEmpiricalScan(int player_id) = 0;
    [[nodiscard]] virtual OverlayRenderData GetOverlayData() const = 0;
    virtual bool RequestOverlayPhase(OverlayPhase phase) = 0;
    virtual NpcSnapshot InteractArchitectNpc(int player_id) = 0;
    [[nodiscard]] virtual NpcSnapshot GetArchitectNpc() const = 0;
};

[[nodiscard]] std::unique_ptr<IE2EAdapter> CreateAdapter(
    const std::string& repo_root, const std::string& build_dir);

} // namespace eco::e2e
