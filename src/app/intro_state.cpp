#include "app/intro_state.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <memory>

#include "app/game_state_manager.h"
#include "app/overworld_state.h"
#include "app/render/palette.h"
#include "app/render/ui_style.h"
#include "core/input_types.h"
#include "core/math_types.h"
#include "core/simulation.h"
#include "core/story.h"

namespace eco::app {

namespace {

inline constexpr float kCharsPerSecond = 52.0f;

std::vector<std::string> FallbackIntroLines() {
    return {
        "ARCHIVIO OSSERVATORIO DEL CRATI // ANNO 2091",
        "Dopo il Grande Smog e l'Oblio, le formule della fisica sono andate perdute.",
        "Chi sopravvive brucia scorie nei generatori del Consorzio Brace, soffocando la Valle.",
        "Ti risvegli tra le rovine del campus universitario. Il tuo taccuino dice: ADA FERRI.",
        "Accanto a te, uno scanner empirico risponde ancora alle leggi della natura.",
        "Misura. Calcola. Rifiuta le scorciatoie inquinanti. Scopri chi osserva la Valle."
    };
}

} // namespace

IntroState::IntroState(AppContext& ctx) : ctx_(ctx) {}

void IntroState::OnEnter() {
    elapsed_ = 0.0f;
    lines_ = eco::IntroCrawl();
    if (lines_.empty()) {
        lines_ = FallbackIntroLines();
    }
    total_chars_ = 0;
    for (const auto& line : lines_) {
        total_chars_ += line.size();
    }
}

void IntroState::Update(float dt) {
    if (std::isfinite(dt) && dt > 0.0f) {
        elapsed_ += dt;
    }

    const auto revealed = static_cast<std::size_t>(elapsed_ * kCharsPerSecond);
    const bool all_revealed = (revealed >= total_chars_);

    const eco::PlayerInput& p0 = ctx_.input.Get(0);
    // X / Backspace salta immediatamente al gioco
    if (p0.cancel_pressed) {
        ctx_.state_manager.RequestTransition(std::make_unique<OverworldState>(ctx_));
        return;
    }

    if (p0.confirm_pressed || p0.interact_pressed || p0.jump_pressed ||
        ctx_.input.advance_dialogue_pressed) {
        if (!all_revealed && total_chars_ > 0) {
            elapsed_ = static_cast<float>(total_chars_ + 10) / kCharsPerSecond;
        } else {
            ctx_.state_manager.RequestTransition(std::make_unique<OverworldState>(ctx_));
        }
    }
}

void IntroState::Draw() {
    const int w = eco::kWindowWidth;
    const int h = eco::kWindowHeight;

    DrawRectangleGradientV(0, 0, w, h, Color{10, 16, 24, 255}, Color{20, 32, 42, 255});

    // Sottile griglia di terminale scientifico sullo sfondo
    for (int gx = 64; gx < w; gx += 64) {
        DrawLine(gx, 0, gx, h, Color{38, 70, 92, 35});
    }
    for (int gy = 64; gy < h; gy += 64) {
        DrawLine(0, gy, w, gy, Color{38, 70, 92, 35});
    }

    const Rectangle card{110.0f, 56.0f, static_cast<float>(w - 220), static_cast<float>(h - 120)};
    DrawPanel(card, Color{85, 210, 195, 220});

    DrawTextShadow(
        "PROLOGO // DIARIO DI BORDO - VALLE DEL CRATI",
        148,
        82,
        20,
        Color{125, 245, 220, 255});
    DrawLine(148, 112, w - 148, 112, Color{70, 155, 175, 180});

    std::size_t budget = static_cast<std::size_t>(elapsed_ * kCharsPerSecond);
    int y = 138;
    const int line_step = std::min(40, (h - 270) / std::max(1, static_cast<int>(lines_.size())));

    for (std::size_t i = 0; i < lines_.size(); ++i) {
        const std::string& full = lines_[i];
        if (budget == 0) {
            break;
        }
        const std::size_t count = std::min(budget, full.size());
        budget -= count;
        const std::string sub = full.substr(0, count);

        const Color col = (i == 0)
                              ? Color{245, 205, 115, 255}
                              : Color{218, 234, 242, 255};
        DrawTextShadow(sub.c_str(), 152, y, 17, col);
        y += line_step;
    }

    const bool all_revealed =
        (static_cast<std::size_t>(elapsed_ * kCharsPerSecond) >= total_chars_);
    const char* footer_prompt = all_revealed
                                    ? "Premi [INVIO] / [E] / [N] per iniziare l'esplorazione"
                                    : "Premi [INVIO] / [N] per completare il testo | [X] Salta";
    const int pw = MeasureUiText(footer_prompt, 16);
    const float pulse = 0.6f + 0.4f * std::sin(elapsed_ * 4.5f);
    DrawTextShadow(
        footer_prompt,
        (w - pw) / 2,
        h - 102,
        16,
        Color{245, 215, 125, static_cast<unsigned char>(160 + 95 * pulse)});
}

} // namespace eco::app
