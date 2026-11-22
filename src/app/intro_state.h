#pragma once

#include <string>
#include <vector>

#include "app/game_state.h"

namespace eco::app {

// Scena introduttiva con testo narrativo a macchina da scrivere (IntroCrawl).
class IntroState : public GameState {
public:
    explicit IntroState(AppContext& ctx);

    void OnEnter() override;
    void Update(float dt) override;
    void Draw() override;

    [[nodiscard]] const char* Name() const noexcept override { return "IntroState"; }

private:
    AppContext& ctx_;
    std::vector<std::string> lines_{};
    float elapsed_ = 0.0f;
    std::size_t total_chars_ = 0;
};

} // namespace eco::app
