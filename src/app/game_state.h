#pragma once

namespace eco {
class EcoSystem;
class PlayerManager;
struct AllPlayersInput;
struct WorldContext;
} // namespace eco

namespace eco::app {

class GameStateManager;

struct AppContext {
    GameStateManager& state_manager;
    eco::WorldContext& world;
    eco::EcoSystem& ecosystem;
    eco::PlayerManager& players;
    const eco::AllPlayersInput& input;
    bool quit_requested = false;
    bool open_settings_requested = false;
    bool relaunch_gpu_requested = false;
    bool toggle_hard_mode_requested = false;
    bool block_state_input_this_frame = false;
    bool overlay_blocking_input = false;
};

class GameState {
public:
    virtual ~GameState() = default;
    virtual void Update(float dt) = 0;
    virtual void Draw() = 0;
    virtual void OnEnter() {}
    virtual void OnExit() {}
    [[nodiscard]] virtual const char* Name() const noexcept { return "GameState"; }
};

} // namespace eco::app
