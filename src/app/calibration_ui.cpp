#include "app/calibration_ui.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>

#include "app/render/palette.h"
#include "app/render/ui_style.h"
#include "core/collectible.h"
#include "core/ecosystem.h"
#include "core/machine.h"
#include "core/math_types.h"
#include "core/npc.h"
#include "core/player.h"
#include "core/story.h"

namespace eco::app {

namespace {

[[nodiscard]] Color EraAccentColor(eco::CollectibleEra era) noexcept {
    switch (era) {
        case eco::CollectibleEra::Era1_Dawn:
            return Color{248, 208, 104, 255};
        case eco::CollectibleEra::Era2_WatersAndSound:
            return Color{92, 228, 242, 255};
        case eco::CollectibleEra::Era3_PitchWinter:
            return Color{244, 152, 82, 255};
        case eco::CollectibleEra::Era4_HarmonySeal:
            return Color{108, 248, 206, 255};
        case eco::CollectibleEra::Count:
            break;
    }
    return pal::kJadeGlow;
}

[[nodiscard]] Color MachineStatusAccent(const eco::Machine& m, Color idle_col) noexcept {
    if (!m.unlocked) {
        return Color{118, 126, 136, 220};
    }
    if (m.state == eco::MachineState::CleanActive) {
        return pal::kAccentClean;
    }
    if (m.state == eco::MachineState::BypassedDirty) {
        return pal::kAccentDirty;
    }
    return idle_col;
}

[[nodiscard]] float ActiveMachineParam(const eco::Machine& m, const eco::WorldContext& world) noexcept {
    if (world.calibration_modal.is_open && world.modal_machine_type == m.type) {
        return world.calibration_modal.slider_value;
    }
    return m.current_param;
}

void DrawArrow2D(Vector2 start, Vector2 end, float thickness, Color color) {
    DrawLineEx(start, end, thickness, color);
    const float dx = end.x - start.x;
    const float dy = end.y - start.y;
    const float len = std::hypot(dx, dy);
    if (len < 6.0f) {
        return;
    }
    const float ux = dx / len;
    const float uy = dy / len;
    const float px = -uy;
    const float py = ux;
    const Vector2 left{end.x - ux * 8.5f + px * 4.8f, end.y - uy * 8.5f + py * 4.8f};
    const Vector2 right{end.x - ux * 8.5f - px * 4.8f, end.y - uy * 8.5f - py * 4.8f};
    DrawLineEx(end, left, thickness, color);
    DrawLineEx(end, right, thickness, color);
}

void DrawPitchFurnaceBypassVfx(float cx, float cy, float sim_time) {
    // Fornace a pece di emergenza sul fianco del monumento quando bypassato
    DrawRectangle(static_cast<int>(cx + 22.0f), static_cast<int>(cy + 4.0f), 16, 15, Color{58, 38, 30, 255});
    DrawRectangle(static_cast<int>(cx + 25.0f), static_cast<int>(cy + 11.0f), 10, 5, Color{238, 108, 42, 235});
    DrawRectangleLines(static_cast<int>(cx + 22.0f), static_cast<int>(cy + 4.0f), 16, 15, pal::kAccentDirty);
    for (int s = 0; s < 5; ++s) {
        const float age = std::fmod(sim_time * 1.45f + static_cast<float>(s) * 0.4f, 2.0f);
        const float sy = cy - 4.0f - age * 30.0f;
        const float sx = cx + 30.0f + std::sin(sim_time * 3.1f + static_cast<float>(s)) * 8.5f;
        DrawCircleV(
            {sx, sy},
            5.2f + age * 4.8f,
            Color{64, 58, 54, static_cast<unsigned char>(185 - age * 82)});
    }
}

[[nodiscard]] int ActivatedCoreMonumentsCount(const eco::WorldContext& world) noexcept {
    int count = 0;
    for (const eco::Machine& m : world.machines) {
        if (m.type != eco::MachineType::CentralAstrolabe && m.state != eco::MachineState::Broken) {
            ++count;
        }
    }
    return count;
}

void DrawWorldLabelTag(const char* text, float cx, float y, Color accent, Color text_col) {
    if (text == nullptr || text[0] == '\0') {
        return;
    }
    const int tw = MeasureUiText(text, 11);
    const int bw = tw + 14;
    const int bx = static_cast<int>(cx) - bw / 2;
    const int by = static_cast<int>(y) - 3;
    DrawRectangle(bx, by, bw, 17, Color{10, 18, 28, 198});
    DrawRectangleLines(bx, by, bw, 17, pal::WithAlpha(accent, 175));
    DrawTextShadow(text, bx + 7, static_cast<int>(y), 11, text_col);
}

void DrawMonumentPrompt(const eco::Machine& m, const eco::WorldContext& world, float y_offset) {
    if (!m.prompt_visible || world.calibration_modal.is_open) {
        return;
    }
    const char* text = m.unlocked
                           ? TextFormat("[E] Calibra %s", eco::MachineName(m.type))
                           : TextFormat("Sigillato: attiva prima i 5 monumenti (%d/5)", ActivatedCoreMonumentsCount(world));
    const Color accent = m.unlocked ? pal::kAmberGlow : Color{210, 145, 78, 240};
    const int pw = std::max(240, MeasureUiText(text, 12) + 24);
    const int px = static_cast<int>(m.pos.x) - pw / 2;
    const int py = static_cast<int>(m.pos.y + y_offset);
    DrawPanel(Rectangle{static_cast<float>(px), static_cast<float>(py), static_cast<float>(pw), 26.0f}, accent);
    DrawTextShadow(text, px + 12, py + 6, 12, accent);
}

void DrawHarmonicConduit(
    Vector2 a,
    Vector2 b,
    eco::MachineState state,
    float sim_time,
    const eco::EcoSystem& eco) {
    const float poll = eco.PollutionRatio();
    Color base_line = pal::WorldColor(Color{84, 74, 62, 160}, Color{98, 126, 134, 175}, poll);
    Color pulse_col = pal::kBronze;
    if (state == eco::MachineState::CleanActive) {
        base_line = Color{52, 178, 176, 215};
        pulse_col = pal::kJadeGlow;
    } else if (state == eco::MachineState::BypassedDirty) {
        base_line = Color{176, 92, 42, 210};
        pulse_col = pal::kAccentDirty;
    }

    const Vector2 corner{b.x, a.y};
    // Canalina in pietra e bronzo incassata nel terreno
    DrawLineEx(a, corner, 6.0f, Color{24, 28, 34, 135});
    DrawLineEx(corner, b, 6.0f, Color{24, 28, 34, 135});
    DrawLineEx(a, corner, 2.6f, base_line);
    DrawLineEx(corner, b, 2.6f, base_line);
    DrawCircleV(a, 4.2f, pal::kBronze);
    DrawCircleV(corner, 4.2f, pal::kBronze);
    DrawCircleV(b, 4.2f, pal::kBronze);

    if (state != eco::MachineState::Broken) {
        const float seg1 = std::hypot(corner.x - a.x, corner.y - a.y);
        const float seg2 = std::hypot(b.x - corner.x, b.y - corner.y);
        const float total = std::max(1.0f, seg1 + seg2);
        for (int i = 0; i < 3; ++i) {
            const float phase = std::fmod(sim_time * 0.45f + static_cast<float>(i) * 0.33f, 1.0f);
            const float dist = phase * total;
            Vector2 p = a;
            if (dist <= seg1 && seg1 > 0.0f) {
                const float t = dist / seg1;
                p = {eco::Lerp(a.x, corner.x, t), eco::Lerp(a.y, corner.y, t)};
            } else if (seg2 > 0.0f) {
                const float t = (dist - seg1) / seg2;
                p = {eco::Lerp(corner.x, b.x, t), eco::Lerp(corner.y, b.y, t)};
            }
            DrawCircleV(p, 5.2f, pal::WithAlpha(pulse_col, 85));
            DrawCircleV(p, 2.8f, pulse_col);
        }
    }
}

// =============================================================================
// 1. ELIOSTATO SOLARE (Terrazze dell'Alba — Ottica / Legge di Lambert)
// =============================================================================
void DrawSolarHeliostatApparatus(const eco::Machine& m, const eco::WorldContext& world) {
    const float cx = m.pos.x;
    const float cy = m.pos.y;
    const float poll = world.ecosystem.PollutionRatio();
    const float active_angle = ActiveMachineParam(m, world);
    const Color accent = MachineStatusAccent(m, pal::kBronzeBright);

    DrawEllipse(static_cast<int>(cx + 4.0f), static_cast<int>(cy + 16.0f), 40.0f, 18.0f, Color{0, 0, 0, 118});

    // Basamento ottagonale in travertino scolpito con ghiera in bronzo
    const Color pad_fill = pal::WorldColor(Color{74, 66, 56, 255}, Color{136, 128, 110, 255}, poll);
    DrawPoly({cx, cy}, 8, 36.0f, 22.5f, pad_fill);
    DrawPolyLinesEx({cx, cy}, 8, 36.0f, 22.5f, 2.4f, accent);
    DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), 25.0f, pal::WithAlpha(pal::kBronze, 170));

    // Raggi solari incidenti (angolo target = 45 deg)
    const float sun_rad = eco::deg2rad(m.target_param);
    const Vector2 sun_dir{std::cos(sun_rad), std::sin(sun_rad)};
    for (int r = -1; r <= 1; ++r) {
        const float perp_x = -sun_dir.y * static_cast<float>(r) * 12.0f;
        const float perp_y = sun_dir.x * static_cast<float>(r) * 12.0f;
        const float pulse = std::fmod(world.sim_time * 36.0f + static_cast<float>(r + 1) * 16.0f, 52.0f);
        const Vector2 ray_start{cx + sun_dir.x * 68.0f + perp_x, cy + sun_dir.y * 68.0f + perp_y};
        const Vector2 ray_end{cx + sun_dir.x * 22.0f + perp_x, cy + sun_dir.y * 22.0f + perp_y};
        DrawLineEx(ray_start, ray_end, 1.8f, Color{252, 222, 108, 185});
        const Vector2 photon{ray_start.x - sun_dir.x * pulse * 0.85f, ray_start.y - sun_dir.y * pulse * 0.85f};
        DrawCircleV(photon, 2.7f, Color{255, 240, 148, 240});
    }

    // Specchio eliostatico in bronzo e vetro solare orientato secondo active_angle
    const float pan_rad = eco::deg2rad(active_angle);
    const Vector2 n_dir{std::cos(pan_rad), std::sin(pan_rad)};
    const Vector2 t_dir{-n_dir.y, n_dir.x};
    const Vector2 p1{cx + t_dir.x * 28.0f, cy + t_dir.y * 28.0f};
    const Vector2 p2{cx - t_dir.x * 28.0f, cy - t_dir.y * 28.0f};

    DrawRectangle(static_cast<int>(cx - 6.0f), static_cast<int>(cy - 2.0f), 12, 14, pal::kBronzeDark);
    DrawLineEx(p1, p2, 10.0f, pal::kBronzeDark);
    DrawLineEx(p1, p2, 6.5f, Color{48, 142, 198, 255});
    DrawLineEx(p1, p2, 2.2f, Color{198, 244, 255, 255});

    DrawArrow2D({cx, cy}, {cx + n_dir.x * 28.0f, cy + n_dir.y * 28.0f}, 2.4f, accent);
    DrawCircleV({cx, cy}, 5.8f, pal::kBronzeBright);
    DrawCircleV({cx, cy}, 2.8f, RAYWHITE);

    // Meridiana / Gnomone empirico di Meridia a sinistra
    const float gx = cx - 68.0f;
    const float gy = cy + 12.0f;
    DrawEllipse(static_cast<int>(gx + 2.0f), static_cast<int>(gy + 6.0f), 18.0f, 8.0f, Color{0, 0, 0, 95});
    DrawCircle(static_cast<int>(gx), static_cast<int>(gy), 17.0f, Color{34, 40, 48, 242});
    DrawCircleLines(static_cast<int>(gx), static_cast<int>(gy), 17.0f, pal::kBronzeBright);
    for (int a = 0; a < 8; ++a) {
        const float rad = static_cast<float>(a) * (eco::kPi * 0.25f);
        DrawLine(
            static_cast<int>(gx + std::cos(rad) * 12.0f),
            static_cast<int>(gy + std::sin(rad) * 12.0f),
            static_cast<int>(gx + std::cos(rad) * 16.0f),
            static_cast<int>(gy + std::sin(rad) * 16.0f),
            pal::WithAlpha(pal::kBronzeBright, 175));
    }
    DrawLineEx({gx, gy}, {gx + sun_dir.x * 15.5f, gy + sun_dir.y * 15.5f}, 2.4f, Color{255, 228, 104, 245});
    DrawCircleV({gx, gy}, 3.4f, RAYWHITE);
    DrawTextShadow("MERIDIANA", static_cast<int>(gx) - 32, static_cast<int>(gy) + 20, 10, pal::kParchmentText);

    if (m.state == eco::MachineState::BypassedDirty) {
        DrawPitchFurnaceBypassVfx(cx, cy, world.sim_time);
    }

    DrawWorldLabelTag("ELIOSTATO SOLARE", cx, cy - 52.0f, accent, pal::kParchmentText);
    DrawMonumentPrompt(m, world, 40.0f);
}

// =============================================================================
// 2. TURBINA IDROSTATICA (Cisterne Ipogee — Idrostatica / Legge di Stevino)
// =============================================================================
void DrawHydroTurbineApparatus(const eco::Machine& m, const eco::WorldContext& world) {
    const float cx = m.pos.x;
    const float cy = m.pos.y;
    const float active_head = ActiveMachineParam(m, world);
    const float eff = m.EvaluateEfficiency(active_head);
    const Color accent = MachineStatusAccent(m, Color{96, 206, 236, 255});

    // Cisterna piezometrica graduata in bronzo e vetro (0..20 m)
    const float tank_x = cx - 138.0f;
    const float tank_bottom = cy + 20.0f;
    const float tank_h = 186.0f;
    const float tank_w = 58.0f;
    const float tank_top = tank_bottom - tank_h;

    DrawRectangle(
        static_cast<int>(tank_x - 4.0f),
        static_cast<int>(tank_top - 5.0f),
        static_cast<int>(tank_w + 8.0f),
        static_cast<int>(tank_h + 10.0f),
        Color{16, 24, 34, 242});

    const float max_h = std::max(1.0f, m.max_param);
    const float fill_ratio = eco::Clamp(active_head / max_h, 0.04f, 1.0f);
    const float water_h = tank_h * fill_ratio;
    const float water_top = tank_bottom - water_h;
    DrawRectangleGradientV(
        static_cast<int>(tank_x + 3.0f),
        static_cast<int>(water_top),
        static_cast<int>(tank_w - 6.0f),
        static_cast<int>(water_h),
        Color{72, 214, 248, 228},
        Color{24, 118, 178, 238});
    DrawLineEx({tank_x + 3.0f, water_top}, {tank_x + tank_w - 3.0f, water_top}, 2.4f, Color{190, 248, 255, 250});

    // Quota ottimale h_opt
    const float opt_ratio = eco::Clamp(m.target_param / max_h, 0.0f, 1.0f);
    const float opt_y = tank_bottom - tank_h * opt_ratio;
    DrawLineEx({tank_x - 8.0f, opt_y}, {tank_x + tank_w + 8.0f, opt_y}, 2.4f, pal::kAccentClean);
    DrawTextShadow("h_opt", static_cast<int>(tank_x - 42.0f), static_cast<int>(opt_y - 6.0f), 11, pal::kAccentClean);

    for (int step = 4; step <= 20; step += 4) {
        const float ty = tank_bottom - tank_h * (static_cast<float>(step) / max_h);
        DrawLine(static_cast<int>(tank_x), static_cast<int>(ty), static_cast<int>(tank_x + 11.0f), static_cast<int>(ty), pal::kBronzeBright);
    }
    DrawRectangleLinesEx(Rectangle{tank_x, tank_top, tank_w, tank_h}, 2.0f, pal::kBronze);

    // Condotta forzata in bronzo dalla cisterna alla chiocciola della turbina
    DrawLineEx({tank_x + tank_w, tank_bottom - 12.0f}, {cx - 28.0f, cy}, 12.0f, pal::kBronzeDark);
    DrawLineEx({tank_x + tank_w, tank_bottom - 12.0f}, {cx - 28.0f, cy}, 7.5f, Color{58, 102, 128, 255});
    for (int wp = 0; wp < 4; ++wp) {
        const float t = std::fmod(world.sim_time * (0.85f + eff * 1.8f) + static_cast<float>(wp) * 0.25f, 1.0f);
        const Vector2 drop{
            eco::Lerp(tank_x + tank_w, cx - 28.0f, t),
            eco::Lerp(tank_bottom - 12.0f, cy, t)
        };
        DrawCircleV(drop, 3.5f, Color{148, 240, 255, 245});
    }

    // Chiocciola e rotore a 8 pale in bronzo e giada
    DrawCircle(static_cast<int>(cx), static_cast<int>(cy), 34.0f, Color{20, 30, 42, 255});
    DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), 34.0f, accent);
    DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), 28.0f, pal::WithAlpha(pal::kBronzeBright, 165));

    const float spin_speed = (m.state == eco::MachineState::CleanActive) ? 6.0f : (0.7f + eff * 3.8f);
    const float base_ang = world.sim_time * spin_speed;
    for (int b = 0; b < 8; ++b) {
        const float a = base_ang + static_cast<float>(b) * (eco::kPi * 0.25f);
        const Vector2 tip{cx + std::cos(a) * 25.0f, cy + std::sin(a) * 25.0f};
        DrawLineEx({cx, cy}, tip, 3.4f, accent);
        DrawCircleV(tip, 2.5f, pal::kBronzeBright);
    }
    DrawCircleV({cx, cy}, 8.5f, pal::kBronzeDark);
    DrawCircleV({cx, cy}, 4.8f, RAYWHITE);

    if (m.state == eco::MachineState::BypassedDirty) {
        DrawPitchFurnaceBypassVfx(cx, cy, world.sim_time);
    }

    DrawWorldLabelTag("TURBINA IDROSTATICA", cx, cy - 54.0f, accent, pal::kParchmentText);
    DrawMonumentPrompt(m, world, -82.0f);
}

// =============================================================================
// 3. PENDOLO ISOCHRONOS (Torre del Pendolo — Galileo T = 2*pi*sqrt(L/g))
// =============================================================================
void DrawPendulumRegulatorApparatus(const eco::Machine& m, const eco::WorldContext& world) {
    const float cx = m.pos.x;
    const float cy = m.pos.y;
    const float active_L = ActiveMachineParam(m, world);
    const float period = std::max(0.45f, eco::ComputePendulumPeriod(active_L));
    const Color accent = MachineStatusAccent(m, pal::kBronzeBright);

    // Grande arco monumentale in pietra e bronzo con fulcro superiore
    const float pivot_y = cy - 92.0f;
    DrawRectangle(static_cast<int>(cx - 56.0f), static_cast<int>(pivot_y - 10.0f), 112, 12, pal::kBronzeDark);
    DrawRectangleLines(static_cast<int>(cx - 56.0f), static_cast<int>(pivot_y - 10.0f), 112, 12, pal::kBronzeBright);
    DrawRectangle(static_cast<int>(cx - 52.0f), static_cast<int>(pivot_y + 2.0f), 10, 124, Color{56, 60, 68, 240});
    DrawRectangle(static_cast<int>(cx + 42.0f), static_cast<int>(pivot_y + 2.0f), 10, 124, Color{56, 60, 68, 240});
    DrawRectangle(static_cast<int>(cx - 58.0f), static_cast<int>(cy + 24.0f), 116, 12, Color{48, 52, 60, 250});
    DrawRectangleLines(static_cast<int>(cx - 58.0f), static_cast<int>(cy + 24.0f), 116, 12, accent);

    // Quadrante goniometrico graduato sotto il pendolo
    const float arc_r = 96.0f;
    for (int i = -4; i <= 4; ++i) {
        const float deg = 90.0f + static_cast<float>(i) * 9.0f;
        const float rad = eco::deg2rad(deg);
        const Vector2 p0{cx + std::cos(rad) * (arc_r - 8.0f), pivot_y + std::sin(rad) * (arc_r - 8.0f)};
        const Vector2 p1{cx + std::cos(rad) * (arc_r + 4.0f), pivot_y + std::sin(rad) * (arc_r + 4.0f)};
        DrawLineEx(p0, p1, (i == 0) ? 2.5f : 1.4f, (i == 0) ? pal::kAccentClean : pal::WithAlpha(pal::kBronzeBright, 175));
    }

    // Oscillazione fisica reale secondo il periodo T = 2*pi*sqrt(L/g)
    const float omega = (2.0f * eco::kPi) / period;
    const float swing_rad = std::sin(world.sim_time * omega) * 0.44f;
    const float visual_len = eco::Lerp(48.0f, 108.0f, eco::Clamp((active_L - m.min_param) / std::max(0.1f, m.max_param - m.min_param), 0.0f, 1.0f));
    const Vector2 pivot{cx, pivot_y + 2.0f};
    const Vector2 bob{
        pivot.x + std::sin(swing_rad) * visual_len,
        pivot.y + std::cos(swing_rad) * visual_len
    };

    // Scia leggera e asta del pendolo
    DrawLineEx(pivot, bob, 3.2f, pal::kBronzeBright);
    DrawCircleV(pivot, 6.0f, pal::kBronzeBright);
    DrawCircleV(pivot, 2.8f, RAYWHITE);

    // Massa del pendolo con nucleo luminoso
    const float beat = 1.0f - std::min(1.0f, std::fabs(swing_rad) / 0.14f);
    if (beat > 0.15f) {
        DrawCircleV(bob, 20.0f + beat * 8.0f, pal::WithAlpha(accent, static_cast<unsigned char>(95.0f * beat)));
    }
    DrawCircleV(bob, 14.0f, pal::kBronzeDark);
    DrawCircleLines(static_cast<int>(bob.x), static_cast<int>(bob.y), 14.0f, accent);
    DrawCircleV(bob, 7.5f, accent);
    DrawCircleV(bob, 3.4f, RAYWHITE);

    if (m.state == eco::MachineState::BypassedDirty) {
        DrawPitchFurnaceBypassVfx(cx + 28.0f, cy + 4.0f, world.sim_time);
    }

    DrawWorldLabelTag("PENDOLO ISOCHRONOS", cx, pivot_y - 28.0f, accent, pal::kParchmentText);
    DrawMonumentPrompt(m, world, 42.0f);
}

// =============================================================================
// 4. RISONATORE ARMONICO (Giardini Sospesi — Acustica / Risonanza Lorentziana)
// =============================================================================
void DrawHarmonicResonatorApparatus(const eco::Machine& m, const eco::WorldContext& world) {
    const float cx = m.pos.x;
    const float cy = m.pos.y;
    const float poll = world.ecosystem.PollutionRatio();
    const float active_f = ActiveMachineParam(m, world);
    const float eff = m.EvaluateEfficiency(active_f);
    const Color accent = MachineStatusAccent(m, pal::kJade);

    DrawEllipse(static_cast<int>(cx + 3.0f), static_cast<int>(cy + 18.0f), 40.0f, 17.0f, Color{0, 0, 0, 120});

    // Basamento circolare in pietra chiara con intarsi di giada
    const Color base_fill = pal::WorldColor(Color{62, 58, 52, 255}, Color{94, 118, 108, 255}, poll);
    DrawCircleV({cx, cy}, 34.0f, base_fill);
    DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), 34.0f, accent);
    DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), 25.0f, pal::WithAlpha(pal::kBronzeBright, 175));

    // Onde sonore concentriche che vibrano in funzione della frequenza f
    const float wave_rate = 0.7f + (active_f / 120.0f) * 0.9f;
    for (int w = 0; w < 3; ++w) {
        const float phase = std::fmod(world.sim_time * wave_rate + static_cast<float>(w) * 0.33f, 1.0f);
        const float rad = 16.0f + phase * (38.0f + eff * 22.0f);
        const auto alpha = static_cast<unsigned char>((1.0f - phase) * (120.0f + eff * 110.0f));
        DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy - 10.0f), rad, pal::WithAlpha(accent, alpha));
    }

    // Rebbi del grande diapason monumentale in bronzo e cristallo di giada
    const float vib = std::sin(world.sim_time * (18.0f + active_f * 0.18f)) * (1.2f + eff * 2.4f);
    DrawRectangle(static_cast<int>(cx - 7.0f), static_cast<int>(cy - 6.0f), 14, 16, pal::kBronzeDark);
    DrawLineEx({cx - 12.0f - vib, cy - 32.0f}, {cx - 10.0f, cy - 4.0f}, 5.5f, pal::kBronze);
    DrawLineEx({cx + 12.0f + vib, cy - 32.0f}, {cx + 10.0f, cy - 4.0f}, 5.5f, pal::kBronze);
    DrawLineEx({cx - 12.0f - vib, cy - 32.0f}, {cx - 10.0f, cy - 4.0f}, 2.2f, accent);
    DrawLineEx({cx + 12.0f + vib, cy - 32.0f}, {cx + 10.0f, cy - 4.0f}, 2.2f, accent);

    // Cristallo armonico sospeso tra i due rebbi
    const float bob = std::sin(world.sim_time * 3.8f) * 2.4f;
    DrawPoly({cx, cy - 20.0f + bob}, 4, 10.5f, 0.0f, pal::WithAlpha(accent, 215));
    DrawPolyLinesEx({cx, cy - 20.0f + bob}, 4, 10.5f, 0.0f, 1.8f, RAYWHITE);

    if (m.state == eco::MachineState::BypassedDirty) {
        DrawPitchFurnaceBypassVfx(cx, cy, world.sim_time);
    }

    DrawWorldLabelTag("RISONATORE ARMONICO", cx, cy - 56.0f, accent, pal::kJadeGlow);
    DrawMonumentPrompt(m, world, 40.0f);
}

// =============================================================================
// 5. SCAMBIATORE DI CARNOT (Camera Geotermica — Termodinamica)
// =============================================================================
void DrawThermalExchangerApparatus(const eco::Machine& m, const eco::WorldContext& world) {
    const float cx = m.pos.x;
    const float cy = m.pos.y;
    const float active_th = ActiveMachineParam(m, world);
    const Color accent = MachineStatusAccent(m, pal::kAmberGlow);

    DrawEllipse(static_cast<int>(cx), static_cast<int>(cy + 26.0f), 42.0f, 13.0f, Color{0, 0, 0, 130});

    // Pozzo in basalto esagonale con serpentine in rame
    DrawPoly({cx, cy}, 6, 36.0f, 30.0f, Color{30, 34, 42, 255});
    DrawPolyLinesEx({cx, cy}, 6, 36.0f, 30.0f, 2.5f, Color{198, 128, 68, 255});
    DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), 27.0f, accent);

    // Condotti laterali in rame (sorgente fredda Tc a sx, scambio a dx)
    DrawRectangle(static_cast<int>(cx - 46.0f), static_cast<int>(cy - 6.0f), 14, 12, Color{72, 144, 168, 255});
    DrawRectangleLines(static_cast<int>(cx - 46.0f), static_cast<int>(cy - 6.0f), 14, 12, pal::kBronzeBright);
    DrawRectangle(static_cast<int>(cx + 32.0f), static_cast<int>(cy - 6.0f), 14, 12, Color{196, 114, 58, 255});
    DrawRectangleLines(static_cast<int>(cx + 32.0f), static_cast<int>(cy - 6.0f), 14, 12, pal::kBronzeBright);

    // Nucleo termico proporzionale a Th
    const float heat_t = eco::Clamp((active_th - m.min_param) / std::max(1.0f, m.max_param - m.min_param), 0.0f, 1.0f);
    const float pulse = 0.5f + 0.5f * std::sin(world.sim_time * 4.0f);
    const Color core_col = (m.state == eco::MachineState::CleanActive)
                               ? pal::LerpColor(pal::kJade, pal::kBronzeBright, 0.35f)
                               : pal::LerpColor(Color{96, 168, 224, 255}, Color{248, 112, 48, 255}, heat_t);
    DrawCircleV({cx, cy}, 17.0f + pulse * 2.5f, pal::WithAlpha(core_col, 95));
    DrawCircleV({cx, cy}, 10.5f, core_col);

    // Ago del termometro termodinamico + tacca target 600 K
    const float target_t = eco::Clamp((m.target_param - m.min_param) / std::max(1.0f, m.max_param - m.min_param), 0.0f, 1.0f);
    const float target_rad = eco::Lerp(-2.4f, 0.8f, target_t);
    const float curr_rad = eco::Lerp(-2.4f, 0.8f, heat_t);
    DrawLineEx(
        {cx + std::cos(target_rad) * 18.0f, cy + std::sin(target_rad) * 18.0f},
        {cx + std::cos(target_rad) * 32.0f, cy + std::sin(target_rad) * 32.0f},
        2.5f,
        pal::kAccentClean);
    DrawArrow2D({cx, cy}, {cx + std::cos(curr_rad) * 25.0f, cy + std::sin(curr_rad) * 25.0f}, 2.8f, RAYWHITE);

    if (m.state == eco::MachineState::CleanActive) {
        for (int v = 0; v < 3; ++v) {
            const float age = std::fmod(world.sim_time * 1.4f + static_cast<float>(v) * 0.6f, 1.8f);
            DrawCircleV(
                {cx - 10.0f + static_cast<float>(v) * 10.0f, cy - 18.0f - age * 20.0f},
                3.5f + age * 2.4f,
                Color{184, 245, 252, static_cast<unsigned char>(135 - age * 65)});
        }
    } else if (m.state == eco::MachineState::BypassedDirty) {
        DrawPitchFurnaceBypassVfx(cx, cy, world.sim_time);
    }

    DrawWorldLabelTag("SCAMBIATORE DI CARNOT", cx, cy - 54.0f, accent, pal::kParchmentText);
    DrawMonumentPrompt(m, world, -82.0f);
}

// =============================================================================
// 6. ASTROLABIO CENTRALE (Santuario dell'Armonia — Sintesi & 4 Anelli d'Era)
// =============================================================================
void DrawCentralAstrolabeApparatus(const eco::Machine& m, const eco::WorldContext& world) {
    const float cx = m.pos.x;
    const float cy = m.pos.y;
    const float active_phi = ActiveMachineParam(m, world);
    const Color accent = MachineStatusAccent(m, pal::kBronzeBright);

    DrawEllipse(static_cast<int>(cx), static_cast<int>(cy + 26.0f), 54.0f, 19.0f, Color{0, 0, 0, 140});

    // Piedistallo monumentale a gradini in marmo avorio e bronzo
    DrawPoly({cx, cy + 4.0f}, 8, 48.0f, 22.5f, Color{36, 42, 52, 248});
    DrawPolyLinesEx({cx, cy + 4.0f}, 8, 48.0f, 22.5f, 2.2f, pal::kBronze);
    DrawPoly({cx, cy + 2.0f}, 8, 36.0f, 22.5f, Color{24, 34, 46, 250});
    DrawPolyLinesEx({cx, cy + 2.0f}, 8, 36.0f, 22.5f, 2.0f, accent);

    // 4 Anelli concentrici delle 4 Ere: ciascuno si illumina con le reliquie raccolte di quell'era!
    for (int e = 0; e < static_cast<int>(eco::CollectibleEra::Count); ++e) {
        const auto era = static_cast<eco::CollectibleEra>(e);
        const int found = world.collectibles.CollectedInEra(era);
        const float ratio = static_cast<float>(found) / static_cast<float>(eco::kCollectiblesPerEra);
        const float ring_r = 22.0f + static_cast<float>(e) * 7.2f;
        const Color era_col = EraAccentColor(era);
        const unsigned char alpha = static_cast<unsigned char>(55.0f + ratio * 185.0f);
        DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy - 10.0f), ring_r, pal::WithAlpha(era_col, alpha));

        // Nodo orbitante per ciascuna era con almeno 1 reliquia trovata
        if (found > 0) {
            const float ang = world.sim_time * (0.65f + static_cast<float>(e) * 0.25f) * ((e % 2 == 0) ? 1.0f : -1.0f);
            const Vector2 node{cx + std::cos(ang) * ring_r, (cy - 10.0f) + std::sin(ang) * ring_r};
            DrawCircleV(node, 3.2f + ratio * 1.4f, era_col);
        }
    }

    // Sfera armillare centrale e braccio di fase
    const float speed = (m.state == eco::MachineState::CleanActive) ? 1.4f : (m.unlocked ? 0.65f : 0.15f);
    const float rot_a = eco::deg2rad(active_phi) + world.sim_time * speed;
    const Vector2 center{cx, cy - 10.0f};
    DrawLineEx(
        {center.x - std::cos(rot_a) * 32.0f, center.y - std::sin(rot_a) * 32.0f},
        {center.x + std::cos(rot_a) * 32.0f, center.y + std::sin(rot_a) * 32.0f},
        2.6f,
        pal::kBronzeBright);
    DrawLineEx(
        {center.x - std::sin(rot_a) * 24.0f, center.y + std::cos(rot_a) * 24.0f},
        {center.x + std::sin(rot_a) * 24.0f, center.y - std::cos(rot_a) * 24.0f},
        2.0f,
        accent);

    const float pulse = 0.5f + 0.5f * std::sin(world.sim_time * 3.6f);
    const Color core = m.state == eco::MachineState::CleanActive
                           ? pal::kJadeGlow
                           : (m.unlocked ? pal::kBronzeBright : Color{98, 114, 128, 220});
    DrawCircleV(center, 14.0f + pulse * 3.0f, pal::WithAlpha(core, 75));
    DrawPoly(center, 4, 9.0f, world.sim_time * 30.0f, core);
    DrawCircleV(center, 3.8f, RAYWHITE);

    DrawWorldLabelTag("ASTROLABIO CENTRALE", cx, cy - 66.0f, accent, pal::kParchmentText);
    DrawMonumentPrompt(m, world, 48.0f);
}

void DrawMonument(const eco::Machine& m, const eco::WorldContext& world) {
    switch (m.type) {
        case eco::MachineType::SolarArray:
            DrawSolarHeliostatApparatus(m, world);
            break;
        case eco::MachineType::HydroTurbine:
            DrawHydroTurbineApparatus(m, world);
            break;
        case eco::MachineType::PendulumRegulator:
            DrawPendulumRegulatorApparatus(m, world);
            break;
        case eco::MachineType::HarmonicResonator:
            DrawHarmonicResonatorApparatus(m, world);
            break;
        case eco::MachineType::ThermalExchanger:
            DrawThermalExchangerApparatus(m, world);
            break;
        case eco::MachineType::CentralAstrolabe:
            DrawCentralAstrolabeApparatus(m, world);
            break;
        case eco::MachineType::Count:
            break;
    }
}

// =============================================================================
// 4 NPC DISTINTI DELLA CONCA DI MERIDIA
// =============================================================================
void DrawWorldNpcCharacter(const eco::WorldNpc& npc, const eco::WorldContext& world) {
    const float nx = npc.pos.x;
    const float ny = npc.pos.y;
    const float bob = std::sin(world.sim_time * 2.6f + static_cast<float>(npc.id) * 1.4f) * 1.15f;

    Color accent = pal::kAmberGlow;
    Color coat_col{42, 60, 82, 255};
    Color hair_col{74, 52, 42, 255};
    switch (npc.id) {
        case eco::NpcId::ClaraSereni:
            accent = npc.recruited ? pal::kAccentClean : pal::kAmberGlow;
            coat_col = Color{36, 56, 78, 255}; // Blu ardesia da ingegnera
            hair_col = Color{66, 44, 34, 255};
            break;
        case eco::NpcId::LorenzoAltieri:
            accent = npc.recruited ? pal::kJadeGlow : Color{216, 174, 102, 255};
            coat_col = Color{102, 46, 48, 255}; // Mantello bordeaux/ocra da archivista
            hair_col = Color{218, 224, 232, 255}; // Capelli d'argento
            break;
        case eco::NpcId::MiriamFerro:
            accent = pal::kJade;
            coat_col = Color{48, 92, 68, 255}; // Tunica verde salvia da botanica
            hair_col = Color{92, 54, 36, 255};
            break;
        case eco::NpcId::MatteoVerrini:
            accent = Color{236, 134, 68, 255};
            coat_col = Color{44, 46, 52, 255}; // Soprabito antracite severo
            hair_col = Color{54, 52, 56, 255};
            break;
        case eco::NpcId::Count:
            break;
    }

    // Ombra a terra
    DrawEllipse(static_cast<int>(nx), static_cast<int>(ny + 16.0f), 13.5f, 5.0f, Color{0, 0, 0, 128});

    // Stivali
    DrawRectangle(static_cast<int>(nx - 6.0f), static_cast<int>(ny + 9.0f), 4, 7, Color{38, 34, 32, 255});
    DrawRectangle(static_cast<int>(nx + 2.0f), static_cast<int>(ny + 9.0f), 4, 7, Color{38, 34, 32, 255});

    // Accessorio dorsale / laterale distintivo per ogni personaggio
    if (npc.id == eco::NpcId::ClaraSereni) {
        // Tubo portaprogetti in cuoio e ottone a tracolla
        DrawLineEx({nx - 10.0f, ny - 5.0f + bob}, {nx + 9.0f, ny + 8.0f + bob}, 4.5f, Color{172, 118, 56, 255});
        DrawCircleV({nx - 10.0f, ny - 5.0f + bob}, 3.0f, pal::kStoneIvory);
    } else if (npc.id == eco::NpcId::MiriamFerro) {
        // Bisaccia botanica e fiala luminosa di giada
        DrawRectangle(static_cast<int>(nx - 12.0f), static_cast<int>(ny - 1.0f + bob), 5, 10, Color{138, 102, 64, 255});
    }

    // Corpo / soprabito / mantello
    DrawRectangle(static_cast<int>(nx - 8.0f), static_cast<int>(ny - 5.0f + bob), 16, 15, coat_col);
    DrawRectangle(static_cast<int>(nx - 8.0f), static_cast<int>(ny - 5.0f + bob), 16, 3, pal::LerpColor(accent, pal::kBronzeBright, 0.35f));
    DrawRectangleLines(static_cast<int>(nx - 8.0f), static_cast<int>(ny - 5.0f + bob), 16, 15, accent);

    // Oggetto in mano caratteristico
    if (npc.id == eco::NpcId::LorenzoAltieri) {
        // Lanterna-astrolabio in bronzo luminosa
        const Vector2 lamp{nx + 11.0f, ny + 1.0f + bob};
        DrawCircleV(lamp, 8.5f, pal::WithAlpha(pal::kBronzeBright, 65));
        DrawCircleLines(static_cast<int>(lamp.x), static_cast<int>(lamp.y), 4.5f, pal::kBronzeBright);
        DrawCircleV(lamp, 2.2f, Color{255, 240, 172, 255});
    } else if (npc.id == eco::NpcId::MiriamFerro) {
        // Fiala di vetro-giada luminescente
        const Vector2 vial{nx + 10.0f, ny + 1.0f + bob};
        DrawCircleV(vial, 7.0f, pal::WithAlpha(pal::kJadeGlow, 75));
        DrawRectangle(static_cast<int>(vial.x - 2.0f), static_cast<int>(vial.y - 4.0f), 4, 8, pal::kJadeGlow);
    } else if (npc.id == eco::NpcId::MatteoVerrini) {
        // Bottoni d'ottone ambrato sul soprabito scuro
        DrawCircle(static_cast<int>(nx - 2.0f), static_cast<int>(ny + 1.0f + bob), 1.4f, pal::kAmberGlow);
        DrawCircle(static_cast<int>(nx - 2.0f), static_cast<int>(ny + 5.0f + bob), 1.4f, pal::kAmberGlow);
    }

    // Testa, capelli e tratto facciale
    const float head_y = ny - 11.0f + bob;
    DrawCircle(static_cast<int>(nx), static_cast<int>(head_y), 7.2f, Color{234, 202, 178, 255});
    DrawCircleSector({nx, head_y - 0.5f}, 8.0f, 175.0f, 365.0f, 12, hair_col);

    if (npc.id == eco::NpcId::ClaraSereni) {
        // Occhiali da rilievo topografico
        DrawRectangle(static_cast<int>(nx - 5.0f), static_cast<int>(head_y - 1.5f), 10, 3, Color{72, 218, 242, 235});
    } else if (npc.id == eco::NpcId::LorenzoAltieri) {
        // Barba argentata e occhiali d'ottone
        DrawCircleSector({nx, head_y + 2.5f}, 5.8f, 20.0f, 160.0f, 8, hair_col);
        DrawCircleLines(static_cast<int>(nx - 2.4f), static_cast<int>(head_y - 0.5f), 2.2f, pal::kBronzeBright);
        DrawCircleLines(static_cast<int>(nx + 2.4f), static_cast<int>(head_y - 0.5f), 2.2f, pal::kBronzeBright);
    } else if (npc.id == eco::NpcId::MiriamFerro) {
        // Fermaglio foglia/fiore di giada
        DrawPoly({nx + 5.5f, head_y - 5.0f}, 4, 3.2f, 20.0f, pal::kJadeGlow);
    } else if (npc.id == eco::NpcId::MatteoVerrini) {
        // Monocolo fumee ambrato
        DrawCircle(static_cast<int>(nx + 2.6f), static_cast<int>(head_y - 0.8f), 2.5f, Color{238, 152, 68, 215});
        DrawCircleLines(static_cast<int>(nx + 2.6f), static_cast<int>(head_y - 0.8f), 2.5f, pal::kBronzeBright);
    }

    // Indicatore flottante sopra il personaggio
    const float h_bob = std::sin(world.sim_time * 3.6f + static_cast<float>(npc.id)) * 2.8f;
    const Vector2 badge_c{nx, ny - 31.0f + h_bob};
    if (!npc.met && !npc.recruited) {
        DrawCircleV(badge_c, 9.0f, Color{18, 28, 40, 238});
        DrawCircleLines(static_cast<int>(badge_c.x), static_cast<int>(badge_c.y), 9.0f, accent);
        DrawTextShadow("!", static_cast<int>(badge_c.x - 2.0f), static_cast<int>(badge_c.y - 6.0f), 14, accent);
    } else if (npc.recruited) {
        DrawPolyLines(badge_c, 6, 8.2f, world.sim_time * 60.0f, accent);
    } else {
        DrawPolyLines(badge_c, 4, 6.2f, 0.0f, pal::WithAlpha(accent, 195));
    }

    const char* title_lbl = TextFormat("%s", npc.name.empty() ? eco::NpcDisplayName(npc.id) : npc.name.c_str());
    DrawWorldLabelTag(title_lbl, nx, ny - 52.0f, accent, accent);

    if (npc.prompt_visible && !world.calibration_modal.is_open) {
        const char* prompt = TextFormat("[E] Parla con %s (%s)", title_lbl, eco::NpcRoleTitle(npc.id));
        const int pw = std::max(250, MeasureUiText(prompt, 12) + 24);
        const int px = static_cast<int>(nx) - pw / 2;
        const int py = (world.current_mode == eco::GameMode::Structure) ? static_cast<int>(ny) - 78 : static_cast<int>(ny) + 24;
        DrawPanel(Rectangle{static_cast<float>(px), static_cast<float>(py), static_cast<float>(pw), 26.0f}, accent);
        DrawTextShadow(prompt, px + 12, py + 6, 12, accent);
    }
}

// =============================================================================
// RELIQUIE COLLEZIONABILI NEL MONDO DI GIOCO (Overworld & Strutture)
// =============================================================================
void DrawCollectiblesInScene(const eco::WorldContext& world) {
    const eco::LocationId loc = world.ActiveLocation();
    for (const eco::CollectibleItem& item : world.collectibles.Items()) {
        if (item.collected || item.location != loc) {
            continue;
        }
        const Color era_col = EraAccentColor(item.era);
        const float phase = static_cast<float>(item.id) * 0.9f;
        const float bob = std::sin(world.sim_time * 3.1f + phase) * 4.0f;
        const float pulse = 0.5f + 0.5f * std::sin(world.sim_time * 3.8f + phase);
        const Vector2 center{item.pos.x, item.pos.y - 4.0f + bob};

        // Ombra morbida e fascio luminoso verticale sottile
        DrawEllipse(static_cast<int>(item.pos.x), static_cast<int>(item.pos.y + 13.0f), 14.0f, 5.2f, Color{0, 0, 0, 115});
        DrawRectangleGradientV(
            static_cast<int>(item.pos.x - 10.0f),
            static_cast<int>(item.pos.y - 42.0f),
            20,
            52,
            pal::WithAlpha(era_col, 0),
            pal::WithAlpha(era_col, static_cast<unsigned char>(32 + 24 * pulse)));

        // Aureola pulsante a due anelli
        DrawCircleV(center, 22.0f + pulse * 4.0f, pal::WithAlpha(era_col, static_cast<unsigned char>(30 + 24 * pulse)));
        DrawCircleLines(
            static_cast<int>(center.x),
            static_cast<int>(center.y),
            18.5f + pulse * 3.0f,
            pal::WithAlpha(era_col, static_cast<unsigned char>(135 + 80 * pulse)));

        // Glifo procedurale della reliquia (Stele, Astrolabio, Prisma, Cilindro, Seme Fossile)
        DrawRelicGlyph(static_cast<int>(item.kind), center, 13.5f, era_col, true, world.sim_time + phase);

        // 3 scintille orbitali dedicate
        for (int s = 0; s < 3; ++s) {
            const float a = world.sim_time * 2.1f + phase + static_cast<float>(s) * (2.0f * eco::kPi / 3.0f);
            const Vector2 orb{
                center.x + std::cos(a) * 19.0f,
                center.y + std::sin(a) * 11.0f
            };
            DrawCircleV(orb, 2.1f, pal::WithAlpha(RAYWHITE, 220));
        }

        // Prompt diegetico quando il giocatore si avvicina
        if (item.prompt_visible && !world.calibration_modal.is_open) {
            const eco::CollectibleMeta& meta = eco::CollectibleMetadata(item.id);
            const char* prompt = TextFormat("[E] Raccogli %s - %s", meta.code.c_str(), meta.title.c_str());
            const int pw = std::max(210, MeasureUiText(prompt, 12) + 22);
            const int px = static_cast<int>(item.pos.x) - pw / 2;
            const int py = static_cast<int>(item.pos.y) - 48;
            DrawPanel(Rectangle{static_cast<float>(px), static_cast<float>(py), static_cast<float>(pw), 25.0f}, era_col);
            DrawTextShadow(prompt, px + 11, py + 5, 12, era_col);
        }
    }
}

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

    // 1. I 6 distretti colorati secondo la palette della Conca
    for (int z = 0; z < static_cast<int>(eco::ZoneId::Count); ++z) {
        const auto zone = static_cast<eco::ZoneId>(z);
        const eco::Rect zb = world.overworld.DistrictBounds(zone);
        const pal::DistrictPalette& dp = pal::DistrictPaletteByIndex(z);
        const Color col = pal::WorldColor(dp.ground_sick, dp.ground_healthy, poll);
        const Rectangle zr{
            map_r.x + (zb.x / world_w) * map_r.width,
            map_r.y + (zb.y / world_h) * map_r.height,
            (zb.width / world_w) * map_r.width,
            (zb.height / world_h) * map_r.height
        };
        DrawRectangleRec(zr, pal::WithAlpha(col, (zone == world.current_zone) ? 205 : 135));
        DrawRectangleLinesEx(zr, 0.8f, pal::WithAlpha(pal::kBronze, 95));
    }

    // 2. Canale centrale (col 24) e stato del Ponte degli Archi
    const float bridge_x = map_r.x + (static_cast<float>(eco::Tilemap::kBridgeDividerCol) + 0.5f) /
                                         static_cast<float>(std::max(1, tm.Cols())) * map_r.width;
    const Color water_c = pal::WorldColor(pal::kSickWater, pal::kHealthyWater, poll);
    DrawLineEx({bridge_x, map_r.y}, {bridge_x, map_r.y + map_r.height}, 3.6f, water_c);
    const float bridge_y = map_r.y + (17.5f / static_cast<float>(std::max(1, tm.Rows()))) * map_r.height;
    DrawRectangle(
        static_cast<int>(bridge_x - 4.0f),
        static_cast<int>(bridge_y - 3.0f),
        8,
        6,
        world.ecosystem.architect_recruited ? pal::kJadeGlow : pal::kAccentDirty);

    // 3. Portali delle 3 strutture interne (losanghe azzurre/dorate)
    for (const eco::PortalLink& portal : world.overworld.portals) {
        const Vector2 mp = to_mini(portal.world_center);
        DrawPoly(mp, 4, 4.2f, 0.0f, Color{96, 212, 245, 245});
        DrawPolyLines(mp, 4, 4.5f, 0.0f, RAYWHITE);
    }

    // 4. Reliquie collezionabili dell'Overworld (anello tenue se da trovare, punto d'oro se trovata)
    for (const eco::CollectibleItem& item : world.collectibles.Items()) {
        if (item.location != eco::LocationId::Overworld) {
            continue;
        }
        const Vector2 mp = to_mini(item.pos);
        if (item.collected) {
            DrawCircleV(mp, 2.3f, pal::kBronzeBright);
        } else {
            DrawCircleLines(static_cast<int>(mp.x), static_cast<int>(mp.y), 2.5f, pal::WithAlpha(EraAccentColor(item.era), 205));
        }
    }

    // 5. Monumenti fisici dell'Overworld
    for (const eco::Machine& m : world.machines) {
        if (m.location != eco::LocationId::Overworld) {
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

    // 5b. Varchi monumentali tra distretti (archi in bronzo) e contorno del distretto attivo
    for (const eco::DistrictGateway& gw : world.overworld.gateways) {
        const Vector2 gp = to_mini(gw.world_center);
        const bool locked = gw.requires_bridge_unlock && !world.ecosystem.architect_recruited;
        const Color gc = locked ? pal::kAccentWarn : pal::kBronzeBright;
        if (gw.IsHorizontalWall()) {
            DrawRectangle(static_cast<int>(gp.x) - 5, static_cast<int>(gp.y) - 2, 10, 4, gc);
            DrawRectangle(static_cast<int>(gp.x) - 5, static_cast<int>(gp.y) - 4, 2, 8, pal::kBronzeDark);
            DrawRectangle(static_cast<int>(gp.x) + 3, static_cast<int>(gp.y) - 4, 2, 8, pal::kBronzeDark);
        } else {
            DrawRectangle(static_cast<int>(gp.x) - 2, static_cast<int>(gp.y) - 5, 4, 10, gc);
        }
    }
    {
        const eco::Rect zb = world.overworld.DistrictBounds(world.current_zone);
        const Rectangle zr{
            map_r.x + (zb.x / world_w) * map_r.width,
            map_r.y + (zb.y / world_h) * map_r.height,
            (zb.width / world_w) * map_r.width,
            (zb.height / world_h) * map_r.height};
        DrawRectangleLinesEx(zr, 1.6f, pal::WithAlpha(pal::kJadeGlow, 235));
    }

    // 5c. Faro pulsante sull'obiettivo corrente
    const eco::ObjectiveTarget objective = world.CurrentObjectiveTarget();
    if (objective.valid && objective.location == eco::LocationId::Overworld) {
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
// BUSSOLA DELL'OBIETTIVO (bronzo e giada): direzione, nome meta e distanza in metri
// 1 metro di gioco = meta' tile (kTileSize / 2 px). Nessuna allocazione dinamica.
// =============================================================================
void DrawObjectiveCompass(const eco::WorldContext& world) {
    if (world.calibration_modal.is_open) {
        return;
    }
    const eco::ObjectiveTarget obj = world.CurrentObjectiveTarget();
    const eco::Player* p0 = world.players.GetPlayer(0);
    if (!obj.valid || p0 == nullptr || !p0->active) {
        return;
    }
    // Il target e' nella scena attiva? Altrimenti la meta e' gia' il portale/uscita fornito dal core.
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

    const float sw = static_cast<float>(eco::kWindowWidth);
    const float sh = static_cast<float>(eco::kWindowHeight);
    const eco::Vec2 cam = world.ActiveCameraTarget();
    const float sx = obj.world_pos.x - cam.x + sw * 0.5f;
    const float sy = obj.world_pos.y - cam.y + sh * 0.5f;
    const bool on_screen = sx > 24.0f && sx < sw - 24.0f && sy > 76.0f && sy < sh - 24.0f;

    // Etichetta testuale in basso al centro
    const char* text = TextFormat("%s (%d m)", obj.label, meters);
    const char* full = TextFormat("-> %s", text);
    const int tw = MeasureUiText(full, 13);
    const int bw = tw + 58;
    const int bx = static_cast<int>(sw * 0.5f) - bw / 2;
    const int by = static_cast<int>(sh) - 158;
    DrawRectangle(bx + 2, by + 2, bw, 30, Color{0, 0, 0, 110});
    DrawRectangle(bx, by, bw, 30, Color{14, 22, 32, 226});
    DrawRectangleLines(bx, by, bw, 30, pal::WithAlpha(pal::kBronze, 235));
    DrawRectangleLines(bx + 2, by + 2, bw - 4, 26, pal::WithAlpha(pal::kJade, static_cast<unsigned char>(110 + 90 * pulse)));

    // Disco bussola con ago che ruota verso la meta
    const Vector2 cc{static_cast<float>(bx) + 20.0f, static_cast<float>(by) + 15.0f};
    DrawCircleV(cc, 12.0f, Color{18, 30, 40, 255});
    DrawCircleLines(static_cast<int>(cc.x), static_cast<int>(cc.y), 12.0f, pal::kBronzeBright);
    const Vector2 tip{cc.x + std::cos(ang) * 10.0f, cc.y + std::sin(ang) * 10.0f};
    const Vector2 l{cc.x + std::cos(ang + 2.5f) * 6.0f, cc.y + std::sin(ang + 2.5f) * 6.0f};
    const Vector2 r{cc.x + std::cos(ang - 2.5f) * 6.0f, cc.y + std::sin(ang - 2.5f) * 6.0f};
    DrawTriangle(tip, l, r, pal::kJadeGlow);
    DrawCircleV(cc, 2.0f, pal::kBronzeBright);
    DrawTextShadow(full, bx + 40, by + 8, 13, obj.is_portal_step ? pal::kBronzeBright : pal::kJadeGlow);

    // Freccia sul bordo dello schermo quando la meta e' fuori vista
    if (!on_screen && dist_px > 1.0f) {
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
        DrawCircleV(e, 18.0f + pulse * 3.0f, pal::WithAlpha(pal::kJade, 48));
        DrawTriangle(a, c2, b, pal::kBronzeBright);
        DrawTriangleLines(a, c2, b, pal::kBronzeDark);
        DrawCircleV(e, 5.0f, pal::kJadeGlow);
        DrawTextShadow(TextFormat("%d m", meters), static_cast<int>(e.x) - 14, static_cast<int>(e.y) + 20, 11, pal::kParchmentText);
    } else if (on_screen) {
        // Anello pulsante sopra la meta quando e' inquadrata
        DrawCircleLines(static_cast<int>(sx), static_cast<int>(sy) - 60, 14.0f + pulse * 4.0f, pal::WithAlpha(pal::kJadeGlow, 200));
        DrawTriangle({sx, sy - 40.0f}, {sx + 8.0f, sy - 54.0f}, {sx - 8.0f, sy - 54.0f}, pal::kBronzeBright);
    }
}

// =============================================================================
// 6 DIAGRAMMI FISICI INTERATTIVI NEL MODAL DI CALIBRAZIONE
// =============================================================================
void DrawCalibrationDiagram(
    const eco::Machine& machine,
    const eco::WorldContext& world,
    Rectangle diag_r) {
    DrawRectangleRec(diag_r, Color{10, 18, 28, 245});
    DrawRectangleLinesEx(diag_r, 1.4f, pal::WithAlpha(pal::kBronze, 200));

    const float val = world.calibration_modal.slider_value;
    const bool ok = world.calibration_modal.can_confirm_clean;
    const Color live_col = ok ? pal::kAccentClean : Color{86, 202, 240, 255};
    const Vector2 c{diag_r.x + diag_r.width * 0.5f, diag_r.y + diag_r.height * 0.44f};

    switch (machine.type) {
        case eco::MachineType::SolarArray: {
            // Diagramma Lambert: vettore solare (oro) vs normale specchio (ciano/giada)
            DrawCircleV(c, 58.0f, Color{16, 26, 38, 255});
            DrawCircleLines(static_cast<int>(c.x), static_cast<int>(c.y), 58.0f, pal::WithAlpha(pal::kBronzeBright, 175));
            const float s_rad = eco::deg2rad(machine.target_param);
            const float p_rad = eco::deg2rad(val);
            DrawArrow2D(c, {c.x + std::cos(s_rad) * 48.0f, c.y + std::sin(s_rad) * 48.0f}, 2.4f, pal::kAmberGlow);
            DrawArrow2D(c, {c.x + std::cos(p_rad) * 44.0f, c.y + std::sin(p_rad) * 44.0f}, 3.0f, live_col);
            DrawTextShadow("Oro: Raggio Solare", static_cast<int>(diag_r.x) + 14, static_cast<int>(diag_r.y + diag_r.height) - 38, 12, pal::kAmberGlow);
            DrawTextShadow("Ciano: Normale Specchio", static_cast<int>(diag_r.x) + 14, static_cast<int>(diag_r.y + diag_r.height) - 20, 12, live_col);
            break;
        }
        case eco::MachineType::HydroTurbine: {
            // Diagramma Stevino: colonna idrostatica + P = rho*g*h
            const Rectangle col_r{diag_r.x + 26.0f, diag_r.y + 18.0f, 44.0f, diag_r.height - 46.0f};
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
            const float opt_y = col_r.y + col_r.height * (1.0f - eco::Clamp(machine.target_param / std::max(1.0f, machine.max_param), 0.0f, 1.0f));
            DrawLineEx({col_r.x - 6.0f, opt_y}, {col_r.x + col_r.width + 6.0f, opt_y}, 2.2f, pal::kAccentClean);
            DrawRectangleLinesEx(col_r, 1.6f, pal::kBronzeBright);

            const float p_kpa = (eco::kWaterDensity * eco::kGravityWater * val) / 1000.0f;
            const float opt_kpa = (eco::kWaterDensity * eco::kGravityWater * machine.target_param) / 1000.0f;
            DrawTextShadow("PRESSIONE", static_cast<int>(diag_r.x) + 84, static_cast<int>(diag_r.y) + 34, 11, pal::kBronzeBright);
            DrawTextShadow(TextFormat("%.1f kPa", p_kpa), static_cast<int>(diag_r.x) + 84, static_cast<int>(diag_r.y) + 52, 16, live_col);
            DrawTextShadow("OTTIMALE", static_cast<int>(diag_r.x) + 84, static_cast<int>(diag_r.y) + 88, 11, pal::kAccentClean);
            DrawTextShadow(TextFormat("%.1f kPa", opt_kpa), static_cast<int>(diag_r.x) + 84, static_cast<int>(diag_r.y) + 106, 15, pal::kAccentClean);
            break;
        }
        case eco::MachineType::PendulumRegulator: {
            // Diagramma Galileo: pendolo oscillante live + periodo T vs T0
            const Vector2 pivot{diag_r.x + diag_r.width * 0.5f, diag_r.y + 20.0f};
            const float t_live = std::max(0.45f, eco::ComputePendulumPeriod(val));
            const float t_opt = std::max(0.45f, eco::ComputePendulumPeriod(machine.target_param));
            const float norm_l = eco::Clamp((val - machine.min_param) / std::max(0.1f, machine.max_param - machine.min_param), 0.0f, 1.0f);
            const float len_px = eco::Lerp(38.0f, 94.0f, norm_l);

            // Riferimento target (traccia tenue dorata)
            const float opt_ang = std::sin(world.sim_time * (2.0f * eco::kPi / t_opt)) * 0.48f;
            const float opt_len = eco::Lerp(38.0f, 94.0f, eco::Clamp((machine.target_param - machine.min_param) / std::max(0.1f, machine.max_param - machine.min_param), 0.0f, 1.0f));
            const Vector2 opt_bob{pivot.x + std::sin(opt_ang) * opt_len, pivot.y + std::cos(opt_ang) * opt_len};
            DrawLineEx(pivot, opt_bob, 1.6f, pal::WithAlpha(pal::kAmberGlow, 120));
            DrawCircleLines(static_cast<int>(opt_bob.x), static_cast<int>(opt_bob.y), 7.5f, pal::WithAlpha(pal::kAmberGlow, 170));

            // Pendolo corrente
            const float cur_ang = std::sin(world.sim_time * (2.0f * eco::kPi / t_live)) * 0.48f;
            const Vector2 cur_bob{pivot.x + std::sin(cur_ang) * len_px, pivot.y + std::cos(cur_ang) * len_px};
            DrawLineEx(pivot, cur_bob, 2.8f, live_col);
            DrawCircleV(pivot, 4.5f, pal::kBronzeBright);
            DrawCircleV(cur_bob, 9.5f, live_col);
            DrawCircleV(cur_bob, 3.8f, RAYWHITE);

            DrawTextShadow(TextFormat("T = %.2f s", t_live), static_cast<int>(diag_r.x) + 14, static_cast<int>(diag_r.y + diag_r.height) - 38, 13, live_col);
            DrawTextShadow(TextFormat("T0 = %.2f s", t_opt), static_cast<int>(diag_r.x) + 14, static_cast<int>(diag_r.y + diag_r.height) - 20, 12, pal::kAmberGlow);
            break;
        }
        case eco::MachineType::HarmonicResonator: {
            // Curva di risonanza Lorentziana live R(f) con cursore scorrevole
            const float gx0 = diag_r.x + 16.0f;
            const float gx1 = diag_r.x + diag_r.width - 16.0f;
            const float gy_bot = diag_r.y + diag_r.height - 46.0f;
            const float gy_top = diag_r.y + 22.0f;
            const float gh = gy_bot - gy_top;

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

            DrawTextShadow(TextFormat("f = %.1f Hz", val), static_cast<int>(diag_r.x) + 14, static_cast<int>(diag_r.y + diag_r.height) - 36, 13, live_col);
            DrawTextShadow("Picco di risonanza Q=3", static_cast<int>(diag_r.x) + 14, static_cast<int>(diag_r.y + diag_r.height) - 18, 11, pal::kBronzeBright);
            break;
        }
        case eco::MachineType::ThermalExchanger: {
            // Diagramma Carnot: serbatoio caldo Th vs freddo Tc=300K + rendimento eta_C
            const float eta_c = eco::ComputeCarnotEta(val, eco::kDefaultCarnotColdTempK) * 100.0f;
            const Rectangle hot_r{diag_r.x + 18.0f, diag_r.y + 18.0f, diag_r.width - 36.0f, 34.0f};
            const Rectangle cold_r{diag_r.x + 18.0f, diag_r.y + 104.0f, diag_r.width - 36.0f, 32.0f};
            DrawRectangleRec(hot_r, Color{156, 68, 34, 235});
            DrawRectangleLinesEx(hot_r, 1.6f, pal::kAmberGlow);
            DrawTextShadow(TextFormat("T_calda = %.0f K", val), static_cast<int>(hot_r.x) + 14, static_cast<int>(hot_r.y) + 9, 13, RAYWHITE);

            DrawRectangleRec(cold_r, Color{26, 86, 132, 235});
            DrawRectangleLinesEx(cold_r, 1.6f, Color{118, 218, 250, 240});
            DrawTextShadow("T_fredda = 300 K", static_cast<int>(cold_r.x) + 14, static_cast<int>(cold_r.y) + 8, 13, RAYWHITE);

            const float mid_x = diag_r.x + diag_r.width * 0.5f;
            DrawArrow2D({mid_x, hot_r.y + hot_r.height + 2.0f}, {mid_x, cold_r.y - 2.0f}, 2.6f, live_col);
            DrawCircleV({mid_x, diag_r.y + 78.0f}, 14.0f, Color{20, 32, 44, 245});
            DrawCircleLines(static_cast<int>(mid_x), static_cast<int>(diag_r.y + 78.0f), 14.0f, live_col);
            DrawTextShadow("W", static_cast<int>(mid_x) - 6, static_cast<int>(diag_r.y) + 71, 13, live_col);

            DrawTextShadow(TextFormat("Carnot eta = %.1f%%", eta_c), static_cast<int>(diag_r.x) + 14, static_cast<int>(diag_r.y + diag_r.height) - 36, 13, live_col);
            DrawTextShadow("Finestra sicura: 600 K", static_cast<int>(diag_r.x) + 14, static_cast<int>(diag_r.y + diag_r.height) - 18, 11, pal::kAccentClean);
            break;
        }
        case eco::MachineType::CentralAstrolabe:
        case eco::MachineType::Count: {
            // Interferenza di fase: onda di riferimento (oro) + onda sfasata (ciano) + risultante (giada)
            const float gx0 = diag_r.x + 14.0f;
            const float gx1 = diag_r.x + diag_r.width - 14.0f;
            const float mid_y = diag_r.y + 74.0f;
            const float amp = 24.0f;
            const float dphi = eco::deg2rad(val - machine.target_param);

            DrawLineEx({gx0, mid_y}, {gx1, mid_y}, 1.0f, pal::WithAlpha(pal::kBronze, 120));
            constexpr int kSteps = 44;
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
            DrawTextShadow("Oro/Ciano: Onde di Fase", static_cast<int>(diag_r.x) + 14, static_cast<int>(diag_r.y + diag_r.height) - 38, 11, pal::kAmberGlow);
            DrawTextShadow("Giada: Interferenza", static_cast<int>(diag_r.x) + 14, static_cast<int>(diag_r.y + diag_r.height) - 20, 12, live_col);
            break;
        }
    }
}

void HandleMouseModalCommit(eco::WorldContext& world, bool is_clean) {
    eco::Machine& target = world.ModalTargetMachine();
    if (is_clean) {
        if (!world.calibration_modal.ConfirmClean(target, world.ecosystem)) {
            return;
        }
        if (target.type == eco::MachineType::CentralAstrolabe) {
            world.simulation_completed = true;
        }
    } else {
        if (!target.allow_bypass) {
            return;
        }
        world.calibration_modal.TriggerBypass(target, world.ecosystem);
    }

    if (world.AllFiveMonumentsActivated()) {
        world.GetMachine(eco::MachineType::CentralAstrolabe).unlocked = true;
    }
    world.story.Observe(world.BuildStorySnapshot());
}

} // namespace

// =============================================================================
// PROTAGONISTA: ELENA VALLI (e colleghi in co-op locale P1..P4)
// =============================================================================
void DrawScientistProtagonist(const eco::Player& p, const eco::WorldContext& world) {
    const float cx = p.pos.x + p.size.x * 0.5f;
    const float cy = p.pos.y + p.size.y * 0.5f;
    const float speed = std::hypot(p.vel.x, (world.current_mode == eco::GameMode::Structure) ? p.y_velocity : p.vel.y);
    const float walk_bob = (speed > 5.0f) ? std::sin(world.sim_time * 15.0f + static_cast<float>(p.player_id)) * 2.1f : 0.0f;
    const float stride = (speed > 5.0f) ? std::sin(world.sim_time * 15.0f + static_cast<float>(p.player_id)) * 5.0f : 0.0f;
    const float dir_x = (std::fabs(p.vel.x) > 1.0f) ? (p.vel.x > 0.0f ? 1.0f : -1.0f) : 1.0f;
    const Color accent = pal::PlayerAccentColor(p.player_id);

    // Impulso della Lente Empirica [F]
    if (p.player_id == 0 && world.scan_pulse_timer > 0.0f) {
        const float progress = eco::Clamp(1.0f - (world.scan_pulse_timer / 0.85f), 0.0f, 1.0f);
        const float r1 = progress * eco::kDefaultScannerRange;
        const auto alpha = static_cast<unsigned char>((1.0f - progress) * 225.0f);
        DrawCircleLines(
            static_cast<int>(world.scan_pulse_origin.x),
            static_cast<int>(world.scan_pulse_origin.y),
            r1,
            pal::WithAlpha(pal::kBronzeBright, alpha));
        DrawCircleLines(
            static_cast<int>(world.scan_pulse_origin.x),
            static_cast<int>(world.scan_pulse_origin.y),
            std::max(4.0f, r1 * 0.68f),
            pal::WithAlpha(pal::kJadeGlow, alpha));
        if (world.last_scan.has_target) {
            const eco::Machine& target_m = world.GetMachine(world.last_scan.target_type);
            if (target_m.location == world.ActiveLocation()) {
                DrawLineEx({cx, cy}, {target_m.pos.x, target_m.pos.y}, 2.2f, pal::WithAlpha(pal::kJadeGlow, alpha));
            }
        }
    }

    // Ombra morbida a terra
    DrawEllipse(
        static_cast<int>(cx),
        static_cast<int>(p.pos.y + p.size.y + 1.5f),
        13.5f,
        5.0f,
        Color{0, 0, 0, 128});

    // Stivali da cammino animati
    DrawLineEx({cx - 4.5f, cy + 5.0f}, {cx - 4.5f - stride, p.pos.y + p.size.y}, 4.2f, Color{54, 42, 34, 255});
    DrawLineEx({cx + 4.5f, cy + 5.0f}, {cx + 4.5f + stride, p.pos.y + p.size.y}, 4.2f, Color{54, 42, 34, 255});

    // Borsa da cartografa in cuoio con pergamena arrotolata e spia della fase percettiva
    const float pack_x = (dir_x >= 0.0f) ? (cx - 13.0f) : (cx + 6.0f);
    DrawRectangle(static_cast<int>(pack_x), static_cast<int>(cy - 4.0f + walk_bob), 7, 12, Color{122, 82, 48, 255});
    DrawRectangleLines(static_cast<int>(pack_x), static_cast<int>(cy - 4.0f + walk_bob), 7, 12, pal::kBronzeBright);
    DrawCircleV({pack_x + 3.5f, cy - 5.5f + walk_bob}, 2.8f, pal::kStoneIvory);
    const Color phase_gem = (world.ecosystem.ui_phase == eco::UiPhase::Phase3_EngineOverlay)
                                ? pal::kJadeGlow
                                : (world.ecosystem.ui_phase == eco::UiPhase::Phase2_Telemetry
                                       ? Color{92, 214, 245, 255}
                                       : pal::kAmberGlow);
    DrawCircle(static_cast<int>(pack_x + 3.5f), static_cast<int>(cy + 1.0f + walk_bob), 2.3f, phase_gem);

    // Mantello chiaro da naturalista/cartografa (Elena Valli) con fibbia in bronzo e sciarpa giocatore
    const Color cloak_col = pal::kStoneIvory;
    DrawRectangle(static_cast<int>(cx - 8.0f), static_cast<int>(cy - 6.0f + walk_bob), 16, 15, cloak_col);
    DrawRectangle(static_cast<int>(cx - 8.0f), static_cast<int>(cy - 6.0f + walk_bob), 16, 4, accent);
    DrawRectangleLines(static_cast<int>(cx - 8.0f), static_cast<int>(cy - 6.0f + walk_bob), 16, 15, pal::kBronze);
    DrawCircleV({cx, cy - 2.0f + walk_bob}, 1.8f, pal::kBronzeBright);

    // Braccio con bussola / lente empirica in ottone
    const float arm_x = (dir_x >= 0.0f) ? (cx + 4.0f) : (cx - 8.0f);
    const Vector2 hand{arm_x + dir_x * 5.2f, cy + 3.8f + walk_bob - stride * 0.3f};
    DrawLineEx({arm_x, cy - 1.0f + walk_bob}, hand, 3.2f, cloak_col);
    DrawCircleV(hand, 2.6f, pal::kBronzeBright);

    // Testa, capelli castano ramato e lente oculare da campo
    const float head_y = cy - 11.5f + walk_bob;
    DrawCircle(static_cast<int>(cx), static_cast<int>(head_y), 7.5f, Color{240, 212, 188, 255});
    DrawCircleSector({cx, head_y - 0.5f}, 8.3f, 165.0f, 375.0f, 12, Color{84, 50, 38, 255});
    const float hair_back_x = (dir_x >= 0.0f) ? (cx - 7.5f) : (cx + 7.5f);
    DrawCircleV({hair_back_x, head_y + 1.5f}, 3.5f, Color{84, 50, 38, 255});

    const float lens_x = (dir_x >= 0.0f) ? (cx - 1.5f) : (cx - 7.5f);
    DrawRectangle(static_cast<int>(lens_x), static_cast<int>(head_y - 2.0f), 9, 4, accent);
    DrawRectangleLines(static_cast<int>(lens_x), static_cast<int>(head_y - 2.0f), 9, 4, pal::kBronzeDark);

    if (p.player_id > 0) {
        DrawTextShadow(
            TextFormat("E%d", p.player_id + 1),
            static_cast<int>(cx - 7.0f),
            static_cast<int>(head_y - 22.0f),
            11,
            accent);
    }
}

// =============================================================================
// MONUMENTI, CONDOTTE, RELIQUIE, NPC E TRAMA GEODETICA (WORLD-SPACE)
// =============================================================================
void DrawSceneMachineAndOverlaysWorldSpace(const eco::WorldContext& world) {
    const eco::OverlayVisibility ov = eco::ComputeOverlayVisibility(world.ecosystem);

    // Fase 3 (Trama Geodetica): griglia geodetica dei Meridiani
    if (ov.show_aabb_hitboxes || ov.show_velocity_vectors) {
        const eco::Tilemap& tm = world.ActiveTilemap();
        const int max_x = static_cast<int>(tm.WorldWidth());
        const int max_y = static_cast<int>(tm.WorldHeight());
        for (int gx = 0; gx <= max_x; gx += 144) {
            DrawLine(gx, 0, gx, max_y, Color{88, 232, 198, 34});
            DrawTextShadow(TextFormat("x=%d", gx), gx + 3, 52, 10, Color{122, 244, 212, 128});
        }
        for (int gy = 0; gy <= max_y; gy += 144) {
            DrawLine(0, gy, max_x, gy, Color{88, 232, 198, 34});
            DrawTextShadow(TextFormat("y=%d", gy), 52, gy + 3, 10, Color{122, 244, 212, 128});
        }
    }

    // Condotte armoniche sotterranee in pietra e bronzo nell'Overworld
    if (world.current_mode == eco::GameMode::Overworld) {
        const eco::Machine& solar = world.GetMachine(eco::MachineType::SolarArray);
        const eco::Machine& hydro = world.GetMachine(eco::MachineType::HydroTurbine);
        const eco::Machine& pendulum = world.GetMachine(eco::MachineType::PendulumRegulator);
        const eco::Machine& harmonic = world.GetMachine(eco::MachineType::HarmonicResonator);
        const eco::Machine& thermal = world.GetMachine(eco::MachineType::ThermalExchanger);
        const eco::Machine& astrolabe = world.GetMachine(eco::MachineType::CentralAstrolabe);
        const eco::WorldNpc& clara = world.GetNpc(eco::NpcId::ClaraSereni);

        const Vector2 solar_v{solar.pos.x, solar.pos.y};
        const Vector2 clara_v{clara.pos.x, clara.pos.y};
        const eco::Vec2 bridge_center = world.overworld.tilemap.TileCenter(17, eco::Tilemap::kBridgeDividerCol);
        const Vector2 bridge_v{bridge_center.x, bridge_center.y};
        const Vector2 harmonic_v{harmonic.pos.x, harmonic.pos.y};
        const Vector2 astrolabe_v{astrolabe.pos.x, astrolabe.pos.y};

        DrawHarmonicConduit(solar_v, clara_v, solar.state, world.sim_time, world.ecosystem);
        if (const eco::PortalLink* p_cistern = world.overworld.FindPortal(eco::StructureId::HypogeanCisterns)) {
            DrawHarmonicConduit({p_cistern->world_center.x, p_cistern->world_center.y}, clara_v, hydro.state, world.sim_time, world.ecosystem);
        }
        if (const eco::PortalLink* p_tower = world.overworld.FindPortal(eco::StructureId::PendulumTower)) {
            DrawHarmonicConduit({p_tower->world_center.x, p_tower->world_center.y}, clara_v, pendulum.state, world.sim_time, world.ecosystem);
        }

        const eco::MachineState bridge_state =
            world.ecosystem.architect_recruited ? eco::MachineState::CleanActive : eco::MachineState::Broken;
        DrawHarmonicConduit(clara_v, bridge_v, bridge_state, world.sim_time, world.ecosystem);

        if (world.ecosystem.architect_recruited) {
            DrawHarmonicConduit(bridge_v, harmonic_v, harmonic.state, world.sim_time, world.ecosystem);
            DrawHarmonicConduit(harmonic_v, astrolabe_v, astrolabe.state != eco::MachineState::Broken ? astrolabe.state : harmonic.state, world.sim_time, world.ecosystem);
            if (const eco::PortalLink* p_geo = world.overworld.FindPortal(eco::StructureId::GeothermalChamber)) {
                DrawHarmonicConduit({p_geo->world_center.x, p_geo->world_center.y}, astrolabe_v, thermal.state, world.sim_time, world.ecosystem);
            }
        }
    }

    // Reliquie collezionabili presenti nella scena corrente (Overworld o Struttura)
    DrawCollectiblesInScene(world);

    // Tutti i monumenti fisici presenti nella scena corrente
    const std::vector<const eco::Machine*> scene_machines = world.MachinesInCurrentScene();
    for (const eco::Machine* m : scene_machines) {
        if (m != nullptr) {
            DrawMonument(*m, world);
        }
    }

    // Tutti gli NPC presenti nella scena corrente
    const std::vector<const eco::WorldNpc*> scene_npcs = world.NpcsInCurrentScene();
    for (const eco::WorldNpc* npc : scene_npcs) {
        if (npc != nullptr) {
            DrawWorldNpcCharacter(*npc, world);
        }
    }

    // Fase 3 (Trama Geodetica): raggi di risonanza dei monumenti e NPC
    if (ov.show_trigger_radii) {
        for (const eco::Machine* m : scene_machines) {
            if (m != nullptr) {
                DrawCircleLines(
                    static_cast<int>(m->pos.x),
                    static_cast<int>(m->pos.y),
                    m->trigger_radius,
                    Color{104, 240, 214, 195});
            }
        }
        for (const eco::WorldNpc* npc : scene_npcs) {
            if (npc != nullptr) {
                DrawCircleLines(
                    static_cast<int>(npc->pos.x),
                    static_cast<int>(npc->pos.y),
                    npc->trigger_radius,
                    Color{246, 204, 108, 195});
            }
        }
    }

    // Fase 3: vettori direzionali sul monumento solare / astrolabio nella scena
    if (ov.show_irradiance_rays) {
        for (const eco::Machine* m : scene_machines) {
            if (m != nullptr && (m->type == eco::MachineType::SolarArray || m->type == eco::MachineType::CentralAstrolabe)) {
                const float s_rad = eco::deg2rad(m->target_param);
                const float p_rad = eco::deg2rad(m->current_param);
                const Vector2 center{m->pos.x, m->pos.y};
                DrawArrow2D(center, {center.x + std::cos(s_rad) * 64.0f, center.y + std::sin(s_rad) * 64.0f}, 2.4f, pal::kAmberGlow);
                DrawArrow2D(center, {center.x + std::cos(p_rad) * 52.0f, center.y + std::sin(p_rad) * 52.0f}, 2.4f, pal::kJadeGlow);
            }
        }
    }

    // Fase 3: cornice di rilievo geodetico e vettori di moto per gli esploratori attivi
    const eco::Machine& active_m = world.ActiveMachine();
    for (const eco::Player* ap : world.players.ActivePlayers()) {
        if (ap == nullptr || !ap->active) {
            continue;
        }
        if (ov.show_aabb_hitboxes) {
            DrawRectangleLinesEx(Rectangle{ap->pos.x, ap->pos.y, ap->size.x, ap->size.y}, 1.8f, Color{92, 244, 196, 240});
            DrawTextShadow(
                TextFormat("P(%.0f,%.0f)", ap->pos.x, ap->pos.y),
                static_cast<int>(ap->pos.x - 16.0f),
                static_cast<int>(ap->pos.y - 22.0f),
                11,
                pal::kJadeGlow);
        }
        if (ov.show_velocity_vectors) {
            const eco::Vec2 c = ap->Center();
            const float vy = (world.current_mode == eco::GameMode::Structure) ? ap->y_velocity : ap->vel.y;
            const Vector2 start{c.x, c.y};
            const Vector2 tip{c.x + ap->vel.x * 0.28f, c.y + vy * 0.28f};
            DrawArrow2D(start, tip, 2.4f, pal::kAmberGlow);
            if (world.current_mode == eco::GameMode::Structure && !ap->isGrounded) {
                DrawArrow2D(start, {c.x, c.y + 34.0f}, 2.0f, Color{240, 144, 72, 240});
                DrawTextShadow("g", static_cast<int>(c.x + 6.0f), static_cast<int>(c.y + 24.0f), 12, pal::kAmberGlow);
            }
            if (ap->player_id == 0 && active_m.location == world.ActiveLocation()) {
                const float dist = active_m.DistanceToPlayer(*ap);
                DrawLineEx(start, {active_m.pos.x, active_m.pos.y}, 1.2f, Color{106, 232, 224, 115});
                DrawTextShadow(
                    TextFormat("d=%.0f", dist),
                    static_cast<int>((c.x + active_m.pos.x) * 0.5f),
                    static_cast<int>((c.y + active_m.pos.y) * 0.5f),
                    11,
                    pal::kJadeGlow);
            }
        }
    }
}

// =============================================================================
// HUD SUPERIORE, LENTE EMPIRICA, MINIMAPPA & MODAL DI CALIBRAZIONE
// =============================================================================
void DrawHudAndCalibrationModalScreenSpace(eco::WorldContext& world) {
    const eco::OverlayVisibility ov = eco::ComputeOverlayVisibility(world.ecosystem);
    const eco::Machine& machine = world.ModalTargetMachine();

    // 1. Barra Superiore (y = 0..52) — stile Conca di Meridia
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
        TextFormat("Fase %d: %s", static_cast<int>(ov.phase), eco::UiPhaseName(ov.phase)),
        16,
        29,
        12,
        phase_col);

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

    // Badge comandi compatti a destra (nessuna sovrapposizione)
    int bx_hint = 768;
    auto draw_hint = [&](const char* key, const char* lbl, Color col) {
        DrawKeyBadge(key, bx_hint, 17, col);
        bx_hint += MeasureKeyBadge(key) + 5;
        DrawTextShadow(lbl, bx_hint, 19, 12, pal::kParchmentText);
        bx_hint += MeasureUiText(lbl, 12) + 12;
    };
    draw_hint("F", "Lente", pal::kJadeGlow);
    draw_hint("E", "Azione", pal::kAmberGlow);
    draw_hint("H", "Diario", pal::kUiBorderCyan);
    draw_hint("J", "Codex", pal::kBronzeBright);
    draw_hint("V", "Vista", pal::kJade);
    draw_hint("ESC", "Menu", LIGHTGRAY);

    // 2. Pannello Lente Empirica & Rilievo Metrico (alto-destra x=846, y=62, w=418)
    if (ov.show_raw_numbers || world.last_scan.active) {
        const float px = static_cast<float>(eco::kWindowWidth - 434);
        const float py = 62.0f;
        const float pw = 418.0f;
        const float ph = ov.show_raw_numbers ? 84.0f : 56.0f;
        DrawPanel(Rectangle{px, py, pw, ph}, pal::kUiBorderCyan);

        if (world.last_scan.active) {
            DrawTextShadow(
                world.last_scan.has_target
                    ? TextFormat(
                          "LENTE [F]: %s = %.2f %s (d=%.0f)",
                          eco::MachineName(world.last_scan.target_type),
                          world.last_scan.measured_target_value,
                          eco::MachineUnit(world.last_scan.target_type),
                          world.last_scan.measured_distance)
                    : "LENTE [F]: Nessun monumento nel raggio (avvicinati)",
                static_cast<int>(px) + 12,
                static_cast<int>(py) + 10,
                12,
                pal::kAmberGlow);
        } else {
            DrawTextShadow(
                "LENTE [F]: Avvicinati a un monumento per misurarlo",
                static_cast<int>(px) + 12,
                static_cast<int>(py) + 10,
                12,
                pal::kParchmentText);
        }

        if (ov.show_raw_numbers) {
            const eco::Player* p0 = world.players.GetPlayer(0);
            const float vx = p0 ? p0->vel.x : 0.0f;
            const float vy = p0 ? ((world.current_mode == eco::GameMode::Structure) ? p0->y_velocity : p0->vel.y) : 0.0f;
            DrawTextShadow(
                TextFormat(
                    "RILIEVO: v=(%.0f, %.0f) | Rend.=%.1f%% | Pece=%.0f MW",
                    vx,
                    vy,
                    world.ActiveMachine().LiveEfficiency() * 100.0f,
                    world.ecosystem.dirty_energy),
                static_cast<int>(px) + 12,
                static_cast<int>(py) + 32,
                12,
                Color{112, 220, 248, 255});
            DrawTextShadow(
                TextFormat(
                    "PERCEZIONE: Fase %d/%d | Ponte degli Archi: %s",
                    static_cast<int>(ov.phase),
                    static_cast<int>(world.ecosystem.MaxUnlockedUiPhase()),
                    world.ecosystem.architect_recruited ? "APERTO" : "CHIUSO"),
                static_cast<int>(px) + 12,
                static_cast<int>(py) + 54,
                12,
                pal::kJadeGlow);
        } else {
            DrawTextShadow(
                "Calibra un monumento senza pece per sbloccare il Rilievo [V]",
                static_cast<int>(px) + 12,
                static_cast<int>(py) + 32,
                12,
                pal::kAmberGlow);
        }
    }

    // 3. Minimappa Overworld sotto il pannello scanner (x=1064, y=158, w=200, h=146)
    DrawOverworldMinimap(world);

    // 3b. Bussola dell'obiettivo (bronzo e giada)
    DrawObjectiveCompass(world);

    // 4. Console Modale di Calibrazione Fisica per tutte le 6 leggi
    if (world.calibration_modal.is_open) {
        DrawRectangle(0, 0, eco::kWindowWidth, eco::kWindowHeight, Color{6, 11, 18, 198});
        const int box_w = 768;
        const int box_h = 408;
        const int bx = (eco::kWindowWidth - box_w) / 2;
        const int by = (eco::kWindowHeight - box_h) / 2;

        const char* modal_title = TextFormat(
            "%s  -  %s",
            eco::MachineName(machine.type),
            eco::MachineLawName(machine.type));
        DrawPanelWithHeader(
            Rectangle{static_cast<float>(bx), static_cast<float>(by), static_cast<float>(box_w), static_cast<float>(box_h)},
            42,
            modal_title,
            pal::kBronzeBright);

        // Formula della legge fisica
        DrawTextShadow(
            TextFormat("Legge: %s", eco::MachineFormulaLabel(machine.type)),
            bx + 24,
            by + 54,
            13,
            Color{132, 224, 248, 255});

        const bool target_revealed =
            ov.show_raw_numbers ||
            (world.last_scan.has_target &&
             world.last_scan.target_type == machine.type);

        if (target_revealed) {
            DrawTextShadow(
                TextFormat(
                    "Misura Empirica Rilevata: Target = %.2f %s",
                    machine.target_param,
                    eco::MachineUnit(machine.type)),
                bx + 24,
                by + 76,
                15,
                pal::kAccentClean);
        } else {
            DrawTextShadow(
                "Sguardo Empirico: osserva il monumento o usa la Lente [F] per leggere il target!",
                bx + 24,
                by + 76,
                13,
                pal::kAmberGlow);
        }

        // Slider interattivo con range reale del monumento
        const float min_param = machine.min_param;
        const float max_param = machine.max_param;
        const Rectangle slider_rect{
            static_cast<float>(bx + 24),
            static_cast<float>(by + 132),
            468.0f,
            26.0f
        };

        const Vector2 mouse = GetMousePosition();
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(
                mouse,
                Rectangle{slider_rect.x - 10.0f, slider_rect.y - 8.0f, slider_rect.width + 20.0f, slider_rect.height + 16.0f})) {
            const float t = eco::Clamp((mouse.x - slider_rect.x) / slider_rect.width, 0.0f, 1.0f);
            world.calibration_modal.SetParameter(eco::Lerp(min_param, max_param, t), machine);
        }

        DrawTextShadow(
            TextFormat(
                "Parametro [A/D, Frecce o Mouse]: %.2f %s   (Intervallo %.0f..%.0f)",
                world.calibration_modal.slider_value,
                eco::MachineUnit(machine.type),
                min_param,
                max_param),
            bx + 24,
            by + 106,
            15,
            pal::kParchmentText);

        const float norm_slider =
            eco::Clamp((world.calibration_modal.slider_value - min_param) / std::max(0.001f, max_param - min_param), 0.0f, 1.0f);
        DrawProgressBar(
            slider_rect,
            norm_slider,
            world.calibration_modal.can_confirm_clean ? pal::kAccentClean : Color{68, 168, 232, 235},
            pal::kBronzeBright);

        // Tacca target sullo slider (visibile solo se target scoperto o in fase >= 2)
        if (target_revealed) {
            const float norm_target =
                eco::Clamp((machine.target_param - min_param) / std::max(0.001f, max_param - min_param), 0.0f, 1.0f);
            const int target_px = static_cast<int>(slider_rect.x + slider_rect.width * norm_target);
            DrawLine(target_px, static_cast<int>(slider_rect.y - 6), target_px, static_cast<int>(slider_rect.y + slider_rect.height + 6), pal::kAmberGlow);
        }

        // Diagramma fisico interattivo a destra
        const Rectangle diag_rect{
            static_cast<float>(bx + 516),
            static_cast<float>(by + 54),
            228.0f,
            188.0f
        };
        DrawCalibrationDiagram(machine, world, diag_rect);

        // Barra di Rendimento con tacca soglia
        const float eff_pct = world.calibration_modal.preview_efficiency * 100.0f;
        const float thresh_pct = machine.Threshold() * 100.0f;
        const Color eff_col = world.calibration_modal.can_confirm_clean ? pal::kAccentClean : pal::kAmberGlow;
        DrawTextShadow(
            TextFormat("Rendimento Calcolato: %.1f%%  (Soglia richiesta: >= %.0f%%)", eff_pct, thresh_pct),
            bx + 24,
            by + 174,
            16,
            eff_col);
        const Rectangle eff_bar{static_cast<float>(bx + 24), static_cast<float>(by + 202), 468.0f, 22.0f};
        DrawProgressBar(eff_bar, world.calibration_modal.preview_efficiency, eff_col, pal::kBronze);
        const int th_x = static_cast<int>(eff_bar.x + eff_bar.width * machine.Threshold());
        DrawLine(th_x, static_cast<int>(eff_bar.y - 3), th_x, static_cast<int>(eff_bar.y + eff_bar.height + 3), RAYWHITE);

        // Pulsanti d'azione (Tastiera + Mouse)
        const Rectangle btn_confirm{static_cast<float>(bx + 24), static_cast<float>(by + 258), 720.0f, 38.0f};
        const Rectangle btn_bypass{static_cast<float>(bx + 24), static_cast<float>(by + 306), 720.0f, 38.0f};
        const Rectangle btn_cancel{static_cast<float>(bx + 24), static_cast<float>(by + 356), 248.0f, 34.0f};

        DrawRectangleRec(
            btn_confirm,
            world.calibration_modal.can_confirm_clean ? Color{22, 112, 82, 255} : Color{32, 42, 54, 255});
        DrawRectangleLinesEx(
            btn_confirm,
            2.0f,
            world.calibration_modal.can_confirm_clean ? pal::kAccentClean : GRAY);
        DrawTextShadow(
            "[INVIO / C / CLICK] CONFERMA CALIBRAZIONE PULITA (+Energia Armonica, -Foschia)",
            bx + 36,
            by + 269,
            15,
            world.calibration_modal.can_confirm_clean ? RAYWHITE : GRAY);

        if (machine.allow_bypass) {
            DrawRectangleRec(btn_bypass, Color{116, 48, 28, 255});
            DrawRectangleLinesEx(btn_bypass, 1.5f, pal::kAccentDirty);
            DrawTextShadow(
                TextFormat("[B / CLICK] %s (+Foschia di Pece!)", eco::kCombustionBypassLabel),
                bx + 36,
                by + 317,
                15,
                pal::kParchmentText);
        } else {
            DrawRectangleRec(btn_bypass, Color{24, 30, 38, 225});
            DrawRectangleLinesEx(btn_bypass, 1.2f, Color{96, 108, 120, 180});
            DrawTextShadow(
                "L'Astrolabio Centrale non accetta fornaci a pece: richiede pura risonanza.",
                bx + 36,
                by + 318,
                14,
                Color{174, 192, 206, 220});
        }

        DrawRectangleRec(btn_cancel, Color{36, 48, 64, 255});
        DrawRectangleLinesEx(btn_cancel, 1.4f, pal::kBronze);
        DrawTextShadow("[X / BACKSPACE / CLICK] Annulla", bx + 36, by + 365, 14, pal::kParchmentText);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(mouse, btn_confirm)) {
                HandleMouseModalCommit(world, true);
            } else if (machine.allow_bypass && CheckCollisionPointRec(mouse, btn_bypass)) {
                HandleMouseModalCommit(world, false);
            } else if (CheckCollisionPointRec(mouse, btn_cancel)) {
                world.calibration_modal.Cancel();
            }
        }
    }
}

} // namespace eco::app
