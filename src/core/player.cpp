#include "core/player.h"

#include <algorithm>
#include <cstddef>

namespace eco {

ColorRgba ColorForPlayerId(int player_id) noexcept {
    static constexpr std::array<ColorRgba, kMaxPlayers> kPalette{
        ColorRgba{59, 130, 246, 255},   // P0: Blue
        ColorRgba{239, 68, 68, 255},    // P1: Coral/Red
        ColorRgba{245, 158, 11, 255},   // P2: Amber/Gold
        ColorRgba{168, 85, 247, 255}    // P3: Violet
    };
    const int clamped_id = std::clamp(player_id, 0, kMaxPlayers - 1);
    return kPalette[static_cast<std::size_t>(clamped_id)];
}

Player::Player(int player_id)
    : player_id(std::clamp(player_id, 0, kMaxPlayers - 1)),
      active(true),
      size{kPlayerWidth, kPlayerHeight},
      gravity_accel(kGravityAccel),
      jump_force(kJumpForce),
      color(ColorForPlayerId(this->player_id)) {}

void Player::ResetVerticalState() noexcept {
    y_velocity = 0.0f;
    isGrounded = false;
    coyote_timer = 0.0f;
    jump_buffer_timer = 0.0f;
}

PlayerManager::PlayerManager() {
    players_[0].active = true;
    for (int i = 1; i < kMaxPlayers; ++i) {
        players_[static_cast<std::size_t>(i)].active = false;
    }
}

bool PlayerManager::JoinPlayer(int player_id, Vec2 spawn_pos) noexcept {
    if (player_id < 0 || player_id >= kMaxPlayers) {
        return false;
    }
    Player& p = players_[static_cast<std::size_t>(player_id)];
    if (p.active) {
        return false;
    }
    p.active = true;
    p.pos = spawn_pos;
    p.vel = {0.0f, 0.0f};
    p.ResetVerticalState();
    return true;
}

bool PlayerManager::LeavePlayer(int player_id) noexcept {
    if (player_id < 0 || player_id >= kMaxPlayers) {
        return false;
    }
    Player& p = players_[static_cast<std::size_t>(player_id)];
    if (!p.active) {
        return false;
    }
    p.active = false;
    p.vel = {0.0f, 0.0f};
    p.ResetVerticalState();
    return true;
}

void PlayerManager::EnsureActive(int player_id, Vec2 spawn_pos) noexcept {
    if (player_id < 0 || player_id >= kMaxPlayers) {
        return;
    }
    Player& p = players_[static_cast<std::size_t>(player_id)];
    if (!p.active) {
        JoinPlayer(player_id, spawn_pos);
    }
}

void PlayerManager::ProcessDropIn(const AllPlayersInput& inputs, Vec2 fallback_spawn) noexcept {
    Vec2 active_anchor = fallback_spawn;
    bool has_active = false;
    if (players_[0].active) {
        active_anchor = players_[0].pos;
        has_active = true;
    } else {
        for (const auto& p : players_) {
            if (p.active) {
                active_anchor = p.pos;
                has_active = true;
                break;
            }
        }
    }

    for (int id = 0; id < kMaxPlayers; ++id) {
        Player& p = players_[static_cast<std::size_t>(id)];
        if (p.active) {
            continue;
        }
        const PlayerInput& in = inputs.Get(id);
        const bool wants_join = in.join_pressed || (id > 0 && in.HasAnyAction());
        if (wants_join) {
            const Vec2 spawn = has_active ? active_anchor : fallback_spawn;
            if (JoinPlayer(id, spawn) && !has_active) {
                active_anchor = p.pos;
                has_active = true;
            }
        }
    }
}

int PlayerManager::ActiveCount() const noexcept {
    int count = 0;
    for (const auto& p : players_) {
        if (p.active) {
            ++count;
        }
    }
    return count;
}

Player* PlayerManager::GetPlayer(int player_id) noexcept {
    if (player_id < 0 || player_id >= kMaxPlayers) {
        return nullptr;
    }
    return &players_[static_cast<std::size_t>(player_id)];
}

const Player* PlayerManager::GetPlayer(int player_id) const noexcept {
    if (player_id < 0 || player_id >= kMaxPlayers) {
        return nullptr;
    }
    return &players_[static_cast<std::size_t>(player_id)];
}

std::vector<Player*> PlayerManager::ActivePlayers() {
    std::vector<Player*> result;
    result.reserve(kMaxPlayers);
    for (auto& p : players_) {
        if (p.active) {
            result.push_back(&p);
        }
    }
    return result;
}

std::vector<const Player*> PlayerManager::ActivePlayers() const {
    std::vector<const Player*> result;
    result.reserve(kMaxPlayers);
    for (const auto& p : players_) {
        if (p.active) {
            result.push_back(&p);
        }
    }
    return result;
}

Vec2 PlayerManager::AverageActivePosition() const noexcept {
    Vec2 sum{0.0f, 0.0f};
    int count = 0;
    for (const auto& p : players_) {
        if (p.active) {
            sum += p.Center();
            ++count;
        }
    }
    if (count == 0) {
        return {0.0f, 0.0f};
    }
    return sum / static_cast<float>(count);
}

Vec2 PlayerManager::AverageActiveTopLeft() const noexcept {
    Vec2 sum{0.0f, 0.0f};
    int count = 0;
    for (const auto& p : players_) {
        if (p.active) {
            sum += p.pos;
            ++count;
        }
    }
    if (count == 0) {
        return {0.0f, 0.0f};
    }
    return sum / static_cast<float>(count);
}

void PlayerManager::PlaceActivePlayers(Vec2 spawn_pos, float spacing_x) noexcept {
    int active_idx = 0;
    for (auto& p : players_) {
        if (p.active) {
            p.pos = {spawn_pos.x + static_cast<float>(active_idx) * spacing_x, spawn_pos.y};
            p.vel = {0.0f, 0.0f};
            p.ResetVerticalState();
            ++active_idx;
        }
    }
}

} // namespace eco
