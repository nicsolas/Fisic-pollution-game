#include "app/calibration_ui.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

#include "app/raylib_input.h"
#include "app/render/palette.h"
#include "app/render/ui_style.h"
#include "app/render/world_entities.h"
#include "core/collectible.h"
#include "core/ecosystem.h"
#include "core/machine.h"
#include "core/math_types.h"
#include "core/npc.h"
#include "core/player.h"
#include "core/story.h"

namespace eco::app {

namespace {

// =============================================================================
// MINIMAPPA ELEGANTE DELL'OVERWORLD ESPANSO (36x50, 6 distretti)
// =============================================================================
void DrawOverworldMinimap(const eco::WorldContext& world) {
    if (world.current_mode != eco::GameMode::Overworld || world.calibration_modal.is_open) {
        return;
    }

    const Rectangle panel{1064.0f, 158.0f, 200.0f, 146.0f};
    DrawPanelWithHeader(panel, 20, "MAPPA DI MERIDIA", pal::kUiBorderBronze);

    const Rectangle map_r{panel.x + 10.0f, panel.y + 28.0f, panel.width - 20.0f, 96.0f};
    DrawRectangleRec(map_r, Color{12, 18, 26, 240});

    const eco::Tilemap& tm = world.overworld.tilemap;
    const float world_w = std::max(1.0f, tm.WorldWidth());
    const float world_h = std::max(1.0f, tm.WorldHeight());
    const float poll = world.ecosystem.PollutionRatio();

    auto to_mini = [&](eco::Vec2 wp) -> Vector2 {
        return Vector2{
            map_r.x + (wp.x / world_w) * map_r.width,
            map_r.y + (wp.y / world_h) * map_r.height
        };
    };

    // 1. Sfondo del distretto attivo secondo la palette della Conca
    {
        const pal::DistrictPalette& dp = pal::GetDistrictPalette(world.current_zone);
        const Color col = pal::WorldColor(dp.ground_sick, dp.ground_healthy, poll);
        DrawRectangleRec(map_r, pal::WithAlpha(col, 205));
    }

    // 2. Canale orientale e stato del Ponte degli Archi (solo nel Quartiere dei Canali)
    if (world.current_zone == eco::ZoneId::CanalQuarter) {
        const float bridge_x = map_r.x + (32.5f / static_cast<float>(std::max(1, tm.Cols()))) * map_r.width;
        const Color water_c = pal::WorldColor(pal::kSickWater, pal::kHealthyWater, poll);
        DrawLineEx({bridge_x, map_r.y}, {bridge_x, map_r.y + map_r.height}, 3.6f, water_c);
        const float bridge_y = map_r.y + (12.5f / static_cast<float>(std::max(1, tm.Rows()))) * map_r.height;
        DrawRectangle(
            static_cast<int>(bridge_x - 4.0f),
            static_cast<int>(bridge_y - 3.0f),
            8,
            6,
            world.ecosystem.architect_recruited ? pal::kJadeGlow : pal::kAccentDirty);
    }

    // 3. Portali delle strutture interne nella zona attiva (losanghe azzurre/dorate)
    for (const eco::PortalLink& portal : world.overworld.portals) {
        if (portal.source_zone != world.current_zone) {
            continue;
        }
        const Vector2 mp = to_mini(portal.world_center);
        DrawPoly(mp, 4, 4.2f, 0.0f, Color{96, 212, 245, 245});
        DrawPolyLines(mp, 4, 4.5f, 0.0f, RAYWHITE);
    }

    // 4. Reliquie collezionabili della zona attiva (nascoste finche' non scoperte o in Fase 3)
    const bool geodesic_vision =
        (world.ecosystem.ui_phase == eco::UiPhase::Phase3_EngineOverlay);
    for (const eco::CollectibleItem& item : world.collectibles.Items()) {
        if (item.location != eco::LocationId::Overworld || item.zone != world.current_zone) {
            continue;
        }
        if (item.hidden && !item.collected && !geodesic_vision) {
            continue;
        }
        const Vector2 mp = to_mini(item.pos);
        if (item.collected) {
            DrawCircleV(mp, 2.3f, pal::kBronzeBright);
        } else {
            DrawCircleLines(static_cast<int>(mp.x), static_cast<int>(mp.y), 2.5f, pal::WithAlpha(EraAccentColor(item.era), 205));
        }
    }

    // 5. Monumenti fisici della zona attiva
    for (const eco::Machine& m : world.machines) {
        if (m.location != eco::LocationId::Overworld || m.zone != world.current_zone) {
            continue;
        }
        const Vector2 mp = to_mini(m.pos);
        const Color mc = (m.state == eco::MachineState::CleanActive)
                             ? pal::kAccentClean
                             : (m.state == eco::MachineState::BypassedDirty
                                    ? pal::kAccentDirty
                                    : pal::kAmberGlow);
        DrawCircleV(mp, 4.0f, Color{12, 18, 26, 235});
        DrawCircleV(mp, 2.9f, mc);
    }

    // 5b. Varchi monumentali della zona attiva (archi in bronzo) e contorno della zona
    for (const eco::ZoneGateway& gw : world.overworld.active_gateways) {
        const Vector2 gp = to_mini(gw.world_center);
        const bool locked = !world.IsZoneGatewayUnlocked(gw);
        const Color gc = locked ? pal::kAccentWarn : pal::kBronzeBright;
        if (gw.IsHorizontalWall()) {
            DrawRectangle(static_cast<int>(gp.x) - 5, static_cast<int>(gp.y) - 2, 10, 4, gc);
            DrawRectangle(static_cast<int>(gp.x) - 5, static_cast<int>(gp.y) - 4, 2, 8, pal::kBronzeDark);
            DrawRectangle(static_cast<int>(gp.x) + 3, static_cast<int>(gp.y) - 4, 2, 8, pal::kBronzeDark);
        } else {
            DrawRectangle(static_cast<int>(gp.x) - 2, static_cast<int>(gp.y) - 5, 4, 10, gc);
        }
    }
    DrawRectangleLinesEx(map_r, 1.6f, pal::WithAlpha(pal::kJadeGlow, 235));

    // 5c. Faro pulsante sull'obiettivo corrente (solo se la guida e' attiva)
    const eco::GuidanceDisplayState gd = world.ComputeGuidanceDisplay();
    const eco::ObjectiveTarget objective = world.CurrentObjectiveTarget();
    if ((gd.show_compass || gd.show_world_beacon) &&
        objective.valid && objective.location == eco::LocationId::Overworld) {
        const Vector2 op = to_mini(objective.world_pos);
        const float ph = std::fmod(world.sim_time * 0.9f, 1.0f);
        DrawCircleLines(static_cast<int>(op.x), static_cast<int>(op.y), 3.0f + ph * 9.0f,
                        pal::WithAlpha(pal::kJadeGlow, static_cast<unsigned char>(230.0f * (1.0f - ph))));
        DrawCircleV(op, 3.4f, pal::kJadeGlow);
        DrawCircleLines(static_cast<int>(op.x), static_cast<int>(op.y), 4.2f, pal::kBronzeBright);
    }

    // 6. Posizione di Elena Valli (cursore ciano pulsante)
    if (const eco::Player* p0 = world.players.GetPlayer(0); p0 != nullptr && p0->active) {
        const Vector2 pp = to_mini(p0->Center());
        const float pulse = 4.2f + 1.4f * std::sin(world.sim_time * 5.0f);
        DrawCircleLines(static_cast<int>(pp.x), static_cast<int>(pp.y), pulse, pal::WithAlpha(pal::PlayerAccentColor(0), 210));
        DrawCircleV(pp, 2.8f, RAYWHITE);
    }

    DrawRectangleLinesEx(map_r, 1.2f, pal::WithAlpha(pal::kBronze, 190));

    // Legenda compatta sul fondo della minimappa
    const int ly = static_cast<int>(map_r.y + map_r.height) + 6;
    DrawCircle(static_cast<int>(panel.x) + 14, ly + 5, 3.0f, pal::kAccentClean);
    DrawTextShadow("Mon.", static_cast<int>(panel.x) + 20, ly, 10, LIGHTGRAY);
    DrawPoly({panel.x + 62.0f, static_cast<float>(ly + 5)}, 4, 3.5f, 0.0f, Color{96, 212, 245, 245});
    DrawTextShadow("Portali", static_cast<int>(panel.x) + 69, ly, 10, LIGHTGRAY);
    DrawCircle(static_cast<int>(panel.x) + 122, ly + 5, 2.8f, pal::kBronzeBright);
    DrawTextShadow(
        TextFormat("Rel. %d/%d", world.collectibles.CollectedCount(), world.collectibles.TotalCount()),
        static_cast<int>(panel.x) + 128,
        ly,
        10,
        pal::kBronzeBright);
}

// =============================================================================
// BUSSOLA EMPIRICA & GUIDA PROGRESSIVA (usa WorldContext::ComputeGuidanceDisplay)
// Nelle fasi iniziali in modalita' Empirical: NESSUNA bussola fissa, NESSUN metro esatto,
// NESSUNA freccia a bordo schermo e NESSUN anello sopra il monumento.
// =============================================================================
void DrawObjectiveCompass(const eco::WorldContext& world) {
    if (world.calibration_modal.is_open) {
        return;
    }
    const eco::GuidanceDisplayState gd = world.ComputeGuidanceDisplay();
    if (!gd.show_compass && !gd.show_edge_arrow && !gd.show_world_beacon) {
        return;
    }

    const eco::ObjectiveTarget obj = world.CurrentObjectiveTarget();
    const eco::Player* p0 = world.players.GetPlayer(0);
    if (!obj.valid || p0 == nullptr || !p0->active) {
        return;
    }
    if (obj.location != world.ActiveLocation()) {
        return;
    }

    const eco::Vec2 pc = p0->Center();
    const float dx = obj.world_pos.x - pc.x;
    const float dy = obj.world_pos.y - pc.y;
    const float dist_px = std::hypot(dx, dy);
    const int meters = static_cast<int>(dist_px / (eco::kTileSize * 0.5f) + 0.5f);
    const float ang = std::atan2(dy, dx);
    const float pulse = 0.5f + 0.5f * std::sin(world.sim_time * 3.4f);

    const float fade = gd.is_scan_pulse_reveal
                           ? eco::Clamp(world.empirical_compass_timer / 1.8f, 0.18f, 1.0f)
                           : 1.0f;
    const auto sc = [fade](unsigned char a) noexcept -> unsigned char {
        return static_cast<unsigned char>(eco::Clamp(static_cast<float>(a) * fade, 0.0f, 255.0f));
    };

    const float sw = static_cast<float>(eco::kWindowWidth);
    const float sh = static_cast<float>(eco::kWindowHeight);
    const eco::Vec2 cam = world.ActiveCameraTarget();
    const float sx = obj.world_pos.x - cam.x + sw * 0.5f;
    const float sy = obj.world_pos.y - cam.y + sh * 0.5f;
    const bool on_screen = sx > 24.0f && sx < sw - 24.0f && sy > 76.0f && sy < sh - 24.0f;

    if (gd.show_compass) {
        const char* full = gd.show_distance_meters
                               ? TextFormat("-> %s (%d m)", obj.label, meters)
                               : TextFormat("-> %s (Direzione Rilevata)", obj.label);
        const int tw = MeasureUiText(full, 13);
        const int bw = std::max(tw + 58, gd.is_scan_pulse_reveal ? 290 : (tw + 58));
        const int bx = static_cast<int>(sw * 0.5f) - bw / 2;
        const int by = static_cast<int>(sh) - 158;

        if (gd.is_scan_pulse_reveal) {
            const char* emp_tag = TextFormat(
                "RILIEVO EMPIRICO ATTIVO [Q]  (%.1f s)",
                std::max(0.0f, world.empirical_compass_timer));
            const int etw = MeasureUiText(emp_tag, 11) + 20;
            const int etx = static_cast<int>(sw * 0.5f) - etw / 2;
            const int ety = by - 20;
            DrawRectangle(etx, ety, etw, 18, Color{12, 26, 36, sc(220)});
            DrawRectangleLines(etx, ety, etw, 18, pal::WithAlpha(pal::kJadeGlow, sc(215)));
            DrawTextShadow(emp_tag, etx + 10, ety + 3, 11, pal::WithAlpha(pal::kJadeGlow, sc(250)));
        }

        DrawRectangle(bx + 2, by + 2, bw, 30, Color{0, 0, 0, sc(110)});
        DrawRectangle(bx, by, bw, 30, Color{14, 22, 32, sc(226)});
        DrawRectangleLines(bx, by, bw, 30, pal::WithAlpha(pal::kBronze, sc(235)));
        DrawRectangleLines(
            bx + 2,
            by + 2,
            bw - 4,
            26,
            pal::WithAlpha(pal::kJade, sc(static_cast<unsigned char>(110 + 90 * pulse))));

        const Vector2 cc{static_cast<float>(bx) + 20.0f, static_cast<float>(by) + 15.0f};
        DrawCircleV(cc, 12.0f, Color{18, 30, 40, sc(255)});
        DrawCircleLines(static_cast<int>(cc.x), static_cast<int>(cc.y), 12.0f, pal::WithAlpha(pal::kBronzeBright, sc(255)));
        const Vector2 tip{cc.x + std::cos(ang) * 10.0f, cc.y + std::sin(ang) * 10.0f};
        const Vector2 l{cc.x + std::cos(ang + 2.5f) * 6.0f, cc.y + std::sin(ang + 2.5f) * 6.0f};
        const Vector2 r{cc.x + std::cos(ang - 2.5f) * 6.0f, cc.y + std::sin(ang - 2.5f) * 6.0f};
        DrawTriangle(tip, l, r, pal::WithAlpha(pal::kJadeGlow, sc(255)));
        DrawCircleV(cc, 2.0f, pal::WithAlpha(pal::kBronzeBright, sc(255)));
        DrawTextShadow(
            full,
            bx + 40,
            by + 8,
            13,
            pal::WithAlpha(obj.is_portal_step ? pal::kBronzeBright : pal::kJadeGlow, sc(255)));
    }

    // Freccia sul bordo dello schermo quando la meta e' fuori vista (solo se abilitata)
    if (gd.show_edge_arrow && !on_screen && dist_px > 1.0f) {
        const float mx = sw * 0.5f;
        const float my = (sh + 52.0f) * 0.5f;
        const float ux = std::cos(ang);
        const float uy = std::sin(ang);
        const float lim_x = sw * 0.5f - 56.0f;
        const float lim_y = (sh - 52.0f) * 0.5f - 56.0f;
        const float tx = (std::fabs(ux) > 0.0001f) ? lim_x / std::fabs(ux) : 1.0e9f;
        const float ty = (std::fabs(uy) > 0.0001f) ? lim_y / std::fabs(uy) : 1.0e9f;
        const float t = std::min(tx, ty);
        const Vector2 e{mx + ux * t, my + uy * t};
        const Vector2 a{e.x + ux * 16.0f, e.y + uy * 16.0f};
        const Vector2 b{e.x - ux * 8.0f - uy * 11.0f, e.y - uy * 8.0f + ux * 11.0f};
        const Vector2 c2{e.x - ux * 8.0f + uy * 11.0f, e.y - uy * 8.0f - ux * 11.0f};
        DrawCircleV(e, 18.0f + pulse * 3.0f, pal::WithAlpha(pal::kJade, sc(48)));
        DrawTriangle(a, c2, b, pal::WithAlpha(pal::kBronzeBright, sc(255)));
        DrawTriangleLines(a, c2, b, pal::WithAlpha(pal::kBronzeDark, sc(255)));
        DrawCircleV(e, 5.0f, pal::WithAlpha(pal::kJadeGlow, sc(255)));
        if (gd.show_distance_meters) {
            DrawTextShadow(
                TextFormat("%d m", meters),
                static_cast<int>(e.x) - 14,
                static_cast<int>(e.y) + 20,
                11,
                pal::WithAlpha(pal::kParchmentText, sc(255)));
        }
    } else if (gd.show_world_beacon && on_screen) {
        // Anello pulsante sopra la meta quando e' inquadrata (solo se abilitato)
        DrawCircleLines(
            static_cast<int>(sx),
            static_cast<int>(sy) - 60,
            14.0f + pulse * 4.0f,
            pal::WithAlpha(pal::kJadeGlow, sc(200)));
        DrawTriangle(
            {sx, sy - 40.0f},
            {sx + 8.0f, sy - 54.0f},
            {sx - 8.0f, sy - 54.0f},
            pal::WithAlpha(pal::kBronzeBright, sc(255)));
    }
}

// =============================================================================
// 6 DIAGRAMMI FISICI INTERATTIVI NEL MODAL DI CALIBRAZIONE
// Non mostra il valore target numerico gratis finche' non viene scoperto!
// =============================================================================
void DrawCalibrationDiagram(
    const eco::Machine& machine,
    const eco::WorldContext& world,
    Rectangle diag_r,
    bool target_revealed,
    bool hard_mode) {
    DrawRectangleRec(diag_r, Color{10, 18, 28, 245});
    DrawRectangleLinesEx(diag_r, 1.4f, pal::WithAlpha(hard_mode ? pal::kAmberGlow : pal::kBronze, 200));

    const bool effective_reveal = !hard_mode && target_revealed;
    const float val = world.calibration_modal.slider_value;
    const bool ok_clean = world.calibration_modal.can_confirm_clean;
    const bool ok_risky = world.calibration_modal.can_confirm_risky;
    const Color live_col = hard_mode
                               ? Color{128, 184, 212, 245}
                               : (ok_clean
                                      ? pal::kAccentClean
                                      : (ok_risky ? pal::kAmberGlow : Color{86, 202, 240, 255}));
    const Vector2 c{diag_r.x + diag_r.width * 0.5f, diag_r.y + diag_r.height * 0.42f};

    switch (machine.type) {
        case eco::MachineType::SolarArray: {
            DrawCircleV(c, 50.0f, Color{16, 26, 38, 255});
            DrawCircleLines(static_cast<int>(c.x), static_cast<int>(c.y), 50.0f, pal::WithAlpha(pal::kBronzeBright, 175));
            const float p_rad = eco::deg2rad(val);
            if (effective_reveal) {
                const float s_rad = eco::deg2rad(machine.target_param);
                DrawArrow2D(c, {c.x + std::cos(s_rad) * 42.0f, c.y + std::sin(s_rad) * 42.0f}, 2.4f, pal::kAmberGlow);
            }
            DrawArrow2D(c, {c.x + std::cos(p_rad) * 38.0f, c.y + std::sin(p_rad) * 38.0f}, 3.0f, live_col);
            DrawTextShadow(
                hard_mode
                    ? "Fisica Pura: misura ombra sul campo"
                    : (effective_reveal ? "Oro: Azimut Solare Rilevato" : "Oro: [Sperimenta per svelare]"),
                static_cast<int>(diag_r.x) + 12,
                static_cast<int>(diag_r.y + diag_r.height) - 36,
                11,
                pal::kAmberGlow);
            DrawTextShadow(
                TextFormat("Specchio: %.1f deg | Stab %.0f%%", val, world.calibration_modal.stabilizer_value * 100.0f),
                static_cast<int>(diag_r.x) + 12,
                static_cast<int>(diag_r.y + diag_r.height) - 19,
                11,
                live_col);
            break;
        }
        case eco::MachineType::HydroTurbine: {
            const Rectangle col_r{diag_r.x + 22.0f, diag_r.y + 14.0f, 42.0f, diag_r.height - 34.0f};
            DrawRectangleRec(col_r, Color{14, 24, 36, 255});
            const float fill = eco::Clamp(val / std::max(1.0f, machine.max_param), 0.03f, 1.0f);
            const float wh = col_r.height * fill;
            DrawRectangleGradientV(
                static_cast<int>(col_r.x + 2),
                static_cast<int>(col_r.y + col_r.height - wh),
                static_cast<int>(col_r.width - 4),
                static_cast<int>(wh),
                Color{72, 212, 248, 235},
                Color{24, 118, 178, 245});
            const bool show_hydro_target =
                !hard_mode && (effective_reveal || world.calibration_modal.piezometric_relic_active);
            if (show_hydro_target) {
                const float opt_y = col_r.y + col_r.height * (1.0f - eco::Clamp(machine.target_param / std::max(1.0f, machine.max_param), 0.0f, 1.0f));
                DrawLineEx({col_r.x - 6.0f, opt_y}, {col_r.x + col_r.width + 6.0f, opt_y}, 2.2f, pal::kAccentClean);
            }
            DrawRectangleLinesEx(col_r, 1.6f, pal::kBronzeBright);

            const float p_kpa = (eco::kWaterDensity * eco::kGravityWater * val) / 1000.0f;
            const float opt_kpa = (eco::kWaterDensity * eco::kGravityWater * machine.target_param) / 1000.0f;
            DrawTextShadow("STEVINO LIVE", static_cast<int>(diag_r.x) + 78, static_cast<int>(diag_r.y) + 18, 11, pal::kBronzeBright);
            DrawTextShadow(TextFormat("h = %.1f m", val), static_cast<int>(diag_r.x) + 78, static_cast<int>(diag_r.y) + 34, 13, pal::kParchmentText);
            DrawTextShadow(TextFormat("P = %.1f kPa", p_kpa), static_cast<int>(diag_r.x) + 78, static_cast<int>(diag_r.y) + 52, 15, live_col);
            if (show_hydro_target) {
                DrawTextShadow("BANDA ARMONICA", static_cast<int>(diag_r.x) + 78, static_cast<int>(diag_r.y) + 84, 11, pal::kAccentClean);
                DrawTextShadow(TextFormat("%.1f kPa", opt_kpa), static_cast<int>(diag_r.x) + 78, static_cast<int>(diag_r.y) + 100, 14, pal::kAccentClean);
            } else if (hard_mode) {
                DrawTextShadow("Fisica Pura:", static_cast<int>(diag_r.x) + 78, static_cast<int>(diag_r.y) + 88, 11, pal::kAmberGlow);
                DrawTextShadow("calcola h = P/(rho*g)", static_cast<int>(diag_r.x) + 78, static_cast<int>(diag_r.y) + 104, 11, pal::kAmberGlow);
            } else {
                DrawTextShadow("Usa [T] o Stele II.2", static_cast<int>(diag_r.x) + 78, static_cast<int>(diag_r.y) + 88, 11, pal::kAmberGlow);
                DrawTextShadow("per quota d'equilibrio", static_cast<int>(diag_r.x) + 78, static_cast<int>(diag_r.y) + 104, 11, pal::kAmberGlow);
            }
            break;
        }
        case eco::MachineType::PendulumRegulator: {
            const Vector2 pivot{diag_r.x + diag_r.width * 0.5f, diag_r.y + 18.0f};
            const float t_live = std::max(0.45f, eco::ComputePendulumPeriod(val));
            const float t_opt = std::max(0.45f, eco::ComputePendulumPeriod(machine.target_param));
            const float norm_l = eco::Clamp((val - machine.min_param) / std::max(0.1f, machine.max_param - machine.min_param), 0.0f, 1.0f);
            const float len_px = eco::Lerp(34.0f, 84.0f, norm_l);

            if (effective_reveal) {
                const float opt_ang = std::sin(world.sim_time * (2.0f * eco::kPi / t_opt)) * 0.48f;
                const float opt_len = eco::Lerp(34.0f, 84.0f, eco::Clamp((machine.target_param - machine.min_param) / std::max(0.1f, machine.max_param - machine.min_param), 0.0f, 1.0f));
                const Vector2 opt_bob{pivot.x + std::sin(opt_ang) * opt_len, pivot.y + std::cos(opt_ang) * opt_len};
                DrawLineEx(pivot, opt_bob, 1.6f, pal::WithAlpha(pal::kAmberGlow, 120));
                DrawCircleLines(static_cast<int>(opt_bob.x), static_cast<int>(opt_bob.y), 7.5f, pal::WithAlpha(pal::kAmberGlow, 170));
            }

            const float cur_ang = std::sin(world.sim_time * (2.0f * eco::kPi / t_live)) * 0.48f;
            const Vector2 cur_bob{pivot.x + std::sin(cur_ang) * len_px, pivot.y + std::cos(cur_ang) * len_px};
            DrawLineEx(pivot, cur_bob, 2.8f, live_col);
            DrawCircleV(pivot, 4.5f, pal::kBronzeBright);
            DrawCircleV(cur_bob, 9.0f, live_col);
            DrawCircleV(cur_bob, 3.6f, RAYWHITE);

            DrawTextShadow(TextFormat("T = %.2f s", t_live), static_cast<int>(diag_r.x) + 12, static_cast<int>(diag_r.y + diag_r.height) - 36, 12, live_col);
            DrawTextShadow(
                hard_mode
                    ? "Fisica Pura: calcola L = g*(T/2pi)^2"
                    : (effective_reveal ? TextFormat("T0 = %.2f s", t_opt) : "Premi [T] per misurare T0"),
                static_cast<int>(diag_r.x) + 12,
                static_cast<int>(diag_r.y + diag_r.height) - 19,
                11,
                pal::kAmberGlow);
            break;
        }
        case eco::MachineType::HarmonicResonator: {
            const float gx0 = diag_r.x + 16.0f;
            const float gx1 = diag_r.x + diag_r.width - 16.0f;
            const float gy_bot = diag_r.y + diag_r.height - 42.0f;
            const float gy_top = diag_r.y + 18.0f;
            const float gh = gy_bot - gy_top;

            if (hard_mode) {
                // In Fisica Pura: oscilloscopio neutro della sola frequenza impostata (senza curva target!)
                const float mid_y = (gy_top + gy_bot) * 0.5f;
                const float amp = gh * 0.36f;
                DrawLineEx({gx0, mid_y}, {gx1, mid_y}, 1.0f, pal::WithAlpha(pal::kBronzeBright, 120));
                const float norm_f = eco::Clamp((val - machine.min_param) / std::max(1.0f, machine.max_param - machine.min_param), 0.0f, 1.0f);
                const float cycles = eco::Lerp(1.5f, 7.5f, norm_f);
                constexpr int kWaveSamples = 56;
                Vector2 prev{};
                for (int i = 0; i <= kWaveSamples; ++i) {
                    const float u = static_cast<float>(i) / static_cast<float>(kWaveSamples);
                    const float x = eco::Lerp(gx0, gx1, u);
                    const float y = mid_y - std::sin(u * cycles * 2.0f * eco::kPi + world.sim_time * 4.2f) * amp;
                    if (i > 0) {
                        DrawLineEx(prev, {x, y}, 2.2f, live_col);
                    }
                    prev = {x, y};
                }
                DrawTextShadow(TextFormat("f = %.1f Hz", val), static_cast<int>(diag_r.x) + 12, static_cast<int>(diag_r.y + diag_r.height) - 34, 12, live_col);
                DrawTextShadow("Oscilloscopio neutro (Q=3) — Fisica Pura", static_cast<int>(diag_r.x) + 12, static_cast<int>(diag_r.y + diag_r.height) - 18, 11, pal::kAmberGlow);
            } else {
                DrawLineEx({gx0, gy_bot}, {gx1, gy_bot}, 1.4f, pal::WithAlpha(pal::kBronzeBright, 150));
                const float y_thresh = gy_bot - gh * machine.Threshold();
                DrawLineEx({gx0, y_thresh}, {gx1, y_thresh}, 1.1f, pal::WithAlpha(pal::kAccentClean, 110));

                constexpr int kSamples = 48;
                Vector2 prev{};
                for (int i = 0; i <= kSamples; ++i) {
                    const float u = static_cast<float>(i) / static_cast<float>(kSamples);
                    const float f = eco::Lerp(machine.min_param, machine.max_param, u);
                    const float r = machine.EvaluateEfficiency(f);
                    const Vector2 pt{eco::Lerp(gx0, gx1, u), gy_bot - r * gh};
                    if (i > 0) {
                        DrawLineEx(prev, pt, 2.2f, pal::kJade);
                    }
                    prev = pt;
                }
                const float cur_u = eco::Clamp((val - machine.min_param) / std::max(1.0f, machine.max_param - machine.min_param), 0.0f, 1.0f);
                const float cur_r = machine.EvaluateEfficiency(val);
                const Vector2 cur_pt{eco::Lerp(gx0, gx1, cur_u), gy_bot - cur_r * gh};
                DrawLineEx({cur_pt.x, gy_bot}, cur_pt, 1.5f, pal::WithAlpha(live_col, 165));
                DrawCircleV(cur_pt, 5.5f, live_col);
                DrawCircleV(cur_pt, 2.4f, RAYWHITE);

                DrawTextShadow(TextFormat("f = %.1f Hz", val), static_cast<int>(diag_r.x) + 12, static_cast<int>(diag_r.y + diag_r.height) - 34, 12, live_col);
                DrawTextShadow("Risonanza Q=3", static_cast<int>(diag_r.x) + 12, static_cast<int>(diag_r.y + diag_r.height) - 18, 11, pal::kBronzeBright);
            }
            break;
        }
        case eco::MachineType::ThermalExchanger: {
            const float eta_c = eco::ComputeCarnotEta(val, eco::kDefaultCarnotColdTempK) * 100.0f;
            const Rectangle hot_r{diag_r.x + 16.0f, diag_r.y + 14.0f, diag_r.width - 32.0f, 30.0f};
            const Rectangle cold_r{diag_r.x + 16.0f, diag_r.y + 88.0f, diag_r.width - 32.0f, 28.0f};
            DrawRectangleRec(hot_r, Color{156, 68, 34, 235});
            DrawRectangleLinesEx(hot_r, 1.6f, pal::kAmberGlow);
            DrawTextShadow(TextFormat("T_calda = %.0f K", val), static_cast<int>(hot_r.x) + 12, static_cast<int>(hot_r.y) + 7, 12, RAYWHITE);

            DrawRectangleRec(cold_r, Color{26, 86, 132, 235});
            DrawRectangleLinesEx(cold_r, 1.6f, Color{118, 218, 250, 240});
            DrawTextShadow("T_fredda = 300 K", static_cast<int>(cold_r.x) + 12, static_cast<int>(cold_r.y) + 6, 12, RAYWHITE);

            const float mid_x = diag_r.x + diag_r.width * 0.5f;
            DrawArrow2D({mid_x, hot_r.y + hot_r.height + 2.0f}, {mid_x, cold_r.y - 2.0f}, 2.4f, live_col);
            DrawCircleV({mid_x, diag_r.y + 66.0f}, 12.0f, Color{20, 32, 44, 245});
            DrawCircleLines(static_cast<int>(mid_x), static_cast<int>(diag_r.y + 66.0f), 12.0f, live_col);
            DrawTextShadow("W", static_cast<int>(mid_x) - 5, static_cast<int>(diag_r.y) + 60, 12, live_col);

            DrawTextShadow(TextFormat("Carnot eta = %.1f%%", eta_c), static_cast<int>(diag_r.x) + 12, static_cast<int>(diag_r.y + diag_r.height) - 34, 12, live_col);
            DrawTextShadow(
                hard_mode
                    ? "Fisica Pura: calcola T_calda da eta_C"
                    : (effective_reveal ? "Equilibrio armonico: 600 K" : "Sperimenta [T] per equilibrio"),
                static_cast<int>(diag_r.x) + 12,
                static_cast<int>(diag_r.y + diag_r.height) - 18,
                11,
                hard_mode ? pal::kAmberGlow : pal::kAccentClean);
            break;
        }
        case eco::MachineType::CentralAstrolabe:
        case eco::MachineType::Count: {
            const float gx0 = diag_r.x + 14.0f;
            const float gx1 = diag_r.x + diag_r.width - 14.0f;
            const float mid_y = diag_r.y + 66.0f;
            const float amp = 21.0f;

            DrawLineEx({gx0, mid_y}, {gx1, mid_y}, 1.0f, pal::WithAlpha(pal::kBronze, 120));
            constexpr int kSteps = 44;
            if (hard_mode) {
                // In Fisica Pura: solo l'onda della fase impostata (nessun riferimento target!)
                const float phi_rad = eco::deg2rad(val);
                Vector2 prev_cur{};
                for (int i = 0; i <= kSteps; ++i) {
                    const float u = static_cast<float>(i) / static_cast<float>(kSteps);
                    const float x = eco::Lerp(gx0, gx1, u);
                    const float theta = u * 4.0f * eco::kPi + world.sim_time * 3.0f;
                    const float y_cur = mid_y - std::sin(theta + phi_rad) * (amp * 0.9f);
                    if (i > 0) {
                        DrawLineEx(prev_cur, {x, y_cur}, 2.2f, live_col);
                    }
                    prev_cur = {x, y_cur};
                }
                DrawTextShadow(TextFormat("Fase impostata: %.1f deg", val), static_cast<int>(diag_r.x) + 12, static_cast<int>(diag_r.y + diag_r.height) - 34, 11, live_col);
                DrawTextShadow("Fisica Pura: verifica con [T]", static_cast<int>(diag_r.x) + 12, static_cast<int>(diag_r.y + diag_r.height) - 18, 11, pal::kAmberGlow);
            } else {
                const float dphi = eco::deg2rad(val - machine.target_param);
                Vector2 prev_ref{};
                Vector2 prev_cur{};
                Vector2 prev_sum{};
                for (int i = 0; i <= kSteps; ++i) {
                    const float u = static_cast<float>(i) / static_cast<float>(kSteps);
                    const float x = eco::Lerp(gx0, gx1, u);
                    const float theta = u * 4.0f * eco::kPi + world.sim_time * 3.0f;
                    const float y_ref = mid_y - std::sin(theta) * (amp * 0.65f);
                    const float y_cur = mid_y - std::sin(theta + dphi) * (amp * 0.65f);
                    const float y_sum = mid_y - (std::sin(theta) + std::sin(theta + dphi)) * (amp * 0.55f);
                    if (i > 0) {
                        DrawLineEx(prev_ref, {x, y_ref}, 1.5f, pal::WithAlpha(pal::kAmberGlow, 175));
                        DrawLineEx(prev_cur, {x, y_cur}, 1.5f, pal::WithAlpha(Color{92, 224, 248, 255}, 175));
                        DrawLineEx(prev_sum, {x, y_sum}, 2.4f, live_col);
                    }
                    prev_ref = {x, y_ref};
                    prev_cur = {x, y_cur};
                    prev_sum = {x, y_sum};
                }
                DrawTextShadow("Oro/Ciano: Onde di Fase", static_cast<int>(diag_r.x) + 12, static_cast<int>(diag_r.y + diag_r.height) - 34, 11, pal::kAmberGlow);
                DrawTextShadow("Giada: Interferenza", static_cast<int>(diag_r.x) + 12, static_cast<int>(diag_r.y + diag_r.height) - 18, 11, live_col);
            }
            break;
        }
    }
}

} // namespace


// =============================================================================
// HUD SUPERIORE, LENTE EMPIRICA, MINIMAPPA & MODAL DI CALIBRAZIONE
// =============================================================================
void DrawHudAndCalibrationModalScreenSpace(eco::WorldContext& world, bool allow_mouse_input) {
    const eco::OverlayVisibility ov = eco::ComputeOverlayVisibility(world.ecosystem);
    const eco::Machine& machine = world.ModalTargetMachine();

    // 1. Barra Superiore (y = 0..52) - stile Conca di Meridia
    DrawRectangleGradientV(0, 0, eco::kWindowWidth, 52, Color{16, 24, 34, 245}, Color{10, 17, 26, 242});
    DrawLine(0, 52, eco::kWindowWidth, 52, pal::kUiBorderBronze);

    Color phase_col = pal::kAmberGlow;
    if (ov.phase == eco::UiPhase::Phase2_Telemetry) {
        phase_col = Color{98, 214, 246, 255};
    } else if (ov.phase == eco::UiPhase::Phase3_EngineOverlay) {
        phase_col = pal::kJadeGlow;
    }

    DrawTextShadow(world.ActiveLocationName(), 16, 7, 15, pal::kParchmentText);
    DrawTextShadow(
        TextFormat(
            "Fase %d: %s%s",
            static_cast<int>(ov.phase),
            eco::UiPhaseName(ov.phase),
            world.hard_physics_mode ? " [FISICA PURA]" : ""),
        16,
        29,
        12,
        world.hard_physics_mode ? pal::kAmberGlow : phase_col);

    // Indicatori Foschia (Pece) & Energia Armonica
    const int smog_x = 304;
    const Color smog_col = world.ecosystem.pollution_level > 35.0f ? pal::kAccentDirty : pal::kAccentClean;
    DrawTextShadow(TextFormat("FOSCHIA: %.0f%%", world.ecosystem.pollution_level), smog_x, 8, 12, smog_col);
    DrawProgressBar(
        Rectangle{static_cast<float>(smog_x), 26.0f, 138.0f, 14.0f},
        world.ecosystem.PollutionRatio(),
        smog_col,
        pal::WithAlpha(pal::kBronze, 200));

    const int en_x = 458;
    DrawTextShadow(TextFormat("ENERGIA: %.0f MW", world.ecosystem.clean_energy), en_x, 8, 12, pal::kAccentClean);
    DrawProgressBar(
        Rectangle{static_cast<float>(en_x), 26.0f, 138.0f, 14.0f},
        eco::Clamp(world.ecosystem.clean_energy / 100.0f, 0.0f, 1.0f),
        pal::kAccentClean,
        pal::WithAlpha(pal::kBronze, 200));

    // Contatore Reliquie & Monumenti
    const int rel_x = 612;
    DrawRelicGlyph(1, {static_cast<float>(rel_x + 11), 26.0f}, 9.5f, pal::kBronzeBright, true, world.sim_time);
    DrawTextShadow(
        TextFormat("RELIQUIE %d/%d", world.collectibles.CollectedCount(), world.collectibles.TotalCount()),
        rel_x + 26,
        8,
        12,
        pal::kBronzeBright);
    DrawTextShadow(
        TextFormat("MONUMENTI %d/5", ActivatedCoreMonumentsCount(world)),
        rel_x + 26,
        27,
        11,
        pal::kJadeGlow);

    // Badge comandi compatti a destra (nessuna sovrapposizione; icone controller se attivo)
    int bx_hint = 768;
    auto draw_hint = [&](GamepadAction act, const char* key, const char* lbl, Color col) {
        DrawActionPromptBadge(static_cast<int>(act), key, bx_hint, 17, col);
        bx_hint += MeasureActionPromptBadge(static_cast<int>(act), key) + 5;
        DrawTextShadow(lbl, bx_hint, 19, 12, pal::kParchmentText);
        bx_hint += MeasureUiText(lbl, 12) + 12;
    };
    draw_hint(GamepadAction::Scan, "Q", "Lente", pal::kJadeGlow);
    draw_hint(GamepadAction::Interact, "E", "Azione", pal::kAmberGlow);
    draw_hint(GamepadAction::Journal, "H", "Diario", pal::kUiBorderCyan);
    draw_hint(GamepadAction::Sprint, "SHIFT", "Corri", pal::kBronzeBright);
    draw_hint(GamepadAction::HudPhase, "V", "Vista", pal::kJade);
    draw_hint(GamepadAction::Pause, "ESC", "Menu", LIGHTGRAY);

    // 2. Pannello Lente Empirica & Rilievo Metrico (alto-destra x=846, y=62, w=418)
    if (ov.show_raw_numbers || world.last_scan.active) {
        const float px = static_cast<float>(eco::kWindowWidth - 434);
        const float py = 62.0f;
        const float pw = 418.0f;
        const float ph = ov.show_raw_numbers ? 84.0f : 56.0f;
        DrawPanel(Rectangle{px, py, pw, ph}, pal::kUiBorderCyan);

        const ControllerPrompts hud_prompts = GetActiveControllerPrompts();
        const char* scan_lbl = hud_prompts.controller_active ? hud_prompts.scan : "[Q]";
        const char* vista_lbl =
            hud_prompts.controller_active
                ? ControllerButtonLabel(hud_prompts.type, GetActionIconInfo(GamepadAction::HudPhase).button)
                : "[V]";

        const bool has_gnomon =
            world.collectibles.IsCollected(eco::CollectibleId::Dawn_BrassGnomon);
        if (world.last_scan.active) {
            if (world.last_scan.has_target) {
                const eco::Machine& scanned_m = world.GetMachine(world.last_scan.target_type);
                const bool show_exact =
                    !world.hard_physics_mode &&
                    (has_gnomon || ov.show_raw_numbers || scanned_m.trials_count > 0 ||
                     scanned_m.state != eco::MachineState::Broken);
                const char* scan_line =
                    show_exact
                        ? TextFormat(
                              "LENTE %s: %s = %.2f %s (d=%.0f)",
                              scan_lbl,
                              eco::MachineName(world.last_scan.target_type),
                              world.last_scan.measured_target_value,
                              eco::MachineUnit(world.last_scan.target_type),
                              world.last_scan.measured_distance)
                        : TextFormat(
                              "LENTE %s: %s rilevato (d=%.0f) -> Indaga con [E]",
                              scan_lbl,
                              eco::MachineName(world.last_scan.target_type),
                              world.last_scan.measured_distance);
                DrawTextShadow(scan_line, static_cast<int>(px) + 12, static_cast<int>(py) + 10, 12, pal::kAmberGlow);
            } else {
                DrawTextShadow(
                    TextFormat("LENTE %s: Rilievo ambientale attivo (osserva indizi e correnti)", scan_lbl),
                    static_cast<int>(px) + 12,
                    static_cast<int>(py) + 10,
                    12,
                    pal::kJadeGlow);
            }
        } else {
            DrawTextShadow(
                TextFormat("LENTE %s: Usa sul campo per rilievo empirico o Prisma remoto", scan_lbl),
                static_cast<int>(px) + 12,
                static_cast<int>(py) + 10,
                12,
                pal::kParchmentText);
        }

        if (ov.show_raw_numbers) {
            const eco::Player* p0 = world.players.GetPlayer(0);
            const float vx = p0 ? p0->vel.x : 0.0f;
            const float vy = p0 ? ((world.current_mode == eco::GameMode::Structure) ? p0->y_velocity : p0->vel.y) : 0.0f;
            const eco::ZoneEcoStatus zeco = world.ActiveSceneEcoStatus();
            DrawTextShadow(
                TextFormat(
                    "RILIEVO: v=(%.0f,%.0f) | Rend.=%.0f%% | Fauna=%d | %s",
                    vx,
                    vy,
                    world.ActiveMachine().LiveEfficiency() * 100.0f,
                    zeco.fauna_count,
                    eco::EcoHealthBandName(zeco.band)),
                static_cast<int>(px) + 12,
                static_cast<int>(py) + 32,
                12,
                Color{112, 220, 248, 255});
            DrawTextShadow(
                TextFormat(
                    "GUIDA: %s | Prove: %d | Ponte Archi: %s",
                    world.hard_physics_mode ? "Fisica Pura (Hard)" : eco::GuidanceAssistModeName(world.guidance_mode),
                    world.total_trials_run,
                    world.ecosystem.architect_recruited ? "APERTO" : "CHIUSO"),
                static_cast<int>(px) + 12,
                static_cast<int>(py) + 54,
                12,
                world.hard_physics_mode ? pal::kAmberGlow : pal::kJadeGlow);
        } else {
            DrawTextShadow(
                TextFormat("Osserva il mondo o calibra senza pece per il Rilievo %s", vista_lbl),
                static_cast<int>(px) + 12,
                static_cast<int>(py) + 32,
                12,
                pal::kAmberGlow);
        }
    }

    // 3. Minimappa Overworld sotto il pannello scanner (x=1064, y=158, w=200, h=146)
    DrawOverworldMinimap(world);

    // 3b. Bussola Empirica / Guida dell'obiettivo (controllata da ComputeGuidanceDisplay)
    DrawObjectiveCompass(world);

    // 4. Banco di Indagine Scientifica & Calibrazione Multi-Soluzione (1280x720)
    static float s_hard_reject_timer = 0.0f;
    if (!world.calibration_modal.is_open) {
        s_hard_reject_timer = 0.0f;
    } else {
        s_hard_reject_timer = std::max(0.0f, s_hard_reject_timer - GetFrameTime());
        const bool hard_mode = world.hard_physics_mode || world.calibration_modal.hard_physics_mode;
        constexpr Color kHardNeutralCol{128, 184, 212, 240};

        DrawRectangle(0, 0, eco::kWindowWidth, eco::kWindowHeight, Color{5, 10, 16, 212});
        const int box_w = 1120;
        const int box_h = 640;
        const int bx = (eco::kWindowWidth - box_w) / 2;
        const int by = (eco::kWindowHeight - box_h) / 2;

        const char* modal_title = TextFormat(
            "%sBANCO D'INDAGINE: %s  |  %s  (%s)  |  Stato: %s",
            hard_mode ? "[FISICA PURA] " : "",
            eco::MachineName(machine.type),
            eco::MachineLawName(machine.type),
            eco::MachineFormulaLabel(machine.type),
            eco::CalibrationApproachName(machine.approach));
        DrawPanelWithHeader(
            Rectangle{static_cast<float>(bx), static_cast<float>(by), static_cast<float>(box_w), static_cast<float>(box_h)},
            38,
            modal_title,
            hard_mode ? pal::kAmberGlow : pal::kBronzeBright);

        const Vector2 mouse = GetMousePosition();
        const bool mouse_clicked = allow_mouse_input && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        const bool mouse_down = allow_mouse_input && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        const ControllerPrompts prompts = GetActiveControllerPrompts();

        // Pulsante Chiudi in alto a destra nell'header
        const Rectangle btn_cancel{
            static_cast<float>(bx + box_w - 142),
            static_cast<float>(by + 6),
            126.0f,
            26.0f
        };
        const bool hover_cancel = allow_mouse_input && CheckCollisionPointRec(mouse, btn_cancel);
        DrawRectangleRec(btn_cancel, hover_cancel ? Color{62, 82, 104, 255} : Color{32, 44, 58, 245});
        DrawRectangleLinesEx(btn_cancel, 1.3f, hover_cancel ? pal::kAmberGlow : pal::kBronze);
        DrawTextShadow(
            prompts.controller_active ? TextFormat("%s Chiudi", prompts.cancel) : "[ESC / X] Chiudi",
            static_cast<int>(btn_cancel.x) + 12,
            static_cast<int>(btn_cancel.y) + 6,
            12,
            pal::kParchmentText);

        // ---------------------------------------------------------------------
        // RIGA 1 (by + 44): Barra visuale a 5 passi del ciclo scientifico
        // 1.OSSERVA -> 2.IPOTESI -> 3.SPERIMENTA -> 4.CONSEGUENZA -> 5.SOLUZIONE
        // ---------------------------------------------------------------------
        const eco::InquiryStage area_stage = world.CurrentAreaInquiryStage();
        int active_step = std::max(
            static_cast<int>(world.calibration_modal.modal_stage),
            static_cast<int>(area_stage));
        if (world.calibration_modal.trials_run > 0) {
            active_step = std::max(active_step, static_cast<int>(eco::InquiryStage::ConsequenceSeen));
        }
        if (machine.state != eco::MachineState::Broken) {
            active_step = static_cast<int>(eco::InquiryStage::Resolved);
        }

        static constexpr const char* kStepLabels[5] = {
            "1. OSSERVA",
            "2. IPOTESI",
            "3. SPERIMENTA",
            "4. CONSEGUENZA",
            "5. SOLUZIONE"
        };
        const float step_y = static_cast<float>(by + 44);
        const float step_w = 202.0f;
        const float step_gap = 15.5f;
        for (int s = 0; s < 5; ++s) {
            const float sx = static_cast<float>(bx + 24) + static_cast<float>(s) * (step_w + step_gap);
            const Rectangle sr{sx, step_y, step_w, 26.0f};
            const bool done_or_cur = (s <= active_step);
            const bool is_cur = (s == active_step);
            const Color scol = is_cur
                                   ? pal::kAmberGlow
                                   : (done_or_cur ? pal::kJadeGlow : Color{92, 112, 128, 185});
            DrawRectangleRec(
                sr,
                is_cur ? Color{36, 48, 34, 240}
                       : (done_or_cur ? Color{18, 38, 44, 235} : Color{14, 22, 32, 210}));
            DrawRectangleLinesEx(sr, is_cur ? 2.0f : 1.2f, scol);
            const int tw = MeasureUiText(kStepLabels[s], 12);
            DrawTextShadow(
                kStepLabels[s],
                static_cast<int>(sr.x + (sr.width - static_cast<float>(tw)) * 0.5f),
                static_cast<int>(sr.y) + 7,
                12,
                scol);
            if (s < 4) {
                DrawTextShadow(
                    "->",
                    static_cast<int>(sr.x + sr.width + 2.0f),
                    static_cast<int>(sr.y) + 7,
                    12,
                    done_or_cur ? pal::kJadeGlow : Color{92, 112, 128, 170});
            }
        }

        // ---------------------------------------------------------------------
        // RIGA 2 (by + 76): Selezione dell'Ipotesi (Tastiera [1-3 / I], Pad [LT], Mouse)
        // ---------------------------------------------------------------------
        if (allow_mouse_input) {
            if (IsKeyPressed(KEY_ONE)) {
                world.calibration_modal.SelectHypothesis(eco::HypothesisKind::HarmonicEquilibrium);
                world.AdvanceInquiryStageForArea(world.ActiveLocation(), world.current_zone, eco::InquiryStage::Hypothesize);
            } else if (IsKeyPressed(KEY_TWO)) {
                world.calibration_modal.SelectHypothesis(eco::HypothesisKind::CriticalOverdrive);
                world.AdvanceInquiryStageForArea(world.ActiveLocation(), world.current_zone, eco::InquiryStage::Hypothesize);
            } else if (IsKeyPressed(KEY_THREE)) {
                world.calibration_modal.SelectHypothesis(eco::HypothesisKind::EmergencyBypass);
                world.AdvanceInquiryStageForArea(world.ActiveLocation(), world.current_zone, eco::InquiryStage::Hypothesize);
            }
        }

        DrawTextShadow(
            prompts.controller_active
                ? TextFormat("Ipotesi %s:", prompts.hypothesis)
                : "Ipotesi [1-3 / I / Click]:",
            bx + 24,
            by + 83,
            13,
            pal::kParchmentText);

        static constexpr eco::HypothesisKind kHypKinds[3] = {
            eco::HypothesisKind::HarmonicEquilibrium,
            eco::HypothesisKind::CriticalOverdrive,
            eco::HypothesisKind::EmergencyBypass
        };
        static constexpr const char* kHypLabels[3] = {
            "[1] Equilibrio Armonico (Pulita)",
            "[2] Regime Supercritico (Rischiosa)",
            "[3] Combustione a Pece (Sporca)"
        };
        const Color hyp_cols[3] = {pal::kAccentClean, pal::kAmberGlow, pal::kAccentDirty};

        for (int h = 0; h < 3; ++h) {
            const Rectangle hr{
                static_cast<float>(bx + 216 + h * 296),
                static_cast<float>(by + 76),
                284.0f,
                26.0f
            };
            const bool sel = (world.calibration_modal.selected_hypothesis == kHypKinds[h]);
            const bool hov = allow_mouse_input && CheckCollisionPointRec(mouse, hr);
            DrawRectangleRec(
                hr,
                sel ? Color{24, 48, 56, 245} : (hov ? Color{22, 34, 46, 235} : Color{14, 22, 32, 220}));
            DrawRectangleLinesEx(hr, sel ? 2.2f : 1.2f, sel ? hyp_cols[h] : pal::WithAlpha(hyp_cols[h], 145));
            const int htw = MeasureUiText(kHypLabels[h], 12);
            DrawTextShadow(
                kHypLabels[h],
                static_cast<int>(hr.x + (hr.width - static_cast<float>(htw)) * 0.5f),
                static_cast<int>(hr.y) + 7,
                12,
                sel ? hyp_cols[h] : pal::kParchmentText);
            if (mouse_clicked && hov) {
                world.calibration_modal.SelectHypothesis(kHypKinds[h]);
                world.AdvanceInquiryStageForArea(world.ActiveLocation(), world.current_zone, eco::InquiryStage::Hypothesize);
            }
        }

        // ---------------------------------------------------------------------
        // SEZIONE CENTRALE (by + 110 .. by + 292):
        // Sinistra: Cursore Principale + Cursore Stabilizzatore + Strumenti Reliquia
        // Destra: Diagramma Fisico Interattivo (NON rivela Target gratis!)
        // ---------------------------------------------------------------------
        const bool has_gnomon_for_target =
            world.collectibles.IsCollected(eco::CollectibleId::Dawn_BrassGnomon) &&
            world.last_scan.has_target &&
            world.last_scan.target_type == machine.type;
        const bool target_revealed =
            !hard_mode &&
            (world.calibration_modal.harmonic_band_revealed ||
             world.calibration_modal.trials_run > 0 ||
             ov.show_raw_numbers ||
             has_gnomon_for_target);

        const Rectangle left_bench{
            static_cast<float>(bx + 24),
            static_cast<float>(by + 110),
            684.0f,
            182.0f
        };
        DrawRectangleRec(left_bench, Color{12, 20, 30, 236});
        DrawRectangleLinesEx(left_bench, 1.3f, pal::WithAlpha(hard_mode ? pal::kAmberGlow : pal::kUiBorderBronze, 195));

        if (hard_mode) {
            const std::string hard_banner = FitModalText(
                "FISICA PURA: tacche, barre verdi e percentuali live rimosse. Calcola il valore e usa Rotellina/Slider (Shift=Fine)!",
                660,
                12);
            DrawTextShadow(hard_banner.c_str(), bx + 36, by + 117, 12, pal::kAmberGlow);
        } else if (target_revealed) {
            const float half_w =
                machine.clean_stabilizer_half_window *
                (world.calibration_modal.precision_relic_active ? 1.30f : 1.0f);
            const std::string rev_txt = FitModalText(
                TextFormat(
                    "Rilievo Svelato: Banda Armonica ~= %.1f %s  |  Stabilizzatore Armonico %.0f%%..%.0f%%%s",
                    machine.target_param,
                    eco::MachineUnit(machine.type),
                    std::max(0.0f, (machine.clean_stabilizer_target - half_w) * 100.0f),
                    std::min(100.0f, (machine.clean_stabilizer_target + half_w) * 100.0f),
                    world.calibration_modal.precision_relic_active ? " [Nonio I.2 +30%]" : ""),
                660,
                12);
            DrawTextShadow(rev_txt.c_str(), bx + 36, by + 117, 12, pal::kAccentClean);
        } else {
            DrawTextShadow(
                "Indagine Empirica: il target NON e' visibile a priori. Osserva gli indizi o premi [T] SPERIMENTA!",
                bx + 36,
                by + 117,
                12,
                pal::kAmberGlow);
        }

        // Slider 1: Parametro Principale [A/D, Frecce SX/DX, Mouse, Rotellina]
        const float min_param = machine.min_param;
        const float max_param = machine.max_param;
        const float span_param = std::max(0.001f, max_param - min_param);
        const Rectangle slider_rect{
            static_cast<float>(bx + 36),
            static_cast<float>(by + 156),
            660.0f,
            20.0f
        };
        const Rectangle stab_rect{
            static_cast<float>(bx + 36),
            static_cast<float>(by + 204),
            660.0f,
            20.0f
        };

        // Controllo fine tramite rotellina del mouse (Standard e Fisica Pura)
        if (allow_mouse_input) {
            const float wheel = GetMouseWheelMove();
            if (std::fabs(wheel) > 0.01f) {
                const bool fine_mod =
                    IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT) ||
                    world.calibration_modal.precision_relic_active;
                const bool coarse_mod =
                    IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
                const Rectangle stab_wheel_zone{
                    stab_rect.x - 10.0f,
                    stab_rect.y - 22.0f,
                    stab_rect.width + 20.0f,
                    stab_rect.height + 28.0f
                };
                if (CheckCollisionPointRec(mouse, stab_wheel_zone)) {
                    const float stab_step = fine_mod ? 0.005f : (coarse_mod ? 0.05f : 0.01f);
                    world.calibration_modal.AdjustStabilizer(wheel * stab_step, machine);
                } else {
                    const float step_mult = fine_mod ? 0.25f : (coarse_mod ? 5.0f : 1.0f);
                    world.calibration_modal.AdjustParameter(wheel * machine.step_param * step_mult, machine);
                }
                s_hard_reject_timer = 0.0f;
            }
        }

        if (mouse_down &&
            CheckCollisionPointRec(
                mouse,
                Rectangle{slider_rect.x - 8.0f, slider_rect.y - 6.0f, slider_rect.width + 16.0f, slider_rect.height + 12.0f})) {
            const float t = eco::Clamp((mouse.x - slider_rect.x) / slider_rect.width, 0.0f, 1.0f);
            world.calibration_modal.SetParameter(eco::Lerp(min_param, max_param, t), machine);
            s_hard_reject_timer = 0.0f;
        }

        DrawTextShadow(
            TextFormat(
                "1. Parametro Principale [A/D, Rotellina, Shift=Fine]: %.2f %s  (%.0f..%.0f)%s",
                world.calibration_modal.slider_value,
                eco::MachineUnit(machine.type),
                min_param,
                max_param,
                world.calibration_modal.precision_relic_active ? " [Nonio]" : ""),
            bx + 36,
            by + 138,
            13,
            pal::kParchmentText);

        const float norm_slider =
            eco::Clamp((world.calibration_modal.slider_value - min_param) / span_param, 0.0f, 1.0f);
        const Color slider_fill_col =
            hard_mode
                ? kHardNeutralCol
                : (world.calibration_modal.can_confirm_clean
                       ? pal::kAccentClean
                       : (world.calibration_modal.can_confirm_risky ? pal::kAmberGlow : Color{68, 168, 232, 235}));
        DrawProgressBar(slider_rect, norm_slider, slider_fill_col, pal::kBronzeBright);

        // Finestra supercritica e tacca target sul cursore principale (nascoste in Fisica Pura)
        if (!hard_mode) {
            const float r0 = eco::Clamp((machine.risky_param_min - min_param) / span_param, 0.0f, 1.0f);
            const float r1 = eco::Clamp((machine.risky_param_max - min_param) / span_param, 0.0f, 1.0f);
            if (r1 > r0) {
                const float rx = slider_rect.x + slider_rect.width * r0;
                const float rw = slider_rect.width * (r1 - r0);
                DrawRectangleRec(Rectangle{rx, slider_rect.y + 2.0f, rw, slider_rect.height - 4.0f}, Color{248, 178, 64, 72});
                DrawRectangleLinesEx(Rectangle{rx, slider_rect.y, rw, slider_rect.height}, 1.2f, pal::kAmberGlow);
            }
            if (target_revealed) {
                const float norm_target = eco::Clamp((machine.target_param - min_param) / span_param, 0.0f, 1.0f);
                const int target_px = static_cast<int>(slider_rect.x + slider_rect.width * norm_target);
                DrawLine(target_px, static_cast<int>(slider_rect.y - 4), target_px, static_cast<int>(slider_rect.y + slider_rect.height + 4), pal::kAccentClean);
            }
        }

        // Slider 2: Stabilizzatore Armonico / Apertura Secondaria [W/S, Frecce SU/GIU, Mouse, Rotellina]
        if (mouse_down &&
            CheckCollisionPointRec(
                mouse,
                Rectangle{stab_rect.x - 8.0f, stab_rect.y - 6.0f, stab_rect.width + 16.0f, stab_rect.height + 12.0f})) {
            const float t = eco::Clamp((mouse.x - stab_rect.x) / stab_rect.width, 0.0f, 1.0f);
            world.calibration_modal.SetStabilizer(t, machine);
            s_hard_reject_timer = 0.0f;
        }

        const bool in_harm_stab = machine.IsInHarmonicStabilizerBand(
            world.calibration_modal.stabilizer_value,
            world.calibration_modal.precision_relic_active);
        const bool in_crit_stab =
            (world.calibration_modal.stabilizer_value <= machine.risky_max_stabilizer);

        DrawTextShadow(
            hard_mode
                ? TextFormat(
                      "2. Stabilizzatore / Valvola [W/S, Rotellina]: %.0f%%   (Calibrazione Manuale)",
                      world.calibration_modal.stabilizer_value * 100.0f)
                : TextFormat(
                      "2. Stabilizzatore / Valvola [W/S o SU/GIU]: %.0f%%   (%s)",
                      world.calibration_modal.stabilizer_value * 100.0f,
                      in_harm_stab
                          ? "In Banda Armonica Pulita"
                          : (in_crit_stab ? "Apertura Supercritica (<40%)" : "Fuori Banda")),
            bx + 36,
            by + 186,
            13,
            hard_mode
                ? pal::kParchmentText
                : (in_harm_stab ? pal::kAccentClean : (in_crit_stab ? pal::kAmberGlow : pal::kParchmentText)));

        DrawProgressBar(
            stab_rect,
            eco::Clamp(world.calibration_modal.stabilizer_value, 0.0f, 1.0f),
            hard_mode
                ? kHardNeutralCol
                : (in_harm_stab ? pal::kAccentClean : (in_crit_stab ? pal::kAmberGlow : Color{92, 178, 224, 235})),
            pal::kBronze);

        // Bande visive sullo Stabilizzatore (nascoste in Fisica Pura)
        if (!hard_mode) {
            const float cw = stab_rect.width * eco::Clamp(machine.risky_max_stabilizer, 0.0f, 1.0f);
            DrawRectangleLinesEx(Rectangle{stab_rect.x, stab_rect.y, cw, stab_rect.height}, 1.1f, pal::WithAlpha(pal::kAmberGlow, 175));

            const float hw =
                machine.clean_stabilizer_half_window *
                (world.calibration_modal.precision_relic_active ? 1.30f : 1.0f);
            const float h0 = eco::Clamp(machine.clean_stabilizer_target - hw, 0.0f, 1.0f);
            const float h1 = eco::Clamp(machine.clean_stabilizer_target + hw, 0.0f, 1.0f);
            const float hx = stab_rect.x + stab_rect.width * h0;
            const float hwidth = stab_rect.width * std::max(0.02f, h1 - h0);
            DrawRectangleRec(Rectangle{hx, stab_rect.y + 2.0f, hwidth, stab_rect.height - 4.0f}, Color{64, 232, 184, 70});
            DrawRectangleLinesEx(Rectangle{hx, stab_rect.y, hwidth, stab_rect.height}, 1.3f, pal::kAccentClean);
        }

        // Indicatori Rendimento, Resa Critica, Stress (nascosti pre-prova in Fisica Pura)
        if (hard_mode) {
            if (s_hard_reject_timer > 0.0f) {
                DrawTextShadow(
                    "FUORI BANDA: calibrazione rifiutata! Ricalcola il parametro o esegui [T] SPERIMENTA.",
                    bx + 36,
                    by + 234,
                    13,
                    pal::kAccentDirty);
            } else {
                DrawTextShadow(
                    "Anteprima live disattivata (Fisica Pura) — Verifica il tuo calcolo con [T] SPERIMENTA o conferma.",
                    bx + 36,
                    by + 234,
                    13,
                    pal::kAmberGlow);
            }
        } else {
            const float eff_pct = world.calibration_modal.preview_efficiency * 100.0f;
            const float thresh_pct = machine.Threshold() * 100.0f;
            const float risky_pct = world.calibration_modal.preview_risky_output * 100.0f;
            const float stress_pct = world.calibration_modal.preview_stress * 100.0f;
            DrawTextShadow(
                TextFormat(
                    "Rendimento Armonico: %.1f%% (Soglia >= %.0f%%)  |  Resa Critica: %.0f%%  |  Stress: %.0f%%",
                    eff_pct,
                    thresh_pct,
                    risky_pct,
                    stress_pct),
                bx + 36,
                by + 234,
                13,
                world.calibration_modal.can_confirm_clean
                    ? pal::kAccentClean
                    : (world.calibration_modal.can_confirm_risky ? pal::kAmberGlow : pal::kParchmentText));
        }

        std::string tools_active;
        if (world.calibration_modal.precision_relic_active) {
            tools_active += "[Stele I.2: Nonio 0.25x e Tolleranza +30%] ";
        }
        if (world.calibration_modal.piezometric_relic_active) {
            const float p_kpa = (eco::kWaterDensity * eco::kGravityWater * world.calibration_modal.slider_value) / 1000.0f;
            tools_active += TextFormat("[Stele II.2: Stevino %.1f kPa] ", p_kpa);
        }
        if (world.calibration_modal.olive_seed_bonus_active) {
            tools_active += "[Seme I.4: Guarigione -22% Foschia]";
        }
        if (tools_active.empty()) {
            tools_active = "Strumenti Reliquia: esplora la zona per trovare Stele di Misura o Semi d'Ulivo.";
        }
        const std::string fitted_tools = FitModalText(tools_active, 660, 12);
        DrawTextShadow(
            fitted_tools.c_str(),
            bx + 36,
            by + 258,
            12,
            (world.calibration_modal.precision_relic_active ||
             world.calibration_modal.piezometric_relic_active ||
             world.calibration_modal.olive_seed_bonus_active)
                ? pal::kJadeGlow
                : Color{168, 198, 212, 225});

        // Diagramma fisico interattivo a destra
        const Rectangle diag_rect{
            static_cast<float>(bx + 720),
            static_cast<float>(by + 110),
            376.0f,
            182.0f
        };
        DrawCalibrationDiagram(machine, world, diag_rect, target_revealed, hard_mode);

        // ---------------------------------------------------------------------
        // RIGA 3 (by + 298 .. by + 408): 3. SPERIMENTA (Impulso di Prova) & 4. CONSEGUENZA
        // ---------------------------------------------------------------------
        const Rectangle trial_panel{
            static_cast<float>(bx + 24),
            static_cast<float>(by + 298),
            1072.0f,
            110.0f
        };
        DrawRectangleRec(trial_panel, Color{14, 24, 36, 240});
        DrawRectangleLinesEx(trial_panel, 1.4f, pal::WithAlpha(pal::kUiBorderCyan, 215));

        const Rectangle btn_trial{
            static_cast<float>(bx + 36),
            static_cast<float>(by + 310),
            274.0f,
            52.0f
        };
        const bool hover_trial = allow_mouse_input && CheckCollisionPointRec(mouse, btn_trial);
        DrawRectangleRec(btn_trial, hover_trial ? Color{32, 96, 128, 255} : Color{22, 68, 94, 250});
        DrawRectangleLinesEx(btn_trial, hover_trial ? 2.4f : 1.8f, pal::kJadeGlow);
        DrawTextShadow(
            prompts.controller_active
                ? TextFormat("%s SPERIMENTA", prompts.trial)
                : "[T / SPAZIO] SPERIMENTA",
            static_cast<int>(btn_trial.x) + 16,
            static_cast<int>(btn_trial.y) + 9,
            15,
            pal::kJadeGlow);
        DrawTextShadow(
            "Invia Impulso di Prova nel Mondo",
            static_cast<int>(btn_trial.x) + 16,
            static_cast<int>(btn_trial.y) + 30,
            12,
            pal::kParchmentText);
        DrawTextShadow(
            TextFormat("Prove eseguite: %d", world.calibration_modal.trials_run),
            bx + 42,
            by + 372,
            12,
            pal::kAmberGlow);

        const int rep_x = bx + 326;
        const int rep_max_w = 754;
        const eco::TrialExperimentReport& tr = world.calibration_modal.last_trial;
        if (tr.executed) {
            const char* dir_str =
                (tr.error_direction < 0)
                    ? "BASSO (Aumenta parametro)"
                    : ((tr.error_direction > 0) ? "ALTO (Riduci parametro)" : "IN BANDA OTTIMALE");
            const Color rep_col = tr.harmonic_band_hit
                                      ? pal::kAccentClean
                                      : (tr.critical_band_hit ? pal::kAmberGlow : Color{118, 224, 248, 255});
            const std::string line1 = FitModalText(
                TextFormat(
                    "REPORT ESPERIMENTO #%d (%s): Rendimento = %.1f%% | Stress = %.0f%% | Direzione: %s",
                    world.calibration_modal.trials_run,
                    eco::HypothesisKindName(tr.tested_hypothesis),
                    tr.measured_efficiency * 100.0f,
                    tr.thermal_or_hydraulic_stress * 100.0f,
                    dir_str),
                rep_max_w,
                13);
            DrawTextShadow(line1.c_str(), rep_x, by + 308, 13, rep_col);

            const std::string line2 = FitModalText(
                std::string("Misura Empirica: ") +
                    ((tr.empirical_reading != nullptr && tr.empirical_reading[0] != '\0')
                         ? tr.empirical_reading
                         : "Rilievo strumentale completato."),
                rep_max_w,
                12);
            DrawTextShadow(line2.c_str(), rep_x, by + 332, 12, pal::kParchmentText);

            const std::string line3 = FitModalText(
                std::string("Conseguenza Ambientale Innescata: ") +
                    ((tr.world_consequence != nullptr && tr.world_consequence[0] != '\0')
                         ? tr.world_consequence
                         : "Risposta fisica propagata nella zona."),
                rep_max_w,
                12);
            DrawTextShadow(line3.c_str(), rep_x, by + 354, 12, pal::kAmberGlow);

            const char* status_hint =
                tr.harmonic_band_hit
                    ? "-> Finestra Armonica centrata! Puoi confermare [INVIO] Soluzione Pulita Lenta."
                    : (tr.critical_band_hit
                           ? "-> Finestra Supercritica attiva! Puoi scegliere [R] Rischiosa o affinare per l'Armonia."
                           : "-> Regola il cursore seguendo la direzione fisica indicata o riprova [T] Sperimenta.");
            DrawTextShadow(status_hint, rep_x, by + 378, 12, pal::kJadeGlow);
        } else {
            DrawTextShadow(
                "3. SPERIMENTA & 4. CONSEGUENZA: Nessun impulso di prova ancora eseguito su questo monumento.",
                rep_x,
                by + 312,
                13,
                pal::kAmberGlow);
            DrawTextShadow(
                "Premi [T], [SPAZIO] o clicca 'SPERIMENTA' per inviare un impulso reale: misurerai rendimento,",
                rep_x,
                by + 338,
                12,
                pal::kParchmentText);
            DrawTextShadow(
                "stress termico/idraulico, direzione dell'errore e vedrai una conseguenza tangibile nella scena!",
                rep_x,
                by + 358,
                12,
                pal::kParchmentText);
            DrawTextShadow(
                "Se possiedi la Stele della Prima Misura (I.2), la prima prova svela subito la banda armonica.",
                rep_x,
                by + 380,
                12,
                pal::kJadeGlow);
        }

        // ---------------------------------------------------------------------
        // RIGA 4 (by + 416 .. by + 626): 5. TRE SCHEDE SOLUZIONE FIANCO A FIANCO
        // 1. Pulita Lenta (Armonia) | 2. Rischiosa ma Efficiente | 3. Sporca Immediata
        // ---------------------------------------------------------------------
        const float card_y = static_cast<float>(by + 416);
        const float card_w = 346.0f;
        const float card_h = 210.0f;
        const float card_gap = 17.0f;

        const Rectangle card_clean{static_cast<float>(bx + 24), card_y, card_w, card_h};
        const Rectangle card_risky{static_cast<float>(bx + 24) + card_w + card_gap, card_y, card_w, card_h};
        const Rectangle card_dirty{static_cast<float>(bx + 24) + (card_w + card_gap) * 2.0f, card_y, card_w, card_h};

        const bool hover_clean = allow_mouse_input && CheckCollisionPointRec(mouse, card_clean);
        const bool hover_risky = allow_mouse_input && CheckCollisionPointRec(mouse, card_risky);
        const bool hover_dirty = allow_mouse_input && machine.allow_bypass && CheckCollisionPointRec(mouse, card_dirty);

        auto draw_solution_card = [&](Rectangle cr,
                                      const char* header_title,
                                      const char* status_line,
                                      const char* reward_line,
                                      const char* conseq_paths,
                                      const char* conseq_eco,
                                      const char* conseq_npc,
                                      const char* action_prompt,
                                      Color accent,
                                      bool ready,
                                      bool hovered) {
            DrawRectangleRec(
                cr,
                ready
                    ? (hovered ? Color{22, 48, 54, 250} : Color{14, 32, 40, 245})
                    : Color{14, 20, 28, 228});
            DrawRectangleRec(
                Rectangle{cr.x, cr.y, cr.width, 26.0f},
                ready ? pal::WithAlpha(accent, 58) : Color{22, 30, 40, 235});
            DrawRectangleLinesEx(
                cr,
                hovered ? 2.4f : (ready ? 1.9f : 1.1f),
                ready ? accent : pal::WithAlpha(accent, 115));

            const int cx_txt = static_cast<int>(cr.x) + 12;
            const int max_cw = static_cast<int>(cr.width) - 24;

            DrawTextShadow(
                FitModalText(header_title, max_cw, 13).c_str(),
                cx_txt,
                static_cast<int>(cr.y) + 6,
                13,
                ready ? accent : pal::WithAlpha(accent, 210));

            DrawTextShadow(
                FitModalText(status_line, max_cw, 12).c_str(),
                cx_txt,
                static_cast<int>(cr.y) + 34,
                12,
                ready ? pal::kJadeGlow : Color{186, 174, 152, 230});

            DrawTextShadow(
                FitModalText(reward_line, max_cw, 12).c_str(),
                cx_txt,
                static_cast<int>(cr.y) + 54,
                12,
                pal::kBronzeBright);

            DrawDivider(cx_txt, static_cast<int>(cr.y) + 74, max_cw, pal::WithAlpha(accent, 140));
            DrawTextShadow("ANTEPRIMA CONSEGUENZE:", cx_txt, static_cast<int>(cr.y) + 81, 11, accent);
            DrawTextShadow(FitModalText(conseq_paths, max_cw, 11).c_str(), cx_txt, static_cast<int>(cr.y) + 99, 11, pal::kParchmentText);
            DrawTextShadow(FitModalText(conseq_eco, max_cw, 11).c_str(), cx_txt, static_cast<int>(cr.y) + 117, 11, pal::kParchmentText);
            DrawTextShadow(FitModalText(conseq_npc, max_cw, 11).c_str(), cx_txt, static_cast<int>(cr.y) + 135, 11, pal::kParchmentText);

            const Rectangle btn_r{cr.x + 10.0f, cr.y + cr.height - 42.0f, cr.width - 20.0f, 32.0f};
            DrawRectangleRec(
                btn_r,
                ready
                    ? (hovered ? pal::WithAlpha(accent, 96) : pal::WithAlpha(accent, 52))
                    : Color{24, 32, 42, 225});
            DrawRectangleLinesEx(btn_r, ready ? 1.8f : 1.0f, ready ? accent : GRAY);
            const std::string act_fit = FitModalText(action_prompt, static_cast<int>(btn_r.width) - 14, 12);
            const int atw = MeasureUiText(act_fit.c_str(), 12);
            DrawTextShadow(
                act_fit.c_str(),
                static_cast<int>(btn_r.x + (btn_r.width - static_cast<float>(atw)) * 0.5f),
                static_cast<int>(btn_r.y) + 9,
                12,
                ready ? RAYWHITE : Color{150, 164, 176, 215});
        };

        // 1. Scheda Pulita Lenta (Armonia)
        const bool trial_matches_current =
            tr.executed &&
            std::fabs(tr.tested_param - world.calibration_modal.slider_value) < 1e-3f &&
            std::fabs(tr.tested_stabilizer - world.calibration_modal.stabilizer_value) < 1e-3f;
        const bool show_clean_ready =
            hard_mode ? (trial_matches_current && tr.harmonic_band_hit) : world.calibration_modal.can_confirm_clean;
        const bool show_risky_ready =
            hard_mode ? (trial_matches_current && tr.critical_band_hit) : world.calibration_modal.can_confirm_risky;

        const float clean_poll_drop =
            world.calibration_modal.olive_seed_bonus_active ? 22.0f : machine.clean_pollution_reduction;
        draw_solution_card(
            card_clean,
            "1. PULITA LENTA (ARMONIA)",
            show_clean_ready
                ? "STATO: PRONTA (Mira + Stab. Armonico)"
                : (hard_mode ? "FISICA PURA: Verifica con [T] o conferma" : "RICHIEDE: Rend. >= Soglia e Stab. Armonico"),
            TextFormat("Bilancio: +%.0f MW Energia | -%.0f%% Foschia", machine.clean_energy_reward, clean_poll_drop),
            "- Percorsi: purifica i rovi e apre sentieri verdi",
            "- Ecosistema: rifioritura flora e ritorno fauna",
            "- NPC: conquista fiducia di Clara e Lorenzo",
            prompts.controller_active
                ? TextFormat("%s CONFERMA ARMONIA PULITA", prompts.confirm)
                : "[INVIO / Click] CONFERMA ARMONIA PULITA",
            pal::kAccentClean,
            show_clean_ready,
            hover_clean);

        // 2. Scheda Rischiosa ma Efficiente (Regime Critico)
        const char* risky_paths =
            (machine.type == eco::MachineType::SolarArray)
                ? "- Percorsi: vetrifica rovi -> apre Cresta Arsa"
                : ((machine.type == eco::MachineType::HydroTurbine)
                       ? "- Percorsi: pressione apre Sfioratore Alto"
                       : "- Percorsi: apre varco ad alta pressione");
        const char* risky_eco =
            (machine.type == eco::MachineType::HydroTurbine)
                ? "- Flussi: +45% velocita' correnti nei Canali"
                : "- Clima: riverbero termico e scintille critiche";
        draw_solution_card(
            card_risky,
            "2. RISCHIOSA EFFICIENTE (CRITICA)",
            show_risky_ready
                ? "STATO: PRONTA (Finestra Supercritica)"
                : (hard_mode ? "FISICA PURA: Verifica con [T] o conferma" : "RICHIEDE: Param. Critico e Stab. <= 40%"),
            TextFormat("Bilancio: +%.0f MW Max Resa | +%.0f%% Stress", machine.risky_energy_reward, machine.risky_pollution_increase),
            risky_paths,
            risky_eco,
            "- NPC: Clara e Lorenzo in allerta tecnica",
            prompts.controller_active
                ? TextFormat("%s ATTIVA REGIME CRITICO", prompts.risky)
                : "[R / Click] ATTIVA REGIME CRITICO",
            pal::kAmberGlow,
            show_risky_ready,
            hover_risky);

        // 3. Scheda Sporca Immediata (Bypass a Pece)
        draw_solution_card(
            card_dirty,
            "3. SPORCA IMMEDIATA (FORNACE PECE)",
            machine.allow_bypass
                ? "STATO: SEMPRE DISPONIBILE (Zero Mira)"
                : "BLOCCATA: Astrolabio richiede pura risonanza",
            TextFormat("Bilancio: +%.0f MW Sporchi | +%.0f%% Foschia Pece", machine.dirty_energy_reward, machine.dirty_pollution_increase),
            "- Percorsi: rovi di pece bloccano sentieri bassi",
            "- Ecosistema: piante spoglie e fauna azzerata",
            "- NPC: Clara e Lorenzo si riparano dai fumi",
            machine.allow_bypass
                ? (prompts.controller_active
                       ? TextFormat("%s BYPASS FORNACE A PECE", prompts.bypass)
                       : "[B / Click] BYPASS FORNACE A PECE")
                : "NON DISPONIBILE SULL'ASTROLABIO",
            pal::kAccentDirty,
            machine.allow_bypass,
            hover_dirty);

        // Gestione click mouse su Trial, 3 Soluzioni e Chiudi
        if (mouse_clicked) {
            if (CheckCollisionPointRec(mouse, btn_cancel)) {
                world.calibration_modal.Cancel();
            } else if (CheckCollisionPointRec(mouse, btn_trial)) {
                (void)world.ExecuteModalTrial();
                s_hard_reject_timer = 0.0f;
            } else if (CheckCollisionPointRec(mouse, card_clean)) {
                if (!world.ExecuteModalConfirmClean() && hard_mode) {
                    s_hard_reject_timer = 2.6f;
                }
            } else if (CheckCollisionPointRec(mouse, card_risky)) {
                if (!world.ExecuteModalConfirmRisky() && hard_mode) {
                    s_hard_reject_timer = 2.6f;
                }
            } else if (machine.allow_bypass && CheckCollisionPointRec(mouse, card_dirty)) {
                (void)world.ExecuteModalBypass();
            }
        }
        // Feedback visivo anche su conferma da tastiera fuori banda in Fisica Pura
        if (allow_mouse_input && hard_mode && world.calibration_modal.is_open &&
            (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_R))) {
            s_hard_reject_timer = 2.6f;
        }
    }
}

} // namespace eco::app
