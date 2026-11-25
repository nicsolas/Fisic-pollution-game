#include "app/render/particles.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "app/render/palette.h"
#include "app/render/ui_style.h"
#include "core/collectible.h"
#include "core/ecosystem.h"
#include "core/story.h"
#include "core/tilemap.h"

namespace eco::app {

namespace {

EnvironmentEffectsConfig g_env_effects{};

// Stato statico (zero allocazioni) per l'interpolazione morbida della tinta atmosferica di distretto
struct DistrictTintState {
    float r = 255.0f;
    float g = 204.0f;
    float b = 112.0f;
    float last_sim_time = -1.0f;
    bool initialized = false;
};

DistrictTintState g_district_tint{};

[[nodiscard]] Color ScaleAlpha(Color c, float factor) noexcept {
    const float clamped = std::clamp(factor, 0.0f, 1.0f);
    return Color{
        c.r,
        c.g,
        c.b,
        static_cast<unsigned char>(static_cast<float>(c.a) * clamped)
    };
}

[[nodiscard]] float Smoothstep01(float t) noexcept {
    const float x = std::clamp(t, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

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

[[nodiscard]] const char* InterludeZoneLabel(eco::ZoneId zone) noexcept {
    switch (zone) {
        case eco::ZoneId::DawnTerraces:
            return "TERRAZZE DELL'ALBA";
        case eco::ZoneId::CanalQuarter:
            return "QUARTIERE DEI CANALI";
        case eco::ZoneId::WindPlateau:
            return "ALTOPIANO DEI VENTI";
        case eco::ZoneId::HangingGardens:
            return "GIARDINI SOSPESI";
        case eco::ZoneId::FumaroleCaldera:
            return "CALDERA DELLE FUMAROLE";
        case eco::ZoneId::HarmonySanctuary:
            return "SANTUARIO DELL'ARMONIA";
        case eco::ZoneId::Count:
            break;
    }
    return "CONCA DI MERIDIA";
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

void DrawSystemicActionBursts(const eco::WorldContext& world) {
    // 1. Impulso di Prova (RunTrial): onde diagnostiche graduate + scintille empiriche
    if (world.trial_pulse_timer > 0.01f) {
        const float t = 1.0f - std::clamp(world.trial_pulse_timer / 1.25f, 0.0f, 1.0f);
        const float fade = (1.0f - t) * (1.0f - t);
        const eco::Vec2 origin = world.trial_pulse_origin;
        const bool is_hydraulic = (world.last_trial_machine == eco::MachineType::HydroTurbine) ||
                                  (world.current_mode == eco::GameMode::Structure &&
                                   world.active_structure == eco::StructureId::HypogeanCisterns);
        const Color ring_primary = is_hydraulic ? pal::kAccentTrial : pal::kAccentRisky;
        const Color ring_secondary = is_hydraulic ? pal::kBioMossCyan : pal::kConduitCleanGold;

        const float r1 = 18.0f + t * 155.0f;
        const float r2 = 10.0f + t * 102.0f;
        const float r3 = 6.0f + t * 56.0f;

        DrawCircleLines(
            static_cast<int>(origin.x),
            static_cast<int>(origin.y),
            r1,
            pal::WithAlpha(ring_primary, static_cast<unsigned char>(235.0f * fade)));
        DrawCircleLines(
            static_cast<int>(origin.x),
            static_cast<int>(origin.y),
            r2,
            pal::WithAlpha(ring_secondary, static_cast<unsigned char>(205.0f * fade)));
        DrawCircleLines(
            static_cast<int>(origin.x),
            static_cast<int>(origin.y),
            r3,
            pal::WithAlpha(RAYWHITE, static_cast<unsigned char>(190.0f * fade)));

        // Tacche goniometriche/manometriche sull'onda esterna per lettura empirica
        for (int deg = 0; deg < 12; ++deg) {
            const float ang = static_cast<float>(deg) * (2.0f * eco::kPi / 12.0f) + t * 0.35f;
            const Vector2 p0{origin.x + std::cos(ang) * (r1 - 6.0f), origin.y + std::sin(ang) * (r1 - 6.0f)};
            const Vector2 p1{origin.x + std::cos(ang) * (r1 + 5.0f), origin.y + std::sin(ang) * (r1 + 5.0f)};
            DrawLineEx(p0, p1, 1.8f, pal::WithAlpha(ring_primary, static_cast<unsigned char>(210.0f * fade)));
        }

        // 18 scintille diagnostiche radiali
        for (int i = 0; i < 18; ++i) {
            const float ang = static_cast<float>(i) * (2.0f * eco::kPi / 18.0f) + HashFloat01(i, 311) * 0.2f;
            const float dist = (14.0f + HashFloat01(i, 317) * 118.0f) * t;
            const Vector2 p{
                origin.x + std::cos(ang) * dist,
                origin.y + std::sin(ang) * dist - t * 14.0f
            };
            const float rad = (2.4f + HashFloat01(i, 331) * 1.6f) * (1.0f - t * 0.5f);
            DrawCircleV(p, rad * 1.9f, pal::WithAlpha(ring_primary, static_cast<unsigned char>(95.0f * fade)));
            DrawCircleV(p, rad, pal::WithAlpha(RAYWHITE, static_cast<unsigned char>(235.0f * fade)));
        }
    }

    // 2. Fioritura Vasca Botanica (Seme d'Ulivo Bianco): onda d'argento/giada e foglioline ascendenti
    if (world.planter_bloom_timer > 0.01f) {
        const float t = 1.0f - std::clamp(world.planter_bloom_timer / 1.50f, 0.0f, 1.0f);
        const float fade = 1.0f - t * t;
        const eco::Vec2 pos = world.planter_bloom_pos;

        const float r_outer = 20.0f + t * 175.0f;
        const float r_inner = 12.0f + t * 110.0f;
        DrawCircleV(
            {pos.x, pos.y},
            r_inner * 0.65f,
            pal::WithAlpha(pal::kJadeGlow, static_cast<unsigned char>(46.0f * fade)));
        DrawCircleLines(
            static_cast<int>(pos.x),
            static_cast<int>(pos.y),
            r_outer,
            pal::WithAlpha(pal::kFloraWhiteOliveHi, static_cast<unsigned char>(230.0f * fade)));
        DrawCircleLines(
            static_cast<int>(pos.x),
            static_cast<int>(pos.y),
            r_inner,
            pal::WithAlpha(pal::kFloraPollenGold, static_cast<unsigned char>(210.0f * fade)));

        for (int i = 0; i < 22; ++i) {
            const float ang = static_cast<float>(i) * (2.0f * eco::kPi / 22.0f) + t * 1.4f;
            const float dist = (12.0f + HashFloat01(i, 419) * 120.0f) * std::sqrt(t);
            const float lift = t * (22.0f + HashFloat01(i, 431) * 32.0f);
            const Vector2 lp{
                pos.x + std::cos(ang) * dist,
                pos.y + std::sin(ang) * dist * 0.72f - lift
            };
            const Color leaf_col = (i % 3 == 0)
                                       ? pal::kFloraPollenGold
                                       : ((i % 2 == 0) ? pal::kFloraWhiteOliveHi : pal::kJadeGlow);
            DrawPoly(
                lp,
                4,
                3.2f * (1.0f - t * 0.35f),
                world.sim_time * 90.0f + static_cast<float>(i * 19),
                pal::WithAlpha(leaf_col, static_cast<unsigned char>(225.0f * fade)));
        }
    }

    // 3. Impulso Remoto del Prisma del Congedo: raggio rifratto tra Elena e il nodo bersaglio
    if (world.prism_pulse_timer > 0.01f) {
        const float t = 1.0f - std::clamp(world.prism_pulse_timer / 1.10f, 0.0f, 1.0f);
        const float fade = (1.0f - t);
        const Vector2 a{world.prism_pulse_from.x, world.prism_pulse_from.y};
        const Vector2 b{world.prism_pulse_to.x, world.prism_pulse_to.y};

        DrawLineEx(a, b, 7.5f * fade, pal::WithAlpha(pal::kBioMossCyan, static_cast<unsigned char>(95.0f * fade)));
        DrawLineEx(a, b, 3.6f * fade, pal::WithAlpha(pal::kConduitCleanGold, static_cast<unsigned char>(195.0f * fade)));
        DrawLineEx(a, b, 1.6f, pal::WithAlpha(RAYWHITE, static_cast<unsigned char>(240.0f * fade)));

        DrawCircleLines(
            static_cast<int>(a.x),
            static_cast<int>(a.y),
            12.0f + t * 34.0f,
            pal::WithAlpha(pal::kBioMossCyan, static_cast<unsigned char>(210.0f * fade)));
        DrawCircleLines(
            static_cast<int>(b.x),
            static_cast<int>(b.y),
            14.0f + t * 52.0f,
            pal::WithAlpha(pal::kConduitCleanGold, static_cast<unsigned char>(235.0f * fade)));

        for (int k = 0; k < 7; ++k) {
            const float u = std::fmod(static_cast<float>(k) / 7.0f + t * 1.35f, 1.0f);
            const Vector2 node{
                a.x + (b.x - a.x) * u,
                a.y + (b.y - a.y) * u
            };
            DrawPoly(
                node,
                6,
                4.2f * fade,
                world.sim_time * 120.0f + static_cast<float>(k * 30),
                pal::WithAlpha(RAYWHITE, static_cast<unsigned char>(220.0f * fade)));
        }
    }
}

void DrawZoneMechanicParticles(
    const eco::WorldContext& world,
    float min_x,
    float min_y,
    float span_x,
    float span_y,
    float weather_scale) {
    if (weather_scale <= 0.01f) {
        return;
    }

    const float sim_time = world.sim_time;
    const float poll = world.ecosystem.PollutionRatio();
    const float health = 1.0f - poll;
    const eco::ZoneId zone = world.current_zone;
    const auto& zm = world.zone_mechanics;

    const float vis_min_x = min_x - 48.0f;
    const float vis_max_x = min_x + span_x + 48.0f;
    const float vis_min_y = min_y - 48.0f;
    const float vis_max_y = min_y + span_y + 48.0f;
    const auto in_view = [&](float px, float py) noexcept {
        return px >= vis_min_x && px <= vis_max_x && py >= vis_min_y && py <= vis_max_y;
    };

    switch (zone) {
        case eco::ZoneId::DawnTerraces: {
            // 1. Polline dorato sospeso nella brezza dell'alba
            const int pollen_count = static_cast<int>((14.0f + 16.0f * health) * weather_scale);
            for (int i = 0; i < pollen_count; ++i) {
                const float rx = HashFloat01(i, 1301);
                const float ry = HashFloat01(i, 1307);
                const float phase = HashFloat01(i, 1319) * 6.28318f;
                const float wx = min_x + std::fmod(rx * span_x + sim_time * (14.0f + (i % 4) * 4.0f), span_x);
                const float wy = min_y + std::fmod(ry * span_y + std::sin(sim_time * 1.4f + phase) * 14.0f + span_y, span_y);
                const float shimmer = 0.55f + 0.45f * std::sin(sim_time * 3.1f + phase);
                const Color pc = (i % 3 == 0)
                                     ? Color{255, 240, 178, static_cast<unsigned char>(155.0f * shimmer)}
                                     : Color{248, 206, 98, static_cast<unsigned char>(135.0f * shimmer)};
                DrawCircleV({wx, wy}, 1.8f + shimmer * 0.9f, pc);
            }

            // 2. Pulviscolo luminoso lungo i fasci solari attivi
            for (std::size_t s = 0; s < zm.solar_beams.size(); ++s) {
                const auto& seg = zm.solar_beams[s];
                if (!seg.active) {
                    continue;
                }
                const float dx = seg.end.x - seg.start.x;
                const float dy = seg.end.y - seg.start.y;
                const float len = std::hypot(dx, dy);
                if (len < 12.0f) {
                    continue;
                }
                const int motes = std::clamp(static_cast<int>((len / 52.0f) * weather_scale), 2, 14);
                const float nx = -dy / len;
                const float ny = dx / len;
                for (int m = 0; m < motes; ++m) {
                    const float base_t = HashFloat01(m + static_cast<int>(s) * 19, 1361);
                    const float t = std::fmod(base_t + sim_time * 0.24f, 1.0f);
                    const float jitter = std::sin(sim_time * 4.2f + static_cast<float>(m * 3 + s)) * 5.0f;
                    const float px = seg.start.x + dx * t + nx * jitter;
                    const float py = seg.start.y + dy * t + ny * jitter;
                    if (!in_view(px, py)) {
                        continue;
                    }
                    DrawCircleV({px, py}, 2.4f, Color{255, 232, 138, 92});
                    DrawCircleV({px, py}, 1.3f, Color{255, 250, 224, 210});
                }
            }

            // 3. Scintille ascendenti dalle piastre fotosensibili illuminate
            for (const auto& plate : zm.solar_plates) {
                if ((!plate.lit && plate.charge <= 0.12f) || !in_view(plate.pos.x, plate.pos.y)) {
                    continue;
                }
                const float intensity = std::max(plate.charge, plate.lit ? 1.0f : 0.0f);
                const int sparks = static_cast<int>(8.0f * intensity * weather_scale);
                for (int i = 0; i < sparks; ++i) {
                    const float life = std::fmod(sim_time * 0.9f + HashFloat01(i + plate.id * 13, 1409), 1.0f);
                    const float ang = HashFloat01(i + plate.id * 17, 1423) * 6.28318f;
                    const float rad = plate.radius * (0.25f + 0.65f * HashFloat01(i, 1433));
                    const float px = plate.pos.x + std::cos(ang) * rad;
                    const float py = plate.pos.y + std::sin(ang) * rad * 0.7f - life * 26.0f;
                    const auto a = static_cast<unsigned char>(210.0f * (1.0f - life) * intensity);
                    DrawCircleV({px, py}, 2.0f * (1.0f - life * 0.5f), Color{255, 234, 142, a});
                }
            }
            break;
        }

        case eco::ZoneId::CanalQuarter: {
            // 1. Goccioline di brezza acquatica sul quartiere dei canali
            const int mist_count = static_cast<int>(16.0f * weather_scale);
            for (int i = 0; i < mist_count; ++i) {
                const float rx = HashFloat01(i, 1511);
                const float ry = HashFloat01(i, 1523);
                const float wx = min_x + std::fmod(rx * span_x + sim_time * 16.0f, span_x);
                const float wy = min_y + std::fmod(ry * span_y - sim_time * 11.0f + span_y * 8.0f, span_y);
                const Color mc = pal::WorldColor(Color{132, 146, 128, 72}, Color{142, 238, 255, 118}, poll);
                DrawCircleV({wx, wy}, 1.9f + static_cast<float>(i % 2) * 0.7f, mc);
            }

            // 2. Scie di schiuma e riflessi vivi nelle correnti d'acqua attive
            for (std::size_t c = 0; c < zm.water_currents.size(); ++c) {
                const auto& cur = zm.water_currents[c];
                if (!cur.active) {
                    continue;
                }
                const float area = cur.bounds.width * cur.bounds.height;
                const int streaks = std::clamp(static_cast<int>((area / 6200.0f) * weather_scale), 3, 20);
                const float flen = std::hypot(cur.flow_dir.x, cur.flow_dir.y);
                const float fx = (flen > 0.001f) ? (cur.flow_dir.x / flen) : 1.0f;
                const float fy = (flen > 0.001f) ? (cur.flow_dir.y / flen) : 0.0f;
                const float speed = std::clamp(cur.strength * 0.55f, 40.0f, 130.0f);

                for (int i = 0; i < streaks; ++i) {
                    const float sx = HashFloat01(i + static_cast<int>(c) * 29, 1559);
                    const float sy = HashFloat01(i + static_cast<int>(c) * 31, 1571);
                    const float px = cur.bounds.x + std::fmod(sx * cur.bounds.width + sim_time * fx * speed + cur.bounds.width * 20.0f,
                                                              std::max(1.0f, cur.bounds.width));
                    const float py = cur.bounds.y + std::fmod(sy * cur.bounds.height + sim_time * fy * speed + cur.bounds.height * 20.0f,
                                                              std::max(1.0f, cur.bounds.height));
                    if (!in_view(px, py)) {
                        continue;
                    }
                    const float slen = 12.0f + HashFloat01(i, 1583) * 14.0f;
                    const Color foam = pal::WorldColor(Color{136, 146, 120, 95}, Color{178, 248, 255, 165}, poll);
                    DrawLineEx({px, py}, {px + fx * slen, py + fy * slen}, 1.5f, foam);
                }
            }
            break;
        }

        case eco::ZoneId::WindPlateau: {
            // Reazione live a WindState (calma -> telegraphing -> raffica impetuosa)
            const auto& w = zm.wind;
            const float wlen = std::hypot(w.wind_dir.x, w.wind_dir.y);
            const float dx = (wlen > 0.001f) ? (w.wind_dir.x / wlen) : -1.0f;
            const float dy = (wlen > 0.001f) ? (w.wind_dir.y / wlen) : 0.15f;

            const float gust = w.gust_active ? std::max(0.35f, w.gust_intensity)
                                             : (w.telegraphing ? 0.30f : 0.10f);
            const int wind_count = static_cast<int>((14.0f + 44.0f * gust) * weather_scale);
            const float base_speed = 95.0f + 290.0f * gust;

            for (int i = 0; i < wind_count; ++i) {
                const float rx = HashFloat01(i, 511);
                const float ry = HashFloat01(i, 613);
                const float spd = base_speed * (0.75f + HashFloat01(i, 719) * 0.55f);
                const float wx = min_x + std::fmod(rx * span_x + sim_time * dx * spd + span_x * 20.0f, span_x);
                const float wy = min_y + std::fmod(ry * span_y + sim_time * dy * spd +
                                                   std::sin(sim_time * 2.2f + static_cast<float>(i)) * (10.0f * (1.0f - gust * 0.6f)) +
                                                   span_y * 20.0f,
                                                   span_y);
                const float len = (18.0f + 44.0f * gust) * (0.7f + HashFloat01(i, 811) * 0.6f);
                const float thickness = w.gust_active ? (1.4f + 1.1f * gust) : 1.2f;
                const auto alpha = static_cast<unsigned char>(std::clamp(58.0f + 135.0f * gust, 40.0f, 205.0f));
                const Color wc = w.telegraphing && !w.gust_active
                                     ? Color{196, 232, 255, alpha}
                                     : pal::WorldColor(Color{165, 152, 134, alpha}, Color{216, 240, 255, alpha}, poll);
                DrawLineEx({wx, wy}, {wx + dx * len, wy + dy * len}, thickness, wc);
            }
            break;
        }

        case eco::ZoneId::HangingGardens: {
            // 1. Petali e foglie portati dalla brezza nei Giardini Sospesi
            const int leaf_count = static_cast<int>((14.0f + 24.0f * health) * weather_scale);
            for (int i = 0; i < leaf_count; ++i) {
                const float rx = HashFloat01(i, 505);
                const float ry = HashFloat01(i, 606);
                const float wx = min_x + std::fmod(rx * span_x + sim_time * 24.0f, span_x);
                const float wy = min_y + std::fmod(ry * span_y + std::sin(sim_time * 2.0f + static_cast<float>(i)) * 18.0f + span_y, span_y);
                const Color lc = (i % 3 == 0)
                                     ? Color{248, 178, 188, 175}
                                     : ((i % 3 == 1) ? Color{252, 228, 128, 160} : Color{118, 228, 152, 165});
                DrawPoly({wx, wy}, 4, 2.7f, sim_time * 75.0f + static_cast<float>(i * 23), lc);
            }

            // 2. Scintille e onde armoniche dalle piastre di risonanza attive o appena colpite
            for (const auto& pad : zm.resonance_pads) {
                if ((!pad.activated && pad.pulse_timer <= 0.01f) || !in_view(pad.pos.x, pad.pos.y)) {
                    continue;
                }
                if (pad.pulse_timer > 0.01f) {
                    const float p = 1.0f - std::clamp(pad.pulse_timer / 0.8f, 0.0f, 1.0f);
                    const float r = pad.radius * (0.85f + p * 1.45f);
                    DrawCircleLines(
                        static_cast<int>(pad.pos.x),
                        static_cast<int>(pad.pos.y),
                        r,
                        pal::WithAlpha(pal::kJadeGlow, static_cast<unsigned char>(190.0f * (1.0f - p))));
                }
                if (pad.activated) {
                    const int sparks = static_cast<int>(6.0f * weather_scale);
                    for (int i = 0; i < sparks; ++i) {
                        const float life = std::fmod(sim_time * 0.75f + HashFloat01(i + pad.id * 11, 1601), 1.0f);
                        const float ang = sim_time * 1.4f + static_cast<float>(i) * (6.28318f / std::max(1, sparks));
                        const float px = pad.pos.x + std::cos(ang) * (pad.radius * 0.65f);
                        const float py = pad.pos.y + std::sin(ang) * (pad.radius * 0.45f) - life * 28.0f;
                        const auto a = static_cast<unsigned char>(200.0f * (1.0f - life));
                        DrawCircleV({px, py}, 2.1f, pal::WithAlpha(pal::kJadeGlow, a));
                    }
                }
            }
            break;
        }

        case eco::ZoneId::FumaroleCaldera: {
            // 1. Braci lente e cenere termale di fondo
            const int ember_count = static_cast<int>(22.0f * weather_scale);
            for (int i = 0; i < ember_count; ++i) {
                const float rx = HashFloat01(i, 521);
                const float ry = HashFloat01(i, 631);
                const float spd = 18.0f + HashFloat01(i, 733) * 24.0f;
                const float wx = min_x + std::fmod(rx * span_x + std::sin(sim_time * 2.1f + static_cast<float>(i)) * 18.0f + span_x, span_x);
                const float wy = min_y + std::fmod(ry * span_y - sim_time * spd + span_y * 10.0f, span_y);
                const Color ec = pal::WorldColor(Color{235, 118, 52, 155}, Color{215, 238, 242, 95}, poll);
                DrawCircleV({wx, wy}, 2.0f + static_cast<float>(i % 3) * 0.7f, ec);
            }

            // 2. Vapore e getti termici reattivi per ogni fumarola/geyser
            for (const auto& g : zm.geysers) {
                if (!in_view(g.pos.x, g.pos.y)) {
                    continue;
                }
                if (g.phase == eco::GeyserPhase::Erupting) {
                    const float eff = std::max(0.45f, g.intensity);
                    const int puffs = static_cast<int>(16.0f * eff * weather_scale);
                    for (int i = 0; i < puffs; ++i) {
                        const float life = std::fmod(sim_time * 1.9f + HashFloat01(i + g.id * 23, 1709), 1.0f);
                        const float spread = (HashFloat01(i + g.id * 7, 1721) - 0.5f) * g.radius * (0.7f + life * 1.2f);
                        const float px = g.pos.x + spread;
                        const float py = g.pos.y - life * (52.0f + 32.0f * eff);
                        const float rad = (4.5f + life * 9.5f) * eff;
                        const auto a = static_cast<unsigned char>(135.0f * (1.0f - life));
                        DrawCircleV({px, py}, rad, Color{236, 224, 212, a});
                        if (i % 3 == 0) {
                            DrawCircleV({px, py + 4.0f}, 2.2f, Color{252, 136, 54, static_cast<unsigned char>(210.0f * (1.0f - life))});
                        }
                    }
                } else if (g.phase == eco::GeyserPhase::Telegraphing) {
                    const int wisps = static_cast<int>(8.0f * weather_scale);
                    for (int i = 0; i < wisps; ++i) {
                        const float life = std::fmod(sim_time * 1.5f + HashFloat01(i + g.id * 19, 1733), 1.0f);
                        const float spread = (HashFloat01(i, 1741) - 0.5f) * g.radius * 0.8f;
                        const float px = g.pos.x + spread;
                        const float py = g.pos.y - life * 24.0f;
                        DrawCircleV({px, py}, 2.6f + life * 2.5f, Color{248, 172, 74, static_cast<unsigned char>(145.0f * (1.0f - life))});
                    }
                } else if (g.phase == eco::GeyserPhase::Calmed) {
                    const int mist = static_cast<int>(5.0f * weather_scale);
                    for (int i = 0; i < mist; ++i) {
                        const float life = std::fmod(sim_time * 0.55f + HashFloat01(i + g.id * 13, 1753), 1.0f);
                        const float spread = std::sin(sim_time * 1.4f + static_cast<float>(i)) * (g.radius * 0.45f);
                        const float px = g.pos.x + spread;
                        const float py = g.pos.y - life * 26.0f;
                        DrawCircleV({px, py}, 3.5f + life * 4.5f, Color{156, 232, 240, static_cast<unsigned char>(78.0f * (1.0f - life))});
                    }
                }
            }
            break;
        }

        case eco::ZoneId::HarmonySanctuary: {
            // Scintille armoniche orbitanti attorno ai 5 sigilli accordati del Santuario
            for (const auto& seal : zm.sanctuary_seals) {
                if ((!seal.attuned && seal.glow <= 0.1f) || !in_view(seal.pos.x, seal.pos.y)) {
                    continue;
                }
                const Color accent = pal::GetDistrictPalette(seal.element_zone).ambient_accent;
                const float g = std::max(seal.glow, seal.attuned ? 1.0f : 0.0f);
                const int motes = static_cast<int>(7.0f * g * weather_scale);
                for (int i = 0; i < motes; ++i) {
                    const float ang = sim_time * 1.6f + static_cast<float>(i) * (6.28318f / std::max(1, motes));
                    const float r = seal.radius * (0.65f + 0.18f * std::sin(sim_time * 2.5f + static_cast<float>(i)));
                    const float px = seal.pos.x + std::cos(ang) * r;
                    const float py = seal.pos.y + std::sin(ang) * r * 0.72f - std::abs(std::sin(sim_time + static_cast<float>(i))) * 8.0f;
                    DrawCircleV({px, py}, 3.0f, pal::WithAlpha(accent, static_cast<unsigned char>(85.0f * g)));
                    DrawCircleV({px, py}, 1.5f, pal::WithAlpha(RAYWHITE, static_cast<unsigned char>(215.0f * g)));
                }
            }
            break;
        }

        case eco::ZoneId::Count:
            break;
    }
}

// ============================================================================
// Vignette Panoramiche Procedurali per gli Intramezzi tra Zone
// ============================================================================

enum class InterludeSceneKind : int {
    DawnArch = 0,          // DawnTerraces <-> CanalQuarter (Arco dell'Alba)
    BasaltStair = 1,       // WindPlateau <-> HangingGardens (Scalinata di Basalto)
    SluiceBridge = 2,      // CanalQuarter <-> WindPlateau (Ponte delle Chiuse / Ponte degli Archi)
    ObsidianThreshold = 3, // HangingGardens <-> FumaroleCaldera (Soglia d'Ossidiana)
    SanctuaryColonnade = 4,// FumaroleCaldera <-> HarmonySanctuary (Colonnato del Santuario)
    GenericMeridia = 5     // Fallback elegante
};

[[nodiscard]] InterludeSceneKind ClassifyThresholdScene(eco::ZoneId a, eco::ZoneId b) noexcept {
    const auto is_pair = [&](eco::ZoneId x, eco::ZoneId y) noexcept {
        return (a == x && b == y) || (a == y && b == x);
    };
    if (is_pair(eco::ZoneId::DawnTerraces, eco::ZoneId::CanalQuarter)) {
        return InterludeSceneKind::DawnArch;
    }
    if (is_pair(eco::ZoneId::CanalQuarter, eco::ZoneId::WindPlateau)) {
        return InterludeSceneKind::SluiceBridge;
    }
    if (is_pair(eco::ZoneId::WindPlateau, eco::ZoneId::HangingGardens)) {
        return InterludeSceneKind::BasaltStair;
    }
    if (is_pair(eco::ZoneId::HangingGardens, eco::ZoneId::FumaroleCaldera)) {
        return InterludeSceneKind::ObsidianThreshold;
    }
    if (is_pair(eco::ZoneId::FumaroleCaldera, eco::ZoneId::HarmonySanctuary)) {
        return InterludeSceneKind::SanctuaryColonnade;
    }
    return InterludeSceneKind::GenericMeridia;
}

void DrawClampedRect(float x, float y, float w, float h, Rectangle clip, Color col) noexcept {
    const float x0 = std::max(x, clip.x);
    const float y0 = std::max(y, clip.y);
    const float x1 = std::min(x + w, clip.x + clip.width);
    const float y1 = std::min(y + h, clip.y + clip.height);
    if (x1 > x0 && y1 > y0) {
        DrawRectangle(
            static_cast<int>(x0),
            static_cast<int>(y0),
            static_cast<int>(x1 - x0),
            static_cast<int>(y1 - y0),
            col);
    }
}

void DrawInterludeHorizonHills(
    Rectangle v,
    float parallax,
    float base_y_ratio,
    float amp,
    float freq,
    float seed,
    Color col) {
    const int steps = 28;
    const float dx = v.width / static_cast<float>(steps);
    const float base_y = v.y + v.height * base_y_ratio;
    for (int i = 0; i < steps; ++i) {
        const float x0 = v.x + static_cast<float>(i) * dx;
        const float x1 = std::min(v.x + v.width, x0 + dx + 1.0f);
        const float h0 = std::sin((static_cast<float>(i) * freq) + parallax * 0.015f + seed) * amp +
                         std::cos((static_cast<float>(i) * freq * 0.55f) - seed) * (amp * 0.45f);
        const float h1 = std::sin((static_cast<float>(i + 1) * freq) + parallax * 0.015f + seed) * amp +
                         std::cos((static_cast<float>(i + 1) * freq * 0.55f) - seed) * (amp * 0.45f);
        const float y0 = std::clamp(base_y + h0, v.y + 40.0f, v.y + v.height - 6.0f);
        const float y1 = std::clamp(base_y + h1, v.y + 40.0f, v.y + v.height - 6.0f);
        DrawTriangle(
            {x0, v.y + v.height},
            {x1, y1},
            {x0, y0},
            col);
        DrawTriangle(
            {x0, v.y + v.height},
            {x1, v.y + v.height},
            {x1, y1},
            col);
    }
}

void DrawInterludePanoramicArt(
    Rectangle v,
    eco::ZoneId from_zone,
    eco::ZoneId to_zone,
    float progress,
    float sim_time,
    float pollution_ratio,
    float alpha) {
    const auto& p_from = pal::GetDistrictPalette(from_zone);
    const auto& p_to = pal::GetDistrictPalette(to_zone);
    const float poll = std::clamp(pollution_ratio, 0.0f, 1.0f);
    const float health = 1.0f - poll;
    const float parallax = (progress - 0.5f) * 54.0f;

    // 1. Cielo panoramico interpolato tra le palette di partenza e arrivo
    const Color sky_top_healthy = pal::LerpColor(
        pal::ShiftBrightness(p_from.atmosphere_tint, -95),
        pal::ShiftBrightness(p_to.atmosphere_tint, -85),
        0.55f);
    const Color sky_bot_healthy = pal::LerpColor(p_from.atmosphere_tint, p_to.atmosphere_tint, 0.55f);
    const Color sky_top = ScaleAlpha(pal::WorldColor(Color{32, 28, 28, 255}, sky_top_healthy, poll), alpha);
    const Color sky_bot = ScaleAlpha(pal::WorldColor(Color{84, 68, 56, 255}, sky_bot_healthy, poll), alpha);
    DrawRectangleGradientV(
        static_cast<int>(v.x),
        static_cast<int>(v.y),
        static_cast<int>(v.width),
        static_cast<int>(v.height),
        sky_top,
        sky_bot);

    // 2. Catene montuose / colline lontane in parallasse
    const Color far_hill = ScaleAlpha(
        pal::WorldColor(Color{48, 42, 40, 225}, pal::ShiftBrightness(p_to.wall_front_healthy, -14), poll),
        alpha);
    const Color mid_hill = ScaleAlpha(
        pal::WorldColor(Color{38, 34, 34, 240}, pal::ShiftBrightness(p_from.wall_front_healthy, -22), poll),
        alpha);
    DrawInterludeHorizonHills(v, parallax * 0.25f, 0.56f, 24.0f, 0.42f, 1.1f, far_hill);
    DrawInterludeHorizonHills(v, parallax * 0.50f, 0.68f, 18.0f, 0.58f, 2.7f, mid_hill);

    const InterludeSceneKind kind = ClassifyThresholdScene(from_zone, to_zone);
    const Color stone_light = ScaleAlpha(pal::WorldColor(p_to.wall_top_sick, p_to.wall_top_healthy, poll), alpha);
    const Color stone_dark = ScaleAlpha(pal::WorldColor(p_to.wall_front_sick, p_to.wall_front_healthy, poll), alpha);
    const Color accent_col = ScaleAlpha(pal::LerpColor(p_from.ambient_accent, p_to.ambient_accent, 0.6f), alpha);
    const Color brass_col = ScaleAlpha(pal::kBronzeBright, alpha);

    switch (kind) {
        case InterludeSceneKind::DawnArch: {
            // Disco solare dell'alba con aloni concentrici
            const Vector2 sun_pos{v.x + v.width * 0.50f - parallax * 0.22f, v.y + v.height * 0.44f};
            DrawCircleV(sun_pos, 108.0f, ScaleAlpha(Color{255, 224, 134, static_cast<unsigned char>(36 + 34 * health)}, alpha));
            DrawCircleV(sun_pos, 64.0f, ScaleAlpha(Color{255, 236, 168, static_cast<unsigned char>(64 + 48 * health)}, alpha));
            DrawCircleV(sun_pos, 32.0f, ScaleAlpha(Color{255, 246, 206, 210}, alpha));

            // Specchio d'acqua del canale in basso a destra
            const Color water_col = ScaleAlpha(pal::WorldColor(p_to.water_sick, p_to.water_healthy, poll), alpha);
            DrawClampedRect(v.x, v.y + v.height * 0.74f, v.width, v.height * 0.26f, v, water_col);
            for (int r = 0; r < 6; ++r) {
                const float ry = v.y + v.height * (0.78f + static_cast<float>(r) * 0.032f);
                const float rw = 160.0f - static_cast<float>(r) * 18.0f;
                const float rx = sun_pos.x - rw * 0.5f + std::sin(sim_time * 2.2f + static_cast<float>(r)) * 10.0f;
                DrawClampedRect(rx, ry, rw, 2.5f, v, ScaleAlpha(Color{255, 238, 178, 120}, alpha));
            }

            // Grande Arco di Travertino dell'Alba al centro
            const float cx = v.x + v.width * 0.50f + parallax * 0.65f;
            const float arch_w = 290.0f;
            const float pillar_w = 52.0f;
            const float top_y = v.y + 78.0f;
            const float base_y = v.y + v.height;
            DrawClampedRect(cx - arch_w * 0.5f, top_y, pillar_w, base_y - top_y, v, stone_dark);
            DrawClampedRect(cx + arch_w * 0.5f - pillar_w, top_y, pillar_w, base_y - top_y, v, stone_dark);
            DrawClampedRect(cx - arch_w * 0.5f - 18.0f, top_y - 22.0f, arch_w + 36.0f, 34.0f, v, stone_light);
            DrawClampedRect(cx - arch_w * 0.5f - 24.0f, top_y - 28.0f, arch_w + 48.0f, 8.0f, v, brass_col);
            // Ghiera gnomonica in ottone sulla chiave di volta
            DrawCircleLines(static_cast<int>(cx), static_cast<int>(top_y + 18.0f), 26.0f, brass_col);
            DrawCircleV({cx, top_y + 18.0f}, 5.5f, accent_col);
            break;
        }

        case InterludeSceneKind::BasaltStair: {
            // Scalinata monumentale in basalto che sale verso le rupi dell'Altopiano dei Venti
            const float base_x = v.x + 80.0f + parallax * 0.55f;
            const Color basalt_top = ScaleAlpha(Color{92, 108, 128, 255}, alpha);
            const Color basalt_side = ScaleAlpha(Color{34, 42, 58, 255}, alpha);
            for (int step = 0; step < 12; ++step) {
                const float t = static_cast<float>(step) / 11.0f;
                const float sx = base_x + t * 520.0f;
                const float sy = v.y + v.height - 22.0f - t * 210.0f;
                const float sw = v.width - (sx - v.x) + 40.0f;
                DrawClampedRect(sx, sy, sw, 26.0f, v, basalt_side);
                DrawClampedRect(sx, sy, sw, 5.0f, v, basalt_top);
            }
            // Monoliti frangivento e cipressi piegati sulla cresta
            for (int m = 0; m < 4; ++m) {
                const float mx = v.x + 210.0f + static_cast<float>(m) * 185.0f + parallax * 0.72f;
                const float my = v.y + 125.0f + static_cast<float>(3 - m) * 28.0f;
                DrawClampedRect(mx, my, 24.0f, 150.0f, v, basalt_side);
                DrawClampedRect(mx + 4.0f, my + 8.0f, 3.0f, 120.0f, v, ScaleAlpha(pal::kPalWindPlateau.inlay_healthy, alpha * 0.85f));
            }
            // Scie di vento oblique in primo piano
            for (int w = 0; w < 14; ++w) {
                const float wy = v.y + 42.0f + HashFloat01(w, 2111) * (v.height - 84.0f);
                const float wx = v.x + std::fmod(HashFloat01(w, 2129) * v.width - sim_time * (180.0f + (w % 4) * 45.0f) + v.width * 20.0f, v.width);
                const float wlen = 65.0f + HashFloat01(w, 2141) * 90.0f;
                const float x2 = std::max(v.x + 4.0f, wx - wlen);
                DrawLineEx({wx, wy}, {x2, wy + 12.0f}, 1.8f, ScaleAlpha(Color{212, 240, 255, 140}, alpha));
            }
            break;
        }

        case InterludeSceneKind::SluiceBridge: {
            // Acqua zaffiro del grande canale
            const float water_y = v.y + v.height * 0.68f;
            const Color water_top = ScaleAlpha(pal::WorldColor(p_from.water_sick, p_from.water_healthy, poll), alpha);
            const Color water_bot = ScaleAlpha(pal::ShiftBrightness(p_from.water_healthy, -45), alpha);
            DrawRectangleGradientV(
                static_cast<int>(v.x),
                static_cast<int>(water_y),
                static_cast<int>(v.width),
                static_cast<int>(v.y + v.height - water_y),
                water_top,
                water_bot);

            // Ponte degli Archi (5 arcate in pietra e bronzo)
            const float deck_y = v.y + v.height * 0.44f;
            DrawClampedRect(v.x, deck_y, v.width, 28.0f, v, stone_light);
            DrawClampedRect(v.x, deck_y - 6.0f, v.width, 6.0f, v, brass_col);
            const int arches = 5;
            const float span = v.width / static_cast<float>(arches);
            for (int a = 0; a <= arches; ++a) {
                const float px = v.x + static_cast<float>(a) * span + parallax * 0.55f - 16.0f;
                DrawClampedRect(px, deck_y + 24.0f, 32.0f, v.y + v.height - (deck_y + 24.0f), v, stone_dark);
                // Sbuffi di schiuma idraulica alla base dei piloni
                const float foam_r = 14.0f + std::sin(sim_time * 3.0f + static_cast<float>(a)) * 4.0f;
                if (px + 16.0f > v.x + 12.0f && px + 16.0f < v.x + v.width - 12.0f) {
                    DrawCircleV({px + 16.0f, water_y + 8.0f}, foam_r, ScaleAlpha(Color{190, 248, 255, 95}, alpha));
                }
            }
            // Tralci smeraldo dei Giardini Sospesi che scendono dall'alto a destra
            for (int vine = 0; vine < 7; ++vine) {
                const float vx = v.x + v.width * 0.58f + static_cast<float>(vine) * 54.0f + parallax * 0.85f;
                if (vx < v.x + 10.0f || vx > v.x + v.width - 10.0f) {
                    continue;
                }
                const float vlen = 72.0f + HashFloat01(vine, 2203) * 115.0f;
                const float sway = std::sin(sim_time * 1.8f + static_cast<float>(vine)) * 8.0f;
                DrawLineEx({vx, v.y + 2.0f}, {vx + sway, v.y + vlen}, 3.0f, ScaleAlpha(pal::kPalHangingGardens.canopy_healthy, alpha));
                DrawCircleV({vx + sway, v.y + vlen}, 5.5f, ScaleAlpha(pal::kPalHangingGardens.blossom_a, alpha));
            }
            break;
        }

        case InterludeSceneKind::ObsidianThreshold: {
            // Terreno scuro di basalto e fessure geotermiche luminose
            const float floor_y = v.y + v.height * 0.72f;
            DrawClampedRect(v.x, floor_y, v.width, v.y + v.height - floor_y, v, ScaleAlpha(Color{30, 24, 28, 255}, alpha));
            for (int f = 0; f < 5; ++f) {
                const float fx = v.x + 140.0f + static_cast<float>(f) * 165.0f + parallax * 0.6f;
                const float glow = 0.65f + 0.35f * std::sin(sim_time * 3.2f + static_cast<float>(f) * 1.3f);
                DrawClampedRect(fx, floor_y + 18.0f, 82.0f, 5.0f, v, ScaleAlpha(Color{252, 134, 48, static_cast<unsigned char>(210.0f * glow)}, alpha));
                DrawClampedRect(fx + 10.0f, floor_y + 16.0f, 62.0f, 2.0f, v, ScaleAlpha(Color{255, 224, 146, static_cast<unsigned char>(220.0f * glow)}, alpha));
            }
            // Colonne di giada a sinistra vs pilastri esagonali d'ossidiana a destra
            for (int col = 0; col < 3; ++col) {
                const float lx = v.x + 56.0f + static_cast<float>(col) * 86.0f + parallax * 0.7f;
                DrawClampedRect(lx, v.y + 40.0f + static_cast<float>(col) * 24.0f, 28.0f, v.y + v.height - (v.y + 40.0f), v, ScaleAlpha(Color{42, 82, 68, 240}, alpha));
                DrawClampedRect(lx + 4.0f, v.y + 46.0f + static_cast<float>(col) * 24.0f, 4.0f, 160.0f, v, ScaleAlpha(pal::kJadeGlow, alpha * 0.65f));
            }
            for (int col = 0; col < 4; ++col) {
                const float rx = v.x + v.width - 96.0f - static_cast<float>(col) * 78.0f + parallax * 0.7f;
                const float top = v.y + 28.0f + static_cast<float>(col % 2) * 34.0f;
                DrawClampedRect(rx, top, 42.0f, v.y + v.height - top, v, ScaleAlpha(Color{36, 32, 42, 248}, alpha));
                DrawClampedRect(rx + 2.0f, top, 4.0f, v.y + v.height - top, v, ScaleAlpha(Color{244, 126, 52, 135}, alpha));
            }
            break;
        }

        case InterludeSceneKind::SanctuaryColonnade: {
            // Cupola lontana dell'Astrolabio Centrale con anelli celesti
            const Vector2 dome_c{v.x + v.width * 0.50f - parallax * 0.25f, v.y + v.height * 0.46f};
            const float pulse = 0.6f + 0.4f * std::sin(sim_time * 2.4f);
            DrawCircleV(dome_c, 92.0f, ScaleAlpha(Color{104, 246, 220, static_cast<unsigned char>(32 + 24 * pulse)}, alpha));
            DrawCircleLines(static_cast<int>(dome_c.x), static_cast<int>(dome_c.y), 74.0f, ScaleAlpha(pal::kJadeGlow, alpha * 0.75f));
            DrawCircleLines(static_cast<int>(dome_c.x), static_cast<int>(dome_c.y), 48.0f, brass_col);
            DrawCircleV(dome_c, 12.0f, ScaleAlpha(Color{248, 246, 232, 230}, alpha));

            // Specchio d'acqua centrale del colonnato
            const float pool_y = v.y + v.height * 0.72f;
            DrawClampedRect(v.x, pool_y, v.width, v.y + v.height - pool_y, v, ScaleAlpha(Color{30, 118, 136, 235}, alpha));
            DrawClampedRect(dome_c.x - 42.0f, pool_y + 12.0f, 84.0f, 4.0f, v, ScaleAlpha(pal::kJadeGlow, alpha * 0.75f));

            // Colonnato simmetrico in marmo avorio in prospettiva
            for (int side = -1; side <= 1; side += 2) {
                for (int c = 0; c < 4; ++c) {
                    const float depth = static_cast<float>(c) / 3.0f;
                    const float cx = v.x + v.width * 0.50f + static_cast<float>(side) * (135.0f + depth * 280.0f) + parallax * (0.4f + depth * 0.5f);
                    const float cw = 18.0f + depth * 22.0f;
                    const float top = v.y + 92.0f - depth * 64.0f;
                    DrawClampedRect(cx - cw * 0.5f, top, cw, v.y + v.height - top, v, stone_light);
                    DrawClampedRect(cx - cw * 0.5f - 4.0f, top, cw + 8.0f, 8.0f, v, brass_col);
                }
            }
            break;
        }

        case InterludeSceneKind::GenericMeridia: {
            // Arco dei Meridiani generico con cresta astrolabica
            const float cx = v.x + v.width * 0.50f + parallax * 0.5f;
            DrawClampedRect(cx - 140.0f, v.y + 84.0f, 40.0f, v.height - 84.0f, v, stone_dark);
            DrawClampedRect(cx + 100.0f, v.y + 84.0f, 40.0f, v.height - 84.0f, v, stone_dark);
            DrawClampedRect(cx - 156.0f, v.y + 62.0f, 312.0f, 28.0f, v, stone_light);
            DrawCircleLines(static_cast<int>(cx), static_cast<int>(v.y + 132.0f), 34.0f, brass_col);
            DrawCircleV({cx, v.y + 132.0f}, 7.0f, accent_col);
            break;
        }
    }

    // 3. Pulviscolo atmosferico fluttuante nella vignetta
    const int motes = 24;
    for (int i = 0; i < motes; ++i) {
        const float rx = HashFloat01(i, 2309);
        const float ry = HashFloat01(i, 2333);
        const float phase = HashFloat01(i, 2347) * 6.28318f;
        const float mx = v.x + 12.0f + std::fmod(rx * (v.width - 24.0f) + sim_time * (14.0f + (i % 3) * 8.0f), v.width - 24.0f);
        const float my = v.y + 12.0f + std::fmod(ry * (v.height - 24.0f) - sim_time * (8.0f + (i % 4) * 5.0f) + v.height * 10.0f, v.height - 24.0f);
        const float glow = 0.5f + 0.5f * std::sin(sim_time * 3.4f + phase);
        DrawCircleV({mx, my}, 2.0f + glow * 1.3f, ScaleAlpha(pal::WithAlpha(p_to.ambient_accent, static_cast<unsigned char>(175.0f * glow)), alpha));
    }

    // 4. Sfumatura morbida sui bordi interni della vignetta
    DrawRectangleGradientV(
        static_cast<int>(v.x),
        static_cast<int>(v.y),
        static_cast<int>(v.width),
        46,
        ScaleAlpha(Color{6, 10, 16, 155}, alpha),
        Color{6, 10, 16, 0});
    DrawRectangleGradientV(
        static_cast<int>(v.x),
        static_cast<int>(v.y + v.height - 64.0f),
        static_cast<int>(v.width),
        64,
        Color{6, 10, 16, 0},
        ScaleAlpha(Color{6, 10, 16, 195}, alpha));

    // 5. Cornice ornamentale in ottone e pietra
    DrawRectangleLinesEx(v, 2.0f, ScaleAlpha(pal::LerpColor(p_to.ambient_accent, pal::kBronzeBright, 0.55f), alpha));
    DrawRectangleLinesEx(
        Rectangle{v.x + 5.0f, v.y + 5.0f, v.width - 10.0f, v.height - 10.0f},
        1.0f,
        ScaleAlpha(pal::WithAlpha(pal::kBronzeBright, 115), alpha));
}

} // namespace

void SetEnvironmentEffectsConfig(const EnvironmentEffectsConfig& cfg) noexcept {
    g_env_effects.particle_density = std::clamp(cfg.particle_density, 0.0f, 1.0f);
    g_env_effects.fog_haze_intensity = std::clamp(cfg.fog_haze_intensity, 0.0f, 1.0f);
    g_env_effects.sun_rays_intensity = std::clamp(cfg.sun_rays_intensity, 0.0f, 1.0f);
    g_env_effects.vignette_intensity = std::clamp(cfg.vignette_intensity, 0.0f, 1.0f);
    g_env_effects.zone_weather_intensity = std::clamp(cfg.zone_weather_intensity, 0.0f, 1.0f);
    g_env_effects.atmosphere_enabled = cfg.atmosphere_enabled;
}

const EnvironmentEffectsConfig& GetEnvironmentEffectsConfig() noexcept {
    return g_env_effects;
}

void SetParticleQualityScale(float scale) noexcept {
    g_env_effects.particle_density = std::clamp(scale, 0.0f, 1.0f);
}

float GetParticleQualityScale() noexcept {
    return g_env_effects.particle_density;
}

void DrawWorldAtmosphericParticles(
    const eco::WorldContext& world,
    eco::Vec2 camera_target) {
    // Il burst della raccolta reliquia e gli impulsi sistemici sono sempre leggibili
    DrawCollectiblePickupBurst(world);
    DrawSystemicActionBursts(world);

    const float p_scale = g_env_effects.particle_density;
    if (p_scale <= 0.01f) {
        return;
    }
    const float w_scale = p_scale * g_env_effects.zone_weather_intensity;

    const float sim_time = world.sim_time;
    const eco::ZoneEcoStatus eco_status = world.ActiveSceneEcoStatus();
    const float local_poll = 1.0f - std::clamp(eco_status.local_vitality, 0.0f, 1.0f);
    const float poll = std::clamp(local_poll * 0.72f + world.ecosystem.PollutionRatio() * 0.28f, 0.0f, 1.0f);
    const float health = 1.0f - poll;
    const eco::CalibrationApproach approach = world.ActiveMachine().approach;
    const float min_x = camera_target.x - static_cast<float>(eco::kWindowWidth) * 0.58f;
    const float min_y = camera_target.y - static_cast<float>(eco::kWindowHeight) * 0.58f;
    const float span_x = static_cast<float>(eco::kWindowWidth) * 1.16f;
    const float span_y = static_cast<float>(eco::kWindowHeight) * 1.16f;

    // Particelle specifiche del regime di calibrazione attivo (CleanSlow vs RiskyEfficient vs DirtyImmediate)
    if (approach == eco::CalibrationApproach::CleanSlow) {
        const int clean_motes = static_cast<int>(18.0f * w_scale);
        for (int i = 0; i < clean_motes; ++i) {
            const float rx = HashFloat01(i, 1901);
            const float ry = HashFloat01(i, 1907);
            const float phase = HashFloat01(i, 1913) * 6.28318f;
            const float wx = min_x + std::fmod(rx * span_x + std::sin(sim_time * 1.1f + phase) * 28.0f + span_x, span_x);
            const float wy = min_y + std::fmod(ry * span_y - sim_time * (11.0f + (i % 4) * 3.0f) + span_y * 10.0f, span_y);
            const float pulse = 0.5f + 0.5f * std::sin(sim_time * 2.8f + phase);
            const Color col = (i % 2 == 0) ? pal::kFloraPollenGold : pal::kFloraWhiteOliveHi;
            DrawCircleV({wx, wy}, 2.4f * pulse, pal::WithAlpha(col, static_cast<unsigned char>(175.0f * pulse)));
        }
    } else if (approach == eco::CalibrationApproach::RiskyEfficient) {
        const int risky_sparks = static_cast<int>(24.0f * w_scale);
        for (int i = 0; i < risky_sparks; ++i) {
            const float rx = HashFloat01(i, 1931);
            const float ry = HashFloat01(i, 1949);
            const float spd = 34.0f + HashFloat01(i, 1951) * 42.0f;
            const float phase = HashFloat01(i, 1973) * 6.28318f;
            const float wx = min_x + std::fmod(rx * span_x + std::sin(sim_time * 6.5f + phase) * 10.0f + span_x, span_x);
            const float wy = min_y + std::fmod(ry * span_y - sim_time * spd + span_y * 20.0f, span_y);
            const float flicker = 0.55f + 0.45f * std::sin(sim_time * 11.0f + phase);
            DrawLineEx(
                {wx, wy},
                {wx + std::sin(sim_time * 9.0f + phase) * 3.0f, wy + 7.5f},
                1.6f,
                pal::WithAlpha(pal::kConduitRiskyAmber, static_cast<unsigned char>(210.0f * flicker)));
            DrawCircleV({wx, wy}, 1.6f, pal::WithAlpha(RAYWHITE, static_cast<unsigned char>(220.0f * flicker)));
        }
    } else if (approach == eco::CalibrationApproach::DirtyImmediate) {
        const int soot_flakes = static_cast<int>(28.0f * w_scale);
        for (int i = 0; i < soot_flakes; ++i) {
            const float rx = HashFloat01(i, 1979);
            const float ry = HashFloat01(i, 1987);
            const float vx = 16.0f + HashFloat01(i, 1993) * 20.0f;
            const float vy = 22.0f + HashFloat01(i, 1997) * 26.0f;
            const float wx = min_x + std::fmod(rx * span_x + sim_time * vx, span_x);
            const float wy = min_y + std::fmod(ry * span_y + sim_time * vy, span_y);
            DrawCircleV({wx, wy}, 2.4f + static_cast<float>(i % 2) * 0.9f, pal::WithAlpha(pal::kConduitDirtySoot, 175));
        }
    }

    if (world.current_mode == eco::GameMode::Overworld) {
        const eco::ZoneId zone = world.current_zone;

        // 1. Lucciole e pulviscolo luminoso della Conca (spariscono con il bypass sporco)
        const int firefly_base = (zone == eco::ZoneId::HangingGardens || zone == eco::ZoneId::HarmonySanctuary) ? 52 : 34;
        const float approach_mult = (approach == eco::CalibrationApproach::DirtyImmediate)
                                        ? 0.0f
                                        : ((approach == eco::CalibrationApproach::CleanSlow) ? 1.35f : 1.0f);
        const int firefly_count = static_cast<int>(
            std::max(0.15f, health) * static_cast<float>(firefly_base) * p_scale *
            (0.35f + 0.65f * g_env_effects.zone_weather_intensity) * approach_mult);
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

        // 2. Effetti meteo/ambientali reattivi alle 6 zone separate e alle loro meccaniche
        DrawZoneMechanicParticles(world, min_x, min_y, span_x, span_y, w_scale);
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
        const float struct_scale = p_scale * (0.45f + 0.55f * g_env_effects.zone_weather_intensity);

        switch (world.active_structure) {
            case eco::StructureId::HypogeanCisterns: {
                const int mist_count = static_cast<int>(38.0f * struct_scale);
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
                    const Color drop_col = (approach == eco::CalibrationApproach::RiskyEfficient)
                                               ? Color{255, 206, 122, alpha}
                                               : pal::WorldColor(
                                                     Color{140, 145, 118, alpha},
                                                     Color{132, 228, 255, alpha},
                                                     poll);
                    DrawCircleV({wx, wy}, 2.1f + static_cast<float>(i % 3) * 0.8f, drop_col);
                }
                break;
            }
            case eco::StructureId::PendulumTower: {
                const int dust_count = static_cast<int>(42.0f * struct_scale);
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
                const int steam_count = static_cast<int>(40.0f * struct_scale);
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
    if (!g_env_effects.atmosphere_enabled) {
        return;
    }

    const float sim_time = world.sim_time;
    const eco::ZoneEcoStatus eco_status = world.ActiveSceneEcoStatus();
    const float local_poll = 1.0f - std::clamp(eco_status.local_vitality, 0.0f, 1.0f);
    const float poll = std::clamp(local_poll * 0.72f + world.ecosystem.PollutionRatio() * 0.28f, 0.0f, 1.0f);
    const float health = 1.0f - poll;
    const eco::CalibrationApproach approach = world.ActiveMachine().approach;
    const int sw = eco::kWindowWidth;
    const int sh = eco::kWindowHeight;

    const float fog_k = g_env_effects.fog_haze_intensity;
    const float ray_k = g_env_effects.sun_rays_intensity;
    const float vig_k = g_env_effects.vignette_intensity;

    // 0. Gradiente / tinta atmosferica di distretto (scalata da fog_haze_intensity)
    const Color zone_tint = UpdateSmoothedDistrictTint(world);
    if (fog_k > 0.01f) {
        if (world.current_mode == eco::GameMode::Overworld) {
            const auto top_a = static_cast<unsigned char>((16.0f + 16.0f * health) * fog_k);
            const auto mid_a = static_cast<unsigned char>((7.0f + 7.0f * health) * fog_k);
            const auto bot_a = static_cast<unsigned char>((12.0f + 12.0f * health) * fog_k);
            DrawRectangle(0, 52, sw, sh - 52, pal::WithAlpha(zone_tint, mid_a));
            DrawRectangleGradientV(0, 52, sw, 148, pal::WithAlpha(zone_tint, top_a), pal::WithAlpha(zone_tint, 0));
            DrawRectangleGradientV(0, sh - 128, sw, 128, pal::WithAlpha(zone_tint, 0), pal::WithAlpha(zone_tint, bot_a));
        } else {
            DrawRectangleGradientV(0, 52, sw, 110, pal::WithAlpha(zone_tint, static_cast<unsigned char>(18.0f * fog_k)), pal::WithAlpha(zone_tint, 0));
        }
    }

    // 1. Raggi solari d'alba (God-Rays) scalati da sun_rays_intensity e potenziati in CleanSlow
    if (world.current_mode == eco::GameMode::Overworld && ray_k > 0.01f &&
        approach != eco::CalibrationApproach::DirtyImmediate) {
        const bool sunlit_zone = (world.current_zone == eco::ZoneId::DawnTerraces ||
                                  world.current_zone == eco::ZoneId::HarmonySanctuary);
        const float clean_boost = (approach == eco::CalibrationApproach::CleanSlow) ? 0.22f : 0.0f;
        const float base_strength = sunlit_zone
                                        ? std::clamp(0.38f + health * 0.62f + clean_boost, 0.25f, 1.0f)
                                        : std::clamp((health - 0.50f) / 0.50f + clean_boost, 0.0f, 1.0f);
        const float ray_strength = base_strength * ray_k;
        if (ray_strength > 0.02f) {
            for (int r = 0; r < 4; ++r) {
                const float shift = std::sin(sim_time * 0.42f + static_cast<float>(r) * 1.35f) * 32.0f;
                const float x0 = 130.0f + static_cast<float>(r) * 295.0f + shift;
                const float w = 72.0f + static_cast<float>(r % 2) * 36.0f;
                const auto alpha = static_cast<unsigned char>(26.0f * ray_strength);
                const Color sun_ray = (world.current_zone == eco::ZoneId::HarmonySanctuary)
                                          ? Color{188, 250, 232, alpha}
                                          : Color{255, 238, 172, alpha};
                DrawTriangle(
                    Vector2{x0, 52.0f},
                    Vector2{x0 + 220.0f, static_cast<float>(sh)},
                    Vector2{x0 + 220.0f + w, static_cast<float>(sh)},
                    sun_ray);
            }
        }
    }

    // 1b. Riverbero Termico / Sovraccarico Critico (RiskyEfficient o Impulso di Prova attivo)
    const float trial_heat = std::clamp(world.trial_pulse_timer / 1.25f, 0.0f, 1.0f);
    if (approach == eco::CalibrationApproach::RiskyEfficient || trial_heat > 0.05f) {
        const float pulse = 0.55f + 0.45f * std::sin(sim_time * 8.2f);
        const float intensity = std::max(
            (approach == eco::CalibrationApproach::RiskyEfficient) ? (0.65f + 0.35f * pulse) : 0.0f,
            trial_heat * 0.85f);
        const auto rim_a = static_cast<unsigned char>(34.0f * intensity * std::max(0.4f, fog_k));
        const Color heat_col = (trial_heat > 0.35f) ? pal::kAccentTrial : pal::kAccentRisky;
        DrawRectangleGradientV(0, 52, sw, 68, pal::WithAlpha(heat_col, rim_a), pal::WithAlpha(heat_col, 0));
        DrawRectangleGradientV(0, sh - 74, sw, 74, pal::WithAlpha(heat_col, 0), pal::WithAlpha(heat_col, rim_a));
    }

    // 2. Foschia acida e cenere di pece obliqua quando l'inquinamento o il bypass sporco sono attivi
    const float dirty_boost = (approach == eco::CalibrationApproach::DirtyImmediate) ? 0.32f : 0.0f;
    const float smog_ratio = std::max(world.ecosystem.SmogAlpha(), poll * 0.35f + dirty_boost);
    if (smog_ratio > 0.01f && fog_k > 0.01f) {
        const auto smog_a = static_cast<unsigned char>(std::clamp(smog_ratio * 185.0f * fog_k, 0.0f, 155.0f));
        const Color smog_col = (approach == eco::CalibrationApproach::DirtyImmediate)
                                   ? Color{46, 38, 32, smog_a}
                                   : pal::LerpColor(Color{54, 58, 52, smog_a}, Color{72, 56, 44, smog_a}, poll);
        DrawRectangle(0, 52, sw, sh - 52, smog_col);

        const float ash_factor = (approach == eco::CalibrationApproach::DirtyImmediate)
                                     ? std::max(0.65f, poll)
                                     : poll;
        const int ash_count = static_cast<int>(ash_factor * 56.0f * g_env_effects.particle_density * fog_k);
        for (int i = 0; i < ash_count; ++i) {
            const float sx = HashFloat01(i, 909) * static_cast<float>(sw);
            const float sy = 52.0f + HashFloat01(i, 1010) * static_cast<float>(sh - 52);
            const float vx = 20.0f + HashFloat01(i, 1111) * 24.0f;
            const float vy = 15.0f + HashFloat01(i, 1212) * 19.0f;
            const float px = std::fmod(sx + sim_time * vx, static_cast<float>(sw));
            const float py = 52.0f + std::fmod((sy - 52.0f) + sim_time * vy + std::sin(sim_time * 1.7f + static_cast<float>(i)) * 10.0f + static_cast<float>(sh),
                                               static_cast<float>(sh - 52));
            const auto a = static_cast<unsigned char>(80 + (i % 4) * 26);
            const Color ash_col = (approach == eco::CalibrationApproach::DirtyImmediate && (i % 2 == 0))
                                      ? Color{92, 78, 66, a}
                                      : Color{166, 150, 132, a};
            DrawCircleV({px, py}, 1.7f + static_cast<float>(i % 2) * 0.9f, ash_col);
        }
    }

    // 3. Vignettatura morbida cinematografica (scalata da vignette_intensity)
    if (vig_k > 0.01f) {
        const auto top_v = static_cast<unsigned char>(78.0f * vig_k);
        const auto bot_v = static_cast<unsigned char>(98.0f * vig_k);
        const auto side_v = static_cast<unsigned char>(54.0f * vig_k);
        DrawRectangleGradientV(0, 52, sw, 38, Color{0, 0, 0, top_v}, Color{0, 0, 0, 0});
        DrawRectangleGradientV(0, sh - 52, sw, 52, Color{0, 0, 0, 0}, Color{0, 0, 0, bot_v});
        DrawRectangleGradientH(0, 52, 48, sh - 52, Color{0, 0, 0, side_v}, Color{0, 0, 0, 0});
        DrawRectangleGradientH(sw - 48, 52, 48, sh - 52, Color{0, 0, 0, 0}, Color{0, 0, 0, side_v});
    }
}

void DrawZoneInterludeOverlay(
    eco::ZoneId from_zone,
    eco::ZoneId to_zone,
    float progress,
    float sim_time,
    float pollution_ratio,
    const char* title,
    const char* body_line1,
    const char* body_line2,
    const char* skip_hint,
    const char* direction_tag) {
    const float p = std::clamp(progress, 0.0f, 1.0f);

    // Dissolvenza morbida in ingresso (0..0.18) e uscita (0.80..1.0)
    float fade = 1.0f;
    if (p < 0.18f) {
        fade = Smoothstep01(p / 0.18f);
    } else if (p > 0.80f) {
        fade = 1.0f - Smoothstep01((p - 0.80f) / 0.20f);
    }
    if (fade <= 0.01f) {
        return;
    }

    const int sw = eco::kWindowWidth;
    const int sh = eco::kWindowHeight;
    const auto& p_to = pal::GetDistrictPalette(to_zone);
    const Color accent = p_to.ambient_accent;

    // 1. Sipario scuro cinematografico (copre in modo pulito lo swap di memoria al 50%)
    DrawRectangle(0, 0, sw, sh, ScaleAlpha(Color{8, 13, 20, 250}, fade));

    // 2. Vignetta panoramica procedurale centrata
    const Rectangle vignette_rect{160.0f, 64.0f, 960.0f, 384.0f};
    DrawInterludePanoramicArt(
        vignette_rect,
        from_zone,
        to_zone,
        p,
        sim_time,
        pollution_ratio,
        fade);

    // 3. Spazio tipografico inferiore curato
    const float text_fade = fade * Smoothstep01(std::clamp((p - 0.05f) / 0.16f, 0.0f, 1.0f));
    const float mid_x = static_cast<float>(sw) * 0.5f;

    char fallback_dir[128]{};
    const char* dir_str = direction_tag;
    if (dir_str == nullptr || dir_str[0] == '\0') {
        std::snprintf(
            fallback_dir,
            sizeof(fallback_dir),
            "%s  ->  %s",
            InterludeZoneLabel(from_zone),
            InterludeZoneLabel(to_zone));
        dir_str = fallback_dir;
    }

    const int dir_w = MeasureUiText(dir_str, 13);
    DrawTextShadow(
        dir_str,
        static_cast<int>(mid_x - static_cast<float>(dir_w) * 0.5f),
        468,
        13,
        ScaleAlpha(pal::LerpColor(accent, pal::kBronzeBright, 0.45f), text_fade));

    if (title != nullptr && title[0] != '\0') {
        const int tw = MeasureUiText(title, 28);
        DrawTextShadow(
            title,
            static_cast<int>(mid_x - static_cast<float>(tw) * 0.5f),
            492,
            28,
            ScaleAlpha(RAYWHITE, text_fade));
    }

    DrawDivider(static_cast<int>(mid_x) - 210, 534, 420, ScaleAlpha(accent, text_fade));

    if (body_line1 != nullptr && body_line1[0] != '\0') {
        const int w1 = MeasureUiText(body_line1, 17);
        DrawTextShadow(
            body_line1,
            static_cast<int>(mid_x - static_cast<float>(w1) * 0.5f),
            552,
            17,
            ScaleAlpha(pal::kParchmentText, text_fade));
    }
    if (body_line2 != nullptr && body_line2[0] != '\0') {
        const int w2 = MeasureUiText(body_line2, 17);
        DrawTextShadow(
            body_line2,
            static_cast<int>(mid_x - static_cast<float>(w2) * 0.5f),
            580,
            17,
            ScaleAlpha(Color{202, 218, 232, 240}, text_fade));
    }

    // 4. Barra di avanzamento discreta centrata
    const Rectangle bar{mid_x - 180.0f, 640.0f, 360.0f, 6.0f};
    DrawRectangleRec(bar, ScaleAlpha(Color{22, 34, 48, 220}, fade));
    const float fill_w = bar.width * p;
    if (fill_w > 1.0f) {
        DrawRectangle(
            static_cast<int>(bar.x),
            static_cast<int>(bar.y),
            static_cast<int>(fill_w),
            static_cast<int>(bar.height),
            ScaleAlpha(accent, fade));
    }
    DrawRectangleLinesEx(bar, 1.0f, ScaleAlpha(pal::WithAlpha(pal::kBronzeBright, 130), fade));

    // 5. Hint discreto per saltare in basso a destra
    if (skip_hint != nullptr && skip_hint[0] != '\0') {
        const char* key_lbl = "SPAZIO";
        const int badge_w = MeasureActionPromptBadge(4, key_lbl); // 4 = GamepadAction::Confirm
        const int hint_w = MeasureUiText(skip_hint, 13);
        const int total_w = badge_w + 8 + hint_w;
        const int hx = sw - 46 - total_w;
        const int hy = sh - 44;
        DrawActionPromptBadge(4, key_lbl, hx, hy, ScaleAlpha(accent, fade));
        DrawTextShadow(skip_hint, hx + badge_w + 8, hy + 2, 13, ScaleAlpha(Color{198, 214, 228, 220}, fade));
    }
}

void DrawZoneInterludeOverlay(const eco::WorldContext& world) {
    if (!world.interlude.active) {
        return;
    }
    const eco::ZoneInterludeText copy = eco::ZoneInterludeCopy(
        world.interlude.from_zone,
        world.interlude.to_zone);
    DrawZoneInterludeOverlay(
        world.interlude.from_zone,
        world.interlude.to_zone,
        world.interlude.Progress(),
        world.sim_time,
        world.ecosystem.PollutionRatio(),
        copy.threshold_title,
        copy.line1,
        copy.line2,
        world.interlude.CanSkip() ? "Salta" : "",
        copy.direction_tag);
}

} // namespace eco::app
