#include "app/raylib_input.h"

#include <raylib.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <string_view>

namespace eco::app {

namespace {

constexpr float kMenuStickThreshold = 0.52f;
constexpr float kMenuRepeatInitialDelay = 0.32f;
constexpr float kMenuRepeatInterval = 0.11f;

bool g_last_input_was_controller = false;
ControllerType g_last_controller_type = ControllerType::None;

ControllerSettingsRuntime InitDefaultControllerSettings() noexcept {
    ControllerSettingsRuntime cfg{};
    cfg.profile_mode = 0;
    cfg.switch_invert_ab = false;
    cfg.stick_deadzone = eco::kGamepadDeadzone;
    cfg.profiles[0] = DefaultBindingsFor(ControllerType::Xbox);
    cfg.profiles[1] = DefaultBindingsFor(ControllerType::PlayStation);
    cfg.profiles[2] = DefaultBindingsFor(ControllerType::Switch);
    return cfg;
}

ControllerSettingsRuntime g_ctrl_settings = InitDefaultControllerSettings();
bool g_ctrl_settings_initialized = false;

void EnsureCtrlSettingsInit() noexcept {
    if (!g_ctrl_settings_initialized) {
        g_ctrl_settings = InitDefaultControllerSettings();
        g_ctrl_settings_initialized = true;
    }
}

struct RepeatAxisState {
    int prev_dir = 0;
    float timer = 0.0f;
    bool repeating = false;

    bool Step(int dir, float dt) noexcept {
        if (dir == 0) {
            prev_dir = 0;
            timer = 0.0f;
            repeating = false;
            return false;
        }
        if (dir != prev_dir) {
            prev_dir = dir;
            timer = 0.0f;
            repeating = false;
            return true;
        }
        timer += dt;
        const float threshold = repeating ? kMenuRepeatInterval : kMenuRepeatInitialDelay;
        if (timer >= threshold) {
            timer -= threshold;
            repeating = true;
            return true;
        }
        return false;
    }
};

RepeatAxisState g_menu_repeat_x{};
RepeatAxisState g_menu_repeat_y{};

std::array<bool, eco::kMaxPlayers> g_prev_lt_down{};
std::array<bool, eco::kMaxPlayers> g_prev_rt_down{};

[[nodiscard]] bool ContainsCaseInsensitive(std::string_view haystack, std::string_view needle) noexcept {
    if (needle.empty() || haystack.size() < needle.size()) {
        return false;
    }
    for (std::size_t i = 0; i + needle.size() <= haystack.size(); ++i) {
        bool match = true;
        for (std::size_t j = 0; j < needle.size(); ++j) {
            const unsigned char a = static_cast<unsigned char>(haystack[i + j]);
            const unsigned char b = static_cast<unsigned char>(needle[j]);
            if (std::tolower(a) != std::tolower(b)) {
                match = false;
                break;
            }
        }
        if (match) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] int ProfileIndexForType(ControllerType type) noexcept {
    switch (type) {
        case ControllerType::PlayStation:
            return 1;
        case ControllerType::Switch:
            return 2;
        case ControllerType::Xbox:
        case ControllerType::Generic:
        case ControllerType::None:
            return 0;
    }
    return 0;
}

[[nodiscard]] ControllerType ResolveEffectiveType(int gp) noexcept {
    EnsureCtrlSettingsInit();
    if (g_ctrl_settings.profile_mode == 1) {
        return ControllerType::Xbox;
    }
    if (g_ctrl_settings.profile_mode == 2) {
        return ControllerType::PlayStation;
    }
    if (g_ctrl_settings.profile_mode == 3) {
        return ControllerType::Switch;
    }
    return DetectControllerType(gp);
}

[[nodiscard]] int RaylibButtonForCode(GamepadButtonCode code) noexcept {
    switch (code) {
        case GamepadButtonCode::FaceDown:    return GAMEPAD_BUTTON_RIGHT_FACE_DOWN;
        case GamepadButtonCode::FaceRight:   return GAMEPAD_BUTTON_RIGHT_FACE_RIGHT;
        case GamepadButtonCode::FaceLeft:    return GAMEPAD_BUTTON_RIGHT_FACE_LEFT;
        case GamepadButtonCode::FaceUp:      return GAMEPAD_BUTTON_RIGHT_FACE_UP;
        case GamepadButtonCode::LB:          return GAMEPAD_BUTTON_LEFT_TRIGGER_1;
        case GamepadButtonCode::RB:          return GAMEPAD_BUTTON_RIGHT_TRIGGER_1;
        case GamepadButtonCode::LT:          return GAMEPAD_BUTTON_LEFT_TRIGGER_2;
        case GamepadButtonCode::RT:          return GAMEPAD_BUTTON_RIGHT_TRIGGER_2;
        case GamepadButtonCode::L3:          return GAMEPAD_BUTTON_LEFT_THUMB;
        case GamepadButtonCode::R3:          return GAMEPAD_BUTTON_RIGHT_THUMB;
        case GamepadButtonCode::MiddleRight: return GAMEPAD_BUTTON_MIDDLE_RIGHT;
        case GamepadButtonCode::MiddleLeft:  return GAMEPAD_BUTTON_MIDDLE_LEFT;
        case GamepadButtonCode::Count:       break;
    }
    return GAMEPAD_BUTTON_RIGHT_FACE_DOWN;
}

[[nodiscard]] bool IsCodeDown(int gp, GamepadButtonCode code) noexcept {
    if (gp < 0 || gp >= eco::kMaxPlayers || !IsGamepadAvailable(gp)) {
        return false;
    }
    if (code == GamepadButtonCode::LT) {
        const float axis = GetGamepadAxisMovement(gp, GAMEPAD_AXIS_LEFT_TRIGGER);
        return IsGamepadButtonDown(gp, GAMEPAD_BUTTON_LEFT_TRIGGER_2) ||
               (std::isfinite(axis) && axis > 0.35f);
    }
    if (code == GamepadButtonCode::RT) {
        const float axis = GetGamepadAxisMovement(gp, GAMEPAD_AXIS_RIGHT_TRIGGER);
        return IsGamepadButtonDown(gp, GAMEPAD_BUTTON_RIGHT_TRIGGER_2) ||
               (std::isfinite(axis) && axis > 0.35f);
    }
    return IsGamepadButtonDown(gp, RaylibButtonForCode(code));
}

[[nodiscard]] bool IsCodePressed(int gp, GamepadButtonCode code) noexcept {
    if (gp < 0 || gp >= eco::kMaxPlayers || !IsGamepadAvailable(gp)) {
        return false;
    }
    if (code == GamepadButtonCode::LT) {
        const bool down = IsCodeDown(gp, GamepadButtonCode::LT);
        return IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_LEFT_TRIGGER_2) ||
               (down && !g_prev_lt_down[static_cast<std::size_t>(gp)]);
    }
    if (code == GamepadButtonCode::RT) {
        const bool down = IsCodeDown(gp, GamepadButtonCode::RT);
        return IsGamepadButtonPressed(gp, GAMEPAD_BUTTON_RIGHT_TRIGGER_2) ||
               (down && !g_prev_rt_down[static_cast<std::size_t>(gp)]);
    }
    return IsGamepadButtonPressed(gp, RaylibButtonForCode(code));
}

void UpdateTriggerEdgeCache() noexcept {
    for (int gp = 0; gp < eco::kMaxPlayers; ++gp) {
        if (IsGamepadAvailable(gp)) {
            g_prev_lt_down[static_cast<std::size_t>(gp)] = IsCodeDown(gp, GamepadButtonCode::LT);
            g_prev_rt_down[static_cast<std::size_t>(gp)] = IsCodeDown(gp, GamepadButtonCode::RT);
        } else {
            g_prev_lt_down[static_cast<std::size_t>(gp)] = false;
            g_prev_rt_down[static_cast<std::size_t>(gp)] = false;
        }
    }
}

[[nodiscard]] GamepadButtonCode BindingFor(ControllerType type, GamepadAction action) noexcept {
    EnsureCtrlSettingsInit();
    const int prof_idx = ProfileIndexForType(type);
    const auto act_idx = static_cast<std::size_t>(action);
    GamepadButtonCode code = g_ctrl_settings.profiles[static_cast<std::size_t>(prof_idx)].bindings[act_idx];
    if (type == ControllerType::Switch && g_ctrl_settings.switch_invert_ab) {
        if (code == GamepadButtonCode::FaceRight) {
            code = GamepadButtonCode::FaceDown;
        } else if (code == GamepadButtonCode::FaceDown) {
            code = GamepadButtonCode::FaceRight;
        }
    }
    return code;
}

[[nodiscard]] bool IsActionPressed(int gp, ControllerType type, GamepadAction action) noexcept {
    return IsCodePressed(gp, BindingFor(type, action));
}

[[nodiscard]] bool IsActionHeld(int gp, ControllerType type, GamepadAction action) noexcept {
    return IsCodeDown(gp, BindingFor(type, action));
}

[[nodiscard]] bool IsAnyKeyboardOrMouseUsed() noexcept {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
        IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) ||
        std::abs(GetMouseDelta().x) > 1.5f ||
        std::abs(GetMouseDelta().y) > 1.5f ||
        std::abs(GetMouseWheelMove()) > 0.01f) {
        return true;
    }
    return IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D) ||
           IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT) ||
           IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE) ||
           IsKeyPressed(KEY_E) || IsKeyPressed(KEY_F) || IsKeyPressed(KEY_Q) ||
           IsKeyPressed(KEY_T) || IsKeyPressed(KEY_R) || IsKeyPressed(KEY_B) || IsKeyPressed(KEY_I) ||
           IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_THREE) ||
           IsKeyPressed(KEY_H) || IsKeyPressed(KEY_J) || IsKeyPressed(KEY_V) || IsKeyPressed(KEY_N) ||
           IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
}

[[nodiscard]] bool IsAnyGamepadActivity(int gp) noexcept {
    if (gp < 0 || !IsGamepadAvailable(gp)) {
        return false;
    }
    if (GetGamepadButtonPressed() != GAMEPAD_BUTTON_UNKNOWN) {
        return true;
    }
    const float dz = std::clamp(g_ctrl_settings.stick_deadzone, 0.10f, 0.45f);
    const float lx = GetGamepadAxisMovement(gp, GAMEPAD_AXIS_LEFT_X);
    const float ly = GetGamepadAxisMovement(gp, GAMEPAD_AXIS_LEFT_Y);
    const float rx = GetGamepadAxisMovement(gp, GAMEPAD_AXIS_RIGHT_X);
    const float ry = GetGamepadAxisMovement(gp, GAMEPAD_AXIS_RIGHT_Y);
    return std::abs(lx) > dz || std::abs(ly) > dz ||
           std::abs(rx) > dz || std::abs(ry) > dz ||
           IsCodeDown(gp, GamepadButtonCode::LT) || IsCodeDown(gp, GamepadButtonCode::RT);
}

void UpdateActiveInputDevice(int primary_gp) noexcept {
    EnsureCtrlSettingsInit();
    if (primary_gp >= 0 && IsGamepadAvailable(primary_gp)) {
        g_last_controller_type = ResolveEffectiveType(primary_gp);
        if (IsAnyGamepadActivity(primary_gp)) {
            g_last_input_was_controller = true;
            return;
        }
    } else {
        g_last_controller_type = ControllerType::None;
        g_last_input_was_controller = false;
        return;
    }
    if (IsAnyKeyboardOrMouseUsed()) {
        g_last_input_was_controller = false;
    }
}

[[nodiscard]] eco::PlayerInput PollSingleGamepadOnly(int gp, ControllerType type) noexcept {
    eco::PlayerInput out{};
    if (gp < 0 || !IsGamepadAvailable(gp)) {
        return out;
    }
    const eco::Vec2 gp_axis{
        GetGamepadAxisMovement(gp, GAMEPAD_AXIS_LEFT_X),
        GetGamepadAxisMovement(gp, GAMEPAD_AXIS_LEFT_Y)
    };
    const bool dpad_left = IsGamepadButtonDown(gp, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
    const bool dpad_right = IsGamepadButtonDown(gp, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
    const bool dpad_up = IsGamepadButtonDown(gp, GAMEPAD_BUTTON_LEFT_FACE_UP);
    const bool dpad_down = IsGamepadButtonDown(gp, GAMEPAD_BUTTON_LEFT_FACE_DOWN);

    const eco::Vec2 dpad_axis{
        eco::CancelOpposingKeys(dpad_left, dpad_right),
        eco::CancelOpposingKeys(dpad_up, dpad_down)
    };

    const bool jump_pressed = IsActionPressed(gp, type, GamepadAction::Jump);
    const bool jump_held = IsActionHeld(gp, type, GamepadAction::Jump);
    const bool interact_pressed = IsActionPressed(gp, type, GamepadAction::Interact);
    const bool scan_pressed = IsActionPressed(gp, type, GamepadAction::Scan);
    const bool toggle_overlay_pressed = IsActionPressed(gp, type, GamepadAction::HudPhase);
    const bool confirm_pressed = IsActionPressed(gp, type, GamepadAction::Confirm);
    const bool cancel_pressed = IsActionPressed(gp, type, GamepadAction::Cancel);
    const bool bypass_pressed = IsActionPressed(gp, type, GamepadAction::Bypass);
    const bool trial_pressed = scan_pressed;
    const bool risky_pressed = IsCodePressed(gp, GamepadButtonCode::RT);
    const int hypothesis_delta = IsCodePressed(gp, GamepadButtonCode::LT) ? 1 : 0;
    const float ry = GetGamepadAxisMovement(gp, GAMEPAD_AXIS_RIGHT_Y);

    out = eco::FilterPlayerInput(
        dpad_axis,
        gp_axis,
        jump_pressed,
        jump_held,
        interact_pressed,
        scan_pressed,
        toggle_overlay_pressed,
        jump_pressed || interact_pressed || confirm_pressed,
        cancel_pressed,
        g_ctrl_settings.stick_deadzone);
    out.sprint_held = IsActionHeld(gp, type, GamepadAction::Sprint);
    out.confirm_pressed = confirm_pressed;
    out.bypass_pressed = bypass_pressed;
    out.trial_pressed = trial_pressed;
    out.risky_pressed = risky_pressed;
    out.hypothesis_delta = hypothesis_delta;
    out.slider_delta = eco::Clamp(out.move_x, -1.0f, 1.0f) * 0.75f;
    const float aux_raw =
        -dpad_axis.y -
        eco::ApplyAnalogDeadzone(gp_axis.y, g_ctrl_settings.stick_deadzone) -
        eco::ApplyAnalogDeadzone(ry, g_ctrl_settings.stick_deadzone);
    out.aux_slider_delta = eco::Clamp(aux_raw, -1.0f, 1.0f) * 0.75f;
    return out;
}

} // namespace

ControllerProfileConfig DefaultBindingsFor(ControllerType type) noexcept {
    ControllerProfileConfig cfg{};
    if (type == ControllerType::Switch) {
        cfg.bindings[static_cast<std::size_t>(GamepadAction::Jump)] = GamepadButtonCode::FaceDown;     // B
        cfg.bindings[static_cast<std::size_t>(GamepadAction::Interact)] = GamepadButtonCode::FaceRight;// A
        cfg.bindings[static_cast<std::size_t>(GamepadAction::Scan)] = GamepadButtonCode::FaceLeft;     // Y
        cfg.bindings[static_cast<std::size_t>(GamepadAction::Sprint)] = GamepadButtonCode::LB;         // L
        cfg.bindings[static_cast<std::size_t>(GamepadAction::Confirm)] = GamepadButtonCode::FaceRight; // A
        cfg.bindings[static_cast<std::size_t>(GamepadAction::Cancel)] = GamepadButtonCode::FaceDown;   // B
        cfg.bindings[static_cast<std::size_t>(GamepadAction::Bypass)] = GamepadButtonCode::FaceUp;     // X
        cfg.bindings[static_cast<std::size_t>(GamepadAction::Pause)] = GamepadButtonCode::MiddleRight; // +
        cfg.bindings[static_cast<std::size_t>(GamepadAction::Journal)] = GamepadButtonCode::MiddleLeft;// -
        cfg.bindings[static_cast<std::size_t>(GamepadAction::HudPhase)] = GamepadButtonCode::R3;       // RS
    } else {
        // Xbox & PlayStation
        cfg.bindings[static_cast<std::size_t>(GamepadAction::Jump)] = GamepadButtonCode::FaceDown;     // A / X
        cfg.bindings[static_cast<std::size_t>(GamepadAction::Interact)] = GamepadButtonCode::FaceRight;// B / O
        cfg.bindings[static_cast<std::size_t>(GamepadAction::Scan)] = GamepadButtonCode::FaceLeft;     // X / Sq
        cfg.bindings[static_cast<std::size_t>(GamepadAction::Sprint)] = GamepadButtonCode::LB;         // LB / L1
        cfg.bindings[static_cast<std::size_t>(GamepadAction::Confirm)] = GamepadButtonCode::FaceDown;  // A / X
        cfg.bindings[static_cast<std::size_t>(GamepadAction::Cancel)] = GamepadButtonCode::FaceRight;  // B / O
        cfg.bindings[static_cast<std::size_t>(GamepadAction::Bypass)] = GamepadButtonCode::FaceUp;     // Y / Tri
        cfg.bindings[static_cast<std::size_t>(GamepadAction::Pause)] = GamepadButtonCode::MiddleRight; // Start / Opt
        cfg.bindings[static_cast<std::size_t>(GamepadAction::Journal)] = GamepadButtonCode::MiddleLeft;// View / Share
        cfg.bindings[static_cast<std::size_t>(GamepadAction::HudPhase)] = GamepadButtonCode::R3;       // RS / R3
    }
    return cfg;
}

void ApplyControllerSettings(const ControllerSettingsRuntime& cfg) noexcept {
    g_ctrl_settings = cfg;
    g_ctrl_settings.stick_deadzone = std::clamp(g_ctrl_settings.stick_deadzone, 0.10f, 0.45f);
    g_ctrl_settings_initialized = true;
}

const ControllerSettingsRuntime& GetControllerSettings() noexcept {
    EnsureCtrlSettingsInit();
    return g_ctrl_settings;
}

const char* GamepadActionLabel(GamepadAction action) noexcept {
    switch (action) {
        case GamepadAction::Jump:     return "Salto (Strutture)";
        case GamepadAction::Interact: return "Interagisci / Parla";
        case GamepadAction::Scan:     return "Lente Empirica (Scan)";
        case GamepadAction::Sprint:   return "Sprint Leggero";
        case GamepadAction::Confirm:  return "Conferma Menu / Modal";
        case GamepadAction::Cancel:   return "Annulla / Indietro";
        case GamepadAction::Bypass:   return "Bypass / Avanza Dialogo";
        case GamepadAction::Pause:    return "Menu di Pausa";
        case GamepadAction::Journal:  return "Diario & Codex";
        case GamepadAction::HudPhase: return "Fase HUD / Rilievo";
        case GamepadAction::Count:    break;
    }
    return "";
}

const char* ControllerButtonLabel(ControllerType type, GamepadButtonCode btn) noexcept {
    if (type == ControllerType::PlayStation) {
        switch (btn) {
            case GamepadButtonCode::FaceDown:    return "[X]";
            case GamepadButtonCode::FaceRight:   return "[O]";
            case GamepadButtonCode::FaceLeft:    return "[Quad]";
            case GamepadButtonCode::FaceUp:      return "[Tri]";
            case GamepadButtonCode::LB:          return "[L1]";
            case GamepadButtonCode::RB:          return "[R1]";
            case GamepadButtonCode::LT:          return "[L2]";
            case GamepadButtonCode::RT:          return "[R2]";
            case GamepadButtonCode::L3:          return "[L3]";
            case GamepadButtonCode::R3:          return "[R3]";
            case GamepadButtonCode::MiddleRight: return "[OPTIONS]";
            case GamepadButtonCode::MiddleLeft:  return "[SHARE]";
            case GamepadButtonCode::Count:       break;
        }
    } else if (type == ControllerType::Switch) {
        switch (btn) {
            case GamepadButtonCode::FaceDown:    return "[B]";
            case GamepadButtonCode::FaceRight:   return "[A]";
            case GamepadButtonCode::FaceLeft:    return "[Y]";
            case GamepadButtonCode::FaceUp:      return "[X]";
            case GamepadButtonCode::LB:          return "[L]";
            case GamepadButtonCode::RB:          return "[R]";
            case GamepadButtonCode::LT:          return "[ZL]";
            case GamepadButtonCode::RT:          return "[ZR]";
            case GamepadButtonCode::L3:          return "[LS]";
            case GamepadButtonCode::R3:          return "[RS]";
            case GamepadButtonCode::MiddleRight: return "[+]";
            case GamepadButtonCode::MiddleLeft:  return "[-]";
            case GamepadButtonCode::Count:       break;
        }
    } else {
        // Xbox / Generic
        switch (btn) {
            case GamepadButtonCode::FaceDown:    return "[A]";
            case GamepadButtonCode::FaceRight:   return "[B]";
            case GamepadButtonCode::FaceLeft:    return "[X]";
            case GamepadButtonCode::FaceUp:      return "[Y]";
            case GamepadButtonCode::LB:          return "[LB]";
            case GamepadButtonCode::RB:          return "[RB]";
            case GamepadButtonCode::LT:          return "[LT]";
            case GamepadButtonCode::RT:          return "[RT]";
            case GamepadButtonCode::L3:          return "[LS]";
            case GamepadButtonCode::R3:          return "[RS]";
            case GamepadButtonCode::MiddleRight: return "[START]";
            case GamepadButtonCode::MiddleLeft:  return "[VIEW]";
            case GamepadButtonCode::Count:       break;
        }
    }
    return "[?]";
}

int FindPrimaryGamepad() noexcept {
    for (int gp = 0; gp < eco::kMaxPlayers; ++gp) {
        if (IsGamepadAvailable(gp)) {
            return gp;
        }
    }
    return -1;
}

ControllerType DetectControllerType(int gamepad_index) noexcept {
    if (gamepad_index < 0 || !IsGamepadAvailable(gamepad_index)) {
        return ControllerType::None;
    }
    const char* raw_name = GetGamepadName(gamepad_index);
    if (raw_name == nullptr || raw_name[0] == '\0') {
        return ControllerType::Generic;
    }
    const std::string_view name(raw_name);
    if (ContainsCaseInsensitive(name, "nintendo") ||
        ContainsCaseInsensitive(name, "switch") ||
        ContainsCaseInsensitive(name, "pro controller") ||
        ContainsCaseInsensitive(name, "joy-con") ||
        ContainsCaseInsensitive(name, "joycon") ||
        ContainsCaseInsensitive(name, "hori")) {
        return ControllerType::Switch;
    }
    if (ContainsCaseInsensitive(name, "playstation") ||
        ContainsCaseInsensitive(name, "dualsense") ||
        ContainsCaseInsensitive(name, "dualshock") ||
        ContainsCaseInsensitive(name, "sony") ||
        ContainsCaseInsensitive(name, "ps4") ||
        ContainsCaseInsensitive(name, "ps5") ||
        ContainsCaseInsensitive(name, "ps3")) {
        return ControllerType::PlayStation;
    }
    if (ContainsCaseInsensitive(name, "xbox") ||
        ContainsCaseInsensitive(name, "xinput") ||
        ContainsCaseInsensitive(name, "microsoft") ||
        ContainsCaseInsensitive(name, "x-box")) {
        return ControllerType::Xbox;
    }
    return ControllerType::Generic;
}

bool PollAnyPressedControllerButton(GamepadButtonCode& out_btn) noexcept {
    const int gp = FindPrimaryGamepad();
    if (gp < 0) {
        return false;
    }
    for (int b = 0; b < static_cast<int>(GamepadButtonCode::Count); ++b) {
        const auto code = static_cast<GamepadButtonCode>(b);
        if (IsCodePressed(gp, code)) {
            out_btn = code;
            UpdateTriggerEdgeCache();
            return true;
        }
    }
    return false;
}

ActionIconInfo GetActionIconInfo(GamepadAction action) noexcept {
    EnsureCtrlSettingsInit();
    ActionIconInfo info{};
    const int primary_gp = FindPrimaryGamepad();
    if (primary_gp >= 0 && g_last_input_was_controller) {
        info.use_controller = true;
        info.type = ResolveEffectiveType(primary_gp);
        info.button = BindingFor(info.type, action);
    }
    return info;
}

ControllerPrompts GetActiveControllerPrompts() noexcept {
    EnsureCtrlSettingsInit();
    ControllerPrompts p{};
    const int primary_gp = FindPrimaryGamepad();
    const ControllerType detected =
        (primary_gp >= 0) ? ResolveEffectiveType(primary_gp) : ControllerType::None;
    p.type = detected;
    p.controller_active = (primary_gp >= 0) && g_last_input_was_controller;

    if (!p.controller_active) {
        return p;
    }

    p.device_name = (detected == ControllerType::PlayStation)
                        ? "PlayStation"
                        : ((detected == ControllerType::Switch)
                               ? "Nintendo Switch"
                               : ((detected == ControllerType::Xbox) ? "Xbox" : "Controller"));
    p.confirm = ControllerButtonLabel(detected, BindingFor(detected, GamepadAction::Confirm));
    p.cancel = ControllerButtonLabel(detected, BindingFor(detected, GamepadAction::Cancel));
    p.interact = ControllerButtonLabel(detected, BindingFor(detected, GamepadAction::Interact));
    p.scan = ControllerButtonLabel(detected, BindingFor(detected, GamepadAction::Scan));
    p.jump = ControllerButtonLabel(detected, BindingFor(detected, GamepadAction::Jump));
    p.sprint = ControllerButtonLabel(detected, BindingFor(detected, GamepadAction::Sprint));
    p.bypass = ControllerButtonLabel(detected, BindingFor(detected, GamepadAction::Bypass));
    p.trial = p.scan;
    p.risky = ControllerButtonLabel(detected, GamepadButtonCode::RT);
    p.hypothesis = ControllerButtonLabel(detected, GamepadButtonCode::LT);
    p.pause = ControllerButtonLabel(detected, BindingFor(detected, GamepadAction::Pause));
    p.journal = ControllerButtonLabel(detected, BindingFor(detected, GamepadAction::Journal));
    p.tab_prev_next = (detected == ControllerType::PlayStation)
                          ? "[L1/R1]"
                          : ((detected == ControllerType::Switch) ? "[L/R]" : "[LB/RB]");
    return p;
}

eco::AllPlayersInput PollRaylibInput() {
    EnsureCtrlSettingsInit();
    eco::AllPlayersInput out{};

    const int primary_gp = FindPrimaryGamepad();
    UpdateActiveInputDevice(primary_gp);
    const ControllerType primary_type = ResolveEffectiveType(primary_gp);

    eco::Vec2 discrete_axis{0.0f, 0.0f};
    eco::Vec2 gamepad_axis{0.0f, 0.0f};
    float gamepad_right_y = 0.0f;

    const bool left = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
    const bool right = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
    const bool up = IsKeyDown(KEY_W) || IsKeyDown(KEY_Z) || IsKeyDown(KEY_UP);
    const bool down = IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN);

    discrete_axis.x = eco::CancelOpposingKeys(left, right);
    discrete_axis.y = eco::CancelOpposingKeys(up, down);

    bool jump_pressed = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W) ||
                        IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_UP);
    bool jump_held = IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_W) ||
                     IsKeyDown(KEY_Z) || IsKeyDown(KEY_UP);
    bool sprint_held = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    bool interact_pressed =
        IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    bool scan_pressed =
        IsKeyPressed(KEY_F) || IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_PERIOD);
    bool toggle_overlay_pressed = IsKeyPressed(KEY_V);
    bool confirm_pressed =
        IsKeyPressed(KEY_C) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    bool bypass_pressed = IsKeyPressed(KEY_B);
    bool trial_pressed = IsKeyPressed(KEY_T) || IsKeyPressed(KEY_SPACE);
    bool risky_pressed = IsKeyPressed(KEY_R);
    int hypothesis_delta = IsKeyPressed(KEY_I) ? 1 : 0;
    bool cancel_pressed = IsKeyPressed(KEY_X) || IsKeyPressed(KEY_BACKSPACE);

    if (primary_gp >= 0 && IsGamepadAvailable(primary_gp)) {
        gamepad_axis.x = GetGamepadAxisMovement(primary_gp, GAMEPAD_AXIS_LEFT_X);
        gamepad_axis.y = GetGamepadAxisMovement(primary_gp, GAMEPAD_AXIS_LEFT_Y);
        gamepad_right_y = GetGamepadAxisMovement(primary_gp, GAMEPAD_AXIS_RIGHT_Y);

        const bool dpad_left = IsGamepadButtonDown(primary_gp, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
        const bool dpad_right = IsGamepadButtonDown(primary_gp, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
        const bool dpad_up = IsGamepadButtonDown(primary_gp, GAMEPAD_BUTTON_LEFT_FACE_UP);
        const bool dpad_down = IsGamepadButtonDown(primary_gp, GAMEPAD_BUTTON_LEFT_FACE_DOWN);

        discrete_axis.x += eco::CancelOpposingKeys(dpad_left, dpad_right);
        discrete_axis.y += eco::CancelOpposingKeys(dpad_up, dpad_down);

        jump_pressed = jump_pressed || IsActionPressed(primary_gp, primary_type, GamepadAction::Jump);
        jump_held = jump_held || IsActionHeld(primary_gp, primary_type, GamepadAction::Jump);
        sprint_held = sprint_held || IsActionHeld(primary_gp, primary_type, GamepadAction::Sprint);
        interact_pressed =
            interact_pressed || IsActionPressed(primary_gp, primary_type, GamepadAction::Interact);
        scan_pressed =
            scan_pressed || IsActionPressed(primary_gp, primary_type, GamepadAction::Scan);
        toggle_overlay_pressed =
            toggle_overlay_pressed || IsActionPressed(primary_gp, primary_type, GamepadAction::HudPhase);
        cancel_pressed =
            cancel_pressed || IsActionPressed(primary_gp, primary_type, GamepadAction::Cancel);
        confirm_pressed =
            confirm_pressed || IsActionPressed(primary_gp, primary_type, GamepadAction::Confirm);
        bypass_pressed =
            bypass_pressed || IsActionPressed(primary_gp, primary_type, GamepadAction::Bypass);
        trial_pressed =
            trial_pressed || IsActionPressed(primary_gp, primary_type, GamepadAction::Scan);
        risky_pressed =
            risky_pressed || IsCodePressed(primary_gp, GamepadButtonCode::RT);
        if (IsCodePressed(primary_gp, GamepadButtonCode::LT)) {
            hypothesis_delta += 1;
        }
    }

    discrete_axis.x = eco::Clamp(discrete_axis.x, -1.0f, 1.0f);
    discrete_axis.y = eco::Clamp(discrete_axis.y, -1.0f, 1.0f);

    eco::PlayerInput filtered = eco::FilterPlayerInput(
        discrete_axis,
        gamepad_axis,
        jump_pressed,
        jump_held,
        interact_pressed,
        scan_pressed,
        toggle_overlay_pressed,
        false,
        cancel_pressed,
        g_ctrl_settings.stick_deadzone);
    filtered.sprint_held = sprint_held;
    filtered.confirm_pressed = confirm_pressed;
    filtered.bypass_pressed = bypass_pressed;
    filtered.trial_pressed = trial_pressed;
    filtered.risky_pressed = risky_pressed;
    filtered.hypothesis_delta = hypothesis_delta;

    float slider_raw = discrete_axis.x + eco::ApplyAnalogDeadzone(gamepad_axis.x, g_ctrl_settings.stick_deadzone);
    if (primary_gp >= 0 && IsGamepadAvailable(primary_gp)) {
        if (IsGamepadButtonDown(primary_gp, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) {
            slider_raw -= 0.35f;
        }
        if (IsGamepadButtonDown(primary_gp, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) {
            slider_raw += 0.35f;
        }
    }
    filtered.slider_delta = eco::Clamp(slider_raw, -1.0f, 1.0f) * 0.75f;

    const float aux_raw =
        -discrete_axis.y -
        eco::ApplyAnalogDeadzone(gamepad_axis.y, g_ctrl_settings.stick_deadzone) -
        eco::ApplyAnalogDeadzone(gamepad_right_y, g_ctrl_settings.stick_deadzone);
    filtered.aux_slider_delta = eco::Clamp(aux_raw, -1.0f, 1.0f) * 0.75f;

    out.players[0] = filtered;

    int next_player_slot = 1;
    for (int gp = 0; gp < eco::kMaxPlayers && next_player_slot < eco::kMaxPlayers; ++gp) {
        if (gp == primary_gp || !IsGamepadAvailable(gp)) {
            continue;
        }
        out.players[static_cast<std::size_t>(next_player_slot)] =
            PollSingleGamepadOnly(gp, ResolveEffectiveType(gp));
        ++next_player_slot;
    }

    out.toggle_state_pressed = IsKeyPressed(KEY_TAB);
    out.toggle_ui_phase_pressed =
        IsKeyPressed(KEY_V) ||
        (primary_gp >= 0 && IsActionPressed(primary_gp, primary_type, GamepadAction::HudPhase));
    out.advance_dialogue_pressed =
        IsKeyPressed(KEY_N) ||
        (primary_gp >= 0 && IsActionPressed(primary_gp, primary_type, GamepadAction::Bypass));
    out.toggle_history_pressed =
        IsKeyPressed(KEY_H) ||
        (primary_gp >= 0 && IsActionPressed(primary_gp, primary_type, GamepadAction::Journal));
    out.toggle_codex_pressed = IsKeyPressed(KEY_J);

    UpdateTriggerEdgeCache();
    return out;
}

MenuInputState PollMenuInput(float dt) noexcept {
    EnsureCtrlSettingsInit();
    MenuInputState m{};
    const float safe_dt = (std::isfinite(dt) && dt > 0.0f) ? std::min(dt, 0.1f) : (1.0f / 60.0f);

    const int primary_gp = FindPrimaryGamepad();
    UpdateActiveInputDevice(primary_gp);
    const ControllerType type = ResolveEffectiveType(primary_gp);

    const bool kb_up = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
    const bool kb_down = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
    const bool kb_left = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
    const bool kb_right = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);

    m.up_held = IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);
    m.down_held = IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
    m.left_held = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A);
    m.right_held = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);

    m.confirm_pressed =
        IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE);
    m.cancel_pressed =
        IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_X);
    m.pause_pressed = IsKeyPressed(KEY_ESCAPE);

    int gp_dir_x = 0;
    int gp_dir_y = 0;
    if (primary_gp >= 0 && IsGamepadAvailable(primary_gp)) {
        const bool dpad_up = IsGamepadButtonDown(primary_gp, GAMEPAD_BUTTON_LEFT_FACE_UP);
        const bool dpad_down = IsGamepadButtonDown(primary_gp, GAMEPAD_BUTTON_LEFT_FACE_DOWN);
        const bool dpad_left = IsGamepadButtonDown(primary_gp, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
        const bool dpad_right = IsGamepadButtonDown(primary_gp, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);

        const float lx = GetGamepadAxisMovement(primary_gp, GAMEPAD_AXIS_LEFT_X);
        const float ly = GetGamepadAxisMovement(primary_gp, GAMEPAD_AXIS_LEFT_Y);
        const float ry = GetGamepadAxisMovement(primary_gp, GAMEPAD_AXIS_RIGHT_Y);

        if (dpad_left || (std::isfinite(lx) && lx < -kMenuStickThreshold)) {
            gp_dir_x -= 1;
        }
        if (dpad_right || (std::isfinite(lx) && lx > kMenuStickThreshold)) {
            gp_dir_x += 1;
        }
        if (dpad_up || (std::isfinite(ly) && ly < -kMenuStickThreshold)) {
            gp_dir_y -= 1;
        }
        if (dpad_down || (std::isfinite(ly) && ly > kMenuStickThreshold)) {
            gp_dir_y += 1;
        }

        m.up_held = m.up_held || (gp_dir_y < 0);
        m.down_held = m.down_held || (gp_dir_y > 0);
        m.left_held = m.left_held || (gp_dir_x < 0);
        m.right_held = m.right_held || (gp_dir_x > 0);

        m.confirm_pressed = m.confirm_pressed || IsActionPressed(primary_gp, type, GamepadAction::Confirm);
        m.cancel_pressed = m.cancel_pressed || IsActionPressed(primary_gp, type, GamepadAction::Cancel);
        m.pause_pressed = m.pause_pressed || IsActionPressed(primary_gp, type, GamepadAction::Pause);
        m.journal_pressed = IsActionPressed(primary_gp, type, GamepadAction::Journal);
        m.tab_prev_pressed = IsGamepadButtonPressed(primary_gp, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
        m.tab_next_pressed = IsGamepadButtonPressed(primary_gp, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
        m.secondary_pressed = IsActionPressed(primary_gp, type, GamepadAction::Bypass);

        if (std::isfinite(ry) && std::abs(ry) > g_ctrl_settings.stick_deadzone) {
            m.scroll_y = ry;
        }
    }

    const bool gp_x_edge = g_menu_repeat_x.Step(gp_dir_x, safe_dt);
    const bool gp_y_edge = g_menu_repeat_y.Step(gp_dir_y, safe_dt);

    m.up_pressed = kb_up || (gp_y_edge && gp_dir_y < 0);
    m.down_pressed = kb_down || (gp_y_edge && gp_dir_y > 0);
    m.left_pressed = kb_left || (gp_x_edge && gp_dir_x < 0);
    m.right_pressed = kb_right || (gp_x_edge && gp_dir_x > 0);

    return m;
}

} // namespace eco::app
