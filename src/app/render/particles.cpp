#include "app/render/particles.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "app/render/palette.h"
#include "core/collectible.h"
#include "core/ecosystem.h"
#include "core/tilemap.h"

namespace eco::app {

namespace {

float g_particle_quality_scale = 1.0f;

// Stato statico (zero allocazioni) per l'interpolazione morbida della tinta atmosferica di distretto
struct DistrictTintState {
    float r = 255.0f;
    float g = 204.0f;
    float b = 112.0f;
    float last_sim_time = -1.0f;
    bool initialized = false;
};

DistrictTintState g_district_tint{};

[[nodiscard]] Color UpdateSmoothedDistrictTint(const eco::WorldContext& world) noexcept {
    const Color target = (world.current_mode == eco::GameMode::Overworld)
                             ? pal::GetDistrictPalette(world.current_zone).atmosphere_tint
                             : pal::GetStructurePalette(world.active_structure).accent_healthy;
    const float tr = static_cast<float>(target.r);
    const float tg = static_cast<float>(target.g);
    const float tb = static_cast<float>(target.b);

    if (!g_district_tint.initialized || world.sim_time < g_district_tint.last_sim_time) {
        g_district_tint.r = tr;
        g_district_tint.g = tg;
        g_district_tint.b = tb;
        g_district_tint.last_sim_time = world.sim_time;
        g_district_tint.initialized = true;
    } else {
        const float dt = std::clamp(world.sim_time - g_district_tint.last_sim_time, 0.0f, 0.1f);
        g_district_tint.last_sim_time = world.sim_time;
        const float blend = 1.0f - std::exp(-dt * 3.8f);
        g_district_tint.r += (tr - g_district_tint.r) * blend;
        g_district_tint.g += (tg - g_district_tint.g) * blend;
        g_district_tint.b += (tb - g_district_tint.b) * blend;
    }

    return Color{
        static_cast<unsigned char>(std::clamp(g_district_tint.r, 0.0f, 255.0f)),
        static_cast<unsigned char>(std::clamp(g_district_tint.g, 0.0f, 255.0f)),
        static_cast<unsigned char>(std::clamp(g_district_tint.b, 0.0f, 255.0f)),
        255
    };
}

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

[[nodiscard]] Color EraVfxColor(eco::CollectibleEra era) noexcept {
    switch (era) {
        case eco::CollectibleEra::Era1_Dawn:
            return Color{248, 208, 104, 255}; // Oro solare
        case eco::CollectibleEra::Era2_WatersAndSound:
            return Color{96, 228, 244, 255};  // Acquamarina
        case eco::CollectibleEra::Era3_PitchWinter:
            return Color{244, 152, 82, 255};  // Ambra / rame
        case eco::CollectibleEra::Era4_HarmonySeal:
            return Color{108, 248, 206, 255}; // Giada radiosa
        case eco::CollectibleEra::Count:
            break;
    }
    return pal::kJadeGlow;
}

void DrawCollectiblePickupBurst(const eco::WorldContext& world) {
    if (!world.collectibles.HasRecentPickup()) {
        return;
    }
    if (world.collectibles.LastCollectedLocation() != world.ActiveLocation()) {
        return;
    }
    const float elapsed = eco::kRelicBannerDuration - world.collectibles.PickupBannerTimer();
    if (elapsed < 0.0f || elapsed > 1.35f) {
        return;
    }

    const float t = std::clamp(elapsed / 1.35f, 0.0f, 1.0f);
    const float fade = (1.0f - t) * (1.0f - t);
    const eco::Vec2 origin = world.collectibles.LastCollectedPos();
    const eco::CollectibleItem* item = world.collectibles.Find(world.collectibles.LastCollectedId());
    const Color base = (item != nullptr) ? EraVfxColor(item->era) : pal::kBronzeBright;

    // Due onde concentriche di luce che si espandono dal punto di raccolta
    const float r1 = 14.0f + t * 88.0f;
    const float r2 = 8.0f + t * 58.0f;
    DrawCircleLines(
        static_cast<int>(origin.x),
        static_cast<int>(origin.y),
        r1,
        pal::WithAlpha(base, static_cast<unsigned char>(220.0f * fade)));
    DrawCircleLines(
        static_cast<int>(origin.x),
        static_cast<int>(origin.y),
        r2,
        pal::WithAlpha(RAYWHITE, static_cast<unsigned char>(195.0f * fade)));

    // 14 scintille radiali ascendenti
    for (int i = 0; i < 14; ++i) {
        const float angle = static_cast<float>(i) * (2.0f * eco::kPi / 14.0f) + HashFloat01(i, 71) * 0.25f;
        const float dist = (16.0f + HashFloat01(i, 83) * 64.0f) * t;
        const float lift = t * t * 28.0f;
        const Vector2 p{
            origin.x + std::cos(angle) * dist,
            origin.y + std::sin(angle) * dist - lift
        };
        const float rad = (2.8f + HashFloat01(i, 97) * 2.0f) * (1.0f - t * 0.65f);
        DrawCircleV(p, rad * 1.8f, pal::WithAlpha(base, static_cast<unsigned char>(90.0f * fade)));
        DrawCircleV(p, rad, pal::WithAlpha(RAYWHITE, static_cast<unsigned char>(235.0f * fade)));
    }
}

} // namespace

void SetParticleQualityScale(float scale) noexcept {
    if (scale < 0.0f) {
        g_particle_quality_scale = 0.0f;
    } else if (scale > 1.0f) {
        g_particle_quality_scale = 1.0f;
    } else {
        g_particle_quality_scale = scale;
    }
}

float GetParticleQualityScale() noexcept {
    return g_particle_quality_scale;
}

void DrawWorldAtmosphericParticles(
    const eco::WorldContext& world,
    eco::Vec2 camera_target) {
    // Il burst della raccolta reliquia e' sempre leggibile anche con particelle ridotte
    DrawCollectiblePickupBurst(world);

    if (g_particle_quality_scale <= 0.01f) {
        return;
    }

    const float sim_time = world.sim_time;
    const float poll = world.ecosystem.PollutionRatio();
    const float health = 1.0f - poll;
    const float min_x = camera_target.x - static_cast<float>(eco::kWindowWidth) * 0.58f;
    const float min_y = camera_target.y - static_cast<float>(eco::kWindowHeight) * 0.58f;
    const float span_x = static_cast<float>(eco::kWindowWidth) * 1.16f;
    const float span_y = static_cast<float>(eco::kWindowHeight) * 1.16f;

    if (world.current_mode == eco::GameMode::Overworld) {
        const eco::ZoneId zone = world.overworld.DistrictAt(camera_target);

        // 1. Lucciole e pulviscolo luminoso della Conca quando l'aria guarisce
        const int firefly_base = (zone == eco::ZoneId::HangingGardens || zone == eco::ZoneId::HarmonySanctuary) ? 52 : 34;
        const int firefly_count = static_cast<int>(health * static_cast<float>(firefly_base) * g_particle_quality_scale);
        for (int i = 0; i < firefly_count; ++i) {
            const float rx = HashFloat01(i, 101);
            const float ry = HashFloat01(i, 202);
            const float speed = 9.0f + HashFloat01(i, 303) * 17.0f;
            const float phase = HashFloat01(i, 404) * 6.28318f;

            const float wx = min_x + std::fmod(rx * span_x + std::sin(sim_time * 0.85f + phase) * 24.0f + span_x, span_x);
            const float wy = min_y + std::fmod(ry * span_y - sim_time * speed + span_y * 10.0f, span_y);
            const float glow = 0.50f + 0.50f * std::sin(sim_time * 3.5f + phase);

            Color c = pal::kJadeGlow;
            if (zone == eco::ZoneId::DawnTerraces || zone == eco::ZoneId::HarmonySanctuary) {
                c = (i % 3 == 0) ? pal::kJadeGlow : pal::kBronzeBright;
            } else if (zone == eco::ZoneId::CanalQuarter) {
                c = (i % 2 == 0) ? Color{135, 240, 255, 255} : pal::kJadeGlow;
            } else if (zone == eco::ZoneId::FumaroleCaldera) {
                c = (i % 2 == 0) ? pal::kAmberGlow : Color{236, 224, 185, 255};
            }
            const auto alpha = static_cast<unsigned char>(185.0f * glow * std::max(0.35f, health));
            DrawCircleV({wx, wy}, 3.8f * glow, pal::WithAlpha(c, static_cast<unsigned char>(alpha / 3)));
            DrawCircleV({wx, wy}, 1.7f, pal::WithAlpha(c, alpha));
        }

        // 2. Effetti caratteristici del distretto inquadrato
        if (zone == eco::ZoneId::WindPlateau) {
            // Correnti d'aria / scie di vento oblique sull'Altopiano dei Venti
            const int wind_count = static_cast<int>(18.0f * g_particle_quality_scale);
            for (int i = 0; i < wind_count; ++i) {
                const float rx = HashFloat01(i, 511);
                const float ry = HashFloat01(i, 613);
                const float spd = 110.0f + HashFloat01(i, 719) * 85.0f;
                const float wx = min_x + std::fmod(rx * span_x + sim_time * spd, span_x);
                const float wy = min_y + std::fmod(ry * span_y + std::sin(sim_time * 1.4f + static_cast<float>(i)) * 16.0f + span_y, span_y);
                const float len = 24.0f + HashFloat01(i, 811) * 28.0f;
                const Color wc = pal::WorldColor(Color{165, 152, 134, 65}, Color{208, 236, 248, 85}, poll);
                DrawLineEx({wx, wy}, {wx + len, wy - 5.0f}, 1.4f, wc);
            }
        } else if (zone == eco::ZoneId::FumaroleCaldera) {
            // Sb sbuffi di vapore termale / braci lente nella Caldera delle Fumarole
            const int ember_count = static_cast<int>(24.0f * g_particle_quality_scale);
            for (int i = 0; i < ember_count; ++i) {
                const float rx = HashFloat01(i, 521);
                const float ry = HashFloat01(i, 631);
                const float spd = 18.0f + HashFloat01(i, 733) * 22.0f;
                const float wx = min_x + std::fmod(rx * span_x + std::sin(sim_time * 2.1f + static_cast<float>(i)) * 18.0f + span_x, span_x);
                const float wy = min_y + std::fmod(ry * span_y - sim_time * spd + span_y * 10.0f, span_y);
                const Color ec = pal::WorldColor(Color{235, 118, 52, 155}, Color{215, 238, 242, 95}, poll);
                DrawCircleV({wx, wy}, 2.0f + static_cast<float>(i % 3) * 0.7f, ec);
            }
        } else if (health > 0.35f) {
            // Petali e foglie portati dalla brezza nei distretti mediterranei / Giardini
            const int leaf_count = static_cast<int>((health - 0.35f) * 32.0f * g_particle_quality_scale);
            for (int i = 0; i < leaf_count; ++i) {
                const float rx = HashFloat01(i, 505);
                const float ry = HashFloat01(i, 606);
                const float wx = min_x + std::fmod(rx * span_x + sim_time * 24.0f, span_x);
                const float wy = min_y + std::fmod(ry * span_y + std::sin(sim_time * 2.0f + static_cast<float>(i)) * 18.0f + span_y, span_y);
                const Color lc = (zone == eco::ZoneId::HangingGardens && (i % 3 == 0))
                                     ? Color{248, 178, 188, 175}
                                     : Color{128, 226, 154, 165};
                DrawPoly({wx, wy}, 4, 2.7f, sim_time * 75.0f + static_cast<float>(i * 23), lc);
            }
        }
    } else {
        // Particelle dedicate a ciascuna delle 3 strutture interne (viewport-culled, zero allocazioni)
        const float map_w = world.ActiveTilemap().WorldWidth();
        const float map_h = world.ActiveTilemap().WorldHeight();
        const float vis_min_x = min_x - 32.0f;
        const float vis_max_x = min_x + span_x + 32.0f;
        const float vis_min_y = min_y - 32.0f;
        const float vis_max_y = min_y + span_y + 32.0f;
        const auto in_viewport = [&](float px, float py) noexcept {
            return px >= vis_min_x && px <= vis_max_x && py >= vis_min_y && py <= vis_max_y;
        };

        switch (world.active_structure) {
            case eco::StructureId::HypogeanCisterns: {
                // Nebbia idraulica e gocce luminose ascendenti nelle Cisterne Ipogee
                const int mist_count = static_cast<int>(38.0f * g_particle_quality_scale);
                for (int i = 0; i < mist_count; ++i) {
                    const float rx = HashFloat01(i, 707);
                    const float ry = HashFloat01(i, 808);
                    const float wx = 72.0f + rx * std::max(200.0f, map_w - 144.0f) + std::sin(sim_time * 1.5f + static_cast<float>(i)) * 12.0f;
                    const float wy = 72.0f + std::fmod(ry * std::max(200.0f, map_h - 144.0f) - sim_time * (14.0f + (i % 5) * 4.0f) + map_h * 10.0f,
                                                       std::max(200.0f, map_h - 144.0f));
                    if (!in_viewport(wx, wy)) {
                        continue;
                    }
                    const auto alpha = static_cast<unsigned char>(55 + (i % 4) * 24);
                    const Color drop_col = pal::WorldColor(
                        Color{140, 145, 118, alpha},
                        Color{132, 228, 255, alpha},
                        poll);
                    DrawCircleV({wx, wy}, 2.1f + static_cast<float>(i % 3) * 0.8f, drop_col);
                }
                break;
            }
            case eco::StructureId::PendulumTower: {
                // Pulviscolo dorato sospeso nei fasci di luce degli oculi della Torre del Pendolo
                const int dust_count = static_cast<int>(42.0f * g_particle_quality_scale);
                for (int i = 0; i < dust_count; ++i) {
                    const float rx = HashFloat01(i, 743);
                    const float ry = HashFloat01(i, 853);
                    const float phase = HashFloat01(i, 911) * 6.28318f;
                    const float wx = 72.0f + rx * std::max(200.0f, map_w - 144.0f) + std::sin(sim_time * 0.7f + phase) * 16.0f;
                    const float wy = 72.0f + std::fmod(ry * std::max(200.0f, map_h - 144.0f) + sim_time * (6.0f + (i % 3) * 3.0f),
                                                       std::max(200.0f, map_h - 144.0f));
                    if (!in_viewport(wx, wy)) {
                        continue;
                    }
                    const float shimmer = 0.45f + 0.55f * std::sin(sim_time * 2.8f + phase);
                    const Color mote = pal::WorldColor(
                        Color{180, 158, 120, static_cast<unsigned char>(95.0f * shimmer)},
                        Color{252, 222, 138, static_cast<unsigned char>(175.0f * shimmer)},
                        poll);
                    DrawCircleV({wx, wy}, 2.0f + shimmer * 1.1f, mote);
                }
                break;
            }
            case eco::StructureId::GeothermalChamber:
            case eco::StructureId::Count: {
                // Vapore termale e scintille ramate nella Camera Geotermica
                const int steam_count = static_cast<int>(40.0f * g_particle_quality_scale);
                for (int i = 0; i < steam_count; ++i) {
                    const float rx = HashFloat01(i, 761);
                    const float ry = HashFloat01(i, 863);
                    const float spd = 18.0f + (i % 5) * 6.0f;
                    const float wx = 72.0f + rx * std::max(200.0f, map_w - 144.0f) + std::sin(sim_time * 1.9f + static_cast<float>(i)) * 14.0f;
                    const float wy = 72.0f + std::fmod(ry * std::max(200.0f, map_h - 144.0f) - sim_time * spd + map_h * 10.0f,
                                                       std::max(200.0f, map_h - 144.0f));
                    if (!in_viewport(wx, wy)) {
                        continue;
                    }
                    const bool is_spark = (i % 4 == 0);
                    const Color col = is_spark
                                          ? pal::WorldColor(Color{245, 112, 48, 175}, Color{245, 198, 102, 165}, poll)
                                          : pal::WorldColor(Color{156, 128, 114, 72}, Color{198, 236, 240, 82}, poll);
                    DrawCircleV({wx, wy}, is_spark ? 2.1f : (3.6f + static_cast<float>(i % 3) * 1.2f), col);
                }
                break;
            }
        }
    }
}

void DrawScreenSpaceAtmosphere(const eco::WorldContext& world) {
    const float sim_time = world.sim_time;
    const float poll = world.ecosystem.PollutionRatio();
    const float health = 1.0f - poll;
    const int sw = eco::kWindowWidth;
    const int sh = eco::kWindowHeight;

    // 0. Gradiente / tinta atmosferica di distretto (interpolata dolcemente al cambio zona)
    const Color zone_tint = UpdateSmoothedDistrictTint(world);
    if (world.current_mode == eco::GameMode::Overworld) {
        const unsigned char top_a = static_cast<unsigned char>(16.0f + 16.0f * health);
        const unsigned char mid_a = static_cast<unsigned char>(7.0f + 7.0f * health);
        const unsigned char bot_a = static_cast<unsigned char>(12.0f + 12.0f * health);
        DrawRectangle(0, 52, sw, sh - 52, pal::WithAlpha(zone_tint, mid_a));
        DrawRectangleGradientV(0, 52, sw, 148, pal::WithAlpha(zone_tint, top_a), pal::WithAlpha(zone_tint, 0));
        DrawRectangleGradientV(0, sh - 128, sw, 128, pal::WithAlpha(zone_tint, 0), pal::WithAlpha(zone_tint, bot_a));
    } else {
        DrawRectangleGradientV(0, 52, sw, 110, pal::WithAlpha(zone_tint, 18), pal::WithAlpha(zone_tint, 0));
    }

    // 1. Raggi solari d'alba (God-Rays) nell'Overworld quando la Conca e' limpida
    if (world.current_mode == eco::GameMode::Overworld && health > 0.72f) {
        const float ray_strength = (health - 0.72f) / 0.28f;
        for (int r = 0; r < 4; ++r) {
            const float shift = std::sin(sim_time * 0.42f + static_cast<float>(r) * 1.35f) * 32.0f;
            const float x0 = 130.0f + static_cast<float>(r) * 295.0f + shift;
            const float w = 72.0f + static_cast<float>(r % 2) * 36.0f;
            const auto alpha = static_cast<unsigned char>(21.0f * ray_strength);
            const Color sun_ray{255, 238, 172, alpha};
            DrawTriangle(
                Vector2{x0, 52.0f},
                Vector2{x0 + 220.0f, static_cast<float>(sh)},
                Vector2{x0 + 220.0f + w, static_cast<float>(sh)},
                sun_ray);
        }
    }

    // 2. Foschia e cenere di pece obliqua quando l'inquinamento e' attivo
    const float smog_ratio = world.ecosystem.SmogAlpha();
    if (smog_ratio > 0.01f) {
        const auto smog_a = static_cast<unsigned char>(smog_ratio * 185.0f);
        const Color smog_col = pal::LerpColor(Color{54, 58, 52, smog_a}, Color{72, 56, 44, smog_a}, poll);
        DrawRectangle(0, 52, sw, sh - 52, smog_col);

        const int ash_count = static_cast<int>(poll * 52.0f * g_particle_quality_scale);
        for (int i = 0; i < ash_count; ++i) {
            const float sx = HashFloat01(i, 909) * static_cast<float>(sw);
            const float sy = 52.0f + HashFloat01(i, 1010) * static_cast<float>(sh - 52);
            const float vx = 20.0f + HashFloat01(i, 1111) * 24.0f;
            const float vy = 15.0f + HashFloat01(i, 1212) * 19.0f;
            const float px = std::fmod(sx + sim_time * vx, static_cast<float>(sw));
            const float py = 52.0f + std::fmod((sy - 52.0f) + sim_time * vy + std::sin(sim_time * 1.7f + static_cast<float>(i)) * 10.0f + static_cast<float>(sh),
                                               static_cast<float>(sh - 52));
            const auto a = static_cast<unsigned char>(80 + (i % 4) * 26);
            DrawCircleV({px, py}, 1.7f + static_cast<float>(i % 2) * 0.9f, Color{166, 150, 132, a});
        }
    }

    // 3. Vignettatura morbida cinematografica
    DrawRectangleGradientV(0, 52, sw, 32, Color{0, 0, 0, 72}, Color{0, 0, 0, 0});
    DrawRectangleGradientV(0, sh - 46, sw, 46, Color{0, 0, 0, 0}, Color{0, 0, 0, 92});
}

} // namespace eco::app
