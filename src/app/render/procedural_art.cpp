#include "app/render/procedural_art.h"

#include <raylib.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "app/render/palette.h"
#include "app/render/particles.h"
#include "app/render/ui_style.h"

namespace eco::app {

namespace {

[[nodiscard]] bool IsWallLike(eco::TileId id) noexcept {
    return id == eco::TileId::Wall || id == eco::TileId::RuinsWall;
}

[[nodiscard]] bool IsWaterLike(eco::TileId id) noexcept {
    return id == eco::TileId::Water ||
           id == eco::TileId::HiddenBridge ||
           id == eco::TileId::ShallowChannel ||
           id == eco::TileId::JadeBridge;
}

[[nodiscard]] bool IsPavedLike(eco::TileId id) noexcept {
    return id == eco::TileId::PavedPath ||
           id == eco::TileId::SanctuaryFloor ||
           id == eco::TileId::HiddenBridge ||
           id == eco::TileId::ZoneGateway ||
           id == eco::TileId::SolarGate;
}

[[nodiscard]] bool IsTileSolidForProp(eco::TileId id) noexcept {
    return id != eco::TileId::Walkable && id != eco::TileId::GardenFloor;
}

[[nodiscard]] eco::Vec2 MirrorDirVector(int dir_index) noexcept {
    switch (dir_index & 3) {
        case 0: return {1.0f, 0.0f};   // Est
        case 1: return {0.0f, 1.0f};   // Sud
        case 2: return {-1.0f, 0.0f};  // Ovest
        default: return {0.0f, -1.0f}; // Nord
    }
}

[[nodiscard]] float NearestActivePlayerDistance(const eco::WorldContext& world, eco::Vec2 pos) noexcept {
    float best = 1e9f;
    for (const eco::Player* p : world.players.ActivePlayers()) {
        if (p != nullptr && p->active) {
            best = std::min(best, p->Center().DistanceTo(pos));
        }
    }
    return best;
}

[[nodiscard]] const eco::ZoneGateway* FindZoneGatewayAtTile(
    const eco::OverworldSim& overworld,
    int row,
    int col) noexcept {
    for (const auto& gw : overworld.active_gateways) {
        if (row >= gw.tile_min.row && row <= gw.tile_max.row &&
            col >= gw.tile_min.col && col <= gw.tile_max.col) {
            return &gw;
        }
    }
    return nullptr;
}

[[nodiscard]] eco::StructureId PortalStructureAtTile(
    const eco::OverworldSim& overworld,
    int row,
    int col) noexcept {
    for (const auto& p : overworld.portals) {
        if (row >= p.tile_min.row && row <= p.tile_max.row &&
            col >= p.tile_min.col && col <= p.tile_max.col) {
            return p.target_structure;
        }
    }
    const eco::ZoneId zone = overworld.active_zone;
    if (zone == eco::ZoneId::WindPlateau) {
        return eco::StructureId::PendulumTower;
    }
    if (zone == eco::ZoneId::FumaroleCaldera) {
        return eco::StructureId::GeothermalChamber;
    }
    return eco::StructureId::HypogeanCisterns;
}

// =============================================================================
// Stele Diegetica Scolpita (pietra + cornice bronzo, stile Conca di Meridia)
// =============================================================================
void DrawMeridianStele(float x, float y, const char* label, Color accent, float health) {
    DrawEllipse(static_cast<int>(x + 3.0f), static_cast<int>(y + 13.0f), 14.0f, 4.5f, Color{0, 0, 0, 105});

    const Color stone_base = pal::LerpColor(Color{74, 68, 62, 255}, Color{138, 134, 122, 255}, health);
    const Color stone_hi = pal::ShiftBrightness(stone_base, 26);
    DrawRectangle(static_cast<int>(x - 7.0f), static_cast<int>(y + 6.0f), 14, 7, stone_base);
    DrawRectangle(static_cast<int>(x - 4.0f), static_cast<int>(y - 7.0f), 8, 15, stone_base);
    DrawLine(static_cast<int>(x - 4.0f), static_cast<int>(y - 7.0f), static_cast<int>(x - 4.0f), static_cast<int>(y + 7.0f), stone_hi);

    const int tw = MeasureUiText(label, 11);
    const int bw = tw + 18;
    const int bx = static_cast<int>(x) - bw / 2;
    const int by = static_cast<int>(y - 25.0f);
    DrawRectangle(bx + 2, by + 2, bw, 19, Color{0, 0, 0, 95});
    DrawRectangle(bx, by, bw, 19, Color{18, 24, 34, 236});
    DrawRectangleLines(bx, by, bw, 19, pal::WithAlpha(pal::kBronze, 220));
    DrawRectangleLines(bx + 2, by + 2, bw - 4, 15, pal::WithAlpha(accent, 190));

    DrawCircle(bx + 5, by + 9, 1.5f, accent);
    DrawCircle(bx + bw - 5, by + 9, 1.5f, accent);
    DrawTextShadow(label, bx + 9, by + 4, 11, pal::kParchmentText);
}

// =============================================================================
// Mosaici Astronomici & Armonici Monumentali (World-Space, sotto i monumenti)
// =============================================================================
void DrawSolarSundialMosaic(eco::Vec2 center, float poll, float sim_time) {
    const float health = 1.0f - poll;
    const Color stone_ring = pal::WorldColor(Color{98, 86, 72, 175}, Color{196, 180, 148, 210}, poll);
    const Color bronze_inlay = pal::WorldColor(Color{116, 88, 54, 175}, Color{226, 182, 92, 235}, poll);
    const Color glow = pal::WithAlpha(pal::kAmberGlow, static_cast<unsigned char>(35.0f + 55.0f * health));

    DrawCircleV({center.x, center.y}, 94.0f, pal::WithAlpha(stone_ring, 70));
    DrawRing({center.x, center.y}, 86.0f, 92.0f, 0.0f, 360.0f, 36, stone_ring);
    DrawRing({center.x, center.y}, 64.0f, 67.0f, 0.0f, 360.0f, 32, bronze_inlay);
    DrawRing({center.x, center.y}, 36.0f, 38.0f, 0.0f, 360.0f, 24, bronze_inlay);

    for (int i = 0; i < 12; ++i) {
        const float ang = static_cast<float>(i) * (eco::kPi / 6.0f);
        const float r0 = (i % 3 == 0) ? 38.0f : 67.0f;
        const float r1 = (i % 3 == 0) ? 94.0f : 86.0f;
        DrawLineEx(
            {center.x + std::cos(ang) * r0, center.y + std::sin(ang) * r0},
            {center.x + std::cos(ang) * r1, center.y + std::sin(ang) * r1},
            (i % 3 == 0) ? 2.4f : 1.4f,
            bronze_inlay);
        if (i % 3 == 0) {
            const float px = center.x + std::cos(ang) * 98.0f;
            const float py = center.y + std::sin(ang) * 98.0f;
            DrawPoly({px, py}, 4, 4.5f, 45.0f, bronze_inlay);
        }
    }

    if (health > 0.25f) {
        const float pulse = 0.5f + 0.5f * std::sin(sim_time * 2.0f);
        DrawRing({center.x, center.y}, 67.0f, 70.0f + pulse * 2.0f, 0.0f, 360.0f, 32, glow);
    }
}

void DrawHarmonicWaveMosaic(eco::Vec2 center, float poll, float sim_time) {
    const float health = 1.0f - poll;
    const Color jade_inlay = pal::WorldColor(Color{94, 96, 78, 170}, Color{72, 224, 182, 225}, poll);
    const Color stone_ring = pal::WorldColor(Color{92, 84, 72, 165}, Color{178, 184, 166, 200}, poll);

    DrawCircleV({center.x, center.y}, 98.0f, pal::WithAlpha(stone_ring, 62));
    for (int ring = 0; ring < 3; ++ring) {
        const float base_r = 44.0f + static_cast<float>(ring) * 24.0f;
        const float ripple = std::sin(sim_time * 2.4f - static_cast<float>(ring) * 1.1f) * 2.0f * health;
        DrawRing({center.x, center.y}, base_r + ripple, base_r + 2.5f + ripple, 0.0f, 360.0f, 36, jade_inlay);
    }

    for (int i = 0; i < 8; ++i) {
        const float ang = static_cast<float>(i) * (eco::kPi / 4.0f);
        const float cx = center.x + std::cos(ang) * 78.0f;
        const float cy = center.y + std::sin(ang) * 78.0f;
        DrawPolyLines({cx, cy}, 4, 8.0f, ang * (180.0f / eco::kPi), jade_inlay);
        DrawCircleV({cx, cy}, 2.4f, pal::WithAlpha(pal::kJadeGlow, static_cast<unsigned char>(80.0f + 150.0f * health)));
    }
}

void DrawWindRoseMosaic(eco::Vec2 center, float poll, float sim_time) {
    const float health = 1.0f - poll;
    const Color slate_inlay = pal::WorldColor(Color{92, 92, 88, 175}, Color{176, 208, 226, 225}, poll);
    const Color bronze_inlay = pal::WorldColor(Color{104, 94, 78, 175}, Color{208, 178, 118, 220}, poll);

    DrawRing({center.x, center.y}, 60.0f, 64.0f, 0.0f, 360.0f, 32, slate_inlay);
    DrawRing({center.x, center.y}, 34.0f, 36.5f, 0.0f, 360.0f, 24, bronze_inlay);

    for (int i = 0; i < 8; ++i) {
        const float ang = static_cast<float>(i) * (eco::kPi / 4.0f);
        const bool major = (i % 2 == 0);
        const float len = major ? 70.0f : 50.0f;
        const Vector2 tip{center.x + std::cos(ang) * len, center.y + std::sin(ang) * len};
        const Vector2 left{center.x + std::cos(ang + 0.25f) * 16.0f, center.y + std::sin(ang + 0.25f) * 16.0f};
        const Vector2 right{center.x + std::cos(ang - 0.25f) * 16.0f, center.y + std::sin(ang - 0.25f) * 16.0f};
        DrawTriangleLines(tip, left, right, major ? bronze_inlay : slate_inlay);
    }

    if (health > 0.35f) {
        const float sweep = sim_time * 18.0f;
        DrawRing({center.x, center.y}, 64.0f, 67.0f, sweep, sweep + 75.0f, 16, pal::WithAlpha(slate_inlay, 150));
    }
}

void DrawSanctuaryGrandMosaic(eco::Vec2 center, float poll, float sim_time, int relics_found, bool all_seals_attuned) {
    const float health = 1.0f - poll;
    const Color marble = pal::WorldColor(Color{116, 108, 98, 185}, Color{216, 210, 192, 225}, poll);
    const Color bronze = pal::WorldColor(Color{128, 104, 70, 195}, Color{226, 188, 104, 245}, poll);
    const Color jade = pal::WorldColor(Color{92, 102, 86, 175}, Color{72, 232, 194, 245}, poll);

    DrawCircleV({center.x, center.y}, 148.0f, pal::WithAlpha(marble, 88));
    DrawRing({center.x, center.y}, 142.0f, 148.0f, 0.0f, 360.0f, 48, bronze);
    DrawRing({center.x, center.y}, 110.0f, 114.0f, 0.0f, 360.0f, 44, jade);
    DrawRing({center.x, center.y}, 76.0f, 79.0f, 0.0f, 360.0f, 36, bronze);
    DrawRing({center.x, center.y}, 42.0f, 44.5f, 0.0f, 360.0f, 28, jade);

    if (all_seals_attuned) {
        const float sweep = sim_time * 24.0f;
        DrawRing({center.x, center.y}, 114.0f, 119.0f, sweep, sweep + 120.0f, 24, pal::WithAlpha(pal::kJadeGlow, 175));
        DrawRing({center.x, center.y}, 114.0f, 119.0f, sweep + 180.0f, sweep + 300.0f, 24, pal::WithAlpha(pal::kAmberGlow, 165));
    }

    for (int i = 0; i < 16; ++i) {
        const float ang = static_cast<float>(i) * (eco::kPi / 8.0f);
        const bool cardinal = (i % 4 == 0);
        const float r0 = cardinal ? 44.0f : 114.0f;
        const float r1 = cardinal ? 148.0f : 142.0f;
        DrawLineEx(
            {center.x + std::cos(ang) * r0, center.y + std::sin(ang) * r0},
            {center.x + std::cos(ang) * r1, center.y + std::sin(ang) * r1},
            cardinal ? 2.6f : 1.4f,
            cardinal ? bronze : pal::WithAlpha(bronze, 180));
    }

    const int lit_eras = std::clamp(relics_found / 4, 0, 4);
    for (int era = 0; era < 4; ++era) {
        const float ang = static_cast<float>(era) * (eco::kPi * 0.5f) + (eco::kPi * 0.25f);
        const Vector2 mp{center.x + std::cos(ang) * 94.0f, center.y + std::sin(ang) * 94.0f};
        const bool lit = (era < lit_eras) || (health > 0.75f) || all_seals_attuned;
        DrawCircleV(mp, 12.0f, pal::WithAlpha(pal::kBasaltDark, 185));
        DrawCircleLines(static_cast<int>(mp.x), static_cast<int>(mp.y), 12.0f, bronze);
        if (lit) {
            const float pulse = 0.65f + 0.35f * std::sin(sim_time * 3.0f + static_cast<float>(era));
            DrawCircleV(mp, 7.5f, pal::WithAlpha(jade, static_cast<unsigned char>(180.0f * pulse)));
            DrawCircleV(mp, 3.5f, pal::kStoneIvory);
        } else {
            DrawCircleV(mp, 4.0f, pal::WithAlpha(bronze, 120));
        }
    }
}

// =============================================================================
// Flora Diversificata, Micro-Props & Affreschi Ambientali
// =============================================================================
void DrawBioCrystalNode(float x, float y, float sim_time, float poll, Color accent) {
    const float health = 1.0f - poll;
    const Color stem_col = pal::WorldColor(Color{70, 62, 54, 255}, Color{42, 118, 96, 255}, poll);
    const Color crystal_col = pal::WorldColor(Color{136, 86, 62, 225}, accent, poll);

    DrawEllipse(static_cast<int>(x + 2.0f), static_cast<int>(y + 11.0f), 14.0f, 5.5f, Color{0, 0, 0, 105});
    DrawPoly({x, y + 8.0f}, 6, 9.0f, 30.0f, pal::WorldColor(Color{72, 66, 60, 255}, Color{138, 142, 134, 255}, poll));
    DrawLineEx({x, y + 8.0f}, {x, y - 9.0f}, 3.5f, stem_col);
    DrawLineEx({x - 5.0f, y + 6.0f}, {x - 10.0f, y - 2.0f}, 2.4f, stem_col);
    DrawLineEx({x + 5.0f, y + 6.0f}, {x + 10.0f, y - 3.0f}, 2.4f, stem_col);

    const float pulse = std::sin(sim_time * 3.0f + x * 0.08f) * 2.0f * health;
    if (health > 0.22f) {
        DrawCircleV({x, y - 10.0f}, 15.0f + pulse, pal::WithAlpha(crystal_col, static_cast<unsigned char>(58.0f * health)));
    }
    DrawPoly({x, y - 10.0f}, 6, 9.0f + pulse, sim_time * 16.0f, crystal_col);
    DrawPolyLines({x, y - 10.0f}, 6, 9.0f + pulse, sim_time * 16.0f, pal::kStoneIvory);
    DrawPoly({x - 10.0f, y - 3.0f}, 5, 4.8f, -sim_time * 12.0f, crystal_col);
    DrawPoly({x + 10.0f, y - 4.0f}, 5, 4.8f, sim_time * 14.0f, crystal_col);
}

void DrawOliveTree(
    float x,
    float y,
    float scale,
    float sim_time,
    float poll,
    const pal::DistrictPalette& dp,
    int seed) {
    const float health = 1.0f - poll;
    const float sway = std::sin(sim_time * 1.4f + static_cast<float>(seed) * 1.9f) * (1.0f + 1.8f * health);

    DrawEllipse(
        static_cast<int>(x + 7.0f * scale),
        static_cast<int>(y + 12.0f * scale),
        22.0f * scale,
        8.5f * scale,
        Color{0, 0, 0, 108});

    const Color trunk_dark = pal::WorldColor(Color{48, 40, 36, 255}, Color{72, 54, 40, 255}, poll);
    const Color trunk_hi = pal::WorldColor(Color{74, 62, 52, 255}, Color{114, 88, 64, 255}, poll);
    DrawLineEx({x - 3.0f * scale, y + 11.0f * scale}, {x - 1.0f * scale + sway * 0.25f, y - 11.0f * scale}, 5.5f * scale, trunk_dark);
    DrawLineEx({x + 3.0f * scale, y + 11.0f * scale}, {x + 1.5f * scale + sway * 0.25f, y - 10.0f * scale}, 5.0f * scale, trunk_dark);
    DrawLineEx({x - 2.0f * scale, y + 9.0f * scale}, {x - 0.5f * scale + sway * 0.25f, y - 11.0f * scale}, 2.4f * scale, trunk_hi);
    DrawLineEx({x, y - 5.0f * scale}, {x - 13.0f * scale + sway * 0.45f, y - 15.0f * scale}, 3.2f * scale, trunk_dark);
    DrawLineEx({x, y - 6.0f * scale}, {x + 13.0f * scale + sway * 0.45f, y - 16.0f * scale}, 3.2f * scale, trunk_dark);

    const float r = (12.0f + 6.5f * health) * scale;
    const Color c_mid = pal::WorldColor(dp.canopy_sick, dp.canopy_healthy, poll);
    const Color c_hi = pal::WorldColor(dp.canopy_hi_sick, dp.canopy_hi_healthy, poll);
    const Color c_sh = pal::ShiftBrightness(c_mid, -26);

    const Vector2 c{x + sway, y - 19.0f * scale};
    DrawCircleV({c.x + 3.0f * scale, c.y + 3.5f * scale}, r * 0.98f, c_sh);
    DrawCircleV({c.x - 12.0f * scale, c.y + 3.0f * scale}, r * 0.72f, c_mid);
    DrawCircleV({c.x + 12.0f * scale, c.y + 2.5f * scale}, r * 0.74f, c_mid);
    DrawCircleV(c, r * 0.92f, c_mid);
    DrawCircleV({c.x - 6.0f * scale, c.y - 4.5f * scale}, r * 0.58f, c_hi);
    DrawCircleV({c.x + 5.0f * scale, c.y - 3.0f * scale}, r * 0.44f, c_hi);

    if (health > 0.45f) {
        const unsigned char alpha = static_cast<unsigned char>(std::clamp((health - 0.45f) * 430.0f, 0.0f, 235.0f));
        DrawCircleV({c.x - 9.0f * scale, c.y - 1.0f * scale}, 2.2f * scale, pal::WithAlpha(dp.blossom_a, alpha));
        DrawCircleV({c.x + 8.0f * scale, c.y + 1.0f * scale}, 2.0f * scale, pal::WithAlpha(dp.blossom_b, alpha));
        DrawCircleV({c.x + 1.0f * scale, c.y - 8.0f * scale}, 2.3f * scale, pal::WithAlpha(dp.blossom_a, alpha));
    }
}

void DrawWindCypress(
    float x,
    float y,
    float scale,
    float sim_time,
    float poll,
    const pal::DistrictPalette& dp,
    int seed,
    float wind_gust_bias = 0.0f) {
    const float health = 1.0f - poll;
    const float wind = 3.2f + wind_gust_bias * 7.5f +
                       std::sin(sim_time * (2.5f + wind_gust_bias * 3.0f) + static_cast<float>(seed) * 1.3f) *
                           (2.2f + 2.0f * health);

    DrawEllipse(
        static_cast<int>(x + 8.0f * scale),
        static_cast<int>(y + 11.0f * scale),
        15.0f * scale,
        5.5f * scale,
        Color{0, 0, 0, 105});

    const Color trunk = pal::WorldColor(Color{50, 44, 40, 255}, Color{70, 52, 38, 255}, poll);
    DrawLineEx({x, y + 11.0f * scale}, {x + wind * 0.4f, y - 16.0f * scale}, 4.5f * scale, trunk);

    const Color c_mid = pal::WorldColor(dp.canopy_sick, dp.canopy_healthy, poll);
    const Color c_hi = pal::WorldColor(dp.canopy_hi_sick, dp.canopy_hi_healthy, poll);
    const Color c_sh = pal::ShiftBrightness(c_mid, -22);

    for (int tier = 0; tier < 4; ++tier) {
        const float tf = static_cast<float>(tier) / 3.0f;
        const float ty = y - (8.0f + tf * 28.0f) * scale;
        const float tx = x + wind * (0.25f + tf * 0.85f);
        const float tr = (11.0f - tf * 5.2f + 2.5f * health) * scale;
        DrawCircleV({tx + 2.0f * scale, ty + 2.0f * scale}, tr, c_sh);
        DrawCircleV({tx, ty}, tr * 0.92f, c_mid);
        DrawCircleV({tx - 2.2f * scale, ty - 2.0f * scale}, tr * 0.52f, c_hi);
    }
}

void DrawFloweringGardenTree(
    float x,
    float y,
    float scale,
    float sim_time,
    float poll,
    const pal::DistrictPalette& dp,
    int seed) {
    const float health = 1.0f - poll;
    const float sway = std::sin(sim_time * 1.8f + static_cast<float>(seed) * 2.1f) * (1.2f + 2.4f * health);

    DrawEllipse(
        static_cast<int>(x + 6.0f * scale),
        static_cast<int>(y + 12.0f * scale),
        21.0f * scale,
        8.0f * scale,
        Color{0, 0, 0, 110});

    DrawRing({x, y + 9.0f * scale}, 7.0f * scale, 10.5f * scale, 0.0f, 360.0f, 16,
             pal::WorldColor(Color{92, 84, 74, 210}, Color{168, 166, 152, 230}, poll));
    const Color trunk = pal::WorldColor(Color{58, 46, 38, 255}, Color{94, 66, 46, 255}, poll);
    DrawLineEx({x, y + 9.0f * scale}, {x + sway * 0.35f, y - 15.0f * scale}, 5.5f * scale, trunk);
    DrawLineEx({x, y - 6.0f * scale}, {x - 10.0f * scale + sway * 0.5f, y - 16.0f * scale}, 2.8f * scale, trunk);
    DrawLineEx({x, y - 7.0f * scale}, {x + 10.0f * scale + sway * 0.5f, y - 17.0f * scale}, 2.8f * scale, trunk);

    const float r = (12.0f + 7.0f * health) * scale;
    const Color c_mid = pal::WorldColor(dp.canopy_sick, dp.canopy_healthy, poll);
    const Color c_hi = pal::WorldColor(dp.canopy_hi_sick, dp.canopy_hi_healthy, poll);
    const Color c_sh = pal::ShiftBrightness(c_mid, -28);

    const Vector2 c{x + sway, y - 21.0f * scale};
    DrawCircleV({c.x + 3.0f * scale, c.y + 3.0f * scale}, r, c_sh);
    DrawCircleV({c.x - 10.0f * scale, c.y + 4.0f * scale}, r * 0.78f, c_mid);
    DrawCircleV({c.x + 10.0f * scale, c.y + 4.0f * scale}, r * 0.76f, c_mid);
    DrawCircleV(c, r * 0.94f, c_mid);
    DrawCircleV({c.x - 4.0f * scale, c.y - 4.5f * scale}, r * 0.62f, c_hi);

    if (health > 0.30f) {
        const unsigned char alpha = static_cast<unsigned char>(std::clamp((health - 0.30f) * 350.0f, 0.0f, 245.0f));
        const Color b1 = pal::WithAlpha(dp.blossom_a, alpha);
        const Color b2 = pal::WithAlpha(dp.blossom_b, alpha);
        const Color b3 = pal::WithAlpha(pal::kJadeGlow, alpha);
        DrawCircleV({c.x - 9.0f * scale, c.y - 1.0f * scale}, 2.8f * scale, b1);
        DrawCircleV({c.x + 8.0f * scale, c.y + 2.0f * scale}, 2.6f * scale, b2);
        DrawCircleV({c.x + 2.0f * scale, c.y - 9.0f * scale}, 2.8f * scale, b1);
        DrawCircleV({c.x - 3.0f * scale, c.y + 6.0f * scale}, 2.4f * scale, b3);
        DrawCircleV({c.x + 11.0f * scale, c.y - 4.0f * scale}, 2.2f * scale, b2);
    }
}

void DrawBasaltThermalVent(float x, float y, float scale, float sim_time, float poll, int seed) {
    const float health = 1.0f - poll;
    DrawEllipse(static_cast<int>(x + 3.0f), static_cast<int>(y + 9.0f), 16.0f * scale, 6.5f * scale, Color{0, 0, 0, 115});

    const Color basalt_dark = pal::WorldColor(Color{50, 42, 40, 255}, Color{62, 68, 78, 255}, poll);
    const Color basalt_top = pal::WorldColor(Color{76, 64, 60, 255}, Color{98, 108, 120, 255}, poll);
    const Color glow = pal::WorldColor(Color{216, 76, 36, 200}, Color{88, 204, 196, 175}, poll);

    DrawPoly({x - 7.0f * scale, y + 3.0f * scale}, 6, 7.0f * scale, 30.0f, basalt_dark);
    DrawPoly({x + 7.0f * scale, y + 4.0f * scale}, 6, 6.5f * scale, 30.0f, basalt_dark);
    DrawPoly({x, y - 2.0f * scale}, 6, 9.5f * scale, 30.0f, basalt_top);
    DrawPolyLines({x, y - 2.0f * scale}, 6, 9.5f * scale, 30.0f, pal::ShiftBrightness(basalt_top, 24));

    DrawCircleV({x, y - 2.0f * scale}, 4.2f * scale, pal::WithAlpha(glow, 185));
    for (int i = 0; i < 3; ++i) {
        const float phase = std::fmod(sim_time * 0.7f + static_cast<float>(seed) * 0.37f + static_cast<float>(i) * 0.33f, 1.0f);
        const float vy = y - 6.0f * scale - phase * 26.0f * scale;
        const float vx = x + std::sin(sim_time * 2.2f + phase * 5.0f + static_cast<float>(seed)) * 4.5f * scale;
        const unsigned char va = static_cast<unsigned char>((1.0f - phase) * (health > 0.5f ? 110.0f : 135.0f));
        const Color wisp = pal::WorldColor(Color{132, 104, 92, va}, Color{216, 242, 246, va}, poll);
        DrawCircleV({vx, vy}, (3.0f + phase * 5.5f) * scale, wisp);
    }
}

void DrawJadeLanternPillar(float x, float y, float sim_time, float poll, Color accent) {
    const float health = 1.0f - poll;
    const float pulse = 0.75f + 0.25f * std::sin(sim_time * 2.8f + x * 0.05f);
    const Color stone = pal::WorldColor(Color{84, 76, 68, 255}, Color{154, 152, 140, 255}, poll);
    const Color light = pal::WorldColor(Color{228, 148, 62, 235}, accent, poll);

    DrawEllipse(static_cast<int>(x + 2.0f), static_cast<int>(y + 10.0f), 9.0f, 3.8f, Color{0, 0, 0, 95});
    DrawRectangle(static_cast<int>(x - 5.0f), static_cast<int>(y + 3.0f), 10, 8, stone);
    DrawRectangle(static_cast<int>(x - 3.0f), static_cast<int>(y - 10.0f), 6, 14, pal::ShiftBrightness(stone, 14));
    DrawRectangleLines(static_cast<int>(x - 5.0f), static_cast<int>(y - 16.0f), 10, 7, pal::kBronze);

    DrawCircleV({x, y - 13.0f}, (13.0f + 5.0f * health) * pulse,
                pal::WithAlpha(light, static_cast<unsigned char>((34.0f + 46.0f * health) * pulse)));
    DrawCircleV({x, y - 13.0f}, 3.6f, light);
    DrawCircleV({x, y - 13.0f}, 1.8f, pal::kStoneIvory);
}

void DrawBrokenColumnDrum(float x, float y, float poll, const pal::DistrictPalette& dp) {
    const Color top = pal::WorldColor(dp.wall_top_sick, dp.wall_top_healthy, poll);
    const Color front = pal::WorldColor(dp.wall_front_sick, dp.wall_front_healthy, poll);
    const Color edge = pal::WorldColor(dp.wall_edge_sick, dp.wall_edge_healthy, poll);

    DrawEllipse(static_cast<int>(x + 4.0f), static_cast<int>(y + 8.0f), 12.0f, 5.0f, Color{0, 0, 0, 95});
    DrawRectangle(static_cast<int>(x - 9.0f), static_cast<int>(y - 5.0f), 18, 12, front);
    DrawEllipse(static_cast<int>(x), static_cast<int>(y + 7.0f), 9.0f, 4.0f, front);
    DrawEllipse(static_cast<int>(x), static_cast<int>(y - 5.0f), 9.0f, 4.2f, top);
    DrawEllipseLines(static_cast<int>(x), static_cast<int>(y - 5.0f), 9.0f, 4.2f, edge);
    DrawLine(static_cast<int>(x - 5.0f), static_cast<int>(y - 3.0f), static_cast<int>(x - 5.0f), static_cast<int>(y + 7.0f), pal::ShiftBrightness(front, 20));
    DrawLine(static_cast<int>(x), static_cast<int>(y - 1.0f), static_cast<int>(x), static_cast<int>(y + 9.0f), pal::ShiftBrightness(front, 20));
    DrawLine(static_cast<int>(x + 5.0f), static_cast<int>(y - 3.0f), static_cast<int>(x + 5.0f), static_cast<int>(y + 7.0f), pal::ShiftBrightness(front, 20));
}

void DrawMeridianStatue(
    float x,
    float y,
    bool hydro_variant,
    float poll,
    float sim_time,
    const pal::DistrictPalette& dp) {
    const float health = 1.0f - poll;
    const Color stone = pal::WorldColor(dp.wall_top_sick, dp.wall_top_healthy, poll);
    const Color stone_dark = pal::WorldColor(dp.wall_front_sick, dp.wall_front_healthy, poll);
    const Color stone_hi = pal::WorldColor(dp.wall_edge_sick, dp.wall_edge_healthy, poll);

    DrawEllipse(static_cast<int>(x + 6.0f), static_cast<int>(y + 13.0f), 15.0f, 6.0f, Color{0, 0, 0, 108});
    DrawRectangle(static_cast<int>(x - 12.0f), static_cast<int>(y + 6.0f), 24, 7, stone_dark);
    DrawRectangle(static_cast<int>(x - 10.0f), static_cast<int>(y + 1.0f), 20, 6, stone);
    DrawLine(static_cast<int>(x - 10.0f), static_cast<int>(y + 1.0f), static_cast<int>(x + 10.0f), static_cast<int>(y + 1.0f), stone_hi);

    DrawTriangle({x - 7.0f, y + 1.0f}, {x + 7.0f, y + 1.0f}, {x, y - 20.0f}, stone);
    DrawTriangleLines({x - 7.0f, y + 1.0f}, {x + 7.0f, y + 1.0f}, {x, y - 20.0f}, stone_dark);
    DrawLineEx({x - 2.0f, y + 1.0f}, {x - 1.0f, y - 15.0f}, 1.6f, stone_hi);
    DrawCircleV({x, y - 22.0f}, 4.5f, stone);
    DrawCircleV({x - 1.0f, y - 23.0f}, 2.2f, stone_hi);

    if (hydro_variant) {
        DrawEllipse(static_cast<int>(x + 7.0f), static_cast<int>(y - 13.0f), 4.0f, 3.0f, pal::kBronze);
        const Color water = pal::WorldColor(dp.water_sick, dp.foam_healthy, poll);
        const float shimmer = std::sin(sim_time * 5.0f + x * 0.1f) * 1.4f;
        DrawLineEx({x + 9.0f, y - 11.0f}, {x + 10.0f + shimmer, y + 6.0f}, 2.2f, pal::WithAlpha(water, 215));
        if (health > 0.35f) {
            DrawEllipse(static_cast<int>(x + 10.0f), static_cast<int>(y + 8.0f), 5.0f, 2.2f, pal::WithAlpha(water, 145));
        }
    } else {
        const float pulse = 0.65f + 0.35f * std::sin(sim_time * 2.5f + x * 0.06f);
        const Vector2 orb{x, y - 31.0f};
        DrawCircleLines(static_cast<int>(orb.x), static_cast<int>(orb.y), 6.2f, pal::kBronzeBright);
        DrawEllipseLines(static_cast<int>(orb.x), static_cast<int>(orb.y), 6.5f, 2.8f, pal::kBronze);
        if (health > 0.25f) {
            DrawCircleV(orb, 2.4f, pal::WithAlpha(dp.ambient_accent, static_cast<unsigned char>(210.0f * pulse * health)));
        }
    }
}

void DrawStonePlanterUrn(
    float x,
    float y,
    float poll,
    float sim_time,
    const pal::DistrictPalette& dp) {
    const float health = 1.0f - poll;
    const Color stone = pal::WorldColor(dp.path_sick, dp.path_healthy, poll);
    const Color leaf = pal::WorldColor(dp.canopy_sick, dp.canopy_healthy, poll);
    const Color leaf_hi = pal::WorldColor(dp.canopy_hi_sick, dp.canopy_hi_healthy, poll);

    DrawEllipse(static_cast<int>(x + 2.0f), static_cast<int>(y + 8.0f), 11.0f, 4.2f, Color{0, 0, 0, 92});
    DrawRectangle(static_cast<int>(x - 7.0f), static_cast<int>(y), 14, 8, pal::ShiftBrightness(stone, -16));
    DrawEllipse(static_cast<int>(x), static_cast<int>(y), 9.5f, 4.2f, stone);
    DrawEllipseLines(static_cast<int>(x), static_cast<int>(y), 9.5f, 4.2f, pal::kBronze);

    const float sway = std::sin(sim_time * 2.0f + x * 0.08f) * 1.2f * health;
    DrawCircleV({x - 4.0f + sway, y - 4.0f}, 5.5f + 2.0f * health, leaf);
    DrawCircleV({x + 4.0f + sway, y - 3.5f}, 5.0f + 2.0f * health, leaf);
    DrawCircleV({x + sway, y - 7.0f}, 4.8f + 1.8f * health, leaf_hi);

    if (health > 0.30f) {
        const unsigned char fa = static_cast<unsigned char>(std::clamp((health - 0.30f) * 350.0f, 0.0f, 245.0f));
        DrawCircleV({x - 4.0f + sway, y - 6.0f}, 2.4f, pal::WithAlpha(dp.blossom_a, fa));
        DrawCircleV({x + 3.5f + sway, y - 7.5f}, 2.2f, pal::WithAlpha(dp.blossom_b, fa));
        DrawCircleV({x + sway, y - 2.5f}, 2.0f, pal::WithAlpha(dp.blossom_a, fa));
    }
}

void DrawSundialObelisk(
    float x,
    float y,
    float poll,
    float sim_time,
    const pal::DistrictPalette& dp) {
    const float health = 1.0f - poll;
    const Color stone = pal::WorldColor(dp.wall_top_sick, dp.wall_top_healthy, poll);
    const Color stone_dark = pal::WorldColor(dp.wall_front_sick, dp.wall_front_healthy, poll);
    const Color stone_hi = pal::WorldColor(dp.wall_edge_sick, dp.wall_edge_healthy, poll);

    DrawTriangle({x - 4.0f, y + 8.0f}, {x + 5.0f, y + 8.0f}, {x + 24.0f, y + 18.0f}, Color{0, 0, 0, 82});
    DrawRectangle(static_cast<int>(x - 7.0f), static_cast<int>(y + 4.0f), 14, 6, stone_dark);
    DrawTriangle({x - 5.0f, y + 4.0f}, {x + 5.0f, y + 4.0f}, {x, y - 28.0f}, stone);
    DrawTriangle({x, y + 4.0f}, {x + 5.0f, y + 4.0f}, {x, y - 28.0f}, stone_dark);
    DrawLineEx({x - 4.5f, y + 3.0f}, {x, y - 27.0f}, 1.5f, stone_hi);
    DrawTriangle({x - 2.6f, y - 20.0f}, {x + 2.6f, y - 20.0f}, {x, y - 29.0f}, pal::kBronzeBright);
    if (health > 0.40f) {
        const float glint = 0.55f + 0.45f * std::sin(sim_time * 3.2f + x * 0.05f);
        DrawCircleV({x, y - 28.0f}, 3.2f * glint, pal::WithAlpha(dp.ambient_accent, static_cast<unsigned char>(175.0f * health)));
    }
}

// Nuovi Affreschi Ambientali dedicati ai landmark narrativi delle 6 zone
void DrawAcousticJadeBasin(float x, float y, float poll, float sim_time, const pal::DistrictPalette& dp) {
    const float health = 1.0f - poll;
    const Color rim = pal::WorldColor(dp.wall_top_sick, dp.wall_top_healthy, poll);
    const Color water = pal::WorldColor(dp.water_sick, dp.water_healthy, poll);
    const float ripple = std::fmod(sim_time * 0.65f, 1.0f);

    DrawEllipse(static_cast<int>(x + 4.0f), static_cast<int>(y + 8.0f), 26.0f, 12.0f, Color{0, 0, 0, 95});
    DrawPoly({x, y}, 8, 22.0f, 22.5f, rim);
    DrawPolyLines({x, y}, 8, 22.0f, 22.5f, pal::kBronze);
    DrawPoly({x, y}, 8, 16.5f, 22.5f, water);
    DrawRing({x, y}, 4.0f + ripple * 11.0f, 5.4f + ripple * 11.0f, 0.0f, 360.0f, 24,
             pal::WithAlpha(pal::kJadeGlow, static_cast<unsigned char>((1.0f - ripple) * (110.0f + 120.0f * health))));
    DrawCircleV({x, y}, 3.2f, pal::kJadeGlow);
}

void DrawGeothermalBrazier(float x, float y, float poll, float sim_time) {
    const float pulse = 0.72f + 0.28f * std::sin(sim_time * 4.5f + x * 0.04f);
    const Color flame = pal::WorldColor(pal::kFumaroleErupt, pal::kAmberGlow, poll);

    DrawEllipse(static_cast<int>(x + 4.0f), static_cast<int>(y + 10.0f), 16.0f, 6.5f, Color{0, 0, 0, 115});
    DrawPoly({x, y + 2.0f}, 6, 13.0f, 30.0f, pal::kBasaltDark);
    DrawPolyLines({x, y + 2.0f}, 6, 13.0f, 30.0f, pal::kValveCopper);
    DrawCircleV({x, y - 2.0f}, 18.0f * pulse, pal::WithAlpha(flame, static_cast<unsigned char>(55.0f * pulse)));
    DrawCircleV({x, y - 2.0f}, 7.5f, pal::WithAlpha(flame, 225));
    DrawCircleV({x, y - 4.0f}, 4.0f * pulse, pal::kSolarBeamCore);
}

void DrawCarvedBasReliefPlaque(float x, float y, float poll, const pal::DistrictPalette& dp, const char* caption) {
    const Color stone = pal::WorldColor(dp.wall_top_sick, dp.wall_top_healthy, poll);
    const Color dark = pal::WorldColor(dp.wall_front_sick, dp.wall_front_healthy, poll);
    const Color inlay = pal::WorldColor(dp.inlay_sick, dp.inlay_healthy, poll);

    DrawRectangle(static_cast<int>(x - 16.0f), static_cast<int>(y + 6.0f), 34, 8, Color{0, 0, 0, 90});
    DrawRectangle(static_cast<int>(x - 16.0f), static_cast<int>(y - 14.0f), 32, 22, dark);
    DrawRectangle(static_cast<int>(x - 14.0f), static_cast<int>(y - 12.0f), 28, 18, stone);
    DrawRectangleLines(static_cast<int>(x - 14.0f), static_cast<int>(y - 12.0f), 28, 18, pal::kBronze);
    DrawLineEx({x - 9.0f, y - 6.0f}, {x + 9.0f, y - 6.0f}, 1.6f, inlay);
    DrawLineEx({x - 7.0f, y - 1.0f}, {x + 7.0f, y - 1.0f}, 1.4f, pal::WithAlpha(inlay, 190));
    DrawCircleV({x, y - 10.0f}, 2.2f, dp.ambient_accent);

    if (caption != nullptr && caption[0] != '\0') {
        DrawMeridianStele(x, y - 18.0f, caption, dp.ambient_accent, 1.0f - poll);
    }
}

// =============================================================================
// Rendering delle 6 Meccaniche Top-Down (Passata a Terra + Passata Elevata)
// =============================================================================
void DrawZoneMechanicsGround(const eco::WorldContext& world, eco::Vec2 cam, float half_w, float half_h) {
    const eco::ZoneId zone = world.overworld.active_zone;
    const eco::ZoneMechanicState& zm = world.zone_mechanics;
    const float sim_time = world.sim_time;
    const float poll = world.ecosystem.PollutionRatio();
    const float health = 1.0f - poll;
    const auto& env_cfg = GetEnvironmentEffectsConfig();
    const float weather_scale = env_cfg.atmosphere_enabled ? env_cfg.zone_weather_intensity : 0.35f;

    if (zone == eco::ZoneId::DawnTerraces) {
        // Piastre fotosensibili (Terrazze dell'Alba)
        for (const auto& plate : zm.solar_plates) {
            if (std::fabs(plate.pos.x - cam.x) > half_w || std::fabs(plate.pos.y - cam.y) > half_h) {
                continue;
            }
            const Color base = plate.lit ? pal::kSolarPlateLit : pal::kSolarPlateIdle;
            const float pulse = 0.72f + 0.28f * std::sin(sim_time * 4.0f + static_cast<float>(plate.id));

            DrawPoly({plate.pos.x, plate.pos.y}, 8, plate.radius + 3.0f, 22.5f, pal::kBronzeDark);
            DrawPoly({plate.pos.x, plate.pos.y}, 8, plate.radius - 1.0f, 22.5f, base);
            DrawPolyLines({plate.pos.x, plate.pos.y}, 8, plate.radius + 3.0f, 22.5f, pal::kBronzeBright);

            // Anello di carica fotosensibile [0..1]
            if (plate.charge > 0.01f) {
                DrawRing({plate.pos.x, plate.pos.y}, plate.radius - 6.0f, plate.radius - 1.5f,
                         -90.0f, -90.0f + 360.0f * std::clamp(plate.charge, 0.0f, 1.0f), 28,
                         pal::kSolarBeamCore);
            }
            // Glifo solare centrale
            for (int r = 0; r < 8; ++r) {
                const float ang = static_cast<float>(r) * (eco::kPi * 0.25f) + (plate.lit ? sim_time * 0.7f : 0.0f);
                DrawLineEx(
                    {plate.pos.x + std::cos(ang) * 6.0f, plate.pos.y + std::sin(ang) * 6.0f},
                    {plate.pos.x + std::cos(ang) * (plate.radius * 0.58f), plate.pos.y + std::sin(ang) * (plate.radius * 0.58f)},
                    1.8f,
                    plate.lit ? pal::kSolarBeamCore : pal::kBronze);
            }
            if (plate.lit) {
                DrawCircleV({plate.pos.x, plate.pos.y}, plate.radius * 1.45f * pulse,
                            pal::WithAlpha(pal::kSolarBeamGlow, static_cast<unsigned char>(85.0f * pulse)));
                DrawCircleV({plate.pos.x, plate.pos.y}, 7.0f, pal::kSolarBeamCore);
            } else {
                DrawCircleV({plate.pos.x, plate.pos.y}, 5.5f, pal::kBronzeDark);
            }
        }
    } else if (zone == eco::ZoneId::CanalQuarter) {
        // Correnti d'acqua direzionali animate (Quartiere dei Canali)
        for (const auto& cur : zm.water_currents) {
            const float cx = cur.bounds.x + cur.bounds.width * 0.5f;
            const float cy = cur.bounds.y + cur.bounds.height * 0.5f;
            if (std::fabs(cx - cam.x) > half_w + cur.bounds.width ||
                std::fabs(cy - cam.y) > half_h + cur.bounds.height) {
                continue;
            }
            const eco::Vec2 dir = cur.flow_dir.Normalized();
            if (dir.LengthSquared() < 0.01f) {
                continue;
            }
            const eco::Vec2 perp{-dir.y, dir.x};
            const Color arrow_col = cur.active
                                        ? pal::WithAlpha(pal::kCurrentArrow, static_cast<unsigned char>(150.0f + 70.0f * health))
                                        : pal::WithAlpha(pal::kCurrentFoam, 68);
            const float speed = cur.active ? 54.0f : 10.0f;
            const int cols = std::max(1, static_cast<int>(cur.bounds.width / 44.0f));
            const int rows = std::max(1, static_cast<int>(cur.bounds.height / 44.0f));
            const float span = std::max(cur.bounds.width, cur.bounds.height);

            for (int r = 0; r < rows; ++r) {
                for (int c = 0; c < cols; ++c) {
                    const float base_x = cur.bounds.x + (static_cast<float>(c) + 0.5f) * (cur.bounds.width / static_cast<float>(cols));
                    const float base_y = cur.bounds.y + (static_cast<float>(r) + 0.5f) * (cur.bounds.height / static_cast<float>(rows));
                    const float shift = std::fmod(sim_time * speed + static_cast<float>((r + c) * 19), 38.0f) - 19.0f;
                    const float ax = std::clamp(base_x + dir.x * shift, cur.bounds.x + 8.0f, cur.bounds.Right() - 8.0f);
                    const float ay = std::clamp(base_y + dir.y * shift, cur.bounds.y + 8.0f, cur.bounds.Bottom() - 8.0f);

                    // Freccia a chevron idraulico super leggibile
                    const Vector2 tip{ax + dir.x * 9.0f, ay + dir.y * 9.0f};
                    const Vector2 wing_l{ax - dir.x * 5.0f + perp.x * 7.5f, ay - dir.y * 5.0f + perp.y * 7.5f};
                    const Vector2 wing_r{ax - dir.x * 5.0f - perp.x * 7.5f, ay - dir.y * 5.0f - perp.y * 7.5f};
                    DrawLineEx(wing_l, tip, cur.active ? 2.4f : 1.4f, arrow_col);
                    DrawLineEx(wing_r, tip, cur.active ? 2.4f : 1.4f, arrow_col);
                    if (cur.active && span > 64.0f) {
                        DrawLineEx({ax - dir.x * 12.0f, ay - dir.y * 12.0f}, {ax + dir.x * 3.0f, ay + dir.y * 3.0f},
                                   1.6f, pal::WithAlpha(pal::kCurrentFoam, 115));
                    }
                }
            }
        }
    } else if (zone == eco::ZoneId::WindPlateau) {
        // Zone d'ombra frangivento sottovento dietro ciascun monolite (Altopiano dei Venti)
        const bool show_shelter = zm.wind.telegraphing || zm.wind.gust_active || zm.wind.gust_intensity > 0.08f;
        const eco::Vec2 wdir = zm.wind.wind_dir.Normalized();
        const eco::Vec2 wperp{-wdir.y, wdir.x};
        const float shelter_alpha = show_shelter ? (0.45f + 0.55f * std::max(zm.wind.gust_intensity, zm.wind.telegraphing ? 0.55f : 0.0f)) : 0.18f;

        for (const auto& wb : zm.windbreaks) {
            if (std::fabs(wb.center.x - cam.x) > half_w + wb.shelter_distance ||
                std::fabs(wb.center.y - cam.y) > half_h + wb.shelter_distance) {
                continue;
            }
            const float half_span = std::max(wb.bounds.width, wb.bounds.height) * 0.55f;
            const Vector2 p0{wb.center.x + wperp.x * half_span, wb.center.y + wperp.y * half_span};
            const Vector2 p1{wb.center.x - wperp.x * half_span, wb.center.y - wperp.y * half_span};
            const Vector2 p2{wb.center.x + wdir.x * wb.shelter_distance - wperp.x * (half_span * 0.72f),
                             wb.center.y + wdir.y * wb.shelter_distance - wperp.y * (half_span * 0.72f)};
            const Vector2 p3{wb.center.x + wdir.x * wb.shelter_distance + wperp.x * (half_span * 0.72f),
                             wb.center.y + wdir.y * wb.shelter_distance + wperp.y * (half_span * 0.72f)};

            const Color fill = pal::WithAlpha(pal::kWindbreakShelter, static_cast<unsigned char>(58.0f * shelter_alpha));
            const Color rim = pal::WithAlpha(pal::kJadeGlow, static_cast<unsigned char>(135.0f * shelter_alpha));
            DrawTriangle(p0, p1, p2, fill);
            DrawTriangle(p0, p2, p3, fill);
            DrawLineEx(p0, p3, 1.8f, rim);
            DrawLineEx(p1, p2, 1.8f, rim);
            DrawLineEx(p3, p2, 1.6f, pal::WithAlpha(rim, static_cast<unsigned char>(100.0f * shelter_alpha)));
        }
    } else if (zone == eco::ZoneId::HangingGardens) {
        // Piastre di risonanza armonica (Giardini Sospesi)
        for (const auto& pad : zm.resonance_pads) {
            if (std::fabs(pad.pos.x - cam.x) > half_w || std::fabs(pad.pos.y - cam.y) > half_h) {
                continue;
            }
            const Color note_col = pal::ResonanceOrderColor(pad.sequence_order);
            const bool is_next = !zm.jade_bridge_unlocked && !pad.activated && (pad.sequence_order == zm.resonance_next_order);
            const float pulse = 0.5f + 0.5f * std::sin(sim_time * 3.8f + static_cast<float>(pad.sequence_order));

            DrawCircleV({pad.pos.x, pad.pos.y}, pad.radius + 3.0f, pal::kBasaltDark);
            DrawCircleV({pad.pos.x, pad.pos.y}, pad.radius, pad.activated ? pal::LerpColor(pal::kResonancePadIdle, note_col, 0.55f) : pal::kResonancePadIdle);
            DrawRing({pad.pos.x, pad.pos.y}, pad.radius - 3.5f, pad.radius + 1.0f, 0.0f, 360.0f, 28,
                     pad.activated ? note_col : pal::kBronze);
            DrawRing({pad.pos.x, pad.pos.y}, pad.radius * 0.45f, pad.radius * 0.54f, 0.0f, 360.0f, 24,
                     pal::WithAlpha(note_col, pad.activated ? 235 : 135));

            // Pallini numerati 1..4 per comunicare chiaramente l'ordine della sequenza
            const int dots = pad.sequence_order + 1;
            const float spacing = 8.0f;
            const float start_x = pad.pos.x - static_cast<float>(dots - 1) * spacing * 0.5f;
            for (int d = 0; d < dots; ++d) {
                DrawCircleV({start_x + static_cast<float>(d) * spacing, pad.pos.y},
                            3.0f,
                            pad.activated ? pal::kStoneIvory : note_col);
            }

            // Suggerimento discreto sulla prossima piastra della sequenza
            if (is_next) {
                DrawRing({pad.pos.x, pad.pos.y}, pad.radius + 3.0f + pulse * 6.0f, pad.radius + 5.5f + pulse * 6.0f,
                         0.0f, 360.0f, 28, pal::WithAlpha(note_col, static_cast<unsigned char>(165.0f * (1.0f - pulse * 0.5f))));
            }
            // Onda acustica espansiva al passaggio del player
            if (pad.pulse_timer > 0.01f) {
                const float wave_t = std::clamp(1.0f - pad.pulse_timer / 0.85f, 0.0f, 1.0f);
                const float wr = pad.radius + wave_t * 48.0f;
                DrawRing({pad.pos.x, pad.pos.y}, wr, wr + 3.0f, 0.0f, 360.0f, 32,
                         pal::WithAlpha(note_col, static_cast<unsigned char>((1.0f - wave_t) * 220.0f)));
            }
        }
    } else if (zone == eco::ZoneId::FumaroleCaldera) {
        // Bocche fumarole con telegraphing termico super leggibile (Caldera delle Fumarole)
        for (const auto& g : zm.geysers) {
            if (std::fabs(g.pos.x - cam.x) > half_w + g.radius || std::fabs(g.pos.y - cam.y) > half_h + g.radius) {
                continue;
            }
            // Cratere esagonale in basalto
            DrawPoly({g.pos.x, g.pos.y}, 6, 22.0f, 30.0f, pal::kBasaltDark);
            DrawPoly({g.pos.x, g.pos.y}, 6, 17.0f, 30.0f, pal::kFumaroleDormant);
            DrawPolyLines({g.pos.x, g.pos.y}, 6, 22.0f, 30.0f, pal::kValveCopper);

            if (g.phase == eco::GeyserPhase::Calmed) {
                DrawCircleV({g.pos.x, g.pos.y}, 11.0f, pal::WithAlpha(pal::kFumaroleCalmed, 165));
                DrawRing({g.pos.x, g.pos.y}, 14.0f, 17.0f, 0.0f, 360.0f, 24, pal::WithAlpha(pal::kJadeGlow, 150));
            } else if (g.phase == eco::GeyserPhase::Telegraphing) {
                const float warn = std::clamp(g.intensity, 0.15f, 1.0f);
                const float jitter = std::sin(sim_time * 28.0f + static_cast<float>(g.id) * 2.0f) * 2.5f;
                const float inner_r = g.radius * warn;
                DrawCircleV({g.pos.x, g.pos.y}, g.radius, pal::WithAlpha(pal::kFumaroleTelegraph, static_cast<unsigned char>(36.0f + 44.0f * warn)));
                DrawRing({g.pos.x, g.pos.y}, g.radius - 2.5f, g.radius + 1.0f, 0.0f, 360.0f, 32,
                         pal::WithAlpha(pal::kFumaroleTelegraph, static_cast<unsigned char>(170.0f + 70.0f * warn)));
                DrawRing({g.pos.x + jitter * 0.4f, g.pos.y}, std::max(6.0f, inner_r - 3.0f), inner_r, 0.0f, 360.0f, 28,
                         pal::WithAlpha(pal::kFumaroleErupt, 210));
                DrawCircleV({g.pos.x, g.pos.y}, 10.0f + warn * 4.0f, pal::kFumaroleTelegraph);
            } else if (g.phase == eco::GeyserPhase::Erupting) {
                const float erupt = std::clamp(g.intensity, 0.4f, 1.0f);
                DrawCircleV({g.pos.x, g.pos.y}, g.radius * (0.95f + 0.08f * std::sin(sim_time * 18.0f)),
                            pal::WithAlpha(pal::kFumaroleErupt, static_cast<unsigned char>(95.0f * erupt * (0.5f + 0.5f * weather_scale))));
                DrawRing({g.pos.x, g.pos.y}, g.radius - 3.0f, g.radius + 2.0f, 0.0f, 360.0f, 32, pal::kFumaroleErupt);
                DrawCircleV({g.pos.x, g.pos.y}, 13.0f, pal::kSolarBeamCore);
            } else {
                // Dormant: brace quieta
                const float ember = 0.5f + 0.5f * std::sin(sim_time * 2.0f + static_cast<float>(g.id));
                DrawCircleV({g.pos.x, g.pos.y}, 7.0f, pal::WithAlpha(pal::kFumaroleErupt, static_cast<unsigned char>(75.0f + 45.0f * ember)));
            }
        }
    } else if (zone == eco::ZoneId::HarmonySanctuary) {
        // 5 Sigilli Armonici radiali + linee geodetiche verso l'Astrolabio Centrale
        const eco::Machine& astro = world.GetMachine(eco::MachineType::CentralAstrolabe);
        for (std::size_t i = 0; i < zm.sanctuary_seals.size(); ++i) {
            const auto& seal = zm.sanctuary_seals[i];
            if (std::fabs(seal.pos.x - cam.x) > half_w + 80.0f || std::fabs(seal.pos.y - cam.y) > half_h + 80.0f) {
                continue;
            }
            const Color col = pal::SanctuarySealColor(static_cast<int>(i));
            const float g = std::clamp(seal.glow, 0.0f, 1.0f);
            const float pulse = 0.7f + 0.3f * std::sin(sim_time * 3.0f + static_cast<float>(i));

            // Linea armonica che collega il sigillo all'Astrolabio Centrale
            DrawLineEx(
                {seal.pos.x, seal.pos.y},
                {astro.pos.x, astro.pos.y},
                seal.attuned ? 4.2f : 1.6f,
                pal::WithAlpha(col, static_cast<unsigned char>(seal.attuned ? (140.0f + 95.0f * pulse) : 52.0f)));
            if (seal.attuned) {
                DrawLineEx({seal.pos.x, seal.pos.y}, {astro.pos.x, astro.pos.y}, 1.8f, pal::WithAlpha(pal::kStoneIvory, 210));
            }

            // Medaglione del sigillo
            DrawCircleV({seal.pos.x, seal.pos.y}, seal.radius, pal::WithAlpha(pal::kBasaltDark, 195));
            DrawRing({seal.pos.x, seal.pos.y}, seal.radius - 4.0f, seal.radius, 0.0f, 360.0f, 32,
                     seal.attuned ? col : pal::kBronze);
            DrawPolyLines({seal.pos.x, seal.pos.y}, 6, seal.radius * 0.65f, sim_time * (seal.attuned ? 22.0f : 5.0f),
                          pal::WithAlpha(col, static_cast<unsigned char>(115.0f + 135.0f * g)));
            if (seal.attuned) {
                DrawCircleV({seal.pos.x, seal.pos.y}, seal.radius * 1.15f * pulse,
                            pal::WithAlpha(col, static_cast<unsigned char>(62.0f * pulse)));
                DrawCircleV({seal.pos.x, seal.pos.y}, 8.5f, pal::kStoneIvory);
            } else {
                DrawCircleV({seal.pos.x, seal.pos.y}, 5.5f, pal::WithAlpha(col, 145));
            }
        }
    }
}

void DrawZoneMechanicsElevated(const eco::WorldContext& world, eco::Vec2 cam, float half_w, float half_h) {
    const eco::ZoneId zone = world.overworld.active_zone;
    const eco::ZoneMechanicState& zm = world.zone_mechanics;
    const float sim_time = world.sim_time;
    const float poll = world.ecosystem.PollutionRatio();
    const auto& env_cfg = GetEnvironmentEffectsConfig();
    const float sun_scale = env_cfg.atmosphere_enabled ? std::clamp(env_cfg.sun_rays_intensity, 0.25f, 1.25f) : 0.45f;
    const float weather_scale = env_cfg.atmosphere_enabled ? std::clamp(env_cfg.zone_weather_intensity, 0.2f, 1.25f) : 0.35f;

    if (zone == eco::ZoneId::DawnTerraces) {
        // 1. Emettitore Eliostatico Solare
        const eco::Vec2 ep = zm.solar_emitter_pos;
        if (std::fabs(ep.x - cam.x) <= half_w && std::fabs(ep.y - cam.y) <= half_h) {
            const float pulse = 0.78f + 0.22f * std::sin(sim_time * 3.5f);
            DrawEllipse(static_cast<int>(ep.x + 3.0f), static_cast<int>(ep.y + 12.0f), 19.0f, 8.0f, Color{0, 0, 0, 110});
            DrawPoly({ep.x, ep.y}, 8, 17.0f, 22.5f, pal::kBronzeDark);
            DrawPoly({ep.x, ep.y}, 8, 13.0f, 22.5f, pal::kBronze);
            DrawCircleV({ep.x, ep.y}, 15.0f * pulse, pal::WithAlpha(pal::kSolarBeamGlow, static_cast<unsigned char>(110.0f * sun_scale)));
            DrawCircleV({ep.x, ep.y}, 6.5f, pal::kSolarBeamCore);
        }

        // 2. Fasci Solari luminosi (multi-strato + particelle di luce che scorrono lungo il raggio)
        for (const auto& seg : zm.solar_beams) {
            if (!seg.active) {
                continue;
            }
            const Vector2 a{seg.start.x, seg.start.y};
            const Vector2 b{seg.end.x, seg.end.y};
            DrawLineEx(a, b, 14.0f, pal::WithAlpha(pal::kSolarBeamGlow, static_cast<unsigned char>(48.0f * sun_scale)));
            DrawLineEx(a, b, 6.5f, pal::WithAlpha(pal::kAmberGlow, static_cast<unsigned char>(165.0f * sun_scale)));
            DrawLineEx(a, b, 2.8f, pal::kSolarBeamCore);

            const eco::Vec2 delta = seg.end - seg.start;
            const float len = delta.Length();
            if (len > 24.0f) {
                const int motes = std::clamp(static_cast<int>(len / 68.0f), 1, 8);
                for (int m = 0; m < motes; ++m) {
                    const float t = std::fmod(sim_time * 0.65f + static_cast<float>(m) / static_cast<float>(motes), 1.0f);
                    const Vector2 mp{seg.start.x + delta.x * t, seg.start.y + delta.y * t};
                    DrawCircleV(mp, 3.6f, pal::WithAlpha(pal::kSolarBeamCore, 230));
                }
            }
            DrawCircleV(b, 7.5f, pal::WithAlpha(pal::kSolarBeamGlow, static_cast<unsigned char>(135.0f * sun_scale)));
        }

        // 3. Specchi Solari orientabili [E]
        for (const auto& m : zm.solar_mirrors) {
            if (std::fabs(m.pos.x - cam.x) > half_w || std::fabs(m.pos.y - cam.y) > half_h) {
                continue;
            }
            const eco::Vec2 dir = MirrorDirVector(m.dir_index);
            const eco::Vec2 perp{-dir.y, dir.x};
            const bool near_player = NearestActivePlayerDistance(world, m.pos) <= m.interact_radius;

            DrawEllipse(static_cast<int>(m.pos.x + 3.0f), static_cast<int>(m.pos.y + 11.0f), 16.0f, 6.5f, Color{0, 0, 0, 108});
            DrawPoly({m.pos.x, m.pos.y}, 8, 15.0f, 22.5f, pal::kBronzeDark);
            DrawPoly({m.pos.x, m.pos.y}, 8, 11.5f, 22.5f, pal::kBronze);

            // Lama dello specchio orientata + freccia direzione raggio riflesso
            const Vector2 blade_a{m.pos.x + perp.x * 13.0f - dir.x * 4.0f, m.pos.y + perp.y * 13.0f - dir.y * 4.0f};
            const Vector2 blade_b{m.pos.x - perp.x * 13.0f - dir.x * 4.0f, m.pos.y - perp.y * 13.0f - dir.y * 4.0f};
            DrawLineEx(blade_a, blade_b, 5.5f, pal::kBronzeBright);
            DrawLineEx(blade_a, blade_b, 2.8f, pal::kMirrorSilver);

            // Punta emissiva nella direzione attiva del raggio
            const Vector2 nozzle{m.pos.x + dir.x * 14.0f, m.pos.y + dir.y * 14.0f};
            DrawLineEx({m.pos.x, m.pos.y}, nozzle, 3.0f, pal::kSolarBeamCore);
            DrawCircleV(nozzle, 3.6f, pal::kAmberGlow);

            if (near_player && m.rotatable) {
                const float pulse = 0.65f + 0.35f * std::sin(sim_time * 6.0f);
                DrawRing({m.pos.x, m.pos.y}, 21.0f, 23.5f, 0.0f, 360.0f, 24,
                         pal::WithAlpha(pal::kSolarBeamCore, static_cast<unsigned char>(195.0f * pulse)));
            }
        }
    } else if (zone == eco::ZoneId::CanalQuarter) {
        // Leve / Chiuse Idrauliche [E]
        for (const auto& lev : zm.sluice_levers) {
            if (std::fabs(lev.pos.x - cam.x) > half_w || std::fabs(lev.pos.y - cam.y) > half_h) {
                continue;
            }
            const bool near_player = NearestActivePlayerDistance(world, lev.pos) <= lev.interact_radius;
            const Color state_col = lev.pulled ? pal::kSluiceOpen : pal::kSluiceClosed;

            DrawEllipse(static_cast<int>(lev.pos.x + 3.0f), static_cast<int>(lev.pos.y + 10.0f), 16.0f, 6.5f, Color{0, 0, 0, 108});
            DrawRectangle(static_cast<int>(lev.pos.x - 13.0f), static_cast<int>(lev.pos.y - 6.0f), 26, 16, pal::kBasaltMid);
            DrawRectangleLines(static_cast<int>(lev.pos.x - 13.0f), static_cast<int>(lev.pos.y - 6.0f), 26, 16, pal::kBronze);

            // Braccio della leva inclinato a sinistra o destra
            const float tilt = lev.pulled ? 12.0f : -12.0f;
            DrawLineEx({lev.pos.x, lev.pos.y}, {lev.pos.x + tilt, lev.pos.y - 20.0f}, 4.0f, pal::kBronzeBright);
            DrawCircleV({lev.pos.x + tilt, lev.pos.y - 21.0f}, 5.5f, state_col);
            DrawCircleV({lev.pos.x + tilt, lev.pos.y - 21.0f}, 2.4f, pal::kStoneIvory);
            DrawCircleV({lev.pos.x, lev.pos.y + 3.0f}, 3.5f, state_col);

            if (near_player) {
                const float pulse = 0.65f + 0.35f * std::sin(sim_time * 6.0f);
                DrawRing({lev.pos.x, lev.pos.y}, 21.0f, 23.5f, 0.0f, 360.0f, 24,
                         pal::WithAlpha(state_col, static_cast<unsigned char>(205.0f * pulse)));
            }
        }
    } else if (zone == eco::ZoneId::WindPlateau) {
        // 1. Monoliti Frangivento 2.5D scolpiti a spirale
        const pal::DistrictPalette& dp = pal::GetDistrictPalette(zone);
        const Color m_top = pal::WorldColor(dp.wall_top_sick, pal::kWindMonolithStone, poll);
        const Color m_front = pal::WorldColor(dp.wall_front_sick, dp.wall_front_healthy, poll);
        const Color m_edge = pal::WorldColor(dp.wall_edge_sick, dp.wall_edge_healthy, poll);

        for (const auto& wb : zm.windbreaks) {
            if (std::fabs(wb.center.x - cam.x) > half_w + 64.0f || std::fabs(wb.center.y - cam.y) > half_h + 64.0f) {
                continue;
            }
            const int x = static_cast<int>(wb.bounds.x);
            const int y = static_cast<int>(wb.bounds.y);
            const int w = static_cast<int>(wb.bounds.width);
            const int h = static_cast<int>(wb.bounds.height);

            // Ombra portata
            DrawRectangle(x + 6, y + 10, w, h, Color{0, 0, 0, 95});
            // Corpo 2.5D elevato sopra il terreno
            DrawRectangle(x, y - 14, w, h, m_top);
            DrawRectangle(x, y + h - 14, w, 14, m_front);
            DrawRectangleLines(x, y - 14, w, h + 14, m_edge);
            // Fregio a spirale del vento in bronzo
            DrawRing({wb.center.x, wb.center.y - 10.0f}, 7.0f, 10.0f, 0.0f, 300.0f, 18, pal::kBronzeBright);
            DrawCircleV({wb.center.x, wb.center.y - 10.0f}, 3.2f, pal::kWindStreak);
        }

        // 2. Linee di raffica / telegraphing del vento nel viewport
        if (zm.wind.telegraphing || zm.wind.gust_active || zm.wind.gust_intensity > 0.08f) {
            const eco::Vec2 wdir = zm.wind.wind_dir.Normalized();
            const float inten = zm.wind.gust_active ? std::clamp(zm.wind.gust_intensity, 0.3f, 1.0f) : 0.45f;
            const Color streak_col = zm.wind.gust_active
                                         ? pal::WithAlpha(pal::kWindStreak, static_cast<unsigned char>(150.0f * inten * weather_scale))
                                         : pal::WithAlpha(pal::kWindTelegraph, static_cast<unsigned char>(120.0f * weather_scale));
            const int streak_count = zm.wind.gust_active ? 18 : 9;
            const float speed = zm.wind.gust_active ? 460.0f : 190.0f;

            for (int i = 0; i < streak_count; ++i) {
                const float seed_x = Hash2Float01(i, 11, 701) * (half_w * 2.0f) - half_w;
                const float seed_y = Hash2Float01(i, 29, 709) * (half_h * 2.0f) - half_h;
                const float travel = std::fmod(sim_time * speed + static_cast<float>(i) * 97.0f, half_w * 2.0f);
                const float sx = cam.x + std::fmod(seed_x + wdir.x * travel + half_w * 3.0f, half_w * 2.0f) - half_w;
                const float sy = cam.y + std::fmod(seed_y + wdir.y * travel + half_h * 3.0f, half_h * 2.0f) - half_h;
                const float len = zm.wind.gust_active ? (48.0f + 46.0f * inten) : 34.0f;
                DrawLineEx({sx, sy}, {sx + wdir.x * len, sy + wdir.y * len}, zm.wind.gust_active ? 2.4f : 1.6f, streak_col);
            }
        }
    } else if (zone == eco::ZoneId::HangingGardens) {
        // Scintille/note armoniche verticali sopra le piastre attive
        for (const auto& pad : zm.resonance_pads) {
            if (!pad.activated && pad.pulse_timer <= 0.01f) {
                continue;
            }
            if (std::fabs(pad.pos.x - cam.x) > half_w || std::fabs(pad.pos.y - cam.y) > half_h) {
                continue;
            }
            const Color col = pal::ResonanceOrderColor(pad.sequence_order);
            const float lift = std::fmod(sim_time * 24.0f + static_cast<float>(pad.sequence_order * 9), 26.0f);
            DrawEllipseLines(static_cast<int>(pad.pos.x), static_cast<int>(pad.pos.y - lift),
                             14.0f, 6.0f, pal::WithAlpha(col, static_cast<unsigned char>((1.0f - lift / 26.0f) * 195.0f)));
        }
    } else if (zone == eco::ZoneId::FumaroleCaldera) {
        // 1. Pennacchi di vapore / getti termici dei geyser
        for (const auto& g : zm.geysers) {
            if (std::fabs(g.pos.x - cam.x) > half_w + 64.0f || std::fabs(g.pos.y - cam.y) > half_h + 64.0f) {
                continue;
            }
            if (g.phase == eco::GeyserPhase::Erupting) {
                const float erupt = std::clamp(g.intensity, 0.4f, 1.0f);
                for (int p = 0; p < 5; ++p) {
                    const float phase = std::fmod(sim_time * 2.4f + static_cast<float>(p) * 0.2f + static_cast<float>(g.id) * 0.13f, 1.0f);
                    const float py = g.pos.y - 6.0f - phase * 62.0f * erupt;
                    const float px = g.pos.x + std::sin(sim_time * 8.0f + phase * 6.0f) * 8.0f;
                    const float r = (10.0f + phase * 18.0f) * erupt;
                    const unsigned char alpha = static_cast<unsigned char>((1.0f - phase) * 185.0f * weather_scale);
                    const Color puff = pal::LerpColor(pal::kFumaroleErupt, Color{238, 224, 212, 255}, phase);
                    DrawCircleV({px, py}, r, pal::WithAlpha(puff, alpha));
                }
            } else if (g.phase == eco::GeyserPhase::Telegraphing) {
                for (int p = 0; p < 2; ++p) {
                    const float phase = std::fmod(sim_time * 1.4f + static_cast<float>(p) * 0.5f, 1.0f);
                    const float py = g.pos.y - 4.0f - phase * 24.0f;
                    DrawCircleV({g.pos.x, py}, 5.0f + phase * 7.0f,
                                pal::WithAlpha(pal::kFumaroleTelegraph, static_cast<unsigned char>((1.0f - phase) * 120.0f * weather_scale)));
                }
            }
        }

        // 2. Valvole di sfogo in rame ossidato [E]
        for (const auto& v : zm.relief_valves) {
            if (std::fabs(v.pos.x - cam.x) > half_w || std::fabs(v.pos.y - cam.y) > half_h) {
                continue;
            }
            const bool near_player = NearestActivePlayerDistance(world, v.pos) <= v.interact_radius;
            const Color state_col = v.open ? pal::kFumaroleCalmed : pal::kFumaroleTelegraph;

            DrawEllipse(static_cast<int>(v.pos.x + 3.0f), static_cast<int>(v.pos.y + 10.0f), 15.0f, 6.0f, Color{0, 0, 0, 115});
            DrawPoly({v.pos.x, v.pos.y + 2.0f}, 6, 14.0f, 30.0f, pal::kBasaltMid);
            DrawRing({v.pos.x, v.pos.y - 6.0f}, 9.0f, 13.0f, 0.0f, 360.0f, 20, pal::kValveCopper);
            const float spin = v.open ? (sim_time * 1.6f) : 0.4f;
            for (int sp = 0; sp < 4; ++sp) {
                const float ang = spin + static_cast<float>(sp) * (eco::kPi * 0.5f);
                DrawLineEx({v.pos.x, v.pos.y - 6.0f},
                           {v.pos.x + std::cos(ang) * 11.0f, v.pos.y - 6.0f + std::sin(ang) * 11.0f},
                           2.4f, pal::kBronzeBright);
            }
            DrawCircleV({v.pos.x, v.pos.y - 6.0f}, 4.5f, state_col);

            if (near_player) {
                const float pulse = 0.65f + 0.35f * std::sin(sim_time * 6.0f);
                DrawRing({v.pos.x, v.pos.y}, 21.0f, 23.5f, 0.0f, 360.0f, 24,
                         pal::WithAlpha(state_col, static_cast<unsigned char>(205.0f * pulse)));
            }
        }
    } else if (zone == eco::ZoneId::HarmonySanctuary) {
        // Cristalli/colonne di luce sopra i sigilli accordati
        for (std::size_t i = 0; i < zm.sanctuary_seals.size(); ++i) {
            const auto& seal = zm.sanctuary_seals[i];
            if (!seal.attuned) {
                continue;
            }
            if (std::fabs(seal.pos.x - cam.x) > half_w || std::fabs(seal.pos.y - cam.y) > half_h) {
                continue;
            }
            const Color col = pal::SanctuarySealColor(static_cast<int>(i));
            const float hover = std::sin(sim_time * 2.8f + static_cast<float>(i)) * 4.0f;
            DrawLineEx({seal.pos.x, seal.pos.y}, {seal.pos.x, seal.pos.y - 34.0f}, 6.0f, pal::WithAlpha(col, 62));
            DrawPoly({seal.pos.x, seal.pos.y - 18.0f + hover}, 4, 8.5f, 45.0f, col);
            DrawPolyLines({seal.pos.x, seal.pos.y - 18.0f + hover}, 4, 8.5f, 45.0f, pal::kStoneIvory);
        }
    }
}

// =============================================================================
// Rendering Portali Strutture (3 architetture uniche nell'Overworld)
// =============================================================================
void DrawStructurePortalTile(
    int rx,
    int ry,
    int tile_px,
    eco::StructureId sid,
    bool left_is_portal,
    bool right_is_portal,
    float poll,
    float sim_time) {
    const float health = 1.0f - poll;
    const float pulse = 0.5f + 0.5f * std::sin(sim_time * 3.8f);
    const int pad_l = left_is_portal ? 0 : 6;
    const int pad_r = right_is_portal ? 0 : 6;

    if (sid == eco::StructureId::HypogeanCisterns) {
        const Color stone = pal::WorldColor(Color{76, 72, 68, 255}, Color{134, 152, 162, 255}, poll);
        const Color water_glow = pal::LerpColor(
            pal::WorldColor(Color{74, 96, 84, 255}, Color{42, 158, 208, 255}, poll),
            Color{116, 242, 255, 255},
            pulse * (0.35f + 0.65f * health));
        DrawRectangle(rx, ry, tile_px, tile_px, stone);
        DrawRectangle(rx + pad_l, ry + 7, tile_px - pad_l - pad_r, tile_px - 9, Color{14, 34, 50, 255});
        for (int s = 0; s < 3; ++s) {
            const float sy = static_cast<float>(ry + 9) + std::fmod(sim_time * 34.0f + static_cast<float>(s * 11), 30.0f);
            DrawLineEx({static_cast<float>(rx + pad_l + 3), sy},
                       {static_cast<float>(rx + tile_px - pad_r - 3), sy}, 2.2f, water_glow);
        }
        DrawLineEx({static_cast<float>(rx + pad_l), static_cast<float>(ry + 6)},
                   {static_cast<float>(rx + tile_px - pad_r), static_cast<float>(ry + 6)}, 2.5f, water_glow);
        DrawLineEx({static_cast<float>(rx + pad_l), static_cast<float>(ry + tile_px - 2)},
                   {static_cast<float>(rx + tile_px - pad_r), static_cast<float>(ry + tile_px - 2)}, 2.0f, water_glow);
        if (!left_is_portal) {
            DrawLineEx({static_cast<float>(rx + 5), static_cast<float>(ry + 6)},
                       {static_cast<float>(rx + 5), static_cast<float>(ry + tile_px - 2)}, 2.5f, pal::kBronzeBright);
        }
        if (!right_is_portal) {
            DrawLineEx({static_cast<float>(rx + tile_px - 5), static_cast<float>(ry + 6)},
                       {static_cast<float>(rx + tile_px - 5), static_cast<float>(ry + tile_px - 2)}, 2.5f, pal::kBronzeBright);
        }
        if (right_is_portal) {
            DrawRectangle(rx + tile_px - 6, ry + 1, 12, 8, pal::ShiftBrightness(stone, 28));
            DrawCircle(rx + tile_px, ry + 5, 2.5f, water_glow);
        }
    } else if (sid == eco::StructureId::PendulumTower) {
        const Color stone = pal::WorldColor(Color{84, 78, 72, 255}, Color{152, 154, 152, 255}, poll);
        const Color gold_glow = pal::LerpColor(pal::kBronze, pal::kAmberGlow, pulse * (0.4f + 0.6f * health));
        DrawRectangle(rx, ry, tile_px, tile_px, stone);
        DrawRectangle(rx + pad_l, ry + 7, tile_px - pad_l - pad_r, tile_px - 9, Color{24, 28, 42, 255});
        DrawLineEx({static_cast<float>(rx + pad_l), static_cast<float>(ry + 6)},
                   {static_cast<float>(rx + tile_px - pad_r), static_cast<float>(ry + 6)}, 2.5f, gold_glow);
        if (!left_is_portal) {
            DrawLineEx({static_cast<float>(rx + 5), static_cast<float>(ry + 6)},
                       {static_cast<float>(rx + 5), static_cast<float>(ry + tile_px - 2)}, 2.5f, gold_glow);
        }
        if (!right_is_portal) {
            DrawLineEx({static_cast<float>(rx + tile_px - 5), static_cast<float>(ry + 6)},
                       {static_cast<float>(rx + tile_px - 5), static_cast<float>(ry + tile_px - 2)}, 2.5f, gold_glow);
        }
        if (right_is_portal) {
            const float cx = static_cast<float>(rx + tile_px);
            const float cy = static_cast<float>(ry + 12);
            DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), 11.0f, gold_glow);
            const float swing = std::sin(sim_time * 2.5f) * 10.0f;
            DrawLineEx({cx, cy}, {cx + swing, static_cast<float>(ry + tile_px - 8)}, 2.0f, gold_glow);
            DrawCircleV({cx + swing, static_cast<float>(ry + tile_px - 8)}, 3.6f, pal::kBronzeBright);
        }
    } else {
        const Color basalt = pal::WorldColor(Color{62, 50, 46, 255}, Color{78, 84, 94, 255}, poll);
        const Color thermal = pal::LerpColor(
            pal::WorldColor(Color{198, 78, 36, 255}, Color{226, 152, 72, 255}, poll),
            pal::kAmberGlow,
            pulse);
        DrawRectangle(rx, ry, tile_px, tile_px, basalt);
        DrawRectangle(rx + pad_l, ry + 7, tile_px - pad_l - pad_r, tile_px - 9, Color{34, 22, 20, 255});
        if (!left_is_portal) {
            DrawRectangle(rx + 1, ry + 4, 5, tile_px - 6, pal::kBronze);
        }
        if (!right_is_portal) {
            DrawRectangle(rx + tile_px - 6, ry + 4, 5, tile_px - 6, pal::kBronze);
        }
        for (int s = 0; s < 3; ++s) {
            const float sy = static_cast<float>(ry + tile_px - 8) - std::fmod(sim_time * 24.0f + static_cast<float>(s * 10), 28.0f);
            DrawLineEx({static_cast<float>(rx + pad_l + 4), sy},
                       {static_cast<float>(rx + tile_px - pad_r - 4), sy}, 2.0f, thermal);
        }
        DrawLineEx({static_cast<float>(rx + pad_l), static_cast<float>(ry + 6)},
                   {static_cast<float>(rx + tile_px - pad_r), static_cast<float>(ry + 6)}, 2.5f, thermal);
        if (right_is_portal) {
            DrawPoly({static_cast<float>(rx + tile_px), static_cast<float>(ry + 6)}, 6, 7.0f, 30.0f, pal::kBronze);
            DrawCircle(rx + tile_px, ry + 6, 3.0f, thermal);
        }
    }
}

// =============================================================================
// Sfondi in Parallasse dedicati alle 3 Strutture Interne
// =============================================================================
void DrawCisternsBackdrop(
    int world_w,
    int world_h,
    float parallax_x,
    float poll,
    float sim_time,
    const pal::StructurePalette& sp) {
    const float health = 1.0f - poll;
    const Color arch_far = pal::WorldColor(sp.arch_far_sick, sp.arch_far_healthy, poll);
    const Color arch_near = pal::WorldColor(sp.arch_near_sick, sp.arch_near_healthy, poll);
    const Color water_fall = pal::WorldColor(sp.liquid_sick, sp.liquid_healthy, poll);
    const Color foam = pal::WorldColor(pal::kSickWaterFoam, pal::kHealthyWaterFoam, poll);

    const float p_far = parallax_x * 0.55f;
    for (int i = -1; i < 10; ++i) {
        const int ax = static_cast<int>(60.0f + static_cast<float>(i) * 210.0f - p_far);
        DrawRectangle(ax, 48, 38, world_h - 130, arch_far);
        DrawCircle(ax + 124, 155, 86.0f, pal::ShiftBrightness(arch_far, -6));
        DrawRing({static_cast<float>(ax + 124), 155.0f}, 76.0f, 88.0f, 180.0f, 360.0f, 24, pal::ShiftBrightness(arch_far, 14));
    }

    for (int i = -1; i < 9; ++i) {
        const int px = static_cast<int>(100.0f + static_cast<float>(i) * 240.0f - parallax_x);
        DrawRectangle(px, 48, 52, world_h - 136, arch_near);
        DrawRectangleLines(px, 48, 52, world_h - 136, pal::ShiftBrightness(arch_near, 18));
        DrawRectangle(px - 2, 160, 56, 6, pal::WithAlpha(pal::kBronze, 175));
        DrawRectangle(px - 2, 360, 56, 6, pal::WithAlpha(pal::kBronze, 175));

        if ((i & 1) == 0) {
            const int wx = px + 96;
            DrawRectangle(wx - 5, 48, 42, world_h - 136, pal::ShiftBrightness(arch_near, -10));
            DrawRectangle(wx, 48, 32, world_h - 136, pal::WithAlpha(water_fall, 145));
            for (int st = 0; st < 6; ++st) {
                const float sy = 56.0f + std::fmod(sim_time * 135.0f + static_cast<float>(st * 98 + i * 37),
                                                   static_cast<float>(std::max(120, world_h - 160)));
                const float sx = static_cast<float>(wx + 6 + (st % 4) * 6);
                DrawLineEx({sx, sy}, {sx, sy + 36.0f}, 2.2f, foam);
            }
            const float mist_r = 22.0f + 5.0f * std::sin(sim_time * 3.0f + static_cast<float>(i));
            DrawEllipse(wx + 16, world_h - 102, mist_r, 8.0f, pal::WithAlpha(foam, static_cast<unsigned char>(65.0f + 45.0f * health)));
        }
    }

    DrawRectangle(48, world_h - 108, world_w - 96, 24, pal::WithAlpha(water_fall, 175));
    for (int x = 64; x < world_w - 96; x += 68) {
        const float dx = std::sin(sim_time * 2.4f + static_cast<float>(x) * 0.03f) * 6.0f;
        DrawLineEx({static_cast<float>(x) + dx, static_cast<float>(world_h - 101)},
                   {static_cast<float>(x + 34) + dx, static_cast<float>(world_h - 101)}, 2.0f, foam);
    }
}

void DrawPendulumTowerBackdrop(
    int world_w,
    int world_h,
    float parallax_x,
    float poll,
    float sim_time,
    const pal::StructurePalette& sp) {
    const float health = 1.0f - poll;
    const Color arch_far = pal::WorldColor(sp.arch_far_sick, sp.arch_far_healthy, poll);
    const Color arch_near = pal::WorldColor(sp.arch_near_sick, sp.arch_near_healthy, poll);
    const Color dial_col = pal::WorldColor(sp.accent_sick, sp.accent_healthy, poll);
    const Color beam_col = pal::WorldColor(sp.liquid_sick, sp.liquid_healthy, poll);

    const float p_far = parallax_x * 0.45f;
    for (int i = 0; i < 6; ++i) {
        const float cx = 190.0f + static_cast<float>(i) * 290.0f - p_far;
        const float cy_top = 185.0f;
        const float cy_mid = 435.0f;
        const unsigned char beam_a = static_cast<unsigned char>(18.0f + 26.0f * health);
        DrawTriangle(
            {cx - 38.0f, cy_top},
            {cx + 38.0f, cy_top},
            {cx + 165.0f, static_cast<float>(world_h - 96)},
            pal::WithAlpha(beam_col, beam_a));
        DrawTriangle(
            {cx - 38.0f, cy_top},
            {cx + 165.0f, static_cast<float>(world_h - 96)},
            {cx + 55.0f, static_cast<float>(world_h - 96)},
            pal::WithAlpha(beam_col, beam_a));

        DrawCircleV({cx, cy_top}, 52.0f, pal::WithAlpha(beam_col, static_cast<unsigned char>(36.0f + 46.0f * health)));
        DrawRing({cx, cy_top}, 46.0f, 54.0f, 0.0f, 360.0f, 32, arch_near);
        DrawRing({cx, cy_top}, 42.0f, 45.0f, 0.0f, 360.0f, 28, pal::WithAlpha(dial_col, 150));
        for (int r = 0; r < 8; ++r) {
            const float a = static_cast<float>(r) * (eco::kPi / 4.0f);
            DrawLineEx(
                {cx + std::cos(a) * 14.0f, cy_top + std::sin(a) * 14.0f},
                {cx + std::cos(a) * 44.0f, cy_top + std::sin(a) * 44.0f},
                1.5f,
                pal::WithAlpha(dial_col, 125));
        }

        DrawCircleV({cx, cy_mid}, 34.0f, pal::WithAlpha(beam_col, static_cast<unsigned char>(24.0f + 32.0f * health)));
        DrawRectangle(static_cast<int>(cx - 34.0f), static_cast<int>(cy_mid), 68, 92,
                      pal::WithAlpha(beam_col, static_cast<unsigned char>(18.0f + 22.0f * health)));
        DrawRing({cx, cy_mid}, 32.0f, 37.0f, 180.0f, 360.0f, 20, arch_near);
        DrawRing({cx, cy_mid}, 46.0f, 48.0f, 0.0f, 360.0f, 32, pal::WithAlpha(dial_col, 72));
        DrawLineEx({cx - 34.0f, cy_mid}, {cx - 34.0f, cy_mid + 92.0f}, 2.0f, pal::WithAlpha(dial_col, 105));
        DrawLineEx({cx + 34.0f, cy_mid}, {cx + 34.0f, cy_mid + 92.0f}, 2.0f, pal::WithAlpha(dial_col, 105));
        DrawLineEx({cx, cy_mid - 32.0f}, {cx, cy_mid + 92.0f}, 1.4f, pal::WithAlpha(dial_col, 85));
        DrawLineEx({cx - 34.0f, cy_mid + 36.0f}, {cx + 34.0f, cy_mid + 36.0f}, 1.4f, pal::WithAlpha(dial_col, 85));
    }

    for (int i = -1; i < 8; ++i) {
        const int px = static_cast<int>(85.0f + static_cast<float>(i) * 235.0f - parallax_x);
        DrawRectangle(px, 48, 36, world_h - 130, arch_far);
        DrawRectangle(px + 6, 48, 24, world_h - 130, arch_near);
        DrawLine(px + 6, 48, px + 6, world_h - 96, pal::ShiftBrightness(arch_near, 22));

        const float arc_cx = static_cast<float>(px + 135);
        DrawRing({arc_cx, 110.0f}, 96.0f, 99.0f, 25.0f, 155.0f, 24, pal::WithAlpha(dial_col, 95));

        if ((i & 1) == 1) {
            const float sway = std::sin(sim_time * 1.15f + static_cast<float>(i)) * 8.0f;
            const float wy = 240.0f + static_cast<float>((i % 3) * 45);
            DrawLineEx({arc_cx, 48.0f}, {arc_cx + sway, wy}, 1.8f, pal::WithAlpha(dial_col, 140));
            DrawRectangle(static_cast<int>(arc_cx + sway - 8.0f), static_cast<int>(wy), 16, 28, pal::WithAlpha(pal::kBronze, 190));
            DrawRectangleLines(static_cast<int>(arc_cx + sway - 8.0f), static_cast<int>(wy), 16, 28, dial_col);
        }
    }

    for (int gy = 144; gy < world_h - 120; gy += 96) {
        DrawLine(48, gy, world_w - 48, gy, pal::WithAlpha(dial_col, 32));
    }
}

void DrawGeothermalBackdrop(
    int world_w,
    int world_h,
    float parallax_x,
    float poll,
    float sim_time,
    const pal::StructurePalette& sp) {
    const float health = 1.0f - poll;
    const Color basalt_far = pal::WorldColor(sp.arch_far_sick, sp.arch_far_healthy, poll);
    const Color basalt_near = pal::WorldColor(sp.arch_near_sick, sp.arch_near_healthy, poll);
    const Color accent = pal::WorldColor(sp.accent_sick, sp.accent_healthy, poll);
    const Color pool_col = pal::WorldColor(sp.liquid_sick, sp.liquid_healthy, poll);

    const float p_far = parallax_x * 0.5f;
    for (int i = -1; i < 24; ++i) {
        const std::uint32_t h = Hash2(i, 7, 313);
        const int bx = static_cast<int>(static_cast<float>(i) * 82.0f - p_far);
        const int bw = 42 + static_cast<int>(h % 26u);
        const int top_y = 48 + static_cast<int>((h >> 4) % 90u);
        const Color col = pal::ShiftBrightness(basalt_far, static_cast<int>(h % 13u) - 6);
        DrawRectangle(bx, top_y, bw, world_h - top_y - 96, col);
        DrawLine(bx, top_y, bx, world_h - 96, pal::ShiftBrightness(col, 14));
        if ((h & 1u) == 0u) {
            const float pulse = 0.60f + 0.40f * std::sin(sim_time * 2.2f + static_cast<float>(i));
            const float vx = static_cast<float>(bx + bw / 2);
            DrawLineEx({vx, static_cast<float>(top_y + 40)}, {vx + 8.0f, static_cast<float>(world_h - 110)},
                       4.5f, pal::WithAlpha(accent, static_cast<unsigned char>(34.0f * pulse)));
            DrawLineEx({vx, static_cast<float>(top_y + 40)}, {vx + 8.0f, static_cast<float>(world_h - 110)},
                       1.8f, pal::WithAlpha(accent, static_cast<unsigned char>(115.0f * pulse)));
        }
    }

    for (int i = -1; i < 8; ++i) {
        const int px = static_cast<int>(110.0f + static_cast<float>(i) * 245.0f - parallax_x);
        DrawRing({static_cast<float>(px + 145), 240.0f}, 96.0f, 108.0f, 180.0f, 360.0f, 22, pal::WithAlpha(basalt_near, 195));
        DrawRing({static_cast<float>(px + 145), 240.0f}, 94.0f, 97.0f, 180.0f, 360.0f, 22, pal::WithAlpha(pal::kBronze, 120));

        DrawRectangle(px, 48, 46, world_h - 134, basalt_near);
        DrawRectangle(px + 10, 48, 26, world_h - 134, pal::WorldColor(Color{96, 62, 46, 220}, Color{84, 114, 110, 220}, poll));
        DrawRectangleLines(px + 10, 48, 26, world_h - 134, pal::WithAlpha(pal::kBronze, 180));
        for (int ry = 110; ry < world_h - 140; ry += 96) {
            DrawRectangle(px + 6, ry, 34, 8, pal::kBronzeDark);
            DrawCircle(px + 12, ry + 4, 1.8f, accent);
            DrawCircle(px + 34, ry + 4, 1.8f, accent);
        }

        const float sx = static_cast<float>(px + 135);
        for (int puff = 0; puff < 3; ++puff) {
            const float phase = std::fmod(sim_time * 0.28f + static_cast<float>(puff) * 0.33f + static_cast<float>(i) * 0.17f, 1.0f);
            const float py = static_cast<float>(world_h - 120) - phase * static_cast<float>(world_h - 240);
            const float wx = sx + std::sin(sim_time * 1.4f + phase * 5.0f + static_cast<float>(i)) * 14.0f;
            const unsigned char pa = static_cast<unsigned char>((1.0f - phase) * (health > 0.5f ? 18.0f : 22.0f));
            const Color steam = pal::WorldColor(Color{176, 112, 82, pa}, Color{198, 238, 244, pa}, poll);
            const float rx_s = 28.0f + phase * 32.0f;
            const float ry_s = 14.0f + phase * 16.0f;
            DrawEllipse(static_cast<int>(wx), static_cast<int>(py), rx_s, ry_s, steam);
            DrawEllipse(static_cast<int>(wx + 12.0f), static_cast<int>(py - 8.0f), rx_s * 0.72f, ry_s * 0.8f, steam);
            DrawEllipse(static_cast<int>(wx - 10.0f), static_cast<int>(py + 5.0f), rx_s * 0.65f, ry_s * 0.75f, steam);
        }
    }

    DrawRectangleGradientV(48, world_h - 220, world_w - 96, 120, Color{0, 0, 0, 0}, pal::WithAlpha(accent, 44));
    DrawRectangle(48, world_h - 108, world_w - 96, 24, pal::WithAlpha(pool_col, 195));
    for (int x = 70; x < world_w - 90; x += 64) {
        const float glow_r = 4.0f + 2.5f * std::sin(sim_time * 3.2f + static_cast<float>(x) * 0.07f);
        DrawCircle(x, world_h - 98, glow_r, pal::WithAlpha(accent, 155));
    }
}

// =============================================================================
// Cache Procedurale Per-Zona (allocata SOLO per la zona attiva, liberata allo switch)
// =============================================================================
struct StaticProp {
    float x = 0.0f;
    float y = 0.0f;
    float scale = 1.0f;
    std::uint32_t h = 0;
    unsigned roll = 0;
};

enum class FrescoKind : std::uint8_t {
    SundialObelisk = 0,
    HydroFountain = 1,
    WindStele = 2,
    AcousticBasin = 3,
    ThermalBrazier = 4,
    SanctuaryStatue = 5,
    BasRelief = 6
};

struct ZoneFresco {
    float x = 0.0f;
    float y = 0.0f;
    FrescoKind kind = FrescoKind::BasRelief;
    const char* caption = "";
};

std::vector<StaticProp> g_prop_cache;
std::vector<ZoneFresco> g_fresco_cache;
eco::ZoneId g_cached_zone = eco::ZoneId::Count;
int g_prop_cache_rows = -1;
int g_prop_cache_cols = -1;

[[nodiscard]] bool IsPropPositionSafe(const eco::WorldContext& world, eco::Vec2 pos) noexcept {
    if (pos.DistanceTo(world.overworld.spawn_point) < 76.0f) {
        return false;
    }
    bool blocked = false;
    world.ForEachMachineInScene([&](const eco::Machine& m) {
        if (pos.DistanceTo(m.pos) < 112.0f) {
            blocked = true;
        }
    });
    if (blocked) {
        return false;
    }
    world.ForEachNpcInScene([&](const eco::WorldNpc& npc) {
        if (pos.DistanceTo(npc.pos) < 80.0f) {
            blocked = true;
        }
    });
    if (blocked) {
        return false;
    }
    for (const auto& p : world.overworld.portals) {
        if (pos.DistanceTo(p.world_center) < 110.0f || pos.DistanceTo(p.exit_spawn) < 76.0f) {
            return false;
        }
    }
    for (const auto* item : world.CollectiblesInCurrentScene()) {
        if (item != nullptr && pos.DistanceTo(item->pos) < 64.0f) {
            return false;
        }
    }
    for (const auto& gw : world.overworld.active_gateways) {
        if (pos.DistanceTo(gw.world_center) < 136.0f) {
            return false;
        }
    }

    // Evita sovrapposizioni con le 6 meccaniche della zona attiva
    const eco::ZoneId zone = world.overworld.active_zone;
    const eco::ZoneMechanicState& zm = world.zone_mechanics;
    if (zone == eco::ZoneId::DawnTerraces) {
        if (pos.DistanceTo(zm.solar_emitter_pos) < 74.0f) {
            return false;
        }
        for (const auto& m : zm.solar_mirrors) {
            if (pos.DistanceTo(m.pos) < 74.0f) {
                return false;
            }
        }
        for (const auto& pl : zm.solar_plates) {
            if (pos.DistanceTo(pl.pos) < pl.radius + 54.0f) {
                return false;
            }
        }
    } else if (zone == eco::ZoneId::CanalQuarter) {
        for (const auto& lev : zm.sluice_levers) {
            if (pos.DistanceTo(lev.pos) < 74.0f) {
                return false;
            }
        }
        for (const auto& cur : zm.water_currents) {
            const float pad = 30.0f;
            if (pos.x >= cur.bounds.x - pad && pos.x <= cur.bounds.Right() + pad &&
                pos.y >= cur.bounds.y - pad && pos.y <= cur.bounds.Bottom() + pad) {
                return false;
            }
        }
    } else if (zone == eco::ZoneId::WindPlateau) {
        for (const auto& wb : zm.windbreaks) {
            if (pos.DistanceTo(wb.center) < 92.0f) {
                return false;
            }
        }
    } else if (zone == eco::ZoneId::HangingGardens) {
        for (const auto& pad : zm.resonance_pads) {
            if (pos.DistanceTo(pad.pos) < pad.radius + 58.0f) {
                return false;
            }
        }
    } else if (zone == eco::ZoneId::FumaroleCaldera) {
        for (const auto& g : zm.geysers) {
            if (pos.DistanceTo(g.pos) < g.radius + 56.0f) {
                return false;
            }
        }
        for (const auto& v : zm.relief_valves) {
            if (pos.DistanceTo(v.pos) < 74.0f) {
                return false;
            }
        }
    } else if (zone == eco::ZoneId::HarmonySanctuary) {
        for (const auto& s : zm.sanctuary_seals) {
            if (pos.DistanceTo(s.pos) < s.radius + 60.0f) {
                return false;
            }
        }
    }
    return true;
}

void PopulateZoneFrescoes(const eco::WorldContext& world) {
    const eco::Tilemap& tilemap = world.overworld.tilemap;
    const eco::ZoneId zone = world.overworld.active_zone;

    struct Candidate {
        int row;
        int col;
        FrescoKind kind;
        const char* caption;
    };
    std::array<Candidate, 3> candidates{};
    switch (zone) {
        case eco::ZoneId::DawnTerraces:
            candidates = {{
                {7, 8, FrescoKind::SundialObelisk, "MERIDIANA D'OTTONE"},
                {19, 27, FrescoKind::BasRelief, "ARCHITRAVE DEL COSENO"},
                {15, 7, FrescoKind::SundialObelisk, ""},
            }};
            break;
        case eco::ZoneId::CanalQuarter:
            candidates = {{
                {6, 10, FrescoKind::HydroFountain, "FONTANA DELLE CHIUSE"},
                {19, 25, FrescoKind::BasRelief, "LAPIDE DI VALERIO"},
                {18, 8, FrescoKind::HydroFountain, ""},
            }};
            break;
        case eco::ZoneId::WindPlateau:
            candidates = {{
                {7, 10, FrescoKind::WindStele, "STELE DEI VENTI"},
                {19, 26, FrescoKind::BasRelief, "ROSA DI BASALTO"},
                {18, 9, FrescoKind::SundialObelisk, ""},
            }};
            break;
        case eco::ZoneId::HangingGardens:
            candidates = {{
                {7, 10, FrescoKind::AcousticBasin, "VASCA ARMONICA"},
                {19, 25, FrescoKind::BasRelief, "PERGOLATO DI GIADA"},
                {18, 8, FrescoKind::AcousticBasin, ""},
            }};
            break;
        case eco::ZoneId::FumaroleCaldera:
            candidates = {{
                {6, 10, FrescoKind::ThermalBrazier, "BRACIERE GEOTERMICO"},
                {19, 26, FrescoKind::BasRelief, "SIGILLO DELLO SCISMA"},
                {18, 8, FrescoKind::ThermalBrazier, ""},
            }};
            break;
        case eco::ZoneId::HarmonySanctuary:
        default:
            candidates = {{
                {6, 10, FrescoKind::SanctuaryStatue, "CUSTODE DELLE ERE"},
                {19, 25, FrescoKind::SanctuaryStatue, "ARCO DI MERIDIA"},
                {6, 25, FrescoKind::AcousticBasin, ""},
            }};
            break;
    }

    for (const auto& cand : candidates) {
        // Cerca la tile Walkable/GardenFloor sicura piu' vicina al punto candidato
        bool placed = false;
        for (int radius = 0; radius <= 4 && !placed; ++radius) {
            for (int dr = -radius; dr <= radius && !placed; ++dr) {
                for (int dc = -radius; dc <= radius && !placed; ++dc) {
                    const int r = cand.row + dr;
                    const int c = cand.col + dc;
                    if (!tilemap.InBounds(r, c) || r <= 1 || r >= tilemap.Rows() - 2 || c <= 1 || c >= tilemap.Cols() - 2) {
                        continue;
                    }
                    if (IsTileSolidForProp(tilemap.GetTile(r, c))) {
                        continue;
                    }
                    const eco::Vec2 pos = tilemap.TileCenter(r, c);
                    if (!IsPropPositionSafe(world, pos)) {
                        continue;
                    }
                    bool too_close_fresco = false;
                    for (const auto& existing : g_fresco_cache) {
                        if (pos.DistanceTo({existing.x, existing.y}) < 140.0f) {
                            too_close_fresco = true;
                            break;
                        }
                    }
                    if (too_close_fresco) {
                        continue;
                    }
                    g_fresco_cache.push_back(ZoneFresco{pos.x, pos.y, cand.kind, cand.caption});
                    placed = true;
                }
            }
        }
    }
}

void RebuildZoneCacheIfNeeded(const eco::WorldContext& world) {
    const eco::Tilemap& tilemap = world.overworld.tilemap;
    const eco::ZoneId active_zone = world.overworld.active_zone;
    if (g_cached_zone == active_zone &&
        g_prop_cache_rows == tilemap.Rows() &&
        g_prop_cache_cols == tilemap.Cols()) {
        return;
    }

    // Libera fisicamente la memoria dei prop/affreschi della zona precedente prima di rigenerare
    ReleaseOverworldProceduralCache();

    g_cached_zone = active_zone;
    g_prop_cache_rows = tilemap.Rows();
    g_prop_cache_cols = tilemap.Cols();

    PopulateZoneFrescoes(world);

    g_prop_cache.reserve(96);
    for (int r = 1; r <= tilemap.Rows() - 2; ++r) {
        for (int c = 1; c <= tilemap.Cols() - 2; ++c) {
            if (((r * 7 + c * 11 + static_cast<int>(active_zone) * 3) % 11) != 0) {
                continue;
            }
            if (IsTileSolidForProp(tilemap.GetTile(r, c))) {
                continue;
            }
            int solid_neighbors = 0;
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0) {
                        continue;
                    }
                    const eco::TileId nb = tilemap.GetTile(r + dr, c + dc);
                    if (nb == eco::TileId::StructurePortal || nb == eco::TileId::HiddenBridge ||
                        nb == eco::TileId::ZoneGateway || nb == eco::TileId::SolarGate ||
                        nb == eco::TileId::JadeBridge) {
                        solid_neighbors += 9;
                    } else if (IsWallLike(nb) || nb == eco::TileId::PavedPath || nb == eco::TileId::ShallowChannel) {
                        ++solid_neighbors;
                    }
                }
            }
            if (solid_neighbors >= 5) {
                continue;
            }

            const std::uint32_t h = Hash2(c, r, 409 + static_cast<int>(active_zone) * 31);
            const eco::Vec2 center = tilemap.TileCenter(r, c);
            const eco::Vec2 pos{
                center.x + static_cast<float>(static_cast<int>(h % 13u) - 6),
                center.y + static_cast<float>(static_cast<int>((h >> 4) % 13u) - 6)};
            if (!IsPropPositionSafe(world, pos)) {
                continue;
            }
            bool near_fresco = false;
            for (const auto& f : g_fresco_cache) {
                if (pos.DistanceTo({f.x, f.y}) < 86.0f) {
                    near_fresco = true;
                    break;
                }
            }
            if (near_fresco) {
                continue;
            }

            StaticProp p;
            p.x = pos.x;
            p.y = pos.y;
            p.scale = 0.88f + static_cast<float>((h >> 8) % 25u) * 0.01f;
            p.h = h;
            p.roll = (h >> 14) % 12u;
            g_prop_cache.push_back(p);
        }
    }
    g_prop_cache.shrink_to_fit();
    g_fresco_cache.shrink_to_fit();
}

// =============================================================================
// Varchi Monumentali di Confine tra le 6 Zone Separate (ZoneGateway)
// =============================================================================
void DrawGatewayPlaque(float cx, float y, const char* text, Color accent) {
    if (text == nullptr || text[0] == '\0') {
        return;
    }
    const int tw = MeasureUiText(text, 11);
    const int bw = tw + 16;
    const int bx = static_cast<int>(cx) - bw / 2;
    const int by = static_cast<int>(y);
    DrawRectangle(bx + 2, by + 2, bw, 18, Color{0, 0, 0, 105});
    DrawRectangle(bx, by, bw, 18, Color{16, 22, 32, 240});
    DrawRectangleLines(bx, by, bw, 18, pal::WithAlpha(pal::kBronze, 230));
    DrawRectangle(bx + 2, by + 2, 3, 14, accent);
    DrawRectangle(bx + bw - 5, by + 2, 3, 14, accent);
    DrawTextShadow(text, bx + 8, by + 3, 11, pal::kParchmentText);
}

void DrawGatewayPillar(float cx, float base_y, float height, Color stone, Color accent, float pulse) {
    const Color dark = pal::ShiftBrightness(stone, -34);
    const Color hi = pal::ShiftBrightness(stone, 26);
    DrawEllipse(static_cast<int>(cx + 4.0f), static_cast<int>(base_y + 2.0f), 17.0f, 6.0f, Color{0, 0, 0, 110});
    DrawRectangle(static_cast<int>(cx - 14.0f), static_cast<int>(base_y - 10.0f), 28, 12, dark);
    DrawRectangle(static_cast<int>(cx - 10.0f), static_cast<int>(base_y - height), 20, static_cast<int>(height - 8.0f), stone);
    DrawRectangle(static_cast<int>(cx + 3.0f), static_cast<int>(base_y - height), 7, static_cast<int>(height - 8.0f), dark);
    DrawLine(static_cast<int>(cx - 10.0f), static_cast<int>(base_y - height), static_cast<int>(cx - 10.0f), static_cast<int>(base_y - 10.0f), hi);
    DrawRectangle(static_cast<int>(cx - 12.0f), static_cast<int>(base_y - height + 10.0f), 24, 4, pal::kBronze);
    DrawRectangle(static_cast<int>(cx - 12.0f), static_cast<int>(base_y - 20.0f), 24, 4, pal::kBronze);
    DrawRectangle(static_cast<int>(cx - 14.0f), static_cast<int>(base_y - height - 5.0f), 28, 7, hi);
    DrawRectangleLines(static_cast<int>(cx - 14.0f), static_cast<int>(base_y - height - 5.0f), 28, 7, pal::kBronzeDark);
    const float ly = base_y - height - 15.0f;
    DrawCircleV({cx, ly}, 14.0f * pulse, pal::WithAlpha(accent, 60));
    DrawCircleV({cx, ly}, 5.8f, accent);
    DrawCircleV({cx, ly}, 2.6f, pal::kStoneIvory);
    DrawCircleLines(static_cast<int>(cx), static_cast<int>(ly), 5.8f, pal::kBronzeBright);
}

void DrawZoneGatewayArch(const eco::ZoneGateway& gw, const eco::WorldContext& world) {
    const float poll = world.ecosystem.PollutionRatio();
    const float tile = world.overworld.tilemap.TileSize();
    const float pulse = 0.78f + 0.22f * std::sin(world.sim_time * 2.6f + gw.world_center.x * 0.01f);
    const pal::DistrictPalette& to_pal = pal::GetDistrictPalette(gw.to_zone);
    const bool locked = !world.IsZoneGatewayUnlocked(gw);
    const Color acc = locked ? pal::kAccentWarn : to_pal.ambient_accent;
    const Color stone = pal::WorldColor(to_pal.wall_top_sick, to_pal.wall_top_healthy, poll);
    const char* plaque_text = locked ? world.ZoneGatewayLockedPlaque(gw) : gw.signpost_label;

    const float x0 = static_cast<float>(gw.tile_min.col) * tile;
    const float x1 = static_cast<float>(gw.tile_max.col + 1) * tile;
    const float y0 = static_cast<float>(gw.tile_min.row) * tile;
    const float y1 = static_cast<float>(gw.tile_max.row + 1) * tile;
    const float mid_x = (x0 + x1) * 0.5f;
    const float mid_y = (y0 + y1) * 0.5f;

    if (gw.IsHorizontalWall()) {
        const bool is_north_edge = (gw.tile_min.row <= 2);
        const float h = 48.0f;
        const float base_y = is_north_edge ? (y1 + 6.0f) : (y0 + 10.0f);
        DrawGatewayPillar(x0 - 8.0f, base_y, h, stone, acc, pulse);
        DrawGatewayPillar(x1 + 8.0f, base_y, h, stone, acc, pulse);

        const float top = base_y - h - 4.0f;
        DrawRectangle(static_cast<int>(x0 - 16.0f), static_cast<int>(top - 6.0f),
                      static_cast<int>(x1 - x0 + 32.0f), 11, pal::ShiftBrightness(stone, 14));
        DrawRectangleLines(static_cast<int>(x0 - 16.0f), static_cast<int>(top - 6.0f),
                           static_cast<int>(x1 - x0 + 32.0f), 11, pal::kBronze);
        DrawRectangle(static_cast<int>(x0), static_cast<int>(top - 3.0f),
                      static_cast<int>(x1 - x0), 5, pal::WithAlpha(acc, 215));

        const float plaque_y = is_north_edge ? (y1 + 14.0f) : (y0 - 30.0f);
        DrawGatewayPlaque(mid_x, plaque_y, plaque_text, acc);
    } else {
        const bool is_west_edge = (gw.tile_min.col <= 2);
        const float px = is_west_edge ? (x1 + 8.0f) : (x0 - 8.0f);
        DrawGatewayPillar(px, y0 - 4.0f, 48.0f, stone, acc, pulse);
        DrawGatewayPillar(px, y1 + 22.0f, 48.0f, stone, acc, pulse);
        const float plaque_x = is_west_edge ? (x1 + 66.0f) : (x0 - 66.0f);
        DrawGatewayPlaque(plaque_x, mid_y - 9.0f, plaque_text, acc);
    }
}

} // namespace

// =============================================================================
// Hash Deterministico 2D & Gestione Cache Per-Zona
// =============================================================================
std::uint32_t Hash2(int x, int y, int seed) noexcept {
    std::uint32_t h = static_cast<std::uint32_t>(x) * 374761393u +
                      static_cast<std::uint32_t>(y) * 668265263u +
                      static_cast<std::uint32_t>(seed) * 1442695041u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

float Hash2Float01(int x, int y, int seed) noexcept {
    return static_cast<float>(Hash2(x, y, seed) & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
}

void ReleaseOverworldProceduralCache() noexcept {
    g_prop_cache.clear();
    g_prop_cache.shrink_to_fit();
    g_fresco_cache.clear();
    g_fresco_cache.shrink_to_fit();
    g_cached_zone = eco::ZoneId::Count;
    g_prop_cache_rows = -1;
    g_prop_cache_cols = -1;
}

std::size_t ProceduralArtCacheBytes() noexcept {
    return g_prop_cache.capacity() * sizeof(StaticProp) +
           g_fresco_cache.capacity() * sizeof(ZoneFresco);
}

// =============================================================================
// Overworld Map — Zona Attiva Dedicata (26x36), 6 Identita' Bioma & Meccaniche
// =============================================================================
void DrawOverworldProceduralMap(const eco::WorldContext& world) {
    const eco::Tilemap& tilemap = world.overworld.tilemap;
    const eco::ZoneId zone = world.overworld.active_zone;
    const pal::DistrictPalette& dp = pal::GetDistrictPalette(zone);
    const float poll = world.ecosystem.PollutionRatio();
    const float health = 1.0f - poll;
    const float sim_time = world.sim_time;
    const bool bridge_unlocked = world.ecosystem.architect_recruited;
    const bool solar_gate_open = world.zone_mechanics.solar_gate_open;
    const bool jade_bridge_unlocked = world.zone_mechanics.jade_bridge_unlocked;
    const float jade_progress = std::clamp(world.zone_mechanics.jade_bridge_progress, 0.0f, 1.0f);
    const float wind_gust = (zone == eco::ZoneId::WindPlateau) ? world.zone_mechanics.wind.gust_intensity : 0.0f;
    const int tile_px = static_cast<int>(tilemap.TileSize());
    if (tile_px <= 0 || tilemap.Rows() <= 0 || tilemap.Cols() <= 0) {
        return;
    }

    const eco::Vec2 cam = world.overworld.camera_target;
    const float half_w = static_cast<float>(eco::kWindowWidth) * 0.5f + tilemap.TileSize() * 2.5f;
    const float half_h = static_cast<float>(eco::kWindowHeight) * 0.5f + tilemap.TileSize() * 2.5f;
    const int min_col = std::max(0, static_cast<int>(std::floor((cam.x - half_w) / tilemap.TileSize())));
    const int max_col = std::min(tilemap.Cols() - 1, static_cast<int>(std::ceil((cam.x + half_w) / tilemap.TileSize())));
    const int min_row = std::max(0, static_cast<int>(std::floor((cam.y - half_h) / tilemap.TileSize())));
    const int max_row = std::min(tilemap.Rows() - 1, static_cast<int>(std::ceil((cam.y + half_h) / tilemap.TileSize())));

    // -------------------------------------------------------------------------
    // PASSATA 1: Terreni, Sentieri, Piazze, Giardini, Canali, Ponti, Barriere, Varchi
    // -------------------------------------------------------------------------
    for (int r = min_row; r <= max_row; ++r) {
        const int row_cols = tilemap.RowCols(r);
        const int c_end = std::min(max_col, row_cols - 1);
        for (int c = min_col; c <= c_end; ++c) {
            const eco::Rect tr = tilemap.TileRect(r, c);
            const int rx = static_cast<int>(tr.x);
            const int ry = static_cast<int>(tr.y);
            const eco::TileId id = tilemap.GetTile(r, c);
            const std::uint32_t h = Hash2(c, r, 17 + static_cast<int>(zone) * 19);
            const int tone_delta = static_cast<int>(h % 7u) - 3;

            if (id == eco::TileId::Walkable) {
                Color ground = pal::ShiftBrightness(
                    pal::WorldColor(dp.ground_sick, dp.ground_healthy, poll),
                    tone_delta);
                DrawRectangle(rx, ry, tile_px, tile_px, ground);

                if ((h & 3u) == 0u) {
                    const int ox = rx + 12 + static_cast<int>((h >> 4) % 24u);
                    const int oy = ry + 12 + static_cast<int>((h >> 9) % 24u);
                    const float erx = 16.0f + static_cast<float>((h >> 14) % 10u);
                    const float ery = 11.0f + static_cast<float>((h >> 18) % 8u);
                    DrawEllipse(ox, oy, erx, ery, pal::ShiftBrightness(ground, ((h >> 2) & 1u) ? 5 : -5));
                }

                const Color grass_col = pal::WorldColor(dp.grass_sick, dp.grass_healthy, poll);
                if (zone == eco::ZoneId::FumaroleCaldera) {
                    // Piastre di basalto esagonale + crepe geotermiche calde
                    if (((r + c) & 1) == 0) {
                        DrawPolyLines({tr.x + 24.0f, tr.y + 24.0f}, 6, 20.0f, 30.0f, pal::ShiftBrightness(ground, -10));
                    }
                    if ((h & 3u) <= 1u) {
                        const Color crack = pal::LerpColor(ground, pal::WorldColor(dp.inlay_sick, dp.inlay_healthy, poll), 0.65f);
                        const Color fissure_dark = pal::ShiftBrightness(ground, -18);
                        const int cx = rx + 12 + static_cast<int>((h >> 5) % 24u);
                        const int cy = ry + 12 + static_cast<int>((h >> 10) % 24u);
                        const int dx1 = static_cast<int>((h >> 14) % 9u) + 4;
                        const int dy1 = static_cast<int>((h >> 18) % 7u) - 3;
                        DrawLine(cx - dx1, cy - dy1, cx, cy, fissure_dark);
                        DrawLine(cx, cy, cx + dx1, cy + dy1 + 3, fissure_dark);
                        if ((h & 3u) == 0u) {
                            DrawLine(cx - dx1 + 1, cy - dy1, cx, cy, pal::WithAlpha(crack, 255));
                            DrawLine(cx, cy, cx + dx1 - 1, cy + dy1 + 3, pal::WithAlpha(crack, 255));
                        }
                    }
                } else {
                    // Striature di vento su roccia nell'Altopiano dei Venti
                    if (zone == eco::ZoneId::WindPlateau && ((h >> 2) & 3u) == 0u) {
                        DrawLine(rx + 6, ry + 26, rx + 38, ry + 20, pal::ShiftBrightness(ground, 10));
                    }
                    // Ciuffi d'erba reattivi al bioma e alle raffiche
                    if ((h & 3u) <= 2u) {
                        const int gx = rx + 8 + static_cast<int>((h >> 4) % 30u);
                        const int gy = ry + 13 + static_cast<int>((h >> 9) % 25u);
                        const int blade_h = 4 + static_cast<int>(health * 5.0f);
                        const float wind_bias = (zone == eco::ZoneId::WindPlateau)
                                                    ? (-3.2f - wind_gust * 6.5f + std::sin(sim_time * 4.2f + static_cast<float>(c) * 0.4f) * 2.0f)
                                                    : (std::sin(sim_time * 1.8f + static_cast<float>(c + r)) * 1.0f);
                        DrawLineEx({static_cast<float>(gx), static_cast<float>(gy)},
                                   {static_cast<float>(gx - 2) + wind_bias, static_cast<float>(gy - blade_h)}, 1.4f, grass_col);
                        DrawLineEx({static_cast<float>(gx + 1), static_cast<float>(gy)},
                                   {static_cast<float>(gx + 3) + wind_bias, static_cast<float>(gy - blade_h + 1)}, 1.3f, grass_col);
                        DrawLineEx({static_cast<float>(gx - 1), static_cast<float>(gy + 1)},
                                   {static_cast<float>(gx) + wind_bias * 0.8f, static_cast<float>(gy - blade_h - 1)}, 1.2f, pal::ShiftBrightness(grass_col, 14));

                        if (health > 0.42f && ((h >> 14) & 3u) == 0u) {
                            const Color flower_raw = ((h >> 16) & 1u) ? dp.blossom_a : dp.blossom_b;
                            const float ft = std::clamp((health - 0.42f) / 0.58f, 0.0f, 1.0f);
                            const Color flower = pal::WithAlpha(pal::LerpColor(ground, flower_raw, ft), 255);
                            DrawCircleV({static_cast<float>(gx) + wind_bias, static_cast<float>(gy - blade_h - 1)}, 2.3f, flower);
                        }
                    }
                    if (((h >> 3) & 7u) == 2u) {
                        const int px = rx + 12 + static_cast<int>((h >> 11) % 24u);
                        const int py = ry + 12 + static_cast<int>((h >> 15) % 24u);
                        DrawCircle(px, py, 2.4f, pal::ShiftBrightness(ground, -14));
                        DrawCircle(px - 1, py - 1, 1.3f, pal::ShiftBrightness(ground, 14));
                    }
                }
            } else if (id == eco::TileId::GardenFloor) {
                Color garden = pal::ShiftBrightness(pal::WorldColor(dp.garden_sick, dp.garden_healthy, poll), tone_delta);
                DrawRectangle(rx, ry, tile_px, tile_px, garden);

                const Color leaf_dark = pal::WithAlpha(pal::ShiftBrightness(garden, -10), 255);
                const Color leaf_hi = pal::WithAlpha(
                    pal::LerpColor(garden, pal::WorldColor(dp.grass_sick, dp.grass_healthy, poll), 0.45f), 255);
                for (int lf = 0; lf < 3; ++lf) {
                    const std::uint32_t lh = Hash2(c * 7 + lf, r * 5 + lf, 307);
                    const int lx = rx + 9 + static_cast<int>(lh % 30u);
                    const int ly = ry + 9 + static_cast<int>((lh >> 6) % 30u);
                    const float lr = 4.5f + static_cast<float>((lh >> 12) % 4u);
                    DrawCircle(lx + 1, ly + 1, lr, leaf_dark);
                    DrawCircle(lx, ly, lr * 0.72f, leaf_hi);
                }

                if (health > 0.25f) {
                    const float ft = std::clamp((health - 0.25f) / 0.75f, 0.0f, 1.0f);
                    const int count = (health > 0.65f) ? 4 : 2;
                    for (int f = 0; f < count; ++f) {
                        const std::uint32_t fh = Hash2(c * 5 + f, r * 3, 211);
                        const float fx = static_cast<float>(rx + 8 + static_cast<int>(fh % 32u));
                        const float fy = static_cast<float>(ry + 8 + static_cast<int>((fh >> 6) % 32u));
                        const Color fc_raw = ((fh >> 12) & 1u) ? dp.blossom_a : dp.blossom_b;
                        const Color fc = pal::WithAlpha(pal::LerpColor(garden, fc_raw, ft), 255);
                        const Color center_col = pal::WithAlpha(pal::LerpColor(garden, pal::kStoneIvory, ft), 255);
                        const float fr = 2.2f + static_cast<float>((fh >> 15) % 3u) * 0.45f + 0.6f * health;
                        DrawCircleV({fx, fy}, fr, fc);
                        DrawCircleV({fx, fy}, fr * 0.42f, center_col);
                    }
                }

                const Color curb = pal::WorldColor(dp.path_sick, dp.path_healthy, poll);
                const Color curb_hi = pal::ShiftBrightness(curb, 20);
                if (tilemap.GetTile(r - 1, c) != eco::TileId::GardenFloor) {
                    DrawRectangle(rx, ry, tile_px, 4, curb_hi);
                }
                if (tilemap.GetTile(r + 1, c) != eco::TileId::GardenFloor) {
                    DrawRectangle(rx, ry + tile_px - 4, tile_px, 4, pal::ShiftBrightness(curb, -18));
                }
                if (tilemap.GetTile(r, c - 1) != eco::TileId::GardenFloor) {
                    DrawRectangle(rx, ry, 4, tile_px, curb_hi);
                }
                if (tilemap.GetTile(r, c + 1) != eco::TileId::GardenFloor) {
                    DrawRectangle(rx + tile_px - 4, ry, 4, tile_px, pal::ShiftBrightness(curb, -18));
                }
            } else if (id == eco::TileId::PavedPath) {
                const Color ground_bg = pal::WorldColor(dp.ground_sick, dp.ground_healthy, poll);
                DrawRectangle(rx, ry, tile_px, tile_px, ground_bg);

                const Color stone = pal::ShiftBrightness(pal::WorldColor(dp.path_sick, dp.path_healthy, poll), tone_delta);
                const Color mortar = pal::ShiftBrightness(stone, -24);
                const Color bevel = pal::ShiftBrightness(stone, 18);

                const bool pn = IsPavedLike(tilemap.GetTile(r - 1, c));
                const bool ps = IsPavedLike(tilemap.GetTile(r + 1, c));
                const bool pw = IsPavedLike(tilemap.GetTile(r, c - 1));
                const bool pe = IsPavedLike(tilemap.GetTile(r, c + 1));

                const int pad_l = pw ? 0 : 4;
                const int pad_r = pe ? 0 : 4;
                const int pad_t = pn ? 0 : 4;
                const int pad_b = ps ? 0 : 4;

                DrawRectangle(rx + pad_l, ry + pad_t, tile_px - pad_l - pad_r, tile_px - pad_t - pad_b, stone);

                const int mid_y = ry + tile_px / 2;
                const int split_top = rx + 16 + static_cast<int>((h >> 4) % 14u);
                const int split_bot = rx + 8 + static_cast<int>((h >> 9) % 28u);
                DrawLine(rx + pad_l, mid_y, rx + tile_px - pad_r, mid_y, mortar);
                DrawLine(split_top, ry + pad_t, split_top, mid_y, mortar);
                DrawLine(split_bot, mid_y, split_bot, ry + tile_px - pad_b, mortar);

                // Dettaglio lastricato specifico per zona
                if (zone == eco::ZoneId::DawnTerraces && ((r + c) % 3 == 0)) {
                    DrawLine(rx + pad_l + 4, ry + tile_px / 2, rx + tile_px - pad_r - 4, ry + tile_px / 2,
                             pal::WithAlpha(pal::kBronzeBright, 115));
                } else if (zone == eco::ZoneId::CanalQuarter && ((h & 3u) == 0u)) {
                    DrawCircle(rx + 10, ry + 10, 1.8f, pal::kBronze);
                } else if (zone == eco::ZoneId::FumaroleCaldera && ((r + c) & 1) == 0) {
                    DrawLine(rx + pad_l + 6, ry + pad_t + 6, rx + tile_px - pad_r - 6, ry + tile_px - pad_b - 6,
                             pal::WithAlpha(pal::kValveCopper, 85));
                }

                if (!pn) DrawLine(rx + pad_l, ry + pad_t, rx + tile_px - pad_r, ry + pad_t, bevel);
                if (!pw) DrawLine(rx + pad_l, ry + pad_t, rx + pad_l, ry + tile_px - pad_b, bevel);
                if (!ps) DrawLine(rx + pad_l, ry + tile_px - pad_b, rx + tile_px - pad_r, ry + tile_px - pad_b, mortar);
                if (!pe) DrawLine(rx + tile_px - pad_r, ry + pad_t, rx + tile_px - pad_r, ry + tile_px - pad_b, mortar);

                if (health > 0.40f && (h & 3u) == 0u && zone != eco::ZoneId::FumaroleCaldera) {
                    const Color moss = pal::WorldColor(dp.grass_sick, dp.grass_healthy, poll);
                    DrawCircle(split_top, mid_y, 2.2f * health, pal::WithAlpha(moss, 200));
                }
            } else if (id == eco::TileId::SanctuaryFloor) {
                const Color slab = pal::ShiftBrightness(pal::WorldColor(dp.sanctuary_sick, dp.sanctuary_healthy, poll), tone_delta);
                const Color inlay = pal::WorldColor(dp.inlay_sick, dp.inlay_healthy, poll);
                const Color groove = pal::ShiftBrightness(slab, -22);
                const Color bevel = pal::ShiftBrightness(slab, 20);

                DrawRectangle(rx, ry, tile_px, tile_px, slab);
                DrawRectangleLines(rx, ry, tile_px, tile_px, groove);
                DrawLine(rx + 1, ry + 1, rx + tile_px - 2, ry + 1, bevel);
                DrawLine(rx + 1, ry + 1, rx + 1, ry + tile_px - 2, bevel);
                DrawRectangleLines(rx + 5, ry + 5, tile_px - 10, tile_px - 10, pal::WithAlpha(inlay, 130));
                if (((r + c) & 1) == 0) {
                    const Vector2 ctr{tr.x + tr.width * 0.5f, tr.y + tr.height * 0.5f};
                    DrawPolyLines(ctr, 4, 10.0f, 0.0f, pal::WithAlpha(inlay, 175));
                    if (health > 0.35f) {
                        DrawCircleV(ctr, 2.2f, pal::WithAlpha(dp.ambient_accent, static_cast<unsigned char>(160.0f * health)));
                    }
                }
            } else if (id == eco::TileId::ZoneGateway) {
                // Soglia monumentale di passaggio verso una zona adiacente
                const eco::ZoneGateway* gw = FindZoneGatewayAtTile(world.overworld, r, c);
                const bool locked = (gw != nullptr && !world.IsZoneGatewayUnlocked(*gw));
                const Color dest_acc = (gw != nullptr) ? pal::GetDistrictPalette(gw->to_zone).ambient_accent : dp.ambient_accent;
                const Color acc = locked ? pal::kAccentWarn : dest_acc;
                const float pulse = 0.65f + 0.35f * std::sin(sim_time * 3.0f + static_cast<float>(r + c));
                const Color slab = pal::WorldColor(dp.sanctuary_sick, dp.sanctuary_healthy, poll);

                DrawRectangle(rx, ry, tile_px, tile_px, slab);
                if (locked) {
                    const float lock_pulse = 0.68f + 0.32f * std::sin(sim_time * 4.4f + static_cast<float>(r + c) * 0.6f);
                    const Color barrier_bg = pal::WithAlpha(pal::kAccentWarn, static_cast<unsigned char>(78.0f * lock_pulse));
                    const Color barrier_bar = pal::WithAlpha(pal::kSolarGateGold, static_cast<unsigned char>(225.0f * lock_pulse));
                    const Color barrier_core = pal::WithAlpha(pal::kSolarBeamCore, static_cast<unsigned char>(235.0f * lock_pulse));

                    DrawRectangle(rx + 2, ry + 2, tile_px - 4, tile_px - 4, barrier_bg);
                    DrawRectangleLines(rx + 2, ry + 2, tile_px - 4, tile_px - 4, pal::WithAlpha(pal::kBronzeBright, 225));

                    const bool horiz = (gw != nullptr) ? gw->IsHorizontalWall() : true;
                    static constexpr std::array<float, 3> kBarOffsets{{11.0f, 24.0f, 37.0f}};
                    for (float offset : kBarOffsets) {
                        if (horiz) {
                            DrawLineEx({tr.x + 4.0f, tr.y + offset},
                                       {tr.x + tr.width - 4.0f, tr.y + offset},
                                       2.8f, barrier_bar);
                            DrawLineEx({tr.x + 6.0f, tr.y + offset},
                                       {tr.x + tr.width - 6.0f, tr.y + offset},
                                       1.2f, barrier_core);
                        } else {
                            DrawLineEx({tr.x + offset, tr.y + 4.0f},
                                       {tr.x + offset, tr.y + tr.height - 4.0f},
                                       2.8f, barrier_bar);
                            DrawLineEx({tr.x + offset, tr.y + 6.0f},
                                       {tr.x + offset, tr.y + tr.height - 6.0f},
                                       1.2f, barrier_core);
                        }
                    }

                    const Vector2 ctr{tr.x + tr.width * 0.5f, tr.y + tr.height * 0.5f};
                    DrawPoly(ctr, 4, 11.0f, 0.0f, Color{24, 18, 14, 220});
                    DrawPoly(ctr, 4, 8.5f, 0.0f, barrier_bar);
                    DrawPolyLines(ctr, 4, 11.0f, 0.0f, pal::kBronzeBright);
                    DrawCircleV(ctr, 3.0f, barrier_core);
                } else {
                    DrawRectangle(rx + 3, ry + 3, tile_px - 6, tile_px - 6, pal::WithAlpha(acc, static_cast<unsigned char>(42.0f * pulse)));
                    DrawRectangleLines(rx + 2, ry + 2, tile_px - 4, tile_px - 4, pal::WithAlpha(pal::kBronze, 215));
                    DrawPolyLines({tr.x + 24.0f, tr.y + 24.0f}, 4, 11.0f, 0.0f, pal::WithAlpha(acc, static_cast<unsigned char>(190.0f * pulse)));
                }
            } else if (id == eco::TileId::ShallowChannel) {
                // Canale guadabile con letto di ciottoli visibile e acqua turchese traslucida
                const Color bed = pal::ShiftBrightness(pal::WorldColor(dp.path_sick, dp.path_healthy, poll), -18);
                const Color water_col = pal::LerpColor(bed, pal::WorldColor(dp.water_sick, dp.water_healthy, poll), 0.62f);
                const Color foam = pal::WorldColor(dp.foam_sick, dp.foam_healthy, poll);
                const Color curb = pal::WorldColor(dp.wall_top_sick, dp.wall_top_healthy, poll);

                DrawRectangle(rx, ry, tile_px, tile_px, water_col);
                // Ciottoli sommersi sul fondale (chiarisce che il canale e' guadabile)
                DrawCircle(rx + 14 + static_cast<int>(h % 8u), ry + 16 + static_cast<int>((h >> 4) % 8u), 4.5f, pal::ShiftBrightness(water_col, -12));
                DrawCircle(rx + 30 + static_cast<int>((h >> 8) % 6u), ry + 32 + static_cast<int>((h >> 12) % 6u), 4.0f, pal::ShiftBrightness(water_col, 10));

                const float shimmer = std::fmod(sim_time * 28.0f + static_cast<float>(r * 13 + c * 17), static_cast<float>(tile_px));
                DrawLineEx({tr.x + 6.0f, tr.y + shimmer}, {tr.x + tr.width - 6.0f, tr.y + shimmer}, 1.8f, pal::WithAlpha(foam, 145));

                // Bordi bassi scolpiti sui lati confinanti con terra
                if (!IsWaterLike(tilemap.GetTile(r - 1, c))) DrawRectangle(rx, ry, tile_px, 4, curb);
                if (!IsWaterLike(tilemap.GetTile(r + 1, c))) DrawRectangle(rx, ry + tile_px - 4, tile_px, 4, pal::ShiftBrightness(curb, -16));
                if (!IsWaterLike(tilemap.GetTile(r, c - 1))) DrawRectangle(rx, ry, 4, tile_px, curb);
                if (!IsWaterLike(tilemap.GetTile(r, c + 1))) DrawRectangle(rx + tile_px - 4, ry, 4, tile_px, pal::ShiftBrightness(curb, -16));
            } else if (id == eco::TileId::JadeBridge) {
                // Ponte di Giada dei Giardini Sospesi (si materializza con la risonanza armonica)
                const Color water_base = pal::WorldColor(dp.water_sick, dp.water_healthy, poll);
                const Color foam = pal::WorldColor(dp.foam_sick, dp.foam_healthy, poll);
                DrawRectangle(rx, ry, tile_px, tile_px, water_base);
                const float flow = std::fmod(sim_time * 18.0f + static_cast<float>(r * 15), static_cast<float>(tile_px));
                DrawLineEx({tr.x + 8.0f, tr.y + flow}, {tr.x + tr.width - 8.0f, tr.y + flow}, 1.6f, pal::WithAlpha(foam, 125));

                const float vis = jade_bridge_unlocked ? 1.0f : jade_progress;
                if (vis > 0.04f) {
                    const unsigned char a = static_cast<unsigned char>(std::clamp(vis * 240.0f, 30.0f, 240.0f));
                    const Color deck = pal::WithAlpha(pal::LerpColor(Color{42, 138, 114, 255}, pal::kJade, vis), a);
                    const Color rim = pal::WithAlpha(pal::kJadeGlow, a);
                    DrawRectangle(rx + 2, ry + 2, tile_px - 4, tile_px - 4, deck);
                    DrawRectangleLines(rx + 2, ry + 2, tile_px - 4, tile_px - 4, rim);
                    DrawPolyLines({tr.x + 24.0f, tr.y + 24.0f}, 6, 12.0f * vis, 30.0f, pal::WithAlpha(pal::kStoneIvory, a));
                    DrawCircleV({tr.x + 24.0f, tr.y + 24.0f}, 3.2f * vis, rim);
                } else {
                    // Piloni sommersi + traccia eterea del ponte bloccato
                    DrawCircle(rx + 12, ry + 12, 4.0f, pal::WithAlpha(pal::kJade, 75));
                    DrawCircle(rx + tile_px - 12, ry + tile_px - 12, 4.0f, pal::WithAlpha(pal::kJade, 75));
                    DrawRectangleLines(rx + 6, ry + 6, tile_px - 12, tile_px - 12, pal::WithAlpha(pal::kJadeGlow, 68));
                }
            } else if (id == eco::TileId::SolarGate) {
                // Barriera Solare delle Terrazze dell'Alba
                const Color slab = pal::WorldColor(dp.sanctuary_sick, dp.sanctuary_healthy, poll);
                DrawRectangle(rx, ry, tile_px, tile_px, slab);
                if (solar_gate_open) {
                    // Binari in ottone aperti e luminosi
                    DrawRectangle(rx + 6, ry + 6, tile_px - 12, tile_px - 12, pal::WithAlpha(pal::kJade, 48));
                    DrawRectangleLines(rx + 4, ry + 4, tile_px - 8, tile_px - 8, pal::WithAlpha(pal::kBronzeBright, 180));
                    DrawCircle(rx + 24, ry + 24, 3.2f, pal::kJadeGlow);
                } else {
                    // Barriera di luce solare bloccata con griglia d'oro e pilastri
                    const float pulse = 0.65f + 0.35f * std::sin(sim_time * 4.2f + static_cast<float>(c));
                    DrawRectangle(rx + 2, ry + 2, tile_px - 4, tile_px - 4, pal::WithAlpha(pal::kSolarGateGold, static_cast<unsigned char>(90.0f * pulse)));
                    DrawRectangle(rx, ry, 7, tile_px, pal::kBronzeDark);
                    DrawRectangle(rx + tile_px - 7, ry, 7, tile_px, pal::kBronzeDark);
                    for (int bar = 10; bar < tile_px; bar += 12) {
                        DrawLineEx({tr.x + 6.0f, tr.y + static_cast<float>(bar)},
                                   {tr.x + tr.width - 6.0f, tr.y + static_cast<float>(bar)},
                                   2.6f, pal::WithAlpha(pal::kSolarBeamCore, static_cast<unsigned char>(210.0f * pulse)));
                    }
                    DrawPoly({tr.x + 24.0f, tr.y + 24.0f}, 8, 8.0f, 22.5f, pal::kSolarGateGold);
                    DrawCircleV({tr.x + 24.0f, tr.y + 24.0f}, 3.5f, pal::kSolarBeamCore);
                }
            } else if (id == eco::TileId::Water) {
                const Color water_base = pal::WorldColor(dp.water_sick, dp.water_healthy, poll);
                const Color water_deep = pal::ShiftBrightness(water_base, -14);
                const Color water_shimmer = pal::ShiftBrightness(water_base, 24);
                const Color foam = pal::WorldColor(dp.foam_sick, dp.foam_healthy, poll);
                const Color bank = pal::WorldColor(dp.wall_top_sick, dp.wall_top_healthy, poll);

                const bool wn = IsWaterLike(tilemap.GetTile(r - 1, c));
                const bool ws = IsWaterLike(tilemap.GetTile(r + 1, c));
                const bool ww = IsWaterLike(tilemap.GetTile(r, c - 1));
                const bool we = IsWaterLike(tilemap.GetTile(r, c + 1));

                DrawRectangle(rx, ry, tile_px, tile_px, water_base);
                if (!wn) DrawRectangle(rx, ry + 6, tile_px, 7, water_deep);
                if (!ww) DrawRectangle(rx + 6, ry, 6, tile_px, water_deep);

                const float flow = std::fmod(sim_time * 22.0f + static_cast<float>(r * 17 + c * 9), static_cast<float>(tile_px));
                const float wy = tr.y + flow;
                const float wave_dx = std::sin(sim_time * 2.6f + static_cast<float>(r + c)) * 4.0f;
                DrawLineEx({tr.x + 10.0f + wave_dx, wy}, {tr.x + tr.width - 10.0f + wave_dx, wy},
                           2.0f, pal::WithAlpha(foam, static_cast<unsigned char>(115.0f + 80.0f * health)));

                const float wy2 = tr.y + std::fmod(flow + 24.0f, static_cast<float>(tile_px));
                DrawLineEx({tr.x + 14.0f - wave_dx * 0.7f, wy2}, {tr.x + tr.width - 14.0f - wave_dx * 0.7f, wy2},
                           1.5f, pal::WithAlpha(water_shimmer, 140));

                if (((h >> 5) & 3u) == 0u && health > 0.25f) {
                    const float glint = 0.5f + 0.5f * std::sin(sim_time * 4.2f + static_cast<float>(c * 3 + r * 5));
                    const float gx = tr.x + 14.0f + static_cast<float>((h >> 8) % 20u);
                    const float gy = tr.y + 14.0f + static_cast<float>((h >> 13) % 20u);
                    DrawCircleV({gx, gy}, 1.8f + glint * 1.2f, pal::WithAlpha(foam, static_cast<unsigned char>(190.0f * glint * health)));
                }

                if (!wn) {
                    DrawRectangle(rx, ry, tile_px, 6, bank);
                    DrawRectangle(rx, ry + 6, tile_px, 5, Color{0, 0, 0, 62});
                    DrawLine(rx, ry + 6, rx + tile_px, ry + 6, foam);
                }
                if (!ws) {
                    DrawRectangle(rx, ry + tile_px - 5, tile_px, 5, pal::ShiftBrightness(bank, -16));
                    DrawLine(rx, ry + tile_px - 5, rx + tile_px, ry + tile_px - 5, pal::WithAlpha(foam, 160));
                }
                if (!ww) {
                    DrawRectangle(rx, ry, 6, tile_px, bank);
                    DrawRectangle(rx + 6, ry, 4, tile_px, Color{0, 0, 0, 52});
                    DrawLine(rx + 6, ry, rx + 6, ry + tile_px, foam);
                }
                if (!we) {
                    DrawRectangle(rx + tile_px - 6, ry, 6, tile_px, pal::ShiftBrightness(bank, -12));
                    DrawLine(rx + tile_px - 6, ry, rx + tile_px - 6, ry + tile_px, pal::WithAlpha(foam, 165));
                }

                if (health > 0.35f && zone != eco::ZoneId::FumaroleCaldera) {
                    if (((h >> 7) & 7u) == 0u && wn && ws && ww && we) {
                        const float lx = tr.x + 24.0f + std::sin(sim_time + static_cast<float>(r)) * 1.5f;
                        const float ly = tr.y + 24.0f;
                        DrawCircleSector({lx, ly}, 7.0f, 25.0f, 340.0f, 12, Color{42, 146, 88, 225});
                        if (health > 0.6f) {
                            DrawCircleV({lx - 1.0f, ly - 1.0f}, 2.8f, dp.blossom_b);
                        }
                    }
                    if (!ww && ((h >> 11) & 3u) == 0u) {
                        const float sway = std::sin(sim_time * 2.4f + static_cast<float>(r)) * 2.0f;
                        DrawLineEx({tr.x + 8.0f, tr.y + 34.0f}, {tr.x + 8.0f + sway, tr.y + 18.0f}, 2.0f, Color{54, 138, 76, 225});
                        DrawLineEx({tr.x + 11.0f, tr.y + 36.0f}, {tr.x + 12.0f + sway, tr.y + 22.0f}, 1.8f, Color{68, 162, 92, 225});
                    }
                }

                if (wn && ws && !ww && !we && (r % 5 == 2)) {
                    const Color arch_stone = pal::WorldColor(dp.wall_top_sick, dp.wall_top_healthy, poll);
                    const Color arch_dark = pal::WorldColor(dp.wall_front_sick, dp.wall_front_healthy, poll);
                    DrawRectangle(rx + 2, ry + 26, tile_px - 4, 10, Color{0, 0, 0, 72});
                    DrawRectangle(rx, ry + 14, tile_px, 12, arch_stone);
                    DrawRectangle(rx, ry + 24, tile_px, 4, arch_dark);
                    DrawLine(rx, ry + 14, rx + tile_px, ry + 14, pal::ShiftBrightness(arch_stone, 22));
                    DrawLineEx({tr.x + 2.0f, tr.y + 19.0f}, {tr.x + tr.width - 2.0f, tr.y + 19.0f}, 2.2f,
                               pal::WithAlpha(foam, static_cast<unsigned char>(130.0f + 80.0f * health)));
                }
            } else if (id == eco::TileId::HiddenBridge) {
                const Color water_base = pal::WorldColor(dp.water_sick, dp.water_healthy, poll);
                const Color foam = pal::WorldColor(dp.foam_sick, dp.foam_healthy, poll);
                DrawRectangle(rx, ry, tile_px, tile_px, water_base);
                const float flow = std::fmod(sim_time * 20.0f + static_cast<float>(r * 19), static_cast<float>(tile_px));
                DrawLineEx({tr.x + 8.0f, tr.y + flow}, {tr.x + tr.width - 8.0f, tr.y + flow}, 1.8f, pal::WithAlpha(foam, 140));

                const bool bn = (tilemap.GetTile(r - 1, c) == eco::TileId::HiddenBridge);
                const bool bs = (tilemap.GetTile(r + 1, c) == eco::TileId::HiddenBridge);

                if (bridge_unlocked) {
                    DrawRectangle(rx + 2, ry + tile_px - 2, tile_px - 2, 7, Color{0, 0, 0, 80});
                    const Color deck = pal::WorldColor(Color{116, 108, 96, 255}, Color{176, 174, 162, 255}, poll);
                    const Color rail = pal::WorldColor(Color{96, 86, 76, 255}, Color{198, 194, 180, 255}, poll);
                    DrawRectangle(rx - 2, ry, tile_px + 4, tile_px, deck);
                    for (int bx = rx + 8; bx < rx + tile_px; bx += 12) {
                        DrawLine(bx, ry + 4, bx, ry + tile_px - 4, pal::ShiftBrightness(deck, -22));
                    }
                    DrawRectangle(rx - 2, ry + tile_px / 2 - 4, tile_px + 4, 8, pal::kBronzeDark);
                    DrawLineEx({tr.x - 2.0f, tr.y + tr.height * 0.5f},
                               {tr.x + tr.width + 2.0f, tr.y + tr.height * 0.5f}, 2.8f, pal::kJade);
                    if (!bn) {
                        DrawRectangle(rx - 2, ry, tile_px + 4, 7, rail);
                        DrawLineEx({tr.x - 2.0f, tr.y + 3.0f}, {tr.x + tr.width + 2.0f, tr.y + 3.0f}, 2.0f, pal::kBronzeBright);
                        DrawCircle(rx + 6, ry + 3, 3.0f, pal::kJadeGlow);
                        DrawCircle(rx + tile_px - 6, ry + 3, 3.0f, pal::kJadeGlow);
                    }
                    if (!bs) {
                        DrawRectangle(rx - 2, ry + tile_px - 7, tile_px + 4, 7, pal::ShiftBrightness(rail, -24));
                        DrawLineEx({tr.x - 2.0f, tr.y + tr.height - 4.0f}, {tr.x + tr.width + 2.0f, tr.y + tr.height - 4.0f}, 2.0f, pal::kBronze);
                    }
                } else {
                    const Color rubble = pal::WorldColor(Color{92, 82, 74, 255}, Color{134, 132, 124, 255}, poll);
                    DrawRectangle(rx, ry + 3, 11, tile_px - 6, rubble);
                    DrawRectangle(rx + tile_px - 11, ry + 3, 11, tile_px - 6, rubble);
                    DrawRectangleLines(rx, ry + 3, 11, tile_px - 6, pal::ShiftBrightness(rubble, -24));
                    DrawRectangleLines(rx + tile_px - 11, ry + 3, 11, tile_px - 6, pal::ShiftBrightness(rubble, -24));

                    const float pulse = 0.55f + 0.45f * std::sin(sim_time * 3.5f);
                    const Color seal = pal::WithAlpha(pal::kAmberGlow, static_cast<unsigned char>(120.0f + 100.0f * pulse));
                    DrawLineEx({tr.x + 11.0f, tr.y + 10.0f}, {tr.x + tr.width - 11.0f, tr.y + 14.0f}, 2.0f, pal::kBronzeDark);
                    DrawLineEx({tr.x + 11.0f, tr.y + tr.height - 10.0f}, {tr.x + tr.width - 11.0f, tr.y + tr.height - 14.0f}, 2.0f, pal::kBronzeDark);
                    DrawLineEx({tr.x + 10.0f, tr.y}, {tr.x + 10.0f, tr.y + tr.height}, 2.6f, seal);
                    DrawLineEx({tr.x + tr.width - 10.0f, tr.y}, {tr.x + tr.width - 10.0f, tr.y + tr.height}, 2.6f, seal);
                    DrawCircleV({tr.x + tr.width * 0.5f, tr.y + tr.height * 0.5f}, 4.5f, pal::WithAlpha(seal, 180));
                }
            } else if (id == eco::TileId::StructurePortal) {
                const eco::StructureId sid = PortalStructureAtTile(world.overworld, r, c);
                const bool left_portal = (tilemap.GetTile(r, c - 1) == eco::TileId::StructurePortal);
                const bool right_portal = (tilemap.GetTile(r, c + 1) == eco::TileId::StructurePortal);
                DrawStructurePortalTile(rx, ry, tile_px, sid, left_portal, right_portal, poll, sim_time);
            } else if (id == eco::TileId::ExitPortal) {
                DrawRectangle(rx, ry, tile_px, tile_px, pal::kBronzeDark);
                DrawRectangle(rx + 4, ry + 4, tile_px - 8, tile_px - 8, pal::kBronze);
                DrawRectangleLines(rx + 2, ry + 2, tile_px - 4, tile_px - 4, pal::kAmberGlow);
            }
        }
    }

    // -------------------------------------------------------------------------
    // PASSATA 1B: Grandi Mosaici Astronomici/Armonici nelle Piazze dei Monumenti
    // -------------------------------------------------------------------------
    world.ForEachMachineInScene([&](const eco::Machine& m) {
        if (std::fabs(m.pos.x - cam.x) > half_w + 160.0f || std::fabs(m.pos.y - cam.y) > half_h + 160.0f) {
            return;
        }
        if (m.type == eco::MachineType::SolarArray) {
            DrawSolarSundialMosaic(m.pos, poll, sim_time);
        } else if (m.type == eco::MachineType::HarmonicResonator) {
            DrawHarmonicWaveMosaic(m.pos, poll, sim_time);
        } else if (m.type == eco::MachineType::CentralAstrolabe) {
            DrawSanctuaryGrandMosaic(m.pos, poll, sim_time, world.collectibles.CollectedCount(),
                                     world.zone_mechanics.AllSealsAttuned());
        }
    });
    if (zone == eco::ZoneId::WindPlateau) {
        if (const eco::PortalLink* tower_portal = world.overworld.FindPortal(eco::StructureId::PendulumTower)) {
            const eco::Vec2 plaza{tower_portal->exit_spawn.x, tower_portal->exit_spawn.y + 18.0f};
            if (std::fabs(plaza.x - cam.x) <= half_w + 100.0f && std::fabs(plaza.y - cam.y) <= half_h + 100.0f) {
                DrawWindRoseMosaic(plaza, poll, sim_time);
            }
        }
    }

    // -------------------------------------------------------------------------
    // PASSATA 1C: Elementi a terra delle 6 Meccaniche Top-Down
    // -------------------------------------------------------------------------
    DrawZoneMechanicsGround(world, cam, half_w, half_h);

    // -------------------------------------------------------------------------
    // PASSATA 2: Murature 2.5D Auto-Tiled & Rovine Stratificate con Ombre Morbide
    // -------------------------------------------------------------------------
    for (int r = min_row; r <= max_row; ++r) {
        const int row_cols = tilemap.RowCols(r);
        const int c_end = std::min(max_col, row_cols - 1);
        for (int c = min_col; c <= c_end; ++c) {
            const eco::TileId id = tilemap.GetTile(r, c);
            if (!IsWallLike(id)) {
                continue;
            }

            const eco::Rect tr = tilemap.TileRect(r, c);
            const int rx = static_cast<int>(tr.x);
            const int ry = static_cast<int>(tr.y);
            const std::uint32_t h = Hash2(c, r, 91 + static_cast<int>(zone) * 13);

            const bool wn = IsWallLike(tilemap.GetTile(r - 1, c));
            const bool ws = IsWallLike(tilemap.GetTile(r + 1, c));
            const bool ww = IsWallLike(tilemap.GetTile(r, c - 1));
            const bool we = IsWallLike(tilemap.GetTile(r, c + 1));

            if (!ws && r + 1 < tilemap.Rows()) {
                DrawRectangle(rx + 4, ry + tile_px, tile_px, 12, Color{0, 0, 0, 78});
                DrawRectangle(rx + 8, ry + tile_px + 12, tile_px - 2, 6, Color{0, 0, 0, 34});
            }
            if (!we && c + 1 < row_cols) {
                DrawRectangle(rx + tile_px, ry + 5, 9, tile_px - 2, Color{0, 0, 0, 62});
                DrawRectangle(rx + tile_px + 9, ry + 8, 5, tile_px - 4, Color{0, 0, 0, 26});
            }

            const Color top_col = pal::ShiftBrightness(
                pal::WorldColor(dp.wall_top_sick, dp.wall_top_healthy, poll),
                static_cast<int>(h % 9u) - 4);
            const Color front_col = pal::WorldColor(dp.wall_front_sick, dp.wall_front_healthy, poll);
            const Color edge_col = pal::WorldColor(dp.wall_edge_sick, dp.wall_edge_healthy, poll);
            const Color mortar = pal::ShiftBrightness(front_col, -16);

            const int front_h = ws ? 0 : 15;
            const int top_h = tile_px - front_h;

            if (id == eco::TileId::RuinsWall) {
                const Color ground_under = pal::WorldColor(dp.ground_sick, dp.ground_healthy, poll);
                DrawRectangle(rx, ry, tile_px, tile_px, ground_under);

                const unsigned variant = h % 3u;
                if (!wn && !ws && !ww && !we) {
                    DrawEllipse(rx + 28, ry + 39, 17.0f, 8.0f, Color{0, 0, 0, 92});
                    DrawRectangle(rx + 10, ry + 14, 28, 25, front_col);
                    DrawEllipse(rx + 24, ry + 39, 14.0f, 6.0f, front_col);
                    DrawEllipse(rx + 24, ry + 14, 15.0f, 7.0f, top_col);
                    DrawEllipseLines(rx + 24, ry + 14, 15.0f, 7.0f, edge_col);
                    DrawLine(rx + 16, ry + 18, rx + 16, ry + 39, pal::ShiftBrightness(front_col, 24));
                    DrawLine(rx + 24, ry + 20, rx + 24, ry + 41, pal::ShiftBrightness(front_col, 24));
                    DrawLine(rx + 32, ry + 18, rx + 32, ry + 39, pal::ShiftBrightness(front_col, 24));
                    DrawRing({tr.x + 24.0f, tr.y + 14.0f}, 6.0f, 8.5f, 0.0f, 360.0f, 16, pal::WithAlpha(pal::kBronze, 195));
                    if (zone == eco::ZoneId::HarmonySanctuary) {
                        const float pulse = 0.7f + 0.3f * std::sin(sim_time * 3.0f + static_cast<float>(c + r));
                        DrawCircleV({tr.x + 24.0f, tr.y + 13.0f}, 9.0f * pulse,
                                    pal::WithAlpha(pal::kJadeGlow, static_cast<unsigned char>(45.0f + 75.0f * health)));
                        DrawCircleV({tr.x + 24.0f, tr.y + 13.0f}, 3.4f, pal::kJadeGlow);
                    } else if (zone == eco::ZoneId::HangingGardens && health > 0.35f) {
                        DrawCircleV({tr.x + 17.0f, tr.y + 22.0f}, 3.2f, dp.canopy_healthy);
                        DrawCircleV({tr.x + 31.0f, tr.y + 28.0f}, 3.0f, dp.canopy_hi_healthy);
                        DrawCircleV({tr.x + 17.0f, tr.y + 22.0f}, 1.8f, dp.blossom_a);
                        DrawCircleV({tr.x + 31.0f, tr.y + 28.0f}, 1.8f, dp.blossom_b);
                    }
                } else {
                    const int inset_l = ww ? 0 : 3;
                    const int inset_r = we ? 0 : 3;
                    const int inset_t = wn ? 0 : 3;
                    DrawRectangle(rx + inset_l, ry + inset_t, tile_px - inset_l - inset_r, top_h - inset_t, top_col);
                    if (front_h > 0) {
                        DrawRectangle(rx + inset_l, ry + top_h, tile_px - inset_l - inset_r, front_h, front_col);
                        DrawLine(rx + inset_l, ry + top_h, rx + tile_px - inset_r, ry + top_h, edge_col);
                        DrawLine(rx + inset_l + 4, ry + top_h + 7, rx + tile_px - inset_r - 4, ry + top_h + 7,
                                 pal::WithAlpha(pal::WorldColor(dp.inlay_sick, dp.inlay_healthy, poll), 175));
                    }
                    const std::uint32_t ch = Hash2(c, r, 733);
                    const int joint_x = rx + 14 + static_cast<int>(ch % 20u);
                    DrawLine(joint_x, ry + inset_t, joint_x, ry + top_h, pal::WithAlpha(mortar, 110));
                    if ((ch & 1u) == 0u) {
                        const int cx0 = rx + 8 + static_cast<int>((ch >> 3) % 14u);
                        const int cy0 = ry + 6 + static_cast<int>((ch >> 7) % 10u);
                        const int cx1 = cx0 + 8 + static_cast<int>((ch >> 11) % 10u);
                        const int cy1 = cy0 + 7 + static_cast<int>((ch >> 15) % 8u);
                        DrawLine(cx0, cy0, cx1, cy1, pal::WithAlpha(mortar, 175));
                        DrawLine(cx1, cy1, cx1 - 4, std::min(ry + top_h - 2, cy1 + 8), pal::WithAlpha(mortar, 150));
                    }
                    if (!wn) DrawLine(rx + inset_l, ry + inset_t, rx + tile_px - inset_r, ry + inset_t, edge_col);
                    if (!ww) DrawLine(rx + inset_l, ry + inset_t, rx + inset_l, ry + tile_px - 1, edge_col);

                    if (zone == eco::ZoneId::FumaroleCaldera && variant == 2u) {
                        const Color flue = pal::WorldColor(Color{206, 82, 36, 200}, Color{92, 178, 172, 165}, poll);
                        DrawCircle(rx + 24, ry + top_h / 2, 10.0f, pal::kBasaltDark);
                        DrawCircleLines(rx + 24, ry + top_h / 2, 10.0f, pal::kBronze);
                        DrawCircle(rx + 24, ry + top_h / 2, 5.0f, pal::WithAlpha(flue, 195));
                    } else if (variant == 1u) {
                        DrawCircleLines(rx + 24, ry + top_h / 2, 8.0f, pal::WithAlpha(pal::kBronze, 165));
                    }
                }
            } else {
                DrawRectangle(rx, ry, tile_px, top_h, top_col);
                if (front_h > 0) {
                    DrawRectangle(rx, ry + top_h, tile_px, front_h, front_col);
                    DrawLine(rx + 16, ry + top_h, rx + 16, ry + tile_px, mortar);
                    DrawLine(rx + 36, ry + top_h, rx + 36, ry + tile_px, mortar);
                    DrawLine(rx, ry + top_h + 7, rx + tile_px, ry + top_h + 7, pal::WithAlpha(mortar, 150));
                    DrawLine(rx, ry + top_h, rx + tile_px, ry + top_h, pal::ShiftBrightness(edge_col, -12));
                    DrawLine(rx, ry + tile_px - 1, rx + tile_px, ry + tile_px - 1, mortar);
                }

                DrawLine(rx + 24, ry, rx + 24, ry + top_h, pal::WithAlpha(mortar, 85));
                if (!wn) {
                    DrawLine(rx, ry, rx + tile_px, ry, edge_col);
                    DrawLine(rx, ry + 1, rx + tile_px, ry + 1, pal::WithAlpha(edge_col, 140));
                }
                if (!ww) DrawLine(rx, ry, rx, ry + tile_px, edge_col);
                if (!we) DrawLine(rx + tile_px - 1, ry, rx + tile_px - 1, ry + tile_px, mortar);

                if (!ws && (zone == eco::ZoneId::HarmonySanctuary || zone == eco::ZoneId::CanalQuarter || zone == eco::ZoneId::DawnTerraces) && ((c & 1) == 0)) {
                    const Color band = pal::WorldColor(dp.inlay_sick, dp.inlay_healthy, poll);
                    DrawRectangle(rx + 10, ry + top_h + 4, tile_px - 20, 3, pal::WithAlpha(band, 190));
                }
            }

            if (!ws && health > 0.25f && (h & 3u) != 0u && zone != eco::ZoneId::FumaroleCaldera) {
                const Color moss = pal::WorldColor(Color{74, 84, 56, 175}, dp.canopy_healthy, poll);
                const Color moss_hi = pal::WorldColor(Color{96, 104, 68, 175}, dp.canopy_hi_healthy, poll);
                const int mx = rx + 6 + static_cast<int>((h >> 5) % 24u);
                const int mw = 9 + static_cast<int>(health * 11.0f);
                DrawRectangle(mx, ry + top_h - 3, mw, 5, moss);
                DrawCircle(mx + mw / 2, ry + top_h + 3, 3.8f * health, moss);
                DrawCircle(mx + mw / 3, ry + top_h + 1, 2.2f * health, moss_hi);
                if (health > 0.62f && ((h >> 12) & 1u) == 0u) {
                    DrawCircle(mx + mw / 2 + 2, ry + top_h + 2, 1.9f, dp.blossom_a);
                }
            }
        }
    }
}

// =============================================================================
// Overworld Props — Varchi, Affreschi, Flora, Meccaniche Elevate & Stele
// =============================================================================
void DrawOverworldProceduralProps(const eco::WorldContext& world) {
    const eco::ZoneId zone = world.overworld.active_zone;
    const pal::DistrictPalette& dp = pal::GetDistrictPalette(zone);
    const float poll = world.ecosystem.PollutionRatio();
    const float health = 1.0f - poll;
    const float sim_time = world.sim_time;
    const eco::Vec2 cam = world.overworld.camera_target;
    const float half_w = static_cast<float>(eco::kWindowWidth) * 0.5f + 120.0f;
    const float half_h = static_cast<float>(eco::kWindowHeight) * 0.5f + 120.0f;
    const float wind_gust = (zone == eco::ZoneId::WindPlateau) ? world.zone_mechanics.wind.gust_intensity : 0.0f;

    RebuildZoneCacheIfNeeded(world);

    // 0. Varchi monumentali tra la zona attiva e le zone adiacenti
    for (const auto& gw : world.overworld.active_gateways) {
        if (std::fabs(gw.world_center.x - cam.x) > half_w + 160.0f ||
            std::fabs(gw.world_center.y - cam.y) > half_h + 160.0f) {
            continue;
        }
        DrawZoneGatewayArch(gw, world);
    }

    // 1. Affreschi ambientali dedicati alla zona attiva
    for (const auto& f : g_fresco_cache) {
        if (std::fabs(f.x - cam.x) > half_w || std::fabs(f.y - cam.y) > half_h) {
            continue;
        }
        switch (f.kind) {
            case FrescoKind::SundialObelisk:
                DrawSundialObelisk(f.x, f.y, poll, sim_time, dp);
                if (f.caption != nullptr && f.caption[0] != '\0') {
                    DrawMeridianStele(f.x, f.y - 30.0f, f.caption, dp.ambient_accent, health);
                }
                break;
            case FrescoKind::HydroFountain:
                DrawMeridianStatue(f.x, f.y, true, poll, sim_time, dp);
                if (f.caption != nullptr && f.caption[0] != '\0') {
                    DrawMeridianStele(f.x, f.y - 32.0f, f.caption, dp.ambient_accent, health);
                }
                break;
            case FrescoKind::WindStele:
                DrawCarvedBasReliefPlaque(f.x, f.y, poll, dp, f.caption);
                break;
            case FrescoKind::AcousticBasin:
                DrawAcousticJadeBasin(f.x, f.y, poll, sim_time, dp);
                if (f.caption != nullptr && f.caption[0] != '\0') {
                    DrawMeridianStele(f.x, f.y - 26.0f, f.caption, dp.ambient_accent, health);
                }
                break;
            case FrescoKind::ThermalBrazier:
                DrawGeothermalBrazier(f.x, f.y, poll, sim_time);
                if (f.caption != nullptr && f.caption[0] != '\0') {
                    DrawMeridianStele(f.x, f.y - 24.0f, f.caption, dp.ambient_accent, health);
                }
                break;
            case FrescoKind::SanctuaryStatue:
                DrawMeridianStatue(f.x, f.y, false, poll, sim_time, dp);
                if (f.caption != nullptr && f.caption[0] != '\0') {
                    DrawMeridianStele(f.x, f.y - 38.0f, f.caption, dp.ambient_accent, health);
                }
                break;
            case FrescoKind::BasRelief:
                DrawCarvedBasReliefPlaque(f.x, f.y, poll, dp, f.caption);
                break;
        }
    }

    // 2. Prop statici pre-calcolati SOLO per la zona attiva
    for (const StaticProp& sp : g_prop_cache) {
        if (std::fabs(sp.x - cam.x) > half_w || std::fabs(sp.y - cam.y) > half_h) {
            continue;
        }
        const float px = sp.x;
        const float py = sp.y;
        const std::uint32_t h = sp.h;
        const float scale = sp.scale;
        const unsigned roll = sp.roll;

        if (roll == 0u) {
            DrawBioCrystalNode(px, py, sim_time, poll, dp.ambient_accent);
        } else if (roll == 1u) {
            DrawJadeLanternPillar(px, py, sim_time, poll, dp.ambient_accent);
        } else if (roll == 2u && (zone == eco::ZoneId::DawnTerraces || zone == eco::ZoneId::HarmonySanctuary || zone == eco::ZoneId::WindPlateau)) {
            DrawBrokenColumnDrum(px, py, poll, dp);
        } else if (roll == 3u && zone != eco::ZoneId::FumaroleCaldera) {
            const bool hydro = (zone == eco::ZoneId::CanalQuarter || zone == eco::ZoneId::HangingGardens);
            DrawMeridianStatue(px, py, hydro, poll, sim_time, dp);
        } else if (roll == 4u && (zone == eco::ZoneId::HangingGardens || zone == eco::ZoneId::HarmonySanctuary)) {
            DrawStonePlanterUrn(px, py, poll, sim_time, dp);
        } else if (roll == 4u && (zone == eco::ZoneId::DawnTerraces || zone == eco::ZoneId::WindPlateau)) {
            DrawSundialObelisk(px, py, poll, sim_time, dp);
        } else {
            switch (zone) {
                case eco::ZoneId::DawnTerraces:
                case eco::ZoneId::HarmonySanctuary:
                    DrawOliveTree(px, py, scale, sim_time, poll, dp, static_cast<int>(h & 0xFFu));
                    break;
                case eco::ZoneId::WindPlateau:
                case eco::ZoneId::CanalQuarter:
                    DrawWindCypress(px, py, scale, sim_time, poll, dp, static_cast<int>(h & 0xFFu), wind_gust);
                    break;
                case eco::ZoneId::HangingGardens:
                    DrawFloweringGardenTree(px, py, scale, sim_time, poll, dp, static_cast<int>(h & 0xFFu));
                    break;
                case eco::ZoneId::FumaroleCaldera:
                    if ((h & 1u) == 0u) {
                        DrawBasaltThermalVent(px, py, scale, sim_time, poll, static_cast<int>(h & 0xFFu));
                    } else {
                        DrawGeothermalBrazier(px, py, poll, sim_time);
                    }
                    break;
                case eco::ZoneId::Count:
                    break;
            }
        }
    }

    // 3. Elementi elevati delle 6 meccaniche top-down (specchi/fasci, leve, monoliti/raffiche, getti/valvole, cristalli)
    DrawZoneMechanicsElevated(world, cam, half_w, half_h);

    // 4. Stele Diegetiche dei Portali Struttura presenti nella zona attiva
    for (const auto& portal : world.overworld.portals) {
        const float sx = portal.world_center.x;
        const float sy = static_cast<float>(portal.tile_min.row) * world.overworld.tilemap.TileSize() - 10.0f;
        if (std::fabs(sx - cam.x) > half_w || std::fabs(sy - cam.y) > half_h) {
            continue;
        }
        Color accent = pal::kJade;
        const char* label = "CISTERNE IPOGEE";
        if (portal.target_structure == eco::StructureId::PendulumTower) {
            accent = pal::kAmberGlow;
            label = "TORRE DEL PENDOLO";
        } else if (portal.target_structure == eco::StructureId::GeothermalChamber) {
            accent = pal::kAccentDirty;
            label = "CAMERA GEOTERMICA";
        }
        DrawMeridianStele(sx, sy, label, accent, health);
    }
}

// =============================================================================
// Strutture Interne Side-Scrolling (3 architetture uniche in parallasse + 2.5D)
// =============================================================================
void DrawStructureProceduralBackdropAndMap(
    const eco::WorldContext& world,
    eco::Vec2 camera_target) {
    const eco::Tilemap& tilemap = world.structure.tilemap;
    const eco::StructureId sid = world.structure.active_structure;
    const pal::StructurePalette& sp = pal::GetStructurePalette(sid);
    const float poll = world.ecosystem.PollutionRatio();
    const float health = 1.0f - poll;
    const float sim_time = world.sim_time;
    const int world_w = static_cast<int>(tilemap.WorldWidth());
    const int world_h = static_cast<int>(tilemap.WorldHeight());
    const float parallax_x = (camera_target.x - static_cast<float>(world_w) * 0.5f) * 0.20f;

    if (sid == eco::StructureId::HypogeanCisterns) {
        DrawCisternsBackdrop(world_w, world_h, parallax_x, poll, sim_time, sp);
    } else if (sid == eco::StructureId::PendulumTower) {
        DrawPendulumTowerBackdrop(world_w, world_h, parallax_x, poll, sim_time, sp);
    } else {
        DrawGeothermalBackdrop(world_w, world_h, parallax_x, poll, sim_time, sp);
    }

    const int tile_px = static_cast<int>(tilemap.TileSize());
    if (tile_px <= 0 || tilemap.Rows() <= 0 || tilemap.Cols() <= 0) {
        return;
    }
    const Color wall_top = pal::WorldColor(sp.wall_top_sick, sp.wall_top_healthy, poll);
    const Color wall_dark = pal::WorldColor(sp.wall_dark_sick, sp.wall_dark_healthy, poll);
    const Color plat_top = pal::WorldColor(sp.plat_top_sick, sp.plat_top_healthy, poll);
    const Color plat_body = pal::WorldColor(sp.plat_body_sick, sp.plat_body_healthy, poll);
    const Color plat_rim = pal::WorldColor(sp.plat_rim_sick, sp.plat_rim_healthy, poll);
    const Color accent = pal::WorldColor(sp.accent_sick, sp.accent_healthy, poll);

    const float ts = tilemap.TileSize();
    const int s_min_col = std::max(0, static_cast<int>(std::floor((camera_target.x - static_cast<float>(eco::kWindowWidth) * 0.5f) / ts)) - 2);
    const int s_max_col = static_cast<int>(std::ceil((camera_target.x + static_cast<float>(eco::kWindowWidth) * 0.5f) / ts)) + 2;
    const int s_min_row = std::max(0, static_cast<int>(std::floor((camera_target.y - static_cast<float>(eco::kWindowHeight) * 0.5f) / ts)) - 2);
    const int s_max_row = std::min(tilemap.Rows() - 1, static_cast<int>(std::ceil((camera_target.y + static_cast<float>(eco::kWindowHeight) * 0.5f) / ts)) + 2);

    for (int r = s_min_row; r <= s_max_row; ++r) {
        const int row_cols = tilemap.RowCols(r);
        const int c_last = std::min(s_max_col, row_cols - 1);
        for (int c = s_min_col; c <= c_last; ++c) {
            const eco::Rect tr = tilemap.TileRect(r, c);
            const int rx = static_cast<int>(tr.x);
            const int ry = static_cast<int>(tr.y);
            const eco::TileId id = tilemap.GetTile(r, c);
            const std::uint32_t h = Hash2(c, r, 557);

            if (id == eco::TileId::Wall) {
                const bool is_perimeter =
                    (r <= 1 || r >= tilemap.Rows() - 2 || c <= 1 || c >= row_cols - 2);
                if (!is_perimeter) {
                    DrawRectangle(rx + 2, ry + tile_px, tile_px - 2, 8, Color{0, 0, 0, 88});
                    DrawRectangle(rx, ry, tile_px, 13, plat_top);
                    DrawRectangle(rx, ry + 13, tile_px, tile_px - 13, plat_body);

                    if (sid == eco::StructureId::HypogeanCisterns) {
                        DrawRing({tr.x + tr.width * 0.5f, tr.y + tr.height}, 14.0f, 18.0f, 180.0f, 360.0f, 12, pal::WithAlpha(pal::kBronze, 180));
                        DrawLine(rx + 24, ry, rx + 24, ry + 13, pal::ShiftBrightness(plat_top, -22));
                    } else if (sid == eco::StructureId::PendulumTower) {
                        for (int tx = rx + 6; tx < rx + tile_px - 4; tx += 8) {
                            DrawLine(tx, ry + 14, tx, ry + 26, pal::WithAlpha(plat_rim, 165));
                        }
                        DrawLine(rx + 4, ry + 28, rx + tile_px - 4, ry + 28, pal::WithAlpha(pal::kBronzeBright, 150));
                    } else {
                        DrawLine(rx + 4, ry + 15, rx + tile_px - 4, ry + tile_px - 4, pal::ShiftBrightness(plat_body, 24));
                        DrawLine(rx + tile_px - 4, ry + 15, rx + 4, ry + tile_px - 4, pal::ShiftBrightness(plat_body, 24));
                        DrawCircle(rx + 6, ry + 7, 2.0f, pal::kBronzeBright);
                        DrawCircle(rx + tile_px - 6, ry + 7, 2.0f, pal::kBronzeBright);
                    }

                    DrawRectangleLines(rx, ry, tile_px, tile_px, pal::ShiftBrightness(plat_body, -16));
                    DrawLineEx({tr.x + 1.0f, tr.y + 2.5f}, {tr.x + tr.width - 1.0f, tr.y + 2.5f}, 2.4f, plat_rim);

                    if ((c % 3 == 1) && r + 1 < tilemap.Rows() && !IsWallLike(tilemap.GetTile(r + 1, c))) {
                        const float lx = tr.x + tr.width * 0.5f;
                        const float ly = tr.y + tr.height + 11.0f;
                        const float pulse = 0.72f + 0.28f * std::sin(sim_time * 3.0f + static_cast<float>(c));
                        DrawLineEx({lx, tr.y + tr.height}, {lx, ly - 3.0f}, 1.5f, pal::kBronze);
                        DrawCircleV({lx, ly}, 12.0f * pulse, pal::WithAlpha(accent, static_cast<unsigned char>(45.0f + 40.0f * health)));
                        DrawCircleV({lx, ly}, 3.2f, accent);
                        DrawCircleV({lx, ly}, 1.4f, pal::kStoneIvory);
                    }
                } else {
                    const Color block = pal::ShiftBrightness(wall_top, static_cast<int>(h % 9u) - 4);
                    DrawRectangle(rx, ry, tile_px, tile_px, wall_dark);
                    DrawRectangle(rx + 2, ry + 2, tile_px - 4, tile_px - 8, block);
                    DrawLine(rx + 2, ry + 2, rx + tile_px - 2, ry + 2, pal::ShiftBrightness(block, 20));
                    DrawLine(rx + 24, ry + 2, rx + 24, ry + tile_px - 6, pal::ShiftBrightness(wall_dark, -8));
                    DrawRectangleLines(rx, ry, tile_px, tile_px, pal::ShiftBrightness(wall_dark, -10));

                    if (r == 0 && c > 0 && c < row_cols - 1 && (c % 2 == 0)) {
                        DrawRectangle(rx + 14, ry + tile_px, 20, 7, wall_dark);
                        DrawLine(rx + 14, ry + tile_px + 7, rx + 34, ry + tile_px + 7, pal::WithAlpha(pal::kBronze, 150));
                    }
                    if (r == tilemap.Rows() - 2 && c > 0 && c < row_cols - 1) {
                        DrawLineEx({tr.x, tr.y + 1.5f}, {tr.x + tr.width, tr.y + 1.5f}, 2.5f, pal::WithAlpha(accent, 215));
                    }
                }
            } else if (id == eco::TileId::SolarGate) {
                // Barriera energetica / paratoia dorata bloccata
                const float pulse = 0.65f + 0.35f * std::sin(sim_time * 4.2f + static_cast<float>(r + c));
                DrawRectangle(rx + 2, ry + 2, tile_px - 4, tile_px - 4,
                              pal::WithAlpha(pal::kSolarGateGold, static_cast<unsigned char>(92.0f * pulse)));
                DrawRectangle(rx, ry, 7, tile_px, pal::kBronzeDark);
                DrawRectangle(rx + tile_px - 7, ry, 7, tile_px, pal::kBronzeDark);
                DrawLine(rx + 7, ry, rx + 7, ry + tile_px, pal::WithAlpha(pal::kBronzeBright, 175));
                DrawLine(rx + tile_px - 7, ry, rx + tile_px - 7, ry + tile_px, pal::WithAlpha(pal::kBronzeBright, 175));
                for (int bar = 10; bar < tile_px; bar += 12) {
                    DrawLineEx(
                        {tr.x + 6.0f, tr.y + static_cast<float>(bar)},
                        {tr.x + tr.width - 6.0f, tr.y + static_cast<float>(bar)},
                        2.6f,
                        pal::WithAlpha(pal::kSolarBeamCore, static_cast<unsigned char>(215.0f * pulse)));
                }
                const Vector2 center{tr.x + tr.width * 0.5f, tr.y + tr.height * 0.5f};
                DrawPoly(center, 4, 8.5f, 0.0f, pal::kSolarGateGold);
                DrawCircleV(center, 3.4f, pal::kSolarBeamCore);
            } else if (id == eco::TileId::ExitPortal) {
                const float pulse = 0.5f + 0.5f * std::sin(sim_time * 3.6f);
                const Color glow = pal::LerpColor(pal::kBronze, pal::kAmberGlow, pulse);
                DrawRectangle(rx, ry, tile_px, tile_px, Color{24, 52, 68, 255});
                DrawRectangle(rx + 5, ry + 5, tile_px - 10, tile_px - 5, pal::LerpColor(Color{38, 102, 128, 255}, Color{84, 194, 214, 255}, 0.35f + 0.65f * health));
                DrawRectangleLinesEx(Rectangle{tr.x + 2.0f, tr.y + 2.0f, tr.width - 4.0f, tr.height - 4.0f}, 2.2f, glow);
                DrawTextShadow("USCITA", rx + 5, ry + 18, 10, pal::kParchmentText);
            }
        }
    }

    // Ponti retrattili / passerelle dei puzzle interni
    const eco::StructurePuzzleState& puzzle = world.structure.puzzle;
    for (const eco::StructureGate& gate : puzzle.gates) {
        if (!gate.is_bridge) {
            continue;
        }
        for (const eco::TileCoord& coord : gate.tiles) {
            if (coord.row < s_min_row || coord.row > s_max_row ||
                coord.col < s_min_col || coord.col > s_max_col) {
                continue;
            }
            const eco::Rect tr = tilemap.TileRect(coord.row, coord.col);
            const int rx = static_cast<int>(tr.x);
            const int ry = static_cast<int>(tr.y);
            if (gate.open) {
                const float pulse = 0.72f + 0.28f * std::sin(sim_time * 3.4f + static_cast<float>(coord.col) * 0.6f);
                DrawRectangle(rx, ry + 4, tile_px, tile_px - 8, Color{26, 62, 58, 240});
                DrawRectangle(rx, ry, tile_px, 11, pal::LerpColor(pal::kJade, pal::kBronzeBright, 0.35f));
                DrawRectangle(rx + 3, ry + 13, tile_px - 6, tile_px - 20, pal::WithAlpha(pal::kJadeBridgeGlow, static_cast<unsigned char>(130.0f * pulse)));
                DrawLineEx({tr.x + 1.0f, tr.y + 2.5f}, {tr.x + tr.width - 1.0f, tr.y + 2.5f}, 2.6f, pal::kJadeGlow);
                DrawLineEx({tr.x + 2.0f, tr.y + tr.height - 4.0f}, {tr.x + tr.width - 2.0f, tr.y + tr.height - 4.0f}, 2.0f, pal::kBronzeBright);
                DrawCircle(rx + 8, ry + 7, 2.2f, pal::kBronzeDark);
                DrawCircle(rx + tile_px - 8, ry + 7, 2.2f, pal::kBronzeDark);
                DrawRectangleLines(rx, ry, tile_px, tile_px - 2, pal::kBronze);
            } else {
                const float ghost_pulse = 0.45f + 0.35f * std::sin(sim_time * 2.6f + static_cast<float>(coord.col) * 0.5f);
                const Color ghost_col = pal::WithAlpha(pal::kJadeGlow, static_cast<unsigned char>(115.0f * ghost_pulse));
                DrawRectangle(rx + 2, ry + 6, tile_px - 4, tile_px - 18, pal::WithAlpha(pal::kJade, static_cast<unsigned char>(28.0f * ghost_pulse)));
                for (int dx = 2; dx < tile_px - 4; dx += 10) {
                    const int seg_end = std::min(rx + dx + 6, rx + tile_px - 2);
                    DrawLine(rx + dx, ry + 5, seg_end, ry + 5, ghost_col);
                    DrawLine(rx + dx, ry + tile_px - 12, seg_end, ry + tile_px - 12, ghost_col);
                }
                DrawLine(rx + 2, ry + 5, rx + 2, ry + tile_px - 12, pal::WithAlpha(pal::kBronzeBright, 85));
                DrawLine(rx + tile_px - 2, ry + 5, rx + tile_px - 2, ry + tile_px - 12, pal::WithAlpha(pal::kBronzeBright, 85));
            }
        }
    }

    // Interruttori / leve / campane / valvole dei puzzle interni
    for (const eco::StructureSwitch& sw : puzzle.switches) {
        const float sx = sw.pos.x;
        const float sy = sw.pos.y;
        const float floor_y = sy + ts * 0.5f;
        const float pulse = 0.65f + 0.35f * std::sin(sim_time * 4.0f + static_cast<float>(sw.id) * 1.1f);

        bool player_near = false;
        for (const eco::Player* p : world.players.ActivePlayers()) {
            if (p != nullptr && p->Center().DistanceTo(sw.pos) <= sw.interact_radius) {
                player_near = true;
                break;
            }
        }

        // Piedistallo in basalto e bronzo
        DrawRectangle(static_cast<int>(sx - 14.0f), static_cast<int>(floor_y - 10.0f), 28, 10, Color{44, 42, 46, 255});
        DrawRectangleLines(static_cast<int>(sx - 14.0f), static_cast<int>(floor_y - 10.0f), 28, 10, pal::kBronze);
        DrawRectangle(static_cast<int>(sx - 8.0f), static_cast<int>(floor_y - 24.0f), 16, 15, pal::kBronzeDark);
        DrawRectangleLines(static_cast<int>(sx - 8.0f), static_cast<int>(floor_y - 24.0f), 16, 15, pal::kBronzeBright);

        // Leva e cristallo luminoso
        const Color crystal_col = sw.active ? pal::kJadeGlow : pal::kAmberGlow;
        const Color glow_col = sw.active
            ? pal::WithAlpha(pal::kJade, static_cast<unsigned char>(95.0f * pulse))
            : pal::WithAlpha(pal::kAmberGlow, static_cast<unsigned char>(48.0f * pulse));
        const float lever_tip_x = sw.active ? (sx + 10.0f) : (sx - 10.0f);
        const float lever_tip_y = floor_y - 34.0f;
        DrawLineEx({sx, floor_y - 21.0f}, {lever_tip_x, lever_tip_y}, 3.2f, pal::kBronzeBright);
        DrawCircleV({lever_tip_x, lever_tip_y}, 13.0f * pulse, glow_col);
        DrawPoly({lever_tip_x, lever_tip_y}, 4, 6.5f, 0.0f, crystal_col);
        DrawCircleV({lever_tip_x, lever_tip_y}, 2.4f, pal::kStoneIvory);

        // Simbolo ASCII sopra l'interruttore
        const char* sym = (sw.symbol != nullptr) ? sw.symbol : "?";
        const int sym_w = MeasureText(sym, 14);
        const int badge_w = std::max(22, sym_w + 10);
        const int badge_x = static_cast<int>(sx) - badge_w / 2;
        const int badge_y = static_cast<int>(floor_y - 58.0f);
        DrawRectangle(badge_x, badge_y, badge_w, 18, Color{18, 22, 28, 210});
        DrawRectangleLines(badge_x, badge_y, badge_w, 18, sw.active ? pal::kJadeGlow : pal::kBronze);
        DrawTextShadow(sym, static_cast<int>(sx) - sym_w / 2, badge_y + 2, 14, sw.active ? pal::kJadeGlow : pal::kParchmentText);

        // Anello di interazione pulsante e prompt [E] quando il giocatore e' vicino
        if (player_near) {
            const float ring_r = 22.0f + 5.0f * std::sin(sim_time * 6.0f);
            DrawRing({sx, floor_y - 20.0f}, ring_r - 2.0f, ring_r, 0.0f, 360.0f, 24,
                     pal::WithAlpha(sw.active ? pal::kJadeGlow : pal::kAmberGlow, 200));
            const char* prompt = "[E]";
            const int pw = MeasureText(prompt, 12);
            const int px = static_cast<int>(sx) - pw / 2;
            const int py = badge_y - 18;
            DrawRectangle(px - 5, py - 2, pw + 10, 16, Color{16, 20, 26, 225});
            DrawRectangleLines(px - 5, py - 2, pw + 10, 16, pal::kAmberGlow);
            DrawTextShadow(prompt, px, py, 12, pal::kAmberGlow);
        }
    }

    // Iscrizione diegetica della camera vicino allo spawn
    if (puzzle.hint_text != nullptr && puzzle.hint_text[0] != '\0') {
        const char* title_str = (puzzle.title != nullptr && puzzle.title[0] != '\0') ? puzzle.title : "CAMERA";
        const int title_w = MeasureText(title_str, 13);
        const int hint_w = MeasureText(puzzle.hint_text, 11);
        const int tablet_w = std::max(title_w, hint_w) + 22;
        const int tablet_h = 40;
        const int tx = std::clamp(
            static_cast<int>(world.structure.spawn_point.x) - 16,
            tile_px * 2 + 8,
            std::max(tile_px * 2 + 8, world_w - tile_px * 2 - tablet_w - 8));
        const int ty = std::max(tile_px * 2 + 8, static_cast<int>(world.structure.spawn_point.y) - 96);
        DrawRectangle(tx, ty, tablet_w, tablet_h, Color{18, 24, 30, 198});
        DrawRectangleLines(tx, ty, tablet_w, tablet_h, pal::WithAlpha(pal::kBronze, 205));
        DrawLine(tx + 6, ty + 19, tx + tablet_w - 6, ty + 19, pal::WithAlpha(pal::kBronze, 120));
        DrawTextShadow(title_str, tx + 10, ty + 4, 13, pal::kAmberGlow);
        DrawTextShadow(puzzle.hint_text, tx + 10, ty + 23, 11, pal::kParchmentText);
    }

    struct ExitCache { bool ready = false; float x = 0.0f; float y = 0.0f; };
    static std::array<ExitCache, static_cast<std::size_t>(eco::StructureId::Count)> exit_cache{};
    ExitCache& ec_slot = exit_cache[static_cast<std::size_t>(sid) % exit_cache.size()];
    if (!ec_slot.ready) {
        const std::vector<eco::TileCoord> exits = tilemap.FindTiles(eco::TileId::ExitPortal);
        if (!exits.empty()) {
            float sum_x = 0.0f;
            int min_exit_r = exits[0].row;
            for (const auto& ec : exits) {
                sum_x += tilemap.TileCenter(ec.row, ec.col).x;
                min_exit_r = std::min(min_exit_r, ec.row);
            }
            ec_slot.x = sum_x / static_cast<float>(exits.size());
            ec_slot.y = static_cast<float>(min_exit_r) * tilemap.TileSize() - 18.0f;
            ec_slot.ready = true;
        }
    }
    if (ec_slot.ready) {
        const char* exit_label = "CONCA DI MERIDIA";
        if (sid == eco::StructureId::PendulumTower) {
            exit_label = "ALTOPIANO [USCITA]";
        } else if (sid == eco::StructureId::GeothermalChamber) {
            exit_label = "CALDERA [USCITA]";
        } else {
            exit_label = "CANALI [USCITA]";
        }
        DrawMeridianStele(ec_slot.x, ec_slot.y, exit_label, pal::kAmberGlow, health);
    }
}

} // namespace eco::app
