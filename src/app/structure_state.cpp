#include "app/structure_state.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
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
    ctx_.world.EnsureInitialized();
    if (ctx_.world.loaded_from_save) {
        ctx_.world.loaded_from_save = false;
        ctx_.world.current_mode = eco::GameMode::Structure;
        ctx_.world.UpdateProximityPrompts();
    } else {
        ctx_.world.EnterStructure(ctx_.world.active_structure);
    }
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
    const eco::ZoneEcoStatus eco_status = ctx_.world.ActiveSceneEcoStatus();
    const float local_poll = 1.0f - std::clamp(eco_status.local_vitality, 0.0f, 1.0f);
    const float poll = std::clamp(
        local_poll * 0.68f + ctx_.ecosystem.PollutionRatio() * 0.32f,
        0.0f,
        1.0f);
    const pal::StructurePalette& sp =
        pal::StructurePaletteByIndex(static_cast<int>(ctx_.world.active_structure));
    ClearBackground(pal::WorldColor(sp.bg_sick, sp.bg_healthy, poll));

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

    // Micro-vibrazione diegetica per colpo d'ariete / Impulso di Prova o regime RiskyEfficient
    float shake_amp = 0.0f;
    if (ctx_.world.trial_pulse_timer > 0.01f || ctx_.world.structure.puzzle.trial_water_hammer_timer > 0.01f) {
        const float hammer = std::max(
            ctx_.world.trial_pulse_timer / 1.25f,
            ctx_.world.structure.puzzle.trial_water_hammer_timer / 1.40f);
        shake_amp = std::max(shake_amp, 1.85f * std::clamp(hammer, 0.0f, 1.0f));
    }
    if (ctx_.world.ActiveMachine().approach == eco::CalibrationApproach::RiskyEfficient) {
        shake_amp = std::max(shake_amp, 0.80f);
    }
    const float t = ctx_.world.sim_time;
    const float shake_x = (shake_amp > 0.01f) ? (std::sin(t * 47.0f) * shake_amp) : 0.0f;
    const float shake_y = (shake_amp > 0.01f) ? (std::cos(t * 61.0f) * shake_amp * 0.85f) : 0.0f;

    Camera2D camera{};
    camera.target = Vector2{cam_x + shake_x, cam_y + shake_y};
    camera.offset = Vector2{half_w, half_h};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    BeginMode2D(camera);

    // 1. Sfondo in parallasse e architettura 2.5D della struttura corrente
    //    (Cisterne Ipogee, Torre del Pendolo o Camera Geotermica)
    DrawStructureProceduralBackdropAndMap(ctx_.world, eco::Vec2{cam_x, cam_y});

    // 2. Monumento fisico della struttura, reliquie collezionabili, NPC e Trama Geodetica
    DrawSceneMachineAndOverlaysWorldSpace(ctx_.world);

    // 3. Tutti gli esploratori attivi (1-4 giocatori)
    for (const eco::Player* ap : ctx_.players.ActivePlayers()) {
        if (ap != nullptr && ap->active) {
            DrawScientistProtagonist(*ap, ctx_.world);
        }
    }

    // 4. Particelle dedicate alla struttura e burst delle reliquie
    DrawWorldAtmosphericParticles(ctx_.world, eco::Vec2{cam_x, cam_y});

    EndMode2D();

    // 5. Foschia e vignettatura in screen-space
    DrawScreenSpaceAtmosphere(ctx_.world);

    // 6. Barra superiore HUD, Lente Empirica e Modal di calibrazione fisica
    DrawHudAndCalibrationModalScreenSpace(ctx_.world, !ctx_.overlay_blocking_input);

    // 7. Layer Narrativo (Obiettivo, Toast di Struttura/Reliquia, Dialoghi, Diario [H] / Codex [J], Epilogo)
    DrawStoryLayer(ctx_.world);
}

} // namespace eco::app
