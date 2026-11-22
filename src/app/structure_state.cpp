#include "app/structure_state.h"

#include <raylib.h>

#include <algorithm>
#include <memory>

#include "app/calibration_ui.h"
#include "app/game_state_manager.h"
#include "app/overworld_state.h"
#include "app/render/palette.h"
#include "app/render/particles.h"
#include "app/render/procedural_art.h"
#include "app/story_ui.h"
#include "core/ecosystem.h"
#include "core/input_types.h"
#include "core/math_types.h"
#include "core/player.h"
#include "core/simulation.h"
#include "core/tilemap.h"

namespace eco::app {

StructureState::StructureState(AppContext& ctx) : ctx_(ctx) {}

void StructureState::OnEnter() {
    ctx_.world.current_mode = eco::GameMode::Structure;
    ctx_.world.EnsureInitialized();
    ctx_.world.structure.OnEnter(ctx_.players, ctx_.ecosystem.architect_recruited);
    ctx_.world.UpdateProximityPrompts();
}

void StructureState::OnExit() {}

void StructureState::Update(float dt) {
    for (int id = 0; id < eco::kMaxPlayers; ++id) {
        const eco::PlayerInput& in = ctx_.input.Get(id);
        last_input_axes_[static_cast<std::size_t>(id)] = in.MoveAxis();
    }

    const eco::WorldStepResult step = ctx_.world.StepWorld(ctx_.input, dt);

    if (step.exit_to_overworld_requested || ctx_.input.toggle_state_pressed) {
        ctx_.world.returning_from_structure = true;
        ctx_.state_manager.RequestTransition(std::make_unique<OverworldState>(ctx_));
    }
}

void StructureState::Draw() {
    const float poll = ctx_.ecosystem.PollutionRatio();
    ClearBackground(pal::WorldColor(pal::kSickDamBg, pal::kHealthyDamBg, poll));

    const eco::Tilemap& tilemap = ctx_.world.structure.tilemap;
    const float half_w = static_cast<float>(eco::kWindowWidth) * 0.5f;
    const float half_h = static_cast<float>(eco::kWindowHeight) * 0.5f;
    const eco::Vec2 raw_cam = ctx_.world.structure.camera_target;
    const float cam_x = eco::Clamp(
        raw_cam.x,
        half_w,
        std::max(half_w, tilemap.WorldWidth() - half_w));
    const float cam_y = eco::Clamp(
        raw_cam.y,
        half_h,
        std::max(half_h, tilemap.WorldHeight() - half_h));

    Camera2D camera{};
    camera.target = Vector2{cam_x, cam_y};
    camera.offset = Vector2{half_w, half_h};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    BeginMode2D(camera);

    // 1. Sfondo in parallasse della Diga del Crati, cascate interne e passerelle metalliche
    DrawStructureProceduralBackdropAndMap(
        tilemap,
        ctx_.ecosystem,
        eco::Vec2{cam_x, cam_y},
        ctx_.world.sim_time);

    // 2. Turbina Idroelettrica, serbatoio graduato e overlay Fase 3
    DrawSceneMachineAndOverlaysWorldSpace(ctx_.world);

    // 3. Tutti i giocatori attivi (1-4 giocatori)
    for (const eco::Player* ap : ctx_.players.ActivePlayers()) {
        if (ap != nullptr && ap->active) {
            DrawScientistProtagonist(*ap, ctx_.world);
        }
    }

    // 4. Vapore e spruzzi d'acqua in world-space
    DrawWorldAtmosphericParticles(
        eco::GameMode::Structure,
        ctx_.world.sim_time,
        ctx_.ecosystem,
        eco::Vec2{cam_x, cam_y});

    EndMode2D();

    // 5. Foschia/vignettatura in screen-space
    DrawScreenSpaceAtmosphere(eco::GameMode::Structure, ctx_.world.sim_time, ctx_.ecosystem);

    // 6. Barra superiore HUD, Scanner/Telemetria e Modal di calibrazione
    DrawHudAndCalibrationModalScreenSpace(ctx_.world);

    // 7. Layer Narrativo (Capitolo/Obiettivo, Dialoghi, Epilogo)
    DrawStoryLayer(ctx_.world);
}

} // namespace eco::app
