#include "app/calibration_ui.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

#include "app/raylib_input.h"
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
    if (m.approach == eco::CalibrationApproach::RiskyEfficient) {
        return Color{250, 174, 64, 255};
    }
    if (m.state == eco::MachineState::CleanActive ||
        m.approach == eco::CalibrationApproach::CleanSlow) {
        return pal::kAccentClean;
    }
    if (m.state == eco::MachineState::BypassedDirty ||
        m.approach == eco::CalibrationApproach::DirtyImmediate) {
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

[[nodiscard]] std::string FitModalText(
    std::string_view text,
    int max_width_px,
    int font_size) {
    if (text.empty()) {
        return {};
    }
    std::string out(text);
    if (MeasureUiText(out.c_str(), font_size) <= max_width_px) {
        return out;
    }
    while (out.size() > 3 && MeasureUiText((out + "..").c_str(), font_size) > max_width_px) {
        out.pop_back();
    }
    return out + "..";
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

void DrawInlineWorldPrompt(
    GamepadAction action,
    const char* kb_key,
    const char* text,
    int cx,
    int py,
    int min_w,
    Color accent) {
    const ActionIconInfo info = GetActionIconInfo(action);
    if (!info.use_controller) {
        const char* full = TextFormat("[%s] %s", kb_key, text);
        const int pw = std::max(min_w, MeasureUiText(full, 12) + 24);
        const int px = cx - pw / 2;
        DrawPanel(Rectangle{static_cast<float>(px), static_cast<float>(py), static_cast<float>(pw), 26.0f}, accent);
        DrawTextShadow(full, px + 12, py + 6, 12, accent);
        return;
    }
    const int icon_w = MeasureControllerButtonIconWidth(static_cast<int>(info.type), static_cast<int>(info.button), 9.0f);
    const int text_w = MeasureUiText(text, 12);
    const int pw = std::max(min_w, icon_w + 8 + text_w + 22);
    const int px = cx - pw / 2;
    DrawPanel(Rectangle{static_cast<float>(px), static_cast<float>(py), static_cast<float>(pw), 26.0f}, accent);
    DrawControllerButtonIcon(
        static_cast<int>(info.type),
        static_cast<int>(info.button),
        Vector2{static_cast<float>(px + 10) + static_cast<float>(icon_w) * 0.5f, static_cast<float>(py + 13)},
        9.0f);
    DrawTextShadow(text, px + 10 + icon_w + 7, py + 6, 12, accent);
}

void DrawMonumentPrompt(const eco::Machine& m, const eco::WorldContext& world, float y_offset) {
    if (!m.prompt_visible || world.calibration_modal.is_open) {
        return;
    }
    const int py = static_cast<int>(m.pos.y + y_offset);
    if (m.unlocked) {
        Color accent = pal::kAmberGlow;
        const char* prompt_txt = TextFormat("Calibra %s", eco::MachineName(m.type));
        if (m.state == eco::MachineState::BypassedDirty ||
            m.approach == eco::CalibrationApproach::DirtyImmediate) {
            accent = pal::kAccentDirty;
            prompt_txt = TextFormat("Ricalibra %s (Spegni Pece e Guarisci)", eco::MachineName(m.type));
        } else if (m.approach == eco::CalibrationApproach::RiskyEfficient) {
            accent = pal::kAmberGlow;
            prompt_txt = TextFormat("Ricalibra %s (Regime Critico -> Armonia)", eco::MachineName(m.type));
        } else if (m.state == eco::MachineState::CleanActive) {
            accent = pal::kAccentClean;
            prompt_txt = TextFormat("Ricalibra %s (Armonia Attiva)", eco::MachineName(m.type));
        }
        DrawInlineWorldPrompt(
            GamepadAction::Interact,
            "E",
            prompt_txt,
            static_cast<int>(m.pos.x),
            py,
            240,
            accent);
    } else {
        const Color accent{210, 145, 78, 240};
        const char* text = TextFormat("Sigillato: attiva prima i 5 monumenti (%d/5)", ActivatedCoreMonumentsCount(world));
        const int pw = std::max(240, MeasureUiText(text, 12) + 24);
        const int px = static_cast<int>(m.pos.x) - pw / 2;
        DrawPanel(Rectangle{static_cast<float>(px), static_cast<float>(py), static_cast<float>(pw), 26.0f}, accent);
        DrawTextShadow(text, px + 12, py + 6, 12, accent);
    }
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
// 1. ELIOSTATO SOLARE (Terrazze dell'Alba - Ottica / Legge di Lambert)
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
// 2. TURBINA IDROSTATICA (Cisterne Ipogee - Idrostatica / Legge di Stevino)
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
// 3. PENDOLO ISOCHRONOS (Torre del Pendolo - Galileo T = 2*pi*sqrt(L/g))
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
// 4. RISONATORE ARMONICO (Giardini Sospesi - Acustica / Risonanza Lorentziana)
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
// 5. SCAMBIATORE DI CARNOT (Camera Geotermica - Termodinamica)
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
// 6. ASTROLABIO CENTRALE (Santuario dell'Armonia - Sintesi & 4 Anelli d'Era)
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
    Color mood_col = accent;
    if (npc.mood == eco::NpcMood::FlourishingTrust) {
        mood_col = pal::kJadeGlow;
    } else if (npc.mood == eco::NpcMood::TenseOverdrive) {
        mood_col = pal::kAmberGlow;
    } else if (npc.mood == eco::NpcMood::ShelteredPolluted) {
        mood_col = pal::kAccentDirty;
    }
    DrawWorldLabelTag(
        TextFormat("%s - %s", title_lbl, eco::NpcMoodName(npc.mood)),
        nx,
        ny - 52.0f,
        mood_col,
        mood_col);

    if (npc.prompt_visible && !world.calibration_modal.is_open) {
        if (npc.status_note != nullptr && npc.status_note[0] != '\0') {
            DrawWorldLabelTag(npc.status_note, nx, ny - 71.0f, mood_col, pal::kParchmentText);
        }
        const int py = (world.current_mode == eco::GameMode::Structure) ? static_cast<int>(ny) - 96 : static_cast<int>(ny) + 24;
        DrawInlineWorldPrompt(
            GamepadAction::Interact,
            "E",
            TextFormat("Parla con %s (%s)", title_lbl, eco::NpcRoleTitle(npc.id)),
            static_cast<int>(nx),
            py,
            250,
            mood_col);
    }
}

// =============================================================================
// RELIQUIE COLLEZIONABILI NEL MONDO DI GIOCO (Overworld & Strutture)
// =============================================================================
void DrawCollectiblesInScene(const eco::WorldContext& world) {
    const eco::LocationId loc = world.ActiveLocation();
    const bool geodesic_vision =
        (world.ecosystem.ui_phase == eco::UiPhase::Phase3_EngineOverlay);

    for (const eco::CollectibleItem& item : world.collectibles.Items()) {
        if (item.collected || item.location != loc) {
            continue;
        }
        if (loc == eco::LocationId::Overworld && item.zone != world.current_zone) {
            continue;
        }

        const Color era_col = EraAccentColor(item.era);
        const float phase = static_cast<float>(item.id) * 0.9f;
        const float bob = std::sin(world.sim_time * 3.1f + phase) * 4.0f;
        const float pulse = 0.5f + 0.5f * std::sin(world.sim_time * 3.8f + phase);
        const Vector2 center{item.pos.x, item.pos.y - 4.0f + bob};

        float reveal = 1.0f;
        if (item.hidden) {
            float nearest_dist = 1e9f;
            for (const eco::Player* ap : world.players.ActivePlayers()) {
                if (ap != nullptr && ap->active) {
                    nearest_dist = std::min(nearest_dist, ap->Center().DistanceTo(item.pos));
                }
            }
            reveal = eco::Clamp((190.0f - nearest_dist) / 110.0f, 0.0f, 1.0f);
            if (geodesic_vision) {
                reveal = std::max(reveal, 0.55f);
            }
            if (reveal < 0.08f) {
                // Luccichio discreto nella nicchia segreta finche' Elena non si avvicina
                const unsigned char glint_a =
                    static_cast<unsigned char>(30.0f + 36.0f * pulse);
                DrawCircleV(center, 2.2f + pulse * 1.6f, pal::WithAlpha(era_col, glint_a));
                DrawCircleV(center, 1.1f, pal::WithAlpha(RAYWHITE, static_cast<unsigned char>(glint_a + 18)));
                continue;
            }
        }

        const auto scale_a = [reveal](float base_alpha) noexcept -> unsigned char {
            return static_cast<unsigned char>(eco::Clamp(base_alpha * reveal, 0.0f, 255.0f));
        };

        // Ombra morbida e fascio luminoso verticale sottile
        DrawEllipse(
            static_cast<int>(item.pos.x),
            static_cast<int>(item.pos.y + 13.0f),
            14.0f,
            5.2f,
            Color{0, 0, 0, scale_a(115.0f)});
        if (!item.hidden || reveal > 0.35f) {
            const float beam_scale = item.hidden ? eco::Clamp((reveal - 0.35f) / 0.65f, 0.0f, 1.0f) : 1.0f;
            DrawRectangleGradientV(
                static_cast<int>(item.pos.x - 10.0f),
                static_cast<int>(item.pos.y - 42.0f),
                20,
                52,
                pal::WithAlpha(era_col, 0),
                pal::WithAlpha(era_col, static_cast<unsigned char>((32.0f + 24.0f * pulse) * beam_scale)));
        }

        // Aureola pulsante a due anelli
        DrawCircleV(center, 22.0f + pulse * 4.0f, pal::WithAlpha(era_col, scale_a(30.0f + 24.0f * pulse)));
        DrawCircleLines(
            static_cast<int>(center.x),
            static_cast<int>(center.y),
            18.5f + pulse * 3.0f,
            pal::WithAlpha(era_col, scale_a(135.0f + 80.0f * pulse)));

        // Glifo procedurale della reliquia (Stele, Astrolabio, Prisma, Cilindro, Seme Fossile)
        const Color glyph_col = pal::WithAlpha(era_col, scale_a(255.0f));
        DrawRelicGlyph(static_cast<int>(item.kind), center, 13.5f, glyph_col, true, world.sim_time + phase);

        // 3 scintille orbitali dedicate
        for (int s = 0; s < 3; ++s) {
            const float a = world.sim_time * 2.1f + phase + static_cast<float>(s) * (2.0f * eco::kPi / 3.0f);
            const Vector2 orb{
                center.x + std::cos(a) * 19.0f,
                center.y + std::sin(a) * 11.0f
            };
            DrawCircleV(orb, 2.1f, pal::WithAlpha(RAYWHITE, scale_a(220.0f)));
        }

        // Prompt diegetico quando il giocatore si avvicina
        if (item.prompt_visible && !world.calibration_modal.is_open) {
            const eco::CollectibleMeta& meta = eco::CollectibleMetadata(item.id);
            DrawInlineWorldPrompt(
                GamepadAction::Interact,
                "E",
                TextFormat("Raccogli %s - %s", meta.code.c_str(), meta.title.c_str()),
                static_cast<int>(item.pos.x),
                static_cast<int>(item.pos.y) - 48,
                210,
                era_col);
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
    bool target_revealed) {
    DrawRectangleRec(diag_r, Color{10, 18, 28, 245});
    DrawRectangleLinesEx(diag_r, 1.4f, pal::WithAlpha(pal::kBronze, 200));

    const float val = world.calibration_modal.slider_value;
    const bool ok_clean = world.calibration_modal.can_confirm_clean;
    const bool ok_risky = world.calibration_modal.can_confirm_risky;
    const Color live_col = ok_clean
                               ? pal::kAccentClean
                               : (ok_risky ? pal::kAmberGlow : Color{86, 202, 240, 255});
    const Vector2 c{diag_r.x + diag_r.width * 0.5f, diag_r.y + diag_r.height * 0.42f};

    switch (machine.type) {
        case eco::MachineType::SolarArray: {
            DrawCircleV(c, 50.0f, Color{16, 26, 38, 255});
            DrawCircleLines(static_cast<int>(c.x), static_cast<int>(c.y), 50.0f, pal::WithAlpha(pal::kBronzeBright, 175));
            const float p_rad = eco::deg2rad(val);
            if (target_revealed) {
                const float s_rad = eco::deg2rad(machine.target_param);
                DrawArrow2D(c, {c.x + std::cos(s_rad) * 42.0f, c.y + std::sin(s_rad) * 42.0f}, 2.4f, pal::kAmberGlow);
            }
            DrawArrow2D(c, {c.x + std::cos(p_rad) * 38.0f, c.y + std::sin(p_rad) * 38.0f}, 3.0f, live_col);
            DrawTextShadow(
                target_revealed ? "Oro: Azimut Solare Rilevato" : "Oro: [Sperimenta per svelare]",
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
            if (target_revealed || world.calibration_modal.piezometric_relic_active) {
                const float opt_y = col_r.y + col_r.height * (1.0f - eco::Clamp(machine.target_param / std::max(1.0f, machine.max_param), 0.0f, 1.0f));
                DrawLineEx({col_r.x - 6.0f, opt_y}, {col_r.x + col_r.width + 6.0f, opt_y}, 2.2f, pal::kAccentClean);
            }
            DrawRectangleLinesEx(col_r, 1.6f, pal::kBronzeBright);

            const float p_kpa = (eco::kWaterDensity * eco::kGravityWater * val) / 1000.0f;
            const float opt_kpa = (eco::kWaterDensity * eco::kGravityWater * machine.target_param) / 1000.0f;
            DrawTextShadow("STEVINO LIVE", static_cast<int>(diag_r.x) + 78, static_cast<int>(diag_r.y) + 18, 11, pal::kBronzeBright);
            DrawTextShadow(TextFormat("h = %.1f m", val), static_cast<int>(diag_r.x) + 78, static_cast<int>(diag_r.y) + 34, 13, pal::kParchmentText);
            DrawTextShadow(TextFormat("P = %.1f kPa", p_kpa), static_cast<int>(diag_r.x) + 78, static_cast<int>(diag_r.y) + 52, 15, live_col);
            if (target_revealed || world.calibration_modal.piezometric_relic_active) {
                DrawTextShadow("BANDA ARMONICA", static_cast<int>(diag_r.x) + 78, static_cast<int>(diag_r.y) + 84, 11, pal::kAccentClean);
                DrawTextShadow(TextFormat("%.1f kPa", opt_kpa), static_cast<int>(diag_r.x) + 78, static_cast<int>(diag_r.y) + 100, 14, pal::kAccentClean);
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

            if (target_revealed) {
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
                target_revealed ? TextFormat("T0 = %.2f s", t_opt) : "Premi [T] per misurare T0",
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
                target_revealed ? "Equilibrio armonico: 600 K" : "Sperimenta [T] per equilibrio",
                static_cast<int>(diag_r.x) + 12,
                static_cast<int>(diag_r.y + diag_r.height) - 18,
                11,
                pal::kAccentClean);
            break;
        }
        case eco::MachineType::CentralAstrolabe:
        case eco::MachineType::Count: {
            const float gx0 = diag_r.x + 14.0f;
            const float gx1 = diag_r.x + diag_r.width - 14.0f;
            const float mid_y = diag_r.y + 66.0f;
            const float amp = 21.0f;
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
            DrawTextShadow("Oro/Ciano: Onde di Fase", static_cast<int>(diag_r.x) + 12, static_cast<int>(diag_r.y + diag_r.height) - 34, 11, pal::kAmberGlow);
            DrawTextShadow("Giada: Interferenza", static_cast<int>(diag_r.x) + 12, static_cast<int>(diag_r.y + diag_r.height) - 18, 11, live_col);
            break;
        }
    }
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

    // Stazioni di Osservazione Diegetica (ObservationNode) e Vasche Botaniche (BioPlanter)
    const eco::Player* p0_scene = world.players.GetPlayer(0);
    const eco::Vec2 p0_pos = (p0_scene != nullptr && p0_scene->active)
                                 ? p0_scene->Center()
                                 : world.ActiveCameraTarget();

    auto draw_observation_node_overlay = [&](const eco::ObservationNode& node) {
        const Color col = node.inspected ? pal::kJadeGlow : pal::kAmberGlow;
        if (node.title != nullptr && node.title[0] != '\0') {
            DrawWorldLabelTag(
                node.inspected ? TextFormat("[OSSERVATO] %s", node.title) : node.title,
                node.pos.x,
                node.pos.y - 38.0f,
                col,
                col);
        }
        if (node.inspected && p0_pos.DistanceTo(node.pos) <= node.interact_radius * 1.45f) {
            if (node.reading_text != nullptr && node.reading_text[0] != '\0') {
                const std::string short_read = FitModalText(node.reading_text, 340, 11);
                DrawWorldLabelTag(short_read.c_str(), node.pos.x, node.pos.y - 57.0f, pal::kJadeGlow, pal::kParchmentText);
            }
            if (node.hypothesis_hint != nullptr && node.hypothesis_hint[0] != '\0') {
                const std::string short_hint = FitModalText(node.hypothesis_hint, 340, 11);
                DrawWorldLabelTag(short_hint.c_str(), node.pos.x, node.pos.y - 76.0f, pal::kAmberGlow, pal::kAmberGlow);
            }
        }
        if (node.prompt_visible && !world.calibration_modal.is_open) {
            const int py = (world.current_mode == eco::GameMode::Structure)
                               ? static_cast<int>(node.pos.y) - 96
                               : static_cast<int>(node.pos.y) + 22;
            DrawInlineWorldPrompt(
                GamepadAction::Interact,
                "E",
                TextFormat(
                    "%s %s",
                    node.inspected ? "Rileggi Rilievo:" : "Osserva Indizio:",
                    (node.title != nullptr && node.title[0] != '\0') ? node.title : "Stazione"),
                static_cast<int>(node.pos.x),
                py,
                220,
                col);
        }
    };

    if (world.current_mode == eco::GameMode::Overworld) {
        for (const eco::ObservationNode& node : world.zone_mechanics.observation_nodes) {
            if (node.zone == world.current_zone) {
                draw_observation_node_overlay(node);
            }
        }

        const bool has_olive_seed =
            world.collectibles.IsCollected(eco::CollectibleId::Dawn_WhiteOliveSeed);
        for (const eco::BioPlanter& planter : world.zone_mechanics.bio_planters) {
            if (planter.zone != world.current_zone) {
                continue;
            }
            const Color pcol = planter.planted ? pal::kJadeGlow : (has_olive_seed ? pal::kAccentClean : pal::kAmberGlow);
            const char* plbl = (planter.label != nullptr && planter.label[0] != '\0')
                                   ? planter.label
                                   : "Vasca Botanica";
            DrawWorldLabelTag(
                planter.planted
                    ? TextFormat("%s (Ulivo Bianco Fiorito)", plbl)
                    : TextFormat("%s (Arida)", plbl),
                planter.pos.x,
                planter.pos.y - 38.0f,
                pcol,
                pcol);
            if (planter.prompt_visible && !world.calibration_modal.is_open && !planter.planted) {
                DrawInlineWorldPrompt(
                    GamepadAction::Interact,
                    "E",
                    has_olive_seed
                        ? "Innesta Seme d'Ulivo Bianco (-6% Foschia, Apre Rovi)"
                        : "Vasca Botanica Arida (Richiede Seme d'Ulivo Bianco)",
                    static_cast<int>(planter.pos.x),
                    static_cast<int>(planter.pos.y) + 22,
                    240,
                    pcol);
            }
        }
    } else {
        for (const eco::ObservationNode& node : world.structure.puzzle.observation_nodes) {
            draw_observation_node_overlay(node);
        }

        // Valvola di Equilibrio Ausiliaria di Stevino nelle Cisterne Ipogee (Stele II.2)
        if (world.active_structure == eco::StructureId::HypogeanCisterns) {
            const auto& pz = world.structure.puzzle;
            if (pz.aux_stevino_valve_pos.x > 1.0f || pz.aux_stevino_valve_pos.y > 1.0f) {
                const Color vcol = pz.aux_stevino_valve_open
                                       ? pal::kJadeGlow
                                       : (pz.aux_stevino_valve_unlocked ? pal::kAmberGlow : Color{138, 158, 174, 220});
                DrawWorldLabelTag(
                    pz.aux_stevino_valve_open
                        ? "VALVOLA DI STEVINO: APERTA"
                        : (pz.aux_stevino_valve_unlocked ? "VALVOLA DI STEVINO [STELE II.2]" : "VALVOLA AUSILIARIA"),
                    pz.aux_stevino_valve_pos.x,
                    pz.aux_stevino_valve_pos.y - 36.0f,
                    vcol,
                    vcol);
                if (pz.aux_valve_prompt_visible && !world.calibration_modal.is_open) {
                    DrawInlineWorldPrompt(
                        GamepadAction::Interact,
                        "E",
                        pz.aux_stevino_valve_unlocked
                            ? (pz.aux_stevino_valve_open
                                   ? "Chiudi Valvola di Equilibrio di Stevino"
                                   : "Apri Valvola di Equilibrio di Stevino [Stele II.2]")
                            : "Valvola Sigillata (Richiede Stele delle Chiusure II.2)",
                        static_cast<int>(pz.aux_stevino_valve_pos.x),
                        static_cast<int>(pz.aux_stevino_valve_pos.y) - 66,
                        240,
                        vcol);
                }
            }
        }
    }

    // Tutti i monumenti fisici presenti nella scena corrente
    const std::vector<const eco::Machine*> scene_machines = world.MachinesInCurrentScene();
    for (const eco::Machine* m : scene_machines) {
        if (m != nullptr) {
            DrawMonument(*m, world);
        }
    }

    // Overlay Strumento 1: Gnomone d'Ottone (I.1) -> vettori ottici multi-rimbalzo e angolo d'ombra con Lente [Q]
    if (world.current_mode == eco::GameMode::Overworld &&
        world.current_zone == eco::ZoneId::DawnTerraces &&
        world.collectibles.IsCollected(eco::CollectibleId::Dawn_BrassGnomon) &&
        (world.scan_pulse_timer > 0.0f || world.empirical_compass_timer > 0.0f || ov.show_irradiance_rays)) {
        const std::vector<eco::SolarBeamSegment> gnomon_beams = world.PredictGnomonBeams();
        for (std::size_t bi = 0; bi < gnomon_beams.size(); ++bi) {
            const auto& seg = gnomon_beams[bi];
            const Vector2 s{seg.start.x, seg.start.y};
            const Vector2 e{seg.end.x, seg.end.y};
            DrawLineEx(s, e, 5.0f, Color{255, 224, 112, 85});
            DrawArrow2D(s, e, 2.4f, Color{122, 248, 216, 235});
            DrawCircleV(e, 4.5f, pal::kAmberGlow);
        }
        const eco::Machine& solar_m = world.GetMachine(eco::MachineType::SolarArray);
        const float sun_rad = eco::deg2rad(solar_m.target_param);
        const Vector2 sc{solar_m.pos.x, solar_m.pos.y};
        DrawArrow2D(
            sc,
            {sc.x + std::cos(sun_rad) * 74.0f, sc.y + std::sin(sun_rad) * 74.0f},
            2.6f,
            pal::kJadeGlow);
        DrawWorldLabelTag(
            TextFormat("GNOMONE [Q]: Ombra/Azimut Solare = %.1f deg (%d rimbalzi)", solar_m.target_param, static_cast<int>(gnomon_beams.size())),
            solar_m.pos.x,
            solar_m.pos.y - 74.0f,
            pal::kJadeGlow,
            pal::kJadeGlow);
    }

    // Overlay Strumento 5: Stele delle Chiusure (II.2) -> cartellino piezometrico Stevino live nelle Cisterne
    if (world.current_mode == eco::GameMode::Structure &&
        world.active_structure == eco::StructureId::HypogeanCisterns &&
        world.collectibles.IsCollected(eco::CollectibleId::Waters_SluiceStele)) {
        const eco::Machine& hydro_m = world.GetMachine(eco::MachineType::HydroTurbine);
        const float h_live = ActiveMachineParam(hydro_m, world);
        const float p_kpa = (eco::kWaterDensity * eco::kGravityWater * h_live) / 1000.0f;
        const float p_opt_kpa = (eco::kWaterDensity * eco::kGravityWater * hydro_m.target_param) / 1000.0f;
        DrawWorldLabelTag(
            TextFormat(
                "Stevino [Stele II.2]: h = %.1f m / P = %.1f kPa (Eq. %.1f m / %.1f kPa)",
                h_live,
                p_kpa,
                hydro_m.target_param,
                p_opt_kpa),
            hydro_m.pos.x - 48.0f,
            hydro_m.pos.y - 108.0f,
            pal::kJadeGlow,
            pal::kParchmentText);
    }

    // Overlay Strumento 6: Prisma del Congedo (IV.1) -> aggancio remoto [Q] Risonanza Prisma entro 220 px
    if (world.collectibles.IsCollected(eco::CollectibleId::Harmony_FarewellPrism) &&
        !world.calibration_modal.is_open) {
        const eco::RemotePrismTarget rt =
            world.FindRemotePrismTarget(p0_pos, eco::kFarewellPrismRemoteRange);
        if (rt.valid) {
            const float pr_pulse = 0.5f + 0.5f * std::sin(world.sim_time * 5.2f);
            DrawLineEx(
                {p0_pos.x, p0_pos.y},
                {rt.world_pos.x, rt.world_pos.y},
                1.6f,
                pal::WithAlpha(pal::kJadeGlow, static_cast<unsigned char>(90 + 70 * pr_pulse)));
            DrawPolyLinesEx(
                {rt.world_pos.x, rt.world_pos.y},
                6,
                22.0f + pr_pulse * 4.0f,
                world.sim_time * 48.0f,
                2.0f,
                pal::kJadeGlow);
            DrawWorldLabelTag(
                TextFormat("[Q] Risonanza Prisma: %s (%.0f px)", rt.label, rt.distance),
                rt.world_pos.x,
                rt.world_pos.y - 44.0f,
                pal::kJadeGlow,
                pal::kJadeGlow);
        }
    }
    if (world.prism_pulse_timer > 0.0f) {
        const float alpha_t = eco::Clamp(world.prism_pulse_timer / 0.55f, 0.0f, 1.0f);
        const auto pa = static_cast<unsigned char>(alpha_t * 235.0f);
        DrawLineEx(
            {world.prism_pulse_from.x, world.prism_pulse_from.y},
            {world.prism_pulse_to.x, world.prism_pulse_to.y},
            4.2f,
            pal::WithAlpha(pal::kJadeGlow, pa));
        DrawCircleV({world.prism_pulse_to.x, world.prism_pulse_to.y}, 16.0f * (1.2f - alpha_t * 0.5f), pal::WithAlpha(pal::kJadeGlow, pa));
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
                    has_gnomon || ov.show_raw_numbers || scanned_m.trials_count > 0 ||
                    scanned_m.state != eco::MachineState::Broken;
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
                    eco::GuidanceAssistModeName(world.guidance_mode),
                    world.total_trials_run,
                    world.ecosystem.architect_recruited ? "APERTO" : "CHIUSO"),
                static_cast<int>(px) + 12,
                static_cast<int>(py) + 54,
                12,
                pal::kJadeGlow);
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
    if (world.calibration_modal.is_open) {
        DrawRectangle(0, 0, eco::kWindowWidth, eco::kWindowHeight, Color{5, 10, 16, 212});
        const int box_w = 1120;
        const int box_h = 640;
        const int bx = (eco::kWindowWidth - box_w) / 2;
        const int by = (eco::kWindowHeight - box_h) / 2;

        const char* modal_title = TextFormat(
            "BANCO D'INDAGINE: %s  |  %s  (%s)  |  Stato: %s",
            eco::MachineName(machine.type),
            eco::MachineLawName(machine.type),
            eco::MachineFormulaLabel(machine.type),
            eco::CalibrationApproachName(machine.approach));
        DrawPanelWithHeader(
            Rectangle{static_cast<float>(bx), static_cast<float>(by), static_cast<float>(box_w), static_cast<float>(box_h)},
            38,
            modal_title,
            pal::kBronzeBright);

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
            world.calibration_modal.harmonic_band_revealed ||
            world.calibration_modal.trials_run > 0 ||
            ov.show_raw_numbers ||
            has_gnomon_for_target;

        const Rectangle left_bench{
            static_cast<float>(bx + 24),
            static_cast<float>(by + 110),
            684.0f,
            182.0f
        };
        DrawRectangleRec(left_bench, Color{12, 20, 30, 236});
        DrawRectangleLinesEx(left_bench, 1.3f, pal::WithAlpha(pal::kUiBorderBronze, 195));

        if (target_revealed) {
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

        // Slider 1: Parametro Principale [A/D, Frecce SX/DX, Mouse]
        const float min_param = machine.min_param;
        const float max_param = machine.max_param;
        const float span_param = std::max(0.001f, max_param - min_param);
        const Rectangle slider_rect{
            static_cast<float>(bx + 36),
            static_cast<float>(by + 156),
            660.0f,
            20.0f
        };
        if (mouse_down &&
            CheckCollisionPointRec(
                mouse,
                Rectangle{slider_rect.x - 8.0f, slider_rect.y - 6.0f, slider_rect.width + 16.0f, slider_rect.height + 12.0f})) {
            const float t = eco::Clamp((mouse.x - slider_rect.x) / slider_rect.width, 0.0f, 1.0f);
            world.calibration_modal.SetParameter(eco::Lerp(min_param, max_param, t), machine);
        }

        DrawTextShadow(
            TextFormat(
                "1. Parametro Principale [A/D o SX/DX]: %.2f %s   (Range %.0f..%.0f)%s",
                world.calibration_modal.slider_value,
                eco::MachineUnit(machine.type),
                min_param,
                max_param,
                world.calibration_modal.precision_relic_active ? "  [Nonio Fine 0.25x]" : ""),
            bx + 36,
            by + 138,
            13,
            pal::kParchmentText);

        const float norm_slider =
            eco::Clamp((world.calibration_modal.slider_value - min_param) / span_param, 0.0f, 1.0f);
        DrawProgressBar(
            slider_rect,
            norm_slider,
            world.calibration_modal.can_confirm_clean
                ? pal::kAccentClean
                : (world.calibration_modal.can_confirm_risky ? pal::kAmberGlow : Color{68, 168, 232, 235}),
            pal::kBronzeBright);

        // Finestra supercritica sul cursore principale (ambra)
        {
            const float r0 = eco::Clamp((machine.risky_param_min - min_param) / span_param, 0.0f, 1.0f);
            const float r1 = eco::Clamp((machine.risky_param_max - min_param) / span_param, 0.0f, 1.0f);
            if (r1 > r0) {
                const float rx = slider_rect.x + slider_rect.width * r0;
                const float rw = slider_rect.width * (r1 - r0);
                DrawRectangleRec(Rectangle{rx, slider_rect.y + 2.0f, rw, slider_rect.height - 4.0f}, Color{248, 178, 64, 72});
                DrawRectangleLinesEx(Rectangle{rx, slider_rect.y, rw, slider_rect.height}, 1.2f, pal::kAmberGlow);
            }
        }
        if (target_revealed) {
            const float norm_target = eco::Clamp((machine.target_param - min_param) / span_param, 0.0f, 1.0f);
            const int target_px = static_cast<int>(slider_rect.x + slider_rect.width * norm_target);
            DrawLine(target_px, static_cast<int>(slider_rect.y - 4), target_px, static_cast<int>(slider_rect.y + slider_rect.height + 4), pal::kAccentClean);
        }

        // Slider 2: Stabilizzatore Armonico / Apertura Secondaria [W/S, Frecce SU/GIU, Mouse]
        const Rectangle stab_rect{
            static_cast<float>(bx + 36),
            static_cast<float>(by + 204),
            660.0f,
            20.0f
        };
        if (mouse_down &&
            CheckCollisionPointRec(
                mouse,
                Rectangle{stab_rect.x - 8.0f, stab_rect.y - 6.0f, stab_rect.width + 16.0f, stab_rect.height + 12.0f})) {
            const float t = eco::Clamp((mouse.x - stab_rect.x) / stab_rect.width, 0.0f, 1.0f);
            world.calibration_modal.SetStabilizer(t, machine);
        }

        const bool in_harm_stab = machine.IsInHarmonicStabilizerBand(
            world.calibration_modal.stabilizer_value,
            world.calibration_modal.precision_relic_active);
        const bool in_crit_stab =
            (world.calibration_modal.stabilizer_value <= machine.risky_max_stabilizer);

        DrawTextShadow(
            TextFormat(
                "2. Stabilizzatore / Valvola [W/S o SU/GIU]: %.0f%%   (%s)",
                world.calibration_modal.stabilizer_value * 100.0f,
                in_harm_stab
                    ? "In Banda Armonica Pulita"
                    : (in_crit_stab ? "Apertura Supercritica (<40%)" : "Fuori Banda")),
            bx + 36,
            by + 186,
            13,
            in_harm_stab ? pal::kAccentClean : (in_crit_stab ? pal::kAmberGlow : pal::kParchmentText));

        DrawProgressBar(
            stab_rect,
            eco::Clamp(world.calibration_modal.stabilizer_value, 0.0f, 1.0f),
            in_harm_stab ? pal::kAccentClean : (in_crit_stab ? pal::kAmberGlow : Color{92, 178, 224, 235}),
            pal::kBronze);

        // Bande visive sullo Stabilizzatore: Critica (< risky_max_stabilizer) e Armonica (clean_stabilizer_target +- window)
        {
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

        // Indicatori Rendimento, Resa Critica, Stress & Strumenti Reliquia Attivi
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
        DrawCalibrationDiagram(machine, world, diag_rect, target_revealed);

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
        const float clean_poll_drop =
            world.calibration_modal.olive_seed_bonus_active ? 22.0f : machine.clean_pollution_reduction;
        draw_solution_card(
            card_clean,
            "1. PULITA LENTA (ARMONIA)",
            world.calibration_modal.can_confirm_clean
                ? "STATO: PRONTA (Mira + Stab. Armonico)"
                : "RICHIEDE: Rend. >= Soglia e Stab. Armonico",
            TextFormat("Bilancio: +%.0f MW Energia | -%.0f%% Foschia", machine.clean_energy_reward, clean_poll_drop),
            "- Percorsi: purifica i rovi e apre sentieri verdi",
            "- Ecosistema: rifioritura flora e ritorno fauna",
            "- NPC: conquista fiducia di Clara e Lorenzo",
            prompts.controller_active
                ? TextFormat("%s CONFERMA ARMONIA PULITA", prompts.confirm)
                : "[INVIO / Click] CONFERMA ARMONIA PULITA",
            pal::kAccentClean,
            world.calibration_modal.can_confirm_clean,
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
            world.calibration_modal.can_confirm_risky
                ? "STATO: PRONTA (Finestra Supercritica)"
                : "RICHIEDE: Param. Critico e Stab. <= 40%",
            TextFormat("Bilancio: +%.0f MW Max Resa | +%.0f%% Stress", machine.risky_energy_reward, machine.risky_pollution_increase),
            risky_paths,
            risky_eco,
            "- NPC: Clara e Lorenzo in allerta tecnica",
            prompts.controller_active
                ? TextFormat("%s ATTIVA REGIME CRITICO", prompts.risky)
                : "[R / Click] ATTIVA REGIME CRITICO",
            pal::kAmberGlow,
            world.calibration_modal.can_confirm_risky,
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
            } else if (CheckCollisionPointRec(mouse, card_clean)) {
                (void)world.ExecuteModalConfirmClean();
            } else if (CheckCollisionPointRec(mouse, card_risky)) {
                (void)world.ExecuteModalConfirmRisky();
            } else if (machine.allow_bypass && CheckCollisionPointRec(mouse, card_dirty)) {
                (void)world.ExecuteModalBypass();
            }
        }
    }
}

} // namespace eco::app
