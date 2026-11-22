#include "app/story_ui.h"

#include <raylib.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

#include "app/render/ui_style.h"
#include "core/math_types.h"
#include "core/story.h"

namespace eco::app {

namespace {

[[nodiscard]] bool StartsWithIgnoreCase(std::string_view text, std::string_view prefix) noexcept {
    if (text.size() < prefix.size()) {
        return false;
    }
    for (std::size_t i = 0; i < prefix.size(); ++i) {
        const auto a = static_cast<unsigned char>(text[i]);
        const auto b = static_cast<unsigned char>(prefix[i]);
        if (std::toupper(a) != std::toupper(b)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] Color SpeakerAccentColor(std::string_view speaker, bool& is_gaia) noexcept {
    is_gaia = false;
    if (StartsWithIgnoreCase(speaker, "GAIA") || StartsWithIgnoreCase(speaker, "// GAIA")) {
        is_gaia = true;
        return Color{85, 255, 175, 255};
    }
    if (StartsWithIgnoreCase(speaker, "LIA") || StartsWithIgnoreCase(speaker, "ARCHITETT")) {
        return Color{245, 198, 88, 255};
    }
    if (StartsWithIgnoreCase(speaker, "BRUNO") || StartsWithIgnoreCase(speaker, "CONSORZIO")) {
        return Color{238, 112, 62, 255};
    }
    if (StartsWithIgnoreCase(speaker, "NONNA") || StartsWithIgnoreCase(speaker, "ROSA")) {
        return Color{105, 210, 250, 255};
    }
    // Default: Ada Ferri (ciano ricercatrice)
    return Color{120, 238, 242, 255};
}

[[nodiscard]] const char* FallbackChapterTitle(eco::Chapter c) noexcept {
    switch (c) {
        case eco::Chapter::Prologo:
            return "PROLOGO // IL RISVEGLIO";
        case eco::Chapter::Atto1_Luce:
            return "ATTO I // LA LUCE DEL CRATI";
        case eco::Chapter::Atto2_Acqua:
            return "ATTO II // L'ACQUA DELLA DIGA";
        case eco::Chapter::Atto3_Codice:
            return "ATTO III // IL CODICE DI GAIA";
        case eco::Chapter::Epilogo:
            return "EPILOGO // EQUILIBRIO";
    }
    return "EQUILIBRIO";
}

[[nodiscard]] eco::Objective FallbackObjective(const eco::WorldContext& world) {
    if (world.simulation_completed) {
        return {"Valle in Equilibrio", "Esplora la Valle o premi [ESC] per uscire"};
    }
    if (!world.last_scan.active && world.solar_machine.state == eco::MachineState::Broken) {
        return {"Misura il Campo Solare", "Avvicinati al pannello e premi [F] per scansionare"};
    }
    if (world.solar_machine.state == eco::MachineState::Broken) {
        return {"Calibra il Campo Solare", "Premi [E] sul pannello e allinea l'angolo con [A]/[D]"};
    }
    if (!world.ecosystem.architect_recruited) {
        return {"Parla con Lia l'Architetta", "Raggiungi Lia a est del campo e premi [E]"};
    }
    if (world.hydro_machine.state == eco::MachineState::Broken) {
        return {"Riattiva la Diga del Crati", "Entra nel portale [DIGA] a nord e calibra la turbina"};
    }
    return {"Attiva il Risonatore Est", "Attraversa il Ponte Bucci e calibra il Risonatore [E]"};
}

} // namespace

void DrawStoryLayer(eco::WorldContext& world) {
    const eco::StoryDirector& story = world.story;
    const eco::Chapter chapter = story.CurrentChapter();

    // 1. Pannello Capitolo & Obiettivo in alto a sinistra (x=16, y=62, w=410, h=64)
    const char* raw_chap = eco::ChapterTitle(chapter);
    const char* chap_title =
        (raw_chap != nullptr && raw_chap[0] != '\0') ? raw_chap : FallbackChapterTitle(chapter);

    eco::Objective obj = story.CurrentObjective();
    if (obj.title.empty()) {
        obj = FallbackObjective(world);
    }

    const Rectangle obj_box{16.0f, 62.0f, 410.0f, 64.0f};
    DrawPanel(obj_box, Color{78, 200, 195, 215});
    DrawTextShadow(chap_title, 28, 69, 13, Color{125, 240, 215, 255});
    DrawTextShadow(obj.title.c_str(), 28, 86, 16, Color{245, 235, 190, 255});
    if (!obj.hint.empty()) {
        DrawTextShadow(obj.hint.c_str(), 28, 106, 12, Color{185, 215, 230, 240});
    }

    // 2. Schermata Epilogo / Finale quando simulation_completed == true (e il modal e' chiuso)
    if (world.simulation_completed && !world.calibration_modal.is_open) {
        const bool clean_ending =
            story.CleanPath() &&
            world.solar_machine.state == eco::MachineState::CleanActive &&
            world.hydro_machine.state == eco::MachineState::CleanActive &&
            world.purifier_machine.state == eco::MachineState::CleanActive;

        std::vector<std::string> ending_lines = eco::EndingText(clean_ending);
        if (ending_lines.empty()) {
            if (clean_ending) {
                ending_lines = {
                    "EPILOGO // EQUILIBRIO PERFETTO (FINALE PULITO)",
                    "Lo smog si dissolve sopra il Ponte Bucci. Il Crati scorre limpido.",
                    "Ogni impianto funziona grazie al calcolo empirico, senza bruciare scorie.",
                    "// GAIA [SYS]: Test superato. Gli umani hanno scelto la via della scienza.",
                    "Ma nei vettori della Fase 3 resta una domanda: chi ha costruito GAIA?"
                };
            } else {
                ending_lines = {
                    "EPILOGO // EQUILIBRIO INCOMPLETO (FINALE GRIGIO)",
                    "La rete elettrica della Valle e' attiva, ma i generatori a combustione fumano.",
                    "Lia osserva l'orizzonte color ruggine: la scorciatoia ha lasciato il segno.",
                    "// GAIA [SYS]: Simulazione conclusa con residuo entropico elevato.",
                    "Forse un'altra iterazione dimostrera' che la matematica basta da sola."
                };
            }
        }

        const int w = eco::kWindowWidth;
        const int card_w = 880;
        const int card_h = 260;
        const int cx = (w - card_w) / 2;
        const int cy = 430;
        const Color border_col = clean_ending
                                     ? Color{90, 250, 190, 245}
                                     : Color{235, 145, 70, 245};

        DrawPanel(
            Rectangle{
                static_cast<float>(cx),
                static_cast<float>(cy),
                static_cast<float>(card_w),
                static_cast<float>(card_h)},
            border_col);

        const char* header = clean_ending
                                 ? "SIMULAZIONE DECIFRATA // FINALE PULITO (0% BYPASS)"
                                 : "VALLE RIACCESA // FINALE GRIGIO ( SCORCIATOIA FOSSILE )";
        DrawTextShadow(header, cx + 26, cy + 16, 19, border_col);
        DrawLine(cx + 26, cy + 44, cx + card_w - 26, cy + 44, border_col);

        int ly = cy + 56;
        for (std::size_t i = 0; i < ending_lines.size() && ly < cy + card_h - 36; ++i) {
            DrawTextShadow(
                ending_lines[i].c_str(),
                cx + 28,
                ly,
                15,
                (i == 0) ? Color{250, 230, 160, 255} : Color{220, 236, 245, 255});
            ly += 26;
        }

        DrawTextShadow(
            "Puoi continuare a esplorare liberamente la Valle del Crati.",
            cx + 28,
            cy + card_h - 26,
            13,
            Color{160, 200, 215, 225});
        return;
    }

    // 3. Dialogue Box in basso (x=140, y=614, w=1000, h=90) — nascosto se il modal e' aperto
    if (world.calibration_modal.is_open || !story.HasActiveDialogue()) {
        return;
    }

    const eco::DialogueLine* line = story.CurrentLine();
    if (line == nullptr || line->text.empty()) {
        return;
    }

    bool is_gaia = false;
    const Color speaker_col = SpeakerAccentColor(line->speaker, is_gaia);

    float jitter_x = 0.0f;
    float jitter_y = 0.0f;
    if (is_gaia) {
        jitter_x = std::sin(world.sim_time * 37.0f) * 1.6f;
        jitter_y = std::cos(world.sim_time * 29.0f) * 1.0f;
    }

    const Rectangle dlg_box{140.0f + jitter_x, 614.0f + jitter_y, 1000.0f, 90.0f};
    DrawPanel(dlg_box, speaker_col);

    // Badge speaker
    const std::string speaker_label =
        line->speaker.empty() ? std::string("ADA") : line->speaker;
    DrawTextShadow(
        speaker_label.c_str(),
        static_cast<int>(dlg_box.x) + 20,
        static_cast<int>(dlg_box.y) + 12,
        16,
        speaker_col);

    // Effetto macchina da scrivere basato su LineElapsed()
    const float elapsed = std::max(0.0f, story.LineElapsed());
    const auto visible_chars = std::min<std::size_t>(
        line->text.size(),
        static_cast<std::size_t>(elapsed * 55.0f) + 1);
    const std::string revealed_text = line->text.substr(0, visible_chars);

    DrawTextShadow(
        revealed_text.c_str(),
        static_cast<int>(dlg_box.x) + 20,
        static_cast<int>(dlg_box.y) + 38,
        16,
        is_gaia ? Color{170, 255, 210, 255} : Color{235, 242, 248, 255});

    // Indicatore "[N] Avanti" in basso a destra del box
    const char* next_hint = "[N] Avanti";
    const int hint_w = MeasureText(next_hint, 13);
    DrawTextShadow(
        next_hint,
        static_cast<int>(dlg_box.x + dlg_box.width) - hint_w - 18,
        static_cast<int>(dlg_box.y + dlg_box.height) - 22,
        13,
        Color{175, 205, 220, 220});
}

} // namespace eco::app
