#include "e2e_adapter.hpp"
#include "e2e_harness.hpp"

#include <memory>
#include <string>

namespace eco::e2e {

namespace {

[[noreturn]] void ThrowContractOnly(const char* op) {
    throw ContractOnlyException(
        std::string("Contract-check backend: live probe '") + op + "' requires live game_core");
}

class ContractCheckAdapter final : public IE2EAdapter {
public:
    ContractCheckAdapter(std::string repo_root, std::string build_dir)
        : repo_root_(std::move(repo_root)), build_dir_(std::move(build_dir)) {}

    [[nodiscard]] bool IsLiveBackend() const override { return false; }

    [[nodiscard]] std::string ProjectRoot() const override { return repo_root_; }
    [[nodiscard]] std::string BuildDir() const override { return build_dir_; }

    [[nodiscard]] bool FileExists(const std::string&) const override {
        ThrowContractOnly("FileExists");
    }
    [[nodiscard]] std::string ReadProjectFile(const std::string&) const override {
        ThrowContractOnly("ReadProjectFile");
    }
    [[nodiscard]] bool HasDisplayServer() const override { return false; }

    [[nodiscard]] CommandResult RunProcess(
        const std::string&,
        const std::vector<std::string>&,
        const std::vector<std::string>&,
        const std::vector<std::pair<std::string, std::string>>&,
        int) const override {
        ThrowContractOnly("RunProcess");
    }
    [[nodiscard]] CommandResult RunCMakeConfigure(const std::string&) const override {
        ThrowContractOnly("RunCMakeConfigure");
    }
    [[nodiscard]] CommandResult RunCMakeBuild(
        const std::string&, const std::string&) const override {
        ThrowContractOnly("RunCMakeBuild");
    }
    [[nodiscard]] CommandResult RunCTest(const std::string&, int) const override {
        ThrowContractOnly("RunCTest");
    }
    [[nodiscard]] CommandResult RunEcoProtoHeadlessE2E(bool) const override {
        ThrowContractOnly("RunEcoProtoHeadlessE2E");
    }
    [[nodiscard]] CommandResult RunEcoProtoSmokeFrames(int) const override {
        ThrowContractOnly("RunEcoProtoSmokeFrames");
    }
    [[nodiscard]] CommandResult RunEcoProtoArgs(
        const std::vector<std::string>&,
        bool,
        const std::vector<std::pair<std::string, std::string>>&) const override {
        ThrowContractOnly("RunEcoProtoArgs");
    }
    [[nodiscard]] ProcessRunResult RunEcoProto(
        const std::vector<std::string>&, int) const override {
        ThrowContractOnly("RunEcoProto");
    }
    [[nodiscard]] bool CoreSourcesHaveZeroRaylibOrGlIncludes() const override {
        ThrowContractOnly("CoreSourcesHaveZeroRaylibOrGlIncludes");
    }
    [[nodiscard]] bool CoreBinaryHasZeroDisplayLibs(const std::string&) const override {
        ThrowContractOnly("CoreBinaryHasZeroDisplayLibs");
    }
    [[nodiscard]] BuildContractReport InspectBuildAndArtifacts() const override {
        ThrowContractOnly("InspectBuildAndArtifacts");
    }

    [[nodiscard]] FixedLoopTrace StepFixedLoop(const std::vector<float>&) const override {
        ThrowContractOnly("StepFixedLoop");
    }

    [[nodiscard]] bool IsGameStateAbstractAndPolymorphic() const override {
        ThrowContractOnly("IsGameStateAbstractAndPolymorphic");
    }
    [[nodiscard]] StateTransitionReport RunStateLifecycleProbe(
        ModeId,
        const std::vector<ModeId>&,
        int,
        float,
        bool) const override {
        ThrowContractOnly("RunStateLifecycleProbe");
    }
    [[nodiscard]] StateTransitionReport RunEmptyManagerProbe(int) const override {
        ThrowContractOnly("RunEmptyManagerProbe");
    }
    [[nodiscard]] StateTransitionReport RunRapidTransitionSequence(
        const std::vector<ModeId>&) const override {
        ThrowContractOnly("RunRapidTransitionSequence");
    }

    [[nodiscard]] SimSnapshot RunHeadlessSimulationSteps(
        int,
        float,
        const std::array<RawInputSnapshot, 4>&,
        bool,
        int) const override {
        ThrowContractOnly("RunHeadlessSimulationSteps");
    }

    [[nodiscard]] PlayerSnapshot ConstructPlayerWithId(int) const override {
        ThrowContractOnly("ConstructPlayerWithId");
    }
    void ResetPlayerManager(int) override { ThrowContractOnly("ResetPlayerManager"); }
    bool TryJoinManagedPlayer(int) override { ThrowContractOnly("TryJoinManagedPlayer"); }
    bool SetManagedPlayerActive(int, bool) override {
        ThrowContractOnly("SetManagedPlayerActive");
    }
    [[nodiscard]] int ManagedActiveCount() const override {
        ThrowContractOnly("ManagedActiveCount");
    }
    [[nodiscard]] std::vector<PlayerSnapshot> GetManagedPlayers() const override {
        ThrowContractOnly("GetManagedPlayers");
    }
    void StepManagedPlayers(const std::array<RawInputSnapshot, 4>&, float) override {
        ThrowContractOnly("StepManagedPlayers");
    }

    [[nodiscard]] float FilterDeadzone(float, float) const override {
        ThrowContractOnly("FilterDeadzone");
    }
    [[nodiscard]] FilteredInput PollPlayerInput(
        int, const RawKeyboardState&, const std::array<RawGamepadState, 4>&) const override {
        ThrowContractOnly("PollPlayerInput");
    }
    [[nodiscard]] Vec2 ApplyGamepadDeadzone(float, float, float) const override {
        ThrowContractOnly("ApplyGamepadDeadzone");
    }
    [[nodiscard]] Vec2 MapDigitalDirections(bool, bool, bool, bool) const override {
        ThrowContractOnly("MapDigitalDirections");
    }

    void ResetSimulation(SceneMode, int) override { ThrowContractOnly("ResetSimulation"); }
    StepResult AdvanceFrame(float, const std::array<InputFrame, 4>&) override {
        ThrowContractOnly("AdvanceFrame");
    }
    StepResult StepFixed(int, const std::array<InputFrame, 4>&) override {
        ThrowContractOnly("StepFixed");
    }
    void RequestModeTransition(SceneMode) override { ThrowContractOnly("RequestModeTransition"); }
    [[nodiscard]] SceneMode CurrentMode() const override { ThrowContractOnly("CurrentMode"); }
    [[nodiscard]] PlayerSnapshot CreateStandalonePlayer(int) const override {
        ThrowContractOnly("CreateStandalonePlayer");
    }
    void SetPlayerActive(int, bool) override { ThrowContractOnly("SetPlayerActive"); }
    void SetPlayerPosition(int, Vec2) override { ThrowContractOnly("SetPlayerPosition"); }
    void SetPlayerVelocity(int, Vec2, float) override { ThrowContractOnly("SetPlayerVelocity"); }
    [[nodiscard]] PlayerSnapshot GetPlayer(int) const override { ThrowContractOnly("GetPlayer"); }
    [[nodiscard]] int ActivePlayerCount() const override {
        ThrowContractOnly("ActivePlayerCount");
    }

    void LoadCustomTilemap(SceneMode, const std::vector<std::vector<int>>&) override {
        ThrowContractOnly("LoadCustomTilemap");
    }
    [[nodiscard]] TileKind GetTileAt(SceneMode, int, int) const override {
        ThrowContractOnly("GetTileAt");
    }
    [[nodiscard]] int TilemapRows(SceneMode) const override { ThrowContractOnly("TilemapRows"); }
    [[nodiscard]] int TilemapCols(SceneMode) const override { ThrowContractOnly("TilemapCols"); }
    [[nodiscard]] bool IsTileSolid(SceneMode, int, int) const override {
        ThrowContractOnly("IsTileSolid");
    }
    [[nodiscard]] CameraSnapshot GetCamera() const override { ThrowContractOnly("GetCamera"); }
    [[nodiscard]] Vec2 NormalizeMovement(Vec2) const override {
        ThrowContractOnly("NormalizeMovement");
    }

    [[nodiscard]] EcoSnapshot GetEcoSystem() const override { ThrowContractOnly("GetEcoSystem"); }
    void SetPollutionLevel(float) override { ThrowContractOnly("SetPollutionLevel"); }
    [[nodiscard]] ColorRgba ComputeTintColor(ColorRgba, float) const override {
        ThrowContractOnly("ComputeTintColor");
    }
    [[nodiscard]] float ComputeSmogAlpha(float) const override {
        ThrowContractOnly("ComputeSmogAlpha");
    }
    [[nodiscard]] MachineSnapshot GetMachine(MachineKind) const override {
        ThrowContractOnly("GetMachine");
    }
    void SetMachinePosition(MachineKind, Vec2) override {
        ThrowContractOnly("SetMachinePosition");
    }
    [[nodiscard]] CalibrationUiSnapshot GetCalibrationUi() const override {
        ThrowContractOnly("GetCalibrationUi");
    }
    void SetCalibrationParameter(float) override {
        ThrowContractOnly("SetCalibrationParameter");
    }
    bool ConfirmCalibrationClean() override { ThrowContractOnly("ConfirmCalibrationClean"); }
    void TriggerCombustionBypass() override { ThrowContractOnly("TriggerCombustionBypass"); }
    void CancelCalibration() override { ThrowContractOnly("CancelCalibration"); }
    [[nodiscard]] float ComputeSolarEfficiency(float, float) const override {
        ThrowContractOnly("ComputeSolarEfficiency");
    }
    [[nodiscard]] float ComputeHydroEfficiency(float, float, float) const override {
        ThrowContractOnly("ComputeHydroEfficiency");
    }
    ScannerSnapshot TriggerEmpiricalScan(int) override {
        ThrowContractOnly("TriggerEmpiricalScan");
    }
    [[nodiscard]] OverlayRenderData GetOverlayData() const override {
        ThrowContractOnly("GetOverlayData");
    }
    bool RequestOverlayPhase(OverlayPhase) override { ThrowContractOnly("RequestOverlayPhase"); }
    NpcSnapshot InteractArchitectNpc(int) override { ThrowContractOnly("InteractArchitectNpc"); }
    [[nodiscard]] NpcSnapshot GetArchitectNpc() const override {
        ThrowContractOnly("GetArchitectNpc");
    }

private:
    std::string repo_root_;
    std::string build_dir_;
};

} // namespace

std::unique_ptr<IE2EAdapter> CreateAdapter(
    const std::string& repo_root, const std::string& build_dir) {
    return std::make_unique<ContractCheckAdapter>(repo_root, build_dir);
}

} // namespace eco::e2e
