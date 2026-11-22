#pragma once

#include <raylib.h>

namespace eco::app {

// Gestione del font dell'interfaccia (Noto Sans SemiBold TTF ad alta leggibilita' con fallback su font bitmap).
void InitUiFont();
void ShutdownUiFont();
void SetUiFontCleanMode(bool use_clean_ttf) noexcept;
[[nodiscard]] bool IsUiFontCleanMode() noexcept;
[[nodiscard]] int MeasureUiText(const char* text, int size) noexcept;

// Pannelli e tipografia della Conca di Meridia (pietra scura, ottone/bronzo e luce geodetica).
void DrawPanel(Rectangle r, Color accent);
void DrawPanelWithHeader(Rectangle r, int header_height, const char* header_title, Color accent);
void DrawSectionCard(Rectangle r, Color accent, bool highlighted);
void DrawTextShadow(const char* text, int x, int y, int size, Color color);
void DrawProgressBar(Rectangle r, float ratio, Color fill_col, Color border_col);

// Badge tasti, divisori e controlli condivisi per HUD, Cronologia Dialoghi [H] e Codex [J].
[[nodiscard]] int MeasureKeyBadge(const char* key_label) noexcept;
void DrawKeyBadge(const char* key_label, int x, int y, Color accent);
void DrawDivider(int x, int y, int width, Color accent);
void DrawTabBar(Rectangle area, const char* const* labels, int count, int active_index, Color accent);
void DrawScrollbar(Rectangle track, float scroll_ratio, float visible_ratio, Color accent);

// Glifo vettoriale per le 5 tipologie di reliquie (0=Stele, 1=Astrolabio, 2=Prisma, 3=Cilindro, 4=Seme Fossile).
void DrawRelicGlyph(
    int kind_index,
    Vector2 center,
    float radius,
    Color accent,
    bool discovered,
    float sim_time);

// Banner/toast diegetico per ingresso distretto o raccolta reliquia (alpha in [0..1]).
void DrawBannerToast(
    Rectangle r,
    const char* eyebrow,
    const char* title,
    const char* subtitle,
    Color accent,
    float alpha);

} // namespace eco::app
