#include "app/intro_state.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string_view>

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

inline constexpr float kCharsPerSecond = 54.0f;

std::vector<std::string> FallbackIntroLines() {
    return {
        "TACCUINO DI CAMPO DI ELENA VALLI - LA CONCA DI MERIDIA",
        "Secoli fa i Meridiani costruirono una conca alimentata soltanto dalla luce, dall'acqua, dal moto dei pendoli e dal respiro della terra.",
        "Dopo l'Inverno di Pece le loro formule andarono disperse e la Compagnia della Pece riaccese le fornaci fumanti per scaldare i villaggi a valle.",
        "Oggi risalgo le Terrazze dell'Alba con la mia Lente Empirica e un diario vuoto, decisa a misurare di nuovo le leggi della natura.",
        "Ogni monumento calibrato senza fuoco restituisce respiro alla Conca; ogni reliquia ritrovata ricompone la memoria delle quattro ere perdute.",
        "Se sapro' leggere la fisica incisa nella pietra, l'Astrolabio del Santuario potra' tornare a girare."
    };
}

std::vector<std::string> WrapIntroParagraph(const std::string& text, int max_width_px, int font_size) {
    std::vector<std::string> out;
    if (text.empty()) {
        return out;
    }

    std::string current;
    std::size_t i = 0;
    while (i < text.size()) {
        while (i < text.size() && text[i] == ' ') {
            ++i;
        }
        if (i >= text.size()) {
            break;
        }
        std::size_t j = i;
        while (j < text.size() && text[j] != ' ') {
            ++j;
        }
        const std::string word = text.substr(i, j - i);
        const std::string candidate = current.empty() ? word : (current + " " + word);
        if (!current.empty() && MeasureUiText(candidate.c_str(), font_size) > max_width_px) {
            out.push_back(current);
            current = word;
        } else {
            current = candidate;
        }
        i = j;
    }
    if (!current.empty()) {
        out.push_back(current);
    }
    return out;
}

} // namespace

IntroState::IntroState(AppContext& ctx) : ctx_(ctx) {}

void IntroState::OnEnter() {
    elapsed_ = 0.0f;
    lines_ = eco::IntroCrawl();
    if (lines_.empty()) {
        lines_ = FallbackIntroLines();
    }

    wrapped_lines_.clear();
    is_paragraph_header_.clear();
    total_chars_ = 0;

    const int max_text_w = eco::kWindowWidth - 310;
    for (std::size_t idx = 0; idx < lines_.size(); ++idx) {
        const auto wrapped = WrapIntroParagraph(lines_[idx], max_text_w, 17);
        for (const auto& wline : wrapped) {
            wrapped_lines_.push_back(wline);
            is_paragraph_header_.push_back(idx == 0);
            total_chars_ += wline.size();
        }
        // Spazio sottile tra paragrafi se ci sono poche righe
        if (idx + 1 < lines_.size() && wrapped_lines_.size() < 12) {
            wrapped_lines_.emplace_back();
            is_paragraph_header_.push_back(false);
        }
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

    DrawRectangleGradientV(0, 0, w, h, Color{12, 18, 26, 255}, Color{22, 34, 44, 255});

    // Archi cartografici e meridiana sullo sfondo (stile taccuino naturalistico)
    const Color geo_col{168, 146, 98, 28};
    DrawCircleLines(w / 2, h / 2, 260.0f, geo_col);
    DrawCircleLines(w / 2, h / 2, 390.0f, geo_col);
    DrawLine(0, h / 2, w, h / 2, Color{120, 170, 180, 22});
    DrawLine(w / 2, 0, w / 2, h, Color{120, 170, 180, 22});

    const Rectangle card{104.0f, 52.0f, static_cast<float>(w - 208), static_cast<float>(h - 108)};
    DrawPanel(card, Color{102, 212, 192, 224});

    DrawTextShadow(
        "PROLOGO - TACCUINO DI CAMPO DI ELENA VALLI",
        144,
        78,
        20,
        Color{140, 242, 218, 255});
    DrawLine(144, 110, w - 144, 110, Color{198, 168, 108, 185});

    std::size_t budget = static_cast<std::size_t>(elapsed_ * kCharsPerSecond);
    int y = 132;
    const int non_empty_count = std::max(1, static_cast<int>(wrapped_lines_.size()));
    const int line_step = std::clamp((h - 268) / non_empty_count, 22, 34);

    for (std::size_t i = 0; i < wrapped_lines_.size(); ++i) {
        const std::string& full = wrapped_lines_[i];
        if (full.empty()) {
            y += line_step / 2;
            continue;
        }
        if (budget == 0) {
            break;
        }
        const std::size_t count = std::min(budget, full.size());
        budget -= count;
        const std::string sub = full.substr(0, count);

        const Color col = is_paragraph_header_[i]
                              ? Color{242, 206, 120, 255}
                              : Color{224, 236, 242, 255};
        DrawTextShadow(sub.c_str(), 148, y, 17, col);
        y += line_step;
    }

    const bool all_revealed =
        (static_cast<std::size_t>(elapsed_ * kCharsPerSecond) >= total_chars_);
    const char* footer_prompt = all_revealed
                                    ? "Premi [INVIO] / [E] / [N] per scendere nelle Terrazze dell'Alba"
                                    : "Premi [INVIO] / [N] per completare il testo | [X] Salta";
    const int pw = MeasureUiText(footer_prompt, 16);
    const float pulse = 0.6f + 0.4f * std::sin(elapsed_ * 4.2f);
    DrawTextShadow(
        footer_prompt,
        (w - pw) / 2,
        h - 96,
        16,
        Color{244, 212, 126, static_cast<unsigned char>(165 + 90 * pulse)});
}

} // namespace eco::app
