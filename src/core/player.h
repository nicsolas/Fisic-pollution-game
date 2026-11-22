#pragma once

#include <array>
#include <vector>

#include "core/input_types.h"
#include "core/math_types.h"

namespace eco {

[[nodiscard]] ColorRgba ColorForPlayerId(int player_id) noexcept;

class Player {
public:
    explicit Player(int player_id = 0);

    int player_id = 0;
    bool active = true;
    Vec2 pos{0.0f, 0.0f};
    Vec2 vel{0.0f, 0.0f};
    Vec2 size{kPlayerWidth, kPlayerHeight};

    // Side-scrolling vertical physics state (M3)
    float y_velocity = 0.0f;
    float gravity_accel = kGravityAccel;
    float jump_force = kJumpForce;
    bool isGrounded = false;
    float coyote_timer = 0.0f;
    float jump_buffer_timer = 0.0f;

    ColorRgba color{};

    [[nodiscard]] Rect Bounds() const noexcept {
        return {pos.x, pos.y, size.x, size.y};
    }

    [[nodiscard]] Vec2 Center() const noexcept {
        return {pos.x + size.x * 0.5f, pos.y + size.y * 0.5f};
    }

    void ResetVerticalState() noexcept;
};

class PlayerManager {
public:
    PlayerManager();

    bool JoinPlayer(int player_id, Vec2 spawn_pos) noexcept;
    bool LeavePlayer(int player_id) noexcept;
    void EnsureActive(int player_id, Vec2 spawn_pos) noexcept;

    void ProcessDropIn(const AllPlayersInput& inputs, Vec2 fallback_spawn) noexcept;

    [[nodiscard]] int ActiveCount() const noexcept;
    [[nodiscard]] Player* GetPlayer(int player_id) noexcept;
    [[nodiscard]] const Player* GetPlayer(int player_id) const noexcept;

    [[nodiscard]] std::vector<Player*> ActivePlayers();
    [[nodiscard]] std::vector<const Player*> ActivePlayers() const;

    [[nodiscard]] const std::array<Player, kMaxPlayers>& Slots() const noexcept {
        return players_;
    }

    [[nodiscard]] Vec2 AverageActivePosition() const noexcept;
    void PlaceActivePlayers(Vec2 spawn_pos, float spacing_x = 0.0f) noexcept;

private:
    [[nodiscard]] Vec2 AverageActiveTopLeft() const noexcept;

    std::array<Player, kMaxPlayers> players_{
        Player(0), Player(1), Player(2), Player(3)
    };
};

} // namespace eco
