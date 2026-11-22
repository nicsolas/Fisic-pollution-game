#pragma once

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "core/math_types.h"

// Palette "La Conca di Meridia" — Ferita (Inverno di Pece) vs Guarita (Alba di Meridia)
// Proprietario: @artista-1
// Tutte le transizioni cromatiche dipendono da EcoSystem::PollutionRatio() in [0..1]
// (0.0 = Conca guarita e luminosa, 1.0 = Conca velata dalla pece e dalla cenere).
namespace eco::app::pal {

// =============================================================================
// Token Materici Condivisi della Civilta' dei Meridiani (Architettura & UI)
// =============================================================================
inline constexpr Color kStoneIvory{218, 210, 192, 255};
inline constexpr Color kTravertineLight{186, 174, 148, 255};
inline constexpr Color kTravertineShadow{116, 108, 92, 255};
inline constexpr Color kBasaltDark{34, 38, 46, 255};
inline constexpr Color kBasaltMid{64, 72, 84, 255};
inline constexpr Color kBronze{184, 138, 72, 255};
inline constexpr Color kBronzeBright{232, 192, 114, 255};
inline constexpr Color kBronzeDark{104, 74, 38, 255};
inline constexpr Color kCopperOxidized{74, 148, 134, 255};
inline constexpr Color kJade{56, 212, 174, 255};
inline constexpr Color kJadeGlow{122, 248, 216, 255};
inline constexpr Color kAmberGlow{244, 178, 72, 255};
inline constexpr Color kPitchDark{38, 30, 28, 255};
inline constexpr Color kParchmentText{236, 228, 208, 255};
inline constexpr Color kInkShadow{12, 18, 26, 225};

// =============================================================================
// Colori Sfondo Cielo / Strutture & Accenti UI (compatibili con tutta la UI)
// =============================================================================
inline constexpr Color kSickSky{44, 36, 32, 255};
inline constexpr Color kHealthySky{18, 40, 56, 255};

inline constexpr Color kSickDamBg{28, 24, 24, 255};
inline constexpr Color kHealthyDamBg{14, 26, 40, 255};

// Alias compatibilita' Overworld (usati come default / fallback)
inline constexpr Color kSickGroundWest{86, 72, 54, 255};
inline constexpr Color kHealthyGroundWest{54, 108, 68, 255};

inline constexpr Color kSickGrassAccent{104, 88, 62, 255};
inline constexpr Color kHealthyGrassAccent{82, 156, 94, 255};

inline constexpr Color kSickGroundEast{80, 68, 54, 255};
inline constexpr Color kHealthyGroundEast{40, 114, 82, 255};

inline constexpr Color kSickPath{104, 92, 78, 255};
inline constexpr Color kHealthyPath{162, 152, 134, 255};

inline constexpr Color kSickWallTop{96, 86, 78, 255};
inline constexpr Color kHealthyWallTop{148, 150, 140, 255};

inline constexpr Color kSickWallFront{52, 44, 42, 255};
inline constexpr Color kHealthyWallFront{66, 78, 86, 255};

inline constexpr Color kSickWallEdge{128, 114, 98, 255};
inline constexpr Color kHealthyWallEdge{192, 198, 186, 255};

inline constexpr Color kSickWater{64, 70, 52, 255};
inline constexpr Color kHealthyWater{26, 132, 178, 255};

inline constexpr Color kSickWaterFoam{124, 120, 92, 185};
inline constexpr Color kHealthyWaterFoam{152, 238, 252, 225};

inline constexpr Color kSickTrunk{64, 50, 42, 255};
inline constexpr Color kHealthyTrunk{92, 66, 46, 255};

inline constexpr Color kSickCanopy{88, 76, 58, 255};
inline constexpr Color kHealthyCanopy{48, 146, 88, 255};

inline constexpr Color kSickCanopyHighlight{108, 94, 70, 255};
inline constexpr Color kHealthyCanopyHighlight{94, 196, 122, 255};

// --- Accenti UI, Pannelli e Stato Macchine ---
inline constexpr Color kUiPanelBg{14, 22, 32, 236};
inline constexpr Color kUiHeaderBg{22, 36, 50, 246};
inline constexpr Color kUiBorderCyan{88, 188, 212, 228};
inline constexpr Color kUiBorderBronze{194, 152, 86, 225};
inline constexpr Color kAccentClean{64, 232, 184, 255};
inline constexpr Color kAccentDirty{232, 124, 52, 255};
inline constexpr Color kAccentWarn{248, 194, 76, 255};

// =============================================================================
// Palette per Distretto dell'Overworld (La Conca di Meridia — 6 Distretti)
// =============================================================================
struct DistrictPalette {
    Color ground_sick;
    Color ground_healthy;
    Color grass_sick;
    Color grass_healthy;
    Color garden_sick;
    Color garden_healthy;
    Color path_sick;
    Color path_healthy;
    Color sanctuary_sick;
    Color sanctuary_healthy;
    Color inlay_sick;
    Color inlay_healthy;
    Color wall_top_sick;
    Color wall_top_healthy;
    Color wall_front_sick;
    Color wall_front_healthy;
    Color wall_edge_sick;
    Color wall_edge_healthy;
    Color water_sick;
    Color water_healthy;
    Color foam_sick;
    Color foam_healthy;
    Color canopy_sick;
    Color canopy_healthy;
    Color canopy_hi_sick;
    Color canopy_hi_healthy;
    Color blossom_a;
    Color blossom_b;
    Color ambient_accent;
};

// 0. Terrazze dell'Alba (SO — Era I: travertino solare, ulivi, meridiane)
inline constexpr DistrictPalette kPalDawnTerraces{
    .ground_sick = {90, 74, 54, 255},
    .ground_healthy = {58, 110, 66, 255},
    .grass_sick = {112, 92, 64, 255},
    .grass_healthy = {92, 156, 86, 255},
    .garden_sick = {86, 72, 50, 255},
    .garden_healthy = {54, 126, 72, 255},
    .path_sick = {114, 98, 80, 255},
    .path_healthy = {174, 158, 132, 255},
    .sanctuary_sick = {120, 106, 88, 255},
    .sanctuary_healthy = {194, 178, 148, 255},
    .inlay_sick = {124, 98, 68, 220},
    .inlay_healthy = {224, 180, 96, 240},
    .wall_top_sick = {108, 94, 78, 255},
    .wall_top_healthy = {176, 160, 134, 255},
    .wall_front_sick = {60, 50, 42, 255},
    .wall_front_healthy = {94, 84, 72, 255},
    .wall_edge_sick = {140, 124, 102, 255},
    .wall_edge_healthy = {218, 204, 176, 255},
    .water_sick = {70, 74, 54, 255},
    .water_healthy = {34, 138, 176, 255},
    .foam_sick = {132, 124, 96, 185},
    .foam_healthy = {180, 240, 248, 220},
    .canopy_sick = {92, 82, 62, 255},
    .canopy_healthy = {70, 134, 78, 255},
    .canopy_hi_sick = {114, 102, 78, 255},
    .canopy_hi_healthy = {128, 184, 114, 255},
    .blossom_a = {252, 218, 112, 240},
    .blossom_b = {246, 236, 198, 235},
    .ambient_accent = {244, 196, 98, 255},
};

// 1. Quartiere dei Canali & Ponte degli Archi (Centro-Ovest — Era II: acqua, pietra calcarea, bronzo)
inline constexpr DistrictPalette kPalCanalQuarter{
    .ground_sick = {74, 70, 58, 255},
    .ground_healthy = {44, 104, 80, 255},
    .grass_sick = {92, 88, 70, 255},
    .grass_healthy = {72, 152, 114, 255},
    .garden_sick = {70, 68, 54, 255},
    .garden_healthy = {42, 122, 92, 255},
    .path_sick = {98, 92, 84, 255},
    .path_healthy = {152, 160, 156, 255},
    .sanctuary_sick = {106, 102, 94, 255},
    .sanctuary_healthy = {168, 178, 176, 255},
    .inlay_sick = {96, 108, 102, 210},
    .inlay_healthy = {86, 212, 218, 235},
    .wall_top_sick = {92, 88, 84, 255},
    .wall_top_healthy = {144, 158, 164, 255},
    .wall_front_sick = {50, 48, 48, 255},
    .wall_front_healthy = {62, 82, 96, 255},
    .wall_edge_sick = {122, 118, 112, 255},
    .wall_edge_healthy = {186, 208, 216, 255},
    .water_sick = {60, 68, 52, 255},
    .water_healthy = {24, 128, 178, 255},
    .foam_sick = {118, 120, 96, 185},
    .foam_healthy = {156, 240, 255, 225},
    .canopy_sick = {82, 76, 62, 255},
    .canopy_healthy = {44, 138, 96, 255},
    .canopy_hi_sick = {104, 96, 78, 255},
    .canopy_hi_healthy = {88, 194, 144, 255},
    .blossom_a = {138, 238, 248, 235},
    .blossom_b = {244, 236, 176, 230},
    .ambient_accent = {86, 208, 238, 255},
};

// 2. Altopiano dei Venti (NO — Era II: basalto-ardesia, cipressi d'altura, rose dei venti)
inline constexpr DistrictPalette kPalWindPlateau{
    .ground_sick = {74, 72, 66, 255},
    .ground_healthy = {52, 102, 86, 255},
    .grass_sick = {94, 90, 82, 255},
    .grass_healthy = {88, 148, 124, 255},
    .garden_sick = {72, 72, 64, 255},
    .garden_healthy = {52, 116, 98, 255},
    .path_sick = {92, 90, 88, 255},
    .path_healthy = {136, 148, 154, 255},
    .sanctuary_sick = {100, 98, 96, 255},
    .sanctuary_healthy = {154, 168, 176, 255},
    .inlay_sick = {106, 104, 98, 210},
    .inlay_healthy = {188, 218, 232, 235},
    .wall_top_sick = {84, 82, 82, 255},
    .wall_top_healthy = {116, 132, 146, 255},
    .wall_front_sick = {46, 46, 50, 255},
    .wall_front_healthy = {54, 68, 84, 255},
    .wall_edge_sick = {116, 114, 114, 255},
    .wall_edge_healthy = {168, 192, 210, 255},
    .water_sick = {56, 64, 60, 255},
    .water_healthy = {36, 124, 164, 255},
    .foam_sick = {116, 120, 112, 185},
    .foam_healthy = {184, 234, 252, 220},
    .canopy_sick = {74, 74, 66, 255},
    .canopy_healthy = {36, 112, 86, 255},
    .canopy_hi_sick = {96, 96, 86, 255},
    .canopy_hi_healthy = {78, 162, 132, 255},
    .blossom_a = {216, 238, 252, 235},
    .blossom_b = {178, 224, 242, 230},
    .ambient_accent = {156, 212, 240, 255},
};

// 3. Giardini Sospesi (Centro-Est — Era II: smeraldo-giada, vasche, fioriture vivaci)
inline constexpr DistrictPalette kPalHangingGardens{
    .ground_sick = {82, 68, 52, 255},
    .ground_healthy = {38, 114, 76, 255},
    .grass_sick = {100, 84, 60, 255},
    .grass_healthy = {68, 168, 102, 255},
    .garden_sick = {78, 66, 48, 255},
    .garden_healthy = {44, 134, 86, 255},
    .path_sick = {104, 92, 78, 255},
    .path_healthy = {166, 164, 146, 255},
    .sanctuary_sick = {112, 100, 86, 255},
    .sanctuary_healthy = {182, 184, 166, 255},
    .inlay_sick = {102, 106, 84, 215},
    .inlay_healthy = {76, 232, 186, 240},
    .wall_top_sick = {96, 86, 76, 255},
    .wall_top_healthy = {154, 162, 146, 255},
    .wall_front_sick = {54, 46, 42, 255},
    .wall_front_healthy = {72, 86, 78, 255},
    .wall_edge_sick = {128, 116, 100, 255},
    .wall_edge_healthy = {196, 212, 188, 255},
    .water_sick = {64, 74, 52, 255},
    .water_healthy = {28, 148, 168, 255},
    .foam_sick = {124, 128, 96, 185},
    .foam_healthy = {158, 248, 238, 225},
    .canopy_sick = {88, 76, 56, 255},
    .canopy_healthy = {42, 156, 94, 255},
    .canopy_hi_sick = {112, 96, 70, 255},
    .canopy_hi_healthy = {96, 214, 138, 255},
    .blossom_a = {252, 152, 164, 240},
    .blossom_b = {254, 224, 106, 240},
    .ambient_accent = {78, 232, 178, 255},
};

// 4. Caldera delle Fumarole (NE — Era III: basalto scuro, rame, vene geotermiche)
inline constexpr DistrictPalette kPalFumaroleCaldera{
    .ground_sick = {56, 46, 44, 255},
    .ground_healthy = {58, 68, 74, 255},
    .grass_sick = {74, 58, 50, 255},
    .grass_healthy = {82, 118, 86, 255},
    .garden_sick = {60, 50, 44, 255},
    .garden_healthy = {68, 102, 76, 255},
    .path_sick = {76, 66, 62, 255},
    .path_healthy = {112, 110, 106, 255},
    .sanctuary_sick = {84, 72, 66, 255},
    .sanctuary_healthy = {128, 122, 112, 255},
    .inlay_sick = {184, 76, 38, 225},
    .inlay_healthy = {224, 158, 78, 235},
    .wall_top_sick = {68, 58, 56, 255},
    .wall_top_healthy = {92, 98, 106, 255},
    .wall_front_sick = {36, 30, 30, 255},
    .wall_front_healthy = {46, 52, 60, 255},
    .wall_edge_sick = {104, 82, 70, 255},
    .wall_edge_healthy = {152, 146, 138, 255},
    .water_sick = {82, 54, 42, 255},
    .water_healthy = {46, 124, 144, 255},
    .foam_sick = {168, 108, 76, 190},
    .foam_healthy = {186, 228, 236, 215},
    .canopy_sick = {72, 58, 52, 255},
    .canopy_healthy = {74, 118, 78, 255},
    .canopy_hi_sick = {94, 74, 62, 255},
    .canopy_hi_healthy = {118, 162, 108, 255},
    .blossom_a = {246, 174, 88, 235},
    .blossom_b = {236, 218, 146, 230},
    .ambient_accent = {236, 146, 74, 255},
};

// 5. Santuario dell'Armonia (SE — Era IV: marmo/travertino avorio, bronzo e giada)
inline constexpr DistrictPalette kPalHarmonySanctuary{
    .ground_sick = {82, 76, 68, 255},
    .ground_healthy = {74, 114, 96, 255},
    .grass_sick = {100, 92, 78, 255},
    .grass_healthy = {102, 164, 134, 255},
    .garden_sick = {82, 76, 64, 255},
    .garden_healthy = {64, 136, 108, 255},
    .path_sick = {112, 104, 94, 255},
    .path_healthy = {178, 174, 160, 255},
    .sanctuary_sick = {122, 114, 102, 255},
    .sanctuary_healthy = {202, 196, 178, 255},
    .inlay_sick = {132, 114, 84, 220},
    .inlay_healthy = {228, 192, 112, 245},
    .wall_top_sick = {108, 100, 92, 255},
    .wall_top_healthy = {184, 180, 166, 255},
    .wall_front_sick = {58, 52, 48, 255},
    .wall_front_healthy = {92, 98, 102, 255},
    .wall_edge_sick = {144, 134, 120, 255},
    .wall_edge_healthy = {230, 224, 206, 255},
    .water_sick = {66, 74, 62, 255},
    .water_healthy = {34, 152, 188, 255},
    .foam_sick = {134, 134, 116, 190},
    .foam_healthy = {178, 248, 252, 235},
    .canopy_sick = {90, 84, 70, 255},
    .canopy_healthy = {56, 152, 118, 255},
    .canopy_hi_sick = {114, 106, 88, 255},
    .canopy_hi_healthy = {126, 220, 186, 255},
    .blossom_a = {122, 248, 218, 240},
    .blossom_b = {252, 228, 138, 240},
    .ambient_accent = {106, 236, 206, 255},
};

[[nodiscard]] inline const DistrictPalette& DistrictPaletteByIndex(int index) noexcept {
    switch (index) {
        case 0: return kPalDawnTerraces;
        case 1: return kPalCanalQuarter;
        case 2: return kPalWindPlateau;
        case 3: return kPalHangingGardens;
        case 4: return kPalFumaroleCaldera;
        default: return kPalHarmonySanctuary;
    }
}

[[nodiscard]] inline const DistrictPalette& GetDistrictPalette(eco::ZoneId zone) noexcept {
    return DistrictPaletteByIndex(static_cast<int>(zone));
}

// =============================================================================
// Palette per le 3 Strutture Interne Side-Scrolling
// =============================================================================
struct StructurePalette {
    Color bg_sick;
    Color bg_healthy;
    Color arch_far_sick;
    Color arch_far_healthy;
    Color arch_near_sick;
    Color arch_near_healthy;
    Color wall_top_sick;
    Color wall_top_healthy;
    Color wall_dark_sick;
    Color wall_dark_healthy;
    Color plat_top_sick;
    Color plat_top_healthy;
    Color plat_body_sick;
    Color plat_body_healthy;
    Color plat_rim_sick;
    Color plat_rim_healthy;
    Color accent_sick;
    Color accent_healthy;
    Color liquid_sick;
    Color liquid_healthy;
};

// 0. Cisterne Ipogee (pietra calcarea umida, cascate, bronzo e giada)
inline constexpr StructurePalette kPalHypogeanCisterns{
    .bg_sick = {26, 26, 28, 255},
    .bg_healthy = {14, 28, 42, 255},
    .arch_far_sick = {38, 36, 36, 255},
    .arch_far_healthy = {22, 44, 64, 255},
    .arch_near_sick = {54, 50, 48, 255},
    .arch_near_healthy = {36, 66, 92, 255},
    .wall_top_sick = {74, 68, 64, 255},
    .wall_top_healthy = {70, 94, 112, 255},
    .wall_dark_sick = {44, 40, 38, 255},
    .wall_dark_healthy = {38, 56, 72, 255},
    .plat_top_sick = {104, 94, 82, 255},
    .plat_top_healthy = {102, 134, 152, 255},
    .plat_body_sick = {64, 58, 52, 255},
    .plat_body_healthy = {52, 78, 98, 255},
    .plat_rim_sick = {166, 138, 96, 255},
    .plat_rim_healthy = {104, 224, 236, 255},
    .accent_sick = {152, 126, 86, 220},
    .accent_healthy = {88, 228, 236, 240},
    .liquid_sick = {68, 76, 56, 185},
    .liquid_healthy = {36, 162, 212, 200},
};

// 1. Torre del Pendolo (pietra chiara d'osservatorio, ottone astronomico, luce zenitale)
inline constexpr StructurePalette kPalPendulumTower{
    .bg_sick = {32, 28, 26, 255},
    .bg_healthy = {20, 30, 44, 255},
    .arch_far_sick = {48, 42, 38, 255},
    .arch_far_healthy = {36, 48, 66, 255},
    .arch_near_sick = {66, 58, 50, 255},
    .arch_near_healthy = {58, 70, 86, 255},
    .wall_top_sick = {88, 78, 68, 255},
    .wall_top_healthy = {114, 112, 102, 255},
    .wall_dark_sick = {54, 46, 40, 255},
    .wall_dark_healthy = {64, 66, 68, 255},
    .plat_top_sick = {118, 102, 82, 255},
    .plat_top_healthy = {166, 146, 110, 255},
    .plat_body_sick = {74, 62, 50, 255},
    .plat_body_healthy = {96, 82, 62, 255},
    .plat_rim_sick = {172, 142, 92, 255},
    .plat_rim_healthy = {236, 198, 116, 255},
    .accent_sick = {176, 142, 84, 210},
    .accent_healthy = {246, 210, 122, 245},
    .liquid_sick = {98, 86, 64, 90},
    .liquid_healthy = {248, 224, 148, 115},
};

// 2. Camera Geotermica (basalto esagonale, rame ossidato, pozze termali e vapore)
inline constexpr StructurePalette kPalGeothermalChamber{
    .bg_sick = {30, 20, 18, 255},
    .bg_healthy = {22, 26, 34, 255},
    .arch_far_sick = {46, 30, 26, 255},
    .arch_far_healthy = {38, 42, 52, 255},
    .arch_near_sick = {64, 42, 36, 255},
    .arch_near_healthy = {58, 62, 72, 255},
    .wall_top_sick = {72, 54, 48, 255},
    .wall_top_healthy = {82, 86, 94, 255},
    .wall_dark_sick = {40, 30, 28, 255},
    .wall_dark_healthy = {44, 48, 56, 255},
    .plat_top_sick = {116, 78, 58, 255},
    .plat_top_healthy = {144, 108, 76, 255},
    .plat_body_sick = {70, 46, 36, 255},
    .plat_body_healthy = {84, 62, 48, 255},
    .plat_rim_sick = {216, 108, 56, 255},
    .plat_rim_healthy = {236, 174, 88, 255},
    .accent_sick = {228, 96, 44, 230},
    .accent_healthy = {242, 178, 84, 240},
    .liquid_sick = {194, 72, 34, 195},
    .liquid_healthy = {56, 168, 178, 185},
};

[[nodiscard]] inline const StructurePalette& StructurePaletteByIndex(int index) noexcept {
    switch (index) {
        case 0: return kPalHypogeanCisterns;
        case 1: return kPalPendulumTower;
        default: return kPalGeothermalChamber;
    }
}

[[nodiscard]] inline const StructurePalette& GetStructurePalette(eco::StructureId id) noexcept {
    return StructurePaletteByIndex(static_cast<int>(id));
}

// =============================================================================
// Funzioni di Interpolazione e Utility Cromatiche
// =============================================================================
[[nodiscard]] inline Color LerpColor(Color a, Color b, float t) noexcept {
    const float ct = std::clamp(t, 0.0f, 1.0f);
    return Color{
        static_cast<unsigned char>(static_cast<float>(a.r) + (static_cast<float>(b.r) - static_cast<float>(a.r)) * ct),
        static_cast<unsigned char>(static_cast<float>(a.g) + (static_cast<float>(b.g) - static_cast<float>(a.g)) * ct),
        static_cast<unsigned char>(static_cast<float>(a.b) + (static_cast<float>(b.b) - static_cast<float>(a.b)) * ct),
        static_cast<unsigned char>(static_cast<float>(a.a) + (static_cast<float>(b.a) - static_cast<float>(a.a)) * ct)
    };
}

// pollution_ratio: 0.0 = guarito (healthy), 1.0 = ferito (sick).
// Curva esponenziale morbida (pow 0.58) cosi' gia' al 35% di smog la Conca appare
// visibilmente velata e il risanamento verso lo 0% esplode in luce e colore.
[[nodiscard]] inline Color WorldColor(Color sick, Color healthy, float pollution_ratio) noexcept {
    const float shaped = std::pow(std::clamp(pollution_ratio, 0.0f, 1.0f), 0.58f);
    return LerpColor(healthy, sick, shaped);
}

[[nodiscard]] inline Color WithAlpha(Color c, unsigned char a) noexcept {
    return Color{c.r, c.g, c.b, a};
}

[[nodiscard]] inline Color ShiftBrightness(Color c, int delta) noexcept {
    auto clamp_u8 = [](int v) -> unsigned char {
        return static_cast<unsigned char>(std::clamp(v, 0, 255));
    };
    return Color{
        clamp_u8(static_cast<int>(c.r) + delta),
        clamp_u8(static_cast<int>(c.g) + delta),
        clamp_u8(static_cast<int>(c.b) + delta),
        c.a
    };
}

// Colori distintivi per 1-4 esploratori (Elena Valli + fino a 3 colleghi in co-op locale)
[[nodiscard]] inline Color PlayerAccentColor(int player_id) noexcept {
    switch (player_id & 3) {
        case 0:
            return Color{64, 218, 238, 255}; // P0 Elena: Ciano astronomico
        case 1:
            return Color{248, 188, 72, 255}; // P1: Ambra solare
        case 2:
            return Color{76, 228, 152, 255}; // P2: Giada smeraldo
        default:
            return Color{242, 118, 156, 255}; // P3: Corallo
    }
}

} // namespace eco::app::pal
