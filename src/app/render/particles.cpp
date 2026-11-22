#include "app/render/particles.h"

#include <cmath>
#include <cstdint>

#include "app/render/palette.h"

namespace eco::app {

namespace {

[[nodiscard]] constexpr std::uint32_t HashU32(std::uint32_t x) noexcept {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

[[nodiscard]] constexpr float HashFloat01(int index, int salt) noexcept {
    const std::uint32_t h = HashU32(static_cast<std::uint32_t>(index * 374761393 + salt * 668265263));
    return static_cast<float>(h & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
}

} // namespace

void DrawWorldAtmosphericParticles(
    eco::GameMode mode,
    float sim_time,
    const eco::EcoSystem& ecosystem,
    eco::Vec2 camera_target) {
    const float poll = ecosystem.PollutionRatio();
    const float health = 1.0f - poll;
    const float min_x = camera_target.x - static_cast<float>(eco::kWindowWidth) * 0.6f;
    const float min_y = camera_target.y - static_cast<float>(eco::kWindowHeight) * 0.6f;
    const float span_x = static_cast<float>(eco::kWindowWidth) * 1.2f;
    const float span_y = static_cast<float>(eco::kWindowHeight) * 1.2f;

    if (mode == eco::GameMode::Overworld) {
        // 1. Lucciole e polline bio-luminescente quando la Valle guarisce
        const int firefly_count = static_cast<int>(health * 42.0f);
        for (int i = 0; i < firefly_count; ++i) {
            const float rx = HashFloat01(i, 101);
            const float ry = HashFloat01(i, 202);
            const float speed = 10.0f + HashFloat01(i, 303) * 18.0f;
            const float phase = HashFloat01(i, 404) * 6.28318f;

            const float wx = min_x + std::fmod(rx * span_x + std::sin(sim_time * 0.9f + phase) * 26.0f + span_x, span_x);
            const float wy = min_y + std::fmod(ry * span_y - sim_time * speed + span_y * 10.0f, span_y);
            const float glow = 0.55f + 0.45f * std::sin(sim_time * 3.8f + phase);
            const Color c = (i % 3 == 0)
                                ? Color{255, 235, 130, static_cast<unsigned char>(180.0f * glow * health)}
                                : Color{95, 250, 215, static_cast<unsigned char>(195.0f * glow * health)};
            DrawCircleV({wx, wy}, 3.6f * glow, pal::WithAlpha(c, static_cast<unsigned char>(c.a / 3)));
            DrawCircleV({wx, wy}, 1.8f, c);
        }

        // 2. Petali / foglie portate dalla brezza quando l'aria e' pulita
        if (health > 0.45f) {
            const int leaf_count = static_cast<int>((health - 0.45f) * 28.0f);
            for (int i = 0; i < leaf_count; ++i) {
                const float rx = HashFloat01(i, 505);
                const float ry = HashFloat01(i, 606);
                const float wx = min_x + std::fmod(rx * span_x + sim_time * 28.0f, span_x);
                const float wy = min_y + std::fmod(ry * span_y + std::sin(sim_time * 2.1f + static_cast<float>(i)) * 18.0f + span_y, span_y);
                DrawPoly({wx, wy}, 4, 2.8f, sim_time * 85.0f + static_cast<float>(i * 25), Color{120, 230, 150, 170});
            }
        }
    } else {
        // StructureState (Diga del Crati): vapore e goccioline nebulizzate vicino al bacino
        constexpr int kMistCount = 34;
        for (int i = 0; i < kMistCount; ++i) {
            const float rx = HashFloat01(i, 707);
            const float ry = HashFloat01(i, 808);
            const float wx = 120.0f + rx * 1100.0f + std::sin(sim_time * 1.7f + static_cast<float>(i)) * 14.0f;
            const float wy = 180.0f + std::fmod(ry * 560.0f - sim_time * (14.0f + (i % 5) * 4.0f) + 5600.0f, 560.0f);
            const unsigned char alpha = static_cast<unsigned char>(55 + (i % 4) * 22);
            const Color drop_col = pal::WorldColor(
                Color{140, 145, 115, alpha},
                Color{135, 225, 255, alpha},
                poll);
            DrawCircleV({wx, wy}, 2.2f + static_cast<float>(i % 3) * 0.8f, drop_col);
        }
    }
}

void DrawScreenSpaceAtmosphere(
    eco::GameMode mode,
    float sim_time,
    const eco::EcoSystem& ecosystem) {
    const float poll = ecosystem.PollutionRatio();
    const float health = 1.0f - poll;
    const int sw = eco::kWindowWidth;
    const int sh = eco::kWindowHeight;

    // 1. Raggi solari obliqui (God-Rays) nell'Overworld quando lo smog e' quasi azzerato (< 25%)
    if (mode == eco::GameMode::Overworld && health > 0.75f) {
        const float ray_strength = (health - 0.75f) / 0.25f;
        for (int r = 0; r < 4; ++r) {
            const float shift = std::sin(sim_time * 0.45f + static_cast<float>(r) * 1.3f) * 35.0f;
            const float x0 = 140.0f + static_cast<float>(r) * 290.0f + shift;
            const float w = 75.0f + static_cast<float>(r % 2) * 35.0f;
            const auto alpha = static_cast<unsigned char>(22.0f * ray_strength);
            const Color sun_ray{255, 240, 175, alpha};
            DrawTriangle(
                Vector2{x0, 48.0f},
                Vector2{x0 + 220.0f, static_cast<float>(sh)},
                Vector2{x0 + 220.0f + w, static_cast<float>(sh)},
                sun_ray);
        }
    }

    // 2. Coltre di smog e pioggia di cenere obliqua quando l'inquinamento e' presente
    const float smog_ratio = ecosystem.SmogAlpha();
    if (smog_ratio > 0.01f) {
        const auto smog_a = static_cast<unsigned char>(smog_ratio * 195.0f);
        const Color smog_col = pal::LerpColor(Color{58, 64, 58, smog_a}, Color{76, 62, 48, smog_a}, poll);
        DrawRectangle(0, 0, sw, sh, smog_col);

        const int ash_count = static_cast<int>(poll * 54.0f);
        for (int i = 0; i < ash_count; ++i) {
            const float sx = HashFloat01(i, 909) * static_cast<float>(sw);
            const float sy = HashFloat01(i, 1010) * static_cast<float>(sh);
            const float vx = 22.0f + HashFloat01(i, 1111) * 24.0f;
            const float vy = 16.0f + HashFloat01(i, 1212) * 20.0f;
            const float px = std::fmod(sx + sim_time * vx, static_cast<float>(sw));
            const float py = std::fmod(sy + sim_time * vy + std::sin(sim_time * 1.8f + static_cast<float>(i)) * 12.0f,
                                       static_cast<float>(sh));
            const auto a = static_cast<unsigned char>(85 + (i % 4) * 28);
            DrawCircleV({px, py}, 1.8f + static_cast<float>(i % 2) * 0.9f, Color{168, 152, 134, a});
        }
    }

    // 3. Vignettatura cinematografica morbida ai bordi dello schermo
    DrawRectangleGradientV(0, 48, sw, 36, Color{0, 0, 0, 75}, Color{0, 0, 0, 0});
    DrawRectangleGradientV(0, sh - 44, sw, 44, Color{0, 0, 0, 0}, Color{0, 0, 0, 95});
}

} // namespace eco::app
