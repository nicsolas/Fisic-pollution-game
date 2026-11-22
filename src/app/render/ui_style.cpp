#include "app/render/ui_style.h"

#include <algorithm>

#include "app/render/palette.h"

namespace eco::app {

void DrawPanel(Rectangle r, Color accent) {
    // Ombra esterna morbida
    DrawRectangle(
        static_cast<int>(r.x) + 4,
        static_cast<int>(r.y) + 4,
        static_cast<int>(r.width),
        static_cast<int>(r.height),
        Color{0, 0, 0, 110});

    // Sfondo principale scuro
    DrawRectangleRec(r, pal::kUiPanelBg);

    // Fascia luminosa superiore sottile
    DrawRectangle(
        static_cast<int>(r.x) + 2,
        static_cast<int>(r.y) + 2,
        static_cast<int>(r.width) - 4,
        3,
        pal::WithAlpha(accent, 110));

    // Bordo principale e angoli tecnici
    DrawRectangleLinesEx(r, 1.8f, accent);
    const int rx = static_cast<int>(r.x);
    const int ry = static_cast<int>(r.y);
    const int rw = static_cast<int>(r.width);
    const int rh = static_cast<int>(r.height);
    const Color corner = pal::ShiftBrightness(accent, 35);
    DrawLine(rx, ry, rx + 10, ry, corner);
    DrawLine(rx, ry, rx, ry + 10, corner);
    DrawLine(rx + rw, ry + rh, rx + rw - 10, ry + rh, corner);
    DrawLine(rx + rw, ry + rh, rx + rw, ry + rh - 10, corner);
}

void DrawPanelWithHeader(Rectangle r, int header_height, const char* header_title, Color accent) {
    DrawPanel(r, accent);
    DrawRectangle(
        static_cast<int>(r.x) + 2,
        static_cast<int>(r.y) + 2,
        static_cast<int>(r.width) - 4,
        header_height,
        pal::kUiHeaderBg);
    DrawLine(
        static_cast<int>(r.x) + 2,
        static_cast<int>(r.y) + header_height + 2,
        static_cast<int>(r.x + r.width) - 2,
        static_cast<int>(r.y) + header_height + 2,
        pal::WithAlpha(accent, 180));
    if (header_title != nullptr) {
        DrawTextShadow(
            header_title,
            static_cast<int>(r.x) + 14,
            static_cast<int>(r.y) + (header_height - 14) / 2 + 2,
            15,
            accent);
    }
}

void DrawTextShadow(const char* text, int x, int y, int size, Color color) {
    if (text == nullptr) {
        return;
    }
    DrawText(text, x + 1, y + 1, size, Color{0, 0, 0, 185});
    DrawText(text, x, y, size, color);
}

void DrawProgressBar(Rectangle r, float ratio, Color fill_col, Color border_col) {
    const float clamped = std::clamp(ratio, 0.0f, 1.0f);
    DrawRectangleRec(r, Color{18, 26, 36, 235});
    const int fill_w = static_cast<int>((r.width - 4.0f) * clamped);
    if (fill_w > 0) {
        DrawRectangle(
            static_cast<int>(r.x) + 2,
            static_cast<int>(r.y) + 2,
            fill_w,
            static_cast<int>(r.height) - 4,
            fill_col);
        DrawRectangle(
            static_cast<int>(r.x) + 2,
            static_cast<int>(r.y) + 2,
            fill_w,
            std::max(2, static_cast<int>(r.height) / 4),
            pal::ShiftBrightness(fill_col, 35));
    }
    DrawRectangleLinesEx(r, 1.2f, border_col);
}

void DrawKeyBadge(const char* key_label, int x, int y, Color accent) {
    const int text_w = MeasureText(key_label, 12);
    const int box_w = text_w + 10;
    DrawRectangle(x, y, box_w, 18, Color{20, 32, 46, 240});
    DrawRectangleLines(x, y, box_w, 18, accent);
    DrawText(key_label, x + 5, y + 3, 12, RAYWHITE);
}

} // namespace eco::app
