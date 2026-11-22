#pragma once

#include "app/game_state.h"

namespace eco::app {

enum class ParticleQuality : int {
    High = 0,   // 100% (Cenere, lucciole, foglie, god-rays)
    Medium = 1, // 50%
    Off = 2     // 0% (Disattivate)
};

struct GraphicsSettings {
    bool fullscreen = false;
    int resolution_index = 0; // 0: 1280x720, 1: 1600x900, 2: 1920x1080
    int fps_index = 0;        // 0: 60 FPS, 1: 120 FPS, 2: 144 FPS, 3: Illimitato
    ParticleQuality particle_quality = ParticleQuality::High;
    bool clean_ttf_font = true;
    bool show_fps = false;
    bool compact_tutorial = false; // true = dialoghi veloci + solo prima battuta essenziale per evento
};

void ApplyGraphicsSettingsToRuntime(const GraphicsSettings& settings);
void ToggleAppFullscreen(GraphicsSettings& settings);

class PauseSettingsMenu {
public:
    void OpenPause() noexcept;
    void OpenSettingsDirect() noexcept;
    void Close() noexcept;

    [[nodiscard]] bool IsOpen() const noexcept { return is_open_; }
    [[nodiscard]] bool IsInSettingsSubmenu() const noexcept { return in_settings_; }

    // Gestisce navigazione Tastiera/Mouse/ESC/F11.
    // Restituisce true se il menu e' attualmente aperto (e quindi il gioco sotto va messo in pausa).
    bool UpdateAndHandleInput(AppContext& ctx, GraphicsSettings& settings);

    // Disegna l'overlay del Menu di Pausa o delle Impostazioni Grafiche in spazio virtuale 1280x720,
    // piu' l'eventuale badge FPS in alto a destra.
    void DrawOverlay(const AppContext& ctx, const GraphicsSettings& settings) const;

private:
    bool is_open_ = false;
    bool in_settings_ = false;
    bool opened_from_title_ = false;
    int pause_cursor_ = 0;
    int settings_cursor_ = 0;
};

} // namespace eco::app
