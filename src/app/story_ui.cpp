#include "app/story_ui.h"

#include <raylib.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "app/render/palette.h"
#include "app/render/ui_style.h"
#include "core/collectible.h"
#include "core/machine.h"
#include "core/math_types.h"
#include "core/story.h"

namespace eco::app {

namespace {

[[nodiscard]] bool ContainsIgnoreCase(std::string_view haystack, std::string_view needle) noexcept {
    if (needle.empty() || haystack.size() < needle.size()) {
        return false;
    }
    for (std::size_t i = 0; i + needle.size() <= haystack.size(); ++i) {
        bool match = true;
        for (std::size_t j = 0; j < needle.size(); ++j) {
            const auto a = static_cast<unsigned char>(haystack[i + j]);
            const auto b = static_cast<unsigned char>(needle[j]);
            if (std::toupper(a) != std::toupper(b)) {
                match = false;
                break;
            }
        }
        if (match) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] Color SpeakerAccentColor(std::string_view speaker, bool& is_echo) noexcept {
    is_echo = false;
    if (ContainsIgnoreCase(speaker, "ECO") || ContainsIgnoreCase(speaker, "MERIDIA")) {
        is_echo = true;
        return pal::kJadeGlow;
    }
    if (ContainsIgnoreCase(speaker, "CLARA") || ContainsIgnoreCase(speaker, "SERENI")) {
        return pal::kBronzeBright;
    }
    if (ContainsIgnoreCase(speaker, "LORENZO") || ContainsIgnoreCase(speaker, "ALTIERI")) {
        return Color{150, 206, 242, 255};
    }
    if (ContainsIgnoreCase(speaker, "MIRIAM") || ContainsIgnoreCase(speaker, "FERRO")) {
        return Color{102, 232, 168, 255};
    }
    if (ContainsIgnoreCase(speaker, "MATTEO") || ContainsIgnoreCase(speaker, "VERRINI")) {
        return pal::kAccentDirty;
    }
    // Default: Elena Valli (ciano naturalista)
    return Color{96, 232, 242, 255};
}

[[nodiscard]] Color EraAccentColor(eco::CollectibleEra era) noexcept {
    switch (era) {
        case eco::CollectibleEra::Era1_Dawn:
            return pal::kBronzeBright;
        case eco::CollectibleEra::Era2_WatersAndSound:
            return Color{96, 218, 244, 255};
        case eco::CollectibleEra::Era3_PitchWinter:
            return pal::kAccentDirty;
        case eco::CollectibleEra::Era4_HarmonySeal:
            return pal::kJadeGlow;
        case eco::CollectibleEra::Count:
            break;
    }
    return pal::kBronze;
}

[[nodiscard]] const char* ShortChapterLabel(eco::Chapter c) noexcept {
    switch (c) {
        case eco::Chapter::Prologo:
            return "Prologo";
        case eco::Chapter::Atto1_Luce:
            return "Cap. I";
        case eco::Chapter::Atto2_Acqua:
            return "Cap. II";
        case eco::Chapter::Atto3_Pendolo:
            return "Cap. III";
        case eco::Chapter::Atto4_Giardini:
            return "Cap. IV";
        case eco::Chapter::Atto5_Fumarole:
            return "Cap. V";
        case eco::Chapter::Epilogo:
            return "Epilogo";
    }
    return "Cap.";
}

[[nodiscard]] const char* ShortEraLabel(int filter_index) noexcept {
    switch (filter_index) {
        case -1:
            return "Tutte (16)";
        case 0:
            return "Era I: Alba";
        case 1:
            return "Era II: Acque";
        case 2:
            return "Era III: Pece";
        case 3:
            return "Era IV: Armonia";
        default:
            return "Tutte";
    }
}

[[nodiscard]] const char* FallbackChapterTitle(eco::Chapter c) noexcept {
    switch (c) {
        case eco::Chapter::Prologo:
            return "PROLOGO - LE TERRAZZE DELL'ALBA";
        case eco::Chapter::Atto1_Luce:
            return "CAPITOLO I - LA MISURA DELLA LUCE";
        case eco::Chapter::Atto2_Acqua:
            return "CAPITOLO II - IL PESO DELL'ACQUA";
        case eco::Chapter::Atto3_Pendolo:
            return "CAPITOLO III - IL TEMPO DEL PENDOLO";
        case eco::Chapter::Atto4_Giardini:
            return "CAPITOLO IV - L'ACCORDO DEI GIARDINI";
        case eco::Chapter::Atto5_Fumarole:
            return "CAPITOLO V - LE CENERI DELLA FRATTURA";
        case eco::Chapter::Epilogo:
            return "EPILOGO - IL CUORE DI MERIDIA";
    }
    return "LA CONCA DI MERIDIA";
}

[[nodiscard]] eco::Objective FallbackObjective(const eco::WorldContext& world) {
    if (world.simulation_completed) {
        return {"Conca di Meridia risvegliata", "Esplora i distretti o apri [H] Diario / [J] Codex"};
    }
    const auto& solar = world.GetMachine(eco::MachineType::SolarArray);
    if (!world.last_scan.active && solar.state == eco::MachineState::Broken) {
        return {"Misura l'Eliostato Solare", "Premi [F] vicino al monumento nelle Terrazze dell'Alba"};
    }
    if (solar.state == eco::MachineState::Broken) {
        return {"Calibra l'Eliostato Solare", "Premi [E] sul monumento e allinea l'angolo con [A]/[D]"};
    }
    if (!world.GetNpc(eco::NpcId::ClaraSereni).recruited) {
        return {"Parla con Clara Sereni", "Raggiungi il Quartiere dei Canali e premi [E] vicino a Clara"};
    }
    if (world.GetMachine(eco::MachineType::HydroTurbine).state == eco::MachineState::Broken) {
        return {"Riattiva le Cisterne Ipogee", "Entra nel portale delle Cisterne e calibra la Turbina"};
    }
    if (world.GetMachine(eco::MachineType::PendulumRegulator).state == eco::MachineState::Broken) {
        return {"Regola il Pendolo Isochronos", "Sali sull'Altopiano dei Venti ed entra nella Torre del Pendolo"};
    }
    if (world.GetMachine(eco::MachineType::HarmonicResonator).state == eco::MachineState::Broken) {
        return {"Accorda il Risonatore Armonico", "Attraversa il Ponte degli Archi verso i Giardini Sospesi"};
    }
    if (world.GetMachine(eco::MachineType::ThermalExchanger).state == eco::MachineState::Broken) {
        return {"Equilibra la Camera Geotermica", "Raggiungi la Caldera delle Fumarole a nord-est"};
    }
    return {"Risveglia l'Astrolabio Centrale", "Raggiungi il Santuario dell'Armonia a sud-est e premi [E]"};
}

[[nodiscard]] std::vector<std::string> WrapTextToWidth(
    std::string_view text,
    int max_width_px,
    int font_size) {
    std::vector<std::string> lines;
    if (text.empty()) {
        return lines;
    }

    const int safe_max_w = std::max(80, max_width_px);
    std::string current;
    std::size_t i = 0;

    while (i < text.size()) {
        if (text[i] == '\n') {
            lines.push_back(current);
            current.clear();
            ++i;
            continue;
        }
        while (i < text.size() && text[i] == ' ') {
            ++i;
        }
        if (i >= text.size()) {
            break;
        }
        std::size_t j = i;
        while (j < text.size() && text[j] != ' ' && text[j] != '\n') {
            ++j;
        }
        const std::string word(text.substr(i, j - i));
        const std::string candidate = current.empty() ? word : (current + " " + word);

        if (!current.empty() && MeasureUiText(candidate.c_str(), font_size) > safe_max_w) {
            lines.push_back(current);
            current = word;
        } else {
            current = candidate;
        }
        i = j;
    }

    if (!current.empty()) {
        lines.push_back(current);
    }
    return lines;
}

[[nodiscard]] float ComputeBannerAlpha(float remaining, float total_duration) noexcept {
    if (remaining <= 0.0f || total_duration <= 0.0f) {
        return 0.0f;
    }
    const float elapsed = std::max(0.0f, total_duration - remaining);
    const float fade_in = std::clamp(elapsed / 0.35f, 0.0f, 1.0f);
    const float fade_out = std::clamp(remaining / 0.65f, 0.0f, 1.0f);
    return std::min(fade_in, fade_out);
}

[[nodiscard]] std::vector<const eco::CollectibleItem*> BuildFilteredCodexItems(
    const eco::CollectibleRegistry& reg,
    int era_filter) {
    std::vector<const eco::CollectibleItem*> items;
    items.reserve(reg.Items().size());
    for (const auto& item : reg.Items()) {
        if (era_filter < 0 || static_cast<int>(item.era) == era_filter) {
            items.push_back(&item);
        }
    }
    return items;
}

} // namespace

void DrawStoryLayer(eco::WorldContext& world) {
    const eco::StoryDirector& story = world.story;
    const eco::Chapter chapter = story.CurrentChapter();

    // 1. Pannello Capitolo & Obiettivo in alto a sinistra (x=16, y=60, w=412, h=70)
    const char* raw_chap = eco::ChapterTitle(chapter);
    const char* chap_title =
        (raw_chap != nullptr && raw_chap[0] != '\0') ? raw_chap : FallbackChapterTitle(chapter);

    eco::Objective obj = story.CurrentObjective();
    if (obj.title.empty()) {
        obj = FallbackObjective(world);
    }

    const Rectangle obj_box{16.0f, 60.0f, 412.0f, 70.0f};
    DrawPanel(obj_box, pal::kUiBorderBronze);
    DrawTextShadow(chap_title, 28, 67, 12, pal::kJadeGlow);
    DrawTextShadow(obj.title.c_str(), 28, 84, 16, pal::kParchmentText);
    if (!obj.hint.empty()) {
        const auto hint_lines = WrapTextToWidth(obj.hint, 386, 12);
        if (!hint_lines.empty()) {
            DrawTextShadow(hint_lines[0].c_str(), 28, 106, 12, Color{186, 212, 224, 240});
        }
    }

    // 2. Banner Toast Diegetico in alto al centro (x=440, y=60, w=400, h=66)
    //    Priorita': raccolta reliquia -> ingresso nuova zona/struttura.
    if (!world.calibration_modal.is_open) {
        if (world.collectibles.HasRecentPickup()) {
            const float alpha = ComputeBannerAlpha(
                world.collectibles.PickupBannerTimer(),
                eco::kRelicBannerDuration);
            if (alpha > 0.01f) {
                const auto& meta = eco::CollectibleMetadata(world.collectibles.LastCollectedId());
                const std::string eyebrow =
                    "RELIQUIA RITROVATA  |  " + (meta.code.empty() ? std::string("ARCHIVIO") : meta.code) +
                    "  (" + std::to_string(world.collectibles.CollectedCount()) + "/" +
                    std::to_string(world.collectibles.TotalCount()) + ")";
                const std::string title =
                    meta.title.empty() ? std::string("Frammento dei Meridiani") : meta.title;
                const std::string sub =
                    !meta.era_name.empty() ? (meta.era_name + "  |  [J] Apri Codex")
                                           : std::string("Premi [J] per consultare il Codex");
                DrawBannerToast(
                    Rectangle{440.0f, 60.0f, 400.0f, 72.0f},
                    eyebrow.c_str(),
                    title.c_str(),
                    sub.c_str(),
                    EraAccentColor(meta.era),
                    alpha);
            }
        } else if (world.zone_banner_timer > 0.0f) {
            const float alpha = ComputeBannerAlpha(world.zone_banner_timer, eco::kZoneBannerDuration);
            if (alpha > 0.01f) {
                const char* eyebrow = world.banner_is_structure ? "STRUTTURA INTERNA" : "DISTRETTO DELLA CONCA";
                const char* title = world.banner_is_structure
                                        ? eco::StructureName(world.banner_structure)
                                        : eco::ZoneName(world.banner_zone);
                const char* sub = world.banner_is_structure
                                      ? eco::StructureSubtitle(world.banner_structure)
                                      : eco::ZoneSubtitle(world.banner_zone);
                const Color banner_accent = world.banner_is_structure
                                                ? pal::GetStructurePalette(world.banner_structure).accent_healthy
                                                : pal::GetDistrictPalette(world.banner_zone).ambient_accent;
                DrawBannerToast(
                    Rectangle{440.0f, 60.0f, 400.0f, 72.0f},
                    eyebrow,
                    title,
                    sub,
                    banner_accent,
                    alpha);
            }
        }
    }

    // 3. Schermata Epilogo / Tre Finali quando simulation_completed == true (e il modal e' chiuso)
    if (world.simulation_completed && !world.calibration_modal.is_open) {
        const bool clean_ending = story.CleanPath();
        const int relics_found = world.collectibles.CollectedCount();
        const int total_relics = world.collectibles.TotalCount();
        const bool true_ending = clean_ending && relics_found >= eco::kRelicsForTrueEnding;

        std::vector<std::string> ending_lines =
            eco::EndingText(clean_ending, relics_found, total_relics);
        if (ending_lines.empty()) {
            if (true_ending) {
                ending_lines = {
                    "ALBA DI MERIDIA - FINALE COMPLETO",
                    "Ogni monumento della Conca lavora in accordo con la natura, senza una sola fornace accesa.",
                    "Con le reliquie raccolte hai ricomposto l'intera storia delle quattro ere dei Meridiani.",
                    "L'Astrolabio Centrale torna a ruotare sopra il Santuario: la scienza torna a essere custodia."
                };
            } else if (clean_ending) {
                ending_lines = {
                    "RINASCITA DELLA CONCA - FINALE PULITO",
                    "Le cinque leggi fisiche illuminano di nuovo la Conca di Meridia senza bruciare pece.",
                    "Alcune reliquie riposano ancora tra le rovine: puoi continuare a cercarle per completare il Codex."
                };
            } else {
                ending_lines = {
                    "IL PREZZO DELLA FRETTA - FINALE VELATO",
                    "L'Astrolabio e' attivo, ma il fumo delle fornaci di emergenza pesa ancora sulle terrazze.",
                    "Matteo Verrini osserva i canali in silenzio: la scorciatoia ha dato luce subito, ma ha lasciato cenere."
                };
            }
        }

        const int w = eco::kWindowWidth;
        const int card_w = 910;
        const int card_h = 314;
        const int cx = (w - card_w) / 2;
        const int cy = 388;
        const Color border_col = true_ending
                                     ? pal::kBronzeBright
                                     : (clean_ending ? pal::kAccentClean : pal::kAccentDirty);

        const char* header = true_ending
                                 ? "EPILOGO - ALBA DI MERIDIA (0 BYPASS | ARCHIVIO COMPLETO)"
                                 : (clean_ending
                                        ? "EPILOGO - RINASCITA DELLA CONCA (0 BYPASS)"
                                        : "EPILOGO - IL PREZZO DELLA FRETTA (FORNACI ATTIVATE)");

        DrawPanelWithHeader(
            Rectangle{
                static_cast<float>(cx),
                static_cast<float>(cy),
                static_cast<float>(card_w),
                static_cast<float>(card_h)},
            42,
            header,
            border_col);

        int ly = cy + 54;
        const int max_line_w = card_w - 56;
        const int content_bottom_limit = cy + card_h - 46;
        for (std::size_t i = 0; i < ending_lines.size() && ly + 18 <= content_bottom_limit; ++i) {
            const auto wrapped = WrapTextToWidth(ending_lines[i], max_line_w, 15);
            for (const auto& wl : wrapped) {
                if (ly + 18 > content_bottom_limit) {
                    break;
                }
                DrawTextShadow(
                    wl.c_str(),
                    cx + 28,
                    ly,
                    15,
                    (i == 0) ? pal::kBronzeBright : pal::kParchmentText);
                ly += 22;
            }
            ly += 3;
        }

        DrawDivider(cx + 24, cy + card_h - 38, card_w - 48, border_col);
        const char* summary = TextFormat(
            "Reliquie nel Codex: %d/%d  |  Dialoghi nel Diario: %d  |  Premi [H] Cronologia, [J] Codex o esplora liberamente.",
            relics_found,
            total_relics,
            static_cast<int>(story.History().size()));
        DrawTextShadow(summary, cx + 28, cy + card_h - 27, 13, Color{176, 212, 224, 235});
        return;
    }

    // 4. Dialogue Box in basso (x=140, y=596, w=1000, h=108) — nascosto se il modal e' aperto
    if (world.calibration_modal.is_open || !story.HasActiveDialogue()) {
        return;
    }

    const eco::DialogueLine* line = story.CurrentLine();
    if (line == nullptr || line->text.empty()) {
        return;
    }

    bool is_echo = false;
    const Color speaker_col = SpeakerAccentColor(line->speaker, is_echo);

    // Per l'Eco di Meridia: leggero respiro luminoso del bordo, mai jitter da glitch/script.
    Color panel_accent = speaker_col;
    if (is_echo) {
        const float glow = 0.72f + 0.28f * std::sin(world.sim_time * 2.6f);
        panel_accent = pal::LerpColor(pal::kJade, pal::kJadeGlow, glow);
    }

    const Rectangle dlg_box{140.0f, 596.0f, 1000.0f, 108.0f};
    DrawPanel(dlg_box, panel_accent);

    // Badge speaker + piccolo indicatore di coda dialoghi
    const std::string speaker_label =
        line->speaker.empty() ? std::string("Elena Valli") : line->speaker;
    DrawTextShadow(
        speaker_label.c_str(),
        static_cast<int>(dlg_box.x) + 22,
        static_cast<int>(dlg_box.y) + 12,
        16,
        panel_accent);

    if (is_echo) {
        const int sp_w = MeasureUiText(speaker_label.c_str(), 16);
        DrawTextShadow(
            "- Risonanza Incisa",
            static_cast<int>(dlg_box.x) + 30 + sp_w,
            static_cast<int>(dlg_box.y) + 14,
            13,
            Color{154, 226, 206, 210});
    }

    // Effetto macchina da scrivere con word-wrap pulito su max 3 righe
    const float elapsed = std::max(0.0f, story.LineElapsed());
    const auto visible_chars = std::min<std::size_t>(
        line->text.size(),
        static_cast<std::size_t>(elapsed * 58.0f) + 1);
    const auto wrapped = WrapTextToWidth(line->text, static_cast<int>(dlg_box.width) - 46, 16);

    std::size_t char_budget = visible_chars;
    int text_y = static_cast<int>(dlg_box.y) + 36;
    const Color text_col = is_echo ? Color{198, 248, 232, 255} : pal::kParchmentText;
    for (std::size_t r = 0; r < wrapped.size() && r < 3; ++r) {
        if (char_budget == 0) {
            break;
        }
        const std::size_t take = std::min(char_budget, wrapped[r].size());
        char_budget -= take;
        // Considera lo spazio perso tra righe wrappate
        if (char_budget > 0 && take == wrapped[r].size()) {
            --char_budget;
        }
        const std::string sub = wrapped[r].substr(0, take);
        DrawTextShadow(sub.c_str(), static_cast<int>(dlg_box.x) + 22, text_y, 16, text_col);
        text_y += 20;
    }

    if (char_budget == 0 && visible_chars >= line->text.size()) {
        const char* continue_hint = "[E / INVIO] CONTINUA";
        const int hint_w = MeasureUiText(continue_hint, 12);
        DrawTextShadow(
            continue_hint,
            static_cast<int>(dlg_box.x + dlg_box.width) - hint_w - 18,
            static_cast<int>(dlg_box.y + dlg_box.height) - 18,
            12,
            pal::kJadeGlow);
    }

    // Suggerimenti compatti in basso a destra: [H] Cronologia (N) | [N] Avanti
    const int queued = story.QueuedLineCount();
    const char* right_hint =
        (queued > 1)
            ? TextFormat("[H] Cronologia (%d)   |   [N] Avanti (+%d)", static_cast<int>(story.History().size()), queued - 1)
            : TextFormat("[H] Cronologia (%d)   |   [N] Avanti", static_cast<int>(story.History().size()));
    const int hint_w = MeasureUiText(right_hint, 12);
    DrawTextShadow(
        right_hint,
        static_cast<int>(dlg_box.x + dlg_box.width) - hint_w - 18,
        static_cast<int>(dlg_box.y + dlg_box.height) - 20,
        12,
        Color{174, 206, 218, 220});
}

// =============================================================================
// JournalOverlay ([H] Cronologia Dialoghi & [J] Codex Collezionabili)
// =============================================================================

void JournalOverlay::OpenHistory() noexcept {
    is_open_ = true;
    active_tab_ = JournalTab::History;
    history_scroll_to_bottom_ = true;
}

void JournalOverlay::OpenCodex() noexcept {
    is_open_ = true;
    active_tab_ = JournalTab::Codex;
    codex_detail_scroll_px_ = 0.0f;
}

void JournalOverlay::Close() noexcept {
    is_open_ = false;
}

bool JournalOverlay::UpdateAndHandleInput(AppContext& ctx, bool allow_open) {
    // Apri con [H] o [J] quando siamo in gameplay (non su Titolo/Intro/Pausa/Calibrazione)
    if (!is_open_) {
        if (!allow_open) {
            return false;
        }
        if (IsKeyPressed(KEY_H)) {
            OpenHistory();
            last_seen_history_count_ = ctx.world.story.History().size();
            return true;
        }
        if (IsKeyPressed(KEY_J)) {
            OpenCodex();
            // Se esiste una reliquia raccolta di recente, selezionala automaticamente nel Codex
            if (ctx.world.collectibles.HasRecentPickup()) {
                era_filter_ = -1;
                codex_selected_row_ = static_cast<int>(ctx.world.collectibles.LastCollectedId());
            }
            return true;
        }
        return false;
    }

    // Chiusura o cambio tab rapido con [ESC], [H], [J], [TAB]
    if (IsKeyPressed(KEY_ESCAPE)) {
        Close();
        return false;
    }
    if (IsKeyPressed(KEY_H)) {
        if (active_tab_ == JournalTab::History) {
            Close();
            return false;
        }
        active_tab_ = JournalTab::History;
        history_scroll_to_bottom_ = true;
    }
    if (IsKeyPressed(KEY_J)) {
        if (active_tab_ == JournalTab::Codex) {
            Close();
            return false;
        }
        active_tab_ = JournalTab::Codex;
        codex_detail_scroll_px_ = 0.0f;
    }
    if (IsKeyPressed(KEY_TAB)) {
        active_tab_ = (active_tab_ == JournalTab::History) ? JournalTab::Codex : JournalTab::History;
        if (active_tab_ == JournalTab::History) {
            history_scroll_to_bottom_ = true;
        } else {
            codex_detail_scroll_px_ = 0.0f;
        }
    }

    // Coordinate pannello principale (1100 x 616 centrato su 1280 x 720)
    const Rectangle modal{90.0f, 52.0f, 1100.0f, 616.0f};
    const Rectangle tab_area{modal.x + 24.0f, modal.y + 54.0f, 520.0f, 36.0f};
    const Rectangle close_btn{modal.x + modal.width - 134.0f, modal.y + 54.0f, 110.0f, 34.0f};

    const Vector2 mouse = GetMousePosition();
    const bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    const float wheel = GetMouseWheelMove();

    if (clicked && CheckCollisionPointRec(mouse, close_btn)) {
        Close();
        return false;
    }

    // Click sulle 2 tab
    if (clicked && CheckCollisionPointRec(mouse, tab_area)) {
        const float half = tab_area.width * 0.5f;
        if (mouse.x < tab_area.x + half) {
            active_tab_ = JournalTab::History;
            history_scroll_to_bottom_ = true;
        } else {
            active_tab_ = JournalTab::Codex;
            codex_detail_scroll_px_ = 0.0f;
        }
    }

    const bool up_pressed = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
    const bool down_pressed = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
    const bool up_held = IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);
    const bool down_held = IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
    const bool left_pressed = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
    const bool right_pressed = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);

    if (active_tab_ == JournalTab::History) {
        // Filtro capitoli (-1 = Tutti, 0..6 = Prologo..Epilogo)
        constexpr int kFilterCount = 8; // -1..6
        if (left_pressed) {
            int idx = (chapter_filter_ + 1 - 1 + kFilterCount) % kFilterCount;
            chapter_filter_ = idx - 1;
            history_scroll_to_bottom_ = true;
        }
        if (right_pressed) {
            int idx = (chapter_filter_ + 1 + 1) % kFilterCount;
            chapter_filter_ = idx - 1;
            history_scroll_to_bottom_ = true;
        }

        // Click sui pulsanti filtro capitolo
        const float filter_y = modal.y + 100.0f;
        const float pill_w = 124.0f;
        const float pill_gap = 8.0f;
        for (int f = -1; f <= 6; ++f) {
            const int idx = f + 1;
            const Rectangle pill{
                modal.x + 24.0f + static_cast<float>(idx) * (pill_w + pill_gap),
                filter_y,
                pill_w,
                28.0f
            };
            if (clicked && CheckCollisionPointRec(mouse, pill)) {
                chapter_filter_ = f;
                history_scroll_to_bottom_ = true;
            }
        }

        // Scorrimento fluido della Cronologia
        const float dt = std::clamp(GetFrameTime(), 0.0f, 0.05f);
        if (up_held) {
            history_scroll_px_ -= 420.0f * dt;
            history_scroll_to_bottom_ = false;
        }
        if (down_held) {
            history_scroll_px_ += 420.0f * dt;
            history_scroll_to_bottom_ = false;
        }
        if (IsKeyPressed(KEY_PAGE_UP)) {
            history_scroll_px_ -= 280.0f;
            history_scroll_to_bottom_ = false;
        }
        if (IsKeyPressed(KEY_PAGE_DOWN)) {
            history_scroll_px_ += 280.0f;
            history_scroll_to_bottom_ = false;
        }
        if (IsKeyPressed(KEY_HOME)) {
            history_scroll_px_ = 0.0f;
            history_scroll_to_bottom_ = false;
        }
        if (IsKeyPressed(KEY_END)) {
            history_scroll_px_ = history_max_scroll_px_;
            history_scroll_to_bottom_ = false;
        }
        if (std::abs(wheel) > 0.01f) {
            history_scroll_px_ -= wheel * 64.0f;
            history_scroll_to_bottom_ = false;
        }
        history_scroll_px_ = std::clamp(history_scroll_px_, 0.0f, history_max_scroll_px_);
        last_seen_history_count_ = ctx.world.story.History().size();
    } else {
        // --- CODEX COLLEZIONABILI ---
        constexpr int kEraFilterCount = 5; // -1..3
        if (left_pressed) {
            int idx = (era_filter_ + 1 - 1 + kEraFilterCount) % kEraFilterCount;
            era_filter_ = idx - 1;
            codex_selected_row_ = 0;
            codex_list_scroll_px_ = 0.0f;
            codex_detail_scroll_px_ = 0.0f;
        }
        if (right_pressed) {
            int idx = (era_filter_ + 1 + 1) % kEraFilterCount;
            era_filter_ = idx - 1;
            codex_selected_row_ = 0;
            codex_list_scroll_px_ = 0.0f;
            codex_detail_scroll_px_ = 0.0f;
        }

        // Click sui filtri Era
        const float era_y = modal.y + 100.0f;
        const float pill_w = 198.0f;
        const float pill_gap = 10.0f;
        for (int e = -1; e <= 3; ++e) {
            const int idx = e + 1;
            const Rectangle pill{
                modal.x + 24.0f + static_cast<float>(idx) * (pill_w + pill_gap),
                era_y,
                pill_w,
                28.0f
            };
            if (clicked && CheckCollisionPointRec(mouse, pill)) {
                era_filter_ = e;
                codex_selected_row_ = 0;
                codex_list_scroll_px_ = 0.0f;
                codex_detail_scroll_px_ = 0.0f;
            }
        }

        const auto filtered = BuildFilteredCodexItems(ctx.world.collectibles, era_filter_);
        const int count = static_cast<int>(filtered.size());
        if (count > 0) {
            codex_selected_row_ = std::clamp(codex_selected_row_, 0, count - 1);
            if (up_pressed) {
                codex_selected_row_ = (codex_selected_row_ - 1 + count) % count;
                codex_detail_scroll_px_ = 0.0f;
            }
            if (down_pressed) {
                codex_selected_row_ = (codex_selected_row_ + 1) % count;
                codex_detail_scroll_px_ = 0.0f;
            }
        } else {
            codex_selected_row_ = 0;
        }

        const Rectangle list_area{modal.x + 24.0f, modal.y + 140.0f, 404.0f, 428.0f};
        const Rectangle detail_area{modal.x + 444.0f, modal.y + 140.0f, 632.0f, 428.0f};

        // Scorrimento con rotellina sulla colonna sotto il cursore
        if (std::abs(wheel) > 0.01f) {
            if (CheckCollisionPointRec(mouse, detail_area)) {
                codex_detail_scroll_px_ -= wheel * 48.0f;
            } else {
                codex_list_scroll_px_ -= wheel * 54.0f;
            }
        }
        if (IsKeyPressed(KEY_PAGE_UP)) {
            codex_detail_scroll_px_ -= 160.0f;
        }
        if (IsKeyPressed(KEY_PAGE_DOWN)) {
            codex_detail_scroll_px_ += 160.0f;
        }

        // Mantieni la riga selezionata visibile nella lista a sinistra quando ci si muove da tastiera
        constexpr float kRowH = 62.0f;
        constexpr float kRowGap = 8.0f;
        const float row_stride = kRowH + kRowGap;
        if (up_pressed || down_pressed) {
            const float sel_top = static_cast<float>(codex_selected_row_) * row_stride;
            const float sel_bot = sel_top + kRowH;
            if (sel_top < codex_list_scroll_px_) {
                codex_list_scroll_px_ = sel_top;
            } else if (sel_bot > codex_list_scroll_px_ + list_area.height) {
                codex_list_scroll_px_ = sel_bot - list_area.height;
            }
        }

        codex_list_scroll_px_ = std::clamp(codex_list_scroll_px_, 0.0f, codex_list_max_scroll_px_);
        codex_detail_scroll_px_ = std::clamp(codex_detail_scroll_px_, 0.0f, codex_detail_max_scroll_px_);

        // Click diretto su una reliquia nella lista
        if (clicked && CheckCollisionPointRec(mouse, list_area)) {
            const float rel_y = (mouse.y - list_area.y) + codex_list_scroll_px_;
            const int hit_idx = static_cast<int>(rel_y / row_stride);
            if (hit_idx >= 0 && hit_idx < count) {
                codex_selected_row_ = hit_idx;
                codex_detail_scroll_px_ = 0.0f;
            }
        }
    }

    return is_open_;
}

void JournalOverlay::DrawOverlay(const AppContext& ctx) const {
    if (!is_open_) {
        return;
    }

    // Oscuramento morbido dello sfondo
    DrawRectangle(0, 0, eco::kWindowWidth, eco::kWindowHeight, Color{6, 11, 18, 206});

    const Rectangle modal{90.0f, 52.0f, 1100.0f, 616.0f};
    const Color header_accent =
        (active_tab_ == JournalTab::History) ? pal::kJade : pal::kBronzeBright;
    DrawPanelWithHeader(
        modal,
        44,
        "DIARIO DI CAMPO DI ELENA VALLI  |  LA CONCA DI MERIDIA",
        header_accent);

    // Barra delle 2 Tab + contatore rapido + pulsante Chiudi
    const std::array<const char*, 2> tab_labels = {
        TextFormat("Cronologia Dialoghi [H] (%d)", static_cast<int>(ctx.world.story.History().size())),
        TextFormat("Codex Reliquie [J] (%d/%d)",
                   ctx.world.collectibles.CollectedCount(),
                   ctx.world.collectibles.TotalCount())
    };
    const Rectangle tab_area{modal.x + 24.0f, modal.y + 54.0f, 520.0f, 36.0f};
    DrawTabBar(tab_area, tab_labels.data(), 2, static_cast<int>(active_tab_), header_accent);

    // Mini barra progresso reliquie nell'intestazione destra
    const float relic_ratio =
        (ctx.world.collectibles.TotalCount() > 0)
            ? static_cast<float>(ctx.world.collectibles.CollectedCount()) /
                  static_cast<float>(ctx.world.collectibles.TotalCount())
            : 0.0f;
    DrawTextShadow(
        "Archivio Meridiani",
        static_cast<int>(modal.x + 568.0f),
        static_cast<int>(modal.y + 55.0f),
        12,
        pal::kTravertineLight);
    DrawProgressBar(
        Rectangle{modal.x + 568.0f, modal.y + 73.0f, 220.0f, 12.0f},
        relic_ratio,
        pal::kBronzeBright,
        pal::kUiBorderBronze);
    DrawTextShadow(
        TextFormat("%d%%", static_cast<int>(std::round(relic_ratio * 100.0f))),
        static_cast<int>(modal.x + 796.0f),
        static_cast<int>(modal.y + 70.0f),
        13,
        pal::kBronzeBright);

    // Pulsante Chiudi [ESC]
    const Rectangle close_btn{modal.x + modal.width - 134.0f, modal.y + 54.0f, 110.0f, 34.0f};
    DrawSectionCard(close_btn, pal::kUiBorderBronze, false);
    const char* close_lbl = "[ESC] Chiudi";
    const int cw = MeasureUiText(close_lbl, 13);
    DrawTextShadow(
        close_lbl,
        static_cast<int>(close_btn.x + (close_btn.width - static_cast<float>(cw)) * 0.5f),
        static_cast<int>(close_btn.y) + 10,
        13,
        pal::kParchmentText);

    if (active_tab_ == JournalTab::History) {
        // ---------------------------------------------------------------------
        // TAB 0: CRONOLOGIA DIALOGHI
        // ---------------------------------------------------------------------
        const float filter_y = modal.y + 100.0f;
        const float pill_w = 124.0f;
        const float pill_gap = 8.0f;
        for (int f = -1; f <= 6; ++f) {
            const int idx = f + 1;
            const Rectangle pill{
                modal.x + 24.0f + static_cast<float>(idx) * (pill_w + pill_gap),
                filter_y,
                pill_w,
                28.0f
            };
            const bool active = (chapter_filter_ == f);
            DrawSectionCard(pill, active ? pal::kJadeGlow : pal::kUiBorderBronze, active);
            const char* label = (f == -1) ? "Tutti" : ShortChapterLabel(static_cast<eco::Chapter>(f));
            const int lw = MeasureUiText(label, 13);
            DrawTextShadow(
                label,
                static_cast<int>(pill.x + (pill.width - static_cast<float>(lw)) * 0.5f),
                static_cast<int>(pill.y) + 7,
                13,
                active ? pal::kJadeGlow : pal::kParchmentText);
        }

        const Rectangle list_area{modal.x + 24.0f, modal.y + 140.0f, modal.width - 66.0f, 428.0f};
        const Rectangle scroll_track{list_area.x + list_area.width + 8.0f, list_area.y, 10.0f, list_area.height};

        DrawRectangleRec(list_area, Color{10, 17, 25, 215});
        DrawRectangleLinesEx(list_area, 1.0f, pal::WithAlpha(pal::kUiBorderBronze, 150));

        // Filtra le voci di cronologia
        const auto& all_history = ctx.world.story.History();
        std::vector<const eco::DialogueHistoryEntry*> filtered;
        filtered.reserve(all_history.size());
        for (const auto& entry : all_history) {
            if (chapter_filter_ < 0 || static_cast<int>(entry.chapter) == chapter_filter_) {
                filtered.push_back(&entry);
            }
        }

        struct RenderedHistoryCard {
            const eco::DialogueHistoryEntry* entry = nullptr;
            std::vector<std::string> lines{};
            float y = 0.0f;
            float height = 0.0f;
        };

        std::vector<RenderedHistoryCard> cards;
        cards.reserve(filtered.size());
        const int wrap_w = static_cast<int>(list_area.width) - 48;
        float cursor_y = 12.0f;
        for (const auto* entry : filtered) {
            RenderedHistoryCard card{};
            card.entry = entry;
            card.lines = WrapTextToWidth(entry->text, wrap_w, 15);
            if (card.lines.empty()) {
                card.lines.push_back("");
            }
            card.y = cursor_y;
            card.height = 38.0f + static_cast<float>(card.lines.size()) * 20.0f + 10.0f;
            cursor_y += card.height + 10.0f;
            cards.push_back(std::move(card));
        }

        const float total_content_h = std::max(cursor_y + 6.0f, list_area.height);
        history_max_scroll_px_ = std::max(0.0f, total_content_h - list_area.height);
        if (history_scroll_to_bottom_) {
            history_scroll_px_ = history_max_scroll_px_;
            history_scroll_to_bottom_ = false;
        }
        history_scroll_px_ = std::clamp(history_scroll_px_, 0.0f, history_max_scroll_px_);

        BeginScissorMode(
            static_cast<int>(list_area.x),
            static_cast<int>(list_area.y),
            static_cast<int>(list_area.width),
            static_cast<int>(list_area.height));

        if (cards.empty()) {
            const char* empty_msg =
                (chapter_filter_ < 0)
                    ? "Nessun dialogo registrato finora. Esplora la Conca e parla con i custodi."
                    : "Nessuna battuta registrata per questo capitolo.";
            const int ew = MeasureUiText(empty_msg, 16);
            DrawTextShadow(
                empty_msg,
                static_cast<int>(list_area.x + (list_area.width - static_cast<float>(ew)) * 0.5f),
                static_cast<int>(list_area.y + list_area.height * 0.45f),
                16,
                Color{168, 194, 206, 220});
        } else {
            for (const auto& card : cards) {
                const float draw_y = list_area.y + card.y - history_scroll_px_;
                if (draw_y + card.height < list_area.y - 10.0f ||
                    draw_y > list_area.y + list_area.height + 10.0f) {
                    continue;
                }

                bool is_echo = false;
                const Color sp_col = SpeakerAccentColor(card.entry->speaker, is_echo);
                const Rectangle card_rect{
                    list_area.x + 12.0f,
                    draw_y,
                    list_area.width - 24.0f,
                    card.height
                };
                DrawSectionCard(card_rect, sp_col, false);

                // Barra verticale laterale colorata per identificare subito chi parla
                DrawRectangle(
                    static_cast<int>(card_rect.x) + 2,
                    static_cast<int>(card_rect.y) + 4,
                    4,
                    static_cast<int>(card_rect.height) - 8,
                    sp_col);

                const std::string sp_name =
                    card.entry->speaker.empty() ? std::string("Elena Valli") : card.entry->speaker;
                DrawTextShadow(
                    sp_name.c_str(),
                    static_cast<int>(card_rect.x) + 16,
                    static_cast<int>(card_rect.y) + 9,
                    15,
                    sp_col);

                // Tag capitolo + luogo sulla destra della card
                const std::string loc =
                    !card.entry->location_name.empty()
                        ? card.entry->location_name
                        : std::string(eco::ZoneName(card.entry->zone));
                const char* meta_tag = TextFormat(
                    "#%02d  |  %s  |  %s",
                    card.entry->sequence_id,
                    ShortChapterLabel(card.entry->chapter),
                    loc.c_str());
                const int mw = MeasureUiText(meta_tag, 13);
                DrawTextShadow(
                    meta_tag,
                    static_cast<int>(card_rect.x + card_rect.width) - mw - 14,
                    static_cast<int>(card_rect.y) + 10,
                    13,
                    Color{164, 194, 208, 230});

                int line_y = static_cast<int>(card_rect.y) + 33;
                const Color body_col = is_echo ? Color{194, 246, 228, 255} : pal::kParchmentText;
                for (const auto& l : card.lines) {
                    DrawTextShadow(l.c_str(), static_cast<int>(card_rect.x) + 16, line_y, 15, body_col);
                    line_y += 20;
                }
            }
        }

        EndScissorMode();

        const float scroll_ratio =
            (history_max_scroll_px_ > 1e-3f) ? (history_scroll_px_ / history_max_scroll_px_) : 0.0f;
        const float visible_ratio = std::clamp(list_area.height / total_content_h, 0.08f, 1.0f);
        DrawScrollbar(scroll_track, scroll_ratio, visible_ratio, pal::kJade);
    } else {
        // ---------------------------------------------------------------------
        // TAB 1: CODEX & ARCHIVIO COLLEZIONABILI (16 Reliquie / 4 Ere)
        // ---------------------------------------------------------------------
        const float era_y = modal.y + 100.0f;
        const float pill_w = 198.0f;
        const float pill_gap = 10.0f;
        for (int e = -1; e <= 3; ++e) {
            const int idx = e + 1;
            const Rectangle pill{
                modal.x + 24.0f + static_cast<float>(idx) * (pill_w + pill_gap),
                era_y,
                pill_w,
                28.0f
            };
            const bool active = (era_filter_ == e);
            const Color accent = (e < 0) ? pal::kBronzeBright
                                         : EraAccentColor(static_cast<eco::CollectibleEra>(e));
            DrawSectionCard(pill, accent, active);

            const char* lbl = nullptr;
            if (e < 0) {
                lbl = TextFormat("Tutte (%d/%d)",
                                 ctx.world.collectibles.CollectedCount(),
                                 ctx.world.collectibles.TotalCount());
            } else {
                const auto era = static_cast<eco::CollectibleEra>(e);
                lbl = TextFormat("%s (%d/%d)",
                                 ShortEraLabel(e),
                                 ctx.world.collectibles.CollectedInEra(era),
                                 ctx.world.collectibles.TotalInEra(era));
            }
            const int lw = MeasureUiText(lbl, 12);
            DrawTextShadow(
                lbl,
                static_cast<int>(pill.x + (pill.width - static_cast<float>(lw)) * 0.5f),
                static_cast<int>(pill.y) + 8,
                12,
                active ? accent : pal::kParchmentText);
        }

        const Rectangle list_area{modal.x + 24.0f, modal.y + 140.0f, 392.0f, 428.0f};
        const Rectangle list_scroll_track{list_area.x + list_area.width + 6.0f, list_area.y, 8.0f, list_area.height};
        const Rectangle detail_area{modal.x + 448.0f, modal.y + 140.0f, 616.0f, 428.0f};
        const Rectangle detail_scroll_track{detail_area.x + detail_area.width + 6.0f, detail_area.y, 8.0f, detail_area.height};

        DrawRectangleRec(list_area, Color{10, 17, 25, 215});
        DrawRectangleLinesEx(list_area, 1.0f, pal::WithAlpha(pal::kUiBorderBronze, 150));

        const auto filtered = BuildFilteredCodexItems(ctx.world.collectibles, era_filter_);
        constexpr float kRowH = 62.0f;
        constexpr float kRowGap = 8.0f;
        const float total_list_h = std::max(
            list_area.height,
            10.0f + static_cast<float>(filtered.size()) * (kRowH + kRowGap));
        codex_list_max_scroll_px_ = std::max(0.0f, total_list_h - list_area.height);
        codex_list_scroll_px_ = std::clamp(codex_list_scroll_px_, 0.0f, codex_list_max_scroll_px_);

        BeginScissorMode(
            static_cast<int>(list_area.x),
            static_cast<int>(list_area.y),
            static_cast<int>(list_area.width),
            static_cast<int>(list_area.height));

        for (int i = 0; i < static_cast<int>(filtered.size()); ++i) {
            const eco::CollectibleItem* item = filtered[static_cast<std::size_t>(i)];
            const auto& meta = eco::CollectibleMetadata(item->id);
            const float ry = list_area.y + 8.0f + static_cast<float>(i) * (kRowH + kRowGap) - codex_list_scroll_px_;
            if (ry + kRowH < list_area.y - 6.0f || ry > list_area.y + list_area.height + 6.0f) {
                continue;
            }

            const bool selected = (i == codex_selected_row_);
            const Color era_col = EraAccentColor(item->era);
            const Rectangle row_rect{list_area.x + 8.0f, ry, list_area.width - 16.0f, kRowH};
            DrawSectionCard(
                row_rect,
                item->collected ? era_col : Color{86, 102, 114, 180},
                selected);

            // Glifo della reliquia
            DrawRelicGlyph(
                static_cast<int>(item->kind),
                Vector2{row_rect.x + 28.0f, row_rect.y + row_rect.height * 0.5f},
                14.0f,
                era_col,
                item->collected,
                ctx.world.sim_time);

            const std::string code_prefix = meta.code.empty() ? std::string("?") : meta.code;
            const std::string display_title =
                item->collected
                    ? (code_prefix + " - " + (meta.title.empty() ? std::string("Reliquia") : meta.title))
                    : (code_prefix + " - Reliquia inesplorata");

            DrawTextShadow(
                display_title.c_str(),
                static_cast<int>(row_rect.x) + 54,
                static_cast<int>(row_rect.y) + 12,
                14,
                item->collected ? pal::kParchmentText : Color{140, 160, 174, 210});

            const std::string zone_lbl =
                !meta.zone_name.empty()
                    ? meta.zone_name
                    : std::string(eco::LocationName(item->location, item->zone));
            const char* sub_lbl = item->collected
                                      ? TextFormat("%s  |  %s", eco::CollectibleKindName(item->kind), zone_lbl.c_str())
                                      : TextFormat("Indizio luogo: %s", zone_lbl.c_str());
            DrawTextShadow(
                sub_lbl,
                static_cast<int>(row_rect.x) + 54,
                static_cast<int>(row_rect.y) + 34,
                11,
                item->collected ? era_col : Color{122, 146, 162, 195});
        }

        EndScissorMode();

        const float list_ratio =
            (codex_list_max_scroll_px_ > 1e-3f) ? (codex_list_scroll_px_ / codex_list_max_scroll_px_) : 0.0f;
        DrawScrollbar(
            list_scroll_track,
            list_ratio,
            std::clamp(list_area.height / total_list_h, 0.12f, 1.0f),
            pal::kBronzeBright);

        // Pannello dettaglio Reliquia (colonna destra)
        DrawRectangleRec(detail_area, Color{12, 20, 30, 232});
        DrawRectangleLinesEx(detail_area, 1.2f, pal::WithAlpha(pal::kUiBorderBronze, 190));

        if (!filtered.empty()) {
            const int safe_sel = std::clamp(codex_selected_row_, 0, static_cast<int>(filtered.size()) - 1);
            const eco::CollectibleItem* sel = filtered[static_cast<std::size_t>(safe_sel)];
            const auto& meta = eco::CollectibleMetadata(sel->id);
            const Color era_col = EraAccentColor(sel->era);

            // Intestazione scheda (fissa in alto)
            const float head_h = 104.0f;
            DrawRectangle(
                static_cast<int>(detail_area.x),
                static_cast<int>(detail_area.y),
                static_cast<int>(detail_area.width),
                static_cast<int>(head_h),
                Color{18, 30, 44, 240});

            DrawRelicGlyph(
                static_cast<int>(sel->kind),
                Vector2{detail_area.x + 50.0f, detail_area.y + 52.0f},
                26.0f,
                era_col,
                sel->collected,
                ctx.world.sim_time);

            const std::string era_lbl =
                !meta.era_name.empty() ? meta.era_name : std::string(eco::EraName(sel->era));
            DrawTextShadow(
                era_lbl.c_str(),
                static_cast<int>(detail_area.x) + 94,
                static_cast<int>(detail_area.y) + 16,
                12,
                era_col);

            const std::string big_title =
                sel->collected
                    ? ((meta.code.empty() ? std::string() : (meta.code + "  ")) + meta.title)
                    : ((meta.code.empty() ? std::string() : (meta.code + "  ")) + "Frammento non ancora ritrovato");
            DrawTextShadow(
                big_title.c_str(),
                static_cast<int>(detail_area.x) + 94,
                static_cast<int>(detail_area.y) + 36,
                20,
                sel->collected ? pal::kParchmentText : Color{150, 172, 186, 220});

            const std::string zone_lbl =
                !meta.zone_name.empty()
                    ? meta.zone_name
                    : std::string(eco::LocationName(sel->location, sel->zone));
            const char* kind_zone = TextFormat(
                "Tipologia: %s   |   Luogo: %s",
                eco::CollectibleKindName(sel->kind),
                zone_lbl.c_str());
            DrawTextShadow(
                kind_zone,
                static_cast<int>(detail_area.x) + 94,
                static_cast<int>(detail_area.y) + 68,
                13,
                Color{174, 204, 218, 230});

            DrawDivider(
                static_cast<int>(detail_area.x) + 16,
                static_cast<int>(detail_area.y + head_h),
                static_cast<int>(detail_area.width) - 32,
                era_col);

            // Corpo scorribile della scheda
            const Rectangle body_rect{
                detail_area.x + 18.0f,
                detail_area.y + head_h + 12.0f,
                detail_area.width - 36.0f,
                detail_area.height - head_h - 22.0f
            };

            if (!sel->collected) {
                codex_detail_max_scroll_px_ = 0.0f;
                codex_detail_scroll_px_ = 0.0f;
                const auto locked_lines = WrapTextToWidth(
                    "Questa testimonianza dei Meridiani attende ancora di essere recuperata. "
                    "Esplora " + zone_lbl + " cercando il bagliore delle reliquie antiche e raccoglila "
                    "avvicinandoti o premendo [E] per trascriverla nel Codex.",
                    static_cast<int>(body_rect.width) - 16,
                    16);
                int ly = static_cast<int>(body_rect.y) + 26;
                for (const auto& l : locked_lines) {
                    DrawTextShadow(l.c_str(), static_cast<int>(body_rect.x) + 8, ly, 16, Color{162, 188, 202, 225});
                    ly += 24;
                }
            } else {
                // Costruisci tutte le righe wrappate (citazione + paragrafi)
                std::vector<std::pair<std::string, Color>> render_rows;
                if (!meta.pickup_line.empty()) {
                    const std::string quote_speaker =
                        meta.pickup_speaker.empty() ? std::string("Elena Valli") : meta.pickup_speaker;
                    bool dummy_echo = false;
                    const Color qcol = SpeakerAccentColor(quote_speaker, dummy_echo);
                    render_rows.emplace_back("Nota di ritrovamento (" + quote_speaker + "):", qcol);
                    const auto qlines = WrapTextToWidth(
                        "\"" + meta.pickup_line + "\"",
                        static_cast<int>(body_rect.width) - 18,
                        15);
                    for (const auto& ql : qlines) {
                        render_rows.emplace_back(ql, Color{226, 238, 226, 255});
                    }
                    render_rows.emplace_back("", BLANK);
                }

                for (const auto& para : meta.codex_paragraphs) {
                    const auto plines = WrapTextToWidth(para, static_cast<int>(body_rect.width) - 18, 15);
                    for (const auto& pl : plines) {
                        render_rows.emplace_back(pl, pal::kParchmentText);
                    }
                    render_rows.emplace_back("", BLANK);
                }

                const float line_h = 22.0f;
                const float total_body_h = std::max(
                    body_rect.height,
                    12.0f + static_cast<float>(render_rows.size()) * line_h);
                codex_detail_max_scroll_px_ = std::max(0.0f, total_body_h - body_rect.height);
                codex_detail_scroll_px_ = std::clamp(codex_detail_scroll_px_, 0.0f, codex_detail_max_scroll_px_);

                BeginScissorMode(
                    static_cast<int>(body_rect.x),
                    static_cast<int>(body_rect.y),
                    static_cast<int>(body_rect.width),
                    static_cast<int>(body_rect.height));

                float y = body_rect.y + 4.0f - codex_detail_scroll_px_;
                for (const auto& [txt, col] : render_rows) {
                    if (!txt.empty() && y + line_h >= body_rect.y - 6.0f && y <= body_rect.y + body_rect.height + 6.0f) {
                        DrawTextShadow(txt.c_str(), static_cast<int>(body_rect.x) + 6, static_cast<int>(y), 15, col);
                    }
                    y += txt.empty() ? 12.0f : line_h;
                }

                EndScissorMode();

                const float det_ratio =
                    (codex_detail_max_scroll_px_ > 1e-3f)
                        ? (codex_detail_scroll_px_ / codex_detail_max_scroll_px_)
                        : 0.0f;
                DrawScrollbar(
                    detail_scroll_track,
                    det_ratio,
                    std::clamp(body_rect.height / total_body_h, 0.14f, 1.0f),
                    era_col);
            }
        }
    }

    // Barra inferiore con i comandi del Diario
    DrawDivider(
        static_cast<int>(modal.x) + 24,
        static_cast<int>(modal.y + modal.height) - 38,
        static_cast<int>(modal.width) - 48,
        header_accent);
    const char* footer_hint =
        (active_tab_ == JournalTab::History)
            ? "[W/S] o Rotellina: Scorri   |   [A/D]: Filtra Capitolo   |   [TAB]: Passa al Codex   |   [ESC/H]: Chiudi"
            : "[W/S] o Click: Seleziona Reliquia   |   [A/D]: Filtra Era   |   Rotellina: Scorri   |   [TAB]: Cronologia   |   [ESC/J]: Chiudi";
    const int fw = MeasureUiText(footer_hint, 13);
    DrawTextShadow(
        footer_hint,
        static_cast<int>(modal.x + (modal.width - static_cast<float>(fw)) * 0.5f),
        static_cast<int>(modal.y + modal.height) - 27,
        13,
        Color{172, 206, 220, 235});
}

} // namespace eco::app
