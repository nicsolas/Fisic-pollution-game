#include "app/overworld_state.h"

#include <raylib.h>

#include <memory>

#include "app/calibration_ui.h"
#include "app/game_state_manager.h"
#include "app/render/palette.h"
#include "app/render/particles.h"
#include "app/render/procedural_art.h"
#include "app/story_ui.h"
#include "app/structure_state.h"
#include "core/ecosystem.h"
#include "core/input_types.h"
#include "core/math_types.h"
#include "core/player.h"
#include "core/simulation.h"

namespace eco::app {

OverworldState::OverworldState(AppContext& ctx) : ctx_(ctx) {}

void OverworldState::OnEnter() {
    ctx_.world.current_mode = eco::GameMode::Overworld;
    ctx_.world.EnsureInitialized();
    ctx_.world.overworld.OnEnter(
        ctx_.players,
        ctx_.world.returning_from_structure,
        ctx_.ecosystem.architect_recruited);
    ctx_.world.returning_from_structure = false;
    ctx_.world.UpdateProximityPrompts();
}

void OverworldState::OnExit() {}

void OverworldState::Update(float dt) {
    const eco::WorldStepResult step = ctx_.world.StepWorld(ctx_.input, dt);

    if (step.enter_structure_requested || ctx_.input.toggle_state_pressed) {
        ctx_.world.returning_from_structure = false;
        ctx_.state_manager.RequestTransition(std::make_unique<StructureState>(ctx_));
    }
}

void OverworldState::Draw() {
    const float poll = ctx_.ecosystem.PollutionRatio();
    ClearBackground(pal::WorldColor(pal::kSickSky, pal::kHealthySky, poll));

    const eco::Vec2 cam_target = ctx_.world.overworld.camera_target;
    Camera2D camera{};
    camera.target = Vector2{cam_target.x, cam_target.y};
    camera.offset = Vector2{
        static_cast<float>(eco::kWindowWidth) * 0.5f,
        static_cast<float>(eco::kWindowHeight) * 0.5f
    };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    BeginMode2D(camera);

    // 1. Tilemap procedurale (terreno, sentieri, Canale del Crati, Ponte Bucci, rovine 2.5D)
    DrawOverworldProceduralMap(ctx_.world.overworld.tilemap, ctx_.ecosystem, ctx_.world.sim_time);

    // 2. Alberi, bio-cristalli e cartelli diegetici
    DrawOverworldProceduralProps(ctx_.world);

    // 3. Condotte energetiche, impianti (Campo Solare, Risonatore Fotonico), Lia e overlay Fase 3
    DrawSceneMachineAndOverlaysWorldSpace(ctx_.world);

    // 4. Tutti i giocatori attivi (1-4 giocatori in co-op locale, ognuno col suo accento cromatico)
    for (const eco::Player* ap : ctx_.players.ActivePlayers()) {
        if (ap != nullptr && ap->active) {
            DrawScientistProtagonist(*ap, ctx_.world);
        }
    }

    // 5. Particelle atmosferiche in world-space (lucciole, polline, foglie)
    DrawWorldAtmosphericParticles(
        eco::GameMode::Overworld,
        ctx_.world.sim_time,
        ctx_.ecosystem,
        cam_target);

    EndMode2D();

    // 6. Atmosfera in screen-space (raggi solari, foschia/cenere da smog, vignettatura)
    DrawScreenSpaceAtmosphere(eco::GameMode::Overworld, ctx_.world.sim_time, ctx_.ecosystem);

    // 7. Barra superiore HUD, Scanner/Telemetria e Modal di calibrazione
    DrawHudAndCalibrationModalScreenSpace(ctx_.world);

    // 8. Layer Narrativo (Capitolo/Obiettivo in alto a sinistra, Dialoghi in basso, Epilogo)
    DrawStoryLayer(ctx_.world);
}

} // namespace eco::app
