#pragma once

#include <array>
#include <cmath>
#include <cstddef>

#include "core/math_types.h"

namespace eco {

struct PlayerInput {
    float move_x = 0.0f;
    float move_y = 0.0f;
    bool jump_pressed = false;
    bool jump_held = false;
    bool sprint_held = false;
    bool interact_pressed = false;
    bool scan_pressed = false;
    bool toggle_overlay_pressed = false;
    bool join_pressed = false;
    bool cancel_pressed = false;
    bool confirm_pressed = false;
    bool bypass_pressed = false;
    float slider_delta = 0.0f;

    [[nodiscard]] constexpr Vec2 MoveAxis() const noexcept {
        return {move_x, move_y};
    }

    [[nodiscard]] constexpr bool HasAnyAction() const noexcept {
        return move_x != 0.0f || move_y != 0.0f || jump_pressed || jump_held ||
               sprint_held || interact_pressed || scan_pressed || toggle_overlay_pressed ||
               join_pressed || cancel_pressed || confirm_pressed ||
               bypass_pressed || slider_delta != 0.0f;
    }
};

struct AllPlayersInput {
    std::array<PlayerInput, kMaxPlayers> players{};
    bool toggle_state_pressed = false;
    bool toggle_ui_phase_pressed = false;
    bool advance_dialogue_pressed = false;
    bool toggle_history_pressed = false;
    bool toggle_codex_pressed = false;

    [[nodiscard]] inline const PlayerInput& Get(int player_id) const noexcept {
        static const PlayerInput kEmpty{};
        if (player_id < 0 || player_id >= kMaxPlayers) {
            return kEmpty;
        }
        return players[static_cast<std::size_t>(player_id)];
    }

    [[nodiscard]] inline PlayerInput* TryMutable(int player_id) noexcept {
        if (player_id < 0 || player_id >= kMaxPlayers) {
            return nullptr;
        }
        return &players[static_cast<std::size_t>(player_id)];
    }
};

[[nodiscard]] constexpr float CancelOpposingKeys(bool negative_held, bool positive_held) noexcept {
    return (positive_held ? 1.0f : 0.0f) - (negative_held ? 1.0f : 0.0f);
}

[[nodiscard]] inline float ApplyAnalogDeadzone(
    float raw_axis,
    float deadzone = kGamepadDeadzone) noexcept {
    if (!std::isfinite(raw_axis)) {
        return 0.0f;
    }
    const float dz = Clamp(deadzone, 0.0f, 0.99f);
    if (std::abs(raw_axis) < dz) {
        return 0.0f;
    }
    return Clamp(raw_axis, -1.0f, 1.0f);
}

[[nodiscard]] inline Vec2 ClampToUnitDisk(Vec2 axis) noexcept {
    const float len_sq = axis.LengthSquared();
    if (!std::isfinite(len_sq) || len_sq <= kEpsilon * kEpsilon) {
        return {0.0f, 0.0f};
    }
    if (len_sq > 1.0f) {
        const float len = std::sqrt(len_sq);
        return {axis.x / len, axis.y / len};
    }
    return axis;
}

[[nodiscard]] inline Vec2 CombineKeyboardAndGamepadAxes(
    Vec2 keyboard_axis,
    Vec2 gamepad_axis,
    float deadzone = kGamepadDeadzone) noexcept {
    const Vec2 kb{
        std::isfinite(keyboard_axis.x) ? Clamp(keyboard_axis.x, -1.0f, 1.0f) : 0.0f,
        std::isfinite(keyboard_axis.y) ? Clamp(keyboard_axis.y, -1.0f, 1.0f) : 0.0f
    };
    const Vec2 gp{
        ApplyAnalogDeadzone(gamepad_axis.x, deadzone),
        ApplyAnalogDeadzone(gamepad_axis.y, deadzone)
    };
    return ClampToUnitDisk({kb.x + gp.x, kb.y + gp.y});
}

[[nodiscard]] inline PlayerInput FilterPlayerInput(
    Vec2 keyboard_axis,
    Vec2 gamepad_axis,
    bool jump_pressed,
    bool jump_held,
    bool interact_pressed,
    bool scan_pressed,
    bool toggle_overlay_pressed,
    bool join_pressed,
    bool cancel_pressed,
    float deadzone = kGamepadDeadzone) noexcept {
    const Vec2 move = CombineKeyboardAndGamepadAxes(keyboard_axis, gamepad_axis, deadzone);
    PlayerInput out{};
    out.move_x = move.x;
    out.move_y = move.y;
    out.jump_pressed = jump_pressed;
    out.jump_held = jump_held;
    out.interact_pressed = interact_pressed;
    out.scan_pressed = scan_pressed;
    out.toggle_overlay_pressed = toggle_overlay_pressed;
    out.join_pressed = join_pressed;
    out.cancel_pressed = cancel_pressed;
    return out;
}

inline void LatchFrameInput(AllPlayersInput& pending, const AllPlayersInput& frame_sample) noexcept {
    for (std::size_t i = 0; i < pending.players.size(); ++i) {
        PlayerInput& dst = pending.players[i];
        const PlayerInput& src = frame_sample.players[i];

        dst.move_x = src.move_x;
        dst.move_y = src.move_y;
        dst.jump_held = src.jump_held;
        dst.sprint_held = src.sprint_held;
        if (std::isfinite(src.slider_delta) && src.slider_delta != 0.0f) {
            dst.slider_delta += src.slider_delta;
        }

        dst.jump_pressed = dst.jump_pressed || src.jump_pressed;
        dst.interact_pressed = dst.interact_pressed || src.interact_pressed;
        dst.scan_pressed = dst.scan_pressed || src.scan_pressed;
        dst.toggle_overlay_pressed = dst.toggle_overlay_pressed || src.toggle_overlay_pressed;
        dst.join_pressed = dst.join_pressed || src.join_pressed;
        dst.cancel_pressed = dst.cancel_pressed || src.cancel_pressed;
        dst.confirm_pressed = dst.confirm_pressed || src.confirm_pressed;
        dst.bypass_pressed = dst.bypass_pressed || src.bypass_pressed;
    }
    pending.toggle_state_pressed = pending.toggle_state_pressed || frame_sample.toggle_state_pressed;
    pending.toggle_ui_phase_pressed =
        pending.toggle_ui_phase_pressed || frame_sample.toggle_ui_phase_pressed;
    pending.advance_dialogue_pressed =
        pending.advance_dialogue_pressed || frame_sample.advance_dialogue_pressed;
    pending.toggle_history_pressed =
        pending.toggle_history_pressed || frame_sample.toggle_history_pressed;
    pending.toggle_codex_pressed =
        pending.toggle_codex_pressed || frame_sample.toggle_codex_pressed;
}

inline void ConsumePressedEdges(AllPlayersInput& pending) noexcept {
    for (auto& p : pending.players) {
        p.jump_pressed = false;
        p.interact_pressed = false;
        p.scan_pressed = false;
        p.toggle_overlay_pressed = false;
        p.join_pressed = false;
        p.cancel_pressed = false;
        p.confirm_pressed = false;
        p.bypass_pressed = false;
        p.slider_delta = 0.0f;
    }
    pending.toggle_state_pressed = false;
    pending.toggle_ui_phase_pressed = false;
    pending.advance_dialogue_pressed = false;
    pending.toggle_history_pressed = false;
    pending.toggle_codex_pressed = false;
}

} // namespace eco
