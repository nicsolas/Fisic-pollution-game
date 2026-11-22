#include "app/render/procedural_art.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>

#include "app/render/palette.h"
#include "app/render/ui_style.h"

namespace eco::app {

namespace {

[[nodiscard]] bool IsPavedPathTile(int r, int c) noexcept {
    // Sentiero lastricato dell'ex campus Unical: collega piazza spawn (c=4..8, r=8..9)
    // -> Campo Solare (c=7..8, r=10) -> Lia Architetta (c=18, r=6) -> Ingresso Diga (c=15..16, r=5)
    // -> Ponte Bucci (c=20, r=8..9) -> Risonatore Est (c=23, r=9).
    if ((r == 8 || r == 9) && c >= 3 && c <= 24 && c != 12 && c != 20) {
        return true;
    }
    if (c == 15 && r >= 5 && r <= 8) {
        return true;
    }
    if (c == 18 && r >= 6 && r <= 8) {
        return true;
    }
    if (c == 7 && r >= 8 && r <= 11) {
        return true;
    }
    return false;
}

void DrawBioCrystalNode(float x, float y, float sim_time, const eco::EcoSystem& eco) {
    const float poll = eco.PollutionRatio();
    const float health = 1.0f - poll;
    const Color stem_col = pal::WorldColor(Color{70, 62, 54, 255}, Color{36, 118, 92, 255}, poll);
    const Color crystal_col = pal::WorldColor(
        Color{142, 82, 58, 225},
        Color{55, 240, 215, 245},
        poll);

    DrawEllipse(static_cast<int>(x), static_cast<int>(y + 11.0f), 14.0f, 5.5f, Color{0, 0, 0, 105});
    DrawLineEx({x, y + 10.0f}, {x, y - 9.0f}, 3.5f, stem_col);
    DrawLineEx({x - 5.0f, y + 8.0f}, {x - 10.0f, y - 2.0f}, 2.5f, stem_col);
    DrawLineEx({x + 5.0f, y + 8.0f}, {x + 10.0f, y - 3.0f}, 2.5f, stem_col);

    const float pulse = std::sin(sim_time * 3.2f + x * 0.1f) * 2.2f * health;
    if (health > 0.25f) {
        DrawCircleV({x, y - 10.0f}, 14.0f + pulse, pal::WithAlpha(crystal_col, static_cast<unsigned char>(55.0f * health)));
    }
    DrawPoly({x, y - 10.0f}, 6, 9.5f + pulse, sim_time * 18.0f, crystal_col);
    DrawPolyLines({x, y - 10.0f}, 6, 9.5f + pulse, sim_time * 18.0f, RAYWHITE);
    DrawPoly({x - 10.0f, y - 3.0f}, 5, 5.0f, -sim_time * 12.0f, crystal_col);
    DrawPoly({x + 10.0f, y - 4.0f}, 5, 5.0f, sim_time * 14.0f, crystal_col);
}

void DrawProceduralTree(float x, float y, float scale, float sim_time, const eco::EcoSystem& eco, int seed) {
    const float poll = eco.PollutionRatio();
    const float health = 1.0f - poll;
    const float sway = std::sin(sim_time * 1.6f + static_cast<float>(seed) * 1.7f) * (1.2f + 2.2f * health);

    // Ombra morbida proiettata verso sud-est
    DrawEllipse(
        static_cast<int>(x + 6.0f * scale),
        static_cast<int>(y + 12.0f * scale),
        20.0f * scale,
        8.0f * scale,
        Color{0, 0, 0, 115});

    // Tronco e rami principali
    const Color trunk_dark = pal::WorldColor(Color{48, 38, 34, 255}, Color{68, 46, 32, 255}, poll);
    const Color trunk_light = pal::WorldColor(pal::kSickTrunk, pal::kHealthyTrunk, poll);
    DrawLineEx({x, y + 11.0f * scale}, {x + sway * 0.3f, y - 14.0f * scale}, 7.0f * scale, trunk_dark);
    DrawLineEx({x - 1.2f * scale, y + 10.0f * scale}, {x - 1.0f * scale + sway * 0.3f, y - 14.0f * scale}, 3.5f * scale, trunk_light);
    DrawLineEx({x, y - 4.0f * scale}, {x - 11.0f * scale + sway * 0.5f, y - 15.0f * scale}, 3.2f * scale, trunk_dark);
    DrawLineEx({x, y - 6.0f * scale}, {x + 11.0f * scale + sway * 0.5f, y - 16.0f * scale}, 3.2f * scale, trunk_dark);

    // Chioma: quando molto inquinato e' rada e color cenere, quando guarito e' rigogliosa a 3 strati
    const float canopy_r = (11.0f + 7.5f * health) * scale;
    const Color c_shadow = pal::WorldColor(Color{62, 54, 44, 230}, Color{28, 98, 58, 245}, poll);
    const Color c_mid = pal::WorldColor(pal::kSickCanopy, pal::kHealthyCanopy, poll);
    const Color c_high = pal::WorldColor(pal::kSickCanopyHighlight, pal::kHealthyCanopyHighlight, poll);

    const Vector2 center{x + sway, y - 20.0f * scale};
    DrawCircleV({center.x + 3.0f * scale, center.y + 3.0f * scale}, canopy_r, c_shadow);
    DrawCircleV({center.x - 9.0f * scale, center.y + 4.0f * scale}, canopy_r * 0.78f, c_mid);
    DrawCircleV({center.x + 9.0f * scale, center.y + 4.0f * scale}, canopy_r * 0.76f, c_mid);
    DrawCircleV(center, canopy_r * 0.92f, c_mid);
    DrawCircleV({center.x - 4.0f * scale, center.y - 4.0f * scale}, canopy_r * 0.62f, c_high);

    // Boccioli luminosi quando la Valle e' quasi pura
    if (health > 0.55f) {
        const Color blossom{135, 255, 215, static_cast<unsigned char>((health - 0.55f) * 500.0f)};
        DrawCircleV({center.x - 7.0f * scale, center.y - 2.0f * scale}, 2.4f * scale, blossom);
        DrawCircleV({center.x + 6.0f * scale, center.y + 1.0f * scale}, 2.2f * scale, blossom);
        DrawCircleV({center.x + 1.0f * scale, center.y - 8.0f * scale}, 2.5f * scale, Color{255, 235, 130, blossom.a});
    }
}

void DrawDiegeticSignpost(float x, float y, const char* label, Color accent) {
    DrawEllipse(static_cast<int>(x), static_cast<int>(y + 12.0f), 10.0f, 3.5f, Color{0, 0, 0, 100});
    DrawRectangle(static_cast<int>(x - 2.0f), static_cast<int>(y - 6.0f), 4, 18, Color{70, 56, 44, 255});
    const int tw = MeasureUiText(label, 11);
    const int bw = tw + 12;
    const int bx = static_cast<int>(x) - bw / 2;
    const int by = static_cast<int>(y - 22.0f);
    DrawRectangle(bx, by, bw, 17, Color{16, 26, 38, 235});
    DrawRectangleLines(bx, by, bw, 17, accent);
    DrawTextShadow(label, bx + 6, by + 3, 11, accent);
}

} // namespace

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

void DrawOverworldProceduralMap(
    const eco::Tilemap& tilemap,
    const eco::EcoSystem& ecosystem,
    float sim_time) {
    const int tile_px = static_cast<int>(tilemap.TileSize());
    const float poll = ecosystem.PollutionRatio();
    const float health = 1.0f - poll;
    const bool bridge_unlocked = ecosystem.architect_recruited;

    // Passata 1: Terreno, Sentieri, Canale del Crati, Ponte Bucci e Portale Diga
    for (int r = 0; r < tilemap.Rows(); ++r) {
        const int row_cols = tilemap.RowCols(r);
        for (int c = 0; c < row_cols; ++c) {
            const eco::Rect tr = tilemap.TileRect(r, c);
            const int rx = static_cast<int>(tr.x);
            const int ry = static_cast<int>(tr.y);
            const eco::TileId id = tilemap.GetTile(r, c);
            const std::uint32_t h = Hash2(c, r, 17);
            const int tone_delta = static_cast<int>(h % 11u) - 5;

            if (id == eco::TileId::Walkable) {
                const bool is_east = (c > 20);
                Color ground = is_east
                                   ? pal::WorldColor(pal::kSickGroundEast, pal::kHealthyGroundEast, poll)
                                   : pal::WorldColor(pal::kSickGroundWest, pal::kHealthyGroundWest, poll);
                ground = pal::ShiftBrightness(ground, tone_delta);
                DrawRectangle(rx, ry, tile_px, tile_px, ground);

                // Sentiero lastricato in pietra tra i punti chiave del campus
                if (IsPavedPathTile(r, c)) {
                    const Color stone = pal::ShiftBrightness(
                        pal::WorldColor(pal::kSickPath, pal::kHealthyPath, poll),
                        tone_delta);
                    DrawRectangle(rx + 4, ry + 4, tile_px - 8, tile_px - 8, stone);
                    DrawRectangleLines(rx + 4, ry + 4, tile_px - 8, tile_px - 8, pal::ShiftBrightness(stone, -18));
                    DrawLine(rx + 24, ry + 5, rx + 24, ry + tile_px - 5, pal::ShiftBrightness(stone, -14));
                    DrawLine(rx + 5, ry + 24, rx + tile_px - 5, ry + 24, pal::ShiftBrightness(stone, -14));
                } else {
                    // Micro-dettagli organici deterministici (ciottoli, ciuffi d'erba, fiori)
                    const Color grass_col = pal::WorldColor(pal::kSickGrassAccent, pal::kHealthyGrassAccent, poll);
                    if ((h & 3u) == 0u) {
                        const int gx = rx + 10 + static_cast<int>((h >> 4) % 26u);
                        const int gy = ry + 12 + static_cast<int>((h >> 9) % 24u);
                        const int blade_h = 3 + static_cast<int>(health * 5.0f);
                        DrawLine(gx, gy, gx - 2, gy - blade_h, grass_col);
                        DrawLine(gx, gy, gx + 2, gy - blade_h, grass_col);
                        if (health > 0.6f && ((h >> 14) & 3u) == 0u) {
                            const Color flower = ((h >> 16) & 1u)
                                                     ? Color{255, 225, 95, 235}
                                                     : Color{90, 245, 225, 235};
                            DrawCircle(gx, gy - blade_h - 1, 2.2f, flower);
                        }
                    }
                    if (((h >> 3) & 7u) == 1u) {
                        const int px = rx + 14 + static_cast<int>((h >> 11) % 20u);
                        const int py = ry + 14 + static_cast<int>((h >> 15) % 20u);
                        DrawCircle(px, py, 2.5f, pal::ShiftBrightness(ground, -16));
                        DrawCircle(px - 1, py - 1, 1.5f, pal::ShiftBrightness(ground, 14));
                    }
                }
            } else if (id == eco::TileId::Wall && c == 20 && r > 0 && r < tilemap.Rows() - 1) {
                // Canale del fiume Crati (colonna 20)
                const Color water_base = pal::WorldColor(pal::kSickWater, pal::kHealthyWater, poll);
                const Color water_deep = pal::ShiftBrightness(water_base, -18);
                const Color foam = pal::WorldColor(pal::kSickWaterFoam, pal::kHealthyWaterFoam, poll);

                DrawRectangle(rx, ry, tile_px, tile_px, water_deep);
                DrawRectangle(rx + 5, ry, tile_px - 10, tile_px, water_base);

                // Sponde in pietra scolpita a sinistra e destra del canale
                const Color bank_col = pal::WorldColor(pal::kSickWallTop, pal::kHealthyWallTop, poll);
                DrawRectangle(rx, ry, 5, tile_px, bank_col);
                DrawRectangle(rx + tile_px - 5, ry, 5, tile_px, bank_col);
                DrawLine(rx + 5, ry, rx + 5, ry + tile_px, foam);
                DrawLine(rx + tile_px - 5, ry, rx + tile_px - 5, ry + tile_px, foam);

                // Onde di corrente che scorrono verso sud
                for (int w = 0; w < 3; ++w) {
                    const float flow = std::fmod(sim_time * 24.0f + static_cast<float>(r * 16 + w * 16), 48.0f);
                    const float wy = tr.y + flow;
                    const float wave_dx = std::sin(sim_time * 3.0f + static_cast<float>(r + w)) * 3.0f;
                    DrawLineEx(
                        {tr.x + 11.0f + wave_dx, wy},
                        {tr.x + tr.width - 11.0f + wave_dx, wy},
                        2.0f,
                        foam);
                }
            } else if (id == eco::TileId::HiddenBridge) {
                // Sottofondo acqua del canale sotto il Ponte Bucci
                const Color water_base = pal::WorldColor(pal::kSickWater, pal::kHealthyWater, poll);
                DrawRectangle(rx, ry, tile_px, tile_px, water_base);
                if (bridge_unlocked) {
                    // PONTE BUCCI (Unical): passerella a travatura reticolare sospesa
                    DrawRectangle(rx - 2, ry + 2, tile_px + 4, tile_px - 4, Color{42, 58, 74, 250});
                    DrawRectangle(rx + 2, ry + 6, tile_px - 4, tile_px - 12, Color{64, 88, 108, 255});
                    for (int b = 6; b < tile_px - 4; b += 8) {
                        DrawLine(rx + 2, ry + b, rx + tile_px - 2, ry + b, Color{95, 245, 230, 215});
                    }
                    // Parapetti reticolari a croce di Sant'Andrea
                    DrawRectangle(rx - 2, ry, tile_px + 4, 5, Color{150, 185, 205, 255});
                    DrawRectangle(rx - 2, ry + tile_px - 5, tile_px + 4, 5, Color{90, 120, 145, 255});
                    DrawLineEx({tr.x, tr.y + 3.0f}, {tr.x + tr.width, tr.y + 3.0f}, 2.0f, pal::kAccentClean);
                    DrawLineEx({tr.x, tr.y + tr.height - 3.0f}, {tr.x + tr.width, tr.y + tr.height - 3.0f}, 2.0f, pal::kAccentClean);
                } else {
                    // Ponte Bucci interrotto: monconi di travi e barriera laser rossa sulle due sponde
                    DrawRectangle(rx, ry + 4, 9, tile_px - 8, Color{72, 68, 64, 255});
                    DrawRectangle(rx + tile_px - 9, ry + 4, 9, tile_px - 8, Color{72, 68, 64, 255});
                    const float l_alpha = 155.0f + 95.0f * std::sin(sim_time * 7.0f);
                    const Color laser_col{255, 65, 65, static_cast<unsigned char>(l_alpha)};
                    DrawLineEx({tr.x + 6.0f, tr.y}, {tr.x + 6.0f, tr.y + tr.height}, 3.0f, laser_col);
                    DrawLineEx({tr.x + tr.width - 6.0f, tr.y}, {tr.x + tr.width - 6.0f, tr.y + tr.height}, 3.0f, laser_col);
                }
            } else if (id == eco::TileId::StructurePortal) {
                // Ingresso monumentale alla Diga del Crati con paratoia e cascata
                const Color concrete = pal::WorldColor(pal::kSickWallFront, pal::kHealthyWallFront, poll);
                DrawRectangle(rx, ry, tile_px, tile_px, concrete);
                const float pulse = 0.5f + 0.5f * std::sin(sim_time * 4.5f);
                const Color water_glow = pal::LerpColor(Color{35, 130, 185, 255}, Color{95, 235, 255, 255}, pulse);
                DrawRectangle(rx + 4, ry + 6, tile_px - 8, tile_px - 10, Color{14, 38, 56, 255});
                for (int s = 0; s < 3; ++s) {
                    const float sy = tr.y + 8.0f + std::fmod(sim_time * 32.0f + static_cast<float>(s * 12), 32.0f);
                    DrawLineEx({tr.x + 8.0f, sy}, {tr.x + tr.width - 8.0f, sy}, 2.0f, water_glow);
                }
                DrawRectangleLinesEx(
                    Rectangle{tr.x + 2.0f, tr.y + 2.0f, tr.width - 4.0f, tr.height - 4.0f},
                    2.0f,
                    water_glow);
            } else if (id == eco::TileId::ExitPortal) {
                DrawRectangle(rx, ry, tile_px, tile_px, Color{185, 135, 45, 255});
                DrawRectangleLines(rx + 2, ry + 2, tile_px - 4, tile_px - 4, GOLD);
            }
        }
    }

    // Passata 2: Muri e Rovine in rilievo 2.5D con ombra portata a terra e muschio procedurale
    for (int r = 0; r < tilemap.Rows(); ++r) {
        const int row_cols = tilemap.RowCols(r);
        for (int c = 0; c < row_cols; ++c) {
            if (tilemap.GetTile(r, c) != eco::TileId::Wall) {
                continue;
            }
            if (c == 20 && r > 0 && r < tilemap.Rows() - 1) {
                continue; // Canale gia' disegnato
            }

            const eco::Rect tr = tilemap.TileRect(r, c);
            const int rx = static_cast<int>(tr.x);
            const int ry = static_cast<int>(tr.y);
            const std::uint32_t h = Hash2(c, r, 91);

            // Ombra portata a sud/sud-est se la tile sottostante e' calpestabile
            if (r + 1 < tilemap.Rows() && tilemap.GetTile(r + 1, c) != eco::TileId::Wall) {
                DrawRectangle(rx + 4, ry + tile_px, tile_px, 10, Color{0, 0, 0, 85});
            }
            if (c + 1 < row_cols && tilemap.GetTile(r, c + 1) != eco::TileId::Wall) {
                DrawRectangle(rx + tile_px, ry + 4, 8, tile_px, Color{0, 0, 0, 65});
            }

            const Color top_col = pal::ShiftBrightness(
                pal::WorldColor(pal::kSickWallTop, pal::kHealthyWallTop, poll),
                static_cast<int>(h % 9u) - 4);
            const Color front_col = pal::WorldColor(pal::kSickWallFront, pal::kHealthyWallFront, poll);
            const Color edge_col = pal::WorldColor(pal::kSickWallEdge, pal::kHealthyWallEdge, poll);

            // Faccia superiore (34 px) + Faccia frontale in ombra (14 px) = rilievo 2.5D
            constexpr int kFrontH = 14;
            const int top_h = tile_px - kFrontH;
            DrawRectangle(rx, ry, tile_px, top_h, top_col);
            DrawRectangle(rx, ry + top_h, tile_px, kFrontH, front_col);

            // Luce sul bordo nord-ovest e linea di spigolo tra top e front
            DrawLine(rx, ry, rx + tile_px, ry, edge_col);
            DrawLine(rx, ry, rx, ry + top_h, edge_col);
            DrawLine(rx, ry + top_h, rx + tile_px, ry + top_h, pal::ShiftBrightness(edge_col, -20));
            DrawRectangleLines(rx, ry, tile_px, tile_px, pal::ShiftBrightness(front_col, -16));

            // Dettaglio conci di pietra / crepe sulla faccia frontale
            DrawLine(rx + 24, ry + top_h, rx + 24, ry + tile_px, pal::ShiftBrightness(front_col, -14));

            // Rampicanti e muschio che ricoprono le rovine quando l'inquinamento scende
            if (health > 0.3f && (h & 3u) != 0u) {
                const Color moss = pal::WorldColor(Color{74, 86, 54, 180}, Color{58, 172, 96, 225}, poll);
                const int mx = rx + 6 + static_cast<int>((h >> 5) % 24u);
                const int mw = 8 + static_cast<int>(health * 10.0f);
                DrawRectangle(mx, ry + top_h - 3, mw, 6, moss);
                DrawCircle(mx + mw / 2, ry + top_h + 3, 3.5f * health, moss);
            }
        }
    }
}

void DrawOverworldProceduralProps(const eco::WorldContext& world) {
    const float t_sim = world.sim_time;
    const eco::EcoSystem& eco = world.ecosystem;

    // 1. Nodi di Bio-Cristallo (reattivi alla salute dell'ecosistema)
    DrawBioCrystalNode(260.0f, 220.0f, t_sim, eco);
    DrawBioCrystalNode(680.0f, 520.0f, t_sim, eco);
    DrawBioCrystalNode(220.0f, 660.0f, t_sim, eco);
    DrawBioCrystalNode(1120.0f, 240.0f, t_sim, eco);
    DrawBioCrystalNode(1160.0f, 620.0f, t_sim, eco);

    // 2. Alberi procedurali della Valle del Crati (solo in coordinate decorative sicure)
    DrawProceduralTree(136.0f, 152.0f, 1.05f, t_sim, eco, 1);
    DrawProceduralTree(515.0f, 148.0f, 0.95f, t_sim, eco, 2);
    DrawProceduralTree(138.0f, 735.0f, 1.10f, t_sim, eco, 3);
    DrawProceduralTree(795.0f, 725.0f, 1.00f, t_sim, eco, 4);
    DrawProceduralTree(1065.0f, 145.0f, 1.05f, t_sim, eco, 5);
    DrawProceduralTree(1225.0f, 730.0f, 1.12f, t_sim, eco, 6);

    // 3. Cartelli diegetici compatti (al posto delle vecchie scritte di debug)
    DrawDiegeticSignpost(
        15.5f * eco::kTileSize,
        3.55f * eco::kTileSize,
        "DIGA DEL CRATI [IN]",
        SKYBLUE);

    DrawDiegeticSignpost(
        19.2f * eco::kTileSize,
        7.55f * eco::kTileSize,
        eco.architect_recruited ? "PONTE BUCCI [APERTO]" : "PONTE BUCCI [CROLLATO]",
        eco.architect_recruited ? pal::kAccentClean : pal::kAccentWarn);
}

void DrawStructureProceduralBackdropAndMap(
    const eco::Tilemap& tilemap,
    const eco::EcoSystem& ecosystem,
    eco::Vec2 camera_target,
    float sim_time) {
    const float poll = ecosystem.PollutionRatio();
    const int world_w = static_cast<int>(tilemap.WorldWidth());
    const int world_h = static_cast<int>(tilemap.WorldHeight());
    const float parallax_x = (camera_target.x - static_cast<float>(world_w) * 0.5f) * 0.18f;

    // 1. Sfondo in parallasse: grandi pilastri e arcate in cemento armato della Diga del Crati
    const Color arch_col = pal::WorldColor(Color{42, 38, 38, 255}, Color{24, 42, 62, 255}, poll);
    const Color pipe_col = pal::WorldColor(Color{56, 50, 46, 255}, Color{36, 62, 88, 255}, poll);
    const Color water_fall = pal::WorldColor(
        Color{82, 88, 62, 115},
        Color{55, 185, 235, 135},
        poll);

    for (int i = 0; i < 6; ++i) {
        const int ax = static_cast<int>(80.0f + static_cast<float>(i) * 230.0f - parallax_x);
        DrawRectangle(ax, 48, 52, world_h - 140, arch_col);
        DrawRectangleLines(ax, 48, 52, world_h - 140, pal::ShiftBrightness(arch_col, 14));

        // Condotte verticali e cascate interne tra i pilastri
        if (i % 2 == 1) {
            const int wx = ax + 88;
            DrawRectangle(wx - 4, 48, 36, world_h - 140, pipe_col);
            DrawRectangle(wx, 48, 28, world_h - 140, water_fall);
            for (int streak = 0; streak < 5; ++streak) {
                const float sy = 56.0f + std::fmod(sim_time * 130.0f + static_cast<float>(streak * 110 + i * 40),
                                                   static_cast<float>(world_h - 160));
                DrawLineEx(
                    {static_cast<float>(wx + 6 + (streak % 3) * 7), sy},
                    {static_cast<float>(wx + 6 + (streak % 3) * 7), sy + 32.0f},
                    2.0f,
                    pal::WorldColor(pal::kSickWaterFoam, pal::kHealthyWaterFoam, poll));
            }
        }
    }

    // Griglia blueprint attenuata della sala macchine
    for (int gx = 48; gx < world_w; gx += 96) {
        DrawLine(gx, 48, gx, world_h - 96, Color{48, 82, 114, 45});
    }
    for (int gy = 48; gy < world_h - 48; gy += 96) {
        DrawLine(48, gy, world_w - 48, gy, Color{48, 82, 114, 45});
    }

    // Bacino d'acqua di scarico sul fondo della diga (sotto il pavimento)
    const Color basin_water = pal::WorldColor(pal::kSickWater, pal::kHealthyWater, poll);
    DrawRectangle(48, world_h - 102, world_w - 96, 16, pal::WithAlpha(basin_water, 165));

    // 2. Tilemap 2.5D della Diga (pareti in cemento e passerelle metalliche bullonate)
    const int tile_px = static_cast<int>(tilemap.TileSize());
    for (int r = 0; r < tilemap.Rows(); ++r) {
        const int row_cols = tilemap.RowCols(r);
        for (int c = 0; c < row_cols; ++c) {
            const eco::Rect tr = tilemap.TileRect(r, c);
            const int rx = static_cast<int>(tr.x);
            const int ry = static_cast<int>(tr.y);
            const eco::TileId id = tilemap.GetTile(r, c);

            if (id == eco::TileId::Wall) {
                const bool is_platform = (r >= 5 && r <= 11 && c > 0 && c < row_cols - 1);
                if (is_platform) {
                    // Passerella metallica a traliccio con bulloni e striscia antiscivolo
                    DrawRectangle(rx + 2, ry + tile_px, tile_px - 2, 7, Color{0, 0, 0, 90});
                    const Color girder_top = pal::WorldColor(Color{92, 84, 76, 255}, Color{72, 108, 138, 255}, poll);
                    const Color girder_body = pal::WorldColor(Color{58, 52, 48, 255}, Color{44, 68, 92, 255}, poll);
                    const Color rim_col = pal::WorldColor(Color{170, 140, 95, 255}, Color{95, 220, 250, 255}, poll);

                    DrawRectangle(rx, ry, tile_px, 12, girder_top);
                    DrawRectangle(rx, ry + 12, tile_px, tile_px - 12, girder_body);
                    // Diagonali del traliccio interno
                    DrawLine(rx + 3, ry + 14, rx + tile_px - 3, ry + tile_px - 3, pal::ShiftBrightness(girder_body, 18));
                    DrawLine(rx + tile_px - 3, ry + 14, rx + 3, ry + tile_px - 3, pal::ShiftBrightness(girder_body, 18));
                    DrawRectangleLines(rx, ry, tile_px, tile_px, rim_col);
                    DrawLineEx({tr.x + 2.0f, tr.y + 3.0f}, {tr.x + tr.width - 2.0f, tr.y + 3.0f}, 2.0f, rim_col);
                    // Bulloni agli angoli
                    DrawCircle(rx + 5, ry + 7, 1.8f, RAYWHITE);
                    DrawCircle(rx + tile_px - 5, ry + 7, 1.8f, RAYWHITE);
                } else {
                    // Muro/pavimento strutturale in cemento armato
                    const Color wall_top = pal::WorldColor(Color{68, 62, 58, 255}, Color{54, 72, 92, 255}, poll);
                    const Color wall_dark = pal::ShiftBrightness(wall_top, -16);
                    DrawRectangle(rx, ry, tile_px, tile_px, wall_dark);
                    DrawRectangle(rx + 2, ry + 2, tile_px - 4, tile_px - 10, wall_top);
                    DrawRectangleLines(rx, ry, tile_px, tile_px, pal::ShiftBrightness(wall_top, 20));
                    if (r == tilemap.Rows() - 2) {
                        // Bordo superiore del pavimento calpestabile
                        DrawLineEx({tr.x, tr.y + 1.0f}, {tr.x + tr.width, tr.y + 1.0f}, 2.0f, Color{110, 175, 215, 200});
                    }
                }
            } else if (id == eco::TileId::ExitPortal) {
                // Portellone stagno d'uscita verso la Valle
                DrawRectangle(rx, ry, tile_px, tile_px, Color{28, 86, 118, 255});
                DrawRectangle(rx + 5, ry + 5, tile_px - 10, tile_px - 5, Color{42, 138, 178, 255});
                DrawRectangleLinesEx(Rectangle{tr.x + 2.0f, tr.y + 2.0f, tr.width - 4.0f, tr.height - 4.0f}, 2.0f, GOLD);
                DrawTextShadow("USCITA", rx + 4, ry + 18, 11, GOLD);
            }
        }
    }

    // Cartelli diegetici compatti nella Diga
    DrawDiegeticSignpost(2.5f * eco::kTileSize, 11.4f * eco::kTileSize, "VALLE [USCITA]", GOLD);
    DrawDiegeticSignpost(11.3f * eco::kTileSize, 10.4f * eco::kTileSize, "ISPEZIONE [SPAZIO: SALTA]", SKYBLUE);
}

} // namespace eco::app
