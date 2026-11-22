#include "app/overworld_state.h"

#include <raylib.h>

#include <algorithm>
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
        if (const eco::Player* p0 = ctx_.players.GetPlayer(0); p0 != nullptr && p0->active) {
            ctx_.world.current_zone = ctx_.world.overworld.DistrictAt(p0->Center());
        }
        ctx_.world.UpdateProximityPrompts();
    }
}

void OverworldState::OnExit() {}

void OverworldState::Update(float dt) {
    const eco::WorldStepResult step = ctx_.world.StepWorld(ctx_.input, dt);

    if (step.enter_structure_requested) {
        ctx_.world.active_structure = step.target_structure;
        ctx_.world.returning_from_structure = false;
        ctx_.state_manager.RequestTransition(std::make_unique<StructureState>(ctx_));
    } else if (ctx_.input.toggle_state_pressed) {
        ctx_.world.returning_from_structure = false;
        ctx_.state_manager.RequestTransition(std::make_unique<StructureState>(ctx_));
    }
}

void OverworldState::Draw() {
    const float poll = ctx_.ecosystem.PollutionRatio();
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

    Camera2D camera{};
    camera.target = Vector2{cam_x, cam_y};
    camera.offset = Vector2{half_w, half_h};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    BeginMode2D(camera);

    // Sincronizza temporaneamente il target camera per il viewport culling della mappa 36x50
    // senza alterare lo stato della simulazione.
    ctx_.world.overworld.camera_target = eco::Vec2{cam_x, cam_y};

    // 1. Mappa procedurale della Conca di Meridia (6 distretti, mosaici, canali, Ponte degli Archi, 3 portali)
    DrawOverworldProceduralMap(ctx_.world);

    // 2. Flora diversificata per bioma, colonne, lanterne e stele diegetiche
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

    // 5. Particelle atmosferiche per distretto e burst luminosi delle reliquie
    DrawWorldAtmosphericParticles(ctx_.world, eco::Vec2{cam_x, cam_y});

    EndMode2D();

    // 6. Atmosfera in screen-space (raggi d'alba, foschia/cenere di pece, vignettatura)
    DrawScreenSpaceAtmosphere(ctx_.world);

    // 7. Barra superiore HUD, Lente Empirica, Minimappa Overworld e Modal di calibrazione fisica
    DrawHudAndCalibrationModalScreenSpace(ctx_.world);

    // 8. Layer Narrativo (Obiettivo, Toast di Zona/Reliquia, Dialoghi, Diario [H] / Codex [J], Epilogo)
    DrawStoryLayer(ctx_.world);
}

} // namespace eco::app
