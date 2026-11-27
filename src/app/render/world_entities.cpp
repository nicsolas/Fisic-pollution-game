#include "app/render/world_entities.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

#include "app/raylib_input.h"
#include "app/render/palette.h"
#include "app/render/ui_style.h"
#include "core/collectible.h"
#include "core/ecosystem.h"
#include "core/machine.h"
#include "core/math_types.h"
#include "core/npc.h"
#include "core/player.h"

namespace eco::app {

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

[[nodiscard]] int ActivatedCoreMonumentsCount(const eco::WorldContext& world) noexcept {
    int count = 0;
    for (const eco::Machine& m : world.machines) {
        if (m.type != eco::MachineType::CentralAstrolabe && m.state != eco::MachineState::Broken) {
            ++count;
        }
    }
    return count;
}

namespace {

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

    // Quota ottimale h_opt (nascosta in Modalita' Fisica Pura)
    if (!world.hard_physics_mode) {
        const float opt_ratio = eco::Clamp(m.target_param / max_h, 0.0f, 1.0f);
        const float opt_y = tank_bottom - tank_h * opt_ratio;
        DrawLineEx({tank_x - 8.0f, opt_y}, {tank_x + tank_w + 8.0f, opt_y}, 2.4f, pal::kAccentClean);
        DrawTextShadow("h_opt", static_cast<int>(tank_x - 42.0f), static_cast<int>(opt_y - 6.0f), 11, pal::kAccentClean);
    }

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

    // Ago del termometro termodinamico + tacca target 600 K (nascosta in Modalita' Fisica Pura)
    const float curr_rad = eco::Lerp(-2.4f, 0.8f, heat_t);
    if (!world.hard_physics_mode) {
        const float target_t = eco::Clamp((m.target_param - m.min_param) / std::max(1.0f, m.max_param - m.min_param), 0.0f, 1.0f);
        const float target_rad = eco::Lerp(-2.4f, 0.8f, target_t);
        DrawLineEx(
            {cx + std::cos(target_rad) * 18.0f, cy + std::sin(target_rad) * 18.0f},
            {cx + std::cos(target_rad) * 32.0f, cy + std::sin(target_rad) * 32.0f},
            2.5f,
            pal::kAccentClean);
    }
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


} // namespace eco::app
