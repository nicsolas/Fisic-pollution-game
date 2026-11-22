#include <raylib.h>
#include <rlgl.h>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include "app/game_state_manager.h"
#include "app/intro_state.h"
#include "app/overworld_state.h"
#include "app/pause_settings_menu.h"
#include "app/raylib_input.h"
#include "app/render/ui_style.h"
#include "app/structure_state.h"
#include "app/title_state.h"
#include "core/ecosystem.h"
#include "core/input_types.h"
#include "core/math_types.h"
#include "core/player.h"
#include "core/simulation.h"
#include "core/tilemap.h"

namespace {

int RunHeadlessE2E() {
    // Declare domain objects BEFORE state_manager so ~GameStateManager() runs OnExit()
    // while AppContext references remain valid.
    eco::WorldContext world;
    eco::AllPlayersInput input{};
    eco::app::GameStateManager state_manager;
    eco::app::AppContext ctx{
        state_manager,
        world,
        world.ecosystem,
        world.players,
        input,
        false
    };

    state_manager.RequestTransition(std::make_unique<eco::app::OverworldState>(ctx));
    state_manager.ApplyPendingTransition();

    if (!state_manager.HasActiveState() ||
        std::string_view(state_manager.CurrentState()->Name()) != "OverworldState") {
        std::cerr << "[headless-e2e] Failed to enter initial OverworldState\n";
        return 1;
    }

    const eco::Player* p0 = world.players.GetPlayer(0);
    if (p0 == nullptr || !p0->active ||
        world.overworld.tilemap.OverlapsSolid(p0->Bounds(), world.ecosystem.architect_recruited)) {
        std::cerr << "[headless-e2e] P0 initial spawn invalid or overlapping solid tile\n";
        return 1;
    }

    if ((world.overworld.camera_target - p0->Center()).Length() > 1e-4f) {
        std::cerr << "[headless-e2e] Initial camera target does not match P0 center\n";
        return 1;
    }

    // Exercise FixedTimestepStepper
    eco::FixedTimestepStepper stepper;
    const int steps = stepper.Accumulate(eco::kFixedDt * 2.0f);
    if (steps != 2) {
        std::cerr << "[headless-e2e] Expected 2 fixed steps, got " << steps << "\n";
        return 1;
    }

    // Exercise M2 normalized diagonal top-down movement (15 fixed steps in {1, 1})
    const eco::Vec2 start_p0_pos = p0->pos;
    const eco::Vec2 start_cam = world.overworld.camera_target;
    eco::AllPlayersInput diag_sample{};
    diag_sample.players[0].move_x = 1.0f;
    diag_sample.players[0].move_y = 1.0f;

    for (int i = 0; i < 15; ++i) {
        input = diag_sample;
        state_manager.Update(eco::kFixedDt);
    }

    const float expected_dist = 15.0f * eco::kTopDownSpeed * eco::kFixedDt;
    const float actual_dist = (p0->pos - start_p0_pos).Length();
    if (std::abs(actual_dist - expected_dist) > 1e-2f) {
        std::cerr << "[headless-e2e] Diagonal movement not normalized: expected "
                  << expected_dist << " got " << actual_dist << "\n";
        return 1;
    }
    if ((world.overworld.camera_target - start_cam).Length() <= 1.0f) {
        std::cerr << "[headless-e2e] Camera target did not follow P0\n";
        return 1;
    }

    // Exercise M2 wall sliding collision (drive diagonally right+down against pillar at col 12, rows 7..9)
    eco::Player* mutable_p0 = world.players.GetPlayer(0);
    const float pillar_left = world.overworld.tilemap.TileRect(8, 12).Left();
    mutable_p0->pos = {pillar_left - mutable_p0->size.x - 2.0f, world.overworld.tilemap.TileRect(8, 11).Top()};
    const float slide_start_y = mutable_p0->pos.y;
    for (int i = 0; i < 10; ++i) {
        input = diag_sample;
        state_manager.Update(eco::kFixedDt);
    }
    if (world.overworld.tilemap.OverlapsSolid(p0->Bounds(), world.ecosystem.architect_recruited)) {
        std::cerr << "[headless-e2e] P0 penetrated solid tile during wall slide\n";
        return 1;
    }
    if (p0->pos.x > pillar_left - p0->size.x + 1e-4f || p0->pos.y <= slide_start_y + 10.0f) {
        std::cerr << "[headless-e2e] P0 failed to stop at X wall and slide along Y\n";
        return 1;
    }

    // Exercise M2 mid-game 1-4 player drop-in without teleporting P0 back to spawn_point
    const eco::Vec2 p0_pos_before_join = p0->pos;
    eco::AllPlayersInput join_sample{};
    join_sample.players[1].join_pressed = true;
    join_sample.players[2].move_x = 1.0f;
    join_sample.players[3].join_pressed = true;
    input = {};
    eco::LatchFrameInput(input, join_sample);

    for (int s = 0; s < steps; ++s) {
        state_manager.Update(stepper.fixed_dt);
        eco::ConsumePressedEdges(input);
    }

    if (world.players.ActiveCount() != 4) {
        std::cerr << "[headless-e2e] Expected 4 active players after drop-in, got "
                  << world.players.ActiveCount() << "\n";
        return 1;
    }
    if ((p0->pos - p0_pos_before_join).Length() > 1e-4f) {
        std::cerr << "[headless-e2e] Mid-game drop-in teleported existing P0\n";
        return 1;
    }
    for (const eco::Player* ap : world.players.ActivePlayers()) {
        if (world.overworld.tilemap.OverlapsSolid(ap->Bounds(), world.ecosystem.architect_recruited)) {
            std::cerr << "[headless-e2e] Joined player overlaps solid tile\n";
            return 1;
        }
    }

    // Exercise portal-driven transition OverworldState -> StructureState
    const auto portals = world.overworld.tilemap.FindTiles(eco::TileId::StructurePortal);
    if (portals.empty()) {
        std::cerr << "[headless-e2e] Overworld map missing StructurePortal tile\n";
        return 1;
    }
    const eco::Vec2 portal_center = world.overworld.tilemap.TileCenter(portals[0].row, portals[0].col);
    mutable_p0->pos = {
        portal_center.x - mutable_p0->size.x * 0.5f,
        portal_center.y + eco::kTileSize - mutable_p0->size.y * 0.5f
    };
    input = {};
    input.players[0].move_y = -1.0f;
    for (int i = 0; i < 30; ++i) {
        state_manager.Update(eco::kFixedDt);
        if (std::string_view(state_manager.CurrentState()->Name()) == "StructureState") {
            break;
        }
    }
    if (!state_manager.HasActiveState() ||
        std::string_view(state_manager.CurrentState()->Name()) != "StructureState") {
        std::cerr << "[headless-e2e] Walking onto StructurePortal did not transition to StructureState\n";
        return 1;
    }

    // Exercise deferred transition StructureState -> OverworldState & return loop guard
    // First test M3 side-scrolling gravity, jump, coyote time, and HydroTurbine interaction inside StructureState
    mutable_p0 = world.players.GetPlayer(0);
    if (mutable_p0 == nullptr || !mutable_p0->isGrounded) {
        for (int i = 0; i < 30 && mutable_p0 != nullptr && !mutable_p0->isGrounded; ++i) {
            input = {};
            state_manager.Update(eco::kFixedDt);
        }
    }
    if (mutable_p0 == nullptr || !mutable_p0->isGrounded) {
        std::cerr << "[headless-e2e] P0 failed to land on floor in StructureState\n";
        return 1;
    }
    const float grounded_y = mutable_p0->pos.y;
    input = {};
    input.players[0].jump_pressed = true;
    input.players[0].jump_held = true;
    state_manager.Update(eco::kFixedDt);
    if (mutable_p0->isGrounded || mutable_p0->y_velocity >= 0.0f) {
        std::cerr << "[headless-e2e] P0 failed to jump in StructureState\n";
        return 1;
    }
    for (int i = 0; i < 65; ++i) {
        input = {};
        state_manager.Update(eco::kFixedDt);
    }
    if (!mutable_p0->isGrounded || std::abs(mutable_p0->pos.y - grounded_y) > 1e-2f) {
        std::cerr << "[headless-e2e] P0 did not return to grounded floor after jump\n";
        return 1;
    }

    // Exercise M4 HydroTurbine combustion bypass in StructureState
    mutable_p0->pos = world.hydro_machine.pos;
    input = {};
    input.players[0].interact_pressed = true;
    state_manager.Update(eco::kFixedDt);
    if (!world.calibration_modal.is_open) {
        std::cerr << "[headless-e2e] Failed to open HydroTurbine calibration modal\n";
        return 1;
    }
    input = {};
    input.players[0].bypass_pressed = true;
    state_manager.Update(eco::kFixedDt);
    if (world.calibration_modal.is_open ||
        world.hydro_machine.state != eco::MachineState::BypassedDirty ||
        world.ecosystem.dirty_energy <= 0.0f) {
        std::cerr << "[headless-e2e] HydroTurbine combustion bypass failed\n";
        return 1;
    }

    // Transition back to OverworldState
    eco::AllPlayersInput toggle_sample{};
    toggle_sample.toggle_state_pressed = true;
    input = {};
    eco::LatchFrameInput(input, toggle_sample);
    state_manager.Update(eco::kFixedDt);
    eco::ConsumePressedEdges(input);

    if (!state_manager.HasActiveState() ||
        std::string_view(state_manager.CurrentState()->Name()) != "OverworldState") {
        std::cerr << "[headless-e2e] Failed transition StructureState -> OverworldState\n";
        return 1;
    }

    if (world.overworld.tilemap.OverlapsTileId(p0->Bounds(), eco::TileId::StructurePortal)) {
        std::cerr << "[headless-e2e] Return spawn overlaps StructurePortal\n";
        return 1;
    }

    input = {};
    for (int i = 0; i < 5; ++i) {
        state_manager.Update(eco::kFixedDt);
    }
    if (std::string_view(state_manager.CurrentState()->Name()) != "OverworldState") {
        std::cerr << "[headless-e2e] Portal return loop guard failed\n";
        return 1;
    }

    // Exercise M4 Empirical Scanner, SolarArray clean cosine calibration, Architect NPC & 3-Phase UI
    const eco::ColorRgba base_col{80, 180, 120, 255};
    const eco::ColorRgba polluted_tint = world.ecosystem.TintColor(base_col);
    mutable_p0->pos = world.solar_machine.pos;
    input = {};
    input.players[0].scan_pressed = true;
    state_manager.Update(eco::kFixedDt);
    if (!world.last_scan.has_target ||
        std::abs(world.last_scan.measured_target_value - world.solar_machine.target_param) > 1e-3f) {
        std::cerr << "[headless-e2e] Empirical measurement scan failed\n";
        return 1;
    }

    input = {};
    input.players[0].interact_pressed = true;
    state_manager.Update(eco::kFixedDt);
    if (!world.calibration_modal.is_open) {
        std::cerr << "[headless-e2e] Failed to open SolarArray calibration modal\n";
        return 1;
    }
    world.calibration_modal.SetParameter(world.last_scan.measured_target_value, world.solar_machine);
    input = {};
    input.players[0].confirm_pressed = true;
    state_manager.Update(eco::kFixedDt);
    if (world.calibration_modal.is_open ||
        world.solar_machine.state != eco::MachineState::CleanActive ||
        world.ecosystem.clean_energy <= 0.0f ||
        world.ecosystem.ui_phase != eco::UiPhase::Phase2_Telemetry) {
        std::cerr << "[headless-e2e] SolarArray clean calibration failed\n";
        return 1;
    }

    const eco::ColorRgba cleaner_tint = world.ecosystem.TintColor(base_col);
    if (cleaner_tint == polluted_tint) {
        std::cerr << "[headless-e2e] EcoSystem visual tint did not react to clean calibration\n";
        return 1;
    }

    // Recruit Architect NPC to unlock Phase 3 Engine Overlays and HiddenBridge
    mutable_p0->pos = world.architect_npc.pos;
    input = {};
    input.players[0].interact_pressed = true;
    state_manager.Update(eco::kFixedDt);
    if (!world.architect_npc.recruited ||
        !world.ecosystem.architect_recruited ||
        world.ecosystem.ui_phase != eco::UiPhase::Phase3_EngineOverlay) {
        std::cerr << "[headless-e2e] Architect NPC recruitment or Phase 3 unlock failed\n";
        return 1;
    }

    std::cout << "[headless-e2e] PASS: M1-M4 stepper, normalized top-down movement, "
              << "AABB wall slide, camera tracking, 4-player drop-in, "
              << "OverworldState<->StructureState transitions, side-scrolling gravity/jump, "
              << "Solar/Hydro Machine calibration & bypass, EcoSystem tint, Empirical Scanner, "
              << "3-Phase Overlays & Architect NPC verified.\n";
    return 0;
}

struct WindowedOptions {
    int smoke_frames = -1;
    std::string screenshot_file{};
    std::string scene = "title";   // title | intro | overworld | structure
    float pollution = -1.0f;       // <0: valore di default
    bool open_pause = false;
    bool open_settings = false;
    bool demo_mode = false;        // false = Campagna Completa (Atti I-V); true = Demo Classica (Atti I-III)
};

int RunWindowed(const WindowedOptions& opt) {
    const int smoke_frames = opt.smoke_frames;
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(eco::kWindowWidth, eco::kWindowHeight, "EQUILIBRIO - Le Leggi della Valle");
    SetWindowMinSize(640, 360);
    // Disabilita l'uscita automatica con ESC: ora ESC apre il Menu di Pausa / Impostazioni!
    SetExitKey(KEY_NULL);
    SetTargetFPS(eco::kTargetFps);

    eco::app::InitUiFont();
    RenderTexture2D canvas = LoadRenderTexture(eco::kWindowWidth, eco::kWindowHeight);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

    // Inner scope ensures GameStateManager runs OnExit() and destroys active states
    // BEFORE CloseWindow() tears down the Raylib/OpenGL context.
    {
        eco::WorldContext world;
        eco::AllPlayersInput ctx_input{};
        eco::app::GameStateManager state_manager;
        eco::app::AppContext ctx{
            state_manager,
            world,
            world.ecosystem,
            world.players,
            ctx_input,
            false
        };

        eco::app::GraphicsSettings graphics_settings{};
        graphics_settings.demo_mode = opt.demo_mode;
        eco::app::InitDefaultMonitor(graphics_settings);
        eco::app::ApplyGraphicsSettingsToRuntime(graphics_settings);
        eco::app::PauseSettingsMenu pause_menu{};

        world.demo_mode = opt.demo_mode;
        world.EnsureInitialized();
        if (opt.pollution >= 0.0f) {
            world.ecosystem.pollution_level = eco::Clamp(opt.pollution, eco::kMinEcoValue, eco::kMaxEcoValue);
        }
        if (opt.scene == "title") {
            state_manager.RequestTransition(std::make_unique<eco::app::TitleState>(ctx));
        } else if (opt.scene == "intro") {
            state_manager.RequestTransition(std::make_unique<eco::app::IntroState>(ctx));
        } else if (opt.scene == "structure") {
            state_manager.RequestTransition(std::make_unique<eco::app::StructureState>(ctx));
        } else {
            state_manager.RequestTransition(std::make_unique<eco::app::OverworldState>(ctx));
        }
        state_manager.ApplyPendingTransition();

        if (opt.open_settings) {
            pause_menu.OpenSettingsDirect();
        } else if (opt.open_pause) {
            pause_menu.OpenPause();
        }

        eco::FixedTimestepStepper stepper;
        eco::AllPlayersInput pending_input{};
        int frame_count = 0;

        while (!WindowShouldClose() && !ctx.quit_requested &&
               (smoke_frames < 0 || frame_count < smoke_frames)) {
            // Calcola letterbox 16:9 usando GetRenderWidth()/GetRenderHeight() (le reali dimensioni
            // del framebuffer OpenGL, che a differenza di GetScreenWidth() non soffrono del bug di
            // disallineamento di SetWindowSize() su Wayland/X11 in Raylib 5.5).
            const int fb_w = std::max(1, GetRenderWidth());
            const int fb_h = std::max(1, GetRenderHeight());
            const float scale = std::min(
                static_cast<float>(fb_w) / static_cast<float>(eco::kWindowWidth),
                static_cast<float>(fb_h) / static_cast<float>(eco::kWindowHeight));
            const float dest_w = static_cast<float>(eco::kWindowWidth) * scale;
            const float dest_h = static_cast<float>(eco::kWindowHeight) * scale;
            const float dest_x = (static_cast<float>(fb_w) - dest_w) * 0.5f;
            const float dest_y = (static_cast<float>(fb_h) - dest_h) * 0.5f;

            // Mouse: le coordinate di input sono relative alla finestra logica (GetScreenWidth)
            // ma se il compositor non ha ancora notificato il resize usiamo fb_w/fb_h coerenti.
            if (scale > 1e-4f) {
                SetMouseOffset(static_cast<int>(-dest_x), static_cast<int>(-dest_y));
                SetMouseScale(1.0f / scale, 1.0f / scale);
            }

            const bool paused = pause_menu.UpdateAndHandleInput(ctx, graphics_settings);
            state_manager.ApplyPendingTransition();

            if (!paused) {
                const eco::AllPlayersInput sampled = eco::app::PollRaylibInput();
                eco::LatchFrameInput(pending_input, sampled);

                const int steps = stepper.Accumulate(GetFrameTime());
                for (int s = 0; s < steps; ++s) {
                    ctx_input = pending_input;
                    state_manager.Update(stepper.fixed_dt);
                    eco::ConsumePressedEdges(pending_input);
                }
            } else {
                // Evita accumulo di dt durante la pausa
                (void)GetFrameTime();
                pending_input = {};
            }

            // Disegna la scena + overlay menu sulla canvas virtuale 1280x720
            BeginTextureMode(canvas);
            state_manager.Draw();
            pause_menu.DrawOverlay(ctx, graphics_settings);
            EndTextureMode();

            // Presenta la canvas scalata in letterbox sulla finestra (o schermo intero),
            // forzando esplicitamente il viewport e la matrice ortografica di rlgl su (fb_w, fb_h)
            // per neutralizzare qualsiasi disallineamento tra CORE.Window.screen e CORE.Window.render.
            BeginDrawing();
            rlViewport(0, 0, fb_w, fb_h);
            rlMatrixMode(RL_PROJECTION);
            rlLoadIdentity();
            rlOrtho(0.0, static_cast<double>(fb_w), static_cast<double>(fb_h), 0.0, 0.0, 1.0);
            rlMatrixMode(RL_MODELVIEW);
            rlLoadIdentity();

            ClearBackground(Color{6, 10, 16, 255});
            const Rectangle src_rec{
                0.0f,
                0.0f,
                static_cast<float>(canvas.texture.width),
                -static_cast<float>(canvas.texture.height)
            };
            const Rectangle dst_rec{dest_x, dest_y, dest_w, dest_h};
            DrawTexturePro(canvas.texture, src_rec, dst_rec, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
            EndDrawing();

            ++frame_count;
            if (!opt.screenshot_file.empty() && smoke_frames > 0 && frame_count == smoke_frames) {
                // Attenzione: raylib 5.5 salva nella cartella di lavoro corrente (toglie i percorsi dal nome).
                TakeScreenshot(opt.screenshot_file.c_str());
            }
        }
    }

    UnloadRenderTexture(canvas);
    eco::app::ShutdownUiFont();
    CloseWindow();
    return 0;
}

bool ParsePositiveInt(std::string_view text, int& out_value) noexcept {
    if (text.empty()) {
        return false;
    }
    int parsed = 0;
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(begin, end, parsed);
    if (ec != std::errc{} || ptr != end || parsed <= 0) {
        return false;
    }
    out_value = parsed;
    return true;
}

} // namespace

int main(int argc, char** argv) {
    bool headless_e2e = false;
    WindowedOptions opt{};
    int& smoke_frames = opt.smoke_frames;
    bool skip_title = false;
    bool scene_given = false;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == "--headless-e2e") {
            headless_e2e = true;
        } else if (arg == "--smoke-frames") {
            if (i + 1 >= argc || !ParsePositiveInt(argv[i + 1], smoke_frames)) {
                std::cerr << "Usage: eco_proto [--headless-e2e | --smoke-frames N [--screenshot FILE]] [--skip-title] [--scene title|overworld|structure] [--pollution N]\n";
                return 2;
            }
            ++i;
        } else if (arg == "--skip-title") {
            skip_title = true;
        } else if (arg == "--screenshot" && i + 1 < argc) {
            opt.screenshot_file = argv[++i];
        } else if (arg == "--scene" && i + 1 < argc) {
            opt.scene = argv[++i];
            scene_given = true;
            if (opt.scene != "title" && opt.scene != "intro" &&
                opt.scene != "overworld" && opt.scene != "structure") {
                std::cerr << "Scena sconosciuta: " << opt.scene << "\n";
                return 2;
            }
        } else if (arg == "--pollution" && i + 1 < argc) {
            opt.pollution = std::strtof(argv[++i], nullptr);
        } else if (arg == "--open-pause") {
            opt.open_pause = true;
        } else if (arg == "--open-settings") {
            opt.open_settings = true;
        } else if (arg == "--demo") {
            opt.demo_mode = true;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            std::cerr << "Usage: eco_proto [--headless-e2e | --smoke-frames N [--screenshot FILE]] [--skip-title] [--scene title|overworld|structure] [--pollution N]\n";
            return 2;
        }
    }

    if (headless_e2e) {
        return RunHeadlessE2E();
    }

    if (skip_title && !scene_given) {
        opt.scene = "overworld";
    }
    return RunWindowed(opt);
}
