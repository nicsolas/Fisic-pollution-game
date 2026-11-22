#include "app/render/ui_style.h"

#include <algorithm>
#include <array>

#include "app/render/palette.h"

namespace eco::app {

namespace {

Font g_custom_font{};
bool g_custom_font_loaded = false;
bool g_use_clean_ttf = true;

[[nodiscard]] bool ActiveCleanFont() noexcept {
    return g_custom_font_loaded && g_use_clean_ttf && g_custom_font.texture.id != 0;
}

[[nodiscard]] float EffectiveFontSize(int size) noexcept {
    // Il font TTF ha proporzioni leggermente piu' compatte in altezza rispetto al bitmap font di Raylib:
    // +1.5px lo rende nitidissimo e molto leggibile senza sbordare dai pannelli esistenti.
    return static_cast<float>(size) + 1.5f;
}

[[nodiscard]] float EffectiveSpacing(int size) noexcept {
    return (size >= 28) ? 1.5f : 0.8f;
}

} // namespace

void InitUiFont() {
    if (g_custom_font_loaded) {
        return;
    }

    constexpr std::array<const char*, 5> kCandidatePaths = {
        "assets/fonts/ui_font.ttf",
        "../assets/fonts/ui_font.ttf",
        "../../assets/fonts/ui_font.ttf",
        "/usr/share/fonts/google-noto/NotoSans-SemiBold.ttf",
        "/usr/share/fonts/google-noto/NotoSans-Medium.ttf"
    };

    for (const char* path : kCandidatePaths) {
        if (FileExists(path)) {
            g_custom_font = LoadFontEx(path, 48, nullptr, 250);
            if (g_custom_font.texture.id != 0 && g_custom_font.glyphCount > 0) {
                SetTextureFilter(g_custom_font.texture, TEXTURE_FILTER_BILINEAR);
                g_custom_font_loaded = true;
                g_use_clean_ttf = true;
                return;
            }
        }
    }
}

void ShutdownUiFont() {
    if (g_custom_font_loaded) {
        UnloadFont(g_custom_font);
        g_custom_font = Font{};
        g_custom_font_loaded = false;
    }
}

void SetUiFontCleanMode(bool use_clean_ttf) noexcept {
    g_use_clean_ttf = use_clean_ttf;
}

bool IsUiFontCleanMode() noexcept {
    return g_use_clean_ttf && g_custom_font_loaded;
}

int MeasureUiText(const char* text, int size) noexcept {
    if (text == nullptr || text[0] == '\0') {
        return 0;
    }
    if (ActiveCleanFont()) {
        const Vector2 dim = MeasureTextEx(g_custom_font, text, EffectiveFontSize(size), EffectiveSpacing(size));
        return static_cast<int>(dim.x + 0.5f);
    }
    return MeasureText(text, size);
}

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
    if (ActiveCleanFont()) {
        const float fsize = EffectiveFontSize(size);
        const float spacing = EffectiveSpacing(size);
        const Vector2 shadow_pos{static_cast<float>(x) + 1.2f, static_cast<float>(y) + 1.2f};
        const Vector2 main_pos{static_cast<float>(x), static_cast<float>(y)};
        DrawTextEx(g_custom_font, text, shadow_pos, fsize, spacing, Color{0, 0, 0, 200});
        DrawTextEx(g_custom_font, text, main_pos, fsize, spacing, color);
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
    const int text_w = MeasureUiText(key_label, 12);
    const int box_w = text_w + 10;
    DrawRectangle(x, y, box_w, 18, Color{20, 32, 46, 240});
    DrawRectangleLines(x, y, box_w, 18, accent);
    DrawTextShadow(key_label, x + 5, y + 2, 12, RAYWHITE);
}

} // namespace eco::app
