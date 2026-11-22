#include "core/physics2d.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "core/player.h"
#include "core/tilemap.h"

namespace eco {

namespace {

[[nodiscard]] bool IsFiniteVec2(Vec2 v) noexcept {
    return std::isfinite(v.x) && std::isfinite(v.y);
}

[[nodiscard]] float SanitizeFinite(float v, float fallback = 0.0f) noexcept {
    return std::isfinite(v) ? v : fallback;
}

} // namespace

Vec2 NormalizeVector(Vec2 v) noexcept {
    if (!IsFiniteVec2(v)) {
        return {0.0f, 0.0f};
    }
    const float len = std::hypot(v.x, v.y);
    if (!std::isfinite(len) || len <= kEpsilon) {
        return {0.0f, 0.0f};
    }
    return {v.x / len, v.y / len};
}

Vec2 ComputeTopDownVelocity(Vec2 move_axis, float speed) noexcept {
    if (!std::isfinite(speed) || speed <= 0.0f) {
        return {0.0f, 0.0f};
    }
    const Vec2 dir = NormalizeVector(move_axis);
    return {dir.x * speed, dir.y * speed};
}

SlideMoveResult MoveAndSlideTopDown(
    Vec2 start_pos,
    Vec2 size,
    Vec2 velocity,
    float dt,
    const Tilemap& tilemap,
    bool bridge_unlocked) noexcept {
    SlideMoveResult result{};
    if (!IsFiniteVec2(start_pos)) {
        return {{0.0f, 0.0f}, {0.0f, 0.0f}, false, false};
    }
    result.position = start_pos;

    if (!IsFiniteVec2(size) || size.x <= 0.0f || size.y <= 0.0f) {
        return result;
    }

    Vec2 vel{SanitizeFinite(velocity.x), SanitizeFinite(velocity.y)};
    if (!std::isfinite(dt) || dt <= 0.0f) {
        result.velocity = vel;
        return result;
    }

    const float total_dx = vel.x * dt;
    const float total_dy = vel.y * dt;
    if (!std::isfinite(total_dx) || !std::isfinite(total_dy)) {
        return result;
    }

    const float max_disp = std::max(std::abs(total_dx), std::abs(total_dy));
    const float tile_sz = (tilemap.TileSize() > 0.0f) ? tilemap.TileSize() : kTileSize;
    const float max_step = std::max(1.0f, std::min({size.x, size.y, tile_sz}) * 0.5f);
    constexpr int kMaxSubSteps = 1024;
    int sub_steps = 1;
    if (max_disp > max_step) {
        const float raw_steps = std::ceil(max_disp / max_step);
        if (!std::isfinite(raw_steps) || raw_steps >= static_cast<float>(kMaxSubSteps)) {
            sub_steps = kMaxSubSteps;
        } else if (raw_steps > 1.0f) {
            sub_steps = static_cast<int>(raw_steps);
        }
    }

    const float step_dx = total_dx / static_cast<float>(sub_steps);
    const float step_dy = total_dy / static_cast<float>(sub_steps);

    const bool has_tiles = (tilemap.Rows() > 0 && tilemap.Cols() > 0);
    const float world_pad = 2.0f * tile_sz;

    Vec2 pos = start_pos;
    bool hit_x = false;
    bool hit_y = false;

    for (int step = 0; step < sub_steps; ++step) {
        // X pass (swept-interval AABB collision)
        if (!hit_x && step_dx != 0.0f) {
            const float prev_x = pos.x;
            const float next_x = prev_x + step_dx;

            if (!has_tiles) {
                pos.x = next_x;
            } else {
                const float sweep_left = (step_dx > 0.0f) ? prev_x : next_x;
                const float sweep_right = (step_dx > 0.0f) ? (next_x + size.x) : (prev_x + size.x);
                const float q_left = std::clamp(sweep_left, -world_pad, tilemap.WorldWidth() + world_pad);
                const float q_right = std::clamp(sweep_right, -world_pad, tilemap.WorldWidth() + world_pad);
                const Rect swept_aabb{q_left, pos.y, q_right - q_left, size.y};
                const TileRange range = tilemap.OverlappingTileRange(swept_aabb, true);

                bool collided_x = false;
                float best_snap_x = next_x;
                float boundary_x = 0.0f;

                if (!range.Empty()) {
                    for (int r = range.min_row; r <= range.max_row; ++r) {
                        for (int c = range.min_col; c <= range.max_col; ++c) {
                            if (!tilemap.IsSolid(r, c, bridge_unlocked)) {
                                continue;
                            }
                            const Rect tr = tilemap.TileRect(r, c);
                            if (tr.Top() >= pos.y + size.y || tr.Bottom() <= pos.y) {
                                continue;
                            }
                            if (step_dx > 0.0f) {
                                if (tr.Right() <= prev_x || !(tr.Left() < next_x + size.x)) {
                                    continue;
                                }
                                const float candidate = tr.Left() - size.x;
                                if (!collided_x || candidate < best_snap_x) {
                                    best_snap_x = candidate;
                                    boundary_x = tr.Left();
                                    collided_x = true;
                                }
                            } else {
                                if (tr.Left() >= prev_x + size.x || !(tr.Right() > next_x)) {
                                    continue;
                                }
                                const float candidate = tr.Right();
                                if (!collided_x || candidate > best_snap_x) {
                                    best_snap_x = candidate;
                                    boundary_x = tr.Right();
                                    collided_x = true;
                                }
                            }
                        }
                    }
                }

                if (collided_x) {
                    if (step_dx > 0.0f) {
                        while (best_snap_x + size.x > boundary_x) {
                            best_snap_x = std::nextafter(
                                best_snap_x, -std::numeric_limits<float>::infinity());
                        }
                    } else {
                        while (best_snap_x < boundary_x) {
                            best_snap_x = std::nextafter(
                                best_snap_x, std::numeric_limits<float>::infinity());
                        }
                    }
                    pos.x = best_snap_x;
                    vel.x = 0.0f;
                    hit_x = true;
                } else {
                    pos.x = next_x;
                }
            }
        }

        // Y pass (swept-interval AABB collision)
        if (!hit_y && step_dy != 0.0f) {
            const float prev_y = pos.y;
            const float next_y = prev_y + step_dy;

            if (!has_tiles) {
                pos.y = next_y;
            } else {
                const float sweep_top = (step_dy > 0.0f) ? prev_y : next_y;
                const float sweep_bottom = (step_dy > 0.0f) ? (next_y + size.y) : (prev_y + size.y);
                const float q_top = std::clamp(sweep_top, -world_pad, tilemap.WorldHeight() + world_pad);
                const float q_bottom = std::clamp(sweep_bottom, -world_pad, tilemap.WorldHeight() + world_pad);
                const Rect swept_aabb{pos.x, q_top, size.x, q_bottom - q_top};
                const TileRange range = tilemap.OverlappingTileRange(swept_aabb, true);

                bool collided_y = false;
                float best_snap_y = next_y;
                float boundary_y = 0.0f;

                if (!range.Empty()) {
                    for (int r = range.min_row; r <= range.max_row; ++r) {
                        for (int c = range.min_col; c <= range.max_col; ++c) {
                            if (!tilemap.IsSolid(r, c, bridge_unlocked)) {
                                continue;
                            }
                            const Rect tr = tilemap.TileRect(r, c);
                            if (tr.Left() >= pos.x + size.x || tr.Right() <= pos.x) {
                                continue;
                            }
                            if (step_dy > 0.0f) {
                                if (tr.Bottom() <= prev_y || !(tr.Top() < next_y + size.y)) {
                                    continue;
                                }
                                const float candidate = tr.Top() - size.y;
                                if (!collided_y || candidate < best_snap_y) {
                                    best_snap_y = candidate;
                                    boundary_y = tr.Top();
                                    collided_y = true;
                                }
                            } else {
                                if (tr.Top() >= prev_y + size.y || !(tr.Bottom() > next_y)) {
                                    continue;
                                }
                                const float candidate = tr.Bottom();
                                if (!collided_y || candidate > best_snap_y) {
                                    best_snap_y = candidate;
                                    boundary_y = tr.Bottom();
                                    collided_y = true;
                                }
                            }
                        }
                    }
                }

                if (collided_y) {
                    if (step_dy > 0.0f) {
                        while (best_snap_y + size.y > boundary_y) {
                            best_snap_y = std::nextafter(
                                best_snap_y, -std::numeric_limits<float>::infinity());
                        }
                    } else {
                        while (best_snap_y < boundary_y) {
                            best_snap_y = std::nextafter(
                                best_snap_y, std::numeric_limits<float>::infinity());
                        }
                    }
                    pos.y = best_snap_y;
                    vel.y = 0.0f;
                    hit_y = true;
                } else {
                    pos.y = next_y;
                }
            }
        }

        if (hit_x && hit_y) {
            break;
        }
    }

    result.position = pos;
    result.velocity = vel;
    result.hit_x = hit_x;
    result.hit_y = hit_y;
    return result;
}

SlideMoveResult MovePlayerTopDown(
    Player& player,
    Vec2 move_axis,
    float dt,
    const Tilemap& tilemap,
    bool bridge_unlocked,
    float speed) noexcept {
    const Vec2 desired_vel = ComputeTopDownVelocity(move_axis, speed);
    const SlideMoveResult res = MoveAndSlideTopDown(
        player.pos, player.size, desired_vel, dt, tilemap, bridge_unlocked);
    player.pos = res.position;
    player.vel = res.velocity;
    return res;
}

bool IsSupportedByFloor(
    Vec2 pos,
    Vec2 size,
    const Tilemap& tilemap,
    bool bridge_unlocked) noexcept {
    if (!IsFiniteVec2(pos) || !IsFiniteVec2(size) || size.x <= 0.0f || size.y <= 0.0f) {
        return false;
    }
    if (tilemap.Rows() <= 0 || tilemap.Cols() <= 0) {
        return false;
    }

    const float foot_y = pos.y + size.y;
    constexpr float kFloorEps = 0.05f;
    const Rect probe{pos.x, foot_y - kFloorEps, size.x, 2.0f * kFloorEps};
    const TileRange range = tilemap.OverlappingTileRange(probe, true);
    if (range.Empty()) {
        return false;
    }

    for (int r = range.min_row; r <= range.max_row; ++r) {
        for (int c = range.min_col; c <= range.max_col; ++c) {
            if (!tilemap.IsSolid(r, c, bridge_unlocked)) {
                continue;
            }
            const Rect tr = tilemap.TileRect(r, c);
            if (tr.Left() >= pos.x + size.x || tr.Right() <= pos.x) {
                continue;
            }
            if (std::fabs(tr.Top() - foot_y) <= kFloorEps) {
                return true;
            }
        }
    }
    return false;
}

SideScrollMoveResult MovePlayerSideScrolling(
    Player& player,
    float move_x,
    bool jump_pressed,
    bool jump_held,
    float dt,
    const Tilemap& tilemap,
    bool bridge_unlocked,
    float horizontal_speed) noexcept {
    (void)jump_held;

    SideScrollMoveResult result{};
    if (!IsFiniteVec2(player.pos)) {
        player.pos = {0.0f, 0.0f};
    }
    if (!IsFiniteVec2(player.size) || player.size.x <= 0.0f || player.size.y <= 0.0f) {
        player.size = {kPlayerWidth, kPlayerHeight};
    }
    if (!std::isfinite(player.y_velocity)) {
        player.y_velocity = 0.0f;
    }
    if (!std::isfinite(player.coyote_timer) || player.coyote_timer < 0.0f) {
        player.coyote_timer = 0.0f;
    }
    if (!std::isfinite(player.jump_buffer_timer) || player.jump_buffer_timer < 0.0f) {
        player.jump_buffer_timer = 0.0f;
    }

    if (!std::isfinite(dt) || dt <= 0.0f) {
        result.position = player.pos;
        result.velocity = player.vel;
        result.y_velocity = player.y_velocity;
        result.is_grounded = player.isGrounded;
        return result;
    }

    const float safe_axis_x = std::isfinite(move_x) ? Clamp(move_x, -1.0f, 1.0f) : 0.0f;
    const float safe_speed =
        (std::isfinite(horizontal_speed) && horizontal_speed > 0.0f) ? horizontal_speed : 0.0f;
    const float desired_vx = safe_axis_x * safe_speed;

    const bool was_grounded_before_x =
        player.isGrounded ||
        (player.y_velocity >= 0.0f &&
         IsSupportedByFloor(player.pos, player.size, tilemap, bridge_unlocked));

    // 1. Axis-separated horizontal movement (Y-walk is disabled in side-scrolling mode)
    const SlideMoveResult x_res = MoveAndSlideTopDown(
        player.pos,
        player.size,
        {desired_vx, 0.0f},
        dt,
        tilemap,
        bridge_unlocked);
    player.pos.x = x_res.position.x;
    player.vel = {x_res.velocity.x, 0.0f};
    result.hit_x = x_res.hit_x;

    // 2. Re-evaluate floor support after horizontal shift
    const bool supported_after_x =
        (player.y_velocity >= 0.0f) &&
        IsSupportedByFloor(player.pos, player.size, tilemap, bridge_unlocked);

    bool walked_off_ledge_this_tick = false;
    if (supported_after_x) {
        player.isGrounded = true;
        player.y_velocity = 0.0f;
        player.coyote_timer = kCoyoteTimeDuration;
    } else if (was_grounded_before_x) {
        player.isGrounded = false;
        player.coyote_timer = kCoyoteTimeDuration;
        walked_off_ledge_this_tick = true;
    } else {
        player.isGrounded = false;
    }

    // 3. Jump buffer & coyote time update
    if (player.jump_buffer_timer > 0.0f) {
        player.jump_buffer_timer = std::max(0.0f, player.jump_buffer_timer - dt);
        if (player.jump_buffer_timer < 1e-5f) {
            player.jump_buffer_timer = 0.0f;
        }
    }
    if (jump_pressed) {
        player.jump_buffer_timer = kJumpBufferDuration;
    }

    const bool can_jump_now = player.isGrounded || (player.coyote_timer > 1e-5f);
    const bool wants_jump_now = jump_pressed || (player.jump_buffer_timer > 1e-5f);
    const float jump_impulse =
        (std::isfinite(player.jump_force) && player.jump_force > 0.0f)
            ? player.jump_force
            : kJumpForce;

    if (can_jump_now && wants_jump_now) {
        player.y_velocity = -jump_impulse;
        player.isGrounded = false;
        player.coyote_timer = 0.0f;
        player.jump_buffer_timer = 0.0f;
        result.jumped = true;
    } else if (!player.isGrounded && !walked_off_ledge_this_tick) {
        player.coyote_timer = std::max(0.0f, player.coyote_timer - dt);
        if (player.coyote_timer < 1e-5f) {
            player.coyote_timer = 0.0f;
        }
    }

    // 4. Gravity & sub-stepped vertical AABB sweep
    if (player.isGrounded) {
        player.y_velocity = 0.0f;
    } else {
        if (!result.jumped) {
            const float grav =
                (std::isfinite(player.gravity_accel) && player.gravity_accel >= 0.0f)
                    ? player.gravity_accel
                    : kGravityAccel;
            player.y_velocity = std::min(player.y_velocity + grav * dt, kMaxFallSpeed);
        } else if (player.y_velocity > kMaxFallSpeed) {
            player.y_velocity = kMaxFallSpeed;
        }

        const float total_dy = player.y_velocity * dt;
        if (std::isfinite(total_dy) && total_dy != 0.0f) {
            const float max_sub = (kMaxVerticalSubStep > 0.0f) ? kMaxVerticalSubStep : 8.0f;
            constexpr int kMaxVertSubSteps = 1024;
            int v_steps = 1;
            if (std::fabs(total_dy) > max_sub) {
                const float raw_steps = std::ceil(std::fabs(total_dy) / max_sub);
                if (!std::isfinite(raw_steps) || raw_steps >= static_cast<float>(kMaxVertSubSteps)) {
                    v_steps = kMaxVertSubSteps;
                } else if (raw_steps > 1.0f) {
                    v_steps = static_cast<int>(raw_steps);
                }
            }

            const float step_dy = total_dy / static_cast<float>(v_steps);
            const bool has_tiles = (tilemap.Rows() > 0 && tilemap.Cols() > 0);
            const float tile_sz = (tilemap.TileSize() > 0.0f) ? tilemap.TileSize() : kTileSize;
            const float world_pad = 2.0f * tile_sz;

            for (int s = 0; s < v_steps; ++s) {
                const float prev_y = player.pos.y;
                const float next_y = prev_y + step_dy;

                if (!has_tiles) {
                    player.pos.y = next_y;
                    continue;
                }

                const float sweep_top = (step_dy > 0.0f) ? prev_y : next_y;
                const float sweep_bottom =
                    (step_dy > 0.0f) ? (next_y + player.size.y) : (prev_y + player.size.y);
                const float q_top =
                    std::clamp(sweep_top, -world_pad, tilemap.WorldHeight() + world_pad);
                const float q_bottom =
                    std::clamp(sweep_bottom, -world_pad, tilemap.WorldHeight() + world_pad);
                const Rect swept_aabb{player.pos.x, q_top, player.size.x, q_bottom - q_top};
                const TileRange range = tilemap.OverlappingTileRange(swept_aabb, true);

                bool collided_y = false;
                float best_snap_y = next_y;
                float boundary_y = 0.0f;

                if (!range.Empty()) {
                    for (int r = range.min_row; r <= range.max_row; ++r) {
                        for (int c = range.min_col; c <= range.max_col; ++c) {
                            if (!tilemap.IsSolid(r, c, bridge_unlocked)) {
                                continue;
                            }
                            const Rect tr = tilemap.TileRect(r, c);
                            if (tr.Left() >= player.pos.x + player.size.x ||
                                tr.Right() <= player.pos.x) {
                                continue;
                            }
                            if (step_dy > 0.0f) {
                                if (tr.Bottom() <= prev_y ||
                                    !(tr.Top() < next_y + player.size.y)) {
                                    continue;
                                }
                                const float candidate = tr.Top() - player.size.y;
                                if (!collided_y || candidate < best_snap_y) {
                                    best_snap_y = candidate;
                                    boundary_y = tr.Top();
                                    collided_y = true;
                                }
                            } else {
                                if (tr.Top() >= prev_y + player.size.y ||
                                    !(tr.Bottom() > next_y)) {
                                    continue;
                                }
                                const float candidate = tr.Bottom();
                                if (!collided_y || candidate > best_snap_y) {
                                    best_snap_y = candidate;
                                    boundary_y = tr.Bottom();
                                    collided_y = true;
                                }
                            }
                        }
                    }
                }

                if (collided_y) {
                    if (step_dy > 0.0f) {
                        while (best_snap_y + player.size.y > boundary_y) {
                            best_snap_y = std::nextafter(
                                best_snap_y, -std::numeric_limits<float>::infinity());
                        }
                        player.pos.y = best_snap_y;
                        player.y_velocity = 0.0f;
                        player.isGrounded = true;
                        player.coyote_timer = kCoyoteTimeDuration;
                        result.hit_floor = true;

                        if (player.jump_buffer_timer > 1e-5f) {
                            player.y_velocity = -jump_impulse;
                            player.isGrounded = false;
                            player.coyote_timer = 0.0f;
                            player.jump_buffer_timer = 0.0f;
                            result.jumped = true;
                        }
                    } else {
                        while (best_snap_y < boundary_y) {
                            best_snap_y = std::nextafter(
                                best_snap_y, std::numeric_limits<float>::infinity());
                        }
                        player.pos.y = best_snap_y;
                        player.y_velocity = 0.0f;
                        player.isGrounded = false;
                        player.coyote_timer = 0.0f;
                        player.jump_buffer_timer = 0.0f;
                        result.hit_ceiling = true;
                    }
                    break;
                } else {
                    player.pos.y = next_y;
                }
            }

            if (!result.hit_floor && !result.hit_ceiling && player.y_velocity >= 0.0f &&
                IsSupportedByFloor(player.pos, player.size, tilemap, bridge_unlocked)) {
                player.y_velocity = 0.0f;
                player.isGrounded = true;
                player.coyote_timer = kCoyoteTimeDuration;
                result.hit_floor = true;

                if (player.jump_buffer_timer > 1e-5f) {
                    player.y_velocity = -jump_impulse;
                    player.isGrounded = false;
                    player.coyote_timer = 0.0f;
                    player.jump_buffer_timer = 0.0f;
                    result.jumped = true;
                }
            }
        }
    }

    result.position = player.pos;
    result.velocity = player.vel;
    result.y_velocity = player.y_velocity;
    result.is_grounded = player.isGrounded;
    return result;
}

Vec2 LerpCameraTarget(
    Vec2 current_target,
    Vec2 desired_target,
    float dt,
    float lerp_speed) noexcept {
    if (!IsFiniteVec2(desired_target)) {
        return IsFiniteVec2(current_target) ? current_target : Vec2{0.0f, 0.0f};
    }
    if (!IsFiniteVec2(current_target)) {
        return desired_target;
    }
    if (!std::isfinite(dt) || dt <= 0.0f || !std::isfinite(lerp_speed) || lerp_speed <= 0.0f) {
        return current_target;
    }
    const float alpha = Clamp(1.0f - std::exp(-lerp_speed * dt), 0.0f, 1.0f);
    return LerpVec2(current_target, desired_target, alpha);
}

Vec2 UpdateAverageCameraTarget(
    Vec2 current_target,
    const PlayerManager& players,
    float dt,
    float lerp_speed) noexcept {
    if (players.ActiveCount() <= 0) {
        return IsFiniteVec2(current_target) ? current_target : Vec2{0.0f, 0.0f};
    }
    return LerpCameraTarget(current_target, players.AverageActivePosition(), dt, lerp_speed);
}

} // namespace eco
