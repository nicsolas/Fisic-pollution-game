#pragma once

#include <array>

#include "core/input_types.h"

namespace eco::app {

enum class ControllerType : int {
    None = 0,
    Xbox = 1,
    PlayStation = 2,
    Switch = 3,
    Generic = 4
};

enum class GamepadAction : int {
    Jump = 0,
    Interact = 1,
    Scan = 2,
    Sprint = 3,
    Confirm = 4,
    Cancel = 5,
    Bypass = 6,
    Pause = 7,
    Journal = 8,
    HudPhase = 9,
    Count = 10
};

enum class GamepadButtonCode : int {
    FaceDown = 0,    // Xbox A | PS Cross | Switch B
    FaceRight = 1,   // Xbox B | PS Circle | Switch A
    FaceLeft = 2,    // Xbox X | PS Square | Switch Y
    FaceUp = 3,      // Xbox Y | PS Triangle | Switch X
    LB = 4,          // Xbox LB | PS L1 | Switch L
    RB = 5,          // Xbox RB | PS R1 | Switch R
    LT = 6,          // Xbox LT | PS L2 | Switch ZL
    RT = 7,          // Xbox RT | PS R2 | Switch ZR
    L3 = 8,          // Left Stick Click
    R3 = 9,          // Right Stick Click
    MiddleRight = 10,// Xbox Start | PS Options | Switch +
    MiddleLeft = 11, // Xbox View | PS Share | Switch -
    Count = 12
};

struct ControllerProfileConfig {
    std::array<GamepadButtonCode, static_cast<std::size_t>(GamepadAction::Count)> bindings{};
};

struct ControllerSettingsRuntime {
    int profile_mode = 0; // 0: Auto, 1: Xbox, 2: PlayStation, 3: Switch
    bool switch_invert_ab = false;
    float stick_deadzone = eco::kGamepadDeadzone;
    std::array<ControllerProfileConfig, 3> profiles{}; // 0: Xbox, 1: PlayStation, 2: Switch
};

struct ControllerPrompts {
    bool controller_active = false;
    ControllerType type = ControllerType::None;
    const char* device_name = "Tastiera/Mouse";
    const char* confirm = "[INVIO]";
    const char* cancel = "[ESC]";
    const char* interact = "[E]";
    const char* scan = "[F]";
    const char* jump = "[SPAZIO]";
    const char* sprint = "[SHIFT]";
    const char* bypass = "[B]";
    const char* pause = "[ESC]";
    const char* journal = "[H/J]";
    const char* tab_prev_next = "[Q/E]";
};

struct MenuInputState {
    bool up_pressed = false;
    bool down_pressed = false;
    bool left_pressed = false;
    bool right_pressed = false;

    bool up_held = false;
    bool down_held = false;
    bool left_held = false;
    bool right_held = false;

    bool confirm_pressed = false;
    bool cancel_pressed = false;
    bool pause_pressed = false;
    bool journal_pressed = false;
    bool tab_prev_pressed = false;
    bool tab_next_pressed = false;
    bool secondary_pressed = false; // Y / Triangolo / X (es. Reset / Bypass)

    float scroll_y = 0.0f; // Right stick Y per scroll fluido di liste/cronologia
};

// Restituisce il profilo di default per Xbox, PlayStation o Switch.
[[nodiscard]] ControllerProfileConfig DefaultBindingsFor(ControllerType type) noexcept;

// Imposta / legge la configurazione runtime dei controller (profili, deadzone, mapping).
void ApplyControllerSettings(const ControllerSettingsRuntime& cfg) noexcept;
[[nodiscard]] const ControllerSettingsRuntime& GetControllerSettings() noexcept;

// Nomi leggibili per azioni e pulsanti specifici per Xbox / PlayStation / Switch.
[[nodiscard]] const char* GamepadActionLabel(GamepadAction action) noexcept;
[[nodiscard]] const char* ControllerButtonLabel(ControllerType type, GamepadButtonCode btn) noexcept;

// Rileva il primo gamepad disponibile (slot 0..3), oppure -1 se nessuno.
[[nodiscard]] int FindPrimaryGamepad() noexcept;

// Classifica il controller in Xbox / PlayStation / Switch / Generic dal nome Raylib.
[[nodiscard]] ControllerType DetectControllerType(int gamepad_index) noexcept;

// Rileva se un pulsante del controller e' stato appena premuto (per il remapping interattivo).
[[nodiscard]] bool PollAnyPressedControllerButton(GamepadButtonCode& out_btn) noexcept;

struct ActionIconInfo {
    bool use_controller = false;
    ControllerType type = ControllerType::None;
    GamepadButtonCode button = GamepadButtonCode::FaceDown;
};

// Restituisce se mostrare l'icona del controller e quale pulsante e' associato all'azione.
[[nodiscard]] ActionIconInfo GetActionIconInfo(GamepadAction action) noexcept;

// Restituisce i prompt adattivi correnti (aggiornati automaticamente in base all'ultimo input).
[[nodiscard]] ControllerPrompts GetActiveControllerPrompts() noexcept;

// Polls Raylib keyboard + gamepads 0..3 into AllPlayersInput.
// Must only be called after InitWindow() (never in headless mode).
[[nodiscard]] eco::AllPlayersInput PollRaylibInput();

// Polls menu navigation (Keyboard + D-Pad + Left Stick con auto-repeat + Xbox/PS/Switch buttons).
// Must only be called after InitWindow().
[[nodiscard]] MenuInputState PollMenuInput(float dt) noexcept;

} // namespace eco::app
