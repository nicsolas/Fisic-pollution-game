#pragma once

#include <cstddef>

#include "app/game_state.h"
#include "core/simulation.h"

namespace eco::app {

enum class JournalTab : int {
    History = 0,
    Codex = 1
};

// Layer narrativo in-game: obiettivo in alto a sinistra, toast diegetico per zona/reliquia,
// dialogue box con word-wrap e schermata dei tre finali della Conca di Meridia.
void DrawStoryLayer(eco::WorldContext& world);

// Overlay interattivo del Diario di Campo:
// - [H] Cronologia dei Dialoghi (scorribile, filtrabile per capitolo, colori dei parlanti)
// - [J] Codex & Archivio delle 16 Reliquie (diviso per Era, stato scoperto/nascosto, schede complete)
class JournalOverlay {
public:
    void OpenHistory() noexcept;
    void OpenCodex() noexcept;
    void Close() noexcept;

    [[nodiscard]] bool IsOpen() const noexcept { return is_open_; }
    [[nodiscard]] JournalTab ActiveTab() const noexcept { return active_tab_; }

    // Gestisce apertura/chiusura ([H]/[J]/[ESC]), cambio tab ([TAB]/click), filtri e scorrimento.
    // Restituisce true se il Diario e' attualmente aperto (la simulazione sottostante va messa in pausa).
    bool UpdateAndHandleInput(AppContext& ctx, bool allow_open);

    // Disegna l'overlay del Diario in spazio virtuale 1280x720.
    void DrawOverlay(const AppContext& ctx) const;

private:
    bool is_open_ = false;
    JournalTab active_tab_ = JournalTab::History;

    // Stato Cronologia Dialoghi
    int chapter_filter_ = -1; // -1 = Tutti i capitoli, 0..6 = Chapter
    mutable float history_scroll_px_ = 0.0f;
    mutable float history_max_scroll_px_ = 0.0f;
    mutable bool history_scroll_to_bottom_ = true;
    std::size_t last_seen_history_count_ = 0;

    // Stato Codex Collezionabili
    int era_filter_ = -1; // -1 = Tutte le Ere, 0..3 = CollectibleEra
    int codex_selected_row_ = 0;
    mutable float codex_list_scroll_px_ = 0.0f;
    mutable float codex_list_max_scroll_px_ = 0.0f;
    mutable float codex_detail_scroll_px_ = 0.0f;
    mutable float codex_detail_max_scroll_px_ = 0.0f;
};

} // namespace eco::app
