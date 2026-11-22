#pragma once

#include "core/math_types.h"

namespace eco {

class Player;
class PlayerManager;
class Tilemap;

struct SlideMoveResult {
    Vec2 position{};
    Vec2 velocity{};
    bool hit_x = false;
    bool hit_y = false;
};

struct SideScrollMoveResult {
    Vec2 position{};
    Vec2 velocity{};
    float y_velocity = 0.0f;
    bool is_grounded = false;
    bool hit_x = false;
    bool hit_floor = false;
    bool hit_ceiling = false;
    bool jumped = false;
};

[[nodiscard]] Vec2 NormalizeVector(Vec2 v) noexcept;
[[nodiscard]] Vec2 ComputeTopDownVelocity(Vec2 move_axis, float speed = kTopDownSpeed) noexcept;

[[nodiscard]] SlideMoveResult MoveAndSlideTopDown(
    Vec2 start_pos,
    Vec2 size,
    Vec2 velocity,
    float dt,
    const Tilemap& tilemap,
    bool bridge_unlocked = false) noexcept;

SlideMoveResult MovePlayerTopDown(
    Player& player,
    Vec2 move_axis,
    float dt,
    const Tilemap& tilemap,
    bool bridge_unlocked = false,
    float speed = kTopDownSpeed) noexcept;

[[nodiscard]] bool IsSupportedByFloor(
    Vec2 pos,
    Vec2 size,
    const Tilemap& tilemap,
    bool bridge_unlocked = false) noexcept;

SideScrollMoveResult MovePlayerSideScrolling(
    Player& player,
    float move_x,
    bool jump_pressed,
    bool jump_held,
    float dt,
    const Tilemap& tilemap,
    bool bridge_unlocked = false,
    float horizontal_speed = kSideMoveSpeed) noexcept;

[[nodiscard]] Vec2 LerpCameraTarget(
    Vec2 current_target,
    Vec2 desired_target,
    float dt,
    float lerp_speed = kCameraLerpSpeed) noexcept;

[[nodiscard]] Vec2 UpdateAverageCameraTarget(
    Vec2 current_target,
    const PlayerManager& players,
    float dt,
    float lerp_speed = kCameraLerpSpeed) noexcept;

} // namespace eco
