#pragma once

#include <functional>
#include <memory>

#include "app/game_state.h"

namespace eco::app {

class GameStateManager {
public:
    using StateFactory = std::function<std::unique_ptr<GameState>()>;

    GameStateManager() = default;
    ~GameStateManager();

    GameStateManager(const GameStateManager&) = delete;
    GameStateManager& operator=(const GameStateManager&) = delete;

    // Deferred transition requests: never destroys current_state_ during Update(dt)
    void RequestTransition(std::unique_ptr<GameState> next_state);
    void RequestTransition(StateFactory factory);

    // Executes pending transition: OnExit() -> destroy old -> swap -> OnEnter().
    // Returns true if a transition occurred.
    bool ApplyPendingTransition();

    // Updates active state if present, then applies any pending transition.
    void Update(float dt);

    // Draws active state if present.
    void Draw();

    [[nodiscard]] bool HasActiveState() const noexcept;
    [[nodiscard]] bool HasPendingTransition() const noexcept;
    [[nodiscard]] GameState* CurrentState() const noexcept;

private:
    std::unique_ptr<GameState> current_state_;
    std::unique_ptr<GameState> pending_state_;
    StateFactory pending_factory_;
};

} // namespace eco::app
