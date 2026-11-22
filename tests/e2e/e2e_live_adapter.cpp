#include "e2e_adapter.hpp"
#include "e2e_paths.hpp"

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
#include "core/tilemap.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace eco::e2e {

namespace {

namespace fs = std::filesystem;

[[nodiscard]] PlayerSnapshot ToPlayerSnapshot(const eco::Player& p) {
    PlayerSnapshot snap;
    snap.player_id = p.player_id;
    snap.active = p.active;
    snap.pos = {p.pos.x, p.pos.y};
    snap.size = {p.size.x, p.size.y};
    snap.vel = {p.vel.x, p.vel.y};
    snap.y_velocity = p.y_velocity;
    snap.gravity_accel = p.gravity_accel;
    snap.jump_force = p.jump_force;
    snap.is_grounded = p.isGrounded;
    snap.coyote_timer = p.coyote_timer;
    snap.jump_buffer_timer = p.jump_buffer_timer;
    snap.color = {p.color.r, p.color.g, p.color.b, p.color.a};
    return snap;
}

[[nodiscard]] std::string ReadFileText(const fs::path& path) {
    std::ifstream ifs(path, std::ios::in | std::ios::binary);
    if (!ifs) {
        return {};
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

[[nodiscard]] bool ContainsCaseInsensitive(std::string_view haystack, std::string_view needle) {
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

[[nodiscard]] bool HasCacheLine(std::string_view cache_text, std::string_view line) {
    std::size_t pos = 0;
    while (pos < cache_text.size()) {
        const std::size_t eol = cache_text.find('\n', pos);
        std::string_view current =
            (eol == std::string_view::npos)
                ? cache_text.substr(pos)
                : cache_text.substr(pos, eol - pos);
        if (!current.empty() && current.back() == '\r') {
            current.remove_suffix(1);
        }
        if (current == line) {
            return true;
        }
        if (eol == std::string_view::npos) {
            break;
        }
        pos = eol + 1;
    }
    return false;
}

[[nodiscard]] FilteredInput ComputeFilteredInput(
    int player_id,
    const RawKeyboardState& kb,
    const std::array<RawGamepadState, 4>& pads) {
    eco::Vec2 kb_axis{0.0f, 0.0f};
    bool jump_pressed = false;
    bool jump_held = false;
    bool interact_pressed = false;
    bool scan_pressed = false;

    // Aligned with src/app/raylib_input.cpp:
    // - P0: WASD/ZQSD movement, Space (jump), E (interact), F/Q (scan; r kept for E2E-M1 compat)
    // - P1: Arrow keys movement, Right Shift (jump), Enter (interact), Right Ctrl/Period (scan)
    if (player_id == 0) {
        const bool left = kb.a || kb.q;
        const bool right = kb.d;
        const bool up = kb.w || kb.z;
        const bool down = kb.s;
        kb_axis.x = eco::CancelOpposingKeys(left, right);
        kb_axis.y = eco::CancelOpposingKeys(up, down);
        jump_pressed = kb.space;
        jump_held = kb.space;
        interact_pressed = kb.e;
        scan_pressed = kb.f || kb.q || kb.r;
    } else if (player_id == 1) {
        kb_axis.x = eco::CancelOpposingKeys(kb.left, kb.right);
        kb_axis.y = eco::CancelOpposingKeys(kb.up, kb.down);
        jump_pressed = kb.right_shift;
        jump_held = kb.right_shift;
        interact_pressed = kb.enter;
        scan_pressed = kb.right_ctrl || kb.period;
    }

    eco::Vec2 gp_axis{0.0f, 0.0f};
    if (player_id >= 0 && player_id < 4 && pads[static_cast<std::size_t>(player_id)].available) {
        const auto& pad = pads[static_cast<std::size_t>(player_id)];
        gp_axis = {pad.axis_x, pad.axis_y};
        jump_pressed = jump_pressed || pad.jump_pressed;
        jump_held = jump_held || pad.jump_pressed;
        interact_pressed = interact_pressed || pad.interact_pressed;
        scan_pressed = scan_pressed || pad.scan_pressed;
    }

    const eco::PlayerInput filtered = eco::FilterPlayerInput(
        kb_axis,
        gp_axis,
        jump_pressed,
        jump_held,
        interact_pressed,
        scan_pressed,
        false,
        false,
        false,
        eco::kGamepadDeadzone);

    FilteredInput out;
    out.move_x = filtered.move_x;
    out.move_y = filtered.move_y;
    out.jump_down = filtered.jump_held;
    out.jump_pressed = filtered.jump_pressed;
    out.interact_pressed = filtered.interact_pressed;
    out.scan_pressed = filtered.scan_pressed;
    return out;
}

class ProbeGameState final : public eco::app::GameState {
public:
    struct Counters {
        int enter_count = 0;
        int exit_count = 0;
        int updates_executed = 0;
        int draws_executed = 0;
        bool destroyed_mid_update = false;
        ModeId current_mode = ModeId::None;
        ModeId mode_after_request_mid_update = ModeId::None;
    };

    ProbeGameState(
        ModeId mode,
        eco::app::GameStateManager& manager,
        eco::EcoSystem& eco,
        std::shared_ptr<Counters> counters,
        std::vector<ModeId> mid_requests,
        bool mutate_eco)
        : mode_(mode),
          manager_(manager),
          eco_(eco),
          counters_(std::move(counters)),
          mid_requests_(std::move(mid_requests)),
          mutate_eco_(mutate_eco) {
        alive_ = std::make_shared<bool>(true);
    }

    ~ProbeGameState() override {
        if (alive_) {
            *alive_ = false;
        }
    }

    void OnEnter() override {
        ++counters_->enter_count;
        counters_->current_mode = mode_;
    }

    void OnExit() override {
        ++counters_->exit_count;
    }

    void Update(float) override {
        ++counters_->updates_executed;
        const std::shared_ptr<bool> local_alive = alive_;
        if (mutate_eco_) {
            eco_.ApplyCleanActivation(25.0f, 10.0f);
            mutate_eco_ = false;
        }
        if (!mid_requests_.empty()) {
            const auto requests = std::move(mid_requests_);
            mid_requests_.clear();
            for (ModeId target : requests) {
                if (target == ModeId::None) {
                    manager_.RequestTransition(std::unique_ptr<eco::app::GameState>{});
                } else {
                    auto next_counters = counters_;
                    auto& mgr = manager_;
                    auto& eco_ref = eco_;
                    manager_.RequestTransition(
                        [target, &mgr, &eco_ref, next_counters]() -> std::unique_ptr<eco::app::GameState> {
                            return std::make_unique<ProbeGameState>(
                                target, mgr, eco_ref, next_counters, std::vector<ModeId>{}, false);
                        });
                }
            }
            counters_->mode_after_request_mid_update = counters_->current_mode;
            if (!(*local_alive)) {
                counters_->destroyed_mid_update = true;
            }
        }
    }

    void Draw() override {
        ++counters_->draws_executed;
    }

    [[nodiscard]] ModeId Mode() const noexcept { return mode_; }

private:
    ModeId mode_;
    eco::app::GameStateManager& manager_;
    eco::EcoSystem& eco_;
    std::shared_ptr<Counters> counters_;
    std::vector<ModeId> mid_requests_;
    bool mutate_eco_ = false;
    std::shared_ptr<bool> alive_;
};

class LiveE2EAdapter final : public IE2EAdapter {
public:
    LiveE2EAdapter(std::string repo_root, std::string build_dir)
        : repo_root_(repo_root.empty() ? std::string(kDefaultProjectRoot) : std::move(repo_root)),
          build_dir_(build_dir.empty() ? std::string(kDefaultGameBuildDir) : std::move(build_dir)) {}

    [[nodiscard]] bool IsLiveBackend() const override { return true; }
    [[nodiscard]] std::string ProjectRoot() const override { return repo_root_; }
    [[nodiscard]] std::string BuildDir() const override { return build_dir_; }

    [[nodiscard]] bool FileExists(const std::string& rel_path) const override {
        std::error_code ec;
        const fs::path p = fs::path(rel_path).is_absolute()
                               ? fs::path(rel_path)
                               : (fs::path(repo_root_) / rel_path);
        return fs::exists(p, ec) && fs::is_regular_file(p, ec);
    }

    [[nodiscard]] std::string ReadProjectFile(const std::string& rel_path) const override {
        const fs::path p = fs::path(rel_path).is_absolute()
                               ? fs::path(rel_path)
                               : (fs::path(repo_root_) / rel_path);
        return ReadFileText(p);
    }

    [[nodiscard]] bool HasDisplayServer() const override {
#if defined(__unix__) || defined(__APPLE__)
        bool wayland_supported = true;
        bool x11_supported = true;
        const std::string cmake_cache = ReadFileText(fs::path(build_dir_) / "CMakeCache.txt");
        if (!cmake_cache.empty()) {
            if (HasCacheLine(cmake_cache, "GLFW_BUILD_X11:BOOL=OFF")) {
                x11_supported = false;
            }
            if (HasCacheLine(cmake_cache, "GLFW_BUILD_WAYLAND:BOOL=OFF")) {
                wayland_supported = false;
            }
        }

        const char* wayland = std::getenv("WAYLAND_DISPLAY");
        const char* xdg = std::getenv("XDG_RUNTIME_DIR");
        if (wayland_supported && wayland != nullptr && wayland[0] != '\0' &&
            xdg != nullptr && xdg[0] != '\0') {
            const fs::path sock = fs::path(xdg) / wayland;
            struct stat st {};
            if (::stat(sock.c_str(), &st) == 0 && S_ISSOCK(st.st_mode)) {
                return true;
            }
        }
        const char* display = std::getenv("DISPLAY");
        if (x11_supported && display != nullptr && display[0] != '\0') {
            // Check local X11 unix socket if display begins with ':'
            if (display[0] == ':') {
                std::string num;
                bool all_digits = true;
                for (std::size_t i = 1; display[i] != '\0' && display[i] != '.'; ++i) {
                    if (!std::isdigit(static_cast<unsigned char>(display[i]))) {
                        all_digits = false;
                        break;
                    }
                    num.push_back(display[i]);
                }
                if (all_digits && !num.empty()) {
                    const fs::path x11_sock = fs::path("/tmp/.X11-unix") / ("X" + num);
                    struct stat st {};
                    if (::stat(x11_sock.c_str(), &st) == 0 && S_ISSOCK(st.st_mode)) {
                        return true;
                    }
                }
            } else {
                return true;
            }
        }
        return false;
#else
        return true;
#endif
    }

    [[nodiscard]] CommandResult RunProcess(
        const std::string& exe,
        const std::vector<std::string>& args,
        const std::vector<std::string>& unset_env = {},
        const std::vector<std::pair<std::string, std::string>>& set_env = {},
        int timeout_ms = 30000) const override {
        CommandResult result;
#if defined(__unix__) || defined(__APPLE__)
        int stdout_pipe[2]{-1, -1};
        int stderr_pipe[2]{-1, -1};
        if (::pipe(stdout_pipe) != 0 || ::pipe(stderr_pipe) != 0) {
            if (stdout_pipe[0] != -1) { ::close(stdout_pipe[0]); ::close(stdout_pipe[1]); }
            if (stderr_pipe[0] != -1) { ::close(stderr_pipe[0]); ::close(stderr_pipe[1]); }
            return result;
        }

        const pid_t pid = ::fork();
        if (pid < 0) {
            ::close(stdout_pipe[0]); ::close(stdout_pipe[1]);
            ::close(stderr_pipe[0]); ::close(stderr_pipe[1]);
            return result;
        }

        if (pid == 0) {
            ::dup2(stdout_pipe[1], STDOUT_FILENO);
            ::dup2(stderr_pipe[1], STDERR_FILENO);
            ::close(stdout_pipe[0]);
            ::close(stdout_pipe[1]);
            ::close(stderr_pipe[0]);
            ::close(stderr_pipe[1]);

            for (const auto& name : unset_env) {
                ::unsetenv(name.c_str());
            }
            for (const auto& [k, v] : set_env) {
                ::setenv(k.c_str(), v.c_str(), 1);
            }

            std::vector<std::string> storage;
            storage.reserve(1 + args.size());
            storage.push_back(exe);
            for (const auto& a : args) {
                storage.push_back(a);
            }
            std::vector<char*> argv_vec;
            argv_vec.reserve(storage.size() + 1);
            for (auto& s : storage) {
                argv_vec.push_back(s.data());
            }
            argv_vec.push_back(nullptr);

            if (exe.find('/') != std::string::npos) {
                ::execv(exe.c_str(), argv_vec.data());
            } else {
                ::execvp(exe.c_str(), argv_vec.data());
            }
            ::_exit(127);
        }

        ::close(stdout_pipe[1]);
        ::close(stderr_pipe[1]);

        ::fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK);
        ::fcntl(stderr_pipe[0], F_SETFL, O_NONBLOCK);

        bool out_open = true;
        bool err_open = true;
        bool timed_out = false;
        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

        while (out_open || err_open) {
            std::array<pollfd, 2> fds{};
            nfds_t nfds = 0;
            if (out_open) {
                fds[nfds].fd = stdout_pipe[0];
                fds[nfds].events = POLLIN | POLLHUP;
                ++nfds;
            }
            if (err_open) {
                fds[nfds].fd = stderr_pipe[0];
                fds[nfds].events = POLLIN | POLLHUP;
                ++nfds;
            }

            const auto now = std::chrono::steady_clock::now();
            if (now >= deadline) {
                timed_out = true;
                ::kill(pid, SIGKILL);
                break;
            }
            const int remaining_ms = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count());
            const int poll_rc = ::poll(fds.data(), nfds, std::min(remaining_ms, 200));
            if (poll_rc < 0 && errno == EINTR) {
                continue;
            }

            auto drain_fd = [](int fd, std::string& dst, bool& is_open) {
                char buf[4096];
                while (true) {
                    const ssize_t n = ::read(fd, buf, sizeof(buf));
                    if (n > 0) {
                        dst.append(buf, static_cast<std::size_t>(n));
                    } else if (n == 0) {
                        is_open = false;
                        break;
                    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    } else if (errno == EINTR) {
                        continue;
                    } else {
                        is_open = false;
                        break;
                    }
                }
            };

            if (out_open) drain_fd(stdout_pipe[0], result.stdout_text, out_open);
            if (err_open) drain_fd(stderr_pipe[0], result.stderr_text, err_open);
        }

        ::close(stdout_pipe[0]);
        ::close(stderr_pipe[0]);

        int status = 0;
        while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) {
        }
        if (timed_out) {
            result.signaled = true;
            result.exit_code = -1;
        } else if (WIFEXITED(status)) {
            result.exit_code = WEXITSTATUS(status);
            result.signaled = false;
        } else if (WIFSIGNALED(status)) {
            result.signaled = true;
            result.exit_code = 128 + WTERMSIG(status);
        }
#else
        (void)exe; (void)args; (void)unset_env; (void)set_env; (void)timeout_ms;
#endif
        return result;
    }

    [[nodiscard]] CommandResult RunCMakeConfigure(const std::string& target_build_dir) const override {
        std::vector<std::string> args = {"-S", repo_root_, "-B", target_build_dir};
        const fs::path cached_raylib = fs::path(build_dir_) / "_deps" / "raylib-src";
        std::error_code ec;
        if (fs::exists(cached_raylib, ec) && fs::is_directory(cached_raylib, ec)) {
            args.push_back("-DFETCHCONTENT_SOURCE_DIR_RAYLIB=" + cached_raylib.string());
        }
        return RunProcess("cmake", args, {}, {}, 60000);
    }

    [[nodiscard]] CommandResult RunCMakeBuild(
        const std::string& target_build_dir, const std::string& target = "") const override {
        std::vector<std::string> args = {"--build", target_build_dir};
        if (!target.empty()) {
            args.push_back("--target");
            args.push_back(target);
        }
        return RunProcess("cmake", args, {}, {}, 120000);
    }

    [[nodiscard]] CommandResult RunCTest(
        const std::string& target_build_dir, int jobs = 1) const override {
        std::vector<std::string> args = {
            "--test-dir", target_build_dir, "--output-on-failure", "--no-tests=error"
        };
        if (jobs > 1) {
            args.push_back("-j");
            args.push_back(std::to_string(jobs));
        }
        return RunProcess("ctest", args, {}, {}, 60000);
    }

    [[nodiscard]] std::string ResolveEcoProtoBinary() const {
        if (const char* env_bin = std::getenv("E2E_ECO_PROTO_BIN");
            env_bin != nullptr && env_bin[0] != '\0') {
            return std::string(env_bin);
        }
        return (fs::path(build_dir_) / "eco_proto").string();
    }

    [[nodiscard]] CommandResult RunEcoProtoHeadlessE2E(
        bool strip_display_env = true) const override {
        return RunEcoProtoArgs({"--headless-e2e"}, strip_display_env, {});
    }

    [[nodiscard]] CommandResult RunEcoProtoSmokeFrames(int frames) const override {
        return RunEcoProtoArgs({"--smoke-frames", std::to_string(frames)}, false, {});
    }

    [[nodiscard]] CommandResult RunEcoProtoArgs(
        const std::vector<std::string>& args,
        bool strip_display_env = false,
        const std::vector<std::pair<std::string, std::string>>& set_env = {}) const override {
        std::vector<std::string> unset_vars;
        if (strip_display_env) {
            unset_vars = {"DISPLAY", "WAYLAND_DISPLAY", "XDG_RUNTIME_DIR"};
        }
        return RunProcess(ResolveEcoProtoBinary(), args, unset_vars, set_env, 15000);
    }

    [[nodiscard]] ProcessRunResult RunEcoProto(
        const std::vector<std::string>& args, int timeout_ms = 5000) const override {
        const CommandResult cr = RunProcess(ResolveEcoProtoBinary(), args, {}, {}, timeout_ms);
        ProcessRunResult pr;
        pr.exit_code = cr.exit_code;
        pr.signaled = cr.signaled;
        pr.stdout_text = cr.stdout_text;
        pr.stderr_text = cr.stderr_text;
        pr.timed_out = cr.signaled && cr.exit_code == -1;
        return pr;
    }

    [[nodiscard]] bool CoreSourcesHaveZeroRaylibOrGlIncludes() const override {
        const fs::path core_dir = fs::path(repo_root_) / "src" / "core";
        std::error_code ec;
        if (!fs::exists(core_dir, ec) || !fs::is_directory(core_dir, ec)) {
            return false;
        }
        static constexpr std::array<const char*, 7> kForbidden = {
            "\"raylib.h\"", "<raylib.h>", "\"GL/", "<GL/",
            "InitWindow", "BeginDrawing", "DrawRectangle"
        };
        for (const auto& entry : fs::recursive_directory_iterator(core_dir, ec)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            const auto ext = entry.path().extension().string();
            if (ext != ".h" && ext != ".hpp" && ext != ".cpp") {
                continue;
            }
            const std::string content = ReadFileText(entry.path());
            for (const char* token : kForbidden) {
                if (content.find(token) != std::string::npos) {
                    return false;
                }
            }
        }
        return true;
    }

    [[nodiscard]] bool CoreBinaryHasZeroDisplayLibs(
        const std::string& target_rel_path) const override {
        fs::path bin_path = fs::path(build_dir_) / target_rel_path;
        std::error_code ec;
        if (!fs::exists(bin_path, ec) && target_rel_path == "eco_e2e_runner") {
            bin_path = fs::read_symlink("/proc/self/exe", ec);
        }
        if (!fs::exists(bin_path, ec)) {
            return false;
        }
        const CommandResult ldd_res = RunProcess("ldd", {bin_path.string()}, {}, {}, 10000);
        if (ldd_res.exit_code != 0) {
            return false;
        }
        static constexpr std::array<const char*, 5> kDisplayLibs = {
            "libraylib", "libGL.so", "libEGL.so", "libX11", "libwayland-client"
        };
        for (const char* lib : kDisplayLibs) {
            if (ldd_res.stdout_text.find(lib) != std::string::npos) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] BuildContractReport InspectBuildAndArtifacts() const override {
        BuildContractReport rep;
        const std::string cmake_txt = ReadProjectFile("CMakeLists.txt");
        rep.cmake_file_exists = !cmake_txt.empty();
        rep.has_game_core_target = cmake_txt.find("game_core") != std::string::npos;
        rep.has_eco_proto_target = cmake_txt.find("eco_proto") != std::string::npos;
        rep.has_eco_tests_target = cmake_txt.find("eco_tests") != std::string::npos;
        rep.core_free_of_raylib_windowing = CoreSourcesHaveZeroRaylibOrGlIncludes();

        static constexpr std::array<const char*, 6> kPhysicsLibs = {
            "box2d", "bullet", "chipmunk", "physx", "jolt", "ode"
        };
        rep.no_external_physics_libs = true;
        for (const char* p : kPhysicsLibs) {
            if (ContainsCaseInsensitive(cmake_txt, p)) {
                rep.no_external_physics_libs = false;
                break;
            }
        }

        std::error_code ec;
        rep.lib_game_core_exists =
            fs::exists(fs::path(build_dir_) / "libgame_core.a", ec) ||
            fs::exists(fs::path(build_dir_) / "src" / "core" / "libgame_core.a", ec);
        rep.eco_proto_exists = fs::exists(fs::path(ResolveEcoProtoBinary()), ec);

        const std::string main_cpp = ReadProjectFile("src/app/main.cpp");
        const auto msaa_pos = main_cpp.find("FLAG_MSAA_4X_HINT");
        const auto init_pos = main_cpp.find("InitWindow");
        rep.window_config_1280x720_60fps_msaa =
            msaa_pos != std::string::npos &&
            init_pos != std::string::npos &&
            msaa_pos < init_pos &&
            (main_cpp.find("1280") != std::string::npos || main_cpp.find("kWindowWidth") != std::string::npos) &&
            (main_cpp.find("720") != std::string::npos || main_cpp.find("kWindowHeight") != std::string::npos) &&
            (main_cpp.find("60") != std::string::npos || main_cpp.find("kTargetFps") != std::string::npos);

        return rep;
    }

    // F03 Fixed-timestep probe
    [[nodiscard]] FixedLoopTrace StepFixedLoop(
        const std::vector<float>& frame_deltas) const override {
        FixedLoopTrace trace;
        eco::FixedTimestepStepper stepper;
        for (float raw_dt : frame_deltas) {
            const int steps = stepper.Accumulate(raw_dt);
            for (int s = 0; s < steps; ++s) {
                trace.step_dts.push_back(stepper.fixed_dt);
                ++trace.total_steps;
            }
        }
        trace.remaining_accumulator = stepper.accumulator;
        return trace;
    }

    // F04 GameState & GameStateManager probes
    [[nodiscard]] bool IsGameStateAbstractAndPolymorphic() const override {
        return std::is_abstract_v<eco::app::GameState> &&
               std::has_virtual_destructor_v<eco::app::GameState>;
    }

    [[nodiscard]] StateTransitionReport RunStateLifecycleProbe(
        ModeId initial,
        const std::vector<ModeId>& mid_update_requests,
        int extra_ticks,
        float dt = ContractConstants::kFixedDt,
        bool mutate_eco = false) const override {
        auto counters = std::make_shared<ProbeGameState::Counters>();
        eco::EcoSystem eco;
        eco::app::GameStateManager manager;

        if (initial != ModeId::None) {
            manager.RequestTransition(std::make_unique<ProbeGameState>(
                initial, manager, eco, counters, mid_requests_or_empty(mid_update_requests), mutate_eco));
            manager.ApplyPendingTransition();
        }

        // First tick executes mid-update transition requests if any
        manager.Update(dt);
        manager.Draw();

        for (int t = 0; t < extra_ticks; ++t) {
            manager.Update(dt);
            manager.Draw();
        }

        StateTransitionReport rep;
        rep.mode_after_request_mid_update = counters->mode_after_request_mid_update;
        rep.mode_after_tick = counters->current_mode;
        rep.old_destroyed_mid_update = counters->destroyed_mid_update;
        rep.exit_count = counters->exit_count;
        rep.enter_count = counters->enter_count;
        rep.updates_executed = counters->updates_executed;
        rep.draws_executed = counters->draws_executed;
        rep.persisted_pollution = eco.pollution_level;
        rep.persisted_clean_energy = eco.clean_energy;
        return rep;
    }

    [[nodiscard]] StateTransitionReport RunEmptyManagerProbe(int ticks) const override {
        eco::app::GameStateManager manager;
        for (int i = 0; i < ticks; ++i) {
            manager.Update(ContractConstants::kFixedDt);
            manager.Draw();
        }
        StateTransitionReport rep;
        rep.mode_after_tick = manager.HasActiveState() ? ModeId::Overworld : ModeId::None;
        return rep;
    }

    [[nodiscard]] StateTransitionReport RunRapidTransitionSequence(
        const std::vector<ModeId>& sequence) const override {
        auto counters = std::make_shared<ProbeGameState::Counters>();
        eco::EcoSystem eco;
        eco::app::GameStateManager manager;

        for (ModeId m : sequence) {
            if (m == ModeId::None) {
                manager.RequestTransition(std::unique_ptr<eco::app::GameState>{});
            } else {
                manager.RequestTransition(std::make_unique<ProbeGameState>(
                    m, manager, eco, counters, std::vector<ModeId>{}, false));
            }
            manager.Update(ContractConstants::kFixedDt);
        }

        StateTransitionReport rep;
        rep.mode_after_tick = counters->current_mode;
        rep.exit_count = counters->exit_count;
        rep.enter_count = counters->enter_count;
        rep.updates_executed = counters->updates_executed;
        rep.persisted_pollution = eco.pollution_level;
        rep.persisted_clean_energy = eco.clean_energy;
        return rep;
    }

    // F05 Headless simulation probe
    [[nodiscard]] SimSnapshot RunHeadlessSimulationSteps(
        int steps,
        float dt,
        const std::array<RawInputSnapshot, 4>& inputs,
        bool query_snapshot_each_step,
        int read_only_repeat_at_end = 0) const override {
        eco::PlayerManager pm;
        eco::EcoSystem eco;
        // Enable all players that have any input or active flag via drop-in
        eco::AllPlayersInput all_in{};
        for (int id = 0; id < 4; ++id) {
            const FilteredInput fi = ComputeFilteredInput(
                id,
                inputs[static_cast<std::size_t>(id)].kb,
                inputs[static_cast<std::size_t>(id)].pads);
            all_in.players[static_cast<std::size_t>(id)].move_x = fi.move_x;
            all_in.players[static_cast<std::size_t>(id)].move_y = fi.move_y;
            all_in.players[static_cast<std::size_t>(id)].jump_pressed = fi.jump_pressed;
            all_in.players[static_cast<std::size_t>(id)].jump_held = fi.jump_down;
            all_in.players[static_cast<std::size_t>(id)].interact_pressed = fi.interact_pressed;
            all_in.players[static_cast<std::size_t>(id)].scan_pressed = fi.scan_pressed;
        }
        pm.ProcessDropIn(all_in, {0.0f, 0.0f});

        auto capture = [&]() -> SimSnapshot {
            SimSnapshot snap;
            snap.mode = ModeId::Overworld;
            snap.active_player_count = pm.ActiveCount();
            snap.pollution = eco.pollution_level;
            snap.clean_energy = eco.clean_energy;
            for (int id = 0; id < 4; ++id) {
                const auto* p = pm.GetPlayer(id);
                snap.player_x.push_back(p ? p->pos.x : 0.0f);
                snap.player_y.push_back(p ? p->pos.y : 0.0f);
            }
            return snap;
        };

        for (int step = 0; step < steps; ++step) {
            for (int id = 0; id < 4; ++id) {
                auto* p = pm.GetPlayer(id);
                if (p != nullptr && p->active) {
                    const FilteredInput fi = ComputeFilteredInput(
                        id,
                        inputs[static_cast<std::size_t>(id)].kb,
                        inputs[static_cast<std::size_t>(id)].pads);
                    const eco::Vec2 dir = eco::Vec2{fi.move_x, fi.move_y}.Normalized();
                    p->vel = dir * eco::kTopDownSpeed;
                    p->pos += p->vel * dt;
                }
            }
            if (query_snapshot_each_step) {
                [[maybe_unused]] const auto mid_snap = capture();
            }
        }

        SimSnapshot final_snap = capture();
        for (int r = 0; r < read_only_repeat_at_end; ++r) {
            final_snap = capture();
        }
        return final_snap;
    }

    // F06 Player & PlayerManager probes
    [[nodiscard]] PlayerSnapshot ConstructPlayerWithId(int raw_id) const override {
        const eco::Player p(raw_id);
        return ToPlayerSnapshot(p);
    }

    void ResetPlayerManager(int active_count) override {
        managed_players_ = eco::PlayerManager();
        for (int i = 0; i < eco::kMaxPlayers; ++i) {
            if (i < active_count) {
                managed_players_.EnsureActive(i, {0.0f, 0.0f});
            } else {
                managed_players_.LeavePlayer(i);
            }
        }
    }

    bool TryJoinManagedPlayer(int id) override {
        return managed_players_.JoinPlayer(id, {0.0f, 0.0f});
    }

    bool SetManagedPlayerActive(int id, bool active) override {
        if (active) {
            return managed_players_.JoinPlayer(id, {0.0f, 0.0f});
        }
        return managed_players_.LeavePlayer(id);
    }

    [[nodiscard]] int ManagedActiveCount() const override {
        return managed_players_.ActiveCount();
    }

    [[nodiscard]] std::vector<PlayerSnapshot> GetManagedPlayers() const override {
        std::vector<PlayerSnapshot> out;
        out.reserve(eco::kMaxPlayers);
        for (const auto& p : managed_players_.Slots()) {
            out.push_back(ToPlayerSnapshot(p));
        }
        return out;
    }

    void StepManagedPlayers(
        const std::array<RawInputSnapshot, 4>& inputs, float dt) override {
        for (int id = 0; id < eco::kMaxPlayers; ++id) {
            auto* p = managed_players_.GetPlayer(id);
            if (p != nullptr && p->active) {
                const FilteredInput fi = ComputeFilteredInput(
                    id,
                    inputs[static_cast<std::size_t>(id)].kb,
                    inputs[static_cast<std::size_t>(id)].pads);
                const eco::Vec2 dir = eco::Vec2{fi.move_x, fi.move_y}.Normalized();
                p->vel = dir * eco::kTopDownSpeed;
                p->pos += p->vel * dt;
            }
        }
    }

    // F07 Input & deadzone probes
    [[nodiscard]] float FilterDeadzone(
        float raw_axis, float deadzone = ContractConstants::kGamepadDeadzone) const override {
        return eco::ApplyAnalogDeadzone(raw_axis, deadzone);
    }

    [[nodiscard]] FilteredInput PollPlayerInput(
        int player_id,
        const RawKeyboardState& kb,
        const std::array<RawGamepadState, 4>& pads) const override {
        return ComputeFilteredInput(player_id, kb, pads);
    }

    [[nodiscard]] Vec2 ApplyGamepadDeadzone(
        float raw_x,
        float raw_y,
        float deadzone = ContractConstants::kGamepadDeadzone) const override {
        return {
            eco::ApplyAnalogDeadzone(raw_x, deadzone),
            eco::ApplyAnalogDeadzone(raw_y, deadzone)
        };
    }

    [[nodiscard]] Vec2 MapDigitalDirections(
        bool left, bool right, bool up, bool down) const override {
        return {
            eco::CancelOpposingKeys(left, right),
            eco::CancelOpposingKeys(up, down)
        };
    }

    // Unified Simulation Interface (M1-M4 wired to eco::WorldContext)
    void ResetSimulation(
        SceneMode initial_mode = SceneMode::Overworld, int active_players = 1) override {
        world_.Reset();
        world_.EnsureInitialized();
        pending_mode_.reset();
        sim_stepper_.Reset();

        world_.current_mode = (initial_mode == SceneMode::Overworld)
                                  ? eco::GameMode::Overworld
                                  : eco::GameMode::Structure;

        for (int i = 0; i < eco::kMaxPlayers; ++i) {
            if (i < active_players) {
                world_.players.EnsureActive(i, {0.0f, 0.0f});
            } else {
                world_.players.LeavePlayer(i);
            }
        }

        if (world_.current_mode == eco::GameMode::Overworld) {
            world_.overworld.OnEnter(world_.players, false, world_.ecosystem.architect_recruited);
        } else {
            world_.structure.OnEnter(world_.players, world_.ecosystem.architect_recruited);
        }

        if (active_players <= 0) {
            for (int i = 0; i < eco::kMaxPlayers; ++i) {
                world_.players.LeavePlayer(i);
            }
        }
        world_.UpdateProximityPrompts();
    }

    StepResult AdvanceFrame(
        float raw_frame_dt, const std::array<InputFrame, 4>& inputs) override {
        const int steps = sim_stepper_.Accumulate(raw_frame_dt);
        StepResult res = StepFixed(steps, inputs);
        res.remaining_accumulator = sim_stepper_.accumulator;
        return res;
    }

    StepResult StepFixed(int steps, const std::array<InputFrame, 4>& inputs) override {
        StepResult res;
        res.fixed_steps_executed = steps;
        res.remaining_accumulator = sim_stepper_.accumulator;
        world_.EnsureInitialized();

        for (int s = 0; s < steps; ++s) {
            eco::AllPlayersInput all_in{};
            for (int id = 0; id < eco::kMaxPlayers; ++id) {
                const auto* p = world_.players.GetPlayer(id);
                if (p == nullptr || !p->active) {
                    continue;
                }
                const auto& in = inputs[static_cast<std::size_t>(id)];
                eco::Vec2 kb{0.0f, 0.0f};
                if (in.use_digital_keys) {
                    kb = {
                        eco::CancelOpposingKeys(in.left, in.right),
                        eco::CancelOpposingKeys(in.up, in.down)
                    };
                } else {
                    kb = {in.move_x, in.move_y};
                }
                eco::Vec2 gp{0.0f, 0.0f};
                if (in.gamepad_connected) {
                    gp = {
                        eco::ApplyAnalogDeadzone(in.gamepad_axis_x, eco::kGamepadDeadzone),
                        eco::ApplyAnalogDeadzone(in.gamepad_axis_y, eco::kGamepadDeadzone)
                    };
                }
                eco::PlayerInput pi{};
                pi.move_x = kb.x + gp.x;
                pi.move_y = kb.y + gp.y;
                pi.jump_pressed = in.jump_pressed;
                pi.jump_held = in.jump_held;
                pi.interact_pressed = in.interact_pressed;
                pi.scan_pressed = in.scan_pressed;
                pi.toggle_overlay_pressed = in.toggle_overlay_phase;
                pi.cancel_pressed = in.cancel_pressed;
                pi.confirm_pressed = in.confirm_pressed;
                pi.bypass_pressed = in.bypass_pressed;
                pi.slider_delta = in.slider_delta;
                all_in.players[static_cast<std::size_t>(id)] = pi;
            }

            const eco::WorldStepResult wres = world_.StepWorld(all_in, sim_stepper_.fixed_dt);

            std::optional<SceneMode> target_transition = pending_mode_;
            pending_mode_.reset();
            if (!target_transition.has_value()) {
                if (wres.enter_structure_requested) {
                    target_transition = SceneMode::Structure;
                } else if (wres.exit_to_overworld_requested) {
                    target_transition = SceneMode::Overworld;
                }
            }

            if (target_transition.has_value()) {
                if (*target_transition == SceneMode::Structure) {
                    world_.current_mode = eco::GameMode::Structure;
                    world_.returning_from_structure = false;
                    world_.structure.OnEnter(world_.players, world_.ecosystem.architect_recruited);
                } else {
                    world_.current_mode = eco::GameMode::Overworld;
                    world_.returning_from_structure = true;
                    world_.overworld.OnEnter(
                        world_.players, true, world_.ecosystem.architect_recruited);
                    world_.returning_from_structure = false;
                }
                world_.UpdateProximityPrompts();
                res.transition_occurred = true;
            }
        }

        res.current_mode = (world_.current_mode == eco::GameMode::Overworld)
                               ? SceneMode::Overworld
                               : SceneMode::Structure;
        return res;
    }

    void RequestModeTransition(SceneMode target) override { pending_mode_ = target; }
    [[nodiscard]] SceneMode CurrentMode() const override {
        return (world_.current_mode == eco::GameMode::Overworld)
                   ? SceneMode::Overworld
                   : SceneMode::Structure;
    }

    [[nodiscard]] PlayerSnapshot CreateStandalonePlayer(int raw_player_id) const override {
        return ConstructPlayerWithId(raw_player_id);
    }

    void SetPlayerActive(int player_id, bool active) override {
        if (active) {
            const eco::Vec2 spawn = (world_.current_mode == eco::GameMode::Overworld)
                                        ? world_.overworld.spawn_point
                                        : world_.structure.spawn_point;
            world_.players.JoinPlayer(player_id, spawn);
        } else {
            world_.players.LeavePlayer(player_id);
        }
        world_.UpdateProximityPrompts();
    }

    void SetPlayerPosition(int player_id, Vec2 pos) override {
        if (auto* p = world_.players.GetPlayer(player_id)) {
            p->pos = {pos.x, pos.y};
            if (world_.current_mode == eco::GameMode::Structure) {
                p->isGrounded = eco::IsSupportedByFloor(
                    p->pos,
                    p->size,
                    world_.structure.tilemap,
                    world_.ecosystem.architect_recruited);
                p->coyote_timer = p->isGrounded ? eco::kCoyoteTimeDuration : 0.0f;
            } else {
                world_.overworld.portal_cooldown_timer = 0.0f;
                world_.overworld.portal_reentry_locked = false;
            }
            world_.UpdateProximityPrompts();
        }
    }

    void SetPlayerVelocity(int player_id, Vec2 vel, float y_velocity = 0.0f) override {
        if (auto* p = world_.players.GetPlayer(player_id)) {
            p->vel = {vel.x, vel.y};
            p->y_velocity = y_velocity;
            if (y_velocity != 0.0f) {
                p->isGrounded = false;
            }
        }
    }

    [[nodiscard]] PlayerSnapshot GetPlayer(int player_id) const override {
        const auto* p = world_.players.GetPlayer(player_id);
        return p ? ToPlayerSnapshot(*p) : PlayerSnapshot{};
    }

    [[nodiscard]] int ActivePlayerCount() const override {
        return world_.players.ActiveCount();
    }

    void LoadCustomTilemap(SceneMode mode, const std::vector<std::vector<int>>& grid) override {
        world_.EnsureInitialized();
        if (mode == SceneMode::Overworld) {
            world_.overworld.tilemap = eco::Tilemap(grid, eco::kTileSize);
            world_.overworld.initialized = true;
        } else {
            world_.structure.tilemap = eco::Tilemap(grid, eco::kTileSize);
            world_.structure.initialized = true;
        }
    }

    [[nodiscard]] TileKind GetTileAt(SceneMode mode, int row, int col) const override {
        const eco::Tilemap& tm = (mode == SceneMode::Overworld)
                                     ? world_.overworld.tilemap
                                     : world_.structure.tilemap;
        return static_cast<TileKind>(static_cast<int>(tm.GetTile(row, col)));
    }

    [[nodiscard]] int TilemapRows(SceneMode mode) const override {
        const eco::Tilemap& tm = (mode == SceneMode::Overworld)
                                     ? world_.overworld.tilemap
                                     : world_.structure.tilemap;
        return tm.Rows();
    }

    [[nodiscard]] int TilemapCols(SceneMode mode) const override {
        const eco::Tilemap& tm = (mode == SceneMode::Overworld)
                                     ? world_.overworld.tilemap
                                     : world_.structure.tilemap;
        return tm.Cols();
    }

    [[nodiscard]] bool IsTileSolid(SceneMode mode, int row, int col) const override {
        const eco::Tilemap& tm = (mode == SceneMode::Overworld)
                                     ? world_.overworld.tilemap
                                     : world_.structure.tilemap;
        return tm.IsSolid(row, col, world_.ecosystem.architect_recruited);
    }

    [[nodiscard]] CameraSnapshot GetCamera() const override {
        const eco::Vec2 target = world_.ActiveCameraTarget();
        CameraSnapshot snap;
        snap.target = {target.x, target.y};
        snap.offset = {
            static_cast<float>(eco::kWindowWidth) * 0.5f,
            static_cast<float>(eco::kWindowHeight) * 0.5f
        };
        snap.zoom = 1.0f;
        return snap;
    }

    [[nodiscard]] Vec2 NormalizeMovement(Vec2 raw) const override {
        const eco::Vec2 n = eco::NormalizeVector({raw.x, raw.y});
        return {n.x, n.y};
    }

    [[nodiscard]] EcoSnapshot GetEcoSystem() const override {
        EcoSnapshot snap;
        snap.pollution_level = world_.ecosystem.pollution_level;
        snap.clean_energy = world_.ecosystem.clean_energy;
        snap.dirty_energy = world_.ecosystem.dirty_energy;
        snap.ui_phase = static_cast<OverlayPhase>(static_cast<int>(world_.ecosystem.ui_phase));
        snap.architect_recruited = world_.ecosystem.architect_recruited;
        const eco::ColorRgba tinted = world_.ecosystem.TintColor({255, 255, 255, 255});
        snap.sample_white_tint = {tinted.r, tinted.g, tinted.b, tinted.a};
        snap.smog_alpha = world_.ecosystem.SmogAlpha();
        return snap;
    }

    void SetPollutionLevel(float value) override {
        if (!std::isfinite(value)) {
            return;
        }
        world_.ecosystem.pollution_level =
            eco::Clamp(value, eco::kMinEcoValue, eco::kMaxEcoValue);
    }

    [[nodiscard]] ColorRgba ComputeTintColor(
        ColorRgba base, float pollution_level) const override {
        eco::EcoSystem tmp;
        tmp.pollution_level = std::isfinite(pollution_level)
                                  ? eco::Clamp(pollution_level, eco::kMinEcoValue, eco::kMaxEcoValue)
                                  : eco::kInitialPollution;
        const eco::ColorRgba t = tmp.TintColor({base.r, base.g, base.b, base.a});
        return {t.r, t.g, t.b, t.a};
    }

    [[nodiscard]] float ComputeSmogAlpha(float pollution_level) const override {
        eco::EcoSystem tmp;
        tmp.pollution_level = std::isfinite(pollution_level)
                                  ? eco::Clamp(pollution_level, eco::kMinEcoValue, eco::kMaxEcoValue)
                                  : eco::kInitialPollution;
        return tmp.SmogAlpha();
    }

    [[nodiscard]] MachineSnapshot GetMachine(MachineKind kind) const override {
        const eco::MachineType core_type =
            (kind == MachineKind::SolarArray)
                ? eco::MachineType::SolarArray
                : eco::MachineType::HydroTurbine;
        const eco::Machine& m = world_.GetMachine(core_type);
        MachineSnapshot snap;
        snap.kind = kind;
        snap.status = static_cast<MachineStatus>(static_cast<int>(m.state));
        snap.pos = {m.pos.x, m.pos.y};
        snap.trigger_radius = m.trigger_radius;
        snap.prompt_visible = m.prompt_visible;
        snap.prompt_text = m.prompt_visible ? std::string(eco::kMachineInteractPrompt) : std::string{};
        snap.current_param = m.current_param;
        snap.target_param = m.target_param;
        snap.live_efficiency = m.LiveEfficiency();
        return snap;
    }

    void SetMachinePosition(MachineKind kind, Vec2 pos) override {
        const eco::MachineType core_type =
            (kind == MachineKind::SolarArray)
                ? eco::MachineType::SolarArray
                : eco::MachineType::HydroTurbine;
        world_.GetMachine(core_type).pos = {pos.x, pos.y};
        world_.UpdateProximityPrompts();
    }

    [[nodiscard]] CalibrationUiSnapshot GetCalibrationUi() const override {
        CalibrationUiSnapshot snap;
        snap.is_open = world_.calibration_modal.is_open;
        snap.target_machine =
            (world_.calibration_modal.target_machine == eco::MachineType::SolarArray)
                ? MachineKind::SolarArray
                : MachineKind::HydroTurbine;
        snap.slider_value = world_.calibration_modal.slider_value;
        snap.preview_efficiency = world_.calibration_modal.preview_efficiency;
        snap.can_confirm_clean = world_.calibration_modal.can_confirm_clean;
        snap.bypass_label = std::string(eco::kCombustionBypassLabel);
        return snap;
    }

    void SetCalibrationParameter(float value) override {
        world_.calibration_modal.SetParameter(value, world_.ActiveMachine());
    }

    bool ConfirmCalibrationClean() override {
        return world_.calibration_modal.ConfirmClean(world_.ActiveMachine(), world_.ecosystem);
    }

    void TriggerCombustionBypass() override {
        world_.calibration_modal.TriggerBypass(world_.ActiveMachine(), world_.ecosystem);
    }

    void CancelCalibration() override {
        world_.calibration_modal.Cancel();
    }

    [[nodiscard]] float ComputeSolarEfficiency(float panel_deg, float sun_deg) const override {
        return eco::ComputeSolarEfficiency(panel_deg, sun_deg);
    }

    [[nodiscard]] float ComputeHydroEfficiency(
        float head_m, float target_head_m, float tolerance_pa) const override {
        return eco::ComputeHydroEfficiency(head_m, target_head_m, tolerance_pa);
    }

    ScannerSnapshot TriggerEmpiricalScan(int player_id) override {
        const eco::EmpiricalScanReading r = world_.TriggerScan(player_id);
        ScannerSnapshot snap;
        snap.active = r.active;
        snap.has_target = r.has_target;
        snap.target_kind = (r.target_type == eco::MachineType::SolarArray)
                               ? MachineKind::SolarArray
                               : MachineKind::HydroTurbine;
        snap.measured_target_value = r.measured_target_value;
        snap.measured_distance = r.measured_distance;
        snap.measured_efficiency = r.measured_efficiency;
        return snap;
    }

    [[nodiscard]] OverlayRenderData GetOverlayData() const override {
        const eco::OverlayVisibility ov = eco::ComputeOverlayVisibility(world_.ecosystem);
        OverlayRenderData out;
        out.phase = static_cast<OverlayPhase>(static_cast<int>(ov.phase));
        out.show_raw_numbers = ov.show_raw_numbers;
        out.show_velocity_vectors = ov.show_velocity_vectors;
        out.show_aabb_hitboxes = ov.show_aabb_hitboxes;
        out.show_irradiance_rays = ov.show_irradiance_rays;
        out.show_trigger_radii = ov.show_trigger_radii;
        return out;
    }

    bool RequestOverlayPhase(OverlayPhase phase) override {
        return eco::TryRequestOverlayPhase(
            world_.ecosystem, static_cast<eco::UiPhase>(static_cast<int>(phase)));
    }

    NpcSnapshot InteractArchitectNpc(int player_id) override {
        if (world_.current_mode == eco::GameMode::Overworld) {
            (void)world_.architect_npc.TryInteract(
                player_id, world_.players, world_.ecosystem);
        }
        return GetArchitectNpc();
    }

    [[nodiscard]] NpcSnapshot GetArchitectNpc() const override {
        NpcSnapshot snap;
        snap.pos = {world_.architect_npc.pos.x, world_.architect_npc.pos.y};
        snap.recruited = world_.architect_npc.recruited;
        snap.hidden_bridge_revealed = world_.architect_npc.hidden_bridge_revealed;
        snap.last_dialogue = world_.architect_npc.last_dialogue;
        return snap;
    }

private:
    static std::vector<ModeId> mid_requests_or_empty(const std::vector<ModeId>& v) { return v; }

    std::string repo_root_;
    std::string build_dir_;
    eco::PlayerManager managed_players_;

    eco::WorldContext world_;
    std::optional<SceneMode> pending_mode_;
    eco::FixedTimestepStepper sim_stepper_;
};

} // namespace

std::unique_ptr<IE2EAdapter> CreateAdapter(
    const std::string& repo_root, const std::string& build_dir) {
    return std::make_unique<LiveE2EAdapter>(repo_root, build_dir);
}

} // namespace eco::e2e
