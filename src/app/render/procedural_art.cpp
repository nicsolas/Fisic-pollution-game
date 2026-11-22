#include "app/render/procedural_art.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <array>
#include <vector>

#include "app/render/palette.h"
#include "app/render/ui_style.h"

namespace eco::app {

namespace {

[[nodiscard]] bool IsWallLike(eco::TileId id) noexcept {
    return id == eco::TileId::Wall || id == eco::TileId::RuinsWall;
}

[[nodiscard]] bool IsWaterLike(eco::TileId id) noexcept {
    return id == eco::TileId::Water || id == eco::TileId::HiddenBridge;
}

[[nodiscard]] bool IsPavedLike(eco::TileId id) noexcept {
    return id == eco::TileId::PavedPath ||
           id == eco::TileId::SanctuaryFloor ||
           id == eco::TileId::HiddenBridge;
}

[[nodiscard]] bool IsTileSolidForProp(eco::TileId id) noexcept {
    return id != eco::TileId::Walkable && id != eco::TileId::GardenFloor;
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
    const eco::ZoneId zone = overworld.DistrictAtTile(row, col);
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
    // Ombra morbida alla base
    DrawEllipse(static_cast<int>(x + 3.0f), static_cast<int>(y + 13.0f), 14.0f, 4.5f, Color{0, 0, 0, 105});

    // Basamento in pietra scolpita e fusto
    const Color stone_base = pal::LerpColor(Color{74, 68, 62, 255}, Color{138, 134, 122, 255}, health);
    const Color stone_hi = pal::ShiftBrightness(stone_base, 26);
    DrawRectangle(static_cast<int>(x - 7.0f), static_cast<int>(y + 6.0f), 14, 7, stone_base);
    DrawRectangle(static_cast<int>(x - 4.0f), static_cast<int>(y - 7.0f), 8, 15, stone_base);
    DrawLine(static_cast<int>(x - 4.0f), static_cast<int>(y - 7.0f), static_cast<int>(x - 4.0f), static_cast<int>(y + 7.0f), stone_hi);

    // Targa superiore in ardesia scura e filetto in bronzo/accento
    const int tw = MeasureUiText(label, 11);
    const int bw = tw + 18;
    const int bx = static_cast<int>(x) - bw / 2;
    const int by = static_cast<int>(y - 25.0f);
    DrawRectangle(bx + 2, by + 2, bw, 19, Color{0, 0, 0, 95});
    DrawRectangle(bx, by, bw, 19, Color{18, 24, 34, 236});
    DrawRectangleLines(bx, by, bw, 19, pal::WithAlpha(pal::kBronze, 220));
    DrawRectangleLines(bx + 2, by + 2, bw - 4, 15, pal::WithAlpha(accent, 190));

    // Piccoli chiodi in bronzo ai lati
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

    // 12 raggi solari incisi nella pavimentazione
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

    // 8 petali armonici incisi in pietra e giada
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

void DrawSanctuaryGrandMosaic(eco::Vec2 center, float poll, float sim_time, int relics_found) {
    const float health = 1.0f - poll;
    const Color marble = pal::WorldColor(Color{116, 108, 98, 185}, Color{216, 210, 192, 225}, poll);
    const Color bronze = pal::WorldColor(Color{128, 104, 70, 195}, Color{226, 188, 104, 245}, poll);
    const Color jade = pal::WorldColor(Color{92, 102, 86, 175}, Color{72, 232, 194, 245}, poll);

    // Grande disco pavimentale in travertino avorio
    DrawCircleV({center.x, center.y}, 136.0f, pal::WithAlpha(marble, 88));
    DrawRing({center.x, center.y}, 130.0f, 136.0f, 0.0f, 360.0f, 48, bronze);
    DrawRing({center.x, center.y}, 102.0f, 106.0f, 0.0f, 360.0f, 44, jade);
    DrawRing({center.x, center.y}, 72.0f, 75.0f, 0.0f, 360.0f, 36, bronze);
    DrawRing({center.x, center.y}, 42.0f, 44.5f, 0.0f, 360.0f, 28, jade);

    // 16 tacche astronomiche sul cerchio esterno
    for (int i = 0; i < 16; ++i) {
        const float ang = static_cast<float>(i) * (eco::kPi / 8.0f);
        const bool cardinal = (i % 4 == 0);
        const float r0 = cardinal ? 44.0f : 106.0f;
        const float r1 = cardinal ? 136.0f : 130.0f;
        DrawLineEx(
            {center.x + std::cos(ang) * r0, center.y + std::sin(ang) * r0},
            {center.x + std::cos(ang) * r1, center.y + std::sin(ang) * r1},
            cardinal ? 2.6f : 1.4f,
            cardinal ? bronze : pal::WithAlpha(bronze, 180));
    }

    // 4 medaglioni delle Ere (si accendono man mano che le reliquie vengono raccolte)
    const int lit_eras = std::clamp(relics_found / 4, 0, 4);
    for (int era = 0; era < 4; ++era) {
        const float ang = static_cast<float>(era) * (eco::kPi * 0.5f) + (eco::kPi * 0.25f);
        const Vector2 mp{center.x + std::cos(ang) * 88.0f, center.y + std::sin(ang) * 88.0f};
        const bool lit = (era < lit_eras) || (health > 0.75f);
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
// Flora Diversificata & Micro-Props Architettonici
// =============================================================================
void DrawBioCrystalNode(float x, float y, float sim_time, float poll, Color accent) {
    const float health = 1.0f - poll;
    const Color stem_col = pal::WorldColor(Color{70, 62, 54, 255}, Color{42, 118, 96, 255}, poll);
    const Color crystal_col = pal::WorldColor(Color{136, 86, 62, 225}, accent, poll);

    DrawEllipse(static_cast<int>(x + 2.0f), static_cast<int>(y + 11.0f), 14.0f, 5.5f, Color{0, 0, 0, 105});
    // Basamento esagonale in pietra
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

    // Ombra portata SE
    DrawEllipse(
        static_cast<int>(x + 7.0f * scale),
        static_cast<int>(y + 12.0f * scale),
        22.0f * scale,
        8.5f * scale,
        Color{0, 0, 0, 108});

    // Tronco nodoso d'ulivo con radici
    const Color trunk_dark = pal::WorldColor(Color{48, 40, 36, 255}, Color{72, 54, 40, 255}, poll);
    const Color trunk_hi = pal::WorldColor(Color{74, 62, 52, 255}, Color{114, 88, 64, 255}, poll);
    DrawLineEx({x - 3.0f * scale, y + 11.0f * scale}, {x - 1.0f * scale + sway * 0.25f, y - 11.0f * scale}, 5.5f * scale, trunk_dark);
    DrawLineEx({x + 3.0f * scale, y + 11.0f * scale}, {x + 1.5f * scale + sway * 0.25f, y - 10.0f * scale}, 5.0f * scale, trunk_dark);
    DrawLineEx({x - 2.0f * scale, y + 9.0f * scale}, {x - 0.5f * scale + sway * 0.25f, y - 11.0f * scale}, 2.4f * scale, trunk_hi);
    DrawLineEx({x, y - 5.0f * scale}, {x - 13.0f * scale + sway * 0.45f, y - 15.0f * scale}, 3.2f * scale, trunk_dark);
    DrawLineEx({x, y - 6.0f * scale}, {x + 13.0f * scale + sway * 0.45f, y - 16.0f * scale}, 3.2f * scale, trunk_dark);

    // Chioma larga e argentata-verde stratificata
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
    int seed) {
    const float health = 1.0f - poll;
    // Cipresso slanciato inclinato dal vento
    const float wind = 3.2f + std::sin(sim_time * 2.5f + static_cast<float>(seed) * 1.3f) * (2.2f + 2.0f * health);

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

    // 4 piani conici sovrapposti che si flettono verso la punta
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

    // Fusto snello con aiuola in pietra alla base
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

    // Fioritura abbondante nei Giardini Sospesi al calare dell'inquinamento
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

    // Grappolo di colonne esagonali di basalto
    const Color basalt_dark = pal::WorldColor(Color{50, 42, 40, 255}, Color{62, 68, 78, 255}, poll);
    const Color basalt_top = pal::WorldColor(Color{76, 64, 60, 255}, Color{98, 108, 120, 255}, poll);
    const Color glow = pal::WorldColor(Color{216, 76, 36, 200}, Color{88, 204, 196, 175}, poll);

    DrawPoly({x - 7.0f * scale, y + 3.0f * scale}, 6, 7.0f * scale, 30.0f, basalt_dark);
    DrawPoly({x + 7.0f * scale, y + 4.0f * scale}, 6, 6.5f * scale, 30.0f, basalt_dark);
    DrawPoly({x, y - 2.0f * scale}, 6, 9.5f * scale, 30.0f, basalt_top);
    DrawPolyLines({x, y - 2.0f * scale}, 6, 9.5f * scale, 30.0f, pal::ShiftBrightness(basalt_top, 24));

    // Bocca termale centrale + filo di vapore che sale dolcemente
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
    // Scanalature verticali classiche
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

    // Ombra portata SE
    DrawEllipse(static_cast<int>(x + 6.0f), static_cast<int>(y + 13.0f), 15.0f, 6.0f, Color{0, 0, 0, 108});
    // Plinto a due gradini
    DrawRectangle(static_cast<int>(x - 12.0f), static_cast<int>(y + 6.0f), 24, 7, stone_dark);
    DrawRectangle(static_cast<int>(x - 10.0f), static_cast<int>(y + 1.0f), 20, 6, stone);
    DrawLine(static_cast<int>(x - 10.0f), static_cast<int>(y + 1.0f), static_cast<int>(x + 10.0f), static_cast<int>(y + 1.0f), stone_hi);

    // Figura stilizzata drappeggiata dei Meridiani
    DrawTriangle(
        {x - 7.0f, y + 1.0f},
        {x + 7.0f, y + 1.0f},
        {x, y - 20.0f},
        stone);
    DrawTriangleLines({x - 7.0f, y + 1.0f}, {x + 7.0f, y + 1.0f}, {x, y - 20.0f}, stone_dark);
    DrawLineEx({x - 2.0f, y + 1.0f}, {x - 1.0f, y - 15.0f}, 1.6f, stone_hi);
    // Testa e spalle
    DrawCircleV({x, y - 22.0f}, 4.5f, stone);
    DrawCircleV({x - 1.0f, y - 23.0f}, 2.2f, stone_hi);

    if (hydro_variant) {
        // Anfora con filo d'acqua turchese che scende dolcemente
        DrawEllipse(static_cast<int>(x + 7.0f), static_cast<int>(y - 13.0f), 4.0f, 3.0f, pal::kBronze);
        const Color water = pal::WorldColor(dp.water_sick, dp.foam_healthy, poll);
        const float shimmer = std::sin(sim_time * 5.0f + x * 0.1f) * 1.4f;
        DrawLineEx({x + 9.0f, y - 11.0f}, {x + 10.0f + shimmer, y + 6.0f}, 2.2f, pal::WithAlpha(water, 215));
        if (health > 0.35f) {
            DrawEllipse(static_cast<int>(x + 10.0f), static_cast<int>(y + 8.0f), 5.0f, 2.2f, pal::WithAlpha(water, 145));
        }
    } else {
        // Sfera armillare in bronzo sollevata sopra la statua
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

    // Ombra lunga da gnomone verso sud-est
    DrawTriangle(
        {x - 4.0f, y + 8.0f},
        {x + 5.0f, y + 8.0f},
        {x + 24.0f, y + 18.0f},
        Color{0, 0, 0, 82});
    DrawRectangle(static_cast<int>(x - 7.0f), static_cast<int>(y + 4.0f), 14, 6, stone_dark);
    // Fusto rastremato
    DrawTriangle({x - 5.0f, y + 4.0f}, {x + 5.0f, y + 4.0f}, {x, y - 28.0f}, stone);
    DrawTriangle({x, y + 4.0f}, {x + 5.0f, y + 4.0f}, {x, y - 28.0f}, stone_dark);
    DrawLineEx({x - 4.5f, y + 3.0f}, {x, y - 27.0f}, 1.5f, stone_hi);
    // Punta piramidale in bronzo lucido
    DrawTriangle({x - 2.6f, y - 20.0f}, {x + 2.6f, y - 20.0f}, {x, y - 29.0f}, pal::kBronzeBright);
    if (health > 0.40f) {
        const float glint = 0.55f + 0.45f * std::sin(sim_time * 3.2f + x * 0.05f);
        DrawCircleV({x, y - 28.0f}, 3.2f * glint, pal::WithAlpha(dp.ambient_accent, static_cast<unsigned char>(175.0f * health)));
    }
}

// =============================================================================
// Verifica distanza di sicurezza per evitare sovrapposizione prop <-> gameplay
// =============================================================================
[[nodiscard]] bool IsPropPositionSafe(const eco::WorldContext& world, eco::Vec2 pos) noexcept {
    if (pos.DistanceTo(world.overworld.spawn_point) < 80.0f) {
        return false;
    }
    for (const auto& m : world.machines) {
        if (m.location == eco::LocationId::Overworld && pos.DistanceTo(m.pos) < 108.0f) {
            return false;
        }
    }
    for (const auto& npc : world.npcs) {
        if (npc.location == eco::LocationId::Overworld && pos.DistanceTo(npc.pos) < 82.0f) {
            return false;
        }
    }
    for (const auto& p : world.overworld.portals) {
        if (pos.DistanceTo(p.world_center) < 112.0f || pos.DistanceTo(p.exit_spawn) < 76.0f) {
            return false;
        }
    }
    for (const auto& item : world.collectibles.Items()) {
        if (item.location == eco::LocationId::Overworld && pos.DistanceTo(item.pos) < 66.0f) {
            return false;
        }
    }
    // Non posizionare alberi a ridosso del Ponte degli Archi
    const float bridge_x = (static_cast<float>(eco::Tilemap::kBridgeDividerCol) + 0.5f) * eco::kTileSize;
    if (std::fabs(pos.x - bridge_x) < 92.0f && pos.y > 14.0f * eco::kTileSize && pos.y < 20.0f * eco::kTileSize) {
        return false;
    }
    return true;
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
        // Portale Idraulico: grande arco calcareo continuo + cascata turchese + chiave di volta
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
        // Architrave superiore e cornice continua
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
            // Chiave di volta centrale a cavallo dei due tile del portale
            DrawRectangle(rx + tile_px - 6, ry + 1, 12, 8, pal::ShiftBrightness(stone, 28));
            DrawCircle(rx + tile_px, ry + 5, 2.5f, water_glow);
        }
    } else if (sid == eco::StructureId::PendulumTower) {
        // Portale Osservatorio: doppio arco in pietra chiara + quadrante astronomico + luce dorata
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
            // Pendolo monumentale al centro del doppio portale
            const float cx = static_cast<float>(rx + tile_px);
            const float cy = static_cast<float>(ry + 12);
            DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), 11.0f, gold_glow);
            const float swing = std::sin(sim_time * 2.5f) * 10.0f;
            DrawLineEx({cx, cy}, {cx + swing, static_cast<float>(ry + tile_px - 8)}, 2.0f, gold_glow);
            DrawCircleV({cx + swing, static_cast<float>(ry + tile_px - 8)}, 3.6f, pal::kBronzeBright);
        }
    } else {
        // Portale Camera Geotermica: grande arco di basalto + rame + onde di calore ascendenti
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

    // Piano 1 (lontano): grandi volte idrauliche e arcate in pietra
    const float p_far = parallax_x * 0.55f;
    for (int i = -1; i < 10; ++i) {
        const int ax = static_cast<int>(60.0f + static_cast<float>(i) * 210.0f - p_far);
        DrawRectangle(ax, 48, 38, world_h - 130, arch_far);
        DrawCircle(ax + 124, 155, 86.0f, pal::ShiftBrightness(arch_far, -6));
        DrawRing({static_cast<float>(ax + 124), 155.0f}, 76.0f, 88.0f, 180.0f, 360.0f, 24, pal::ShiftBrightness(arch_far, 14));
    }

    // Piano 2 (medio): contrafforti scolpiti, condotte in bronzo e cascate luminose
    for (int i = -1; i < 9; ++i) {
        const int px = static_cast<int>(100.0f + static_cast<float>(i) * 240.0f - parallax_x);
        DrawRectangle(px, 48, 52, world_h - 136, arch_near);
        DrawRectangleLines(px, 48, 52, world_h - 136, pal::ShiftBrightness(arch_near, 18));
        // Fasce in bronzo attorno ai pilastri
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
            // Nebbia/spruzzo alla base della cascata
            const float mist_r = 22.0f + 5.0f * std::sin(sim_time * 3.0f + static_cast<float>(i));
            DrawEllipse(wx + 16, world_h - 102, mist_r, 8.0f, pal::WithAlpha(foam, static_cast<unsigned char>(65.0f + 45.0f * health)));
        }
    }

    // Bacino idraulico riflettente lungo il fondo
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

    // Piano 1 (lontano): grandi oculi astronomici in alto + finestre ad arco centrali e fasci di luce
    const float p_far = parallax_x * 0.45f;
    for (int i = 0; i < 6; ++i) {
        const float cx = 190.0f + static_cast<float>(i) * 290.0f - p_far;
        const float cy_top = 185.0f;
        const float cy_mid = 435.0f;
        // Fascio di luce diagonale che scende dall'alto fino al pavimento
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

        // Oculo circolare superiore con raggiera in bronzo
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

        // Grande quadrante astronomico e finestra ad arco a mezza altezza (ben visibile anche dal basso!)
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

    // Piano 2 (medio): pilastri dell'osservatorio, grandi archi graduati e contrappesi oscillanti
    for (int i = -1; i < 8; ++i) {
        const int px = static_cast<int>(85.0f + static_cast<float>(i) * 235.0f - parallax_x);
        DrawRectangle(px, 48, 36, world_h - 130, arch_far);
        DrawRectangle(px + 6, 48, 24, world_h - 130, arch_near);
        DrawLine(px + 6, 48, px + 6, world_h - 96, pal::ShiftBrightness(arch_near, 22));

        // Arco goniometrico gigante inciso tra i pilastri
        const float arc_cx = static_cast<float>(px + 135);
        DrawRing({arc_cx, 110.0f}, 96.0f, 99.0f, 25.0f, 155.0f, 24, pal::WithAlpha(dial_col, 95));

        // Contrappesi d'ottone appesi a catene che dondolano lentamente
        if ((i & 1) == 1) {
            const float sway = std::sin(sim_time * 1.15f + static_cast<float>(i)) * 8.0f;
            const float wy = 240.0f + static_cast<float>((i % 3) * 45);
            DrawLineEx({arc_cx, 48.0f}, {arc_cx + sway, wy}, 1.8f, pal::WithAlpha(dial_col, 140));
            DrawRectangle(static_cast<int>(arc_cx + sway - 8.0f), static_cast<int>(wy), 16, 28, pal::WithAlpha(pal::kBronze, 190));
            DrawRectangleLines(static_cast<int>(arc_cx + sway - 8.0f), static_cast<int>(wy), 16, 28, dial_col);
        }
    }

    // Incisione metrica orizzontale morbida lungo la sala
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

    // Piano 1 (lontano): colonne di basalto esagonali della caverna vulcanica + vene luminose
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
            // Vena geotermica verticale luminosa nella roccia basaltica
            const float pulse = 0.60f + 0.40f * std::sin(sim_time * 2.2f + static_cast<float>(i));
            const float vx = static_cast<float>(bx + bw / 2);
            DrawLineEx(
                {vx, static_cast<float>(top_y + 40)},
                {vx + 8.0f, static_cast<float>(world_h - 110)},
                4.5f,
                pal::WithAlpha(accent, static_cast<unsigned char>(34.0f * pulse)));
            DrawLineEx(
                {vx, static_cast<float>(top_y + 40)},
                {vx + 8.0f, static_cast<float>(world_h - 110)},
                1.8f,
                pal::WithAlpha(accent, static_cast<unsigned char>(115.0f * pulse)));
        }
    }

    // Piano 2 (medio): grandi archi termali in rame ossidato e condotte verticali con scie di vapore morbide
    for (int i = -1; i < 8; ++i) {
        const int px = static_cast<int>(110.0f + static_cast<float>(i) * 245.0f - parallax_x);
        // Arco termale tra i pilastri
        DrawRing({static_cast<float>(px + 145), 240.0f}, 96.0f, 108.0f, 180.0f, 360.0f, 22, pal::WithAlpha(basalt_near, 195));
        DrawRing({static_cast<float>(px + 145), 240.0f}, 94.0f, 97.0f, 180.0f, 360.0f, 22, pal::WithAlpha(pal::kBronze, 120));

        DrawRectangle(px, 48, 46, world_h - 134, basalt_near);
        DrawRectangle(px + 10, 48, 26, world_h - 134, pal::WorldColor(Color{96, 62, 46, 220}, Color{84, 114, 110, 220}, poll));
        DrawRectangleLines(px + 10, 48, 26, world_h - 134, pal::WithAlpha(pal::kBronze, 180));
        // Giunti rivettati
        for (int ry = 110; ry < world_h - 140; ry += 96) {
            DrawRectangle(px + 6, ry, 34, 8, pal::kBronzeDark);
            DrawCircle(px + 12, ry + 4, 1.8f, accent);
            DrawCircle(px + 34, ry + 4, 1.8f, accent);
        }

        // Scie di vapore morbide e volumetriche (ellissi tenui sovrapposte, mai bolle nette)
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

    // Bagliore termale morbido dal basso + pozza geotermica luminosa
    DrawRectangleGradientV(48, world_h - 220, world_w - 96, 120, Color{0, 0, 0, 0}, pal::WithAlpha(accent, 44));
    DrawRectangle(48, world_h - 108, world_w - 96, 24, pal::WithAlpha(pool_col, 195));
    for (int x = 70; x < world_w - 90; x += 64) {
        const float glow_r = 4.0f + 2.5f * std::sin(sim_time * 3.2f + static_cast<float>(x) * 0.07f);
        DrawCircle(x, world_h - 98, glow_r, pal::WithAlpha(accent, 155));
    }
}

} // namespace

// =============================================================================
// Hash Deterministico 2D
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

// =============================================================================
// Overworld Map — La Conca di Meridia (6 Distretti, Viewport Culling, 2.5D)
// =============================================================================
void DrawOverworldProceduralMap(const eco::WorldContext& world) {
    const eco::Tilemap& tilemap = world.overworld.tilemap;
    const float poll = world.ecosystem.PollutionRatio();
    const float health = 1.0f - poll;
    const float sim_time = world.sim_time;
    const bool bridge_unlocked = world.ecosystem.architect_recruited;
    const int tile_px = static_cast<int>(tilemap.TileSize());
    if (tile_px <= 0 || tilemap.Rows() <= 0 || tilemap.Cols() <= 0) {
        return;
    }

    // Frustum / Viewport Culling con margine di 2 tile per ombre e sovrapposizioni
    const eco::Vec2 cam = world.overworld.camera_target;
    const float half_w = static_cast<float>(eco::kWindowWidth) * 0.5f + tilemap.TileSize() * 2.5f;
    const float half_h = static_cast<float>(eco::kWindowHeight) * 0.5f + tilemap.TileSize() * 2.5f;
    const int min_col = std::max(0, static_cast<int>(std::floor((cam.x - half_w) / tilemap.TileSize())));
    const int max_col = std::min(tilemap.Cols() - 1, static_cast<int>(std::ceil((cam.x + half_w) / tilemap.TileSize())));
    const int min_row = std::max(0, static_cast<int>(std::floor((cam.y - half_h) / tilemap.TileSize())));
    const int max_row = std::min(tilemap.Rows() - 1, static_cast<int>(std::ceil((cam.y + half_h) / tilemap.TileSize())));

    // -------------------------------------------------------------------------
    // PASSATA 1: Terreni, Sentieri, Piazze Intarsiate, Giardini, Acqua, Ponte, Portali
    // -------------------------------------------------------------------------
    for (int r = min_row; r <= max_row; ++r) {
        const int row_cols = tilemap.RowCols(r);
        const int c_end = std::min(max_col, row_cols - 1);
        for (int c = min_col; c <= c_end; ++c) {
            const eco::Rect tr = tilemap.TileRect(r, c);
            const int rx = static_cast<int>(tr.x);
            const int ry = static_cast<int>(tr.y);
            const eco::TileId id = tilemap.GetTile(r, c);
            const eco::ZoneId zone = tilemap.DistrictAtTile(r, c);
            const pal::DistrictPalette& dp = pal::GetDistrictPalette(zone);
            const std::uint32_t h = Hash2(c, r, 17);
            const int tone_delta = static_cast<int>(h % 7u) - 3;

            if (id == eco::TileId::Walkable) {
                Color ground = pal::ShiftBrightness(
                    pal::WorldColor(dp.ground_sick, dp.ground_healthy, poll),
                    tone_delta);
                DrawRectangle(rx, ry, tile_px, tile_px, ground);

                // Velatura organica opaca (LerpColor con alpha 255 -> zero artefatti FBO!)
                if ((h & 3u) == 0u) {
                    const int ox = rx + 12 + static_cast<int>((h >> 4) % 24u);
                    const int oy = ry + 12 + static_cast<int>((h >> 9) % 24u);
                    const float erx = 16.0f + static_cast<float>((h >> 14) % 10u);
                    const float ery = 11.0f + static_cast<float>((h >> 18) % 8u);
                    const Color patch = pal::ShiftBrightness(ground, ((h >> 2) & 1u) ? 5 : -5);
                    DrawEllipse(ox, oy, erx, ery, patch);
                }

                // Dettagli specifici per bioma
                const Color grass_col = pal::WorldColor(dp.grass_sick, dp.grass_healthy, poll);
                if (zone == eco::ZoneId::FumaroleCaldera) {
                    // Crepe frastagliate nel basalto vulcanico + vene termali calde (opache)
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
                            DrawLine(cx, cy, cx - 2, cy + 7, pal::WithAlpha(pal::LerpColor(ground, crack, 0.7f), 255));
                        }
                    }
                    if (health > 0.35f && ((h >> 3) & 3u) == 1u) {
                        const int gx = rx + 12 + static_cast<int>((h >> 7) % 24u);
                        const int gy = ry + 14 + static_cast<int>((h >> 12) % 22u);
                        DrawCircle(gx, gy, 2.8f, pal::WithAlpha(pal::LerpColor(ground, grass_col, 0.72f), 255));
                        DrawCircle(gx + 3, gy + 1, 2.0f, pal::WithAlpha(pal::LerpColor(ground, grass_col, 0.58f), 255));
                    }
                } else {
                    // Ciuffi d'erba multipli (inclinati dal vento sull'Altopiano dei Venti!)
                    if ((h & 3u) <= 2u) {
                        const int gx = rx + 8 + static_cast<int>((h >> 4) % 30u);
                        const int gy = ry + 13 + static_cast<int>((h >> 9) % 25u);
                        const int blade_h = 4 + static_cast<int>(health * 5.0f);
                        const float wind_bias = (zone == eco::ZoneId::WindPlateau)
                                                    ? (3.2f + std::sin(sim_time * 3.5f + static_cast<float>(c) * 0.4f) * 2.0f)
                                                    : (std::sin(sim_time * 1.8f + static_cast<float>(c + r)) * 1.0f);
                        DrawLineEx({static_cast<float>(gx), static_cast<float>(gy)},
                                   {static_cast<float>(gx - 2) + wind_bias, static_cast<float>(gy - blade_h)}, 1.4f, grass_col);
                        DrawLineEx({static_cast<float>(gx + 1), static_cast<float>(gy)},
                                   {static_cast<float>(gx + 3) + wind_bias, static_cast<float>(gy - blade_h + 1)}, 1.3f, grass_col);
                        DrawLineEx({static_cast<float>(gx - 1), static_cast<float>(gy + 1)},
                                   {static_cast<float>(gx) + wind_bias * 0.8f, static_cast<float>(gy - blade_h - 1)}, 1.2f, pal::ShiftBrightness(grass_col, 14));

                        // Fiori spontanei reattivi allo smog (opachi via LerpColor)
                        if (health > 0.42f && ((h >> 14) & 3u) == 0u) {
                            const Color flower_raw = ((h >> 16) & 1u) ? dp.blossom_a : dp.blossom_b;
                            const float ft = std::clamp((health - 0.42f) / 0.58f, 0.0f, 1.0f);
                            const Color flower = pal::WithAlpha(pal::LerpColor(ground, flower_raw, ft), 255);
                            DrawCircleV({static_cast<float>(gx) + wind_bias, static_cast<float>(gy - blade_h - 1)}, 2.3f, flower);
                        }
                    }
                    // Piccoli ciottoli naturali in pietra calcarea/basalto
                    if (((h >> 3) & 7u) == 2u) {
                        const int px = rx + 12 + static_cast<int>((h >> 11) % 24u);
                        const int py = ry + 12 + static_cast<int>((h >> 15) % 24u);
                        DrawCircle(px, py, 2.4f, pal::ShiftBrightness(ground, -14));
                        DrawCircle(px - 1, py - 1, 1.3f, pal::ShiftBrightness(ground, 14));
                    }
                }
            } else if (id == eco::TileId::GardenFloor) {
                // Giardino terrazzato organico con fogliame irregolare, bordure in pietra e fioriture (alpha 255)
                Color garden = pal::ShiftBrightness(
                    pal::WorldColor(dp.garden_sick, dp.garden_healthy, poll),
                    tone_delta);
                DrawRectangle(rx, ry, tile_px, tile_px, garden);

                // Rosette di foglie e felci a posizioni deterministiche variate (opache, zero artefatti FBO!)
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

                // Fioriture a grappolo che sbocciano vistosamente quando la Conca guarisce
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

                // Bordo in pietra del terrazzamento solo sui lati non-GardenFloor
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
                // Lastricato a conci sfalsati (running bond) con cordoli auto-tiled
                const Color ground_bg = pal::WorldColor(dp.ground_sick, dp.ground_healthy, poll);
                DrawRectangle(rx, ry, tile_px, tile_px, ground_bg);

                const Color stone = pal::ShiftBrightness(
                    pal::WorldColor(dp.path_sick, dp.path_healthy, poll),
                    tone_delta);
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

                // Fughe sfalsate in due file di lastroni
                const int mid_y = ry + tile_px / 2;
                const int split_top = rx + 16 + static_cast<int>((h >> 4) % 14u);
                const int split_bot = rx + 8 + static_cast<int>((h >> 9) % 28u);
                DrawLine(rx + pad_l, mid_y, rx + tile_px - pad_r, mid_y, mortar);
                DrawLine(split_top, ry + pad_t, split_top, mid_y, mortar);
                DrawLine(split_bot, mid_y, split_bot, ry + tile_px - pad_b, mortar);

                // Luce NW sui cordoli esposti
                if (!pn) {
                    DrawLine(rx + pad_l, ry + pad_t, rx + tile_px - pad_r, ry + pad_t, bevel);
                }
                if (!pw) {
                    DrawLine(rx + pad_l, ry + pad_t, rx + pad_l, ry + tile_px - pad_b, bevel);
                }
                if (!ps) {
                    DrawLine(rx + pad_l, ry + tile_px - pad_b, rx + tile_px - pad_r, ry + tile_px - pad_b, mortar);
                }
                if (!pe) {
                    DrawLine(rx + tile_px - pad_r, ry + pad_t, rx + tile_px - pad_r, ry + tile_px - pad_b, mortar);
                }

                // Filo di muschio tra le pietre quando l'ecosistema migliora
                if (health > 0.40f && (h & 3u) == 0u) {
                    const Color moss = pal::WorldColor(dp.grass_sick, dp.grass_healthy, poll);
                    DrawCircle(split_top, mid_y, 2.2f * health, pal::WithAlpha(moss, 200));
                }
            } else if (id == eco::TileId::SanctuaryFloor) {
                // Pavimentazione monumentale intarsiata in marmo/travertino, bronzo e giada
                const Color slab = pal::ShiftBrightness(
                    pal::WorldColor(dp.sanctuary_sick, dp.sanctuary_healthy, poll),
                    tone_delta);
                const Color inlay = pal::WorldColor(dp.inlay_sick, dp.inlay_healthy, poll);
                const Color groove = pal::ShiftBrightness(slab, -22);
                const Color bevel = pal::ShiftBrightness(slab, 20);

                DrawRectangle(rx, ry, tile_px, tile_px, slab);
                DrawRectangleLines(rx, ry, tile_px, tile_px, groove);
                DrawLine(rx + 1, ry + 1, rx + tile_px - 2, ry + 1, bevel);
                DrawLine(rx + 1, ry + 1, rx + 1, ry + tile_px - 2, bevel);

                // Motivo geometrico a cornice e rombo intarsiato
                DrawRectangleLines(rx + 5, ry + 5, tile_px - 10, tile_px - 10, pal::WithAlpha(inlay, 130));
                if (((r + c) & 1) == 0) {
                    const Vector2 ctr{tr.x + tr.width * 0.5f, tr.y + tr.height * 0.5f};
                    DrawPolyLines(ctr, 4, 10.0f, 0.0f, pal::WithAlpha(inlay, 175));
                    if (health > 0.35f) {
                        DrawCircleV(ctr, 2.2f, pal::WithAlpha(dp.ambient_accent, static_cast<unsigned char>(160.0f * health)));
                    }
                }
            } else if (id == eco::TileId::Water) {
                // Canali e specchi d'acqua continui (zero griglia interna) con sponde scolpite, riflessi e ninfee
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
                // Profondita' morbida solo lungo i bordi interni delle sponde
                if (!wn) {
                    DrawRectangle(rx, ry + 6, tile_px, 7, water_deep);
                }
                if (!ww) {
                    DrawRectangle(rx + 6, ry, 6, tile_px, water_deep);
                }

                // Riflessi luminosi e onde di corrente animate
                const float flow = std::fmod(sim_time * 22.0f + static_cast<float>(r * 17 + c * 9), static_cast<float>(tile_px));
                const float wy = tr.y + flow;
                const float wave_dx = std::sin(sim_time * 2.6f + static_cast<float>(r + c)) * 4.0f;
                DrawLineEx(
                    {tr.x + 10.0f + wave_dx, wy},
                    {tr.x + tr.width - 10.0f + wave_dx, wy},
                    2.0f,
                    pal::WithAlpha(foam, static_cast<unsigned char>(115.0f + 80.0f * health)));

                const float wy2 = tr.y + std::fmod(flow + 24.0f, static_cast<float>(tile_px));
                DrawLineEx(
                    {tr.x + 14.0f - wave_dx * 0.7f, wy2},
                    {tr.x + tr.width - 14.0f - wave_dx * 0.7f, wy2},
                    1.5f,
                    pal::WithAlpha(water_shimmer, 140));

                // Scintillio superficiale
                if (((h >> 5) & 3u) == 0u && health > 0.25f) {
                    const float glint = 0.5f + 0.5f * std::sin(sim_time * 4.2f + static_cast<float>(c * 3 + r * 5));
                    const float gx = tr.x + 14.0f + static_cast<float>((h >> 8) % 20u);
                    const float gy = tr.y + 14.0f + static_cast<float>((h >> 13) % 20u);
                    DrawCircleV({gx, gy}, 1.8f + glint * 1.2f, pal::WithAlpha(foam, static_cast<unsigned char>(190.0f * glint * health)));
                }

                // Sponde scolpite auto-tiled sui lati adiacenti a terra/muri
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

                // Ninfee e canneti quando l'acqua si risana (non nella Caldera)
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

                // Arcate decorative d'acquedotto che scavalcano periodicamente i canali nord-sud
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
                // Ponte degli Archi sopra il canale centrale (zero laser sci-fi!)
                const Color water_base = pal::WorldColor(dp.water_sick, dp.water_healthy, poll);
                const Color foam = pal::WorldColor(dp.foam_sick, dp.foam_healthy, poll);
                DrawRectangle(rx, ry, tile_px, tile_px, water_base);
                const float flow = std::fmod(sim_time * 20.0f + static_cast<float>(r * 19), static_cast<float>(tile_px));
                DrawLineEx({tr.x + 8.0f, tr.y + flow}, {tr.x + tr.width - 8.0f, tr.y + flow}, 1.8f, pal::WithAlpha(foam, 140));

                const bool bn = (tilemap.GetTile(r - 1, c) == eco::TileId::HiddenBridge);
                const bool bs = (tilemap.GetTile(r + 1, c) == eco::TileId::HiddenBridge);

                if (bridge_unlocked) {
                    // Ombra del ponte sull'acqua
                    DrawRectangle(rx + 2, ry + tile_px - 2, tile_px - 2, 7, Color{0, 0, 0, 80});
                    // Carreggiata in travertino scolpito + canale centrale in bronzo e giada
                    const Color deck = pal::WorldColor(Color{116, 108, 96, 255}, Color{176, 174, 162, 255}, poll);
                    const Color rail = pal::WorldColor(Color{96, 86, 76, 255}, Color{198, 194, 180, 255}, poll);
                    DrawRectangle(rx - 2, ry, tile_px + 4, tile_px, deck);
                    for (int bx = rx + 8; bx < rx + tile_px; bx += 12) {
                        DrawLine(bx, ry + 4, bx, ry + tile_px - 4, pal::ShiftBrightness(deck, -22));
                    }
                    // Guida centrale in bronzo e giada luminescente
                    DrawRectangle(rx - 2, ry + tile_px / 2 - 4, tile_px + 4, 8, pal::kBronzeDark);
                    DrawLineEx({tr.x - 2.0f, tr.y + tr.height * 0.5f},
                               {tr.x + tr.width + 2.0f, tr.y + tr.height * 0.5f}, 2.8f, pal::kJade);

                    // Parapetti ad archi in pietra e bronzo sui bordi nord/sud del ponte
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
                    // Ponte degli Archi interrotto: monconi di arcate in pietra + sigillo ambrato dei Meridiani
                    const Color rubble = pal::WorldColor(Color{92, 82, 74, 255}, Color{134, 132, 124, 255}, poll);
                    DrawRectangle(rx, ry + 3, 11, tile_px - 6, rubble);
                    DrawRectangle(rx + tile_px - 11, ry + 3, 11, tile_px - 6, rubble);
                    DrawRectangleLines(rx, ry + 3, 11, tile_px - 6, pal::ShiftBrightness(rubble, -24));
                    DrawRectangleLines(rx + tile_px - 11, ry + 3, 11, tile_px - 6, pal::ShiftBrightness(rubble, -24));

                    // Catene di bronzo spezzate e sigillo d'ambra pulsante (coerente con la civilta')
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
    for (const auto& m : world.machines) {
        if (m.location != eco::LocationId::Overworld) {
            continue;
        }
        if (std::fabs(m.pos.x - cam.x) > half_w + 150.0f || std::fabs(m.pos.y - cam.y) > half_h + 150.0f) {
            continue;
        }
        if (m.type == eco::MachineType::SolarArray) {
            DrawSolarSundialMosaic(m.pos, poll, sim_time);
        } else if (m.type == eco::MachineType::HarmonicResonator) {
            DrawHarmonicWaveMosaic(m.pos, poll, sim_time);
        } else if (m.type == eco::MachineType::CentralAstrolabe) {
            DrawSanctuaryGrandMosaic(m.pos, poll, sim_time, world.collectibles.CollectedCount());
        }
    }
    if (const eco::PortalLink* tower_portal = world.overworld.FindPortal(eco::StructureId::PendulumTower)) {
        const eco::Vec2 plaza{tower_portal->exit_spawn.x, tower_portal->exit_spawn.y + 18.0f};
        if (std::fabs(plaza.x - cam.x) <= half_w + 100.0f && std::fabs(plaza.y - cam.y) <= half_h + 100.0f) {
            DrawWindRoseMosaic(plaza, poll, sim_time);
        }
    }

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
            const eco::ZoneId zone = tilemap.DistrictAtTile(r, c);
            const pal::DistrictPalette& dp = pal::GetDistrictPalette(zone);
            const std::uint32_t h = Hash2(c, r, 91);

            const bool wn = IsWallLike(tilemap.GetTile(r - 1, c));
            const bool ws = IsWallLike(tilemap.GetTile(r + 1, c));
            const bool ww = IsWallLike(tilemap.GetTile(r, c - 1));
            const bool we = IsWallLike(tilemap.GetTile(r, c + 1));

            // Ombra portata morbida a 2 strati verso sud/sud-est
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

            // Se la tile a sud e' esposta disegniamo la faccia frontale 2.5D (15px)
            const int front_h = ws ? 0 : 15;
            const int top_h = tile_px - front_h;

            if (id == eco::TileId::RuinsWall) {
                // Rovine antiche stratificate: colonne scanalate, conci spezzati, rilievi in bronzo
                const Color ground_under = pal::WorldColor(dp.ground_sick, dp.ground_healthy, poll);
                DrawRectangle(rx, ry, tile_px, tile_px, ground_under);

                const unsigned variant = h % 3u;
                if (!wn && !ws && !ww && !we) {
                    // Colonna monumentale isolata scanalata (piloni acquedotto, pergolati, colonnato Santuario)
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
                        // Braciere di giada acceso sui capitelli del Santuario dell'Armonia
                        const float pulse = 0.7f + 0.3f * std::sin(sim_time * 3.0f + static_cast<float>(c + r));
                        DrawCircleV({tr.x + 24.0f, tr.y + 13.0f}, 9.0f * pulse,
                                    pal::WithAlpha(pal::kJadeGlow, static_cast<unsigned char>(45.0f + 75.0f * health)));
                        DrawCircleV({tr.x + 24.0f, tr.y + 13.0f}, 3.4f, pal::kJadeGlow);
                    } else if (zone == eco::ZoneId::HangingGardens && health > 0.35f) {
                        // Rampicanti fioriti avvolti attorno alle colonne dei Giardini Sospesi
                        DrawCircleV({tr.x + 17.0f, tr.y + 22.0f}, 3.2f, dp.canopy_healthy);
                        DrawCircleV({tr.x + 31.0f, tr.y + 28.0f}, 3.0f, dp.canopy_hi_healthy);
                        DrawCircleV({tr.x + 17.0f, tr.y + 22.0f}, 1.8f, dp.blossom_a);
                        DrawCircleV({tr.x + 31.0f, tr.y + 28.0f}, 1.8f, dp.blossom_b);
                    }
                } else {
                    // Blocco di rovine scolpite con spigoli erosi e fregio geometrico
                    const int inset_l = ww ? 0 : 3;
                    const int inset_r = we ? 0 : 3;
                    const int inset_t = wn ? 0 : 3;
                    DrawRectangle(rx + inset_l, ry + inset_t, tile_px - inset_l - inset_r, top_h - inset_t, top_col);
                    if (front_h > 0) {
                        DrawRectangle(rx + inset_l, ry + top_h, tile_px - inset_l - inset_r, front_h, front_col);
                        DrawLine(rx + inset_l, ry + top_h, rx + tile_px - inset_r, ry + top_h, edge_col);
                        // Fregio in bronzo inciso sulla faccia frontale
                        DrawLine(rx + inset_l + 4, ry + top_h + 7, rx + tile_px - inset_r - 4, ry + top_h + 7,
                                 pal::WithAlpha(pal::WorldColor(dp.inlay_sick, dp.inlay_healthy, poll), 175));
                    }
                    // Crepe variate per blocco e fughe a conci (mai linee identiche su blocchi adiacenti!)
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
                    if (!wn) {
                        DrawLine(rx + inset_l, ry + inset_t, rx + tile_px - inset_r, ry + inset_t, edge_col);
                    }
                    if (!ww) {
                        DrawLine(rx + inset_l, ry + inset_t, rx + inset_l, ry + tile_px - 1, edge_col);
                    }
                    // Capitello/fregio circolare sul tetto della rovina (o camino di fornace nella Caldera!)
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
                // Muratura monumentale continua (Wall)
                DrawRectangle(rx, ry, tile_px, top_h, top_col);
                if (front_h > 0) {
                    DrawRectangle(rx, ry + top_h, tile_px, front_h, front_col);
                    // Conci di pietra sulla facciata frontale
                    DrawLine(rx + 16, ry + top_h, rx + 16, ry + tile_px, mortar);
                    DrawLine(rx + 36, ry + top_h, rx + 36, ry + tile_px, mortar);
                    DrawLine(rx, ry + top_h + 7, rx + tile_px, ry + top_h + 7, pal::WithAlpha(mortar, 150));
                    // Cornice superiore della facciata
                    DrawLine(rx, ry + top_h, rx + tile_px, ry + top_h, pal::ShiftBrightness(edge_col, -12));
                    DrawLine(rx, ry + tile_px - 1, rx + tile_px, ry + tile_px - 1, mortar);
                }

                // Fughe leggere sulla sommita' per dare scala architettonica
                DrawLine(rx + 24, ry, rx + 24, ry + top_h, pal::WithAlpha(mortar, 85));
                if (!wn) {
                    DrawLine(rx, ry, rx + tile_px, ry, edge_col);
                    DrawLine(rx, ry + 1, rx + tile_px, ry + 1, pal::WithAlpha(edge_col, 140));
                }
                if (!ww) {
                    DrawLine(rx, ry, rx, ry + tile_px, edge_col);
                }
                if (!we) {
                    DrawLine(rx + tile_px - 1, ry, rx + tile_px - 1, ry + tile_px, mortar);
                }

                // Accento architettonico in bronzo/giada nel Santuario e nel Quartiere dei Canali
                if (!ws && (zone == eco::ZoneId::HarmonySanctuary || zone == eco::ZoneId::CanalQuarter) && ((c & 1) == 0)) {
                    const Color band = pal::WorldColor(dp.inlay_sick, dp.inlay_healthy, poll);
                    DrawRectangle(rx + 10, ry + top_h + 4, tile_px - 20, 3, pal::WithAlpha(band, 190));
                }
            }

            // Edera, muschio e piccoli fiori che abbracciano muri e rovine al calare dello smog
            if (!ws && health > 0.25f && (h & 3u) != 0u) {
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

namespace {

// Prop statici dell'Overworld: calcolati UNA sola volta (zero scansioni di mappa e zero
// allocazioni durante il loop di gioco); il disegno filtra solo per viewport.
struct StaticProp {
    float x = 0.0f;
    float y = 0.0f;
    float scale = 1.0f;
    std::uint32_t h = 0;
    unsigned roll = 0;
    eco::ZoneId zone = eco::ZoneId::DawnTerraces;
};

std::vector<StaticProp> g_prop_cache;
int g_prop_cache_rows = -1;
int g_prop_cache_cols = -1;
eco::Vec2 g_prop_cache_spawn{-1.0f, -1.0f};

void RebuildPropCacheIfNeeded(const eco::WorldContext& world) {
    const eco::Tilemap& tilemap = world.overworld.tilemap;
    if (g_prop_cache_rows == tilemap.Rows() && g_prop_cache_cols == tilemap.Cols() &&
        g_prop_cache_spawn.x == world.overworld.spawn_point.x &&
        g_prop_cache_spawn.y == world.overworld.spawn_point.y) {
        return;
    }
    g_prop_cache_rows = tilemap.Rows();
    g_prop_cache_cols = tilemap.Cols();
    g_prop_cache_spawn = world.overworld.spawn_point;
    g_prop_cache.clear();
    g_prop_cache.reserve(256);

    for (int r = 1; r <= tilemap.Rows() - 2; ++r) {
        for (int c = 1; c <= tilemap.Cols() - 2; ++c) {
            if (((r * 7 + c * 11) % 13) != 0) {
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
                    if (nb == eco::TileId::StructurePortal || nb == eco::TileId::HiddenBridge) {
                        solid_neighbors += 9;
                    } else if (IsWallLike(nb) || nb == eco::TileId::PavedPath) {
                        ++solid_neighbors;
                    }
                }
            }
            if (solid_neighbors >= 5) {
                continue;
            }
            const std::uint32_t h = Hash2(c, r, 409);
            const eco::Vec2 center = tilemap.TileCenter(r, c);
            const eco::Vec2 pos{
                center.x + static_cast<float>(static_cast<int>(h % 13u) - 6),
                center.y + static_cast<float>(static_cast<int>((h >> 4) % 13u) - 6)};
            if (!IsPropPositionSafe(world, pos)) {
                continue;
            }
            // Mai dentro/attorno ai varchi monumentali tra distretti
            bool near_gateway = false;
            for (const auto& gw : world.overworld.gateways) {
                if (pos.DistanceTo(gw.world_center) < 150.0f) {
                    near_gateway = true;
                    break;
                }
            }
            if (near_gateway) {
                continue;
            }
            StaticProp p;
            p.x = pos.x;
            p.y = pos.y;
            p.scale = 0.88f + static_cast<float>((h >> 8) % 25u) * 0.01f;
            p.h = h;
            p.roll = (h >> 14) % 12u;
            p.zone = tilemap.DistrictAtTile(r, c);
            g_prop_cache.push_back(p);
        }
    }
}

// Targa direzionale appesa ai pilastri del varco (testo non allocante)
void DrawGatewayPlaque(float cx, float y, const char* text, Color accent) {
    const int tw = MeasureUiText(text, 11);
    const int bw = tw + 16;
    const int bx = static_cast<int>(cx) - bw / 2;
    const int by = static_cast<int>(y);
    DrawRectangle(bx + 2, by + 2, bw, 18, Color{0, 0, 0, 100});
    DrawRectangle(bx, by, bw, 18, Color{16, 22, 32, 238});
    DrawRectangleLines(bx, by, bw, 18, pal::WithAlpha(pal::kBronze, 230));
    DrawRectangle(bx + 2, by + 2, 3, 14, accent);
    DrawRectangle(bx + bw - 5, by + 2, 3, 14, accent);
    DrawTextShadow(text, bx + 8, by + 3, 11, pal::kParchmentText);
}

void DrawGatewayPillar(float cx, float base_y, float height, Color stone, Color accent, float pulse) {
    const Color dark = pal::ShiftBrightness(stone, -34);
    const Color hi = pal::ShiftBrightness(stone, 26);
    DrawEllipse(static_cast<int>(cx + 4.0f), static_cast<int>(base_y + 2.0f), 17.0f, 6.0f, Color{0, 0, 0, 110});
    DrawRectangle(static_cast<int>(cx - 15.0f), static_cast<int>(base_y - 10.0f), 30, 12, dark);
    DrawRectangle(static_cast<int>(cx - 11.0f), static_cast<int>(base_y - height), 22, static_cast<int>(height - 8.0f), stone);
    DrawRectangle(static_cast<int>(cx + 4.0f), static_cast<int>(base_y - height), 7, static_cast<int>(height - 8.0f), dark);
    DrawLine(static_cast<int>(cx - 11.0f), static_cast<int>(base_y - height), static_cast<int>(cx - 11.0f), static_cast<int>(base_y - 10.0f), hi);
    // scanalature e fasce bronzo
    DrawLine(static_cast<int>(cx - 4.0f), static_cast<int>(base_y - height + 8.0f), static_cast<int>(cx - 4.0f), static_cast<int>(base_y - 14.0f), hi);
    DrawRectangle(static_cast<int>(cx - 13.0f), static_cast<int>(base_y - height + 12.0f), 26, 4, pal::kBronze);
    DrawRectangle(static_cast<int>(cx - 13.0f), static_cast<int>(base_y - 22.0f), 26, 4, pal::kBronze);
    // capitello + lanterna del distretto di destinazione
    DrawRectangle(static_cast<int>(cx - 15.0f), static_cast<int>(base_y - height - 5.0f), 30, 7, hi);
    DrawRectangleLines(static_cast<int>(cx - 15.0f), static_cast<int>(base_y - height - 5.0f), 30, 7, pal::kBronzeDark);
    const float ly = base_y - height - 17.0f;
    DrawCircleV({cx, ly}, 15.0f * pulse, pal::WithAlpha(accent, 58));
    DrawCircleV({cx, ly}, 6.2f, accent);
    DrawCircleV({cx, ly}, 2.8f, pal::kStoneIvory);
    DrawCircleLines(static_cast<int>(cx), static_cast<int>(ly), 6.2f, pal::kBronzeBright);
}

void DrawDistrictGateway(const eco::DistrictGateway& gw, const eco::WorldContext& world) {
    const float poll = world.ecosystem.PollutionRatio();
    const float tile = eco::kTileSize;
    const float pulse = 0.78f + 0.22f * std::sin(world.sim_time * 2.6f + gw.world_center.x * 0.01f);
    const pal::DistrictPalette& to_pal = pal::GetDistrictPalette(gw.to_zone);
    const pal::DistrictPalette& from_pal = pal::GetDistrictPalette(gw.from_zone);
    const Color to_acc = to_pal.ambient_accent;
    const Color from_acc = from_pal.ambient_accent;
    const Color stone = pal::WorldColor(to_pal.wall_top_sick, to_pal.wall_top_healthy, poll);
    const bool locked = gw.requires_bridge_unlock && !world.ecosystem.architect_recruited;

    const float x0 = static_cast<float>(gw.tile_min.col) * tile;
    const float x1 = static_cast<float>(gw.tile_max.col + 1) * tile;
    const float y0 = static_cast<float>(gw.tile_min.row) * tile;
    const float y1 = static_cast<float>(gw.tile_max.row + 1) * tile;

    if (gw.IsHorizontalWall()) {
        // Arco monumentale: pilastri a Ovest/Est, architrave con lanterne bicolore, stele direzionali N/S
        const float h = (y1 - y0) + 34.0f;
        const float base_y = y1 + 2.0f;
        DrawGatewayPillar(x0 - 6.0f, base_y, h, stone, from_acc, pulse);
        DrawGatewayPillar(x1 + 6.0f, base_y, h, stone, to_acc, pulse);
        // Architrave
        const float top = base_y - h - 4.0f;
        DrawRectangle(static_cast<int>(x0 - 18.0f), static_cast<int>(top - 7.0f), static_cast<int>(x1 - x0 + 36.0f), 12,
                      pal::ShiftBrightness(stone, 14));
        DrawRectangleLines(static_cast<int>(x0 - 18.0f), static_cast<int>(top - 7.0f), static_cast<int>(x1 - x0 + 36.0f), 12, pal::kBronze);
        DrawRectangleGradientH(static_cast<int>(x0), static_cast<int>(top - 4.0f), static_cast<int>(x1 - x0), 6,
                               pal::WithAlpha(from_acc, 220), pal::WithAlpha(to_acc, 220));
        // Tappeto di luce sul passaggio, sfumato verso la zona di destinazione
        DrawRectangleGradientV(static_cast<int>(x0), static_cast<int>(y0), static_cast<int>(x1 - x0), static_cast<int>(y1 - y0),
                               pal::WithAlpha(to_acc, static_cast<unsigned char>(42.0f * pulse)), pal::WithAlpha(from_acc, 34));
        // Stele direzionali: sopra (verso nord) e sotto (verso sud)
        const float mid_x = (x0 + x1) * 0.5f;
        // La stele di andata va sul lato da cui si parte: 'N ->' sul lato sud, 'S ->' sul lato nord
        const bool go_north = (gw.signpost_label[0] == 'N');
        DrawGatewayPlaque(mid_x, go_north ? (y1 + 18.0f) : (top - 30.0f), gw.signpost_label, to_acc);
        DrawGatewayPlaque(mid_x, go_north ? (top - 30.0f) : (y1 + 18.0f), gw.reverse_label, from_acc);
    } else {
        // Varco verticale (Ponte degli Archi): pilastri a Nord/Sud, targhe a Est/Ovest
        const float mid_y = (y0 + y1) * 0.5f;
        const Color acc_east = locked ? pal::kAccentWarn : pal::kJadeGlow;
        DrawGatewayPillar(x0 - 22.0f, y0 - 8.0f, 54.0f, stone, locked ? pal::kAccentWarn : from_acc, pulse);
        DrawGatewayPillar(x0 - 22.0f, y1 + 22.0f, 54.0f, stone, locked ? pal::kAccentWarn : to_acc, pulse);
        DrawGatewayPillar(x1 + 22.0f, y0 - 8.0f, 54.0f, stone, locked ? pal::kAccentWarn : from_acc, pulse);
        DrawGatewayPillar(x1 + 22.0f, y1 + 22.0f, 54.0f, stone, locked ? pal::kAccentWarn : to_acc, pulse);
        // 'E ->' sul lato ovest (si parte da li'), ritorno sul lato est
        DrawGatewayPlaque(x0 - 70.0f, mid_y - 9.0f, gw.signpost_label, acc_east);
        DrawGatewayPlaque(x1 + 70.0f, mid_y - 9.0f, gw.reverse_label, from_acc);
    }
}

} // namespace

// =============================================================================
// Overworld Props — Flora Diversificata, Lanterne, Bio-Cristalli & Stele
// =============================================================================
void DrawOverworldProceduralProps(const eco::WorldContext& world) {
    const float poll = world.ecosystem.PollutionRatio();
    const float health = 1.0f - poll;
    const float sim_time = world.sim_time;
    const eco::Vec2 cam = world.overworld.camera_target;
    const float half_w = static_cast<float>(eco::kWindowWidth) * 0.5f + 120.0f;
    const float half_h = static_cast<float>(eco::kWindowHeight) * 0.5f + 120.0f;

    RebuildPropCacheIfNeeded(world);

    // 0. Varchi monumentali tra i distretti (DistrictGateway) — sotto flora/prop
    for (const auto& gw : world.overworld.gateways) {
        if (std::fabs(gw.world_center.x - cam.x) > half_w + 160.0f ||
            std::fabs(gw.world_center.y - cam.y) > half_h + 160.0f) {
            continue;
        }
        DrawDistrictGateway(gw, world);
    }

    // 1. Prop statici pre-calcolati: solo filtro viewport
    for (const StaticProp& sp : g_prop_cache) {
        if (std::fabs(sp.x - cam.x) > half_w || std::fabs(sp.y - cam.y) > half_h) {
            continue;
        }
        const float px = sp.x;
        const float py = sp.y;
        const std::uint32_t h = sp.h;
        const float scale = sp.scale;
        const unsigned roll = sp.roll;
        const eco::ZoneId zone = sp.zone;
        const pal::DistrictPalette& dp = pal::GetDistrictPalette(zone);
        const eco::Vec2 pos{px, py};
        {
            if (roll == 0u) {
                DrawBioCrystalNode(pos.x, pos.y, sim_time, poll, dp.ambient_accent);
            } else if (roll == 1u) {
                DrawJadeLanternPillar(pos.x, pos.y, sim_time, poll, dp.ambient_accent);
            } else if (roll == 2u && (zone == eco::ZoneId::DawnTerraces || zone == eco::ZoneId::HarmonySanctuary || zone == eco::ZoneId::WindPlateau)) {
                DrawBrokenColumnDrum(pos.x, pos.y, poll, dp);
            } else if (roll == 3u && zone != eco::ZoneId::FumaroleCaldera) {
                const bool hydro = (zone == eco::ZoneId::CanalQuarter || zone == eco::ZoneId::HangingGardens);
                DrawMeridianStatue(pos.x, pos.y, hydro, poll, sim_time, dp);
            } else if (roll == 4u && (zone == eco::ZoneId::HangingGardens || zone == eco::ZoneId::HarmonySanctuary)) {
                DrawStonePlanterUrn(pos.x, pos.y, poll, sim_time, dp);
            } else if (roll == 4u && (zone == eco::ZoneId::DawnTerraces || zone == eco::ZoneId::WindPlateau)) {
                DrawSundialObelisk(pos.x, pos.y, poll, sim_time, dp);
            } else {
                // Flora specifica del distretto
                switch (zone) {
                    case eco::ZoneId::DawnTerraces:
                    case eco::ZoneId::HarmonySanctuary:
                        DrawOliveTree(pos.x, pos.y, scale, sim_time, poll, dp, static_cast<int>(h & 0xFFu));
                        break;
                    case eco::ZoneId::WindPlateau:
                    case eco::ZoneId::CanalQuarter:
                        DrawWindCypress(pos.x, pos.y, scale, sim_time, poll, dp, static_cast<int>(h & 0xFFu));
                        break;
                    case eco::ZoneId::HangingGardens:
                        DrawFloweringGardenTree(pos.x, pos.y, scale, sim_time, poll, dp, static_cast<int>(h & 0xFFu));
                        break;
                    case eco::ZoneId::FumaroleCaldera:
                        if ((h & 1u) == 0u) {
                            DrawBasaltThermalVent(pos.x, pos.y, scale, sim_time, poll, static_cast<int>(h & 0xFFu));
                        } else {
                            DrawOliveTree(pos.x, pos.y, scale * 0.86f, sim_time, poll, dp, static_cast<int>(h & 0xFFu));
                        }
                        break;
                    case eco::ZoneId::Count:
                        break;
                }
            }
        }
    }

    // 2. Lanterne architettoniche di benvenuto ai lati del Ponte degli Archi
    const float bridge_x = (static_cast<float>(eco::Tilemap::kBridgeDividerCol) + 0.5f) * eco::kTileSize;
    const float bridge_y_top = 15.4f * eco::kTileSize;
    const float bridge_y_bot = 19.3f * eco::kTileSize;
    const Color bridge_light = world.ecosystem.architect_recruited ? pal::kJadeGlow : pal::kAmberGlow;
    if (std::fabs(bridge_x - cam.x) <= half_w) {
        DrawJadeLanternPillar(bridge_x - eco::kTileSize * 1.15f, bridge_y_top, sim_time, poll, bridge_light);
        DrawJadeLanternPillar(bridge_x - eco::kTileSize * 1.15f, bridge_y_bot, sim_time, poll, bridge_light);
        DrawJadeLanternPillar(bridge_x + eco::kTileSize * 1.15f, bridge_y_top, sim_time, poll, bridge_light);
        DrawJadeLanternPillar(bridge_x + eco::kTileSize * 1.15f, bridge_y_bot, sim_time, poll, bridge_light);
    }

    // 3. Stele Diegetiche scolpite (posizionate dinamicamente da world.overworld.portals)
    for (const auto& portal : world.overworld.portals) {
        const float sx = portal.world_center.x;
        const float sy = static_cast<float>(portal.tile_min.row) * eco::kTileSize - 10.0f;
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

    // Stele diegetica del Ponte degli Archi (posizionata su terreno asciutto a nord-ovest del ponte)
    const float stele_bridge_x = 22.6f * eco::kTileSize;
    const float stele_bridge_y = 15.35f * eco::kTileSize;
    if (std::fabs(stele_bridge_x - cam.x) <= half_w && std::fabs(stele_bridge_y - cam.y) <= half_h) {
        DrawMeridianStele(
            stele_bridge_x,
            stele_bridge_y,
            world.ecosystem.architect_recruited ? "PONTE DEGLI ARCHI" : "PONTE SIGILLATO",
            world.ecosystem.architect_recruited ? pal::kAccentClean : pal::kAccentWarn,
            health);
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

    // 1. Sfondo in parallasse dedicato a ciascuna struttura
    if (sid == eco::StructureId::HypogeanCisterns) {
        DrawCisternsBackdrop(world_w, world_h, parallax_x, poll, sim_time, sp);
    } else if (sid == eco::StructureId::PendulumTower) {
        DrawPendulumTowerBackdrop(world_w, world_h, parallax_x, poll, sim_time, sp);
    } else {
        DrawGeothermalBackdrop(world_w, world_h, parallax_x, poll, sim_time, sp);
    }

    // 2. Tilemap 2.5D strutturale (piattaforme sospese, murature scolpite, portale di uscita)
    const int tile_px = static_cast<int>(tilemap.TileSize());
    const Color wall_top = pal::WorldColor(sp.wall_top_sick, sp.wall_top_healthy, poll);
    const Color wall_dark = pal::WorldColor(sp.wall_dark_sick, sp.wall_dark_healthy, poll);
    const Color plat_top = pal::WorldColor(sp.plat_top_sick, sp.plat_top_healthy, poll);
    const Color plat_body = pal::WorldColor(sp.plat_body_sick, sp.plat_body_healthy, poll);
    const Color plat_rim = pal::WorldColor(sp.plat_rim_sick, sp.plat_rim_healthy, poll);
    const Color accent = pal::WorldColor(sp.accent_sick, sp.accent_healthy, poll);

    // Viewport culling stretto: solo le tile visibili (margine 2 tile per ombre/lanterne)
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

            if (IsWallLike(id)) {
                const bool is_perimeter = (r == 0 || r >= tilemap.Rows() - 2 || c == 0 || c == row_cols - 1);
                if (!is_perimeter) {
                    // Piattaforma / ballatoio sopraelevato con stile dedicato per struttura
                    DrawRectangle(rx + 2, ry + tile_px, tile_px - 2, 8, Color{0, 0, 0, 88});
                    DrawRectangle(rx, ry, tile_px, 13, plat_top);
                    DrawRectangle(rx, ry + 13, tile_px, tile_px - 13, plat_body);

                    if (sid == eco::StructureId::HypogeanCisterns) {
                        // Arco di rinforzo in bronzo e pietra scolpita sotto la passerella
                        DrawRing({tr.x + tr.width * 0.5f, tr.y + tr.height}, 14.0f, 18.0f, 180.0f, 360.0f, 12, pal::WithAlpha(pal::kBronze, 180));
                        DrawLine(rx + 24, ry, rx + 24, ry + 13, pal::ShiftBrightness(plat_top, -22));
                    } else if (sid == eco::StructureId::PendulumTower) {
                        // Ballatoio d'osservatorio in ottone con tacche graduate
                        for (int tx = rx + 6; tx < rx + tile_px - 4; tx += 8) {
                            DrawLine(tx, ry + 14, tx, ry + 26, pal::WithAlpha(plat_rim, 165));
                        }
                        DrawLine(rx + 4, ry + 28, rx + tile_px - 4, ry + 28, pal::WithAlpha(pal::kBronzeBright, 150));
                    } else {
                        // Passerella geotermica in rame rivettato con traliccio termale
                        DrawLine(rx + 4, ry + 15, rx + tile_px - 4, ry + tile_px - 4, pal::ShiftBrightness(plat_body, 24));
                        DrawLine(rx + tile_px - 4, ry + 15, rx + 4, ry + tile_px - 4, pal::ShiftBrightness(plat_body, 24));
                        DrawCircle(rx + 6, ry + 7, 2.0f, pal::kBronzeBright);
                        DrawCircle(rx + tile_px - 6, ry + 7, 2.0f, pal::kBronzeBright);
                    }

                    DrawRectangleLines(rx, ry, tile_px, tile_px, pal::ShiftBrightness(plat_body, -16));
                    DrawLineEx({tr.x + 1.0f, tr.y + 2.5f}, {tr.x + tr.width - 1.0f, tr.y + 2.5f}, 2.4f, plat_rim);

                    // Lanterne sospese o sfoghi termali sotto le piattaforme (ogni 3 tile)
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
                    // Murature perimetrali e pavimento a conci scolpiti
                    const Color block = pal::ShiftBrightness(wall_top, static_cast<int>(h % 9u) - 4);
                    DrawRectangle(rx, ry, tile_px, tile_px, wall_dark);
                    DrawRectangle(rx + 2, ry + 2, tile_px - 4, tile_px - 8, block);
                    DrawLine(rx + 2, ry + 2, rx + tile_px - 2, ry + 2, pal::ShiftBrightness(block, 20));
                    DrawLine(rx + 24, ry + 2, rx + 24, ry + tile_px - 6, pal::ShiftBrightness(wall_dark, -8));
                    DrawRectangleLines(rx, ry, tile_px, tile_px, pal::ShiftBrightness(wall_dark, -10));

                    if (r == 0 && c > 0 && c < row_cols - 1 && (c % 2 == 0)) {
                        // Mensole/archi scolpiti lungo la volta superiore
                        DrawRectangle(rx + 14, ry + tile_px, 20, 7, wall_dark);
                        DrawLine(rx + 14, ry + tile_px + 7, rx + 34, ry + tile_px + 7, pal::WithAlpha(pal::kBronze, 150));
                    }
                    if (r == tilemap.Rows() - 2 && c > 0 && c < row_cols - 1) {
                        // Bordo superiore del pavimento calpestabile con intarsio luminoso
                        DrawLineEx({tr.x, tr.y + 1.5f}, {tr.x + tr.width, tr.y + 1.5f}, 2.5f, pal::WithAlpha(accent, 215));
                    }
                }
            } else if (id == eco::TileId::ExitPortal) {
                // Portale monumentale d'uscita verso la Conca di Meridia
                const float pulse = 0.5f + 0.5f * std::sin(sim_time * 3.6f);
                const Color glow = pal::LerpColor(pal::kBronze, pal::kAmberGlow, pulse);
                DrawRectangle(rx, ry, tile_px, tile_px, Color{24, 52, 68, 255});
                DrawRectangle(rx + 5, ry + 5, tile_px - 10, tile_px - 5, pal::LerpColor(Color{38, 102, 128, 255}, Color{84, 194, 214, 255}, 0.35f + 0.65f * health));
                DrawRectangleLinesEx(Rectangle{tr.x + 2.0f, tr.y + 2.0f, tr.width - 4.0f, tr.height - 4.0f}, 2.2f, glow);
                DrawTextShadow("USCITA", rx + 5, ry + 18, 10, pal::kParchmentText);
            }
        }
    }

    // 3. Stele diegetica d'uscita posizionata dinamicamente sopra i tile ExitPortal
    // Posizione stele d'uscita: calcolata una sola volta per struttura (zero allocazioni per-frame)
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
        const float ex = ec_slot.x;
        const float ey = ec_slot.y;
        const char* exit_label = "CONCA DI MERIDIA";
        if (sid == eco::StructureId::PendulumTower) {
            exit_label = "ALTOPIANO [USCITA]";
        } else if (sid == eco::StructureId::GeothermalChamber) {
            exit_label = "CALDERA [USCITA]";
        } else {
            exit_label = "CANALI [USCITA]";
        }
        DrawMeridianStele(ex, ey, exit_label, pal::kAmberGlow, health);
    }
}

} // namespace eco::app
