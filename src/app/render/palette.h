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
    Color atmosphere_tint;
};

// 0. Terrazze dell'Alba (SO — Era I: travertino dorato caldo, ulivi, meridiane solari)
inline constexpr DistrictPalette kPalDawnTerraces{
    .ground_sick = {104, 82, 54, 255},
    .ground_healthy = {86, 126, 62, 255},
    .grass_sick = {126, 102, 64, 255},
    .grass_healthy = {128, 174, 82, 255},
    .garden_sick = {98, 78, 50, 255},
    .garden_healthy = {76, 138, 68, 255},
    .path_sick = {128, 108, 82, 255},
    .path_healthy = {206, 182, 136, 255},
    .sanctuary_sick = {136, 118, 92, 255},
    .sanctuary_healthy = {224, 200, 152, 255},
    .inlay_sick = {142, 108, 66, 225},
    .inlay_healthy = {244, 194, 88, 245},
    .wall_top_sick = {122, 102, 78, 255},
    .wall_top_healthy = {202, 178, 134, 255},
    .wall_front_sick = {70, 56, 42, 255},
    .wall_front_healthy = {114, 94, 68, 255},
    .wall_edge_sick = {158, 136, 104, 255},
    .wall_edge_healthy = {242, 222, 176, 255},
    .water_sick = {76, 76, 54, 255},
    .water_healthy = {40, 146, 176, 255},
    .foam_sick = {142, 132, 98, 185},
    .foam_healthy = {202, 244, 246, 220},
    .canopy_sick = {104, 88, 60, 255},
    .canopy_healthy = {92, 146, 74, 255},
    .canopy_hi_sick = {132, 112, 78, 255},
    .canopy_hi_healthy = {158, 198, 112, 255},
    .blossom_a = {255, 222, 104, 245},
    .blossom_b = {252, 240, 196, 240},
    .ambient_accent = {252, 202, 86, 255},
    .atmosphere_tint = {255, 204, 112, 255},
};

// 1. Quartiere dei Canali & Ponte degli Archi (Centro-Ovest — Era II: blu-pietra, acqua zaffiro, bronzo)
inline constexpr DistrictPalette kPalCanalQuarter{
    .ground_sick = {62, 68, 66, 255},
    .ground_healthy = {36, 98, 94, 255},
    .grass_sick = {80, 88, 82, 255},
    .grass_healthy = {60, 148, 132, 255},
    .garden_sick = {58, 66, 64, 255},
    .garden_healthy = {34, 118, 108, 255},
    .path_sick = {88, 94, 98, 255},
    .path_healthy = {146, 168, 184, 255},
    .sanctuary_sick = {96, 104, 110, 255},
    .sanctuary_healthy = {166, 188, 204, 255},
    .inlay_sick = {86, 112, 120, 215},
    .inlay_healthy = {76, 218, 244, 245},
    .wall_top_sick = {82, 88, 94, 255},
    .wall_top_healthy = {128, 156, 178, 255},
    .wall_front_sick = {44, 50, 58, 255},
    .wall_front_healthy = {50, 78, 104, 255},
    .wall_edge_sick = {112, 120, 128, 255},
    .wall_edge_healthy = {182, 214, 236, 255},
    .water_sick = {52, 66, 58, 255},
    .water_healthy = {16, 124, 192, 255},
    .foam_sick = {112, 124, 108, 190},
    .foam_healthy = {168, 244, 255, 235},
    .canopy_sick = {72, 76, 70, 255},
    .canopy_healthy = {36, 134, 112, 255},
    .canopy_hi_sick = {94, 98, 90, 255},
    .canopy_hi_healthy = {82, 194, 168, 255},
    .blossom_a = {128, 238, 255, 240},
    .blossom_b = {236, 244, 252, 235},
    .ambient_accent = {78, 212, 248, 255},
    .atmosphere_tint = {84, 192, 244, 255},
};

// 2. Altopiano dei Venti (NO — Era II: ardesia fredda, roccia ventosa, cipressi d'altura)
inline constexpr DistrictPalette kPalWindPlateau{
    .ground_sick = {66, 68, 74, 255},
    .ground_healthy = {62, 92, 104, 255},
    .grass_sick = {86, 88, 94, 255},
    .grass_healthy = {96, 142, 152, 255},
    .garden_sick = {64, 68, 72, 255},
    .garden_healthy = {58, 108, 116, 255},
    .path_sick = {86, 88, 96, 255},
    .path_healthy = {134, 148, 166, 255},
    .sanctuary_sick = {96, 98, 106, 255},
    .sanctuary_healthy = {152, 168, 188, 255},
    .inlay_sick = {104, 108, 118, 215},
    .inlay_healthy = {196, 228, 248, 240},
    .wall_top_sick = {76, 78, 86, 255},
    .wall_top_healthy = {104, 122, 146, 255},
    .wall_front_sick = {40, 42, 52, 255},
    .wall_front_healthy = {44, 58, 80, 255},
    .wall_edge_sick = {112, 116, 126, 255},
    .wall_edge_healthy = {168, 196, 224, 255},
    .water_sick = {50, 60, 68, 255},
    .water_healthy = {34, 118, 166, 255},
    .foam_sick = {114, 122, 128, 185},
    .foam_healthy = {196, 238, 255, 225},
    .canopy_sick = {66, 70, 72, 255},
    .canopy_healthy = {38, 104, 98, 255},
    .canopy_hi_sick = {90, 94, 96, 255},
    .canopy_hi_healthy = {82, 158, 152, 255},
    .blossom_a = {222, 242, 255, 240},
    .blossom_b = {172, 218, 248, 235},
    .ambient_accent = {168, 222, 252, 255},
    .atmosphere_tint = {150, 192, 238, 255},
};

// 3. Giardini Sospesi (Centro-Est — Era II: smeraldo rigoglioso, vasche, fioriture vivaci)
inline constexpr DistrictPalette kPalHangingGardens{
    .ground_sick = {74, 68, 48, 255},
    .ground_healthy = {28, 128, 70, 255},
    .grass_sick = {94, 86, 56, 255},
    .grass_healthy = {58, 184, 96, 255},
    .garden_sick = {70, 66, 44, 255},
    .garden_healthy = {34, 152, 82, 255},
    .path_sick = {100, 92, 76, 255},
    .path_healthy = {172, 184, 156, 255},
    .sanctuary_sick = {108, 102, 84, 255},
    .sanctuary_healthy = {190, 202, 174, 255},
    .inlay_sick = {98, 110, 80, 215},
    .inlay_healthy = {64, 244, 172, 245},
    .wall_top_sick = {90, 86, 74, 255},
    .wall_top_healthy = {136, 166, 138, 255},
    .wall_front_sick = {50, 46, 40, 255},
    .wall_front_healthy = {56, 86, 68, 255},
    .wall_edge_sick = {122, 118, 98, 255},
    .wall_edge_healthy = {192, 224, 186, 255},
    .water_sick = {58, 72, 48, 255},
    .water_healthy = {24, 158, 164, 255},
    .foam_sick = {120, 130, 94, 185},
    .foam_healthy = {164, 252, 236, 230},
    .canopy_sick = {82, 76, 52, 255},
    .canopy_healthy = {32, 168, 86, 255},
    .canopy_hi_sick = {108, 98, 66, 255},
    .canopy_hi_healthy = {92, 230, 128, 255},
    .blossom_a = {255, 132, 166, 245},
    .blossom_b = {255, 228, 92, 245},
    .ambient_accent = {66, 242, 168, 255},
    .atmosphere_tint = {76, 228, 146, 255},
};

// 4. Caldera delle Fumarole (NE — Era III: basalto scuro vulcanico, rame, brace geotermica)
inline constexpr DistrictPalette kPalFumaroleCaldera{
    .ground_sick = {44, 34, 34, 255},
    .ground_healthy = {48, 52, 60, 255},
    .grass_sick = {66, 46, 40, 255},
    .grass_healthy = {84, 108, 76, 255},
    .garden_sick = {50, 38, 36, 255},
    .garden_healthy = {64, 92, 70, 255},
    .path_sick = {64, 52, 50, 255},
    .path_healthy = {92, 86, 86, 255},
    .sanctuary_sick = {74, 58, 54, 255},
    .sanctuary_healthy = {108, 98, 94, 255},
    .inlay_sick = {224, 78, 32, 240},
    .inlay_healthy = {244, 152, 62, 245},
    .wall_top_sick = {56, 44, 44, 255},
    .wall_top_healthy = {74, 76, 86, 255},
    .wall_front_sick = {28, 22, 24, 255},
    .wall_front_healthy = {34, 36, 46, 255},
    .wall_edge_sick = {112, 74, 58, 255},
    .wall_edge_healthy = {142, 126, 118, 255},
    .water_sick = {92, 48, 34, 255},
    .water_healthy = {46, 114, 134, 255},
    .foam_sick = {196, 108, 66, 195},
    .foam_healthy = {192, 226, 234, 215},
    .canopy_sick = {64, 48, 42, 255},
    .canopy_healthy = {74, 108, 72, 255},
    .canopy_hi_sick = {88, 64, 52, 255},
    .canopy_hi_healthy = {118, 152, 98, 255},
    .blossom_a = {252, 156, 68, 240},
    .blossom_b = {244, 212, 122, 235},
    .ambient_accent = {248, 132, 56, 255},
    .atmosphere_tint = {228, 104, 48, 255},
};

// 5. Santuario dell'Armonia (SE — Era IV: marmo avorio luminoso, turchese-giada e bronzo)
inline constexpr DistrictPalette kPalHarmonySanctuary{
    .ground_sick = {88, 82, 74, 255},
    .ground_healthy = {88, 136, 118, 255},
    .grass_sick = {106, 98, 86, 255},
    .grass_healthy = {120, 184, 156, 255},
    .garden_sick = {86, 80, 70, 255},
    .garden_healthy = {74, 154, 128, 255},
    .path_sick = {124, 116, 106, 255},
    .path_healthy = {210, 206, 192, 255},
    .sanctuary_sick = {136, 128, 116, 255},
    .sanctuary_healthy = {232, 228, 212, 255},
    .inlay_sick = {144, 124, 88, 225},
    .inlay_healthy = {92, 248, 222, 250},
    .wall_top_sick = {118, 110, 102, 255},
    .wall_top_healthy = {216, 212, 198, 255},
    .wall_front_sick = {64, 58, 54, 255},
    .wall_front_healthy = {98, 116, 122, 255},
    .wall_edge_sick = {156, 146, 134, 255},
    .wall_edge_healthy = {246, 242, 228, 255},
    .water_sick = {68, 78, 68, 255},
    .water_healthy = {36, 168, 204, 255},
    .foam_sick = {142, 144, 128, 190},
    .foam_healthy = {196, 254, 255, 240},
    .canopy_sick = {96, 90, 78, 255},
    .canopy_healthy = {64, 168, 136, 255},
    .canopy_hi_sick = {122, 114, 98, 255},
    .canopy_hi_healthy = {142, 236, 206, 255},
    .blossom_a = {112, 252, 228, 245},
    .blossom_b = {255, 236, 152, 245},
    .ambient_accent = {92, 248, 222, 255},
    .atmosphere_tint = {120, 244, 224, 255},
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
