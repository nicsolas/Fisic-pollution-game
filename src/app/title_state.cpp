#include "app/title_state.h"

#include <raylib.h>

#include <cmath>
#include <memory>

#include "app/game_state_manager.h"
#include "app/intro_state.h"
#include "app/render/palette.h"
#include "app/render/ui_style.h"
#include "core/input_types.h"
#include "core/math_types.h"
#include "core/simulation.h"

namespace eco::app {

TitleState::TitleState(AppContext& ctx) : ctx_(ctx) {}

void TitleState::OnEnter() {
    anim_time_ = 0.0f;
}

void TitleState::Update(float dt) {
    if (std::isfinite(dt) && dt > 0.0f) {
        anim_time_ += dt;
    }

    const eco::PlayerInput& p0 = ctx_.input.Get(0);
    if (p0.confirm_pressed || p0.interact_pressed || p0.jump_pressed ||
        ctx_.input.advance_dialogue_pressed) {
        ctx_.state_manager.RequestTransition(std::make_unique<IntroState>(ctx_));
    }
}

void TitleState::Draw() {
    const int w = eco::kWindowWidth;
    const int h = eco::kWindowHeight;
    const float t = anim_time_;

    // Transizione ciclica morbida tra valle ferita (smog) e valle guarita (luce/lucciole)
    const float heal_wave = 0.45f + 0.35f * std::sin(t * 0.45f);
    const float poll_ratio = 1.0f - heal_wave;

    // 1. Cielo sfumato procedurale
    const Color sky_top = pal::LerpColor(Color{28, 22, 24, 255}, Color{18, 46, 78, 255}, heal_wave);
    const Color sky_bot = pal::WorldColor(pal::kSickSky, pal::kHealthySky, poll_ratio);
    DrawRectangleGradientV(0, 0, w, h, sky_top, sky_bot);

    // Sole / Nucleo di risonanza all'orizzonte
    const float sun_x = static_cast<float>(w) * 0.72f;
    const float sun_y = 225.0f + std::sin(t * 0.7f) * 8.0f;
    const Color sun_glow = pal::LerpColor(Color{215, 110, 55, 55}, Color{130, 245, 215, 75}, heal_wave);
    const Color sun_core = pal::LerpColor(Color{235, 145, 75, 220}, Color{215, 255, 240, 240}, heal_wave);
    DrawCircleV({sun_x, sun_y}, 95.0f + std::sin(t * 1.8f) * 6.0f, sun_glow);
    DrawCircleV({sun_x, sun_y}, 54.0f, sun_core);

    // 2. Sagoma lontana dei monti della Sila e della Catena Costiera
    const Color mountain_far = pal::LerpColor(Color{48, 40, 38, 255}, Color{36, 74, 96, 255}, heal_wave);
    for (int x = 0; x < w; x += 4) {
        const float fx = static_cast<float>(x);
        const float ridge =
            350.0f - 55.0f * std::sin(fx * 0.0045f + 0.4f) - 28.0f * std::cos(fx * 0.012f);
        DrawRectangle(x, static_cast<int>(ridge), 4, h - static_cast<int>(ridge), mountain_far);
    }

    // 3. Silhouette della Diga del Crati a nord-est
    const int dam_x = 810;
    const int dam_y = 285;
    DrawRectangle(dam_x, dam_y, 210, 140, Color{34, 46, 58, 255});
    DrawRectangleLines(dam_x, dam_y, 210, 140, Color{78, 128, 160, 200});
    for (int i = 0; i < 4; ++i) {
        const int arch_x = dam_x + 18 + i * 46;
        DrawRectangle(arch_x, dam_y + 24, 30, 110, Color{20, 32, 44, 255});
        // Cascata animata dai condotti della diga
        const float flow_alpha = 110.0f + 65.0f * std::sin(t * 4.0f + static_cast<float>(i));
        DrawRectangle(
            arch_x + 8,
            dam_y + 38,
            14,
            96,
            Color{80, 205, 240, static_cast<unsigned char>(flow_alpha)});
    }

    // 4. Colline intermedie della Valle del Crati
    const Color hill_col = pal::LerpColor(Color{62, 54, 42, 255}, Color{38, 98, 68, 255}, heal_wave);
    for (int x = 0; x < w; x += 4) {
        const float fx = static_cast<float>(x);
        const float hill_y =
            430.0f - 36.0f * std::sin(fx * 0.006f + 1.7f) - 16.0f * std::sin(fx * 0.015f);
        DrawRectangle(x, static_cast<int>(hill_y), 4, h - static_cast<int>(hill_y), hill_col);
    }

    // 5. Silhouette iconica del Ponte Bucci (viadotto reticolare del campus)
    const int bridge_y = 408;
    DrawRectangle(80, bridge_y, w - 160, 10, Color{56, 76, 94, 255});
    DrawRectangle(80, bridge_y + 26, w - 160, 6, Color{42, 58, 74, 255});
    for (int bx = 96; bx < w - 180; bx += 36) {
        DrawLine(bx, bridge_y + 10, bx + 18, bridge_y + 26, Color{95, 165, 195, 210});
        DrawLine(bx + 18, bridge_y + 26, bx + 36, bridge_y + 10, Color{95, 165, 195, 210});
    }
    // Piloni del Ponte Bucci
    for (int px = 170; px < w - 180; px += 190) {
        DrawRectangle(px, bridge_y + 32, 22, 160, Color{34, 46, 58, 255});
    }
    // Luci guida olografiche lungo il ponte
    for (int lx = 110; lx < w - 170; lx += 48) {
        const float pulse = 0.5f + 0.5f * std::sin(t * 3.2f + static_cast<float>(lx) * 0.05f);
        const Color lamp = pal::LerpColor(
            Color{230, 120, 60, 180},
            Color{100, 250, 230, static_cast<unsigned char>(150 + 95 * pulse)},
            heal_wave);
        DrawCircle(lx, bridge_y + 5, 3.0f, lamp);
    }

    // 6. Fiume Crati e primo piano scuro
    const Color river_col = pal::WorldColor(pal::kSickWater, pal::kHealthyWater, poll_ratio);
    DrawRectangle(0, 520, w, 90, river_col);
    for (int wv = 0; wv < 18; ++wv) {
        const float wy = 532.0f + static_cast<float>(wv % 5) * 15.0f;
        const float wx = std::fmod(static_cast<float>(wv * 97) + t * 38.0f, static_cast<float>(w));
        DrawLineEx({wx, wy}, {wx + 42.0f, wy}, 2.0f, Color{190, 245, 255, 120});
    }
    DrawRectangle(0, 595, w, h - 595, Color{12, 18, 26, 255});

    // 7. Particelle: cenere a sinistra che si trasforma in lucciole/polline a destra
    for (int i = 0; i < 56; ++i) {
        const float fi = static_cast<float>(i);
        const float px = std::fmod(fi * 73.0f + t * (22.0f + static_cast<float>(i % 5) * 7.0f),
                                   static_cast<float>(w));
        const float py = std::fmod(
            fi * 41.0f + std::sin(t * 1.7f + fi) * 26.0f + 540.0f - t * (10.0f + static_cast<float>(i % 4) * 4.0f),
            580.0f);
        const float norm_x = px / static_cast<float>(w);
        const float firefly_mix = eco::Clamp(norm_x * 0.65f + heal_wave * 0.55f, 0.0f, 1.0f);
        const Color pcol = pal::LerpColor(
            Color{155, 140, 125, 150},
            Color{130, 255, 195, 225},
            firefly_mix);
        const float radius = 2.0f + 1.4f * firefly_mix * (0.5f + 0.5f * std::sin(t * 4.0f + fi));
        DrawCircleV({px, py < 20.0f ? py + 520.0f : py}, radius, pcol);
    }

    // 8. Pannello centrale del Titolo
    const Rectangle title_box{220.0f, 54.0f, 840.0f, 226.0f};
    DrawPanel(title_box, Color{88, 220, 200, 230});

    const char* main_title = "EQUILIBRIO";
    const int title_size = 58;
    const int title_w = MeasureUiText(main_title, title_size);
    DrawTextShadow(main_title, (w - title_w) / 2, 74, title_size, Color{135, 255, 225, 255});

    const char* subtitle = "LE LEGGI DELLA VALLE";
    const int sub_size = 23;
    const int sub_w = MeasureUiText(subtitle, sub_size);
    DrawTextShadow(subtitle, (w - sub_w) / 2, 148, sub_size, Color{240, 210, 120, 255});

    const char* tagline =
        "Valle del Crati, 2091 // Misura la fisica. Rifiuta lo smog. Decifra il codice di GAIA.";
    const int tag_size = 15;
    const int tag_w = MeasureUiText(tagline, tag_size);
    DrawTextShadow(tagline, (w - tag_w) / 2, 188, tag_size, Color{210, 230, 240, 255});

    // Prompt lampeggiante
    const float blink = 0.55f + 0.45f * std::sin(t * 4.2f);
    const char* prompt = "Premi [INVIO] o [E] per risvegliarti nell'Osservatorio";
    const int prompt_size = 19;
    const int prompt_w = MeasureUiText(prompt, prompt_size);
    const Rectangle prompt_box{
        static_cast<float>((w - prompt_w) / 2 - 24),
        482.0f,
        static_cast<float>(prompt_w + 48),
        46.0f};
    DrawPanel(prompt_box, Color{245, 200, 95, static_cast<unsigned char>(140 + 110 * blink)});
    DrawTextShadow(
        prompt,
        (w - prompt_w) / 2,
        495,
        prompt_size,
        Color{255, 245, 200, static_cast<unsigned char>(165 + 90 * blink)});

    // Crediti e controlli sintetici in basso
    const char* footer =
        "Progetto C++20 & Raylib // Fisica Empirica & Ecologia (Unical - Valle del Crati) - v0.2.0";
    const int foot_w = MeasureUiText(footer, 14);
    DrawTextShadow(footer, (w - foot_w) / 2, h - 54, 14, Color{155, 195, 215, 230});

    const char* controls =
        "Comandi: WASD Muovi | [F] Scanner | [E] Azione | [ESC] Menu & Impostazioni | [F11] Schermo Intero";
    const int ctrl_w = MeasureUiText(controls, 13);
    DrawTextShadow(controls, (w - ctrl_w) / 2, h - 30, 13, Color{120, 165, 185, 210});
}

} // namespace eco::app
