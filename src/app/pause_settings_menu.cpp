#include "app/pause_settings_menu.h"

#include <raylib.h>
#include <rlgl.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>

#include "app/audio/audio_engine.h"
#include "app/game_state_manager.h"
#include "app/gpu_selector.h"
#include "app/overworld_state.h"
#include "app/render/palette.h"
#include "app/render/particles.h"
#include "app/render/ui_style.h"
#include "app/story_ui.h"
#include "app/title_state.h"
#include "core/math_types.h"
#include "core/save_data.h"
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

struct VolumeOption {
    float level;
    const char* label;
};

constexpr std::array<VolumeOption, 5> kVolumeOptions = {{
    {0.00f, "< Muto (0%) >"},
    {0.25f, "< Basso (25%) >"},
    {0.50f, "< Medio (50%) >"},
    {0.75f, "< Normale (75%) >"},
    {1.00f, "< Massimo (100%) >"}
}};

[[nodiscard]] const char* ParticleQualityLabel(ParticleQuality q) noexcept {
    switch (q) {
        case ParticleQuality::High:
            return "Alta (100% - Lucciole, Polline, Vapore)";
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

[[nodiscard]] bool IsGameplayStateActive(const AppContext& ctx) noexcept {
    if (!ctx.state_manager.HasActiveState()) {
        return false;
    }
    const std::string_view name(ctx.state_manager.CurrentState()->Name());
    return name == "OverworldState" || name == "StructureState";
}

[[nodiscard]] std::string FormatMonitorLabel(int monitor_index) {
    const int count = IsWindowReady() ? std::max(1, GetMonitorCount()) : 1;
    if (monitor_index < 0 || monitor_index >= count) {
        return "< Auto (Monitor Corrente) >";
    }
    const char* raw_name = IsWindowReady() ? GetMonitorName(monitor_index) : nullptr;
    std::string mname = (raw_name != nullptr && raw_name[0] != '\0')
                            ? std::string(raw_name)
                            : ("Schermo " + std::to_string(monitor_index + 1));
    if (mname.size() > 16) {
        mname = mname.substr(0, 16);
    }
    const int mw = IsWindowReady() ? GetMonitorWidth(monitor_index) : 1280;
    const int mh = IsWindowReady() ? GetMonitorHeight(monitor_index) : 720;
    return "< Monitor " + std::to_string(monitor_index + 1) + "/" + std::to_string(count) +
           ": " + mname + " (" + std::to_string(mw) + "x" + std::to_string(mh) + ") >";
}

[[nodiscard]] std::string FormatGpuLabel(const std::string& gpu_id) {
    const auto& list = EnumerateGpuOptions();
    const int idx = FindGpuOptionIndex(gpu_id);
    if (idx >= 0 && idx < static_cast<int>(list.size())) {
        return "< " + list[static_cast<std::size_t>(idx)].label + " >";
    }
    return "< Auto (Predefinita di Sistema) >";
}

void AdjustSettingValue(GraphicsSettings& settings, int row, int dir) {
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
            // Ciclo: -1 (Auto) -> 0 -> 1 -> ... -> count-1
            const int total_slots = count + 1;
            int slot = (settings.monitor_index < 0 || settings.monitor_index >= count)
                           ? 0
                           : (settings.monitor_index + 1);
            slot = (slot + dir + total_slots) % total_slots;
            settings.monitor_index = (slot == 0) ? -1 : (slot - 1);
            ApplySelectedMonitor(settings);
            break;
        }
        case 2: { // GPU / Scheda Grafica
            const auto& gpus = EnumerateGpuOptions();
            const int n = std::max(1, static_cast<int>(gpus.size()));
            int idx = FindGpuOptionIndex(settings.gpu_preference);
            idx = (idx + dir + n) % n;
            settings.gpu_preference = gpus[static_cast<std::size_t>(idx)].id;
            break;
        }
        case 3: { // Risoluzione finestra
            const int n = static_cast<int>(kResolutions.size());
            settings.resolution_index = (settings.resolution_index + dir + n) % n;
            ApplySafeWindowResolution(settings);
            break;
        }
        case 4: { // Limite FPS
            const int n = static_cast<int>(kFpsOptions.size());
            settings.fps_index = (settings.fps_index + dir + n) % n;
            ApplyGraphicsSettingsToRuntime(settings);
            break;
        }
        case 5: { // Qualita' particelle
            int q = static_cast<int>(settings.particle_quality);
            q = (q + dir + 3) % 3;
            settings.particle_quality = static_cast<ParticleQuality>(q);
            ApplyGraphicsSettingsToRuntime(settings);
            break;
        }
        case 6: { // Font Interfaccia
            settings.clean_ttf_font = !settings.clean_ttf_font;
            ApplyGraphicsSettingsToRuntime(settings);
            break;
        }
        case 7: { // Contatore FPS
            settings.show_fps = !settings.show_fps;
            break;
        }
        case 8: { // Volume Audio
            const int n = static_cast<int>(kVolumeOptions.size());
            settings.audio_volume_index = (settings.audio_volume_index + dir + n) % n;
            ApplyGraphicsSettingsToRuntime(settings);
            break;
        }
        case 9: { // Paesaggio Sonoro
            settings.ambient_audio = !settings.ambient_audio;
            ApplyGraphicsSettingsToRuntime(settings);
            break;
        }
        default:
            break;
    }
    (void)SaveGraphicsSettings(settings);
    PlayUiSfx(SfxId::UiClick, 0.58f, (dir > 0) ? 1.06f : 0.94f);
}

[[nodiscard]] int ParseIntSetting(const std::string& s, int fallback) noexcept {
    try {
        return std::stoi(s);
    } catch (...) {
        return fallback;
    }
}

} // namespace

bool LoadGraphicsSettings(GraphicsSettings& settings, const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        const std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        while (!val.empty() && (val.back() == '\r' || val.back() == '\n')) {
            val.pop_back();
        }

        if (key == "fullscreen") {
            settings.fullscreen = (ParseIntSetting(val, 0) != 0);
        } else if (key == "monitor_index") {
            settings.monitor_index = ParseIntSetting(val, -1);
        } else if (key == "resolution_index") {
            settings.resolution_index =
                std::clamp(ParseIntSetting(val, 1), 0, static_cast<int>(kResolutions.size()) - 1);
        } else if (key == "fps_index") {
            settings.fps_index =
                std::clamp(ParseIntSetting(val, 0), 0, static_cast<int>(kFpsOptions.size()) - 1);
        } else if (key == "particle_quality") {
            const int q = std::clamp(ParseIntSetting(val, 0), 0, 2);
            settings.particle_quality = static_cast<ParticleQuality>(q);
        } else if (key == "clean_ttf_font") {
            settings.clean_ttf_font = (ParseIntSetting(val, 1) != 0);
        } else if (key == "show_fps") {
            settings.show_fps = (ParseIntSetting(val, 0) != 0);
        } else if (key == "compact_tutorial") {
            settings.compact_tutorial = (ParseIntSetting(val, 0) != 0);
        } else if (key == "audio_volume_index") {
            settings.audio_volume_index =
                std::clamp(ParseIntSetting(val, 3), 0, static_cast<int>(kVolumeOptions.size()) - 1);
        } else if (key == "ambient_audio") {
            settings.ambient_audio = (ParseIntSetting(val, 1) != 0);
        } else if (key == "gpu_preference") {
            if (!val.empty()) {
                settings.gpu_preference = val;
            }
        }
    }
    return true;
}

bool SaveGraphicsSettings(const GraphicsSettings& settings, const std::string& path) {
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path target(path);
    if (target.has_parent_path()) {
        fs::create_directories(target.parent_path(), ec);
    }
    std::ofstream out(target, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    out << "fullscreen=" << (settings.fullscreen ? 1 : 0) << "\n";
    out << "monitor_index=" << settings.monitor_index << "\n";
    out << "resolution_index=" << settings.resolution_index << "\n";
    out << "fps_index=" << settings.fps_index << "\n";
    out << "particle_quality=" << static_cast<int>(settings.particle_quality) << "\n";
    out << "clean_ttf_font=" << (settings.clean_ttf_font ? 1 : 0) << "\n";
    out << "show_fps=" << (settings.show_fps ? 1 : 0) << "\n";
    out << "compact_tutorial=" << (settings.compact_tutorial ? 1 : 0) << "\n";
    out << "audio_volume_index=" << settings.audio_volume_index << "\n";
    out << "ambient_audio=" << (settings.ambient_audio ? 1 : 0) << "\n";
    out << "gpu_preference=" << settings.gpu_preference << "\n";
    return out.good();
}

int ResolveTargetMonitor(const GraphicsSettings& settings) noexcept {
    if (!IsWindowReady()) {
        return 0;
    }
    const int count = GetMonitorCount();
    if (count <= 0) {
        return 0;
    }
    if (settings.monitor_index >= 0 && settings.monitor_index < count) {
        return settings.monitor_index;
    }
    const int cur = GetCurrentMonitor();
    return (cur >= 0 && cur < count) ? cur : 0;
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

    const int vol_idx = std::clamp(
        settings.audio_volume_index,
        0,
        static_cast<int>(kVolumeOptions.size()) - 1);
    const float vol = kVolumeOptions[static_cast<std::size_t>(vol_idx)].level;
    ApplyGlobalAudioSettings(vol, vol <= 0.001f, settings.ambient_audio);

    if (IsWindowReady()) {
        const int idx = std::clamp(settings.fps_index, 0, static_cast<int>(kFpsOptions.size()) - 1);
        SetTargetFPS(kFpsOptions[static_cast<std::size_t>(idx)].target_fps);
    }
}

void ApplySelectedMonitor(GraphicsSettings& settings) {
    if (!IsWindowReady()) {
        return;
    }
    const int target_mon = ResolveTargetMonitor(settings);
    // In Raylib/GLFW:
    // - Se fullscreen == true, SetWindowMonitor(target_mon) sposta istantaneamente il fullscreen
    //   sul monitor scelto (risolvendo il salto al secondo monitor su Wayland e X11!).
    // - Se fullscreen == false, SetWindowMonitor centra la finestra sul monitor scelto (su X11)
    //   e memorizza il monitor per il successivo F11 anche su Wayland.
    SetWindowMonitor(target_mon);
}

void ApplySafeWindowResolution(GraphicsSettings& settings) {
    if (!IsWindowReady()) {
        return;
    }

    const int idx = std::clamp(settings.resolution_index, 0, static_cast<int>(kResolutions.size()) - 1);
    const auto& res = kResolutions[static_cast<std::size_t>(idx)];

    const int mon = ResolveTargetMonitor(settings);
    const int mon_w = GetMonitorWidth(mon);
    const int mon_h = GetMonitorHeight(mon);

    if (mon_w > 0 && mon_h > 0 && (res.width >= mon_w || res.height >= mon_h - 32)) {
        if (!IsWindowFullscreen()) {
            ToggleFullscreen();
        }
        SetWindowMonitor(mon);
        settings.fullscreen = true;
        return;
    }

    if (IsWindowFullscreen()) {
        ToggleFullscreen();
    }
    if (IsWindowMaximized()) {
        RestoreWindow();
    }
    settings.fullscreen = false;
    SetWindowSize(res.width, res.height);
    if (settings.monitor_index >= 0) {
        SetWindowMonitor(mon);
    }
}

void ToggleAppFullscreen(GraphicsSettings& settings) {
    settings.fullscreen = !settings.fullscreen;
    if (!IsWindowReady()) {
        return;
    }

    const int mon = ResolveTargetMonitor(settings);
    const int mon_w = GetMonitorWidth(mon);
    const int mon_h = GetMonitorHeight(mon);

    if (settings.fullscreen) {
        if (!IsWindowFullscreen()) {
            ToggleFullscreen();
        }
        // Forza sempre lo schermo scelto dall'utente (fix multi-monitor su Wayland/X11)
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
        if (settings.monitor_index >= 0) {
            SetWindowMonitor(mon);
        }
    }
    (void)SaveGraphicsSettings(settings);
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

void PauseSettingsMenu::TriggerSaveFeedback(const char* message) noexcept {
    save_feedback_timer_ = 2.6f;
    save_feedback_msg_ = (message != nullptr) ? std::string(message) : std::string("Partita salvata!");
}

bool PauseSettingsMenu::UpdateAndHandleInput(
    AppContext& ctx,
    GraphicsSettings& settings,
    JournalOverlay* journal) {
    if (save_feedback_timer_ > 0.0f) {
        save_feedback_timer_ = std::max(0.0f, save_feedback_timer_ - GetFrameTime());
    }

    // Richiesta di apertura impostazioni proveniente dal menu del Titolo
    if (ctx.open_settings_requested) {
        ctx.open_settings_requested = false;
        OpenSettingsDirect();
        return true;
    }

    // Scorciatoia globale F11 o Alt+Invio per Schermo Intero sul monitor scelto
    if (IsKeyPressed(KEY_F11) ||
        ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER))) {
        ToggleAppFullscreen(settings);
    }

    // Scorciatoia globale M per attivare/silenziare rapidamente l'audio
    if (IsKeyPressed(KEY_M)) {
        settings.audio_volume_index = (settings.audio_volume_index == 0) ? 3 : 0;
        ApplyGraphicsSettingsToRuntime(settings);
        (void)SaveGraphicsSettings(settings);
        if (settings.audio_volume_index > 0) {
            PlayUiSfx(SfxId::UiClick, 0.55f, 1.05f);
        }
    }

    // Avanzamento rapido dei dialoghi se il giocatore attiva il ritmo sintetico
    if (!is_open_ && (journal == nullptr || !journal->IsOpen()) &&
        settings.compact_tutorial && ctx.world.story.HasActiveDialogue() &&
        ctx.world.story.LineElapsed() > 3.2f) {
        ctx.world.story.AdvanceDialogue();
    }

    const bool on_title = IsTitleStateActive(ctx);

    // Priorita' ESC: Calibrazione -> Diario [H/J] -> Menu di Pausa / Impostazioni
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (!is_open_) {
            if (ctx.world.calibration_modal.is_open) {
                ctx.world.calibration_modal.Cancel();
                return false;
            }
            if (journal != nullptr && journal->IsOpen()) {
                journal->Close();
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
        // --- MENU DI PAUSA (7 Voci, inclusa Salva Partita) ---
        constexpr int kPauseItemCount = 7;
        if (up_pressed) {
            pause_cursor_ = (pause_cursor_ - 1 + kPauseItemCount) % kPauseItemCount;
            PlayUiSfx(SfxId::UiClick, 0.45f, 1.04f);
        }
        if (down_pressed) {
            pause_cursor_ = (pause_cursor_ + 1) % kPauseItemCount;
            PlayUiSfx(SfxId::UiClick, 0.45f, 0.96f);
        }

        const int box_w = 550;
        const int box_h = 468;
        const int bx = (eco::kWindowWidth - box_w) / 2;
        const int by = (eco::kWindowHeight - box_h) / 2;

        int activated_action = -1;
        for (int i = 0; i < kPauseItemCount; ++i) {
            const Rectangle btn_rect{
                static_cast<float>(bx + 36),
                static_cast<float>(by + 68 + i * 50),
                static_cast<float>(box_w - 72),
                40.0f
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
            case 0: // Riprendi Esplorazione
                Close();
                return false;
            case 1: // Salva Partita Ora
                if (eco::SaveWorldToFile(ctx.world)) {
                    TriggerSaveFeedback("Partita salvata nella Conca di Meridia!");
                    PlayUiSfx(SfxId::UiClick, 0.72f, 1.16f);
                } else {
                    TriggerSaveFeedback("Errore durante il salvataggio.");
                }
                break;
            case 2: // Diario: Cronologia & Codex [H / J]
                Close();
                if (journal != nullptr) {
                    journal->OpenHistory();
                }
                return false;
            case 3: // Impostazioni Schermo, GPU & Audio
                in_settings_ = true;
                settings_cursor_ = 0;
                break;
            case 4: // Ricomincia Viaggio (Reset Conca)
                ctx.world.Reset();
                ctx.world.EnsureInitialized();
                ctx.state_manager.RequestTransition(std::make_unique<OverworldState>(ctx));
                Close();
                return false;
            case 5: // Salva e Torna alla Schermata del Titolo
                if (IsGameplayStateActive(ctx)) {
                    (void)eco::SaveWorldToFile(ctx.world);
                }
                ctx.state_manager.RequestTransition(std::make_unique<TitleState>(ctx));
                Close();
                return false;
            case 6: // Salva ed Esci dal Gioco
                if (IsGameplayStateActive(ctx)) {
                    (void)eco::SaveWorldToFile(ctx.world);
                }
                ctx.quit_requested = true;
                Close();
                return false;
            default:
                break;
        }
    } else {
        // --- SOTTOMENU IMPOSTAZIONI SCHERMO, GPU & AUDIO (10 Opzioni + Indietro) ---
        constexpr int kOptionCount = 10;
        const bool gpu_changed = (settings.gpu_preference != settings.initial_gpu_preference);
        const int kSettingsRowCount = kOptionCount + (gpu_changed ? 2 : 1);

        if (up_pressed) {
            settings_cursor_ = (settings_cursor_ - 1 + kSettingsRowCount) % kSettingsRowCount;
            PlayUiSfx(SfxId::UiClick, 0.45f, 1.04f);
        }
        if (down_pressed) {
            settings_cursor_ = (settings_cursor_ + 1) % kSettingsRowCount;
            PlayUiSfx(SfxId::UiClick, 0.45f, 0.96f);
        }

        if (settings_cursor_ < kOptionCount) {
            if (left_pressed) {
                AdjustSettingValue(settings, settings_cursor_, -1);
            } else if (right_pressed || confirm_pressed) {
                AdjustSettingValue(settings, settings_cursor_, +1);
            }
        } else if (confirm_pressed) {
            if (gpu_changed && settings_cursor_ == kOptionCount) {
                (void)SaveGraphicsSettings(settings);
                if (IsGameplayStateActive(ctx)) {
                    (void)eco::SaveWorldToFile(ctx.world);
                }
                ctx.relaunch_gpu_requested = true;
                return false;
            }
            PlayUiSfx(SfxId::UiClick, 0.55f, 0.94f);
            if (opened_from_title_) {
                Close();
                return false;
            }
            in_settings_ = false;
            return true;
        }

        const int box_w = 820;
        const int box_h = 594;
        const int bx = (eco::kWindowWidth - box_w) / 2;
        const int by = (eco::kWindowHeight - box_h) / 2;

        for (int i = 0; i < kOptionCount; ++i) {
            const Rectangle row_rect{
                static_cast<float>(bx + 26),
                static_cast<float>(by + 60 + i * 42),
                static_cast<float>(box_w - 52),
                35.0f
            };
            if (CheckCollisionPointRec(mouse, row_rect)) {
                if (GetMouseDelta().x != 0.0f || GetMouseDelta().y != 0.0f || mouse_clicked) {
                    settings_cursor_ = i;
                }
                if (mouse_clicked) {
                    const float mid_x = row_rect.x + row_rect.width * 0.65f;
                    AdjustSettingValue(settings, i, (mouse.x < mid_x) ? -1 : +1);
                }
            }
        }

        const Rectangle back_rect{
            static_cast<float>(bx + 26),
            static_cast<float>(by + box_h - 52),
            240.0f,
            36.0f
        };
        const int back_row_index = gpu_changed ? (kOptionCount + 1) : kOptionCount;
        if (CheckCollisionPointRec(mouse, back_rect)) {
            if (GetMouseDelta().x != 0.0f || GetMouseDelta().y != 0.0f || mouse_clicked) {
                settings_cursor_ = back_row_index;
            }
            if (mouse_clicked) {
                PlayUiSfx(SfxId::UiClick, 0.55f, 0.94f);
                if (opened_from_title_) {
                    Close();
                    return false;
                }
                in_settings_ = false;
            }
        }

        if (gpu_changed) {
            const Rectangle apply_gpu_rect{
                static_cast<float>(bx + 282),
                static_cast<float>(by + box_h - 52),
                275.0f,
                36.0f
            };
            if (CheckCollisionPointRec(mouse, apply_gpu_rect)) {
                if (GetMouseDelta().x != 0.0f || GetMouseDelta().y != 0.0f || mouse_clicked) {
                    settings_cursor_ = kOptionCount;
                }
                if (mouse_clicked) {
                    (void)SaveGraphicsSettings(settings);
                    if (IsGameplayStateActive(ctx)) {
                        (void)eco::SaveWorldToFile(ctx.world);
                    }
                    ctx.relaunch_gpu_requested = true;
                    return false;
                }
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

    // Toast di conferma salvataggio (visibile sia in pausa che in gioco)
    if (save_feedback_timer_ > 0.0f && !save_feedback_msg_.empty()) {
        const float alpha = std::clamp(save_feedback_timer_ * 1.4f, 0.0f, 1.0f);
        DrawBannerToast(
            Rectangle{440.0f, 16.0f, 400.0f, 46.0f},
            "ARCHIVIO CONCA",
            save_feedback_msg_.c_str(),
            nullptr,
            pal::kJadeGlow,
            alpha);
    }

    if (!is_open_) {
        return;
    }

    // 2. Oscuramento morbido dello sfondo di gioco in pausa
    DrawRectangle(0, 0, eco::kWindowWidth, eco::kWindowHeight, Color{5, 10, 18, 198});

    if (!in_settings_) {
        // --- DISEGNO MENU DI PAUSA ---
        const int box_w = 550;
        const int box_h = 468;
        const int bx = (eco::kWindowWidth - box_w) / 2;
        const int by = (eco::kWindowHeight - box_h) / 2;

        DrawPanelWithHeader(
            Rectangle{
                static_cast<float>(bx),
                static_cast<float>(by),
                static_cast<float>(box_w),
                static_cast<float>(box_h)},
            44,
            "PAUSA  |  LA CONCA DI MERIDIA",
            pal::kUiBorderBronze);

        constexpr std::array<const char*, 7> kLabels = {
            "Riprendi Esplorazione [ESC]",
            "Salva Partita Ora",
            "Diario: Cronologia & Codex [H / J]",
            "Impostazioni Schermo, GPU & Audio",
            "Ricomincia Viaggio (Reset Conca)",
            "Salva e Torna al Titolo",
            "Salva ed Esci dal Gioco"
        };

        for (int i = 0; i < static_cast<int>(kLabels.size()); ++i) {
            const bool selected = (pause_cursor_ == i);
            const Rectangle btn{
                static_cast<float>(bx + 36),
                static_cast<float>(by + 68 + i * 50),
                static_cast<float>(box_w - 72),
                40.0f
            };
            const Color accent = (i == 6)
                                     ? Color{236, 108, 76, 255}
                                     : ((i == 1) ? pal::kJadeGlow
                                                 : ((i == 2) ? pal::kBronzeBright : pal::kAccentClean));
            DrawSectionCard(btn, selected ? accent : pal::kUiBorderBronze, selected);

            const char* label = kLabels[static_cast<std::size_t>(i)];
            const int tw = MeasureUiText(label, 16);
            DrawTextShadow(
                label,
                bx + (box_w - tw) / 2,
                static_cast<int>(btn.y) + 10,
                16,
                selected ? RAYWHITE : pal::kParchmentText);
        }

        const char* hint = "Usa [W/S], [Frecce] o Mouse | [INVIO] Conferma | [F11] Schermo Intero";
        const int hw = MeasureUiText(hint, 13);
        DrawTextShadow(hint, bx + (box_w - hw) / 2, by + box_h - 26, 13, Color{154, 192, 208, 225});
    } else {
        // --- DISEGNO SOTTOMENU IMPOSTAZIONI SCHERMO, GPU & AUDIO ---
        const int box_w = 820;
        const int box_h = 594;
        const int bx = (eco::kWindowWidth - box_w) / 2;
        const int by = (eco::kWindowHeight - box_h) / 2;

        DrawPanelWithHeader(
            Rectangle{
                static_cast<float>(bx),
                static_cast<float>(by),
                static_cast<float>(box_w),
                static_cast<float>(box_h)},
            44,
            "IMPOSTAZIONI SCHERMO, GPU, GRAFICA & AUDIO",
            pal::kAccentClean);

        const int res_idx = std::clamp(settings.resolution_index, 0, static_cast<int>(kResolutions.size()) - 1);
        const int fps_idx = std::clamp(settings.fps_index, 0, static_cast<int>(kFpsOptions.size()) - 1);
        const int vol_idx = std::clamp(settings.audio_volume_index, 0, static_cast<int>(kVolumeOptions.size()) - 1);

        const std::string monitor_str = FormatMonitorLabel(settings.monitor_index);
        const std::string gpu_str = FormatGpuLabel(settings.gpu_preference);

        const std::array<const char*, 10> row_titles = {
            "Modalita' Schermo [F11]",
            "Monitor Schermo Intero",
            "GPU / Scheda Grafica",
            "Risoluzione Finestra",
            "Frequenza Fotogrammi (FPS)",
            "Particelle & Effetti Atmosferici",
            "Stile Font Interfaccia",
            "Contatore FPS a Schermo",
            "Volume Audio [M Muto]",
            "Paesaggio Sonoro"
        };

        const std::array<const char*, 10> row_values = {
            settings.fullscreen ? "< Schermo Intero >" : "< Finestra Ridimensionabile >",
            monitor_str.c_str(),
            gpu_str.c_str(),
            kResolutions[static_cast<std::size_t>(res_idx)].label,
            kFpsOptions[static_cast<std::size_t>(fps_idx)].label,
            ParticleQualityLabel(settings.particle_quality),
            settings.clean_ttf_font ? "< Noto Sans HD (Nitido) >" : "< Pixel Bitmap (Retro) >",
            settings.show_fps ? "< Attivo (Mostra FPS) >" : "< Disattivato >",
            kVolumeOptions[static_cast<std::size_t>(vol_idx)].label,
            settings.ambient_audio ? "< Ambiente + Interazioni >" : "< Solo Interazioni >"
        };

        for (int i = 0; i < 10; ++i) {
            const bool selected = (settings_cursor_ == i);
            const Rectangle row_rect{
                static_cast<float>(bx + 26),
                static_cast<float>(by + 60 + i * 42),
                static_cast<float>(box_w - 52),
                35.0f
            };

            DrawSectionCard(row_rect, selected ? pal::kAccentClean : pal::kUiBorderBronze, selected);

            DrawTextShadow(
                row_titles[static_cast<std::size_t>(i)],
                static_cast<int>(row_rect.x) + 14,
                static_cast<int>(row_rect.y) + 8,
                15,
                selected ? RAYWHITE : pal::kParchmentText);

            const char* val_text = row_values[static_cast<std::size_t>(i)];
            const int vw = MeasureUiText(val_text, 14);
            DrawTextShadow(
                val_text,
                static_cast<int>(row_rect.x + row_rect.width) - vw - 14,
                static_cast<int>(row_rect.y) + 9,
                14,
                selected ? pal::kJadeGlow : pal::kBronzeBright);
        }

        // Riga informativa GPU OpenGL attualmente attiva
        const bool gpu_changed = (settings.gpu_preference != settings.initial_gpu_preference);
        const std::string gpu_status =
            std::string("GPU Attiva Ora: ") +
            (settings.active_gpu_renderer.empty() ? std::string("OpenGL") : settings.active_gpu_renderer) +
            (gpu_changed ? "   [Premi 'Applica GPU & Riavvia' per commutare]" : "");
        DrawTextShadow(
            gpu_status.c_str(),
            bx + 28,
            by + box_h - 88,
            13,
            gpu_changed ? pal::kAmberGlow : Color{148, 206, 214, 230});

        // Pulsanti in basso
        const int back_row_index = gpu_changed ? 11 : 10;
        const bool back_selected = (settings_cursor_ == back_row_index);
        const Rectangle back_rect{
            static_cast<float>(bx + 26),
            static_cast<float>(by + box_h - 52),
            240.0f,
            36.0f
        };
        DrawSectionCard(back_rect, back_selected ? pal::kAccentClean : pal::kUiBorderCyan, back_selected);
        const char* back_label = IsTitleStateActive(ctx) ? "[ESC] Torna al Titolo" : "[ESC] Torna alla Pausa";
        DrawTextShadow(back_label, static_cast<int>(back_rect.x) + 20, static_cast<int>(back_rect.y) + 9, 15, RAYWHITE);

        if (gpu_changed) {
            const bool apply_selected = (settings_cursor_ == 10);
            const Rectangle apply_rect{
                static_cast<float>(bx + 282),
                static_cast<float>(by + box_h - 52),
                275.0f,
                36.0f
            };
            DrawSectionCard(apply_rect, apply_selected ? pal::kJadeGlow : pal::kAmberGlow, apply_selected);
            DrawTextShadow(
                "Applica GPU & Riavvia",
                static_cast<int>(apply_rect.x) + 34,
                static_cast<int>(apply_rect.y) + 9,
                15,
                RAYWHITE);
        }

        const char* hint = "[A/D] o Click modifica | [M] Muto";
        const int hw = MeasureUiText(hint, 13);
        DrawTextShadow(hint, bx + box_w - hw - 26, by + box_h - 42, 13, Color{155, 195, 215, 225});
    }
}

} // namespace eco::app
