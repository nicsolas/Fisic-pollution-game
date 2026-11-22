#include "app/calibration_ui.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>

#include "app/render/palette.h"
#include "app/render/ui_style.h"
#include "core/ecosystem.h"
#include "core/machine.h"
#include "core/math_types.h"
#include "core/npc.h"
#include "core/player.h"

namespace eco::app {

namespace {

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
    const Vector2 left{end.x - ux * 9.0f + px * 5.0f, end.y - uy * 9.0f + py * 5.0f};
    const Vector2 right{end.x - ux * 9.0f - px * 5.0f, end.y - uy * 9.0f - py * 5.0f};
    DrawLineEx(end, left, thickness, color);
    DrawLineEx(end, right, thickness, color);
}

void DrawEnergyConduit(
    Vector2 a,
    Vector2 b,
    eco::MachineState state,
    float sim_time,
    const eco::EcoSystem& eco) {
    const float poll = eco.PollutionRatio();
    Color base_line = pal::WorldColor(Color{74, 66, 58, 175}, Color{48, 86, 112, 185}, poll);
    Color pulse_col = Color{80, 110, 130, 140};
    if (state == eco::MachineState::CleanActive) {
        base_line = Color{28, 168, 205, 225};
        pulse_col = pal::kAccentClean;
    } else if (state == eco::MachineState::BypassedDirty) {
        base_line = Color{175, 88, 34, 225};
        pulse_col = pal::kAccentDirty;
    }

    const Vector2 corner{b.x, a.y};
    // Guaina esterna scura + conduttore interno
    DrawLineEx(a, corner, 5.0f, Color{16, 24, 32, 160});
    DrawLineEx(corner, b, 5.0f, Color{16, 24, 32, 160});
    DrawLineEx(a, corner, 2.8f, base_line);
    DrawLineEx(corner, b, 2.8f, base_line);
    DrawCircleV(a, 4.5f, base_line);
    DrawCircleV(corner, 4.5f, base_line);
    DrawCircleV(b, 4.5f, base_line);

    if (state != eco::MachineState::Broken) {
        const float seg1 = std::hypot(corner.x - a.x, corner.y - a.y);
        const float seg2 = std::hypot(b.x - corner.x, b.y - corner.y);
        const float total = std::max(1.0f, seg1 + seg2);
        for (int i = 0; i < 3; ++i) {
            const float phase = std::fmod(sim_time * 0.65f + static_cast<float>(i) * 0.33f, 1.0f);
            const float dist = phase * total;
            Vector2 p = a;
            if (dist <= seg1 && seg1 > 0.0f) {
                const float t = dist / seg1;
                p = {eco::Lerp(a.x, corner.x, t), eco::Lerp(a.y, corner.y, t)};
            } else if (seg2 > 0.0f) {
                const float t = (dist - seg1) / seg2;
                p = {eco::Lerp(corner.x, b.x, t), eco::Lerp(corner.y, b.y, t)};
            }
            DrawCircleV(p, 5.0f, pal::WithAlpha(pulse_col, 90));
            DrawCircleV(p, 3.2f, pulse_col);
        }
    }
}

void DrawSolarArrayApparatus(
    const eco::Machine& m,
    const eco::WorldContext& world,
    bool is_purifier) {
    const float cx = m.pos.x;
    const float cy = m.pos.y;
    const float poll = world.ecosystem.PollutionRatio();
    const float active_angle =
        (world.calibration_modal.is_open &&
         ((!is_purifier && !world.calibrating_purifier &&
           world.calibration_modal.target_machine == eco::MachineType::SolarArray) ||
          (is_purifier && world.calibrating_purifier)))
            ? world.calibration_modal.slider_value
            : m.current_param;

    // Ombra a terra sotto il basamento dell'impianto
    DrawEllipse(static_cast<int>(cx + 4.0f), static_cast<int>(cy + 14.0f), 38.0f, 18.0f, Color{0, 0, 0, 115});

    // Basamento ottagonale tecnico
    const Color pad_fill = pal::WorldColor(Color{46, 44, 42, 255}, Color{30, 48, 66, 255}, poll);
    const Color pad_edge = (m.state == eco::MachineState::CleanActive)
                               ? pal::kAccentClean
                               : (m.state == eco::MachineState::BypassedDirty
                                      ? pal::kAccentDirty
                                      : Color{95, 155, 195, 225});
    DrawPoly({cx, cy}, 8, 34.0f, 22.5f, pad_fill);
    DrawPolyLinesEx({cx, cy}, 8, 34.0f, 22.5f, 2.0f, pad_edge);
    DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), 23.0f, Color{75, 120, 155, 130});

    // Raggi solari / armonici incidenti di riferimento (fenomeno fisico sempre osservabile)
    const float sun_rad = eco::deg2rad(m.target_param);
    const Vector2 sun_dir{std::cos(sun_rad), std::sin(sun_rad)};
    for (int r = -1; r <= 1; ++r) {
        const float perp_x = -sun_dir.y * static_cast<float>(r) * 12.0f;
        const float perp_y = sun_dir.x * static_cast<float>(r) * 12.0f;
        const float pulse = std::fmod(world.sim_time * 38.0f + static_cast<float>(r + 1) * 15.0f, 52.0f);
        const Vector2 ray_start{cx + sun_dir.x * 66.0f + perp_x, cy + sun_dir.y * 66.0f + perp_y};
        const Vector2 ray_end{cx + sun_dir.x * 22.0f + perp_x, cy + sun_dir.y * 22.0f + perp_y};
        DrawLineEx(
            ray_start,
            ray_end,
            1.8f,
            is_purifier ? Color{110, 235, 255, 180} : Color{255, 228, 95, 185});
        const Vector2 photon{
            ray_start.x - sun_dir.x * pulse * 0.85f,
            ray_start.y - sun_dir.y * pulse * 0.85f
        };
        DrawCircleV(photon, 2.6f, is_purifier ? SKYBLUE : YELLOW);
    }

    if (is_purifier) {
        // TORRE DEL RISONATORE FOTONICO / PURIFICATORE ATMOSFERICO (Est)
        // Struttura verticale con bocchette di aspirazione smog, specchio eliostatico e 2 anelli controrotanti
        const float r_speed = (m.state == eco::MachineState::CleanActive) ? 95.0f : 22.0f;
        const float a1 = eco::deg2rad(active_angle + world.sim_time * r_speed);
        const float a2 = eco::deg2rad(-active_angle * 0.7f - world.sim_time * r_speed * 0.8f);

        // Colonna della torre in elevazione 2.5D
        DrawRectangle(static_cast<int>(cx - 11.0f), static_cast<int>(cy - 22.0f), 22, 26, Color{36, 56, 76, 255});
        DrawRectangleLines(static_cast<int>(cx - 11.0f), static_cast<int>(cy - 22.0f), 22, 26, pad_edge);

        // Anelli armonici e specchio orientato secondo active_angle
        DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy - 10.0f), 22.0f, SKYBLUE);
        DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy - 10.0f), 15.0f, pal::kAccentClean);
        DrawLineEx(
            {cx - std::cos(a1) * 21.0f, (cy - 10.0f) - std::sin(a1) * 21.0f},
            {cx + std::cos(a1) * 21.0f, (cy - 10.0f) + std::sin(a1) * 21.0f},
            3.0f,
            Color{110, 255, 235, 255});
        DrawLineEx(
            {cx - std::cos(a2) * 14.0f, (cy - 10.0f) - std::sin(a2) * 14.0f},
            {cx + std::cos(a2) * 14.0f, (cy - 10.0f) + std::sin(a2) * 14.0f},
            2.2f,
            SKYBLUE);

        // Vettore normale dello specchio eliostatico
        const float pan_rad = eco::deg2rad(active_angle);
        DrawArrow2D(
            {cx, cy - 10.0f},
            {cx + std::cos(pan_rad) * 28.0f, (cy - 10.0f) + std::sin(pan_rad) * 28.0f},
            2.4f,
            pad_edge);
        DrawCircleV({cx, cy - 10.0f}, 6.5f, pad_edge);
        DrawCircleV({cx, cy - 10.0f}, 3.2f, RAYWHITE);

        DrawTextShadow("RISONATORE FOTONICO", static_cast<int>(cx) - 70, static_cast<int>(cy) - 52, 12, SKYBLUE);
    } else {
        // CAMPO SOLARE (Ovest): pannello fotovoltaico a celle + normale + gnomone empirico
        const float pan_rad = eco::deg2rad(active_angle);
        const Vector2 n_dir{std::cos(pan_rad), std::sin(pan_rad)};
        const Vector2 t_dir{-n_dir.y, n_dir.x};
        const Vector2 p1{cx + t_dir.x * 26.0f, cy + t_dir.y * 26.0f};
        const Vector2 p2{cx - t_dir.x * 26.0f, cy - t_dir.y * 26.0f};

        // Supporto meccanico centrale
        DrawRectangle(static_cast<int>(cx - 5.0f), static_cast<int>(cy - 2.0f), 10, 12, Color{58, 72, 86, 255});

        // Superficie del pannello solare con cornice metallica e celle blu
        DrawLineEx(p1, p2, 9.0f, Color{24, 58, 96, 255});
        DrawLineEx(p1, p2, 6.0f, Color{38, 138, 218, 255});
        DrawLineEx(p1, p2, 2.0f, Color{155, 232, 255, 255});

        // Freccia normale del pannello
        DrawArrow2D({cx, cy}, {cx + n_dir.x * 27.0f, cy + n_dir.y * 27.0f}, 2.5f, pad_edge);
        DrawCircleV({cx, cy}, 5.5f, pad_edge);

        // Gnomone / Meridiana empirica a fianco (a cx - 64, cy + 12)
        const float gx = cx - 64.0f;
        const float gy = cy + 12.0f;
        DrawEllipse(static_cast<int>(gx + 2.0f), static_cast<int>(gy + 6.0f), 17.0f, 8.0f, Color{0, 0, 0, 95});
        DrawCircle(static_cast<int>(gx), static_cast<int>(gy), 16.0f, Color{24, 36, 48, 240});
        DrawCircleLines(static_cast<int>(gx), static_cast<int>(gy), 16.0f, Color{130, 185, 215, 215});
        // Tacche graduate della meridiana
        for (int a = 0; a < 8; ++a) {
            const float rad = static_cast<float>(a) * (eco::kPi * 0.25f);
            DrawLine(
                static_cast<int>(gx + std::cos(rad) * 12.0f),
                static_cast<int>(gy + std::sin(rad) * 12.0f),
                static_cast<int>(gx + std::cos(rad) * 15.5f),
                static_cast<int>(gy + std::sin(rad) * 15.5f),
                Color{150, 195, 220, 160});
        }
        DrawLineEx(
            {gx, gy},
            {gx + sun_dir.x * 15.0f, gy + sun_dir.y * 15.0f},
            2.4f,
            Color{255, 228, 95, 240});
        DrawCircleV({gx, gy}, 3.5f, RAYWHITE);
        DrawTextShadow("GNOMONE", static_cast<int>(gx) - 26, static_cast<int>(gy) + 19, 10, LIGHTGRAY);
        DrawTextShadow("CAMPO SOLARE", static_cast<int>(cx) - 46, static_cast<int>(cy) - 50, 12, RAYWHITE);
    }

    // Generatore diesel d'emergenza e pennacchio di fumo nero se in stato BypassedDirty
    if (m.state == eco::MachineState::BypassedDirty) {
        DrawRectangle(static_cast<int>(cx + 20.0f), static_cast<int>(cy + 6.0f), 16, 14, Color{92, 52, 32, 255});
        DrawRectangleLines(static_cast<int>(cx + 20.0f), static_cast<int>(cy + 6.0f), 16, 14, ORANGE);
        for (int s = 0; s < 5; ++s) {
            const float age = std::fmod(world.sim_time * 1.5f + static_cast<float>(s) * 0.4f, 2.0f);
            const float sy = cy - 6.0f - age * 30.0f;
            const float sx = cx + 26.0f + std::sin(world.sim_time * 3.2f + static_cast<float>(s)) * 9.0f;
            DrawCircleV(
                {sx, sy},
                5.5f + age * 5.2f,
                Color{72, 66, 60, static_cast<unsigned char>(190 - age * 85)});
        }
    }

    if (m.prompt_visible && !world.calibration_modal.is_open) {
        const int pw = 316;
        const int px = static_cast<int>(cx) - pw / 2;
        const int py = static_cast<int>(cy) + 38;
        DrawPanel(Rectangle{static_cast<float>(px), static_cast<float>(py), static_cast<float>(pw), 26.0f}, YELLOW);
        DrawTextShadow(eco::kMachineInteractPrompt, px + 10, py + 6, 12, YELLOW);
    }
}

void DrawHydroTurbineApparatus(const eco::Machine& m, const eco::WorldContext& world) {
    const float cx = m.pos.x;
    const float cy = m.pos.y;
    const float active_head =
        (world.calibration_modal.is_open &&
          world.calibration_modal.target_machine == eco::MachineType::HydroTurbine)
            ? world.calibration_modal.slider_value
            : m.current_param;
    const float eff = m.EvaluateEfficiency(active_head);

    // Serbatoio piezometrico graduato a sinistra della turbina
    const float tank_x = cx - 138.0f;
    const float tank_bottom = cy + 20.0f;
    const float tank_h = 184.0f;
    const float tank_w = 56.0f;
    const float tank_top = tank_bottom - tank_h;

    DrawRectangle(
        static_cast<int>(tank_x - 3.0f),
        static_cast<int>(tank_top - 4.0f),
        static_cast<int>(tank_w + 6.0f),
        static_cast<int>(tank_h + 8.0f),
        Color{12, 20, 30, 240});

    // Colonna d'acqua proporzionale al battente idrostatico h (0..15m)
    const float fill_ratio = eco::Clamp(active_head / 15.0f, 0.05f, 1.0f);
    const float water_h = tank_h * fill_ratio;
    const float water_top = tank_bottom - water_h;
    DrawRectangleGradientV(
        static_cast<int>(tank_x + 3.0f),
        static_cast<int>(water_top),
        static_cast<int>(tank_w - 6.0f),
        static_cast<int>(water_h),
        Color{65, 205, 250, 225},
        Color{22, 118, 185, 235});
    // Menisco superiore increspato
    DrawLineEx(
        {tank_x + 3.0f, water_top},
        {tank_x + tank_w - 3.0f, water_top},
        2.5f,
        Color{175, 245, 255, 245});

    // Indicatore del battente ottimale (target_param = 10.0m)
    const float opt_ratio = eco::Clamp(m.target_param / 15.0f, 0.0f, 1.0f);
    const float opt_y = tank_bottom - tank_h * opt_ratio;
    DrawLineEx({tank_x - 8.0f, opt_y}, {tank_x + tank_w + 8.0f, opt_y}, 2.5f, pal::kAccentClean);
    DrawTextShadow("h_opt", static_cast<int>(tank_x - 42.0f), static_cast<int>(opt_y - 6.0f), 12, pal::kAccentClean);

    // Tacche graduate ogni 3 metri
    for (int hm = 3; hm <= 15; hm += 3) {
        const float ty = tank_bottom - tank_h * (static_cast<float>(hm) / 15.0f);
        DrawLine(
            static_cast<int>(tank_x),
            static_cast<int>(ty),
            static_cast<int>(tank_x + 11.0f),
            static_cast<int>(ty),
            RAYWHITE);
    }
    DrawRectangleLinesEx(
        Rectangle{tank_x, tank_top, tank_w, tank_h},
        2.0f,
        Color{115, 190, 235, 255});

    // Condotta forzata bullonata dal fondo del serbatoio alla chiocciola della turbina
    DrawLineEx({tank_x + tank_w, tank_bottom - 12.0f}, {cx - 28.0f, cy}, 12.0f, Color{34, 52, 72, 255});
    DrawLineEx({tank_x + tank_w, tank_bottom - 12.0f}, {cx - 28.0f, cy}, 8.0f, Color{62, 95, 128, 255});
    for (int wp = 0; wp < 4; ++wp) {
        const float t = std::fmod(world.sim_time * (0.8f + eff * 1.8f) + static_cast<float>(wp) * 0.25f, 1.0f);
        const Vector2 drop{
            eco::Lerp(tank_x + tank_w, cx - 28.0f, t),
            eco::Lerp(tank_bottom - 12.0f, cy, t)
        };
        DrawCircleV(drop, 3.6f, Color{130, 235, 255, 245});
    }

    // Cassa a chiocciola della Turbina & Rotore Pelton/Francis a 8 pale
    const Color ring_col = (m.state == eco::MachineState::CleanActive)
                               ? pal::kAccentClean
                               : (m.state == eco::MachineState::BypassedDirty
                                      ? pal::kAccentDirty
                                      : Color{105, 180, 235, 255});
    DrawCircle(static_cast<int>(cx), static_cast<int>(cy), 33.0f, Color{20, 30, 44, 255});
    DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), 33.0f, ring_col);
    DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), 27.0f, pal::WithAlpha(ring_col, 120));

    const float spin_speed = (m.state == eco::MachineState::CleanActive)
                                 ? 6.0f
                                 : (0.6f + eff * 3.8f);
    const float base_ang = world.sim_time * spin_speed;
    for (int b = 0; b < 8; ++b) {
        const float a = base_ang + static_cast<float>(b) * (eco::kPi * 0.25f);
        const Vector2 tip{cx + std::cos(a) * 25.0f, cy + std::sin(a) * 25.0f};
        DrawLineEx({cx, cy}, tip, 3.5f, ring_col);
        DrawCircleV(tip, 2.5f, RAYWHITE);
    }
    DrawCircleV({cx, cy}, 8.5f, Color{42, 62, 82, 255});
    DrawCircleV({cx, cy}, 5.0f, RAYWHITE);
    DrawTextShadow("TURBINA IDROELETTRICA", static_cast<int>(cx - 75.0f), static_cast<int>(cy - 52.0f), 12, RAYWHITE);

    if (m.state == eco::MachineState::BypassedDirty) {
        for (int s = 0; s < 4; ++s) {
            const float age = std::fmod(world.sim_time * 1.5f + static_cast<float>(s) * 0.5f, 2.0f);
            const float sy = cy - 18.0f - age * 28.0f;
            const float sx = cx + 18.0f + std::sin(world.sim_time * 3.0f + static_cast<float>(s)) * 8.0f;
            DrawCircleV({sx, sy}, 5.5f + age * 4.5f, Color{80, 74, 68, static_cast<unsigned char>(180 - age * 80)});
        }
    }

    if (m.prompt_visible && !world.calibration_modal.is_open) {
        const int pw = 316;
        const int px = static_cast<int>(cx) - pw / 2;
        const int py = static_cast<int>(cy) - 78;
        DrawPanel(Rectangle{static_cast<float>(px), static_cast<float>(py), static_cast<float>(pw), 26.0f}, YELLOW);
        DrawTextShadow(eco::kMachineInteractPrompt, px + 10, py + 6, 12, YELLOW);
    }
}

void DrawLiaArchitectNpc(const eco::WorldContext& world) {
    const float nx = world.architect_npc.pos.x;
    const float ny = world.architect_npc.pos.y;
    const bool rec = world.architect_npc.recruited;
    const Color arch_col = rec ? pal::kAccentClean : pal::kAccentWarn;
    const float bob = std::sin(world.sim_time * 2.8f) * 1.2f;

    // Ombra a terra
    DrawEllipse(static_cast<int>(nx), static_cast<int>(ny + 16.0f), 13.0f, 5.0f, Color{0, 0, 0, 125});

    // Stivali da cantiere
    DrawRectangle(static_cast<int>(nx - 6.0f), static_cast<int>(ny + 9.0f), 4, 7, Color{42, 36, 32, 255});
    DrawRectangle(static_cast<int>(nx + 2.0f), static_cast<int>(ny + 9.0f), 4, 7, Color{42, 36, 32, 255});

    // Tubo portaprogetti / rotolo di blueprint a tracolla sulla schiena
    DrawLineEx({nx - 10.0f, ny - 5.0f + bob}, {nx + 9.0f, ny + 8.0f + bob}, 4.5f, Color{185, 130, 62, 255});
    DrawCircleV({nx - 10.0f, ny - 5.0f + bob}, 3.0f, Color{230, 215, 180, 255});

    // Giacca tecnica da ingegnera strutturista con bande catarifrangenti
    DrawRectangle(static_cast<int>(nx - 8.0f), static_cast<int>(ny - 5.0f + bob), 16, 15, Color{28, 48, 68, 255});
    DrawLine(
        static_cast<int>(nx - 7.0f),
        static_cast<int>(ny + 3.0f + bob),
        static_cast<int>(nx + 7.0f),
        static_cast<int>(ny + 3.0f + bob),
        arch_col);
    DrawRectangleLines(static_cast<int>(nx - 8.0f), static_cast<int>(ny - 5.0f + bob), 16, 15, arch_col);

    // Viso, capelli raccolti e casco giallo da cantiere (elmetto Unical)
    const float head_y = ny - 11.0f + bob;
    DrawCircle(static_cast<int>(nx), static_cast<int>(head_y), 7.0f, Color{232, 198, 172, 255});
    // Casco protettivo giallo/ambra
    DrawCircleSector(
        {nx, head_y - 1.0f},
        8.2f,
        180.0f,
        360.0f,
        12,
        Color{245, 190, 42, 255});
    DrawLineEx({nx - 9.0f, head_y - 1.0f}, {nx + 9.0f, head_y - 1.0f}, 2.2f, Color{255, 215, 70, 255});
    // Occhiali tecnici
    DrawRectangle(static_cast<int>(nx - 5.0f), static_cast<int>(head_y - 1.0f), 10, 3, Color{40, 210, 240, 235});

    // Indicatore flottante sopra la testa: "!" quando attende una prova matematica, cubo olografico quando reclutata
    const float h_bob = std::sin(world.sim_time * 3.8f) * 3.0f;
    if (!rec) {
        const Vector2 badge_c{nx, ny - 31.0f + h_bob};
        DrawCircleV(badge_c, 9.0f, Color{24, 32, 44, 235});
        DrawCircleLines(static_cast<int>(badge_c.x), static_cast<int>(badge_c.y), 9.0f, arch_col);
        DrawTextShadow("!", static_cast<int>(badge_c.x - 2.0f), static_cast<int>(badge_c.y - 6.0f), 14, arch_col);
    } else {
        DrawPolyLines({nx, ny - 30.0f + h_bob}, 6, 8.5f, world.sim_time * 75.0f, arch_col);
    }
    DrawTextShadow("LIA (ARCHITETTA)", static_cast<int>(nx - 52.0f), static_cast<int>(ny - 50.0f), 11, arch_col);

    if (world.architect_npc.AnyActivePlayerInRange(world.players) &&
        !world.calibration_modal.is_open) {
        const int pw = 328;
        const int px = static_cast<int>(nx) - pw / 2;
        const int py = static_cast<int>(ny) + 22;
        DrawPanel(Rectangle{static_cast<float>(px), static_cast<float>(py), static_cast<float>(pw), 25.0f}, arch_col);
        DrawTextShadow(
            rec
                ? "[E] Lia: Ponte Bucci e Telemetria Matrice attivi"
                : "[E] Parla con Lia (Richiede prova Energia Pulita)",
            px + 10,
            py + 6,
            12,
            arch_col);
    }
}

} // namespace

void DrawScientistProtagonist(const eco::Player& p, const eco::WorldContext& world) {
    const float cx = p.pos.x + p.size.x * 0.5f;
    const float cy = p.pos.y + p.size.y * 0.5f;
    const float speed = std::hypot(p.vel.x, (world.current_mode == eco::GameMode::Structure) ? p.y_velocity : p.vel.y);
    const float walk_bob = (speed > 5.0f) ? std::sin(world.sim_time * 15.0f + static_cast<float>(p.player_id)) * 2.2f : 0.0f;
    const float stride = (speed > 5.0f) ? std::sin(world.sim_time * 15.0f + static_cast<float>(p.player_id)) * 5.2f : 0.0f;
    const float dir_x = (std::fabs(p.vel.x) > 1.0f) ? (p.vel.x > 0.0f ? 1.0f : -1.0f) : 1.0f;
    const Color accent = pal::PlayerAccentColor(p.player_id);

    // Onda radar empirica quando il giocatore attiva lo Scanner [F/Q]
    if (p.player_id == 0 && world.scan_pulse_timer > 0.0f) {
        const float progress = 1.0f - (world.scan_pulse_timer / 0.85f);
        const float r1 = progress * eco::kDefaultScannerRange;
        const auto alpha = static_cast<unsigned char>((1.0f - progress) * 230.0f);
        DrawCircleLines(
            static_cast<int>(world.scan_pulse_origin.x),
            static_cast<int>(world.scan_pulse_origin.y),
            r1,
            Color{80, 245, 255, alpha});
        DrawCircleLines(
            static_cast<int>(world.scan_pulse_origin.x),
            static_cast<int>(world.scan_pulse_origin.y),
            std::max(4.0f, r1 * 0.65f),
            Color{120, 255, 200, alpha});
        if (world.last_scan.has_target) {
            const eco::Machine& target_m =
                (world.current_mode == eco::GameMode::Overworld &&
                 world.ecosystem.architect_recruited &&
                 std::fabs(world.last_scan.measured_target_value - world.purifier_machine.target_param) < 1e-2f)
                    ? world.purifier_machine
                    : world.ActiveMachine();
            DrawLineEx(
                {cx, cy},
                {target_m.pos.x, target_m.pos.y},
                2.2f,
                Color{90, 255, 230, alpha});
        }
    }

    // Ombra morbida a terra
    DrawEllipse(
        static_cast<int>(cx),
        static_cast<int>(p.pos.y + p.size.y + 1.5f),
        13.0f,
        4.8f,
        Color{0, 0, 0, 130});

    // Gambe e stivali animati
    DrawLineEx(
        {cx - 4.5f, cy + 5.0f},
        {cx - 4.5f - stride, p.pos.y + p.size.y},
        4.2f,
        Color{34, 46, 60, 255});
    DrawLineEx(
        {cx + 4.5f, cy + 5.0f},
        {cx + 4.5f + stride, p.pos.y + p.size.y},
        4.2f,
        Color{34, 46, 60, 255});

    // Zaino telemetrico con antenna e LED stato UI
    const float pack_x = (dir_x >= 0.0f) ? (cx - 13.0f) : (cx + 6.0f);
    DrawRectangle(
        static_cast<int>(pack_x),
        static_cast<int>(cy - 5.0f + walk_bob),
        7,
        13,
        Color{40, 58, 76, 255});
    DrawLineEx(
        {pack_x + 3.5f, cy - 5.0f + walk_bob},
        {pack_x + 3.5f, cy - 12.0f + walk_bob},
        1.5f,
        LIGHTGRAY);
    const Color led_col = (world.ecosystem.ui_phase == eco::UiPhase::Phase3_EngineOverlay)
                              ? pal::kAccentClean
                              : (world.ecosystem.ui_phase == eco::UiPhase::Phase2_Telemetry
                                     ? SKYBLUE
                                     : ORANGE);
    DrawCircle(static_cast<int>(pack_x + 3.5f), static_cast<int>(cy + walk_bob), 2.5f, led_col);

    // Camice da scienziata (Ada Ferri / squadra) con sciarpa/bordo del colore giocatore
    const Color coat_col = Color{234, 242, 250, 255};
    DrawRectangle(
        static_cast<int>(cx - 8.0f),
        static_cast<int>(cy - 6.0f + walk_bob),
        16,
        15,
        coat_col);
    // Sciarpa / colletto distintivo del player_id
    DrawRectangle(
        static_cast<int>(cx - 8.0f),
        static_cast<int>(cy - 6.0f + walk_bob),
        16,
        4,
        accent);
    DrawRectangleLines(
        static_cast<int>(cx - 8.0f),
        static_cast<int>(cy - 6.0f + walk_bob),
        16,
        15,
        pal::ShiftBrightness(accent, -25));

    // Braccio con strumento scanner
    const float arm_x = (dir_x >= 0.0f) ? (cx + 4.0f) : (cx - 8.0f);
    DrawLineEx(
        {arm_x, cy - 1.0f + walk_bob},
        {arm_x + dir_x * 5.0f, cy + 4.0f + walk_bob - stride * 0.3f},
        3.2f,
        coat_col);

    // Testa, capelli castano/ramati e visore scientifico
    const float head_y = cy - 11.5f + walk_bob;
    DrawCircle(static_cast<int>(cx), static_cast<int>(head_y), 7.5f, Color{240, 212, 190, 255});
    // Capelli (caschetto con ciocca posteriore)
    DrawCircleSector(
        {cx, head_y - 0.5f},
        8.2f,
        165.0f,
        375.0f,
        12,
        Color{76, 48, 38, 255});
    const float hair_back_x = (dir_x >= 0.0f) ? (cx - 7.5f) : (cx + 7.5f);
    DrawCircleV({hair_back_x, head_y + 1.5f}, 3.5f, Color{76, 48, 38, 255});

    // Occhiali/visore olografico orientato nella direzione dello sguardo
    const float visor_x = (dir_x >= 0.0f) ? (cx - 1.5f) : (cx - 7.5f);
    DrawRectangle(
        static_cast<int>(visor_x),
        static_cast<int>(head_y - 2.0f),
        9,
        4,
        accent);
    DrawRectangleLines(
        static_cast<int>(visor_x),
        static_cast<int>(head_y - 2.0f),
        9,
        4,
         Color{20, 34, 48, 255});

    // Badge P2..P4 se ci sono piu' giocatori attivi in co-op locale
    if (p.player_id > 0) {
        DrawTextShadow(
            TextFormat("P%d", p.player_id + 1),
            static_cast<int>(cx - 7.0f),
            static_cast<int>(head_y - 22.0f),
            11,
            accent);
    }
}

void DrawSceneMachineAndOverlaysWorldSpace(const eco::WorldContext& world) {
    const eco::OverlayVisibility ov = eco::ComputeOverlayVisibility(world.ecosystem);

    // Fase 3: Griglia cartesiana della Matrice di GAIA
    if (ov.show_aabb_hitboxes || ov.show_velocity_vectors) {
        const eco::Tilemap& tm = world.ActiveTilemap();
        const int max_x = static_cast<int>(tm.WorldWidth());
        const int max_y = static_cast<int>(tm.WorldHeight());
        for (int gx = 0; gx <= max_x; gx += 96) {
            DrawLine(gx, 0, gx, max_y, Color{50, 255, 160, 38});
            DrawTextShadow(TextFormat("x=%d", gx), gx + 3, 52, 10, Color{80, 255, 175, 130});
        }
        for (int gy = 0; gy <= max_y; gy += 96) {
            DrawLine(0, gy, max_x, gy, Color{50, 255, 160, 38});
            DrawTextShadow(TextFormat("y=%d", gy), 52, gy + 3, 10, Color{80, 255, 175, 130});
        }
    }

    if (world.current_mode == eco::GameMode::Overworld) {
        // Condotte energetiche tra gli impianti e il Ponte Bucci
        const Vector2 solar_v{world.solar_machine.pos.x, world.solar_machine.pos.y};
        const Vector2 npc_v{world.architect_npc.pos.x, world.architect_npc.pos.y};
        const Vector2 portal_v{15.5f * eco::kTileSize, 4.5f * eco::kTileSize};
        const Vector2 bridge_v{20.5f * eco::kTileSize, 8.5f * eco::kTileSize};
        const Vector2 purifier_v{world.purifier_machine.pos.x, world.purifier_machine.pos.y};

        DrawEnergyConduit(solar_v, npc_v, world.solar_machine.state, world.sim_time, world.ecosystem);
        DrawEnergyConduit(portal_v, npc_v, world.hydro_machine.state, world.sim_time, world.ecosystem);
        const eco::MachineState bridge_state =
            world.ecosystem.architect_recruited ? eco::MachineState::CleanActive : eco::MachineState::Broken;
        DrawEnergyConduit(npc_v, bridge_v, bridge_state, world.sim_time, world.ecosystem);
        if (world.ecosystem.architect_recruited) {
            DrawEnergyConduit(bridge_v, purifier_v, world.purifier_machine.state, world.sim_time, world.ecosystem);
        }

        // Impianto Ovest: Campo Solare
        DrawSolarArrayApparatus(world.solar_machine, world, false);

        // Impianto Est (oltre il Ponte Bucci): Torre del Risonatore Fotonico
        DrawSolarArrayApparatus(world.purifier_machine, world, true);

        // Lia Moretti, l'Architetta
        DrawLiaArchitectNpc(world);
    } else {
        // Diga del Crati: Turbina Idroelettrica e serbatoio graduato
        DrawHydroTurbineApparatus(world.hydro_machine, world);
    }

    const eco::Machine& active_m = world.ActiveMachine();

    // Fase 3: Raggi di trigger euclidei
    if (ov.show_trigger_radii) {
        DrawCircleLines(
            static_cast<int>(active_m.pos.x),
            static_cast<int>(active_m.pos.y),
            active_m.trigger_radius,
            Color{80, 235, 255, 210});
        if (world.current_mode == eco::GameMode::Overworld) {
            DrawCircleLines(
                static_cast<int>(world.architect_npc.pos.x),
                static_cast<int>(world.architect_npc.pos.y),
                world.architect_npc.trigger_radius,
                Color{255, 210, 90, 210});
            DrawCircleLines(
                static_cast<int>(world.purifier_machine.pos.x),
                static_cast<int>(world.purifier_machine.pos.y),
                world.purifier_machine.trigger_radius,
                Color{120, 255, 220, 210});
        }
    }

    // Fase 3: Vettori di irraggiamento e normale
    if (ov.show_irradiance_rays) {
        const float sun_rad = eco::deg2rad(active_m.target_param);
        const float pan_rad = eco::deg2rad(active_m.current_param);
        const Vector2 center{active_m.pos.x, active_m.pos.y};
        DrawArrow2D(
            center,
            {center.x + std::cos(sun_rad) * 65.0f, center.y + std::sin(sun_rad) * 65.0f},
            2.5f,
            YELLOW);
        DrawArrow2D(
            center,
            {center.x + std::cos(pan_rad) * 52.0f, center.y + std::sin(pan_rad) * 52.0f},
            2.5f,
            SKYBLUE);
    }

    // Fase 3: AABB Hitbox e Vettori Velocita'/Gravita' per tutti i giocatori attivi
    for (const eco::Player* ap : world.players.ActivePlayers()) {
        if (ap == nullptr || !ap->active) {
            continue;
        }
        if (ov.show_aabb_hitboxes) {
            DrawRectangleLinesEx(
                Rectangle{ap->pos.x, ap->pos.y, ap->size.x, ap->size.y},
                2.0f,
                Color{60, 255, 140, 255});
            DrawTextShadow(
                TextFormat("AABB(%.0f,%.0f)", ap->pos.x, ap->pos.y),
                static_cast<int>(ap->pos.x - 18.0f),
                static_cast<int>(ap->pos.y - 24.0f),
                11,
                Color{80, 255, 155, 255});
        }
        if (ov.show_velocity_vectors) {
            const eco::Vec2 c = ap->Center();
            const float vy = (world.current_mode == eco::GameMode::Structure)
                                 ? ap->y_velocity
                                 : ap->vel.y;
            const Vector2 start{c.x, c.y};
            const Vector2 tip{c.x + ap->vel.x * 0.28f, c.y + vy * 0.28f};
            DrawArrow2D(start, tip, 2.5f, MAGENTA);
            if (world.current_mode == eco::GameMode::Structure && !ap->isGrounded) {
                DrawArrow2D(start, {c.x, c.y + 34.0f}, 2.0f, ORANGE);
                DrawTextShadow("g", static_cast<int>(c.x + 6.0f), static_cast<int>(c.y + 24.0f), 12, ORANGE);
            }
            if (ap->player_id == 0) {
                const float dist = active_m.DistanceToPlayer(*ap);
                DrawLineEx(start, {active_m.pos.x, active_m.pos.y}, 1.2f, Color{90, 220, 255, 110});
                DrawTextShadow(
                    TextFormat("d=%.0fpx", dist),
                    static_cast<int>((c.x + active_m.pos.x) * 0.5f),
                    static_cast<int>((c.y + active_m.pos.y) * 0.5f),
                    11,
                    SKYBLUE);
            }
        }
    }
}

void DrawHudAndCalibrationModalScreenSpace(eco::WorldContext& world) {
    const eco::OverlayVisibility ov = eco::ComputeOverlayVisibility(world.ecosystem);
    const eco::Machine& machine = world.ModalTargetMachine();

    // 1. Barra Superiore Compatta (y = 0..52)
    DrawRectangle(0, 0, eco::kWindowWidth, 52, Color{10, 16, 24, 238});
    DrawLine(0, 52, eco::kWindowWidth, 52, pal::kUiBorderCyan);

    const char* phase_title = "FASE 1: REALISMO EMPIRICO";
    Color phase_col = ORANGE;
    if (ov.phase == eco::UiPhase::Phase2_Telemetry) {
        phase_title = "FASE 2: TELEMETRIA NUMERICA [V]";
        phase_col = SKYBLUE;
    } else if (ov.phase == eco::UiPhase::Phase3_EngineOverlay) {
        phase_title = "FASE 3: CODICE DELLA MATRICE [V]";
        phase_col = pal::kAccentClean;
    }

    DrawTextShadow(
        world.current_mode == eco::GameMode::Overworld
            ? "VALLE DEL CRATI // CAMPUS OSSERVATORIO"
            : "DIGA DEL CRATI // CAMERA IDROSTATICA",
        16,
        8,
        15,
        RAYWHITE);
    DrawTextShadow(phase_title, 16, 29, 13, phase_col);

    // Indicatori Smog Atmosferico & Energia Pulita al centro della barra superiore
    const int smog_x = 385;
    const Color smog_col = world.ecosystem.pollution_level > 35.0f ? pal::kAccentDirty : pal::kAccentClean;
    DrawTextShadow(
        TextFormat("SMOG: %.0f%%", world.ecosystem.pollution_level),
        smog_x,
        8,
        13,
        smog_col);
    DrawProgressBar(
        Rectangle{static_cast<float>(smog_x), 26.0f, 165.0f, 14.0f},
        world.ecosystem.PollutionRatio(),
        smog_col,
        Color{90, 135, 168, 210});

    const int en_x = 572;
    DrawTextShadow(
        TextFormat("ENERGIA PULITA: %.0f MW", world.ecosystem.clean_energy),
        en_x,
        8,
        13,
        pal::kAccentClean);
    DrawProgressBar(
        Rectangle{static_cast<float>(en_x), 26.0f, 165.0f, 14.0f},
        eco::Clamp(world.ecosystem.clean_energy / 100.0f, 0.0f, 1.0f),
        pal::kAccentClean,
        Color{90, 135, 168, 210});

    // Tasti rapidi sintetici a destra nella barra superiore
    DrawTextShadow(
        "[WASD] Muovi  [F] Scan  [E] Azione  [V] Fase UI  [N] Dialogo  [ESC] Menu",
        762,
        18,
        13,
        LIGHTGRAY);

    // 2. Pannello Scanner Empirico & Telemetria (in ALTO A DESTRA x = kWindowWidth - 434, y = 62,
    // cosi' l'angolo alto-sinistra resta interamente libero per DrawStoryLayer!)
    if (ov.show_raw_numbers || world.last_scan.active) {
        const float px = static_cast<float>(eco::kWindowWidth - 434);
        const float py = 62.0f;
        const float pw = 418.0f;
        const float ph = ov.show_raw_numbers ? 82.0f : 54.0f;
        DrawPanel(Rectangle{px, py, pw, ph}, pal::kUiBorderCyan);

        if (world.last_scan.active) {
            DrawTextShadow(
                world.last_scan.has_target
                    ? TextFormat(
                          "SCANNER [F]: Target = %.2f %s (Dist: %.0f px)",
                          world.last_scan.measured_target_value,
                          world.last_scan.target_type == eco::MachineType::SolarArray ? "deg" : "m",
                          world.last_scan.measured_distance)
                    : "SCANNER [F]: Nessun impianto nel raggio (avvicinati)",
                static_cast<int>(px) + 12,
                static_cast<int>(py) + 10,
                13,
                YELLOW);
        } else {
            DrawTextShadow(
                "SCANNER [F]: Premi vicino a un impianto per misurarlo",
                static_cast<int>(px) + 12,
                static_cast<int>(py) + 10,
                13,
                LIGHTGRAY);
        }

        if (ov.show_raw_numbers) {
            const eco::Player* p0 = world.players.GetPlayer(0);
            const float vx = p0 ? p0->vel.x : 0.0f;
            const float vy = p0 ? ((world.current_mode == eco::GameMode::Structure) ? p0->y_velocity : p0->vel.y) : 0.0f;
            DrawTextShadow(
                TextFormat(
                    "TELEMETRIA: v=(%.0f, %.0f) | Eff=%.1f%% | Sporca=%.0f MW",
                    vx,
                    vy,
                    world.ActiveMachine().LiveEfficiency() * 100.0f,
                    world.ecosystem.dirty_energy),
                static_cast<int>(px) + 12,
                static_cast<int>(py) + 32,
                12,
                SKYBLUE);
            DrawTextShadow(
                TextFormat(
                    "MATRICE: Fase %d/%d | Ponte Bucci: %s",
                    static_cast<int>(ov.phase),
                    static_cast<int>(world.ecosystem.MaxUnlockedUiPhase()),
                    world.ecosystem.architect_recruited ? "ATTIVO" : "CROLLATO"),
                static_cast<int>(px) + 12,
                static_cast<int>(py) + 54,
                12,
                pal::kAccentClean);
        } else {
            DrawTextShadow(
                "Calibra senza inquinare per sbloccare la Telemetria [V]",
                static_cast<int>(px) + 12,
                static_cast<int>(py) + 30,
                12,
                ORANGE);
        }
    }

    // 3. Console Modale di Calibrazione Fisica (Tastiera + Mouse invariati)
    if (world.calibration_modal.is_open) {
        DrawRectangle(0, 0, eco::kWindowWidth, eco::kWindowHeight, Color{4, 10, 18, 190});
        const int box_w = 700;
        const int box_h = 370;
        const int bx = (eco::kWindowWidth - box_w) / 2;
        const int by = (eco::kWindowHeight - box_h) / 2;

        const bool is_solar = (world.calibration_modal.target_machine == eco::MachineType::SolarArray);
        const char* modal_title =
            world.calibrating_purifier
                ? "RISONATORE FOTONICO // ARMONICA DI FASE: E = max(0, cos(dTheta)) > 80%"
                : (is_solar
                       ? "CAMPO SOLARE // LEGGE DI LAMBERT: E = max(0, cos(Theta_p - Theta_s)) > 80%"
                       : "TURBINA IDROELETTRICA // LEGGE DI STEVINO: P = rho * g * h > 80%");

        DrawPanelWithHeader(
            Rectangle{static_cast<float>(bx), static_cast<float>(by), static_cast<float>(box_w), static_cast<float>(box_h)},
            40,
            modal_title,
            SKYBLUE);

        const bool target_revealed =
            ov.show_raw_numbers ||
            (world.last_scan.has_target &&
             std::fabs(world.last_scan.measured_target_value - machine.target_param) < 1e-2f);

        if (target_revealed) {
            DrawTextShadow(
                TextFormat(
                    "Misura Empirica Rilevata: Target Ottimale = %.2f %s",
                    machine.target_param,
                    is_solar ? "deg" : "m (98.1 kPa)"),
                bx + 24,
                by + 56,
                16,
                pal::kAccentClean);
        } else {
            DrawTextShadow(
                "FASE 1 (Nessun dato pre-calcolato): Allinea visivamente l'indicatore o usa lo Scanner [F]!",
                bx + 24,
                by + 56,
                14,
                ORANGE);
        }

        // Slider interattivo (Trascinamento Mouse + tasti A/D / Frecce)
        const float min_param = is_solar ? -90.0f : 0.0f;
        const float max_param = is_solar ? 180.0f : 15.0f;
        const Rectangle slider_rect{
            static_cast<float>(bx + 24),
            static_cast<float>(by + 114),
            440.0f,
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
                "Parametro Impostato [A/D, Frecce o Mouse]: %.2f %s",
                world.calibration_modal.slider_value,
                is_solar ? "deg" : "m"),
            bx + 24,
            by + 88,
            16,
            RAYWHITE);

        const float norm_slider =
            eco::Clamp((world.calibration_modal.slider_value - min_param) / (max_param - min_param), 0.0f, 1.0f);
        DrawProgressBar(
            slider_rect,
            norm_slider,
            world.calibration_modal.can_confirm_clean ? pal::kAccentClean : Color{60, 155, 230, 230},
            SKYBLUE);

        // Tacca del target sullo slider
        const float norm_target =
            eco::Clamp((machine.target_param - min_param) / (max_param - min_param), 0.0f, 1.0f);
        const int target_px = static_cast<int>(slider_rect.x + slider_rect.width * norm_target);
        DrawLine(
            target_px,
            static_cast<int>(slider_rect.y - 6),
            target_px,
            static_cast<int>(slider_rect.y + slider_rect.height + 6),
            YELLOW);

        // Diagramma fisico in tempo reale a destra nel modal
        const Vector2 diag_c{static_cast<float>(bx + 585), static_cast<float>(by + 135)};
        DrawCircleV(diag_c, 56.0f, Color{12, 20, 30, 255});
        DrawCircleLines(static_cast<int>(diag_c.x), static_cast<int>(diag_c.y), 56.0f, SKYBLUE);
        if (is_solar) {
            const float s_rad = eco::deg2rad(machine.target_param);
            const float p_rad = eco::deg2rad(world.calibration_modal.slider_value);
            DrawArrow2D(
                diag_c,
                {diag_c.x + std::cos(s_rad) * 48.0f, diag_c.y + std::sin(s_rad) * 48.0f},
                2.5f,
                YELLOW);
            DrawArrow2D(
                diag_c,
                {diag_c.x + std::cos(p_rad) * 44.0f, diag_c.y + std::sin(p_rad) * 44.0f},
                3.0f,
                world.calibration_modal.can_confirm_clean ? pal::kAccentClean : SKYBLUE);
            DrawTextShadow("Giallo: Raggio", bx + 535, by + 198, 12, YELLOW);
            DrawTextShadow("Ciano: Normale", bx + 535, by + 214, 12, SKYBLUE);
        } else {
            const float p_kpa =
                (eco::kWaterDensity * eco::kGravityWater * world.calibration_modal.slider_value) / 1000.0f;
            const float opt_kpa =
                (eco::kWaterDensity * eco::kGravityWater * machine.target_param) / 1000.0f;
            DrawTextShadow(TextFormat("P = %.1f kPa", p_kpa), bx + 542, by + 118, 14, SKYBLUE);
            DrawTextShadow(TextFormat("Opt: %.1f kPa", opt_kpa), bx + 542, by + 140, 13, pal::kAccentClean);
        }

        // Barra di Rendimento
        const float eff_pct = world.calibration_modal.preview_efficiency * 100.0f;
        const Color eff_col = world.calibration_modal.can_confirm_clean ? pal::kAccentClean : ORANGE;
        DrawTextShadow(
            TextFormat("Rendimento Fisico Calcolato: %.1f%% (Minimo richiesto: > 80.0%%)", eff_pct),
            bx + 24,
            by + 156,
            17,
            eff_col);
        DrawProgressBar(
            Rectangle{static_cast<float>(bx + 24), static_cast<float>(by + 182), 440.0f, 20.0f},
            world.calibration_modal.preview_efficiency,
            eff_col,
            RAYWHITE);

        // Pulsanti d'azione (Tastiera + Mouse)
        const Rectangle btn_confirm{static_cast<float>(bx + 24), static_cast<float>(by + 224), 652.0f, 38.0f};
        const Rectangle btn_bypass{static_cast<float>(bx + 24), static_cast<float>(by + 272), 652.0f, 38.0f};
        const Rectangle btn_cancel{static_cast<float>(bx + 24), static_cast<float>(by + 320), 220.0f, 34.0f};

        DrawRectangleRec(
            btn_confirm,
            world.calibration_modal.can_confirm_clean ? Color{22, 118, 82, 255} : Color{36, 46, 58, 255});
        DrawRectangleLinesEx(
            btn_confirm,
            2.0f,
            world.calibration_modal.can_confirm_clean ? pal::kAccentClean : GRAY);
        DrawTextShadow(
            "[INVIO / C / CLICK] CONFERMA CALIBRAZIONE PULITA (+Energia Pulita, -Inquinamento)",
            bx + 36,
            by + 235,
            15,
            world.calibration_modal.can_confirm_clean ? RAYWHITE : GRAY);

        DrawRectangleRec(btn_bypass, Color{128, 52, 28, 255});
        DrawRectangleLinesEx(btn_bypass, 1.5f, ORANGE);
        DrawTextShadow(
            TextFormat("[B / CLICK] %s (+Smog!)", eco::kCombustionBypassLabel),
            bx + 36,
            by + 283,
            15,
            RAYWHITE);

        DrawRectangleRec(btn_cancel, Color{42, 54, 70, 255});
        DrawRectangleLinesEx(btn_cancel, 1.5f, LIGHTGRAY);
        DrawTextShadow("[X / BACKSPACE / CLICK] Annulla", bx + 32, by + 329, 14, LIGHTGRAY);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(mouse, btn_confirm)) {
                if (world.calibration_modal.ConfirmClean(world.ModalTargetMachine(), world.ecosystem)) {
                    if (world.calibrating_purifier) {
                        world.simulation_completed = true;
                    }
                    world.calibrating_purifier = false;
                }
            } else if (CheckCollisionPointRec(mouse, btn_bypass)) {
                world.calibration_modal.TriggerBypass(world.ModalTargetMachine(), world.ecosystem);
                if (world.calibrating_purifier) {
                    world.simulation_completed = true;
                }
                world.calibrating_purifier = false;
            } else if (CheckCollisionPointRec(mouse, btn_cancel)) {
                world.calibration_modal.Cancel();
                world.calibrating_purifier = false;
            }
        }
    }
}

} // namespace eco::app
