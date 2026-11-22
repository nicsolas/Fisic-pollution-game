#pragma once

#include <string>

#include "core/ecosystem.h"
#include "core/math_types.h"
#include "core/player.h"

namespace eco {

inline constexpr float kDefaultNpcTriggerRadius = 72.0f;

struct OverlayVisibility {
    UiPhase phase = UiPhase::Phase1_Minimal;
    bool show_raw_numbers = false;
    bool show_velocity_vectors = false;
    bool show_aabb_hitboxes = false;
    bool show_irradiance_rays = false;
    bool show_trigger_radii = false;
};

[[nodiscard]] OverlayVisibility ComputeOverlayVisibility(const EcoSystem& ecosystem) noexcept;
bool TryRequestOverlayPhase(EcoSystem& ecosystem, UiPhase requested) noexcept;

struct ArchitectNpc {
    Vec2 pos{888.0f, 312.0f};
    float trigger_radius = kDefaultNpcTriggerRadius;
    bool recruited = false;
    bool hidden_bridge_revealed = false;
    std::string last_dialogue{};

    void Reset(Vec2 default_pos = {888.0f, 312.0f});
    [[nodiscard]] bool IsPlayerInRange(const Player& player) const noexcept;
    [[nodiscard]] bool AnyActivePlayerInRange(const PlayerManager& players) const noexcept;
    bool TryInteract(int player_id, const PlayerManager& players, EcoSystem& ecosystem);
};

} // namespace eco
