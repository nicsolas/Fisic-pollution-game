#include "app/raylib_input.h"

#include <raylib.h>

namespace eco::app {

eco::AllPlayersInput PollRaylibInput() {
    eco::AllPlayersInput out{};

    // Pure Single-Player input mapping for the Scientist protagonist (slot 0):
    // Both WASD/ZQSD and Arrow keys control movement seamlessly.
    eco::Vec2 discrete_axis{0.0f, 0.0f};
    eco::Vec2 gamepad_axis{0.0f, 0.0f};

    const bool left = IsKeyDown(KEY_A) || IsKeyDown(KEY_Q) || IsKeyDown(KEY_LEFT);
    const bool right = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
    const bool up = IsKeyDown(KEY_W) || IsKeyDown(KEY_Z) || IsKeyDown(KEY_UP);
    const bool down = IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN);

    discrete_axis.x = eco::CancelOpposingKeys(left, right);
    discrete_axis.y = eco::CancelOpposingKeys(up, down);

    bool jump_pressed = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W) ||
                        IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_UP) ||
                        IsKeyPressed(KEY_RIGHT_SHIFT);
    bool jump_held = IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_W) ||
                     IsKeyDown(KEY_Z) || IsKeyDown(KEY_UP) ||
                     IsKeyDown(KEY_RIGHT_SHIFT);
    bool interact_pressed =
        IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    bool scan_pressed =
        IsKeyPressed(KEY_F) || IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_PERIOD);
    bool toggle_overlay_pressed = IsKeyPressed(KEY_V);
    bool confirm_pressed =
        IsKeyPressed(KEY_C) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    bool bypass_pressed = IsKeyPressed(KEY_B);
    bool cancel_pressed = IsKeyPressed(KEY_X) || IsKeyPressed(KEY_BACKSPACE);

    if (IsGamepadAvailable(0)) {
        gamepad_axis.x = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        gamepad_axis.y = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);

        const bool dpad_left = IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
        const bool dpad_right = IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
        const bool dpad_up = IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP);
        const bool dpad_down = IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN);

        discrete_axis.x += eco::CancelOpposingKeys(dpad_left, dpad_right);
        discrete_axis.y += eco::CancelOpposingKeys(dpad_up, dpad_down);

        jump_pressed =
            jump_pressed || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        jump_held = jump_held || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        interact_pressed =
            interact_pressed || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
        scan_pressed =
            scan_pressed || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
        cancel_pressed =
            cancel_pressed || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP);
        confirm_pressed =
            confirm_pressed || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
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
        eco::kGamepadDeadzone);
    filtered.confirm_pressed = confirm_pressed;
    filtered.bypass_pressed = bypass_pressed;

    const float horiz_raw = discrete_axis.x + eco::ApplyAnalogDeadzone(gamepad_axis.x);
    filtered.slider_delta = eco::Clamp(horiz_raw, -1.0f, 1.0f) * 0.75f;

    out.players[0] = filtered;
    out.toggle_state_pressed = IsKeyPressed(KEY_TAB);
    out.toggle_ui_phase_pressed = IsKeyPressed(KEY_V);
    out.advance_dialogue_pressed =
        IsKeyPressed(KEY_N) ||
        (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP));
    out.toggle_history_pressed = IsKeyPressed(KEY_H);
    out.toggle_codex_pressed = IsKeyPressed(KEY_J);
    return out;
}

} // namespace eco::app
