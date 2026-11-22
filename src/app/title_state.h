#pragma once

#include "app/game_state.h"
#include "core/save_data.h"

namespace eco::app {

// Schermata del titolo con menu interattivo "Riprendi da dove hai lasciato" e
// sfondo procedurale ridisegnato dinamicamente in base alla zona o struttura dell'ultimo salvataggio.
class TitleState : public GameState {
public:
    explicit TitleState(AppContext& ctx);

    void OnEnter() override;
    void Update(float dt) override;
    void Draw() override;

    void RefreshSaveSummary();

    [[nodiscard]] const char* Name() const noexcept override { return "TitleState"; }

private:
    AppContext& ctx_;
    float anim_time_ = 0.0f;
    eco::SaveSummary save_summary_{};
    int menu_cursor_ = 0;
};

} // namespace eco::app
