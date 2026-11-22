#include "app/pause_settings_menu.h"

#include <raylib.h>
#include <rlgl.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>

#include "app/game_state_manager.h"
#include "app/overworld_state.h"
#include "app/render/palette.h"
#include "app/render/particles.h"
#include "app/render/ui_style.h"
#include "app/title_state.h"
#include "core/math_types.h"
#include "core/simulation.h"

namespace eco::app {

namespace {

struct ResolutionOption {
    int width;
    int height;
    const char* label;
};

constexpr std::array<ResolutionOption, 4> kResolutions = {{
    {1024, 576, "1024 x 576 (Compatta 16:9)"},
    {1280, 720, "1280 x 720 (HD 16:9)"},
    {1600, 900, "1600 x 900 (HD+ 16:9)"},
    {1920, 1080, "1920 x 1080 (Full HD / Max Monitor)"}
}};

struct FpsOption {
    int target_fps;
    const char* label;
};

constexpr std::array<FpsOption, 4> kFpsOptions = {{
    {60, "60 FPS (Consigliato)"},
    {120, "120 FPS"},
    {144, "144 FPS"},
    {0, "Illimitato"}
}};

[[nodiscard]] const char* ParticleQualityLabel(ParticleQuality q) noexcept {
    switch (q) {
        case ParticleQuality::High:
            return "Alta (100% - Lucciole, Cenere, Raggi)";
        case ParticleQuality::Medium:
            return "Media (50% - Bilanciata)";
        case ParticleQuality::Off:
            return "Disattivata (Off)";
    }
    return "Alta";
}

[[nodiscard]] bool IsTitleStateActive(const AppContext& ctx) noexcept {
    if (!ctx.state_manager.HasActiveState()) {
        return false;
    }
    return std::string_view(ctx.state_manager.CurrentState()->Name()) == "TitleState";
}

[[nodiscard]] int SafeMonitorIndex(int monitor_index) noexcept {
    if (!IsWindowReady()) {
        return 0;
    }
    const int count = GetMonitorCount();
    if (count <= 0) {
        return 0;
    }
    if (monitor_index < 0) {
        return std::clamp(GetCurrentMonitor(), 0, count - 1);
    }
    return std::clamp(monitor_index, 0, count - 1);
}

[[nodiscard]] std::string FormatMonitorLabel(int monitor_index) {
    if (!IsWindowReady()) {
        return "< Monitor 1/1 >";
    }
    const int count = std::max(1, GetMonitorCount());
    const int idx = SafeMonitorIndex(monitor_index);
    const int mw = GetMonitorWidth(idx);
    const int mh = GetMonitorHeight(idx);
    const char* raw_name = GetMonitorName(idx);
    std::string name = (raw_name != nullptr && raw_name[0] != '\0') ? std::string(raw_name) : std::string("Display");
    if (name.size() > 18) {
        name = name.substr(0, 15) + "...";
    }
    return std::string("< Monitor ") + std::to_string(idx + 1) + "/" + std::to_string(count) +
           ": " + name + " (" + std::to_string(mw) + "x" + std::to_string(mh) + ") >";
}

void AdjustSettingValue(AppContext& ctx, GraphicsSettings& settings, int row, int dir) {
    if (dir == 0) {
        return;
    }
    switch (row) {
        case 0: { // Modalita' schermo
            ToggleAppFullscreen(settings);
            break;
        }
        case 1: { // Selezione Monitor / Schermo
            const int count = IsWindowReady() ? std::max(1, GetMonitorCount()) : 1;
            const int cur = SafeMonitorIndex(settings.monitor_index);
            settings.monitor_index = (cur + dir + count) % count;
            ApplySelectedMonitor(settings);
            break;
        }
        case 2: { // Risoluzione finestra
            const int n = static_cast<int>(kResolutions.size());
            settings.resolution_index = (settings.resolution_index + dir + n) % n;
            ApplySafeWindowResolution(settings);
            break;
        }
        case 3: { // Limite FPS
            const int n = static_cast<int>(kFpsOptions.size());
            settings.fps_index = (settings.fps_index + dir + n) % n;
            ApplyGraphicsSettingsToRuntime(settings);
            break;
        }
        case 4: { // Qualita' particelle
            int q = static_cast<int>(settings.particle_quality);
            q = (q + dir + 3) % 3;
            settings.particle_quality = static_cast<ParticleQuality>(q);
            ApplyGraphicsSettingsToRuntime(settings);
            break;
        }
        case 5: { // Font Interfaccia
            settings.clean_ttf_font = !settings.clean_ttf_font;
            ApplyGraphicsSettingsToRuntime(settings);
            break;
        }
        case 6: { // Contatore FPS
            settings.show_fps = !settings.show_fps;
            break;
        }
        case 7: { // Dettaglio Tutorial / Dialoghi
            settings.compact_tutorial = !settings.compact_tutorial;
            break;
        }
        case 8: { // Modalita' Storia: Campagna Completa (Atti I-V) vs Demo (Atti I-III)
            settings.demo_mode = !settings.demo_mode;
            ctx.world.demo_mode = settings.demo_mode;
            break;
        }
        default:
            break;
    }
}

} // namespace

void InitDefaultMonitor(GraphicsSettings& settings) {
    if (!IsWindowReady()) {
        settings.monitor_index = 0;
        return;
    }
    const int count = GetMonitorCount();
    if (count <= 0) {
        settings.monitor_index = 0;
        return;
    }
    // Su Wayland GetCurrentMonitor() ritorna spesso sempre 0 (anche se il monitor 0 e' lo schermo secondario).
    // Selezioniamo di default il monitor con la risoluzione (area) maggiore, lasciando poi libera scelta nel menu!
    int best = std::clamp(GetCurrentMonitor(), 0, count - 1);
    long long best_area = static_cast<long long>(GetMonitorWidth(best)) * GetMonitorHeight(best);
    for (int i = 0; i < count; ++i) {
        const long long area = static_cast<long long>(GetMonitorWidth(i)) * GetMonitorHeight(i);
        if (area > best_area) {
            best_area = area;
            best = i;
        }
    }
    settings.monitor_index = best;
}

void ApplyGraphicsSettingsToRuntime(const GraphicsSettings& settings) {
    SetUiFontCleanMode(settings.clean_ttf_font);

    switch (settings.particle_quality) {
        case ParticleQuality::High:
            SetParticleQualityScale(1.0f);
            break;
        case ParticleQuality::Medium:
            SetParticleQualityScale(0.5f);
            break;
        case ParticleQuality::Off:
            SetParticleQualityScale(0.0f);
            break;
    }

    if (IsWindowReady()) {
        const int idx = std::clamp(settings.fps_index, 0, static_cast<int>(kFpsOptions.size()) - 1);
        SetTargetFPS(kFpsOptions[static_cast<std::size_t>(idx)].target_fps);
    }
}

void ApplySelectedMonitor(GraphicsSettings& settings) {
    if (!IsWindowReady()) {
        return;
    }
    const int mon = SafeMonitorIndex(settings.monitor_index);
    settings.monitor_index = mon;

    if (IsWindowFullscreen()) {
        // In modalita' Fullscreen, SetWindowMonitor() chiama glfwSetWindowMonitor() sul wl_output/monitor
        // selezionato alla sua risoluzione nativa, senza mai chiamare glfwSetWindowPos() (zero errori Wayland!).
        SetWindowMonitor(mon);
    } else {
        // Su X11/Windows possiamo centrare subito la finestra sul monitor scelto;
        // su Wayland evitiamo glfwSetWindowPos() (non supportato) e il monitor verra' usato al prossimo Fullscreen.
        const bool is_wayland = (std::getenv("WAYLAND_DISPLAY") != nullptr);
        if (!is_wayland) {
            SetWindowMonitor(mon);
        }
    }
}

void ApplySafeWindowResolution(GraphicsSettings& settings) {
    if (!IsWindowReady()) {
        return;
    }

    const int idx = std::clamp(settings.resolution_index, 0, static_cast<int>(kResolutions.size()) - 1);
    const auto& res = kResolutions[static_cast<std::size_t>(idx)];

    const int mon = SafeMonitorIndex(settings.monitor_index);
    settings.monitor_index = mon;
    const int mon_w = GetMonitorWidth(mon);
    const int mon_h = GetMonitorHeight(mon);

    // Se la risoluzione scelta eccede o uguaglia il monitor selezionato, passiamo a Fullscreen sul monitor scelto.
    if (mon_w > 0 && mon_h > 0 && (res.width >= mon_w || res.height >= mon_h - 32)) {
        if (!IsWindowFullscreen()) {
            SetWindowSize(mon_w, mon_h);
            ToggleFullscreen();
        }
        SetWindowMonitor(mon);
        settings.fullscreen = true;
        return;
    }

    // Se eravamo in fullscreen e l'utente sceglie una risoluzione finestra che entra nel monitor:
    if (IsWindowFullscreen()) {
        ToggleFullscreen();
    }
    if (IsWindowMaximized()) {
        RestoreWindow();
    }
    settings.fullscreen = false;
    SetWindowSize(res.width, res.height);
}

void ToggleAppFullscreen(GraphicsSettings& settings) {
    settings.fullscreen = !settings.fullscreen;
    if (!IsWindowReady()) {
        return;
    }

    const int mon = SafeMonitorIndex(settings.monitor_index);
    settings.monitor_index = mon;
    const int mon_w = GetMonitorWidth(mon);
    const int mon_h = GetMonitorHeight(mon);

    if (settings.fullscreen) {
        if (!IsWindowFullscreen()) {
            if (mon_w > 0 && mon_h > 0) {
                SetWindowSize(mon_w, mon_h);
            }
            ToggleFullscreen();
        }
        // Sposta immediatamente il fullscreen esattamente sul monitor scelto dall'utente!
        SetWindowMonitor(mon);
    } else {
        if (IsWindowFullscreen()) {
            ToggleFullscreen();
        }
        const int idx = std::clamp(settings.resolution_index, 0, static_cast<int>(kResolutions.size()) - 1);
        const auto& res = kResolutions[static_cast<std::size_t>(idx)];
        const int safe_w = (mon_w > 0 && res.width >= mon_w) ? 1280 : res.width;
        const int safe_h = (mon_h > 0 && res.height >= mon_h - 32) ? 720 : res.height;
        SetWindowSize(safe_w, safe_h);
    }
}

void PauseSettingsMenu::OpenPause() noexcept {
    is_open_ = true;
    in_settings_ = false;
    opened_from_title_ = false;
    pause_cursor_ = 0;
}

void PauseSettingsMenu::OpenSettingsDirect() noexcept {
    is_open_ = true;
    in_settings_ = true;
    opened_from_title_ = true;
    settings_cursor_ = 0;
}

void PauseSettingsMenu::Close() noexcept {
    is_open_ = false;
    in_settings_ = false;
    opened_from_title_ = false;
}

bool PauseSettingsMenu::UpdateAndHandleInput(AppContext& ctx, GraphicsSettings& settings) {
    // Scorciatoia globale F11 o Alt+Invio per Schermo Intero in qualsiasi momento
    if (IsKeyPressed(KEY_F11) ||
        ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER))) {
        ToggleAppFullscreen(settings);
    }

    // Se la modalita' dialoghi sintetici (tutorial compatto) e' attiva, velocizziamo i dialoghi lunghi
    if (settings.compact_tutorial && ctx.world.story.HasActiveDialogue() && ctx.world.story.LineElapsed() > 3.2f) {
        ctx.world.story.AdvanceDialogue();
    }

    const bool on_title = IsTitleStateActive(ctx);

    // Gestione tasto ESC (e tasto O sulla schermata del titolo per aprire direttamente le impostazioni)
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (!is_open_) {
            if (ctx.world.calibration_modal.is_open) {
                ctx.world.calibration_modal.Cancel();
                ctx.world.calibrating_purifier = false;
                ctx.world.calibrating_thermal = false;
                return false;
            }
            if (on_title) {
                OpenSettingsDirect();
            } else {
                OpenPause();
            }
            return true;
        }

        if (in_settings_) {
            if (opened_from_title_) {
                Close();
                return false;
            }
            in_settings_ = false;
            return true;
        }

        Close();
        return false;
    }

    if (!is_open_ && on_title && IsKeyPressed(KEY_O)) {
        OpenSettingsDirect();
        return true;
    }

    if (!is_open_) {
        return false;
    }

    const Vector2 mouse = GetMousePosition();
    const bool mouse_clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    const bool up_pressed = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
    const bool down_pressed = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
    const bool left_pressed = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
    const bool right_pressed = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
    const bool confirm_pressed =
        IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_E) || IsKeyPressed(KEY_SPACE);

    if (!in_settings_) {
        // --- MENU DI PAUSA (5 Voci) ---
        constexpr int kPauseItemCount = 5;
        if (up_pressed) {
            pause_cursor_ = (pause_cursor_ - 1 + kPauseItemCount) % kPauseItemCount;
        }
        if (down_pressed) {
            pause_cursor_ = (pause_cursor_ + 1) % kPauseItemCount;
        }

        const int box_w = 520;
        const int box_h = 380;
        const int bx = (eco::kWindowWidth - box_w) / 2;
        const int by = (eco::kWindowHeight - box_h) / 2;

        int activated_action = -1;
        for (int i = 0; i < kPauseItemCount; ++i) {
            const Rectangle btn_rect{
                static_cast<float>(bx + 36),
                static_cast<float>(by + 74 + i * 52),
                static_cast<float>(box_w - 72),
                42.0f
            };
            if (CheckCollisionPointRec(mouse, btn_rect)) {
                if (GetMouseDelta().x != 0.0f || GetMouseDelta().y != 0.0f || mouse_clicked) {
                    pause_cursor_ = i;
                }
                if (mouse_clicked) {
                    activated_action = i;
                }
            }
        }

        if (confirm_pressed) {
            activated_action = pause_cursor_;
        }

        switch (activated_action) {
            case 0: // Riprendi Partita
                Close();
                return false;
            case 1: // Impostazioni Grafiche
                in_settings_ = true;
                settings_cursor_ = 0;
                break;
            case 2: // Ricomincia Partita / Reset Valle
                ctx.world.Reset();
                ctx.world.EnsureInitialized();
                ctx.state_manager.RequestTransition(std::make_unique<OverworldState>(ctx));
                Close();
                return false;
            case 3: // Torna alla Schermata del Titolo
                ctx.world.Reset();
                ctx.world.EnsureInitialized();
                ctx.state_manager.RequestTransition(std::make_unique<TitleState>(ctx));
                Close();
                return false;
            case 4: // Esci dal Gioco
                ctx.quit_requested = true;
                Close();
                return false;
            default:
                break;
        }
    } else {
        // --- SOTTOMENU IMPOSTAZIONI GRAFICHE (9 Opzioni + Pulsante Indietro) ---
        constexpr int kOptionCount = 9;
        constexpr int kSettingsRowCount = kOptionCount + 1; // 0..8 opzioni, 9 = Indietro
        if (up_pressed) {
            settings_cursor_ = (settings_cursor_ - 1 + kSettingsRowCount) % kSettingsRowCount;
        }
        if (down_pressed) {
            settings_cursor_ = (settings_cursor_ + 1) % kSettingsRowCount;
        }

        if (settings_cursor_ < kOptionCount) {
            if (left_pressed) {
                AdjustSettingValue(ctx, settings, settings_cursor_, -1);
            } else if (right_pressed || confirm_pressed) {
                AdjustSettingValue(ctx, settings, settings_cursor_, +1);
            }
        } else if (confirm_pressed) {
            if (opened_from_title_) {
                Close();
                return false;
            }
            in_settings_ = false;
            return true;
        }

        const int box_w = 780;
        const int box_h = 560;
        const int bx = (eco::kWindowWidth - box_w) / 2;
        const int by = (eco::kWindowHeight - box_h) / 2;

        for (int i = 0; i < kOptionCount; ++i) {
            const Rectangle row_rect{
                static_cast<float>(bx + 28),
                static_cast<float>(by + 58 + i * 46),
                static_cast<float>(box_w - 56),
                38.0f
            };
            if (CheckCollisionPointRec(mouse, row_rect)) {
                if (GetMouseDelta().x != 0.0f || GetMouseDelta().y != 0.0f || mouse_clicked) {
                    settings_cursor_ = i;
                }
                if (mouse_clicked) {
                    const float mid_x = row_rect.x + row_rect.width * 0.65f;
                    AdjustSettingValue(ctx, settings, i, (mouse.x < mid_x) ? -1 : +1);
                }
            }
        }

        const Rectangle back_rect{
            static_cast<float>(bx + 28),
            static_cast<float>(by + box_h - 54),
            240.0f,
            36.0f
        };
        if (CheckCollisionPointRec(mouse, back_rect)) {
            if (GetMouseDelta().x != 0.0f || GetMouseDelta().y != 0.0f || mouse_clicked) {
                settings_cursor_ = kOptionCount;
            }
            if (mouse_clicked) {
                if (opened_from_title_) {
                    Close();
                    return false;
                }
                in_settings_ = false;
            }
        }
    }

    return is_open_;
}

void PauseSettingsMenu::DrawOverlay(const AppContext& ctx, const GraphicsSettings& settings) const {
    // 1. Contatore FPS opzionale in alto a destra
    if (settings.show_fps) {
        const char* fps_text = TextFormat("FPS: %d", GetFPS());
        const int tw = MeasureUiText(fps_text, 13);
        const Rectangle fps_box{
            static_cast<float>(eco::kWindowWidth - tw - 28),
            8.0f,
            static_cast<float>(tw + 16),
            24.0f
        };
        DrawRectangleRec(fps_box, Color{10, 22, 32, 230});
        DrawRectangleLinesEx(fps_box, 1.2f, pal::kAccentClean);
        DrawTextShadow(fps_text, static_cast<int>(fps_box.x) + 8, 12, 13, pal::kAccentClean);
    }

    if (!is_open_) {
        return;
    }

    // 2. Oscuramento morbido dello sfondo di gioco in pausa
    DrawRectangle(0, 0, eco::kWindowWidth, eco::kWindowHeight, Color{4, 10, 18, 195});

    if (!in_settings_) {
        // --- DISEGNO MENU DI PAUSA ---
        const int box_w = 520;
        const int box_h = 380;
        const int bx = (eco::kWindowWidth - box_w) / 2;
        const int by = (eco::kWindowHeight - box_h) / 2;

        DrawPanelWithHeader(
            Rectangle{static_cast<float>(bx), static_cast<float>(by), static_cast<float>(box_w), static_cast<float>(box_h)},
            44,
            "PAUSA // TERMINALE DELL'OSSERVATORIO",
            pal::kUiBorderCyan);

        constexpr std::array<const char*, 5> kLabels = {
            "Riprendi Esplorazione [ESC]",
            "Impostazioni Grafiche & Schermo",
            "Ricomincia Partita (Reset Valle)",
            "Torna alla Schermata del Titolo",
            "Esci dal Gioco"
        };

        for (int i = 0; i < static_cast<int>(kLabels.size()); ++i) {
            const bool selected = (pause_cursor_ == i);
            const Rectangle btn{
                static_cast<float>(bx + 36),
                static_cast<float>(by + 74 + i * 52),
                static_cast<float>(box_w - 72),
                42.0f
            };
            const Color fill = selected ? Color{24, 62, 82, 245} : Color{14, 24, 36, 230};
            const Color border = selected
                                     ? ((i == 4) ? Color{240, 105, 75, 255} : pal::kAccentClean)
                                     : Color{55, 95, 122, 185};
            DrawRectangleRec(btn, fill);
            DrawRectangleLinesEx(btn, selected ? 2.0f : 1.2f, border);

            const char* label = kLabels[static_cast<std::size_t>(i)];
            const int tw = MeasureUiText(label, 17);
            DrawTextShadow(
                label,
                bx + (box_w - tw) / 2,
                static_cast<int>(btn.y) + 11,
                17,
                selected ? RAYWHITE : Color{185, 210, 225, 235});
        }

        const char* hint = "Usa [W/S], [Frecce] o il Mouse | [INVIO / CLICK] Conferma | [F11] Schermo Intero";
        const int hw = MeasureUiText(hint, 13);
        DrawTextShadow(hint, bx + (box_w - hw) / 2, by + box_h - 28, 13, Color{145, 185, 205, 220});
    } else {
        // --- DISEGNO SOTTOMENU IMPOSTAZIONI GRAFICHE ---
        constexpr int kOptionCount = 9;
        const int box_w = 780;
        const int box_h = 560;
        const int bx = (eco::kWindowWidth - box_w) / 2;
        const int by = (eco::kWindowHeight - box_h) / 2;

        DrawPanelWithHeader(
            Rectangle{static_cast<float>(bx), static_cast<float>(by), static_cast<float>(box_w), static_cast<float>(box_h)},
            44,
            "IMPOSTAZIONI GRAFICHE & SCHERMO",
            pal::kAccentClean);

        const int res_idx = std::clamp(settings.resolution_index, 0, static_cast<int>(kResolutions.size()) - 1);
        const int fps_idx = std::clamp(settings.fps_index, 0, static_cast<int>(kFpsOptions.size()) - 1);
        const std::string monitor_label = FormatMonitorLabel(settings.monitor_index);

        const std::array<const char*, kOptionCount> row_titles = {
            "Modalita' Schermo [F11]",
            "Schermo / Monitor",
            "Risoluzione Finestra",
            "Frequenza Fotogrammi (FPS)",
            "Particelle & Effetti Atmosferici",
            "Stile Font Interfaccia",
            "Contatore FPS a Schermo",
            "Ritmo Dialoghi & Tutorial",
            "Modalita' Storia"
        };

        const std::array<const char*, kOptionCount> row_values = {
            settings.fullscreen ? "< Schermo Intero (Fullscreen) >" : "< Finestra Ridimensionabile >",
            monitor_label.c_str(),
            kResolutions[static_cast<std::size_t>(res_idx)].label,
            kFpsOptions[static_cast<std::size_t>(fps_idx)].label,
            ParticleQualityLabel(settings.particle_quality),
            settings.clean_ttf_font ? "< Noto Sans HD (Nitido) >" : "< Pixel Bitmap (Retro) >",
            settings.show_fps ? "< Attivo (Mostra FPS) >" : "< Disattivato >",
            settings.compact_tutorial ? "< Sintetico / Rapido >" : "< Completo (Consigliato) >",
            ctx.world.demo_mode ? "< Demo Classica (Atti I-III) >" : "< Campagna Completa (Atti I-V) >"
        };

        for (int i = 0; i < kOptionCount; ++i) {
            const bool selected = (settings_cursor_ == i);
            const Rectangle row_rect{
                static_cast<float>(bx + 28),
                static_cast<float>(by + 58 + i * 46),
                static_cast<float>(box_w - 56),
                38.0f
            };

            DrawRectangleRec(row_rect, selected ? Color{22, 54, 72, 245} : Color{14, 24, 34, 225});
            DrawRectangleLinesEx(
                row_rect,
                selected ? 1.8f : 1.0f,
                selected ? pal::kAccentClean : Color{52, 86, 110, 175});

            DrawTextShadow(
                row_titles[static_cast<std::size_t>(i)],
                static_cast<int>(row_rect.x) + 16,
                static_cast<int>(row_rect.y) + 9,
                15,
                selected ? RAYWHITE : Color{190, 215, 230, 240});

            const char* val_text = row_values[static_cast<std::size_t>(i)];
            const int vw = MeasureUiText(val_text, 15);
            DrawTextShadow(
                val_text,
                static_cast<int>(row_rect.x + row_rect.width) - vw - 16,
                static_cast<int>(row_rect.y) + 9,
                15,
                selected ? Color{130, 255, 215, 255} : Color{240, 210, 120, 240});
        }

        // Pulsante Indietro / Chiudi
        const bool back_selected = (settings_cursor_ == kOptionCount);
        const Rectangle back_rect{
            static_cast<float>(bx + 28),
            static_cast<float>(by + box_h - 54),
            240.0f,
            36.0f
        };
        DrawRectangleRec(back_rect, back_selected ? Color{28, 72, 92, 245} : Color{18, 32, 46, 235});
        DrawRectangleLinesEx(back_rect, back_selected ? 2.0f : 1.3f, back_selected ? pal::kAccentClean : SKYBLUE);
        const char* back_label = IsTitleStateActive(ctx) ? "[ESC] Torna al Titolo" : "[ESC] Torna alla Pausa";
        DrawTextShadow(back_label, static_cast<int>(back_rect.x) + 22, static_cast<int>(back_rect.y) + 9, 15, RAYWHITE);

        const char* hint = "Cambia valori con [A/D], [Frecce Sinistra/Destra], [INVIO] o Click del Mouse";
        const int hw = MeasureUiText(hint, 13);
        DrawTextShadow(hint, bx + box_w - hw - 28, by + box_h - 44, 13, Color{155, 195, 215, 225});
    }
}

} // namespace eco::app
