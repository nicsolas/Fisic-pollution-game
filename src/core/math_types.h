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

// Authoritative Interface Contract Enums
enum class TileId : int {
    Walkable = 0,
    Wall = 1,
    StructurePortal = 2,
    ExitPortal = 3,
    HiddenBridge = 4
};

enum class GameMode { Overworld, Structure };
enum class MachineType { SolarArray, HydroTurbine };
enum class MachineState { Broken, CleanActive, BypassedDirty };
enum class UiPhase { Phase1_Minimal = 1, Phase2_Telemetry = 2, Phase3_EngineOverlay = 3 };

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
