#include "app/game_state_manager.h"

#include <utility>

namespace eco::app {

GameStateManager::~GameStateManager() {
    if (current_state_) {
        current_state_->OnExit();
        current_state_.reset();
    }
}

void GameStateManager::RequestTransition(std::unique_ptr<GameState> next_state) {
    if (!next_state) {
        return;
    }
    pending_factory_ = nullptr;
    pending_state_ = std::move(next_state);
}

void GameStateManager::RequestTransition(StateFactory factory) {
    if (!factory) {
        return;
    }
    pending_state_.reset();
    pending_factory_ = std::move(factory);
}

bool GameStateManager::ApplyPendingTransition() {
    std::unique_ptr<GameState> next = std::move(pending_state_);
    StateFactory factory = std::move(pending_factory_);
    pending_state_.reset();
    pending_factory_ = nullptr;

    if (!next && !factory) {
        return false;
    }

    if (!next && factory) {
        next = factory();
        if (!next) {
            return false;
        }
    }

    if (current_state_) {
        current_state_->OnExit();
        current_state_.reset();
    }

    current_state_ = std::move(next);
    current_state_->OnEnter();
    return true;
}

void GameStateManager::Update(float dt) {
    if (current_state_) {
        current_state_->Update(dt);
    }
    ApplyPendingTransition();
}

void GameStateManager::Draw() {
    if (current_state_) {
        current_state_->Draw();
    }
}

bool GameStateManager::HasActiveState() const noexcept {
    return current_state_ != nullptr;
}

bool GameStateManager::HasPendingTransition() const noexcept {
    return pending_state_ != nullptr || static_cast<bool>(pending_factory_);
}

GameState* GameStateManager::CurrentState() const noexcept {
    return current_state_.get();
}

} // namespace eco::app
