#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace eco {

// Window & Fixed-Timestep Constants
inline constexpr int kWindowWidth = 1280;
inline constexpr int kWindowHeight = 720;
inline constexpr int kTargetFps = 60;

inline constexpr float kFixedDt = 1.0f / 60.0f;
inline constexpr float kMaxFrameDelta = 0.25f;

// Tile & Player Dimensions
inline constexpr float kTileSize = 48.0f;
inline constexpr float kPlayerWidth = 30.0f;
inline constexpr float kPlayerHeight = 30.0f;

// Movement & Side-Scrolling Physics Constants
inline constexpr float kTopDownSpeed = 220.0f;
inline constexpr float kSideMoveSpeed = 210.0f;
inline constexpr float kGravityAccel = 1200.0f;
inline constexpr float kJumpForce = 480.0f;
inline constexpr float kMaxFallSpeed = 900.0f;
inline constexpr float kMaxVerticalSubStep = 8.0f;
inline constexpr float kCoyoteTimeDuration = 0.15f;
inline constexpr float kJumpBufferDuration = 0.12f;

// Input & Multiplayer Constants
inline constexpr float kGamepadDeadzone = 0.2f;
inline constexpr int kMaxPlayers = 4;

// Camera Constants
inline constexpr float kCameraLerpSpeed = 6.0f;

// Math Constants
inline constexpr float kPi = 3.14159265358979323846f;
inline constexpr float kEpsilon = 1e-6f;

// EcoSystem Constants
inline constexpr float kInitialPollution = 35.0f;
inline constexpr float kMinEcoValue = 0.0f;
inline constexpr float kMaxEcoValue = 100.0f;

// Authoritative Interface Contract Enums (La Conca di Meridia)
enum class TileId : int {
    Walkable = 0,
    Wall = 1,
    StructurePortal = 2,
    ExitPortal = 3,
    HiddenBridge = 4,
    Water = 5,
    PavedPath = 6,
    SanctuaryFloor = 7,
    RuinsWall = 8,
    GardenFloor = 9
};

enum class GameMode { Overworld, Structure };

enum class ZoneId : int {
    DawnTerraces = 0,      // Terrazze dell'Alba (SO)
    CanalQuarter = 1,      // Quartiere dei Canali & Ponte degli Archi (Centro-Ovest)
    WindPlateau = 2,       // Altopiano dei Venti (NO)
    HangingGardens = 3,    // Giardini Sospesi (Centro-Est)
    FumaroleCaldera = 4,   // Caldera delle Fumarole (NE)
    HarmonySanctuary = 5,  // Santuario dell'Armonia (SE)
    Count = 6
};

enum class StructureId : int {
    HypogeanCisterns = 0,  // Cisterne Ipogee
    PendulumTower = 1,     // Torre del Pendolo
    GeothermalChamber = 2, // Camera Geotermica
    Count = 3
};

enum class LocationId : int {
    Overworld = 0,
    HypogeanCisterns = 1,
    PendulumTower = 2,
    GeothermalChamber = 3
};

enum class MachineType : int {
    SolarArray = 0,        // Eliostato Solare (Legge di Lambert)
    HydroTurbine = 1,      // Turbina Idrostatica (Legge di Stevino)
    PendulumRegulator = 2, // Pendolo Isochronos (Isocronismo di Galileo)
    HarmonicResonator = 3, // Risonatore Armonico (Curva di Risonanza)
    ThermalExchanger = 4,  // Scambiatore di Carnot (Termodinamica)
    CentralAstrolabe = 5,  // Astrolabio Centrale (Sintesi Armonica)
    Count = 6
};

enum class MachineState { Broken, CleanActive, BypassedDirty };
enum class UiPhase { Phase1_Minimal = 1, Phase2_Telemetry = 2, Phase3_EngineOverlay = 3 };

[[nodiscard]] constexpr LocationId LocationFromStructure(StructureId id) noexcept {
    switch (id) {
        case StructureId::HypogeanCisterns:
            return LocationId::HypogeanCisterns;
        case StructureId::PendulumTower:
            return LocationId::PendulumTower;
        case StructureId::GeothermalChamber:
            return LocationId::GeothermalChamber;
        case StructureId::Count:
            break;
    }
    return LocationId::HypogeanCisterns;
}

[[nodiscard]] constexpr StructureId StructureFromLocation(LocationId loc) noexcept {
    switch (loc) {
        case LocationId::HypogeanCisterns:
            return StructureId::HypogeanCisterns;
        case LocationId::PendulumTower:
            return StructureId::PendulumTower;
        case LocationId::GeothermalChamber:
            return StructureId::GeothermalChamber;
        case LocationId::Overworld:
            break;
    }
    return StructureId::HypogeanCisterns;
}

[[nodiscard]] constexpr const char* ZoneName(ZoneId zone) noexcept {
    switch (zone) {
        case ZoneId::DawnTerraces:
            return "Terrazze dell'Alba";
        case ZoneId::CanalQuarter:
            return "Quartiere dei Canali";
        case ZoneId::WindPlateau:
            return "Altopiano dei Venti";
        case ZoneId::HangingGardens:
            return "Giardini Sospesi";
        case ZoneId::FumaroleCaldera:
            return "Caldera delle Fumarole";
        case ZoneId::HarmonySanctuary:
            return "Santuario dell'Armonia";
        case ZoneId::Count:
            break;
    }
    return "Conca di Meridia";
}

[[nodiscard]] constexpr const char* ZoneSubtitle(ZoneId zone) noexcept {
    switch (zone) {
        case ZoneId::DawnTerraces:
            return "Meridiane di travertino e luce nascente";
        case ZoneId::CanalQuarter:
            return "Acquedotti antichi e Ponte degli Archi";
        case ZoneId::WindPlateau:
            return "Creste di basalto e torri del tempo";
        case ZoneId::HangingGardens:
            return "Terrazze fiorite e risonanze di giada";
        case ZoneId::FumaroleCaldera:
            return "Rovine delle fornaci e vene termali";
        case ZoneId::HarmonySanctuary:
            return "Mosaico astronomico e Astrolabio Centrale";
        case ZoneId::Count:
            break;
    }
    return "Altopiano dei Meridiani";
}

[[nodiscard]] constexpr const char* StructureName(StructureId id) noexcept {
    switch (id) {
        case StructureId::HypogeanCisterns:
            return "Cisterne Ipogee";
        case StructureId::PendulumTower:
            return "Torre del Pendolo";
        case StructureId::GeothermalChamber:
            return "Camera Geotermica";
        case StructureId::Count:
            break;
    }
    return "Struttura di Meridia";
}

[[nodiscard]] constexpr const char* StructureSubtitle(StructureId id) noexcept {
    switch (id) {
        case StructureId::HypogeanCisterns:
            return "Volte idrauliche e bacino di Stevino";
        case StructureId::PendulumTower:
            return "Oculi solari e quadrante isocrono";
        case StructureId::GeothermalChamber:
            return "Colonne di basalto e scambiatore di Carnot";
        case StructureId::Count:
            break;
    }
    return "Architettura dei Meridiani";
}

[[nodiscard]] constexpr const char* LocationName(LocationId loc, ZoneId zone) noexcept {
    if (loc == LocationId::Overworld) {
        return ZoneName(zone);
    }
    return StructureName(StructureFromLocation(loc));
}

[[nodiscard]] constexpr const char* UiPhaseName(UiPhase phase) noexcept {
    switch (phase) {
        case UiPhase::Phase1_Minimal:
            return "Sguardo Empirico";
        case UiPhase::Phase2_Telemetry:
            return "Rilievo Metrico";
        case UiPhase::Phase3_EngineOverlay:
            return "Trama Geodetica";
    }
    return "Sguardo Empirico";
}

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    [[nodiscard]] constexpr Vec2 operator+(const Vec2& rhs) const noexcept {
        return {x + rhs.x, y + rhs.y};
    }

    [[nodiscard]] constexpr Vec2 operator-(const Vec2& rhs) const noexcept {
        return {x - rhs.x, y - rhs.y};
    }

    [[nodiscard]] constexpr Vec2 operator*(float scalar) const noexcept {
        return {x * scalar, y * scalar};
    }

    [[nodiscard]] constexpr Vec2 operator/(float scalar) const noexcept {
        return {x / scalar, y / scalar};
    }

    constexpr Vec2& operator+=(const Vec2& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    constexpr Vec2& operator-=(const Vec2& rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    constexpr Vec2& operator*=(float scalar) noexcept {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    [[nodiscard]] constexpr bool operator==(const Vec2& rhs) const noexcept = default;

    [[nodiscard]] constexpr float LengthSquared() const noexcept {
        return x * x + y * y;
    }

    [[nodiscard]] inline float Length() const noexcept {
        return std::sqrt(LengthSquared());
    }

    [[nodiscard]] inline Vec2 Normalized() const noexcept {
        const float len = Length();
        if (!std::isfinite(len) || len <= kEpsilon) {
            return {0.0f, 0.0f};
        }
        return {x / len, y / len};
    }

    [[nodiscard]] constexpr float Dot(const Vec2& other) const noexcept {
        return x * other.x + y * other.y;
    }

    [[nodiscard]] inline float DistanceTo(const Vec2& other) const noexcept {
        return (*this - other).Length();
    }
};

[[nodiscard]] constexpr Vec2 operator*(float scalar, const Vec2& vec) noexcept {
    return vec * scalar;
}

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    [[nodiscard]] constexpr float Left() const noexcept { return x; }
    [[nodiscard]] constexpr float Right() const noexcept { return x + width; }
    [[nodiscard]] constexpr float Top() const noexcept { return y; }
    [[nodiscard]] constexpr float Bottom() const noexcept { return y + height; }
    [[nodiscard]] constexpr Vec2 Center() const noexcept {
        return {x + width * 0.5f, y + height * 0.5f};
    }
    [[nodiscard]] constexpr Vec2 Size() const noexcept { return {width, height}; }

    [[nodiscard]] constexpr bool Overlaps(const Rect& other) const noexcept {
        return Left() < other.Right() && Right() > other.Left() &&
               Top() < other.Bottom() && Bottom() > other.Top();
    }

    [[nodiscard]] constexpr bool operator==(const Rect& rhs) const noexcept = default;
};

using AABB = Rect;

struct ColorRgba {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;

    [[nodiscard]] constexpr bool operator==(const ColorRgba&) const noexcept = default;
};

[[nodiscard]] constexpr float deg2rad(float degrees) noexcept {
    return degrees * (kPi / 180.0f);
}

[[nodiscard]] constexpr float rad2deg(float radians) noexcept {
    return radians * (180.0f / kPi);
}

[[nodiscard]] constexpr float Clamp(float value, float min_val, float max_val) noexcept {
    if (min_val > max_val) {
        const float tmp = min_val;
        min_val = max_val;
        max_val = tmp;
    }
    if (!(value >= min_val)) {
        return min_val;
    }
    if (value > max_val) {
        return max_val;
    }
    return value;
}

[[nodiscard]] constexpr float Lerp(float a, float b, float t) noexcept {
    return a + (b - a) * t;
}

[[nodiscard]] constexpr Vec2 LerpVec2(Vec2 a, Vec2 b, float t) noexcept {
    return {Lerp(a.x, b.x, t), Lerp(a.y, b.y, t)};
}

[[nodiscard]] inline ColorRgba LerpColor(ColorRgba a, ColorRgba b, float t) noexcept {
    const float clamped_t = Clamp(t, 0.0f, 1.0f);
    auto lerp_channel = [clamped_t](std::uint8_t c0, std::uint8_t c1) noexcept -> std::uint8_t {
        const float v = std::round(Lerp(static_cast<float>(c0), static_cast<float>(c1), clamped_t));
        return static_cast<std::uint8_t>(Clamp(v, 0.0f, 255.0f));
    };
    return {
        lerp_channel(a.r, b.r),
        lerp_channel(a.g, b.g),
        lerp_channel(a.b, b.b),
        lerp_channel(a.a, b.a)
    };
}

struct FixedTimestepStepper {
    float fixed_dt = kFixedDt;
    float max_frame_delta = kMaxFrameDelta;
    float accumulator = 0.0f;

    [[nodiscard]] inline int Accumulate(float raw_frame_dt) noexcept {
        if (!std::isfinite(fixed_dt) || fixed_dt <= 0.0f ||
            !std::isfinite(max_frame_delta) || max_frame_delta <= 0.0f) {
            return 0;
        }
        if (!std::isfinite(raw_frame_dt) || raw_frame_dt <= 0.0f) {
            raw_frame_dt = 0.0f;
        }
        if (raw_frame_dt > max_frame_delta) {
            raw_frame_dt = max_frame_delta;
        }
        if (!std::isfinite(accumulator) || accumulator < 0.0f) {
            accumulator = 0.0f;
        }
        constexpr int kMaxSteps = 4096;
        const float max_safe_acc = fixed_dt * static_cast<float>(kMaxSteps);
        if (std::isfinite(max_safe_acc) && raw_frame_dt > max_safe_acc) {
            raw_frame_dt = max_safe_acc;
        }
        accumulator += raw_frame_dt;
        if (!std::isfinite(accumulator) ||
            (std::isfinite(max_safe_acc) && accumulator > max_safe_acc)) {
            accumulator = max_safe_acc;
        }
        int steps = 0;
        while (accumulator >= fixed_dt && steps < kMaxSteps) {
            const float next_acc = accumulator - fixed_dt;
            if (next_acc >= accumulator) {
                accumulator = 0.0f;
                break;
            }
            accumulator = next_acc;
            ++steps;
        }
        return steps;
    }

    [[nodiscard]] inline float Alpha() const noexcept {
        if (!std::isfinite(fixed_dt) || fixed_dt <= 0.0f) {
            return 0.0f;
        }
        return Clamp(accumulator / fixed_dt, 0.0f, 1.0f);
    }

    inline void Reset() noexcept {
        accumulator = 0.0f;
    }
};

} // namespace eco
