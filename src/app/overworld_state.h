#pragma once

#include "app/game_state.h"

namespace eco::app {

class OverworldState final : public GameState {
public:
    explicit OverworldState(AppContext& ctx);

    void OnEnter() override;
    void OnExit() override;
    void Update(float dt) override;
    void Draw() override;
    [[nodiscard]] const char* Name() const noexcept override { return "OverworldState"; }

private:
    AppContext& ctx_;
};

} // namespace eco::app
