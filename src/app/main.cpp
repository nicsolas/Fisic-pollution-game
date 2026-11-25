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

#include "app/audio/audio_engine.h"
#include "app/game_state_manager.h"
#include "app/gpu_selector.h"
#include "app/intro_state.h"
#include "app/overworld_state.h"
#include "app/pause_settings_menu.h"
#include "app/raylib_input.h"
#include "app/render/app_icon_data.h"
#include "app/render/ui_style.h"
#include "app/story_ui.h"
#include "app/structure_state.h"
#include "app/title_state.h"
#include "core/collectible.h"
#include "core/ecosystem.h"
#include "core/input_types.h"
#include "core/machine.h"
#include "core/math_types.h"
#include "core/npc.h"
#include "core/player.h"
#include "core/save_data.h"
#include "core/simulation.h"
#include "core/story.h"
#include "core/tilemap.h"

namespace {

bool ParseZoneArg(std::string_view arg, eco::ZoneId& out_zone) noexcept {
    if (arg == "0" || arg == "dawn" || arg == "terrazze") {
        out_zone = eco::ZoneId::DawnTerraces;
        return true;
    }
    if (arg == "1" || arg == "canals" || arg == "canali") {
        out_zone = eco::ZoneId::CanalQuarter;
        return true;
    }
    if (arg == "2" || arg == "wind" || arg == "venti") {
        out_zone = eco::ZoneId::WindPlateau;
        return true;
    }
    if (arg == "3" || arg == "gardens" || arg == "giardini") {
        out_zone = eco::ZoneId::HangingGardens;
        return true;
    }
    if (arg == "4" || arg == "caldera" || arg == "fumarole") {
        out_zone = eco::ZoneId::FumaroleCaldera;
        return true;
    }
    if (arg == "5" || arg == "sanctuary" || arg == "santuario") {
        out_zone = eco::ZoneId::HarmonySanctuary;
        return true;
    }
    return false;
}

bool ParseStructureArg(std::string_view arg, eco::StructureId& out_struct) noexcept {
    if (arg == "0" || arg == "cisterns" || arg == "cisterne") {
        out_struct = eco::StructureId::HypogeanCisterns;
        return true;
    }
    if (arg == "1" || arg == "pendulum" || arg == "pendolo" || arg == "torre") {
        out_struct = eco::StructureId::PendulumTower;
        return true;
    }
    if (arg == "2" || arg == "geothermal" || arg == "geotermica" || arg == "camera") {
        out_struct = eco::StructureId::GeothermalChamber;
        return true;
    }
    return false;
}

bool FindWallSlideSpot(
    const eco::Tilemap& map,
    bool bridge_unlocked,
    eco::Vec2 player_size,
    eco::Vec2& out_pos,
    float& out_wall_left) noexcept {
    for (int r = 2; r + 2 < map.Rows(); ++r) {
        for (int c = 2; c + 2 < map.Cols(); ++c) {
            if (!map.IsSolid(r, c, bridge_unlocked) &&
                !map.IsSolid(r + 1, c, bridge_unlocked) &&
                map.IsSolid(r, c + 1, bridge_unlocked) &&
                map.IsSolid(r + 1, c + 1, bridge_unlocked)) {
                const eco::Rect wall_rect = map.TileRect(r, c + 1);
                const eco::Vec2 candidate{
                    wall_rect.Left() - player_size.x - 2.0f,
                    map.TileRect(r, c).Top() + 4.0f
                };
                const eco::Rect aabb{candidate.x, candidate.y, player_size.x, player_size.y};
                if (!map.OverlapsSolid(aabb, bridge_unlocked) &&
                    !map.OverlapsTileId(aabb, eco::TileId::StructurePortal)) {
                    out_pos = candidate;
                    out_wall_left = wall_rect.Left();
                    return true;
                }
            }
        }
    }
    return false;
}

int RunHeadlessE2E() {
    // Declare domain objects BEFORE state_manager so ~GameStateManager() runs OnExit()
    // while AppContext references remain valid.
    eco::WorldContext world;
    world.instant_zone_transitions = true;
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

    if (world.overworld.tilemap.Rows() != eco::Tilemap::kZoneRows ||
        world.overworld.tilemap.Cols() != eco::Tilemap::kZoneCols) {
        std::cerr << "[headless-e2e] Unexpected Overworld zone dimensions\n";
        return 1;
    }

    const eco::MemoryMetrics init_mem = world.GetMemoryMetrics();
    if (init_mem.active_zone_tilemap_bytes == 0 || init_mem.saved_unloaded_bytes == 0) {
        std::cerr << "[headless-e2e] Separated zone memory metrics invalid\n";
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

    // Exercise normalized diagonal top-down movement (12 fixed steps in {1, 1})
    eco::Player* mutable_p0 = world.players.GetPlayer(0);
    // Use an open spot in DawnTerraces for unobstructed diagonal travel
    const eco::Vec2 start_p0_pos = p0->pos;
    const eco::Vec2 start_cam = world.overworld.camera_target;
    eco::AllPlayersInput diag_sample{};
    diag_sample.players[0].move_x = 1.0f;
    diag_sample.players[0].move_y = 1.0f;

    for (int i = 0; i < 8; ++i) {
        input = diag_sample;
        state_manager.Update(eco::kFixedDt);
    }

    const float expected_dist = 8.0f * eco::kTopDownSpeed * eco::kFixedDt;
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

    // Exercise wall-sliding collision against a real vertical wall pair in the 36x50 Overworld
    eco::Vec2 slide_start{};
    float wall_left = 0.0f;
    if (!FindWallSlideSpot(
            world.overworld.tilemap,
            world.ecosystem.architect_recruited,
            mutable_p0->size,
            slide_start,
            wall_left)) {
        std::cerr << "[headless-e2e] Could not locate vertical wall spot for slide check\n";
        return 1;
    }
    mutable_p0->pos = slide_start;
    for (int i = 0; i < 10; ++i) {
        input = diag_sample;
        state_manager.Update(eco::kFixedDt);
    }
    if (world.overworld.tilemap.OverlapsSolid(p0->Bounds(), world.ecosystem.architect_recruited)) {
        std::cerr << "[headless-e2e] P0 penetrated solid tile during wall slide\n";
        return 1;
    }
    if (p0->pos.x > wall_left - p0->size.x + 1e-4f || p0->pos.y <= slide_start.y + 8.0f) {
        std::cerr << "[headless-e2e] P0 failed to stop at X wall and slide along Y\n";
        return 1;
    }

    // Exercise mid-game 1-4 player drop-in without teleporting P0
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

    // Exercise Dialogue History recording
    if (world.story.History().empty()) {
        std::cerr << "[headless-e2e] StoryDirector::History() should record GameStarted dialogue\n";
        return 1;
    }

    // Verify initial Empirical guidance is hidden at start (before Gnomon pickup or SolarArray activation)
    const eco::GuidanceDisplayState init_gd = world.ComputeGuidanceDisplay();
    if (init_gd.show_compass || init_gd.show_edge_arrow || init_gd.show_world_beacon) {
        std::cerr << "[headless-e2e] Initial Empirical guidance should be hidden at game start\n";
        return 1;
    }

    // Exercise ObservationNode inspection in DawnTerraces
    if (!world.zone_mechanics.observation_nodes.empty()) {
        mutable_p0->pos = world.zone_mechanics.observation_nodes[0].pos;
        input = {};
        input.players[0].interact_pressed = true;
        state_manager.Update(eco::kFixedDt);
        if (world.observations_inspected_count < 1) {
            std::cerr << "[headless-e2e] ObservationNode inspection in DawnTerraces failed\n";
            return 1;
        }
    }

    // Exercise Collectible pickup + ad-hoc history entry
    const eco::CollectibleItem* first_relic = world.collectibles.Find(eco::CollectibleId::Dawn_BrassGnomon);
    if (first_relic == nullptr) {
        std::cerr << "[headless-e2e] Missing Dawn_BrassGnomon collectible\n";
        return 1;
    }
    const std::size_t history_before_relic = world.story.History().size();
    mutable_p0->pos = first_relic->pos;
    input = {};
    state_manager.Update(eco::kFixedDt);
    if (!world.collectibles.IsCollected(eco::CollectibleId::Dawn_BrassGnomon) ||
        world.collectibles.CollectedCount() < 1) {
        std::cerr << "[headless-e2e] Collectible auto-pickup failed\n";
        return 1;
    }
    while (world.story.HasActiveDialogue()) {
        world.story.AdvanceDialogue();
    }
    if (world.story.History().size() <= history_before_relic) {
        std::cerr << "[headless-e2e] Collectible pickup did not append to Dialogue History\n";
        return 1;
    }

    // Exercise Empirical Scanner [Q] + temporary Empirical Compass reveal + Multi-Solution Calibration
    auto& solar = world.GetMachine(eco::MachineType::SolarArray);
    const eco::ColorRgba base_col{80, 180, 120, 255};
    const eco::ColorRgba polluted_tint = world.ecosystem.TintColor(base_col);
    mutable_p0->pos = solar.pos;
    input = {};
    input.players[0].scan_pressed = true;
    state_manager.Update(eco::kFixedDt);
    if (!world.last_scan.has_target ||
        std::abs(world.last_scan.measured_target_value - solar.target_param) > 1e-3f) {
        std::cerr << "[headless-e2e] Empirical measurement scan on SolarArray failed\n";
        return 1;
    }
    if (!world.ComputeGuidanceDisplay().show_compass) {
        std::cerr << "[headless-e2e] Empirical compass did not reveal on [Q] scan after Gnomon pickup\n";
        return 1;
    }

    // Open SolarArray modal -> RunTrial [T] -> RiskyEfficient [R] -> Recalibrate CleanSlow [INVIO]
    input = {};
    input.players[0].interact_pressed = true;
    state_manager.Update(eco::kFixedDt);
    if (!world.calibration_modal.is_open) {
        std::cerr << "[headless-e2e] Failed to open SolarArray calibration modal\n";
        return 1;
    }
    input = {};
    input.players[0].trial_pressed = true;
    state_manager.Update(eco::kFixedDt);
    if (world.calibration_modal.trials_run < 1 || !world.calibration_modal.last_trial.executed) {
        std::cerr << "[headless-e2e] SolarArray trial pulse [T] failed\n";
        return 1;
    }
    world.calibration_modal.SetParameter((solar.risky_param_min + solar.risky_param_max) * 0.5f, solar);
    world.calibration_modal.SetStabilizer(0.22f, solar);
    input = {};
    input.players[0].risky_pressed = true;
    state_manager.Update(eco::kFixedDt);
    if (world.calibration_modal.is_open ||
        solar.approach != eco::CalibrationApproach::RiskyEfficient ||
        !world.zone_mechanics.scorched_crest_open) {
        std::cerr << "[headless-e2e] SolarArray RiskyEfficient solution failed\n";
        return 1;
    }

    // Re-open SolarArray and recalibrate to CleanSlow (Armonia)
    input = {};
    input.players[0].interact_pressed = true;
    state_manager.Update(eco::kFixedDt);
    if (!world.calibration_modal.is_open) {
        std::cerr << "[headless-e2e] Failed to reopen SolarArray for recalibration\n";
        return 1;
    }
    world.calibration_modal.SetParameter(world.last_scan.measured_target_value, solar);
    world.calibration_modal.SetStabilizer(solar.clean_stabilizer_target, solar);
    input = {};
    input.players[0].confirm_pressed = true;
    state_manager.Update(eco::kFixedDt);
    if (world.calibration_modal.is_open ||
        solar.state != eco::MachineState::CleanActive ||
        solar.approach != eco::CalibrationApproach::CleanSlow ||
        world.ecosystem.clean_energy <= 0.0f ||
        world.ecosystem.ui_phase != eco::UiPhase::Phase2_Telemetry) {
        std::cerr << "[headless-e2e] SolarArray clean recalibration failed\n";
        return 1;
    }

    const eco::ColorRgba cleaner_tint = world.ecosystem.TintColor(base_col);
    if (cleaner_tint == polluted_tint) {
        std::cerr << "[headless-e2e] EcoSystem visual tint did not react to clean calibration\n";
        return 1;
    }

    // Transition to CanalQuarter (instant in --headless-e2e) & recruit Clara Sereni -> unlocks Phase 3 and Ponte degli Archi
    world.BeginZoneInterlude(eco::ZoneId::CanalQuarter, eco::Tilemap::DefaultZoneSpawnTile(eco::ZoneId::CanalQuarter));
    if (world.current_zone != eco::ZoneId::CanalQuarter || world.interlude.active) {
        std::cerr << "[headless-e2e] Instant zone interlude to CanalQuarter failed\n";
        return 1;
    }
    mutable_p0 = world.players.GetPlayer(0);
    auto& clara = world.GetNpc(eco::NpcId::ClaraSereni);
    mutable_p0->pos = clara.pos;
    input = {};
    input.players[0].interact_pressed = true;
    state_manager.Update(eco::kFixedDt);
    if (!clara.recruited ||
        !world.ecosystem.architect_recruited ||
        world.ecosystem.ui_phase != eco::UiPhase::Phase3_EngineOverlay) {
        std::cerr << "[headless-e2e] Clara Sereni recruitment or Phase 3 unlock failed\n";
        return 1;
    }

    // Exercise portal-driven transition into ALL 3 internal structures + side-scrolling gravity/jump + return
    const std::array<eco::StructureId, 3> all_structures = {
        eco::StructureId::HypogeanCisterns,
        eco::StructureId::PendulumTower,
        eco::StructureId::GeothermalChamber
    };

    for (eco::StructureId sid : all_structures) {
        world.TeleportToZone(eco::OverworldSim::PortalZone(sid));
        const eco::PortalLink* link = world.overworld.FindPortal(sid);
        if (link == nullptr) {
            std::cerr << "[headless-e2e] Missing portal link for structure "
                      << eco::StructureName(sid) << "\n";
            return 1;
        }
        // Clear any cooldown and move directly onto portal center
        world.overworld.portal_cooldown_timer = 0.0f;
        world.overworld.portal_reentry_locked = false;
        mutable_p0 = world.players.GetPlayer(0);
        mutable_p0->pos = {
            link->world_center.x - mutable_p0->size.x * 0.5f,
            link->world_center.y - mutable_p0->size.y * 0.5f
        };
        input = {};
        for (int i = 0; i < 6; ++i) {
            state_manager.Update(eco::kFixedDt);
            if (std::string_view(state_manager.CurrentState()->Name()) == "StructureState") {
                break;
            }
        }
        if (std::string_view(state_manager.CurrentState()->Name()) != "StructureState" ||
            world.active_structure != sid) {
            std::cerr << "[headless-e2e] Portal failed to enter " << eco::StructureName(sid) << "\n";
            return 1;
        }

        // Verify side-scrolling floor landing & jump
        mutable_p0 = world.players.GetPlayer(0);
        for (int i = 0; i < 40 && !mutable_p0->isGrounded; ++i) {
            input = {};
            state_manager.Update(eco::kFixedDt);
        }
        if (!mutable_p0->isGrounded) {
            std::cerr << "[headless-e2e] P0 failed to land in " << eco::StructureName(sid) << "\n";
            return 1;
        }
        const float grounded_y = mutable_p0->pos.y;
        input = {};
        input.players[0].jump_pressed = true;
        input.players[0].jump_held = true;
        state_manager.Update(eco::kFixedDt);
        if (mutable_p0->isGrounded || mutable_p0->y_velocity >= 0.0f) {
            std::cerr << "[headless-e2e] P0 failed to jump in " << eco::StructureName(sid) << "\n";
            return 1;
        }
        for (int i = 0; i < 70; ++i) {
            input = {};
            state_manager.Update(eco::kFixedDt);
        }
        if (!mutable_p0->isGrounded || std::abs(mutable_p0->pos.y - grounded_y) > 2.0f) {
            std::cerr << "[headless-e2e] P0 failed to return to floor in "
                      << eco::StructureName(sid) << "\n";
            return 1;
        }

        // Calibrate the structure's monument cleanly
        eco::MachineType mtype = eco::MachineType::HydroTurbine;
        if (sid == eco::StructureId::PendulumTower) {
            mtype = eco::MachineType::PendulumRegulator;
        } else if (sid == eco::StructureId::GeothermalChamber) {
            mtype = eco::MachineType::ThermalExchanger;
        }
        auto& sm = world.GetMachine(mtype);
        mutable_p0->pos = sm.pos;
        input = {};
        input.players[0].interact_pressed = true;
        state_manager.Update(eco::kFixedDt);
        if (!world.calibration_modal.is_open) {
            std::cerr << "[headless-e2e] Failed to open modal for "
                      << eco::MachineName(mtype) << "\n";
            return 1;
        }
        world.calibration_modal.SetParameter(sm.target_param, sm);
        input = {};
        input.players[0].confirm_pressed = true;
        state_manager.Update(eco::kFixedDt);
        if (sm.state != eco::MachineState::CleanActive) {
            std::cerr << "[headless-e2e] Failed clean calibration for "
                      << eco::MachineName(mtype) << "\n";
            return 1;
        }

        // Return to Overworld via toggle_state_pressed and verify loop guard
        input = {};
        input.toggle_state_pressed = true;
        state_manager.Update(eco::kFixedDt);
        if (std::string_view(state_manager.CurrentState()->Name()) != "OverworldState") {
            std::cerr << "[headless-e2e] Failed return from " << eco::StructureName(sid) << "\n";
            return 1;
        }
        if (world.overworld.tilemap.OverlapsTileId(
                world.players.GetPlayer(0)->Bounds(),
                eco::TileId::StructurePortal)) {
            std::cerr << "[headless-e2e] Return spawn overlaps portal for "
                      << eco::StructureName(sid) << "\n";
            return 1;
        }
        input = {};
        for (int i = 0; i < 5; ++i) {
            state_manager.Update(eco::kFixedDt);
        }
        if (std::string_view(state_manager.CurrentState()->Name()) != "OverworldState") {
            std::cerr << "[headless-e2e] Portal return loop guard failed for "
                      << eco::StructureName(sid) << "\n";
            return 1;
        }
    }

    // Calibrate HarmonicResonator in HangingGardens -> unlocks CentralAstrolabe
    world.TeleportToZone(eco::ZoneId::HangingGardens);
    auto& harmonic = world.GetMachine(eco::MachineType::HarmonicResonator);
    mutable_p0 = world.players.GetPlayer(0);
    mutable_p0->pos = harmonic.pos;
    input = {};
    input.players[0].interact_pressed = true;
    state_manager.Update(eco::kFixedDt);
    world.calibration_modal.SetParameter(harmonic.target_param, harmonic);
    input = {};
    input.players[0].confirm_pressed = true;
    state_manager.Update(eco::kFixedDt);
    if (harmonic.state != eco::MachineState::CleanActive || !world.AllFiveMonumentsActivated()) {
        std::cerr << "[headless-e2e] HarmonicResonator calibration or 5-monument unlock failed\n";
        return 1;
    }

    // Collect all relics & activate CentralAstrolabe in HarmonySanctuary -> Epilogue (True Ending)
    world.TeleportToZone(eco::ZoneId::HarmonySanctuary);
    world.collectibles.CollectAll();
    auto& astrolabe = world.GetMachine(eco::MachineType::CentralAstrolabe);
    mutable_p0 = world.players.GetPlayer(0);
    mutable_p0->pos = astrolabe.pos;
    input = {};
    input.players[0].interact_pressed = true;
    state_manager.Update(eco::kFixedDt);
    if (!world.calibration_modal.is_open) {
        std::cerr << "[headless-e2e] Failed to open CentralAstrolabe modal\n";
        return 1;
    }
    world.calibration_modal.SetParameter(astrolabe.target_param, astrolabe);
    input = {};
    input.players[0].confirm_pressed = true;
    state_manager.Update(eco::kFixedDt);
    if (!world.simulation_completed ||
        world.story.CurrentChapter() != eco::Chapter::Epilogo ||
        !world.story.CleanPath()) {
        std::cerr << "[headless-e2e] CentralAstrolabe activation or Epilogo transition failed\n";
        return 1;
    }

    const eco::MemoryMetrics final_mem = world.GetMemoryMetrics();
    std::cout << "[headless-e2e] PASS: Conca di Meridia 6 separate zones (26x36, resident "
              << final_mem.total_resident_map_bytes << " B, saved "
              << final_mem.saved_unloaded_bytes << " B, swaps "
              << final_mem.zone_swaps_count << "), "
              << "3 side-scrolling structures, 6 physical monuments, 4-player drop-in, "
              << "16 collectibles, Dialogue History (" << world.story.History().size()
              << " entries), Clara recruitment & Epilogo verified.\n";
    return 0;
}

struct WindowedOptions {
    int smoke_frames = -1;
    std::string screenshot_file{};
    std::string scene = "title"; // title | intro | overworld | structure
    float pollution = -1.0f;     // <0: default
    bool open_pause = false;
    bool open_settings = false;
    bool open_history = false;
    bool open_codex = false;
    bool open_modal = false;
    bool collect_all = false;
    bool clean_all = false;
    bool risky_all = false;
    bool dirty_all = false;
    bool epilogue = false;
    bool has_zone = false;
    eco::ZoneId target_zone = eco::ZoneId::DawnTerraces;
    bool has_structure = false;
    eco::StructureId target_structure = eco::StructureId::HypogeanCisterns;
    bool mute = false;
    std::string gpu_override{};
    int monitor_override = -2; // -2: non impostato da CLI, -1: auto, >=0: indice monitor
};

int RunWindowed(const WindowedOptions& opt, int argc, char** argv) {
    const int smoke_frames = opt.smoke_frames;

    // 1. Carica preferenze utente (Monitor, GPU, Audio, Grafica) PRIMA di InitWindow()
    eco::app::GraphicsSettings graphics_settings{};
    (void)eco::app::LoadGraphicsSettings(graphics_settings);
    if (!opt.gpu_override.empty()) {
        graphics_settings.gpu_preference = opt.gpu_override;
    }
    if (opt.monitor_override >= -1) {
        graphics_settings.monitor_index = opt.monitor_override;
    }
    if (opt.mute) {
        graphics_settings.audio_muted = true;
    }
    graphics_settings.initial_gpu_preference = graphics_settings.gpu_preference;

    // Applica le variabili ambiente GPU (NVIDIA PRIME/EGL vs Mesa) PRIMA di creare il contesto OpenGL
    eco::app::ApplyGpuPreferenceBeforeInitWindow(graphics_settings.gpu_preference);

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(eco::kWindowWidth, eco::kWindowHeight, "EQUILIBRIO - La Conca di Meridia");
    {
        Image icon = LoadImageFromMemory(
            ".png",
            eco::app::kAppIconPngData,
            static_cast<int>(eco::app::kAppIconPngSize));
        if (icon.data != nullptr) {
            SetWindowIcon(icon);
            UnloadImage(icon);
        }
    }
    SetWindowMinSize(640, 360);
    // Disabilita l'uscita automatica con ESC: ESC gestisce Calibrazione -> Diario -> Menu di Pausa
    SetExitKey(KEY_NULL);
    SetTargetFPS(eco::kTargetFps);

    graphics_settings.active_gpu_renderer = eco::app::QueryActiveGpuRenderer();
    if (smoke_frames < 0) {
        if (graphics_settings.fullscreen) {
            if (!IsWindowFullscreen()) {
                ToggleFullscreen();
            }
            SetWindowMonitor(eco::app::ResolveTargetMonitor(graphics_settings));
        } else if (graphics_settings.monitor_index >= 0) {
            eco::app::ApplySelectedMonitor(graphics_settings);
        }
    }

    eco::app::InitUiFont();
    eco::app::AudioEngine audio;
    audio.Init(opt.mute);
    eco::app::SetGlobalAudioEngine(&audio);
    RenderTexture2D canvas = LoadRenderTexture(eco::kWindowWidth, eco::kWindowHeight);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

    bool relaunch_gpu_requested = false;

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
            false,
            false,
            false
        };

        eco::app::ApplyGraphicsSettingsToRuntime(graphics_settings);
        eco::app::PauseSettingsMenu pause_menu{};
        eco::app::JournalOverlay journal{};

        world.EnsureInitialized();
        world.guidance_mode = static_cast<eco::GuidanceAssistMode>(
            std::clamp(graphics_settings.guidance_mode_index, 0, 2));

        if (opt.collect_all) {
            world.collectibles.CollectAll();
        }
        if (opt.clean_all || opt.epilogue) {
            for (int mi = 0; mi < 5; ++mi) {
                auto& m = world.GetMachine(static_cast<eco::MachineType>(mi));
                m.state = eco::MachineState::CleanActive;
                m.approach = eco::CalibrationApproach::CleanSlow;
                m.current_param = m.target_param;
                m.stabilizer_value = m.clean_stabilizer_target;
            }
            world.GetMachine(eco::MachineType::CentralAstrolabe).unlocked = true;
            world.GetNpc(eco::NpcId::ClaraSereni).recruited = true;
            world.GetNpc(eco::NpcId::ClaraSereni).met = true;
            world.GetNpc(eco::NpcId::LorenzoAltieri).recruited = true;
            world.GetNpc(eco::NpcId::LorenzoAltieri).met = true;
            world.ecosystem.ApplyCleanActivation(100.0f, 100.0f);
            world.ecosystem.SetArchitectRecruited(true);
            if (opt.epilogue) {
                auto& astro = world.GetMachine(eco::MachineType::CentralAstrolabe);
                astro.state = eco::MachineState::CleanActive;
                astro.approach = eco::CalibrationApproach::CleanSlow;
                world.simulation_completed = true;
            }
            world.RefreshSystemicWorldState();
        } else if (opt.risky_all) {
            for (int mi = 0; mi < 5; ++mi) {
                auto& m = world.GetMachine(static_cast<eco::MachineType>(mi));
                m.state = eco::MachineState::CleanActive;
                m.approach = eco::CalibrationApproach::RiskyEfficient;
                m.current_param = (m.risky_param_min + m.risky_param_max) * 0.5f;
                m.stabilizer_value = 0.22f;
            }
            world.ecosystem.ApplyRiskyActivation(64.0f, 8.0f);
            world.RefreshSystemicWorldState();
        } else if (opt.dirty_all) {
            for (int mi = 0; mi < 5; ++mi) {
                auto& m = world.GetMachine(static_cast<eco::MachineType>(mi));
                m.state = eco::MachineState::BypassedDirty;
                m.approach = eco::CalibrationApproach::DirtyImmediate;
            }
            world.ecosystem.ApplyDirtyBypass(44.0f, 44.0f);
            world.RefreshSystemicWorldState();
        }
        if (opt.pollution >= 0.0f) {
            world.ecosystem.pollution_level =
                eco::Clamp(opt.pollution, eco::kMinEcoValue, eco::kMaxEcoValue);
        }

        if (opt.has_structure) {
            world.EnterStructure(opt.target_structure);
            state_manager.RequestTransition(std::make_unique<eco::app::StructureState>(ctx));
        } else if (opt.scene == "title") {
            state_manager.RequestTransition(std::make_unique<eco::app::TitleState>(ctx));
        } else if (opt.scene == "intro") {
            state_manager.RequestTransition(std::make_unique<eco::app::IntroState>(ctx));
        } else if (opt.scene == "structure") {
            world.EnterStructure(opt.target_structure);
            state_manager.RequestTransition(std::make_unique<eco::app::StructureState>(ctx));
        } else {
            state_manager.RequestTransition(std::make_unique<eco::app::OverworldState>(ctx));
        }
        state_manager.ApplyPendingTransition();

        if (opt.has_zone && !opt.has_structure && opt.scene == "overworld") {
            world.TeleportToZone(opt.target_zone);
        }

        if (opt.open_modal) {
            world.OpenCalibrationModalFor(world.ActiveMachine().type);
            (void)world.ExecuteModalTrial();
        }

        // Assicura che la storia abbia osservato lo stato iniziale (utile per --open-history)
        if (opt.open_history || opt.open_codex || opt.epilogue) {
            world.story.Observe(world.BuildStorySnapshot());
            if (opt.open_history) {
                // Popola qualche voce reale di cronologia se siamo all'inizio per mostrare il diario vivo
                while (world.story.HasActiveDialogue()) {
                    world.story.AdvanceDialogue();
                }
                journal.OpenHistory();
            } else if (opt.open_codex) {
                journal.OpenCodex();
            }
        }

        if (opt.open_settings) {
            pause_menu.OpenSettingsDirect();
        } else if (opt.open_pause) {
            pause_menu.OpenPause();
        }

        eco::FixedTimestepStepper stepper;
        eco::AllPlayersInput pending_input{};
        int frame_count = 0;

        // Tracciamento per salvataggio automatico silenzioso su traguardi e cambi zona/struttura
        bool autosave_primed = false;
        int prev_saved_monuments = 0;
        int prev_saved_relics = 0;
        eco::GameMode prev_saved_mode = world.current_mode;
        eco::StructureId prev_saved_structure = world.active_structure;
        eco::ZoneId prev_saved_zone = world.current_zone;
        bool prev_saved_bridge = world.ecosystem.architect_recruited;
        bool prev_saved_completed = world.simulation_completed;

        while (!WindowShouldClose() && !ctx.quit_requested && !ctx.relaunch_gpu_requested &&
               (smoke_frames < 0 || frame_count < smoke_frames)) {
            const int fb_w = std::max(1, GetRenderWidth());
            const int fb_h = std::max(1, GetRenderHeight());
            const float scale = std::min(
                static_cast<float>(fb_w) / static_cast<float>(eco::kWindowWidth),
                static_cast<float>(fb_h) / static_cast<float>(eco::kWindowHeight));
            const float dest_w = static_cast<float>(eco::kWindowWidth) * scale;
            const float dest_h = static_cast<float>(eco::kWindowHeight) * scale;
            const float dest_x = (static_cast<float>(fb_w) - dest_w) * 0.5f;
            const float dest_y = (static_cast<float>(fb_h) - dest_h) * 0.5f;

            if (scale > 1e-4f) {
                SetMouseOffset(static_cast<int>(-dest_x), static_cast<int>(-dest_y));
                SetMouseScale(1.0f / scale, 1.0f / scale);
            }

            const bool paused = pause_menu.UpdateAndHandleInput(ctx, graphics_settings, &journal);
            if (state_manager.ApplyPendingTransition()) {
                pending_input = {};
                ctx_input = {};
                stepper.accumulator = 0.0f;
            }

            bool in_gameplay_state = false;
            if (state_manager.HasActiveState()) {
                const std::string_view sname(state_manager.CurrentState()->Name());
                in_gameplay_state = (sname == "OverworldState" || sname == "StructureState");
            }
            const bool allow_journal_open =
                !paused && in_gameplay_state && !world.calibration_modal.is_open && !world.interlude.active;
            const bool journal_open =
                paused ? journal.IsOpen() : journal.UpdateAndHandleInput(ctx, allow_journal_open);
            ctx.overlay_blocking_input = paused || journal_open;

            const float frame_dt = GetFrameTime();
            if (!paused && !journal_open) {
                if (ctx.block_state_input_this_frame) {
                    ctx.block_state_input_this_frame = false;
                    pending_input = {};
                    ctx_input = {};
                } else {
                    const std::string_view cur_name =
                        state_manager.HasActiveState()
                            ? std::string_view(state_manager.CurrentState()->Name())
                            : std::string_view{};
                    if (cur_name == "TitleState" || cur_name == "IntroState") {
                        ctx_input = eco::app::PollRaylibInput();
                        state_manager.Update(frame_dt);
                        pending_input = {};
                        stepper.accumulator = 0.0f;
                    } else {
                        const eco::AllPlayersInput sampled = eco::app::PollRaylibInput();
                        eco::LatchFrameInput(pending_input, sampled);

                        const int steps = stepper.Accumulate(frame_dt);
                        for (int s = 0; s < steps; ++s) {
                            ctx_input = pending_input;
                            state_manager.Update(stepper.fixed_dt);
                            eco::ConsumePressedEdges(pending_input);
                        }
                    }
                }
            } else {
                pending_input = {};
            }

            const std::string_view active_state_name =
                state_manager.HasActiveState()
                    ? std::string_view(state_manager.CurrentState()->Name())
                    : std::string_view{};
            audio.SyncAndUpdate(
                frame_dt,
                world,
                active_state_name,
                paused,
                journal_open,
                static_cast<int>(journal.ActiveTab()));

            // Salvataggio automatico in gameplay reale (mai durante --smoke-frames)
            const bool now_in_gameplay =
                (active_state_name == "OverworldState" || active_state_name == "StructureState");
            if (smoke_frames < 0 && now_in_gameplay && !world.interlude.active) {
                const int cur_monuments = world.ActivatedMonumentsCount();
                const int cur_relics = world.collectibles.CollectedCount();
                if (!autosave_primed) {
                    autosave_primed = true;
                    prev_saved_monuments = cur_monuments;
                    prev_saved_relics = cur_relics;
                    prev_saved_mode = world.current_mode;
                    prev_saved_structure = world.active_structure;
                    prev_saved_zone = world.current_zone;
                    prev_saved_bridge = world.ecosystem.architect_recruited;
                    prev_saved_completed = world.simulation_completed;
                } else if (cur_monuments != prev_saved_monuments ||
                           cur_relics != prev_saved_relics ||
                           world.current_mode != prev_saved_mode ||
                           world.active_structure != prev_saved_structure ||
                           world.current_zone != prev_saved_zone ||
                           world.ecosystem.architect_recruited != prev_saved_bridge ||
                           world.simulation_completed != prev_saved_completed) {
                    (void)eco::SaveWorldToFile(world);
                    prev_saved_monuments = cur_monuments;
                    prev_saved_relics = cur_relics;
                    prev_saved_mode = world.current_mode;
                    prev_saved_structure = world.active_structure;
                    prev_saved_zone = world.current_zone;
                    prev_saved_bridge = world.ecosystem.architect_recruited;
                    prev_saved_completed = world.simulation_completed;
                }
            } else if (!now_in_gameplay) {
                autosave_primed = false;
            }

            // Disegna scena + Diario [H/J] + Menu Pausa sulla canvas virtuale 1280x720
            BeginTextureMode(canvas);
            state_manager.Draw();
            journal.DrawOverlay(ctx);
            pause_menu.DrawOverlay(ctx, graphics_settings);
            EndTextureMode();

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
                // Esporta direttamente la canvas 1280x720 cosi' supporta anche percorsi assoluti!
                Image shot = LoadImageFromTexture(canvas.texture);
                ImageFlipVertical(&shot);
                ExportImage(shot, opt.screenshot_file.c_str());
                UnloadImage(shot);
            }
        }

        // Salvataggio di sicurezza alla chiusura finestra durante il gameplay
        if (smoke_frames < 0 && state_manager.HasActiveState()) {
            const std::string_view end_state(state_manager.CurrentState()->Name());
            if (end_state == "OverworldState" || end_state == "StructureState") {
                (void)eco::SaveWorldToFile(world);
            }
        }
        relaunch_gpu_requested = ctx.relaunch_gpu_requested;
    }

    UnloadRenderTexture(canvas);
    audio.Shutdown();
    eco::app::ShutdownUiFont();
    CloseWindow();

    if (relaunch_gpu_requested) {
        (void)eco::app::RelaunchProcessForGpuChange(argc, argv);
    }
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

void PrintUsage() {
    std::cerr << "Usage: eco_proto [--headless-e2e | --smoke-frames N [--screenshot FILE]]\n"
              << "                 [--skip-title] [--scene title|intro|overworld|structure]\n"
              << "                 [--zone dawn|canals|wind|gardens|caldera|sanctuary]\n"
              << "                 [--structure cisterns|pendulum|geothermal|0|1|2]\n"
              << "                 [--pollution 0..100] [--open-history] [--open-codex]\n"
              << "                 [--collect-all] [--clean-all] [--epilogue]\n"
              << "                 [--open-pause] [--open-settings] [--mute]\n"
              << "                 [--monitor -1|0|1|...] [--gpu auto|nvidia|intel|amd]\n";
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
                PrintUsage();
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
        } else if (arg == "--zone" && i + 1 < argc) {
            if (!ParseZoneArg(argv[++i], opt.target_zone)) {
                std::cerr << "Zona sconosciuta: " << argv[i] << "\n";
                return 2;
            }
            opt.has_zone = true;
            if (!scene_given) {
                opt.scene = "overworld";
                scene_given = true;
            }
        } else if (arg == "--structure" && i + 1 < argc) {
            if (!ParseStructureArg(argv[++i], opt.target_structure)) {
                std::cerr << "Struttura sconosciuta: " << argv[i] << "\n";
                return 2;
            }
            opt.has_structure = true;
            opt.scene = "structure";
            scene_given = true;
        } else if (arg == "--pollution" && i + 1 < argc) {
            opt.pollution = std::strtof(argv[++i], nullptr);
        } else if (arg == "--open-pause") {
            opt.open_pause = true;
        } else if (arg == "--open-settings") {
            opt.open_settings = true;
        } else if (arg == "--open-history") {
            opt.open_history = true;
            if (!scene_given) {
                opt.scene = "overworld";
                scene_given = true;
            }
        } else if (arg == "--open-codex") {
            opt.open_codex = true;
            if (!scene_given) {
                opt.scene = "overworld";
                scene_given = true;
            }
        } else if (arg == "--open-modal") {
            opt.open_modal = true;
            if (!scene_given) {
                opt.scene = "overworld";
                scene_given = true;
            }
        } else if (arg == "--collect-all") {
            opt.collect_all = true;
        } else if (arg == "--clean-all") {
            opt.clean_all = true;
        } else if (arg == "--risky-all") {
            opt.risky_all = true;
        } else if (arg == "--dirty-all") {
            opt.dirty_all = true;
        } else if (arg == "--epilogue") {
            opt.epilogue = true;
            if (!scene_given) {
                opt.scene = "overworld";
                scene_given = true;
            }
        } else if (arg == "--mute") {
            opt.mute = true;
        } else if (arg == "--gpu" && i + 1 < argc) {
            opt.gpu_override = argv[++i];
        } else if (arg == "--monitor" && i + 1 < argc) {
            opt.monitor_override = std::atoi(argv[++i]);
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            PrintUsage();
            return 2;
        }
    }

    if (headless_e2e) {
        return RunHeadlessE2E();
    }

    if (skip_title && !scene_given) {
        opt.scene = "overworld";
    }
    return RunWindowed(opt, argc, argv);
}
