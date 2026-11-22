#pragma once

#include <string>

#include "app/game_state.h"

namespace eco::app {

class JournalOverlay;

enum class ParticleQuality : int {
    High = 0,   // 100% (Polline, lucciole, cenere, vapore, raggi solari)
    Medium = 1, // 50%
    Off = 2     // 0% (Disattivate)
};

struct GraphicsSettings {
    bool fullscreen = false;
    int monitor_index = -1;   // -1: Auto (Monitor corrente), 0..N-1: Monitor specifico
    int resolution_index = 1; // 0: 1024x576, 1: 1280x720, 2: 1600x900, 3: 1920x1080
    int fps_index = 0;        // 0: 60 FPS, 1: 120 FPS, 2: 144 FPS, 3: Illimitato
    ParticleQuality particle_quality = ParticleQuality::High;
    bool clean_ttf_font = true;
    bool show_fps = false;
    bool compact_tutorial = false; // true = avanzamento dialoghi piu' rapido
    int audio_volume_index = 3;    // 0: 0% (Muto), 1: 25%, 2: 50%, 3: 75%, 4: 100%
    int music_volume_index = 4;
    int effects_volume_index = 4;
    bool ambient_audio = true;     // true: Ambiente + Interazioni, false: Solo Interazioni
    std::string gpu_preference = "auto";
    std::string initial_gpu_preference = "auto";
    std::string active_gpu_renderer{};
};

[[nodiscard]] inline const char* DefaultSettingsFilePath() noexcept {
    return "saves/settings.cfg";
}

bool LoadGraphicsSettings(
    GraphicsSettings& settings,
    const std::string& path = DefaultSettingsFilePath());
bool SaveGraphicsSettings(
    const GraphicsSettings& settings,
    const std::string& path = DefaultSettingsFilePath());

[[nodiscard]] int ResolveTargetMonitor(const GraphicsSettings& settings) noexcept;
void ApplyGraphicsSettingsToRuntime(const GraphicsSettings& settings);
void ApplySelectedMonitor(GraphicsSettings& settings);
void ApplySafeWindowResolution(GraphicsSettings& settings);
void ToggleAppFullscreen(GraphicsSettings& settings);

class PauseSettingsMenu {
public:
    enum class SettingsCategory : int {
        Game = 0,
        Graphics = 1,
        Audio = 2
    };

    void OpenPause() noexcept;
    void OpenSettingsDirect() noexcept;
    void Close() noexcept;
    void TriggerSaveFeedback(const char* message = "Partita salvata!") noexcept;

    [[nodiscard]] bool IsOpen() const noexcept { return is_open_; }
    [[nodiscard]] bool IsInSettingsSubmenu() const noexcept { return in_settings_; }

    // Gestisce navigazione Tastiera/Mouse/ESC/F11/M.
    // Restituisce true se il menu e' attualmente aperto (e quindi il gioco sotto va messo in pausa).
    bool UpdateAndHandleInput(
        AppContext& ctx,
        GraphicsSettings& settings,
        JournalOverlay* journal = nullptr);

    // Disegna l'overlay del Menu di Pausa o delle Impostazioni in spazio virtuale 1280x720,
    // piu' l'eventuale badge FPS in alto a destra e il toast di salvataggio.
    void DrawOverlay(const AppContext& ctx, const GraphicsSettings& settings) const;

private:
    bool is_open_ = false;
    bool in_settings_ = false;
    bool opened_from_title_ = false;
    int pause_cursor_ = 0;
    int settings_cursor_ = 0;
    SettingsCategory settings_category_ = SettingsCategory::Game;
    float save_feedback_timer_ = 0.0f;
    std::string save_feedback_msg_{};
};

} // namespace eco::app
