#pragma once

#include <raylib.h>

namespace eco::app {

void DrawPanel(Rectangle r, Color accent);
void DrawPanelWithHeader(Rectangle r, int header_height, const char* header_title, Color accent);
void DrawTextShadow(const char* text, int x, int y, int size, Color color);
void DrawProgressBar(Rectangle r, float ratio, Color fill_col, Color border_col);
void DrawKeyBadge(const char* key_label, int x, int y, Color accent);

} // namespace eco::app
