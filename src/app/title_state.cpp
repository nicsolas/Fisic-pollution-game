#include "app/title_state.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "app/audio/audio_engine.h"
#include "app/game_state_manager.h"
#include "app/intro_state.h"
#include "app/overworld_state.h"
#include "app/raylib_input.h"
#include "app/render/palette.h"
#include "app/render/ui_style.h"
#include "app/structure_state.h"
#include "core/input_types.h"
#include "core/math_types.h"
#include "core/save_data.h"
#include "core/simulation.h"

namespace eco::app {

namespace {

void DrawDefaultMeridiaPanorama(int w, int h, float t, float heal_wave) {
    const float poll_ratio = 1.0f - heal_wave;
    const Color sky_top = pal::LerpColor(Color{30, 24, 24, 255}, Color{16, 44, 68, 255}, heal_wave);
    const Color sky_bot = pal::WorldColor(pal::kSickSky, pal::kHealthySky, poll_ratio);
    DrawRectangleGradientV(0, 0, w, h, sky_top, sky_bot);

    const float sun_x = static_cast<float>(w) * 0.74f;
    const float sun_y = 216.0f + std::sin(t * 0.65f) * 7.0f;
    const Color arc_col = pal::LerpColor(Color{190, 145, 95, 28}, Color{110, 220, 200, 38}, heal_wave);
    DrawCircleLines(static_cast<int>(sun_x), static_cast<int>(sun_y), 148.0f, arc_col);
    DrawCircleLines(static_cast<int>(sun_x), static_cast<int>(sun_y), 215.0f, arc_col);

    const Color sun_glow = pal::LerpColor(Color{215, 118, 58, 52}, Color{140, 240, 210, 72}, heal_wave);
    const Color sun_core = pal::LerpColor(Color{240, 165, 88, 225}, Color{245, 248, 220, 242}, heal_wave);
    DrawCircleV({sun_x, sun_y}, 94.0f + std::sin(t * 1.6f) * 5.0f, sun_glow);
    DrawCircleV({sun_x, sun_y}, 50.0f, sun_core);

    const Color mountain_far = pal::LerpColor(Color{52, 43, 38, 255}, Color{36, 70, 88, 255}, heal_wave);
    for (int x = 0; x < w; x += 4) {
        const float fx = static_cast<float>(x);
        const float ridge =
            352.0f - 58.0f * std::sin(fx * 0.0042f + 0.35f) - 24.0f * std::cos(fx * 0.011f);
        DrawRectangle(x, static_cast<int>(ridge), 4, h - static_cast<int>(ridge), mountain_far);
    }

    const Color monument_dark = pal::LerpColor(Color{38, 34, 34, 255}, Color{28, 50, 64, 255}, heal_wave);
    const Color bronze_trim = pal::LerpColor(Color{155, 110, 68, 200}, Color{198, 168, 108, 225}, heal_wave);

    // Torre del Pendolo
    const int tower_x = 152;
    const int tower_y = 248;
    DrawRectangle(tower_x, tower_y, 56, 165, monument_dark);
    DrawRectangle(tower_x - 8, tower_y - 10, 72, 14, monument_dark);
    DrawRectangleLines(tower_x, tower_y, 56, 165, bronze_trim);
    DrawCircle(tower_x + 28, tower_y + 38, 15.0f, Color{18, 30, 42, 255});
    DrawCircleLines(tower_x + 28, tower_y + 38, 15.0f, bronze_trim);
    const float pend_ang = std::sin(t * 2.1f) * 0.32f;
    const Vector2 pend_top{static_cast<float>(tower_x + 28), static_cast<float>(tower_y + 52)};
    const Vector2 pend_bob{pend_top.x + std::sin(pend_ang) * 44.0f, pend_top.y + std::cos(pend_ang) * 44.0f};
    DrawLineEx(pend_top, pend_bob, 2.0f, bronze_trim);
    DrawCircleV(pend_bob, 4.5f, pal::LerpColor(Color{235, 150, 75, 230}, Color{125, 245, 210, 240}, heal_wave));

    // Santuario dell'Armonia
    const int sanc_cx = 980;
    const int sanc_base_y = 385;
    DrawRectangle(sanc_cx - 98, sanc_base_y - 68, 196, 80, monument_dark);
    DrawCircleSector({static_cast<float>(sanc_cx), static_cast<float>(sanc_base_y - 68)}, 78.0f, 180.0f, 360.0f, 24, monument_dark);
    DrawRingLines({static_cast<float>(sanc_cx), static_cast<float>(sanc_base_y - 68)}, 77.0f, 79.0f, 180.0f, 360.0f, 24, bronze_trim);
    DrawCircleLines(sanc_cx, sanc_base_y - 148, 18.0f, bronze_trim);
    DrawCircle(sanc_cx, sanc_base_y - 148, 5.5f + std::sin(t * 3.0f) * 1.2f,
               pal::LerpColor(Color{230, 140, 72, 220}, Color{110, 245, 215, 245}, heal_wave));

    // Colline
    const Color hill_col = pal::LerpColor(Color{66, 56, 44, 255}, Color{42, 94, 72, 255}, heal_wave);
    for (int x = 0; x < w; x += 4) {
        const float fx = static_cast<float>(x);
        const float hill_y = 428.0f - 34.0f * std::sin(fx * 0.0058f + 1.6f) - 14.0f * std::sin(fx * 0.014f);
        DrawRectangle(x, static_cast<int>(hill_y), 4, h - static_cast<int>(hill_y), hill_col);
    }

    // Ponte degli Archi
    const int bridge_y = 422;
    const Color stone_body = pal::LerpColor(Color{74, 66, 58, 255}, Color{118, 128, 124, 255}, heal_wave);
    const Color stone_shadow = pal::LerpColor(Color{42, 38, 36, 255}, Color{54, 72, 80, 255}, heal_wave);
    DrawRectangle(0, bridge_y, w, 16, stone_body);
    DrawRectangle(0, bridge_y + 16, w, 5, bronze_trim);
    for (int ax = -20; ax < w + 112; ax += 112) {
        DrawRectangle(ax, bridge_y + 21, 22, 130, stone_shadow);
        DrawRingLines({static_cast<float>(ax + 67), static_cast<float>(bridge_y + 56)}, 36.0f, 41.0f, 180.0f, 360.0f, 18, stone_body);
        const float pulse = 0.55f + 0.45f * std::sin(t * 2.8f + static_cast<float>(ax) * 0.03f);
        const Color lamp = pal::LerpColor(Color{232, 132, 64, 195}, Color{110, 245, 215, static_cast<unsigned char>(160 + 90 * pulse)}, heal_wave);
        DrawCircle(ax + 11, bridge_y - 5, 4.0f, lamp);
    }

    const Color water_col = pal::WorldColor(pal::kSickWater, pal::kHealthyWater, poll_ratio);
    DrawRectangle(0, 535, w, 85, water_col);
    for (int wv = 0; wv < 20; ++wv) {
        const float wy = 544.0f + static_cast<float>(wv % 5) * 14.0f;
        const float wx = std::fmod(static_cast<float>(wv * 89) + t * 34.0f, static_cast<float>(w));
        DrawLineEx({wx, wy}, {wx + 46.0f, wy}, 2.0f, Color{205, 242, 245, 115});
    }
    DrawRectangle(0, 610, w, h - 610, Color{14, 20, 28, 255});
    DrawLine(0, 610, w, 610, bronze_trim);
}

// Disegna lo sfondo ridisegnato su misura per la zona/struttura dell'ultimo salvataggio!
void DrawSavedZoneBackdrop(const eco::SaveSummary& save, int w, int h, float t) {
    const float poll = eco::Clamp(save.pollution_level / 100.0f, 0.0f, 1.0f);
    const float heal = 1.0f - poll;

    if (save.mode == eco::GameMode::Structure) {
        const pal::StructurePalette sp = pal::GetStructurePalette(save.structure);
        const Color bg_top = pal::WorldColor(sp.bg_sick, sp.bg_healthy, poll);
        const Color bg_bot = pal::WorldColor(sp.arch_far_sick, sp.arch_far_healthy, poll);
        const Color accent = pal::WorldColor(sp.accent_sick, sp.accent_healthy, poll);
        const Color wall = pal::WorldColor(sp.arch_near_sick, sp.arch_near_healthy, poll);
        DrawRectangleGradientV(0, 0, w, h, bg_top, bg_bot);

        if (save.structure == eco::StructureId::HypogeanCisterns) {
            // Volte calcaree, cascate animate e bacino idrostatico di Stevino
            for (int ax = 60; ax < w + 160; ax += 220) {
                DrawRectangle(ax - 18, 80, 36, h - 180, wall);
                DrawRingLines({static_cast<float>(ax + 92), 185.0f}, 86.0f, 98.0f, 180.0f, 360.0f, 24, wall);
                // Cascata d'acqua animata
                const Color wf = pal::WithAlpha(accent, 95);
                DrawRectangle(ax + 76, 120, 32, h - 220, wf);
                for (int d = 0; d < 7; ++d) {
                    const float dy = 130.0f + std::fmod(t * 150.0f + static_cast<float>(d * 78 + ax), 440.0f);
                    DrawLineEx({static_cast<float>(ax + 84), dy}, {static_cast<float>(ax + 84), dy + 28.0f}, 2.0f, Color{215, 248, 255, 165});
                    DrawLineEx({static_cast<float>(ax + 100), dy + 18.0f}, {static_cast<float>(ax + 100), dy + 42.0f}, 2.0f, Color{215, 248, 255, 140});
                }
            }
            // Bacino sul fondo e ruota turbina Pelton a destra
            DrawRectangle(0, 560, w, h - 560, pal::WorldColor(pal::kSickWater, pal::kHealthyWater, poll));
            const Vector2 turb{980.0f, 510.0f};
            DrawCircleLines(static_cast<int>(turb.x), static_cast<int>(turb.y), 68.0f, accent);
            for (int b = 0; b < 8; ++b) {
                const float a = t * 1.8f + static_cast<float>(b) * eco::kPi * 0.25f;
                DrawLineEx(turb, {turb.x + std::cos(a) * 64.0f, turb.y + std::sin(a) * 64.0f}, 3.0f, pal::kBronzeBright);
            }
            DrawCircleV(turb, 16.0f, pal::kBronze);
        } else if (save.structure == eco::StructureId::PendulumTower) {
            // Oculi astronomici con fasci di luce e grande pendolo oscillante
            for (int ox = 220; ox <= 1060; ox += 420) {
                DrawCircle(ox, 260, 108.0f, Color{22, 34, 48, 220});
                DrawCircleLines(ox, 260, 108.0f, pal::kBronzeBright);
                DrawCircleLines(ox, 260, 92.0f, pal::WithAlpha(accent, 110));
                DrawTriangle(
                    {static_cast<float>(ox - 45), 260.0f},
                    {static_cast<float>(ox - 165), static_cast<float>(h)},
                    {static_cast<float>(ox + 95), static_cast<float>(h)},
                    pal::WithAlpha(pal::kAmberGlow, 24));
            }
            // Pilastri verticali e ballatoi
            for (int px = 80; px < w; px += 280) {
                DrawRectangle(px, 0, 26, h, pal::WithAlpha(wall, 190));
            }
            // Grande Pendolo Isochronos oscillante sul lato destro
            const Vector2 pivot{980.0f, 48.0f};
            const float ang = std::sin(t * 2.21f) * 0.36f; // T ~ 2.84 s
            const Vector2 bob{pivot.x + std::sin(ang) * 340.0f, pivot.y + std::cos(ang) * 340.0f};
            DrawRingLines(pivot, 336.0f, 344.0f, 65.0f, 115.0f, 20, pal::WithAlpha(pal::kBronzeBright, 130));
            DrawLineEx(pivot, bob, 4.0f, pal::kBronzeBright);
            DrawCircleV(bob, 28.0f, pal::kBronze);
            DrawCircleV(bob, 14.0f, accent);
        } else {
            // Camera Geotermica: colonne di basalto, archi in rame e vapori termali
            for (int bx = 40; bx < w; bx += 160) {
                DrawRectangle(bx, 60, 42, h - 140, wall);
                DrawRectangle(bx + 6, 60, 8, h - 140, pal::ShiftBrightness(wall, 18));
                DrawRingLines({static_cast<float>(bx + 100), 170.0f}, 68.0f, 76.0f, 180.0f, 360.0f, 20, pal::kBronze);
            }
            // Vene geotermiche e vasche di vapore
            DrawRectangle(0, 580, w, h - 580, Color{34, 24, 22, 255});
            for (int vx = 120; vx < w; vx += 240) {
                DrawRectangle(vx, 588, 120, 18, pal::WithAlpha(accent, 175));
                for (int p = 0; p < 5; ++p) {
                    const float py = 575.0f - std::fmod(t * 48.0f + static_cast<float>(p * 74 + vx), 360.0f);
                    const float px = static_cast<float>(vx + 60) + std::sin(t * 1.4f + static_cast<float>(p)) * 22.0f;
                    DrawCircleV({px, py}, 22.0f + static_cast<float>(p) * 5.0f, Color{230, 228, 225, 22});
                }
            }
        }

        // Banchina inferiore pulita per footer
        DrawRectangle(0, 622, w, h - 622, Color{12, 18, 26, 238});
        DrawLine(0, 622, w, 622, pal::kUiBorderBronze);
        return;
    }

    // Overworld: 6 distretti ridisegnati con palette reale del distretto + stato foschia salvato
    const pal::DistrictPalette dp = pal::GetDistrictPalette(save.zone);
    const Color ground = pal::WorldColor(dp.ground_sick, dp.ground_healthy, poll);
    const Color stone = pal::WorldColor(dp.path_sick, dp.path_healthy, poll);
    const Color accent = pal::WorldColor(dp.inlay_sick, dp.inlay_healthy, poll);

    const Color sky_top = pal::LerpColor(Color{34, 26, 24, 255}, pal::ShiftBrightness(accent, -55), heal);
    const Color sky_bot = pal::LerpColor(pal::kSickSky, pal::ShiftBrightness(ground, 15), heal);
    DrawRectangleGradientV(0, 0, w, h, sky_top, sky_bot);

    // Profilo montuoso comune ma intonato al distretto
    const Color far_ridge = pal::ShiftBrightness(ground, -28);
    for (int x = 0; x < w; x += 4) {
        const float fx = static_cast<float>(x);
        const float y = 360.0f - 52.0f * std::sin(fx * 0.0045f + static_cast<float>(save.zone)) -
                        20.0f * std::cos(fx * 0.011f);
        DrawRectangle(x, static_cast<int>(y), 4, h - static_cast<int>(y), far_ridge);
    }

    switch (save.zone) {
        case eco::ZoneId::DawnTerraces: {
            // Alba calda, terrazze a gradoni in travertino ed Eliostato Solare
            const Vector2 sun{990.0f, 195.0f + std::sin(t * 0.7f) * 6.0f};
            DrawCircleV(sun, 115.0f, pal::WithAlpha(pal::kAmberGlow, 52));
            DrawCircleV(sun, 54.0f, pal::LerpColor(Color{235, 165, 88, 230}, Color{250, 242, 200, 245}, heal));
            // Terrazze scalinate
            for (int step = 0; step < 4; ++step) {
                const int ty = 435 + step * 44;
                DrawRectangle(0, ty, w, 46, pal::ShiftBrightness(ground, -8 * step));
                DrawRectangle(0, ty, w, 7, stone);
            }
            // Grande Eliostato Solare a destra
            const Vector2 helio{970.0f, 430.0f};
            DrawRectangle(static_cast<int>(helio.x) - 14, static_cast<int>(helio.y), 28, 95, stone);
            DrawLineEx({helio.x - 56.0f, helio.y - 26.0f}, {helio.x + 56.0f, helio.y + 26.0f}, 9.0f, pal::kBronzeBright);
            DrawLineEx(helio, sun, 2.0f, pal::WithAlpha(accent, 95));
            break;
        }
        case eco::ZoneId::CanalQuarter: {
            // Acquedotto monumentale e Ponte degli Archi sull'acqua riflettente
            DrawDefaultMeridiaPanorama(w, h, t, heal);
            return;
        }
        case eco::ZoneId::WindPlateau: {
            // Altopiano d'ardesia, cipressi inclinati dal vento e Torre del Pendolo
            for (int x = 0; x < w; x += 4) {
                const float fx = static_cast<float>(x);
                const float y = 445.0f - 48.0f * std::sin(fx * 0.0038f + 0.4f);
                DrawRectangle(x, static_cast<int>(y), 4, h - static_cast<int>(y), ground);
            }
            // Cipressi oscillanti
            for (int cx = 90; cx < w; cx += 135) {
                if (cx > 310 && cx < 940) {
                    continue;
                }
                const float sway = std::sin(t * 2.4f + static_cast<float>(cx) * 0.02f) * 8.0f;
                const Vector2 base{static_cast<float>(cx), 540.0f};
                DrawTriangle(
                    {base.x + sway, base.y - 125.0f},
                    {base.x - 20.0f, base.y},
                    {base.x + 20.0f, base.y},
                    pal::ShiftBrightness(ground, -22));
            }
            // Torre del Pendolo a destra
            DrawRectangle(985, 220, 74, 280, stone);
            DrawCircle(1022, 268, 22.0f, Color{20, 32, 46, 255});
            DrawCircleLines(1022, 268, 22.0f, pal::kBronzeBright);
            // Linee di vento
            for (int s = 0; s < 10; ++s) {
                const float wy = 160.0f + static_cast<float>(s * 42);
                const float wx = std::fmod(t * 140.0f + static_cast<float>(s * 173), static_cast<float>(w + 180)) - 180.0f;
                DrawLineEx({wx, wy}, {wx + 120.0f, wy - 8.0f}, 2.0f, Color{210, 232, 240, 58});
            }
            break;
        }
        case eco::ZoneId::HangingGardens: {
            // Giardini terrazzati rigogliosi, vasche di giada e Risonatore Armonico vibrante
            for (int tier = 0; tier < 3; ++tier) {
                const int ty = 430 + tier * 60;
                DrawRectangle(0, ty, w, 65, pal::ShiftBrightness(ground, 10 - tier * 10));
                DrawRectangle(0, ty, w, 8, stone);
                // Vasche turchesi
                DrawRectangle(80 + tier * 110, ty + 18, 170, 26, pal::WorldColor(pal::kSickWater, pal::kHealthyWater, poll));
                DrawRectangle(930 - tier * 90, ty + 18, 170, 26, pal::WorldColor(pal::kSickWater, pal::kHealthyWater, poll));
            }
            // Risonatore Armonico a destra con onde di giada
            const Vector2 res{1010.0f, 380.0f};
            for (int r = 0; r < 4; ++r) {
                const float rad = 32.0f + std::fmod(t * 52.0f + static_cast<float>(r * 38), 150.0f);
                const float a = 1.0f - (rad - 32.0f) / 150.0f;
                DrawCircleLines(static_cast<int>(res.x), static_cast<int>(res.y), rad,
                                pal::WithAlpha(accent, static_cast<unsigned char>(160 * a)));
            }
            DrawPoly(res, 6, 26.0f, 30.0f, pal::kBronze);
            DrawPoly(res, 6, 15.0f, 30.0f, accent);
            break;
        }
        case eco::ZoneId::FumaroleCaldera: {
            // Basalto scuro, ciminiere delle fornaci e fessure geotermiche ambrate
            DrawRectangle(0, 440, w, h - 440, ground);
            for (int cx = 110; cx < w; cx += 280) {
                DrawRectangle(cx, 355, 36, 110, stone);
                DrawRectangle(cx - 6, 350, 48, 10, pal::kBronze);
                for (int p = 0; p < 5; ++p) {
                    const float py = 345.0f - std::fmod(t * 42.0f + static_cast<float>(p * 55 + cx), 240.0f);
                    const float px = static_cast<float>(cx + 18) + std::sin(t * 1.2f + static_cast<float>(p)) * 16.0f;
                    DrawCircleV({px, py}, 18.0f + static_cast<float>(p) * 4.0f,
                                pal::LerpColor(Color{95, 86, 78, 38}, Color{225, 232, 236, 32}, heal));
                }
            }
            // Crepe ambrate nel terreno
            for (int fx = 80; fx < w; fx += 190) {
                DrawLineEx({static_cast<float>(fx), 520.0f}, {static_cast<float>(fx + 62), 548.0f}, 3.0f,
                           pal::WithAlpha(accent, 175));
                DrawLineEx({static_cast<float>(fx + 62), 548.0f}, {static_cast<float>(fx + 108), 530.0f}, 2.5f,
                           pal::WithAlpha(accent, 145));
            }
            break;
        }
        case eco::ZoneId::HarmonySanctuary: {
            // Colonnato d'avorio, grande mosaico astronomico e Astrolabio Centrale radioso
            const Vector2 astro{975.0f, 310.0f};
            for (int r = 1; r <= 4; ++r) {
                DrawCircleLines(static_cast<int>(astro.x), static_cast<int>(astro.y),
                                36.0f * static_cast<float>(r),
                                pal::WithAlpha((r % 2 == 0) ? accent : pal::kBronzeBright, 155));
            }
            DrawCircleV(astro, 18.0f + std::sin(t * 3.0f) * 3.0f, accent);
            // Colonnato simmetrico
            DrawRectangle(0, 495, w, h - 495, stone);
            for (int col = 70; col < w; col += 140) {
                if (col > 270 && col < 920) {
                    continue;
                }
                DrawRectangle(col, 310, 26, 195, pal::kStoneIvory);
                DrawRectangle(col - 6, 304, 38, 12, pal::kBronzeBright);
            }
            break;
        }
        case eco::ZoneId::Count:
            break;
    }

    // Particelle atmosferiche intonate allo stato salvato
    for (int i = 0; i < 42; ++i) {
        const float fi = static_cast<float>(i);
        const float px = std::fmod(fi * 79.0f + t * (16.0f + static_cast<float>(i % 4) * 5.0f), static_cast<float>(w));
        const float py = std::fmod(fi * 47.0f + 560.0f - t * (10.0f + static_cast<float>(i % 3) * 4.0f), 580.0f);
        const Color pcol = pal::LerpColor(Color{165, 146, 132, 130}, accent, heal);
        DrawCircleV({px, py < 24.0f ? py + 520.0f : py}, 2.0f + 1.2f * heal, pcol);
    }

    DrawRectangle(0, 622, w, h - 622, Color{12, 18, 26, 238});
    DrawLine(0, 622, w, 622, pal::kUiBorderBronze);
}

} // namespace

TitleState::TitleState(AppContext& ctx) : ctx_(ctx) {}

void TitleState::RefreshSaveSummary() {
    save_summary_ = eco::ReadSaveSummary();
    menu_cursor_ = 0;
}

void TitleState::OnEnter() {
    anim_time_ = 0.0f;
    RefreshSaveSummary();
}

void TitleState::Update(float dt) {
    if (std::isfinite(dt) && dt > 0.0f) {
        anim_time_ += dt;
    }

    const bool has_save = save_summary_.exists;
    const int item_count = has_save ? 4 : 3;

    const MenuInputState menu_in = PollMenuInput(dt);
    const bool up_pressed = menu_in.up_pressed;
    const bool down_pressed = menu_in.down_pressed;
    if (up_pressed) {
        menu_cursor_ = (menu_cursor_ - 1 + item_count) % item_count;
        PlayUiSfx(SfxId::UiClick, 0.48f, 1.05f);
    }
    if (down_pressed) {
        menu_cursor_ = (menu_cursor_ + 1) % item_count;
        PlayUiSfx(SfxId::UiClick, 0.48f, 0.95f);
    }

    const int w = eco::kWindowWidth;
    const int btn_w = 430;
    const int btn_h = 44;
    const int btn_x = (w - btn_w) / 2;
    const int menu_top_y = has_save ? 404 : 426;

    const Vector2 mouse = GetMousePosition();
    const bool mouse_clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    int activated = -1;

    for (int i = 0; i < item_count; ++i) {
        const Rectangle r{
            static_cast<float>(btn_x),
            static_cast<float>(menu_top_y + i * 52),
            static_cast<float>(btn_w),
            static_cast<float>(btn_h)
        };
        if (CheckCollisionPointRec(mouse, r)) {
            if (GetMouseDelta().x != 0.0f || GetMouseDelta().y != 0.0f || mouse_clicked) {
                if (menu_cursor_ != i) {
                    menu_cursor_ = i;
                    PlayUiSfx(SfxId::UiClick, 0.40f, 1.02f);
                }
            }
            if (mouse_clicked) {
                activated = i;
            }
        }
    }

    if (activated < 0 && (menu_in.confirm_pressed || IsKeyPressed(KEY_E))) {
        activated = menu_cursor_;
    }

    if (activated < 0) {
        return;
    }

    PlayUiSfx(SfxId::UiClick, 0.65f, 1.08f);

    if (has_save) {
        switch (activated) {
            case 0: { // Riprendi da dove hai lasciato
                if (eco::LoadWorldFromFile(ctx_.world)) {
                    if (ctx_.world.current_mode == eco::GameMode::Structure) {
                        ctx_.state_manager.RequestTransition(std::make_unique<StructureState>(ctx_));
                    } else {
                        ctx_.state_manager.RequestTransition(std::make_unique<OverworldState>(ctx_));
                    }
                } else {
                    ctx_.world.Reset();
                    ctx_.world.EnsureInitialized();
                    ctx_.state_manager.RequestTransition(std::make_unique<IntroState>(ctx_));
                }
                break;
            }
            case 1: { // Nuova Partita
                ctx_.world.Reset();
                ctx_.world.EnsureInitialized();
                ctx_.state_manager.RequestTransition(std::make_unique<IntroState>(ctx_));
                break;
            }
            case 2: { // Impostazioni (Schermo, GPU & Audio)
                ctx_.open_settings_requested = true;
                break;
            }
            case 3: { // Esci
                ctx_.quit_requested = true;
                break;
            }
            default:
                break;
        }
    } else {
        switch (activated) {
            case 0: { // Inizia il Viaggio
                ctx_.world.Reset();
                ctx_.world.EnsureInitialized();
                ctx_.state_manager.RequestTransition(std::make_unique<IntroState>(ctx_));
                break;
            }
            case 1: { // Impostazioni
                ctx_.open_settings_requested = true;
                break;
            }
            case 2: { // Esci
                ctx_.quit_requested = true;
                break;
            }
            default:
                break;
        }
    }
}

void TitleState::Draw() {
    const int w = eco::kWindowWidth;
    const int h = eco::kWindowHeight;
    const float t = anim_time_;

    // 1. Sfondo dinamico: se esiste un salvataggio, ridisegna la zona esatta in cui hai lasciato!
    if (save_summary_.exists) {
        DrawSavedZoneBackdrop(save_summary_, w, h, t);
    } else {
        const float heal_wave = 0.48f + 0.36f * std::sin(t * 0.42f);
        DrawDefaultMeridiaPanorama(w, h, t, heal_wave);
    }

    // 2. Pannello centrale del Titolo
    const Rectangle title_box{220.0f, 36.0f, 840.0f, 206.0f};
    DrawPanel(title_box, Color{102, 214, 194, 232});

    const char* main_title = "EQUILIBRIO";
    const int title_size = 54;
    const int title_w = MeasureUiText(main_title, title_size);
    DrawTextShadow(main_title, (w - title_w) / 2, 52, title_size, Color{150, 248, 224, 255});

    const char* subtitle = "LA CONCA DI MERIDIA";
    const int sub_size = 22;
    const int sub_w = MeasureUiText(subtitle, sub_size);
    DrawTextShadow(subtitle, (w - sub_w) / 2, 120, sub_size, Color{238, 205, 125, 255});

    const char* tagline =
        "Misura la natura. Rifiuta le fornaci di pece. Ricostruisci la memoria dei Meridiani.";
    const int tag_w = MeasureUiText(tagline, 15);
    DrawTextShadow(tagline, (w - tag_w) / 2, 160, 15, Color{216, 232, 240, 255});

    const char* sub_tagline =
        "Sei distretti, tre strutture sotterranee e sedici reliquie per risvegliare l'Astrolabio.";
    const int sub_tag_w = MeasureUiText(sub_tagline, 13);
    DrawTextShadow(sub_tagline, (w - sub_tag_w) / 2, 188, 13, Color{170, 208, 218, 235});

    // 3. Scheda di Riepilogo Salvataggio ("Ultimo luogo visitato") quando esiste un salvataggio
    if (save_summary_.exists) {
        const Rectangle save_card{315.0f, 262.0f, 650.0f, 120.0f};
        DrawPanelWithHeader(
            save_card,
            30,
            "ULTIMA PARTITA SALVATA  |  RIPRENDI DA DOVE HAI LASCIATO",
            pal::kBronzeBright);

        const std::string loc_line =
            std::string("Zona: ") + save_summary_.location_name +
            (save_summary_.timestamp.empty() ? std::string() : ("  (" + save_summary_.timestamp + ")"));
        DrawTextShadow(loc_line.c_str(), 334, 302, 17, pal::kJadeGlow);

        DrawTextShadow(
            save_summary_.chapter_title.c_str(),
            334,
            326,
            14,
            pal::kParchmentText);

        const char* stats_line = TextFormat(
            "Monumenti attivi: %d/5   |   Reliquie: %d/%d   |   Foschia: %d%%",
            save_summary_.monuments_active,
            save_summary_.relics_found,
            save_summary_.total_relics,
            static_cast<int>(std::lround(save_summary_.pollution_level)));
        DrawTextShadow(stats_line, 334, 352, 14, Color{236, 208, 138, 255});
    }

    // 4. Menu principale interattivo
    const bool has_save = save_summary_.exists;
    std::vector<const char*> labels;
    if (has_save) {
        labels = {
            "Riprendi da dove hai lasciato",
            "Nuova Partita (Ricomincia)",
            "Impostazioni (Schermo, Audio & Controller) [O]",
            "Esci dal Gioco"
        };
    } else {
        labels = {
            "Inizia il Viaggio nella Conca",
            "Impostazioni (Schermo, Audio & Controller) [O]",
            "Esci dal Gioco"
        };
    }

    const int btn_w = 430;
    const int btn_h = 44;
    const int btn_x = (w - btn_w) / 2;
    const int menu_top_y = has_save ? 404 : 426;
    const float blink = 0.65f + 0.35f * std::sin(t * 3.8f);

    for (std::size_t i = 0; i < labels.size(); ++i) {
        const bool selected = (menu_cursor_ == static_cast<int>(i));
        const Rectangle r{
            static_cast<float>(btn_x),
            static_cast<float>(menu_top_y + static_cast<int>(i) * 52),
            static_cast<float>(btn_w),
            static_cast<float>(btn_h)
        };
        const bool is_quit = (i + 1 == labels.size());
        const Color accent = is_quit
                                 ? Color{235, 110, 76, 255}
                                 : ((i == 0 && has_save) ? pal::kJadeGlow : pal::kBronzeBright);
        const Color border = selected
                                 ? Color{accent.r, accent.g, accent.b, static_cast<unsigned char>(175 + 80 * blink)}
                                 : pal::kUiBorderBronze;
        DrawSectionCard(r, border, selected);

        const int tw = MeasureUiText(labels[i], 17);
        DrawTextShadow(
            labels[i],
            btn_x + (btn_w - tw) / 2,
            static_cast<int>(r.y) + 12,
            17,
            selected ? RAYWHITE : pal::kParchmentText);
    }

    // 5. Footer controlli
    const ControllerPrompts prompts = GetActiveControllerPrompts();
    const char* controls = prompts.controller_active
        ? TextFormat("Controller %s: [Levetta/D-Pad] Scegli | %s Conferma | %s Impostazioni | [M] Muto",
                     prompts.device_name, prompts.confirm, prompts.pause)
        : "[W/S] o Mouse Scegli | [INVIO/E] Conferma | [O] Impostazioni Schermo & GPU | [M] Muto | [F11] Schermo Intero";
    const int ctrl_w = MeasureUiText(controls, 13);
    DrawTextShadow(controls, (w - ctrl_w) / 2, h - 30, 13, Color{150, 192, 210, 232});
}

} // namespace eco::app
