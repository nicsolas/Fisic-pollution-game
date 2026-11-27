#include "app/overworld_state.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <memory>

#include "app/calibration_ui.h"
#include "app/game_state_manager.h"
#include "app/render/palette.h"
#include "app/render/particles.h"
#include "app/render/procedural_art.h"
#include "app/render/world_entities.h"
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
    ctx_.world.EnsureInitialized();
    if (ctx_.world.loaded_from_save) {
        ctx_.world.loaded_from_save = false;
        ctx_.world.current_mode = eco::GameMode::Overworld;
        ctx_.world.UpdateProximityPrompts();
    } else if (ctx_.world.returning_from_structure) {
        ctx_.world.ExitToOverworld();
        ctx_.world.returning_from_structure = false;
    } else {
        ctx_.world.current_mode = eco::GameMode::Overworld;
        ctx_.world.overworld.OnEnter(
            ctx_.players,
            false,
            ctx_.ecosystem.architect_recruited,
            ctx_.world.active_structure);
        ctx_.world.current_zone = ctx_.world.overworld.active_zone;
        ctx_.world.UpdateProximityPrompts();
    }
}

void OverworldState::OnExit() {}

void OverworldState::Update(float dt) {
    // Se e' attivo un intramezzo tra zone, consenti lo skip rapido con SPAZIO/INVIO/E
    if (ctx_.world.interlude.active) {
        const eco::PlayerInput& p0 = ctx_.input.Get(0);
        const bool skip_requested =
            p0.jump_pressed || p0.confirm_pressed || p0.interact_pressed ||
            ctx_.input.advance_dialogue_pressed;
        if (skip_requested && ctx_.world.interlude.CanSkip()) {
            ctx_.world.SkipZoneInterlude();
        }
        (void)ctx_.world.StepWorld(ctx_.input, dt);
        return;
    }

    const eco::WorldStepResult step = ctx_.world.StepWorld(ctx_.input, dt);

    if (step.enter_structure_requested) {
        ctx_.world.active_structure = step.target_structure;
        ctx_.world.returning_from_structure = false;
        ctx_.state_manager.RequestTransition(std::make_unique<StructureState>(ctx_));
    } else if (ctx_.input.toggle_state_pressed && !ctx_.world.interlude.active) {
        ctx_.world.returning_from_structure = false;
        ctx_.state_manager.RequestTransition(std::make_unique<StructureState>(ctx_));
    }
}

void OverworldState::Draw() {
    const eco::ZoneEcoStatus eco_status = ctx_.world.ActiveSceneEcoStatus();
    const float local_poll = 1.0f - std::clamp(eco_status.local_vitality, 0.0f, 1.0f);
    const float poll = std::clamp(
        local_poll * 0.68f + ctx_.ecosystem.PollutionRatio() * 0.32f,
        0.0f,
        1.0f);
    ClearBackground(pal::WorldColor(pal::kSickSky, pal::kHealthySky, poll));

    const eco::Tilemap& tilemap = ctx_.world.overworld.tilemap;
    const float half_w = static_cast<float>(eco::kWindowWidth) * 0.5f;
    const float half_h = static_cast<float>(eco::kWindowHeight) * 0.5f;
    const eco::Vec2 raw_cam = ctx_.world.overworld.camera_target;
    const float cam_x = eco::Clamp(
        raw_cam.x,
        half_w,
        std::max(half_w, tilemap.WorldWidth() - half_w));
    const float cam_y = eco::Clamp(
        raw_cam.y,
        half_h,
        std::max(half_h, tilemap.WorldHeight() - half_h));

    // Micro-vibrazione diegetica per Impulso di Prova (RunTrial) o regime RiskyEfficient
    float shake_amp = 0.0f;
    if (ctx_.world.trial_pulse_timer > 0.01f) {
        shake_amp = std::max(shake_amp, 1.55f * std::clamp(ctx_.world.trial_pulse_timer / 1.25f, 0.0f, 1.0f));
    }
    if (ctx_.world.ActiveMachine().approach == eco::CalibrationApproach::RiskyEfficient) {
        shake_amp = std::max(shake_amp, 0.65f);
    }
    const float t = ctx_.world.sim_time;
    const float shake_x = (shake_amp > 0.01f) ? (std::sin(t * 43.0f) * shake_amp) : 0.0f;
    const float shake_y = (shake_amp > 0.01f) ? (std::cos(t * 57.0f) * shake_amp * 0.85f) : 0.0f;

    Camera2D camera{};
    camera.target = Vector2{cam_x + shake_x, cam_y + shake_y};
    camera.offset = Vector2{half_w, half_h};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    BeginMode2D(camera);

    // Sincronizza temporaneamente il target camera per il viewport culling della mappa di zona
    // senza alterare lo stato della simulazione.
    ctx_.world.overworld.camera_target = eco::Vec2{cam_x, cam_y};

    // 1. Mappa procedurale della zona attiva (pavimentazione, canali, meccaniche a terra, varchi)
    DrawOverworldProceduralMap(ctx_.world);

    // 2. Flora diversificata per bioma, colonne, lanterne, affreschi e meccaniche 2D di zona
    DrawOverworldProceduralProps(ctx_.world);

    ctx_.world.overworld.camera_target = raw_cam;

    // 3. Condotte armoniche, monumenti fisici, reliquie collezionabili, NPC e Trama Geodetica
    DrawSceneMachineAndOverlaysWorldSpace(ctx_.world);

    // 4. Tutti gli esploratori attivi (Elena Valli + eventuali colleghi in co-op locale)
    for (const eco::Player* ap : ctx_.players.ActivePlayers()) {
        if (ap != nullptr && ap->active) {
            DrawScientistProtagonist(*ap, ctx_.world);
        }
    }

    // 5. Particelle atmosferiche per distretto, reazione alle meccaniche e burst reliquie
    DrawWorldAtmosphericParticles(ctx_.world, eco::Vec2{cam_x, cam_y});

    EndMode2D();

    // 6. Atmosfera in screen-space (raggi d'alba, foschia/cenere di pece, vignettatura)
    DrawScreenSpaceAtmosphere(ctx_.world);

    // 7. Se e' in corso un intramezzo tra zone, mostra la vignetta panoramica procedurale
    if (ctx_.world.interlude.active) {
        DrawZoneInterludeOverlay(ctx_.world);
        return;
    }

    // 8. Barra superiore HUD, Lente Empirica, Minimappa Overworld e Modal di calibrazione fisica
    DrawHudAndCalibrationModalScreenSpace(ctx_.world, !ctx_.overlay_blocking_input);

    // 9. Layer Narrativo (Obiettivo, Toast di Zona/Reliquia, Dialoghi, Diario [H] / Codex [J], Epilogo)
    DrawStoryLayer(ctx_.world);
}

} // namespace eco::app
