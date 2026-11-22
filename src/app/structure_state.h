#pragma once

#include <array>

#include "app/game_state.h"
#include "core/math_types.h"

namespace eco::app {

class StructureState final : public GameState {
public:
    explicit StructureState(AppContext& ctx);

    void OnEnter() override;
    void OnExit() override;
    void Update(float dt) override;
    void Draw() override;
    [[nodiscard]] const char* Name() const noexcept override { return "StructureState"; }

private:
    AppContext& ctx_;
    std::array<eco::Vec2, eco::kMaxPlayers> last_input_axes_{};
};

} // namespace eco::app
