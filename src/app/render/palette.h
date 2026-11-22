#pragma once

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

// Palette "Valle Ferita" vs "Valle Guarita" (proprietario: artista).
// Tutte le transizioni cromatiche dipendono da EcoSystem::PollutionRatio() in [0..1]
// (0 = valle guarita e luminosa, 1 = valle ferita dallo smog).
namespace eco::app::pal {

// --- Sfondo / Cielo ---
inline constexpr Color kSickSky{48, 38, 32, 255};
inline constexpr Color kHealthySky{18, 42, 58, 255};

inline constexpr Color kSickDamBg{32, 28, 28, 255};
inline constexpr Color kHealthyDamBg{14, 24, 38, 255};

// --- Terreno Campus Ovest (c < 20) ---
inline constexpr Color kSickGroundWest{86, 72, 50, 255};
inline constexpr Color kHealthyGroundWest{44, 112, 66, 255};

inline constexpr Color kSickGrassAccent{102, 86, 56, 255};
inline constexpr Color kHealthyGrassAccent{68, 158, 86, 255};

// --- Terreno Giardino Est (c > 20, oltre il Ponte Bucci) ---
inline constexpr Color kSickGroundEast{78, 66, 54, 255};
inline constexpr Color kHealthyGroundEast{38, 122, 88, 255};

// --- Sentiero Lastricato ---
inline constexpr Color kSickPath{96, 84, 68, 255};
inline constexpr Color kHealthyPath{128, 136, 126, 255};

// --- Rovine / Muri 2.5D ---
inline constexpr Color kSickWallTop{94, 84, 76, 255};
inline constexpr Color kHealthyWallTop{118, 138, 144, 255};

inline constexpr Color kSickWallFront{52, 44, 40, 255};
inline constexpr Color kHealthyWallFront{54, 70, 82, 255};

inline constexpr Color kSickWallEdge{122, 106, 92, 255};
inline constexpr Color kHealthyWallEdge{156, 190, 198, 255};

// --- Acqua del Canale del Crati & Diga ---
inline constexpr Color kSickWater{72, 78, 52, 255};
inline constexpr Color kHealthyWater{28, 142, 192, 255};

inline constexpr Color kSickWaterFoam{130, 126, 92, 190};
inline constexpr Color kHealthyWaterFoam{140, 235, 255, 220};

// --- Flora / Alberi ---
inline constexpr Color kSickTrunk{64, 50, 42, 255};
inline constexpr Color kHealthyTrunk{92, 64, 44, 255};

inline constexpr Color kSickCanopy{88, 76, 58, 255};
inline constexpr Color kHealthyCanopy{46, 152, 88, 255};

inline constexpr Color kSickCanopyHighlight{108, 92, 68, 255};
inline constexpr Color kHealthyCanopyHighlight{88, 204, 122, 255};

// --- Accenti UI e Macchine ---
inline constexpr Color kUiPanelBg{10, 18, 28, 232};
inline constexpr Color kUiHeaderBg{18, 34, 52, 245};
inline constexpr Color kUiBorderCyan{64, 185, 235, 225};
inline constexpr Color kAccentClean{64, 240, 185, 255};
inline constexpr Color kAccentDirty{240, 130, 45, 255};
inline constexpr Color kAccentWarn{255, 195, 70, 255};

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
// Usiamo una curva esponenziale morbida (std::pow(pollution_ratio, 0.58f)) cosi'
// già a smog default (35%) la valle appare visibilmente arida/ocra, rendendo
// drammatico e gratificante il passaggio a 0% smog (verde brillante).
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

// Colori distintivi per i 1-4 giocatori (P0 Ada + fino a 3 colleghi in co-op locale)
[[nodiscard]] inline Color PlayerAccentColor(int player_id) noexcept {
    switch (player_id & 3) {
        case 0:
            return Color{55, 220, 245, 255}; // P0 Ada: Ciano scientifico
        case 1:
            return Color{255, 185, 55, 255}; // P1: Ambra solare
        case 2:
            return Color{80, 235, 145, 255}; // P2: Verde smeraldo
        default:
            return Color{245, 105, 165, 255}; // P3: Corallo
    }
}

} // namespace eco::app::pal
