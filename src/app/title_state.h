#pragma once

#include "app/game_state.h"

namespace eco::app {

// Schermata del titolo con paesaggio procedurale animato (Valle del Crati, Ponte Bucci, Diga).
class TitleState : public GameState {
public:
    explicit TitleState(AppContext& ctx);

    void OnEnter() override;
    void Update(float dt) override;
    void Draw() override;

    [[nodiscard]] const char* Name() const noexcept override { return "TitleState"; }

private:
    AppContext& ctx_;
    float anim_time_ = 0.0f;
};

} // namespace eco::app
