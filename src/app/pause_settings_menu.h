#pragma once

#include <array>
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
    // Schermo & Grafica
    bool fullscreen = false;
    int monitor_index = -1;   // -1: Auto (Monitor corrente), 0..N-1: Monitor specifico
    int resolution_index = 1; // 0: 1024x576, 1: 1280x720, 2: 1600x900, 3: 1920x1080
    int fps_index = 0;        // 0: 60 FPS, 1: 120 FPS, 2: 144 FPS, 3: Illimitato
    ParticleQuality particle_quality = ParticleQuality::High;
    bool clean_ttf_font = true;
    std::string gpu_preference = "auto";
    std::string initial_gpu_preference = "auto";
    std::string active_gpu_renderer{};

    // Gioco & Interfaccia
    bool show_fps = false;
    bool compact_tutorial = false; // true = avanzamento dialoghi piu' rapido
    int dialogue_speed_index = 1;  // 0: Lento, 1: Normale, 2: Rapido

    // Effetti Ambientali granulari (0.0f .. 1.0f)
    float particle_density = 1.0f;       // Densita' particelle globali
    float fog_haze_intensity = 1.0f;     // Intensita' foschia/nebbia atmosferica
    float sun_rays_intensity = 1.0f;     // Raggi solari / luce volumetrica
    float vignette_intensity = 1.0f;     // Vignettatura atmosferica
    float zone_weather_intensity = 1.0f; // Meteo/effetti di zona (vento, vapore, polline, scintille)
    bool atmosphere_enabled = true;      // Master toggle atmosfera screen-space

    // Audio & Suoni granulari (0.0f .. 1.0f)
    bool audio_muted = false;       // Mute rapido [M] che preserva master_volume
    float master_volume = 0.75f;    // Volume generale
    float music_volume = 0.85f;     // Volume musica / tappeto armonico
    float effects_volume = 0.90f;   // Volume effetti / interazioni & meccaniche
    float ambient_volume = 0.85f;   // Volume ambiente / natura (vento, acqua, vapore)
    bool ambient_audio = true;      // Paesaggio sonoro ambientale attivo/disattivo
    bool ui_sounds_enabled = true;  // Suoni interfaccia (menu, click, tab) on/off

    // Controller (Xbox / PlayStation / Switch)
    int controller_profile_mode = 0; // 0: Auto, 1: Xbox, 2: PlayStation, 3: Switch
    bool switch_invert_ab = false;   // Scambia A/B su Switch
    float controller_deadzone = 0.20f;
    std::array<std::array<int, 10>, 3> controller_bindings{}; // [0:Xbox, 1:PS, 2:Switch][action]
    bool controller_bindings_initialized = false;
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
        Environment = 2,
        Audio = 3,
        Controller = 4
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
    int hovered_pause_row_ = -1;
    int hovered_settings_row_ = -1;
    int hovered_tab_ = -1;
    int dragging_slider_row_ = -1;
    SettingsCategory settings_category_ = SettingsCategory::Game;
    int controller_edit_profile_ = 0; // 0: Xbox, 1: PlayStation, 2: Switch
    int listening_action_ = -1;       // -1 = nessuno, 0..9 = azione in attesa di tasto
    float save_feedback_timer_ = 0.0f;
    std::string save_feedback_msg_{};
};

} // namespace eco::app
