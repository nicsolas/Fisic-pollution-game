#pragma once

#include <raylib.h>

namespace eco::app {

// Gestione del font dell'interfaccia (Noto Sans SemiBold TTF ad alta leggibilita' con fallback su font pixel).
void InitUiFont();
void ShutdownUiFont();
void SetUiFontCleanMode(bool use_clean_ttf) noexcept;
[[nodiscard]] bool IsUiFontCleanMode() noexcept;
[[nodiscard]] int MeasureUiText(const char* text, int size) noexcept;

void DrawPanel(Rectangle r, Color accent);
void DrawPanelWithHeader(Rectangle r, int header_height, const char* header_title, Color accent);
void DrawTextShadow(const char* text, int x, int y, int size, Color color);
void DrawProgressBar(Rectangle r, float ratio, Color fill_col, Color border_col);
void DrawKeyBadge(const char* key_label, int x, int y, Color accent);

} // namespace eco::app
