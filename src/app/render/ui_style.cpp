#include "app/render/ui_style.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "app/render/palette.h"

namespace eco::app {

namespace {

Font g_custom_font{};
bool g_custom_font_loaded = false;
bool g_use_clean_ttf = true;

constexpr Color kBrassFrame{178, 146, 92, 165};
constexpr Color kBrassBright{232, 198, 134, 215};

[[nodiscard]] bool ActiveCleanFont() noexcept {
    return g_custom_font_loaded && g_use_clean_ttf && g_custom_font.texture.id != 0;
}

[[nodiscard]] float EffectiveFontSize(int size) noexcept {
    return static_cast<float>(size) + 1.5f;
}

[[nodiscard]] float EffectiveSpacing(int size) noexcept {
    return (size >= 28) ? 1.5f : 0.8f;
}

[[nodiscard]] Color ScaleAlpha(Color c, float factor) noexcept {
    const float clamped = std::clamp(factor, 0.0f, 1.0f);
    return Color{
        c.r,
        c.g,
        c.b,
        static_cast<unsigned char>(static_cast<float>(c.a) * clamped)
    };
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
    const int rx = static_cast<int>(r.x);
    const int ry = static_cast<int>(r.y);
    const int rw = static_cast<int>(r.width);
    const int rh = static_cast<int>(r.height);

    // Ombra esterna stratificata per staccare il pannello dallo sfondo
    DrawRectangle(rx + 5, ry + 6, rw, rh, Color{0, 0, 0, 95});
    DrawRectangle(rx + 2, ry + 3, rw, rh, Color{0, 0, 0, 75});

    // Sfondo in ardesia profonda con leggero gradiente verticale
    const Color top_bg = pal::ShiftBrightness(pal::kUiPanelBg, 7);
    const Color bot_bg = pal::ShiftBrightness(pal::kUiPanelBg, -4);
    DrawRectangleGradientV(rx, ry, rw, rh, top_bg, bot_bg);

    // Sottile filetto interno in ottone (estetica strumenti dei Meridiani)
    if (rw > 12 && rh > 12) {
        DrawRectangleLinesEx(
            Rectangle{r.x + 3.0f, r.y + 3.0f, r.width - 6.0f, r.height - 6.0f},
            1.0f,
            pal::WithAlpha(kBrassFrame, 72));
    }

    // Fascia luminosa superiore sottile
    DrawRectangle(rx + 2, ry + 2, std::max(0, rw - 4), 2, pal::WithAlpha(accent, 145));

    // Bordo principale e graffe geometriche agli angoli
    DrawRectangleLinesEx(r, 1.6f, pal::WithAlpha(accent, 215));
    const Color corner = pal::LerpColor(accent, kBrassBright, 0.35f);
    const int arm = std::min(12, std::min(rw, rh) / 4);
    if (arm >= 4) {
        // Alto-sinistra
        DrawLine(rx, ry, rx + arm, ry, corner);
        DrawLine(rx, ry, rx, ry + arm, corner);
        // Alto-destra
        DrawLine(rx + rw, ry, rx + rw - arm, ry, corner);
        DrawLine(rx + rw, ry, rx + rw, ry + arm, corner);
        // Basso-sinistra
        DrawLine(rx, ry + rh, rx + arm, ry + rh, corner);
        DrawLine(rx, ry + rh, rx, ry + rh - arm, corner);
        // Basso-destra
        DrawLine(rx + rw, ry + rh, rx + rw - arm, ry + rh, corner);
        DrawLine(rx + rw, ry + rh, rx + rw, ry + rh - arm, corner);
    }
}

void DrawPanelWithHeader(Rectangle r, int header_height, const char* header_title, Color accent) {
    DrawPanel(r, accent);
    const int rx = static_cast<int>(r.x);
    const int ry = static_cast<int>(r.y);
    const int rw = static_cast<int>(r.width);

    DrawRectangleGradientV(
        rx + 2,
        ry + 2,
        std::max(0, rw - 4),
        header_height,
        pal::ShiftBrightness(pal::kUiHeaderBg, 8),
        pal::kUiHeaderBg);
    DrawLine(
        rx + 2,
        ry + header_height + 2,
        rx + rw - 2,
        ry + header_height + 2,
        pal::WithAlpha(accent, 190));

    // Piccola losanga ornamentale a sinistra del titolo
    const float diamond_x = r.x + 16.0f;
    const float diamond_y = r.y + static_cast<float>(header_height) * 0.5f + 2.0f;
    DrawPoly({diamond_x, diamond_y}, 4, 4.8f, 0.0f, pal::LerpColor(accent, kBrassBright, 0.4f));

    if (header_title != nullptr) {
        DrawTextShadow(
            header_title,
            rx + 28,
            ry + (header_height - 14) / 2 + 2,
            15,
            pal::ShiftBrightness(accent, 22));
    }
}

void DrawSectionCard(Rectangle r, Color accent, bool highlighted) {
    const Color fill = highlighted
                           ? pal::LerpColor(pal::kUiHeaderBg, accent, 0.16f)
                           : Color{14, 24, 36, 210};
    DrawRectangleRec(r, fill);
    DrawRectangle(
        static_cast<int>(r.x),
        static_cast<int>(r.y),
        highlighted ? 4 : 2,
        static_cast<int>(r.height),
        highlighted ? accent : pal::WithAlpha(accent, 150));
    DrawRectangleLinesEx(
        r,
        highlighted ? 1.5f : 1.0f,
        highlighted ? pal::WithAlpha(accent, 220) : pal::WithAlpha(kBrassFrame, 95));
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
        DrawTextEx(
            g_custom_font,
            text,
            shadow_pos,
            fsize,
            spacing,
            Color{0, 0, 0, static_cast<unsigned char>(std::min(210, static_cast<int>(color.a)))});
        DrawTextEx(g_custom_font, text, main_pos, fsize, spacing, color);
        return;
    }
    DrawText(
        text,
        x + 1,
        y + 1,
        size,
        Color{0, 0, 0, static_cast<unsigned char>(std::min(190, static_cast<int>(color.a)))});
    DrawText(text, x, y, size, color);
}

void DrawProgressBar(Rectangle r, float ratio, Color fill_col, Color border_col) {
    const float clamped = std::clamp(ratio, 0.0f, 1.0f);
    DrawRectangleRec(r, Color{14, 22, 32, 238});
    const int fill_w = static_cast<int>((r.width - 4.0f) * clamped);
    if (fill_w > 0) {
        DrawRectangleGradientH(
            static_cast<int>(r.x) + 2,
            static_cast<int>(r.y) + 2,
            fill_w,
            static_cast<int>(r.height) - 4,
            pal::ShiftBrightness(fill_col, -18),
            fill_col);
        DrawRectangle(
            static_cast<int>(r.x) + 2,
            static_cast<int>(r.y) + 2,
            fill_w,
            std::max(2, static_cast<int>(r.height) / 4),
            pal::ShiftBrightness(fill_col, 38));
    }
    DrawRectangleLinesEx(r, 1.2f, border_col);
}

int MeasureKeyBadge(const char* key_label) noexcept {
    if (key_label == nullptr || key_label[0] == '\0') {
        return 18;
    }
    return MeasureUiText(key_label, 12) + 12;
}

void DrawKeyBadge(const char* key_label, int x, int y, Color accent) {
    if (key_label == nullptr) {
        return;
    }
    const int box_w = MeasureKeyBadge(key_label);
    DrawRectangle(x + 1, y + 2, box_w, 18, Color{0, 0, 0, 120});
    DrawRectangleGradientV(x, y, box_w, 18, Color{26, 40, 56, 245}, Color{16, 26, 38, 245});
    DrawRectangleLines(x, y, box_w, 18, pal::WithAlpha(accent, 220));
    DrawLine(x + 1, y + 1, x + box_w - 2, y + 1, pal::WithAlpha(kBrassBright, 110));
    DrawTextShadow(key_label, x + 6, y + 2, 12, RAYWHITE);
}

void DrawDivider(int x, int y, int width, Color accent) {
    if (width <= 8) {
        return;
    }
    const int mid = x + width / 2;
    DrawLine(x, y, mid - 8, y, pal::WithAlpha(accent, 110));
    DrawLine(mid + 8, y, x + width, y, pal::WithAlpha(accent, 110));
    DrawPoly({static_cast<float>(mid), static_cast<float>(y)}, 4, 3.6f, 0.0f, pal::LerpColor(accent, kBrassBright, 0.5f));
}

void DrawTabBar(Rectangle area, const char* const* labels, int count, int active_index, Color accent) {
    if (labels == nullptr || count <= 0) {
        return;
    }
    const float gap = 8.0f;
    const float tab_w = (area.width - gap * static_cast<float>(count - 1)) / static_cast<float>(count);
    for (int i = 0; i < count; ++i) {
        const Rectangle tr{
            area.x + static_cast<float>(i) * (tab_w + gap),
            area.y,
            tab_w,
            area.height
        };
        const bool active = (i == active_index);
        const Color bg = active ? pal::LerpColor(pal::kUiHeaderBg, accent, 0.22f) : Color{14, 22, 34, 210};
        DrawRectangleRec(tr, bg);
        DrawRectangleLinesEx(
            tr,
            active ? 1.8f : 1.0f,
            active ? accent : pal::WithAlpha(kBrassFrame, 115));
        if (active) {
            DrawRectangle(
                static_cast<int>(tr.x) + 2,
                static_cast<int>(tr.y + tr.height) - 3,
                std::max(0, static_cast<int>(tr.width) - 4),
                3,
                accent);
        }
        const char* label = (labels[i] != nullptr) ? labels[i] : "";
        const int tw = MeasureUiText(label, 14);
        const int tx = static_cast<int>(tr.x + (tr.width - static_cast<float>(tw)) * 0.5f);
        const int ty = static_cast<int>(tr.y + (tr.height - 15.0f) * 0.5f);
        DrawTextShadow(label, tx, ty, 14, active ? RAYWHITE : Color{165, 182, 198, 220});
    }
}

void DrawScrollbar(Rectangle track, float scroll_ratio, float visible_ratio, Color accent) {
    DrawRectangleRec(track, Color{12, 20, 30, 210});
    DrawRectangleLinesEx(track, 1.0f, pal::WithAlpha(kBrassFrame, 110));

    const float vis = std::clamp(visible_ratio, 0.12f, 1.0f);
    const float pos = std::clamp(scroll_ratio, 0.0f, 1.0f);
    const float inner_h = std::max(8.0f, track.height - 4.0f);
    const float thumb_h = std::max(18.0f, inner_h * vis);
    const float thumb_y = track.y + 2.0f + (inner_h - thumb_h) * pos;
    const Rectangle thumb{track.x + 2.0f, thumb_y, std::max(4.0f, track.width - 4.0f), thumb_h};

    DrawRectangleRec(thumb, pal::WithAlpha(accent, 200));
    DrawRectangleLinesEx(thumb, 1.0f, pal::ShiftBrightness(accent, 30));
}

void DrawRelicGlyph(
    int kind_index,
    Vector2 center,
    float radius,
    Color accent,
    bool discovered,
    float sim_time) {
    const Color primary = discovered ? accent : Color{78, 92, 106, 175};
    const Color secondary = discovered ? pal::LerpColor(accent, kBrassBright, 0.45f) : Color{56, 66, 78, 165};
    const Color core = discovered ? RAYWHITE : Color{95, 108, 120, 175};
    const float pulse = discovered ? (0.5f + 0.5f * std::sin(sim_time * 3.4f)) : 0.0f;

    if (discovered) {
        DrawCircleV(center, radius * (1.28f + 0.14f * pulse), pal::WithAlpha(primary, static_cast<unsigned char>(34 + 26 * pulse)));
    }

    switch (kind_index % 5) {
        case 0: {
            // Stele incisa: obelisco/stele verticale in pietra con glifo solare luminoso
            const float w = radius * 0.92f;
            const float h = radius * 1.48f;
            const Rectangle body{center.x - w * 0.5f, center.y - h * 0.42f, w, h * 0.86f};
            DrawRectangleRec(body, Color{28, 36, 46, 235});
            DrawTriangle(
                {center.x, center.y - h * 0.62f},
                {center.x - w * 0.5f, center.y - h * 0.42f},
                {center.x + w * 0.5f, center.y - h * 0.42f},
                secondary);
            DrawRectangleLinesEx(body, 1.5f, primary);
            DrawLineEx(
                {center.x, center.y - h * 0.25f},
                {center.x, center.y + h * 0.28f},
                1.6f,
                core);
            DrawCircleV({center.x, center.y - h * 0.1f}, radius * 0.22f, primary);
            break;
        }
        case 1: {
            // Astrolabio: doppi anelli armillari rotanti in ottone e centro celeste
            const float rot = discovered ? sim_time * 42.0f : 15.0f;
            DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), radius * 0.88f, secondary);
            DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), radius * 0.56f, primary);
            const float a = rot * 0.0174533f;
            DrawLineEx(
                {center.x - std::cos(a) * radius * 0.86f, center.y - std::sin(a) * radius * 0.86f},
                {center.x + std::cos(a) * radius * 0.86f, center.y + std::sin(a) * radius * 0.86f},
                1.6f,
                primary);
            DrawLineEx(
                {center.x - std::sin(a) * radius * 0.62f, center.y + std::cos(a) * radius * 0.62f},
                {center.x + std::sin(a) * radius * 0.62f, center.y - std::cos(a) * radius * 0.62f},
                1.4f,
                secondary);
            DrawCircleV(center, radius * 0.22f, core);
            break;
        }
        case 2: {
            // Prisma ottico: cristallo sfaccettato con raggio rifratto
            DrawPoly(center, 6, radius * 0.86f, 30.0f, pal::WithAlpha(primary, discovered ? 110 : 55));
            DrawPolyLinesEx(center, 6, radius * 0.86f, 30.0f, 1.8f, primary);
            DrawTriangleLines(
                {center.x, center.y - radius * 0.66f},
                {center.x - radius * 0.58f, center.y + radius * 0.44f},
                {center.x + radius * 0.58f, center.y + radius * 0.44f},
                core);
            DrawCircleV(center, radius * 0.18f, core);
            break;
        }
        case 3: {
            // Cilindro inciso / rullo d'archivio in bronzo con bande armoniche
            const float w = radius * 1.15f;
            const float h = radius * 1.24f;
            const Rectangle body{center.x - w * 0.5f, center.y - h * 0.5f, w, h};
            DrawRectangleRec(body, Color{32, 38, 48, 235});
            DrawRectangleLinesEx(body, 1.6f, secondary);
            DrawEllipse(static_cast<int>(center.x), static_cast<int>(center.y - h * 0.5f), w * 0.5f, radius * 0.22f, secondary);
            DrawEllipseLines(static_cast<int>(center.x), static_cast<int>(center.y + h * 0.5f), w * 0.5f, radius * 0.22f, secondary);
            for (int row = -1; row <= 1; ++row) {
                const float ly = center.y + static_cast<float>(row) * (radius * 0.30f);
                DrawLineEx({center.x - w * 0.32f, ly}, {center.x + w * 0.32f, ly}, 1.5f, (row == 0) ? core : primary);
            }
            break;
        }
        default: {
            // Seme fossile / capsula botanica dei Meridiani con germoglio luminoso
            DrawCircleV(center, radius * 0.72f, Color{24, 42, 38, 235});
            DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), radius * 0.76f, secondary);
            DrawPoly({center.x, center.y - radius * 0.1f}, 4, radius * 0.52f, 45.0f, pal::WithAlpha(primary, discovered ? 190 : 85));
            DrawLineEx(
                {center.x, center.y + radius * 0.48f},
                {center.x, center.y - radius * 0.48f},
                1.8f,
                core);
            DrawLineEx(
                {center.x, center.y + radius * 0.05f},
                {center.x + radius * 0.38f, center.y - radius * 0.26f},
                1.5f,
                primary);
            DrawLineEx(
                {center.x, center.y + radius * 0.14f},
                {center.x - radius * 0.36f, center.y - radius * 0.18f},
                1.5f,
                primary);
            break;
        }
    }
}

void DrawBannerToast(
    Rectangle r,
    const char* eyebrow,
    const char* title,
    const char* subtitle,
    Color accent,
    float alpha) {
    const float a = std::clamp(alpha, 0.0f, 1.0f);
    if (a <= 0.02f) {
        return;
    }
    const int rx = static_cast<int>(r.x);
    const int ry = static_cast<int>(r.y);
    const int rw = static_cast<int>(r.width);
    const int rh = static_cast<int>(r.height);

    const float bg_a = std::clamp(0.45f + a * 0.55f, 0.0f, 1.0f) * a;
    DrawRectangle(rx + 3, ry + 4, rw, rh, ScaleAlpha(Color{0, 0, 0, 145}, a));
    DrawRectangleGradientV(
        rx,
        ry,
        rw,
        rh,
        ScaleAlpha(Color{16, 26, 38, 246}, bg_a),
        ScaleAlpha(Color{10, 17, 26, 244}, bg_a));
    DrawRectangle(rx + 2, ry + 2, std::max(0, rw - 4), 2, ScaleAlpha(accent, a));
    DrawRectangleLinesEx(r, 1.5f, ScaleAlpha(accent, a));

    const float mid_x = r.x + r.width * 0.5f;
    if (eyebrow != nullptr && eyebrow[0] != '\0') {
        const int ew = MeasureUiText(eyebrow, 11);
        DrawTextShadow(eyebrow, static_cast<int>(mid_x - static_cast<float>(ew) * 0.5f), ry + 5, 11, ScaleAlpha(accent, a));
    }
    if (title != nullptr && title[0] != '\0') {
        const int tw = MeasureUiText(title, 16);
        DrawTextShadow(
            title,
            static_cast<int>(mid_x - static_cast<float>(tw) * 0.5f),
            ry + (eyebrow != nullptr && eyebrow[0] != '\0' ? 18 : 10),
            16,
            ScaleAlpha(RAYWHITE, a));
    }
    if (subtitle != nullptr && subtitle[0] != '\0') {
        const int sw = MeasureUiText(subtitle, 12);
        DrawTextShadow(
            subtitle,
            static_cast<int>(mid_x - static_cast<float>(sw) * 0.5f),
            ry + rh - 17,
            12,
            ScaleAlpha(Color{198, 216, 232, 240}, a));
    }
}

} // namespace eco::app
