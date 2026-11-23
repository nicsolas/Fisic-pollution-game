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
    DrawCategoryTabs(area, labels, count, active_index, -1, accent);
}

void DrawCategoryTabs(
    Rectangle area,
    const char* const* labels,
    int count,
    int active_index,
    int hovered_index,
    Color accent) {
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
        const bool hovered = (i == hovered_index) && !active;
        const Color bg = active
                             ? pal::LerpColor(pal::kUiHeaderBg, accent, 0.24f)
                             : (hovered ? pal::LerpColor(pal::kUiHeaderBg, accent, 0.10f) : Color{14, 22, 34, 214});
        DrawRectangleRec(tr, bg);
        DrawRectangleLinesEx(
            tr,
            active ? 1.8f : (hovered ? 1.4f : 1.0f),
            active ? accent : (hovered ? pal::WithAlpha(accent, 175) : pal::WithAlpha(kBrassFrame, 115)));
        if (active) {
            DrawRectangle(
                static_cast<int>(tr.x) + 2,
                static_cast<int>(tr.y + tr.height) - 3,
                std::max(0, static_cast<int>(tr.width) - 4),
                3,
                accent);
            DrawRectangle(
                static_cast<int>(tr.x) + 2,
                static_cast<int>(tr.y) + 2,
                std::max(0, static_cast<int>(tr.width) - 4),
                1,
                pal::WithAlpha(kBrassBright, 130));
        }
        const char* label = (labels[i] != nullptr) ? labels[i] : "";
        const int tw = MeasureUiText(label, 14);
        const int tx = static_cast<int>(tr.x + (tr.width - static_cast<float>(tw)) * 0.5f);
        const int ty = static_cast<int>(tr.y + (tr.height - 15.0f) * 0.5f);
        const Color text_col = active
                                   ? RAYWHITE
                                   : (hovered ? Color{216, 230, 244, 240} : Color{165, 182, 198, 220});
        DrawTextShadow(label, tx, ty, 14, text_col);
    }
}

Rectangle SliderTrackRect(Rectangle row_rect) noexcept {
    const float right_pad = 14.0f;
    const float value_reserve = 64.0f;
    const float gap = 12.0f;
    const float max_track_w = std::max(80.0f, row_rect.width * 0.44f - value_reserve);
    const float track_w = std::clamp(row_rect.width * 0.34f, std::min(110.0f, max_track_w), std::min(220.0f, max_track_w));
    const float track_h = 12.0f;
    const float track_x = row_rect.x + row_rect.width - right_pad - value_reserve - gap - track_w;
    const float track_y = row_rect.y + (row_rect.height - track_h) * 0.5f;
    return Rectangle{track_x, track_y, track_w, track_h};
}

namespace {

void DrawSettingRowBackground(Rectangle r, Color accent, bool focused, bool hovered) {
    const Color fill = focused
                           ? pal::LerpColor(pal::kUiHeaderBg, accent, 0.20f)
                           : (hovered ? pal::LerpColor(Color{16, 26, 38, 222}, accent, 0.08f)
                                      : Color{14, 23, 35, 206});
    DrawRectangleRec(r, fill);
    if (focused) {
        DrawRectangle(
            static_cast<int>(r.x),
            static_cast<int>(r.y),
            4,
            static_cast<int>(r.height),
            accent);
    } else if (hovered) {
        DrawRectangle(
            static_cast<int>(r.x),
            static_cast<int>(r.y),
            2,
            static_cast<int>(r.height),
            pal::WithAlpha(accent, 165));
    }
    DrawRectangleLinesEx(
        r,
        focused ? 1.5f : 1.0f,
        focused ? pal::WithAlpha(accent, 225)
                : (hovered ? pal::WithAlpha(accent, 140) : pal::WithAlpha(kBrassFrame, 82)));
}

} // namespace

void DrawSettingSlider(Rectangle track, float value01, Color accent, bool focused, bool hovered) {
    const float v = std::clamp(value01, 0.0f, 1.0f);
    const float radius = track.height * 0.5f;

    // Binario scuro incassato
    DrawRectangleRounded(track, 0.85f, 8, Color{10, 16, 24, 242});
    DrawRectangleRoundedLinesEx(
        track,
        0.85f,
        8,
        1.1f,
        focused ? pal::WithAlpha(accent, 210) : pal::WithAlpha(kBrassFrame, 125));

    // Tacche discrete al 25%, 50%, 75%
    for (int step = 1; step <= 3; ++step) {
        const float tx = track.x + track.width * (static_cast<float>(step) * 0.25f);
        DrawLine(
            static_cast<int>(tx),
            static_cast<int>(track.y + 2.0f),
            static_cast<int>(tx),
            static_cast<int>(track.y + track.height - 2.0f),
            Color{130, 146, 164, 72});
    }

    // Riempimento graduato
    const float fill_w = std::max(0.0f, (track.width - 2.0f) * v);
    if (fill_w > 1.5f) {
        const Rectangle fill_rect{
            track.x + 1.0f,
            track.y + 1.0f,
            fill_w,
            std::max(2.0f, track.height - 2.0f)
        };
        const Color fill_col = focused ? pal::ShiftBrightness(accent, 14) : accent;
        DrawRectangleRounded(fill_rect, 0.85f, 8, fill_col);
        DrawRectangle(
            static_cast<int>(fill_rect.x + 2.0f),
            static_cast<int>(fill_rect.y + 1.0f),
            std::max(0, static_cast<int>(fill_rect.width - 4.0f)),
            2,
            pal::WithAlpha(RAYWHITE, 75));
    }

    // Cursore (thumb) in ottone / accento
    const float knob_x = track.x + radius + (track.width - radius * 2.0f) * v;
    const float knob_y = track.y + radius;
    const float knob_r = focused ? (radius + 3.2f) : (hovered ? (radius + 2.6f) : (radius + 1.8f));
    if (focused || hovered) {
        DrawCircleV({knob_x, knob_y}, knob_r + 3.2f, pal::WithAlpha(accent, focused ? 65 : 40));
    }
    DrawCircleV({knob_x, knob_y}, knob_r, Color{242, 236, 220, 255});
    DrawCircleLines(static_cast<int>(knob_x), static_cast<int>(knob_y), knob_r, focused ? accent : kBrassBright);
    DrawCircleV({knob_x, knob_y}, knob_r * 0.42f, accent);
}

void DrawSettingToggle(Rectangle pill, bool enabled, Color accent, bool focused, bool hovered) {
    const Color track_col = enabled
                                ? pal::LerpColor(Color{24, 58, 56, 245}, accent, 0.48f)
                                : Color{18, 26, 36, 240};
    DrawRectangleRounded(pill, 0.95f, 10, track_col);
    DrawRectangleRoundedLinesEx(
        pill,
        0.95f,
        10,
        focused ? 1.6f : 1.2f,
        enabled ? (focused ? pal::ShiftBrightness(accent, 22) : accent)
                : (focused ? pal::WithAlpha(accent, 190) : pal::WithAlpha(kBrassFrame, 125)));

    const float pad = 3.0f;
    const float knob_r = std::max(4.0f, pill.height * 0.5f - pad);
    const float knob_x = enabled
                             ? (pill.x + pill.width - pad - knob_r)
                             : (pill.x + pad + knob_r);
    const float knob_y = pill.y + pill.height * 0.5f;

    if (focused || hovered) {
        DrawCircleV({knob_x, knob_y}, knob_r + 2.5f, pal::WithAlpha(enabled ? accent : kBrassBright, 55));
    }
    DrawCircleV({knob_x, knob_y}, knob_r, enabled ? RAYWHITE : Color{176, 188, 200, 240});
    DrawCircleLines(static_cast<int>(knob_x), static_cast<int>(knob_y), knob_r, enabled ? accent : Color{96, 112, 128, 210});
}

void DrawSliderRow(
    Rectangle r,
    const char* label,
    float ratio,
    const char* value_text,
    Color accent,
    bool focused,
    bool hovered) {
    DrawSettingRowBackground(r, accent, focused, hovered);

    const int text_y = static_cast<int>(r.y + (r.height - 16.0f) * 0.5f);
    if (label != nullptr) {
        DrawTextShadow(
            label,
            static_cast<int>(r.x) + 14,
            text_y,
            15,
            focused ? RAYWHITE : Color{216, 226, 236, 240});
    }

    const Rectangle track = SliderTrackRect(r);
    DrawSettingSlider(track, ratio, accent, focused, hovered);

    if (value_text != nullptr && value_text[0] != '\0') {
        const int vw = MeasureUiText(value_text, 14);
        const int vx = static_cast<int>(r.x + r.width) - 14 - vw;
        const int vy = static_cast<int>(r.y + (r.height - 15.0f) * 0.5f);
        DrawTextShadow(
            value_text,
            vx,
            vy,
            14,
            focused ? pal::ShiftBrightness(accent, 28) : Color{196, 212, 228, 235});
    }
}

void DrawToggleRow(
    Rectangle r,
    const char* label,
    bool enabled,
    const char* state_text,
    Color accent,
    bool focused,
    bool hovered) {
    DrawSettingRowBackground(r, accent, focused, hovered);

    const int text_y = static_cast<int>(r.y + (r.height - 16.0f) * 0.5f);
    if (label != nullptr) {
        DrawTextShadow(
            label,
            static_cast<int>(r.x) + 14,
            text_y,
            15,
            focused ? RAYWHITE : Color{216, 226, 236, 240});
    }

    const float pill_w = 46.0f;
    const float pill_h = 22.0f;
    const Rectangle pill{
        r.x + r.width - 14.0f - pill_w,
        r.y + (r.height - pill_h) * 0.5f,
        pill_w,
        pill_h
    };
    DrawSettingToggle(pill, enabled, accent, focused, hovered);

    if (state_text != nullptr && state_text[0] != '\0') {
        const int sw = MeasureUiText(state_text, 14);
        const int sx = static_cast<int>(pill.x) - 12 - sw;
        const int sy = static_cast<int>(r.y + (r.height - 15.0f) * 0.5f);
        const Color st_col = enabled
                                 ? (focused ? pal::ShiftBrightness(accent, 28) : accent)
                                 : Color{154, 168, 184, 220};
        DrawTextShadow(state_text, sx, sy, 14, st_col);
    }
}

void DrawChoiceRow(
    Rectangle r,
    const char* label,
    const char* value_text,
    Color accent,
    bool focused,
    bool hovered) {
    DrawSettingRowBackground(r, accent, focused, hovered);

    const int text_y = static_cast<int>(r.y + (r.height - 16.0f) * 0.5f);
    if (label != nullptr) {
        DrawTextShadow(
            label,
            static_cast<int>(r.x) + 14,
            text_y,
            15,
            focused ? RAYWHITE : Color{216, 226, 236, 240});
    }

    const float box_w = std::clamp(r.width * 0.36f, 150.0f, 245.0f);
    const float box_h = std::max(24.0f, r.height - 10.0f);
    const Rectangle box{
        r.x + r.width - 14.0f - box_w,
        r.y + (r.height - box_h) * 0.5f,
        box_w,
        box_h
    };
    DrawRectangleRec(box, Color{11, 18, 28, 232});
    DrawRectangleLinesEx(
        box,
        focused ? 1.4f : 1.0f,
        focused ? pal::WithAlpha(accent, 220) : pal::WithAlpha(kBrassFrame, 110));

    // Frecce direzionali < >
    const Color arrow_col = focused ? accent : (hovered ? pal::WithAlpha(accent, 190) : Color{140, 158, 176, 200});
    const float cy = box.y + box.height * 0.5f;
    DrawTriangle(
        {box.x + 8.0f, cy},
        {box.x + 15.0f, cy + 4.5f},
        {box.x + 15.0f, cy - 4.5f},
        arrow_col);
    DrawTriangle(
        {box.x + box.width - 8.0f, cy},
        {box.x + box.width - 15.0f, cy - 4.5f},
        {box.x + box.width - 15.0f, cy + 4.5f},
        arrow_col);

    if (value_text != nullptr && value_text[0] != '\0') {
        const int vw = MeasureUiText(value_text, 14);
        const int vx = static_cast<int>(box.x + (box.width - static_cast<float>(vw)) * 0.5f);
        const int vy = static_cast<int>(box.y + (box.height - 15.0f) * 0.5f);
        DrawTextShadow(value_text, vx, vy, 14, focused ? RAYWHITE : Color{208, 222, 236, 238});
    }
}

void DrawActionButton(
    Rectangle r,
    const char* label,
    Color accent,
    bool focused,
    bool hovered,
    bool primary) {
    const Color top_col = primary
                              ? pal::LerpColor(pal::kUiHeaderBg, accent, focused ? 0.38f : (hovered ? 0.28f : 0.20f))
                              : (focused ? pal::LerpColor(pal::kUiHeaderBg, accent, 0.24f)
                                         : (hovered ? pal::LerpColor(pal::kUiHeaderBg, accent, 0.12f)
                                                    : Color{18, 28, 42, 232}));
    const Color bot_col = pal::ShiftBrightness(top_col, -10);

    DrawRectangle(
        static_cast<int>(r.x) + 2,
        static_cast<int>(r.y) + 3,
        static_cast<int>(r.width),
        static_cast<int>(r.height),
        Color{0, 0, 0, 100});
    DrawRectangleGradientV(
        static_cast<int>(r.x),
        static_cast<int>(r.y),
        static_cast<int>(r.width),
        static_cast<int>(r.height),
        top_col,
        bot_col);

    DrawRectangleLinesEx(
        r,
        (focused || primary) ? 1.7f : 1.1f,
        focused ? accent : (primary ? pal::WithAlpha(accent, 210) : (hovered ? pal::WithAlpha(accent, 175) : pal::WithAlpha(kBrassFrame, 120))));

    if (focused || primary) {
        DrawRectangle(
            static_cast<int>(r.x) + 2,
            static_cast<int>(r.y) + 2,
            std::max(0, static_cast<int>(r.width) - 4),
            2,
            pal::WithAlpha(accent, focused ? 220 : 145));
    }

    if (label != nullptr && label[0] != '\0') {
        const int lw = MeasureUiText(label, 15);
        const int lx = static_cast<int>(r.x + (r.width - static_cast<float>(lw)) * 0.5f);
        const int ly = static_cast<int>(r.y + (r.height - 16.0f) * 0.5f);
        DrawTextShadow(
            label,
            lx,
            ly,
            15,
            (focused || primary || hovered) ? RAYWHITE : Color{204, 218, 232, 235});
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

void DrawControllerButtonIcon(
    int controller_type,
    int button_code,
    Vector2 center,
    float radius) {
    const float r = std::max(8.0f, radius);
    const bool is_ps = (controller_type == 2);
    const bool is_switch = (controller_type == 3);

    // Pulsanti frontali (0=FaceDown, 1=FaceRight, 2=FaceLeft, 3=FaceUp)
    if (button_code >= 0 && button_code <= 3) {
        if (is_ps) {
            DrawCircleV(center, r, Color{24, 28, 38, 248});
            DrawCircleLinesV(center, r, Color{138, 152, 176, 220});
            const float s = r * 0.46f;
            if (button_code == 0) {
                // Croce blu (Cross)
                const Color col{92, 176, 255, 255};
                DrawLineEx({center.x - s, center.y - s}, {center.x + s, center.y + s}, 2.3f, col);
                DrawLineEx({center.x + s, center.y - s}, {center.x - s, center.y + s}, 2.3f, col);
            } else if (button_code == 1) {
                // Cerchio rosso (Circle)
                const Color col{246, 102, 102, 255};
                DrawRingLines(center, s * 0.92f, s * 1.18f, 0.0f, 360.0f, 20, col);
                DrawCircleLinesV(center, s * 1.05f, col);
            } else if (button_code == 2) {
                // Quadrato rosa (Square)
                const Color col{236, 122, 214, 255};
                DrawRectangleLinesEx(
                    Rectangle{center.x - s, center.y - s, s * 2.0f, s * 2.0f},
                    2.1f,
                    col);
            } else {
                // Triangolo verde acqua (Triangle)
                const Color col{72, 224, 178, 255};
                const Vector2 p1{center.x, center.y - s * 1.08f};
                const Vector2 p2{center.x - s * 1.04f, center.y + s * 0.86f};
                const Vector2 p3{center.x + s * 1.04f, center.y + s * 0.86f};
                DrawLineEx(p1, p2, 2.1f, col);
                DrawLineEx(p2, p3, 2.1f, col);
                DrawLineEx(p3, p1, 2.1f, col);
            }
            return;
        }

        if (is_switch) {
            const char* letter = (button_code == 0) ? "B" : ((button_code == 1) ? "A" : ((button_code == 2) ? "Y" : "X"));
            const Color fill = (button_code == 1) ? Color{178, 38, 48, 248} : Color{32, 36, 44, 248};
            const Color border = (button_code == 1) ? Color{252, 126, 134, 255} : Color{186, 196, 210, 235};
            DrawCircleV(center, r, fill);
            DrawCircleLinesV(center, r, border);
            const int tw = MeasureUiText(letter, 12);
            DrawTextShadow(letter, static_cast<int>(center.x) - tw / 2, static_cast<int>(center.y) - 6, 12, RAYWHITE);
            return;
        }

        // Xbox / Generic
        const char* letter = (button_code == 0) ? "A" : ((button_code == 1) ? "B" : ((button_code == 2) ? "X" : "Y"));
        const Color fill = (button_code == 0)
                               ? Color{34, 138, 64, 250}
                               : ((button_code == 1)
                                      ? Color{186, 44, 48, 250}
                                      : ((button_code == 2) ? Color{36, 108, 196, 250} : Color{196, 148, 28, 250}));
        DrawCircleV(center, r, fill);
        DrawCircleLinesV(center, r, Color{240, 244, 250, 220});
        const int tw = MeasureUiText(letter, 12);
        DrawTextShadow(letter, static_cast<int>(center.x) - tw / 2, static_cast<int>(center.y) - 6, 12, RAYWHITE);
        return;
    }

    // Dorsali / Grilletti / Stick / Start-Options-+ / View-Share--
    const char* pill_text = "?";
    if (is_ps) {
        switch (button_code) {
            case 4: pill_text = "L1"; break;
            case 5: pill_text = "R1"; break;
            case 6: pill_text = "L2"; break;
            case 7: pill_text = "R2"; break;
            case 8: pill_text = "L3"; break;
            case 9: pill_text = "R3"; break;
            case 10: pill_text = "OPT"; break;
            case 11: pill_text = "SHR"; break;
            default: break;
        }
    } else if (is_switch) {
        switch (button_code) {
            case 4: pill_text = "L"; break;
            case 5: pill_text = "R"; break;
            case 6: pill_text = "ZL"; break;
            case 7: pill_text = "ZR"; break;
            case 8: pill_text = "LS"; break;
            case 9: pill_text = "RS"; break;
            case 10: pill_text = "+"; break;
            case 11: pill_text = "-"; break;
            default: break;
        }
    } else {
        switch (button_code) {
            case 4: pill_text = "LB"; break;
            case 5: pill_text = "RB"; break;
            case 6: pill_text = "LT"; break;
            case 7: pill_text = "RT"; break;
            case 8: pill_text = "LS"; break;
            case 9: pill_text = "RS"; break;
            case 10: pill_text = "STR"; break;
            case 11: pill_text = "VIW"; break;
            default: break;
        }
    }

    const int tw = MeasureUiText(pill_text, 11);
    const float pw = std::max(r * 2.2f, static_cast<float>(tw + 14));
    const float ph = r * 1.65f;
    const Rectangle pill{center.x - pw * 0.5f, center.y - ph * 0.5f, pw, ph};
    DrawRectangleRounded(pill, 0.42f, 6, Color{24, 34, 48, 245});
    DrawRectangleRoundedLinesEx(pill, 0.42f, 6, 1.3f, pal::kBronzeBright);
    DrawTextShadow(
        pill_text,
        static_cast<int>(center.x) - tw / 2,
        static_cast<int>(center.y) - 6,
        11,
        pal::kJadeGlow);
}

} // namespace eco::app
