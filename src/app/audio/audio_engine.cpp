#include "app/audio/audio_engine.h"

#include <raylib.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "core/math_types.h"

namespace eco::app {

namespace {

constexpr int kSampleRate = 44100;
constexpr float kTwoPi = 2.0f * eco::kPi;

AudioEngine* g_audio_engine = nullptr;

// Generatore pseudo-casuale deterministico veloce per rumore acustico morbido
struct FastRng {
    std::uint32_t state = 0x5EED1234u;

    [[nodiscard]] float NextSigned() noexcept {
        state = state * 1664525u + 1013904223u;
        const float u01 = static_cast<float>((state >> 8) & 0x00FFFFFFu) / 16777215.0f;
        return u01 * 2.0f - 1.0f;
    }
};

[[nodiscard]] float SmoothEnvelope(float t, float duration, float attack, float release) noexcept {
    if (duration <= 0.0f || t < 0.0f || t > duration) {
        return 0.0f;
    }
    float env = 1.0f;
    if (attack > 0.0f && t < attack) {
        env = t / attack;
    }
    if (release > 0.0f && t > duration - release) {
        env = std::min(env, (duration - t) / release);
    }
    return std::clamp(env, 0.0f, 1.0f);
}

[[nodiscard]] Sound BuildSoundFromSamples(const std::vector<float>& samples) {
    const int frame_count = static_cast<int>(samples.size());
    if (frame_count <= 0) {
        return Sound{};
    }

    auto* pcm = static_cast<std::int16_t*>(
        RL_MALLOC(static_cast<std::size_t>(frame_count) * sizeof(std::int16_t)));
    if (pcm == nullptr) {
        return Sound{};
    }

    for (int i = 0; i < frame_count; ++i) {
        const float clamped = std::clamp(samples[static_cast<std::size_t>(i)], -0.98f, 0.98f);
        pcm[i] = static_cast<std::int16_t>(std::lround(clamped * 32767.0f));
    }

    Wave wave{};
    wave.frameCount = static_cast<unsigned int>(frame_count);
    wave.sampleRate = static_cast<unsigned int>(kSampleRate);
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = pcm;

    Sound snd = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return snd;
}

[[nodiscard]] std::vector<float> SynthFootstepStone() {
    constexpr float kDur = 0.085f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    FastRng rng{0x101u};
    float lp = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float decay = std::exp(-t * 48.0f);
        const float body = std::sin(kTwoPi * (165.0f - 55.0f * (t / kDur)) * t) * 0.42f;
        const float raw_noise = rng.NextSigned();
        lp += 0.28f * (raw_noise - lp);
        const float click = (raw_noise - lp) * 0.35f * std::exp(-t * 90.0f);
        const float env = SmoothEnvelope(t, kDur, 0.003f, 0.020f);
        out[static_cast<std::size_t>(i)] = (body + lp * 0.25f + click) * decay * env * 0.42f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthFootstepGrass() {
    constexpr float kDur = 0.095f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    FastRng rng{0x202u};
    float lp = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float raw = rng.NextSigned();
        lp += 0.14f * (raw - lp);
        const float rustle = (raw - lp) * 0.32f;
        const float thump = std::sin(kTwoPi * 96.0f * t) * 0.24f * std::exp(-t * 36.0f);
        const float env = SmoothEnvelope(t, kDur, 0.008f, 0.035f);
        out[static_cast<std::size_t>(i)] = (lp * 0.22f + rustle + thump) * env * 0.38f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthJump() {
    constexpr float kDur = 0.145f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    FastRng rng{0x303u};
    float phase = 0.0f;
    float lp = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float u = t / kDur;
        const float freq = 175.0f + 225.0f * std::pow(u, 0.72f);
        phase += kTwoPi * freq / static_cast<float>(kSampleRate);
        lp += 0.09f * (rng.NextSigned() - lp);
        const float env = SmoothEnvelope(t, kDur, 0.008f, 0.055f);
        out[static_cast<std::size_t>(i)] = (std::sin(phase) * 0.45f + lp * 0.22f) * env * 0.46f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthLand() {
    constexpr float kDur = 0.12f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    FastRng rng{0x404u};
    float phase = 0.0f;
    float lp = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float freq = 118.0f * std::exp(-t * 9.0f) + 46.0f;
        phase += kTwoPi * freq / static_cast<float>(kSampleRate);
        lp += 0.18f * (rng.NextSigned() - lp);
        const float env = SmoothEnvelope(t, kDur, 0.004f, 0.055f);
        out[static_cast<std::size_t>(i)] =
            (std::sin(phase) * 0.62f + lp * 0.30f) * std::exp(-t * 16.0f) * env * 0.56f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthScanPulse() {
    constexpr float kDur = 0.48f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.012f, 0.18f);
        const float shimmer = 1.0f + 0.06f * std::sin(kTwoPi * 11.0f * t);
        const float s1 = std::sin(kTwoPi * 587.33f * t) * std::exp(-t * 5.2f) * 0.46f;
        const float s2 = std::sin(kTwoPi * 880.00f * t) * std::exp(-t * 6.8f) * 0.28f;
        const float s3 = std::sin(kTwoPi * 1174.66f * t) * std::exp(-t * 9.5f) * 0.18f;
        out[static_cast<std::size_t>(i)] = (s1 + s2 + s3) * shimmer * env * 0.52f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthPortalEnter() {
    constexpr float kDur = 0.50f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    FastRng rng{0x505u};
    float lp = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.045f, 0.22f);
        lp += 0.055f * (rng.NextSigned() - lp);
        const float drone =
            std::sin(kTwoPi * 146.83f * t) * 0.42f +
            std::sin(kTwoPi * 220.00f * t) * 0.30f +
            std::sin(kTwoPi * 440.00f * t) * 0.14f * std::exp(-t * 3.5f);
        out[static_cast<std::size_t>(i)] = (drone * 0.65f + lp * 0.55f) * env * 0.48f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthRelicCollect() {
    constexpr float kDur = 0.72f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    constexpr std::array<float, 3> kFreqs = {587.33f, 739.99f, 1174.66f};
    constexpr std::array<float, 3> kStarts = {0.0f, 0.095f, 0.195f};
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float sum = 0.0f;
        for (std::size_t k = 0; k < kFreqs.size(); ++k) {
            const float dt = t - kStarts[k];
            if (dt >= 0.0f) {
                const float f = kFreqs[k];
                const float atk = std::min(1.0f, dt / 0.008f);
                const float dec = std::exp(-dt * 5.2f);
                const float bell =
                    std::sin(kTwoPi * f * dt) * 0.65f +
                    std::sin(kTwoPi * f * 2.0f * dt) * 0.22f * std::exp(-dt * 9.0f) +
                    std::sin(kTwoPi * f * 3.0f * dt) * 0.09f * std::exp(-dt * 14.0f);
                sum += bell * atk * dec * 0.42f;
            }
        }
        const float tail_env = SmoothEnvelope(t, kDur, 0.004f, 0.10f);
        out[static_cast<std::size_t>(i)] = sum * tail_env * 0.58f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthNpcInteract() {
    constexpr float kDur = 0.19f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.006f, 0.09f);
        const float note1 = std::sin(kTwoPi * 392.0f * t) * std::exp(-t * 14.0f) * 0.52f;
        const float dt2 = std::max(0.0f, t - 0.045f);
        const float note2 =
            (t >= 0.045f) ? (std::sin(kTwoPi * 523.25f * dt2) * std::exp(-dt2 * 16.0f) * 0.36f) : 0.0f;
        out[static_cast<std::size_t>(i)] = (note1 + note2) * env * 0.44f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthModalOpen() {
    constexpr float kDur = 0.13f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.005f, 0.055f);
        const float a = std::sin(kTwoPi * 329.63f * t) * std::exp(-t * 18.0f) * 0.45f;
        const float b = (t >= 0.032f)
                            ? (std::sin(kTwoPi * 493.88f * (t - 0.032f)) * std::exp(-(t - 0.032f) * 20.0f) * 0.42f)
                            : 0.0f;
        out[static_cast<std::size_t>(i)] = (a + b) * env * 0.42f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthModalClose() {
    constexpr float kDur = 0.115f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.005f, 0.05f);
        const float a = std::sin(kTwoPi * 440.0f * t) * std::exp(-t * 22.0f) * 0.42f;
        const float b = (t >= 0.028f)
                            ? (std::sin(kTwoPi * 293.66f * (t - 0.028f)) * std::exp(-(t - 0.028f) * 22.0f) * 0.40f)
                            : 0.0f;
        out[static_cast<std::size_t>(i)] = (a + b) * env * 0.38f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthSliderTick() {
    constexpr float kDur = 0.028f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.002f, 0.014f);
        const float tick = std::sin(kTwoPi * 940.0f * t) * 0.6f + std::sin(kTwoPi * 1410.0f * t) * 0.25f;
        out[static_cast<std::size_t>(i)] = tick * std::exp(-t * 95.0f) * env * 0.26f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthCalibrateClean() {
    constexpr float kDur = 1.05f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    constexpr std::array<float, 5> kNotes = {293.66f, 440.00f, 554.37f, 659.25f, 739.99f};
    constexpr std::array<float, 5> kDelays = {0.0f, 0.055f, 0.11f, 0.165f, 0.22f};
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float sum = 0.0f;
        for (std::size_t k = 0; k < kNotes.size(); ++k) {
            const float dt = t - kDelays[k];
            if (dt >= 0.0f) {
                const float f = kNotes[k];
                const float atk = std::min(1.0f, dt / 0.015f);
                const float dec = std::exp(-dt * 2.8f);
                const float voice =
                    std::sin(kTwoPi * f * dt) * 0.72f +
                    std::sin(kTwoPi * f * 2.0f * dt) * 0.18f * std::exp(-dt * 5.0f);
                sum += voice * atk * dec * 0.26f;
            }
        }
        const float env = SmoothEnvelope(t, kDur, 0.008f, 0.22f);
        out[static_cast<std::size_t>(i)] = sum * env * 0.62f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthCalibrateBypass() {
    constexpr float kDur = 0.75f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    FastRng rng{0x606u};
    float lp = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.025f, 0.28f);
        lp += 0.045f * (rng.NextSigned() - lp);
        const float rumble =
            std::sin(kTwoPi * 73.42f * t) * 0.50f +
            std::sin(kTwoPi * 109.0f * t) * 0.26f +
            std::sin(kTwoPi * 155.56f * t) * 0.16f;
        out[static_cast<std::size_t>(i)] =
            (rumble * 0.55f + lp * 0.95f) * std::exp(-t * 1.9f) * env * 0.58f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthBridgeUnlock() {
    constexpr float kDur = 1.15f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    constexpr std::array<float, 4> kNotes = {146.83f, 220.00f, 293.66f, 587.33f};
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.04f, 0.35f);
        float sum = 0.0f;
        for (std::size_t k = 0; k < kNotes.size(); ++k) {
            const float f = kNotes[k];
            sum += std::sin(kTwoPi * f * t) * std::exp(-t * (1.8f + 0.5f * static_cast<float>(k))) * 0.30f;
        }
        out[static_cast<std::size_t>(i)] = sum * env * 0.58f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthZoneDiscover() {
    constexpr float kDur = 0.42f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.04f, 0.22f);
        const float tone =
            std::sin(kTwoPi * 440.0f * t) * 0.42f +
            std::sin(kTwoPi * 659.25f * t) * 0.28f;
        out[static_cast<std::size_t>(i)] = tone * std::exp(-t * 3.8f) * env * 0.30f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthPhaseShift() {
    constexpr float kDur = 0.22f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    float phase = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float u = t / kDur;
        const float freq = 493.88f + 246.11f * u;
        phase += kTwoPi * freq / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.008f, 0.095f);
        out[static_cast<std::size_t>(i)] =
            (std::sin(phase) * 0.55f + std::sin(phase * 1.5f) * 0.22f) * env * 0.36f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthUiClick() {
    constexpr float kDur = 0.036f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.002f, 0.018f);
        const float s = std::sin(kTwoPi * 640.0f * t) * 0.6f + std::sin(kTwoPi * 960.0f * t) * 0.22f;
        out[static_cast<std::size_t>(i)] = s * std::exp(-t * 82.0f) * env * 0.28f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthEpilogueChord() {
    constexpr float kDur = 1.85f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    constexpr std::array<float, 6> kNotes = {146.83f, 220.00f, 293.66f, 369.99f, 440.00f, 587.33f};
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.14f, 0.65f);
        float sum = 0.0f;
        for (std::size_t k = 0; k < kNotes.size(); ++k) {
            const float f = kNotes[k];
            const float shimmer = 1.0f + 0.04f * std::sin(kTwoPi * (1.5f + 0.3f * static_cast<float>(k)) * t);
            sum += std::sin(kTwoPi * f * t) * shimmer * 0.22f;
        }
        out[static_cast<std::size_t>(i)] = sum * env * 0.56f;
    }
    return out;
}

// --- Nuovi SFX procedurali per le 6 meccaniche top-down e gli intramezzi ---

[[nodiscard]] std::vector<float> SynthSolarMirrorRotate() {
    // Scatto di ottone + breve bagliore cristallino
    constexpr float kDur = 0.22f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    FastRng rng{0x8101u};
    float lp = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.004f, 0.09f);
        lp += 0.22f * (rng.NextSigned() - lp);
        const float clack = (std::sin(kTwoPi * 260.0f * t) * 0.5f + lp * 0.3f) * std::exp(-t * 48.0f);
        const float glint = (std::sin(kTwoPi * 1046.5f * t) * 0.32f +
                             std::sin(kTwoPi * 1567.98f * t) * 0.18f) * std::exp(-t * 14.0f);
        out[static_cast<std::size_t>(i)] = (clack + glint) * env * 0.52f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthSolarPlateUnlock() {
    // Accordo solare radioso (E5 -> B5 -> E6)
    constexpr float kDur = 0.85f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    constexpr std::array<float, 4> kNotes = {523.25f, 659.25f, 783.99f, 1046.50f};
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.02f, 0.28f);
        float sum = 0.0f;
        for (std::size_t k = 0; k < kNotes.size(); ++k) {
            const float dt = t - static_cast<float>(k) * 0.055f;
            if (dt >= 0.0f) {
                sum += std::sin(kTwoPi * kNotes[k] * dt) * std::exp(-dt * 3.8f) * 0.28f;
            }
        }
        out[static_cast<std::size_t>(i)] = sum * env * 0.58f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthSluiceLeverToggle() {
    // Leva idraulica in pietra/bronzo + scroscio breve d'acqua
    constexpr float kDur = 0.42f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    FastRng rng{0x8202u};
    float lp = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.008f, 0.16f);
        lp += 0.08f * (rng.NextSigned() - lp);
        const float thud = std::sin(kTwoPi * (140.0f - 45.0f * (t / kDur)) * t) * std::exp(-t * 24.0f) * 0.55f;
        const float rush = lp * 0.85f * SmoothEnvelope(t, kDur, 0.04f, 0.18f);
        out[static_cast<std::size_t>(i)] = (thud + rush) * env * 0.56f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthWindGustWhoosh() {
    // Raffica di vento sull'Altopiano
    constexpr float kDur = 0.95f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    FastRng rng{0x8303u};
    float lp1 = 0.0f;
    float lp2 = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float u = t / kDur;
        const float swell = std::sin(eco::kPi * u);
        const float alpha = 0.015f + 0.055f * swell;
        lp1 += alpha * (rng.NextSigned() - lp1);
        lp2 += alpha * (lp1 - lp2);
        const float whistle = std::sin(kTwoPi * (260.0f + 130.0f * swell) * t) * 0.08f * swell;
        out[static_cast<std::size_t>(i)] = (lp2 * 1.35f + whistle) * swell * 0.55f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthResonancePadNote() {
    // Campana armonica di giada (440Hz base, trasposta via pitch per ogni piastra!)
    constexpr float kDur = 0.68f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.008f, 0.22f);
        const float tone =
            std::sin(kTwoPi * 440.0f * t) * 0.64f * std::exp(-t * 4.2f) +
            std::sin(kTwoPi * 880.0f * t) * 0.22f * std::exp(-t * 7.5f) +
            std::sin(kTwoPi * 1320.0f * t) * 0.10f * std::exp(-t * 11.0f);
        out[static_cast<std::size_t>(i)] = tone * env * 0.58f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthResonanceSequenceFail() {
    // Breve tono smorzato discendente quando la sequenza si azzera
    constexpr float kDur = 0.30f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    float phase = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float freq = 280.0f - 95.0f * (t / kDur);
        phase += kTwoPi * freq / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.01f, 0.12f);
        out[static_cast<std::size_t>(i)] =
            (std::sin(phase) * 0.55f + std::sin(phase * 1.41f) * 0.20f) * env * 0.42f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthJadeBridgeUnlock() {
    // Risveglio del ponte di giada (arpeggio cristallino luminoso)
    constexpr float kDur = 1.10f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    constexpr std::array<float, 4> kNotes = {392.00f, 493.88f, 587.33f, 783.99f};
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.02f, 0.32f);
        float sum = 0.0f;
        for (std::size_t k = 0; k < kNotes.size(); ++k) {
            const float dt = t - static_cast<float>(k) * 0.07f;
            if (dt >= 0.0f) {
                sum += (std::sin(kTwoPi * kNotes[k] * dt) * 0.65f +
                        std::sin(kTwoPi * kNotes[k] * 2.0f * dt) * 0.20f * std::exp(-dt * 6.0f)) *
                       std::exp(-dt * 2.6f) * 0.32f;
            }
        }
        out[static_cast<std::size_t>(i)] = sum * env * 0.58f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthGeyserErupt() {
    // Getto di vapore pressurizzato + rombo geotermico
    constexpr float kDur = 0.65f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    FastRng rng{0x8404u};
    float lp = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.025f, 0.25f);
        const float raw = rng.NextSigned();
        lp += 0.12f * (raw - lp);
        const float hiss = (raw - lp) * 0.45f;
        const float rumble = std::sin(kTwoPi * 72.0f * t) * 0.38f + lp * 0.55f;
        out[static_cast<std::size_t>(i)] = (hiss + rumble) * env * 0.54f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthValveClose() {
    // Rotazione valvola metallica + sfiato decrescente
    constexpr float kDur = 0.38f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    FastRng rng{0x8505u};
    float lp = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.008f, 0.14f);
        lp += 0.18f * (rng.NextSigned() - lp);
        const float clang = (std::sin(kTwoPi * 310.0f * t) * 0.45f +
                             std::sin(kTwoPi * 465.0f * t) * 0.28f) * std::exp(-t * 16.0f);
        const float vent = (rng.NextSigned() - lp) * 0.25f * std::exp(-t * 6.0f);
        out[static_cast<std::size_t>(i)] = (clang + vent) * env * 0.52f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthSanctuarySealTune() {
    // Risonanza sacra dei 5 sigilli dell'Astrolabio
    constexpr float kDur = 0.90f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.025f, 0.34f);
        const float voice =
            std::sin(kTwoPi * 293.66f * t) * 0.35f +
            std::sin(kTwoPi * 440.00f * t) * 0.32f +
            std::sin(kTwoPi * 587.33f * t) * 0.26f +
            std::sin(kTwoPi * 880.00f * t) * 0.14f * std::exp(-t * 3.5f);
        out[static_cast<std::size_t>(i)] = voice * std::exp(-t * 2.2f) * env * 0.56f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthInterludeChime() {
    // Accordo morbido di passaggio soglia tra zone
    constexpr float kDur = 0.95f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.06f, 0.42f);
        const float pad =
            std::sin(kTwoPi * 293.66f * t) * 0.34f +
            std::sin(kTwoPi * 440.00f * t) * 0.26f +
            std::sin(kTwoPi * 659.25f * t) * 0.20f;
        out[static_cast<std::size_t>(i)] = pad * std::exp(-t * 1.9f) * env * 0.44f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthTrialPulse() {
    // Impulso di prova sperimentale (RunTrial): scatto d'ottone + sweep diagnostico limpido
    constexpr float kDur = 0.56f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    FastRng rng{0x9101u};
    float phase = 0.0f;
    float lp = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float u = t / kDur;
        const float env = SmoothEnvelope(t, kDur, 0.006f, 0.20f);
        const float freq = 330.0f + 330.0f * std::sin(u * eco::kPi * 0.85f);
        phase += kTwoPi * freq / static_cast<float>(kSampleRate);
        lp += 0.16f * (rng.NextSigned() - lp);
        const float transient = (std::sin(kTwoPi * 220.0f * t) * 0.45f + lp * 0.30f) * std::exp(-t * 36.0f);
        const float probe =
            std::sin(phase) * 0.46f +
            std::sin(phase * 1.5f) * 0.24f * std::exp(-t * 4.5f) +
            std::sin(kTwoPi * 987.77f * t) * 0.16f * std::exp(-t * 7.5f);
        out[static_cast<std::size_t>(i)] = (transient + probe) * env * 0.54f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthCalibrateRisky() {
    // Soluzione Rischiosa / Regime Supercritico: ronzio armonico teso + sfiato ad alta pressione
    constexpr float kDur = 0.92f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    FastRng rng{0x9202u};
    float lp = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.015f, 0.30f);
        const float raw = rng.NextSigned();
        lp += 0.10f * (raw - lp);
        const float buzz_mod = 0.78f + 0.22f * std::sin(kTwoPi * 26.0f * t);
        const float chord =
            std::sin(kTwoPi * 146.83f * t) * 0.36f +
            std::sin(kTwoPi * 220.00f * t) * 0.30f +
            std::sin(kTwoPi * 311.13f * t) * 0.22f +
            std::sin(kTwoPi * 622.25f * t) * 0.16f * std::exp(-t * 3.2f);
        const float hiss = (raw - lp) * 0.24f * std::exp(-t * 2.8f);
        out[static_cast<std::size_t>(i)] = (chord * buzz_mod + hiss) * std::exp(-t * 1.6f) * env * 0.58f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthObservationInspect() {
    // Lettura stazione diegetica (gnomone/idrometro/stevino): tocco metallico + nota chiara
    constexpr float kDur = 0.34f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.004f, 0.14f);
        const float click = std::sin(kTwoPi * 784.0f * t) * std::exp(-t * 42.0f) * 0.40f;
        const float chime =
            std::sin(kTwoPi * 523.25f * t) * 0.44f * std::exp(-t * 6.5f) +
            std::sin(kTwoPi * 783.99f * t) * 0.28f * std::exp(-t * 8.5f);
        out[static_cast<std::size_t>(i)] = (click + chime) * env * 0.48f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthBioPlanterBloom() {
    // Innesto Seme d'Ulivo Bianco: arpeggio caldo + fruscio di foglie vive
    constexpr float kDur = 0.96f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    FastRng rng{0x9404u};
    float lp = 0.0f;
    constexpr std::array<float, 4> kNotes = {329.63f, 440.00f, 554.37f, 659.25f};
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.018f, 0.32f);
        const float raw = rng.NextSigned();
        lp += 0.14f * (raw - lp);
        const float leaves = (raw - lp) * 0.18f * SmoothEnvelope(t, 0.48f, 0.03f, 0.22f);
        float sum = 0.0f;
        for (std::size_t k = 0; k < kNotes.size(); ++k) {
            const float dt = t - static_cast<float>(k) * 0.065f;
            if (dt >= 0.0f) {
                sum += std::sin(kTwoPi * kNotes[k] * dt) * std::exp(-dt * 3.1f) * 0.28f;
            }
        }
        out[static_cast<std::size_t>(i)] = (sum + leaves) * env * 0.58f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthPrismRemoteToggle() {
    // Impulso remoto del Prisma del Congedo: risonanza ottica cristallina
    constexpr float kDur = 0.52f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = SmoothEnvelope(t, kDur, 0.005f, 0.20f);
        const float shimmer = 1.0f + 0.12f * std::sin(kTwoPi * 16.0f * t);
        const float crystal =
            std::sin(kTwoPi * 659.25f * t) * 0.42f * std::exp(-t * 5.2f) +
            std::sin(kTwoPi * 987.77f * t) * 0.32f * std::exp(-t * 6.4f) +
            std::sin(kTwoPi * 1318.51f * t) * 0.22f * std::exp(-t * 8.5f);
        out[static_cast<std::size_t>(i)] = crystal * shimmer * env * 0.54f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthFaunaChirp() {
    // Breve cinguettio doppio cristallino di rondine / fauna di zona guarita
    constexpr float kDur = 0.28f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    float phase1 = 0.0f;
    float phase2 = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        float s = 0.0f;
        if (t < 0.11f) {
            const float u = t / 0.11f;
            const float f = 1760.0f + 520.0f * std::sin(u * eco::kPi);
            phase1 += kTwoPi * f / static_cast<float>(kSampleRate);
            s += std::sin(phase1) * SmoothEnvelope(t, 0.11f, 0.012f, 0.045f) * 0.34f;
        }
        if (t >= 0.13f) {
            const float dt = t - 0.13f;
            const float u = dt / 0.14f;
            const float f = 1975.5f + 640.0f * (1.0f - u * 0.45f);
            phase2 += kTwoPi * f / static_cast<float>(kSampleRate);
            s += std::sin(phase2) * SmoothEnvelope(dt, 0.14f, 0.012f, 0.06f) * 0.30f;
        }
        out[static_cast<std::size_t>(i)] = s;
    }
    return out;
}

// --- Tappeti MUSICALI (armonie tonali, controllati da music_volume_) ---
enum class MusicLayer : int {
    OverworldClean = 0,
    OverworldSmog,
    CriticalOverdrive,
    Cisterns,
    Tower,
    Geothermal,
    Count
};

// --- Tappeti AMBIENTALI / NATURA (aria, acqua, vento, vapore, controllati da ambient_volume_) ---
enum class NatureLayer : int {
    AirBreeze = 0,
    CanalWater,
    LaminarStream,
    PlateauWind,
    ThermalSteam,
    Count
};

[[nodiscard]] std::vector<float> SynthMusicLoop(MusicLayer layer) {
    constexpr float kDur = 4.0f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);

    auto snap = [](float hz) noexcept -> float {
        return std::round(hz * kDur) / kDur;
    };

    const float f_d2 = snap(73.42f);
    const float f_a2 = snap(110.00f);
    const float f_d3 = snap(146.83f);
    const float f_e3 = snap(164.81f);
    const float f_a3 = snap(220.00f);
    const float f_f4 = snap(369.99f);
    const float breath_hz = 0.25f;

    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float breath = 0.78f + 0.22f * std::sin(kTwoPi * breath_hz * t);
        float sample = 0.0f;

        switch (layer) {
            case MusicLayer::OverworldClean: {
                const float pad =
                    std::sin(kTwoPi * f_d3 * t) * 0.26f +
                    std::sin(kTwoPi * f_a3 * t) * 0.17f +
                    std::sin(kTwoPi * f_f4 * t) * 0.07f * (0.5f + 0.5f * std::sin(kTwoPi * 0.5f * t));
                sample = pad * breath * 0.28f;
                break;
            }
            case MusicLayer::OverworldSmog: {
                const float low_drone =
                    std::sin(kTwoPi * f_d2 * t) * 0.28f +
                    std::sin(kTwoPi * f_e3 * t) * 0.10f;
                sample = low_drone * breath * 0.26f;
                break;
            }
            case MusicLayer::CriticalOverdrive: {
                // Ronzio armonico teso di regime supercritico (battimento a 1.5 Hz)
                const float pulse_mod = 0.72f + 0.28f * std::sin(kTwoPi * 1.5f * t);
                const float overdrive =
                    std::sin(kTwoPi * f_a2 * t) * 0.24f +
                    std::sin(kTwoPi * snap(165.0f) * t) * 0.18f +
                    std::sin(kTwoPi * snap(311.25f) * t) * 0.08f;
                sample = overdrive * pulse_mod * 0.25f;
                break;
            }
            case MusicLayer::Cisterns: {
                const float cavern =
                    std::sin(kTwoPi * f_a2 * t) * 0.24f +
                    std::sin(kTwoPi * f_d3 * t) * 0.16f;
                sample = cavern * breath * 0.28f;
                break;
            }
            case MusicLayer::Tower: {
                const float hum =
                    std::sin(kTwoPi * f_d3 * t) * 0.20f +
                    std::sin(kTwoPi * f_a3 * t) * 0.12f;
                const float cycle_t = std::fmod(t, 2.0f);
                const float tick_env = std::exp(-cycle_t * 28.0f);
                const float pend_tick =
                    (std::sin(kTwoPi * 440.0f * cycle_t) * 0.18f +
                     std::sin(kTwoPi * 660.0f * cycle_t) * 0.08f) * tick_env;
                sample = (hum * breath + pend_tick) * 0.28f;
                break;
            }
            case MusicLayer::Geothermal: {
                const float magma =
                    std::sin(kTwoPi * f_d2 * t) * 0.32f +
                    std::sin(kTwoPi * f_a2 * t) * 0.18f;
                sample = magma * breath * 0.28f;
                break;
            }
            case MusicLayer::Count:
                break;
        }
        out[static_cast<std::size_t>(i)] = sample;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthNatureLoop(NatureLayer layer) {
    constexpr float kDur = 4.0f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    FastRng rng{0xB100u + static_cast<std::uint32_t>(layer) * 131u};

    std::vector<float> noise(static_cast<std::size_t>(n), 0.0f);
    float lp1 = 0.0f;
    float lp2 = 0.0f;
    const float alpha = (layer == NatureLayer::CanalWater) ? 0.042f
                        : (layer == NatureLayer::LaminarStream) ? 0.022f
                        : (layer == NatureLayer::PlateauWind) ? 0.028f
                        : (layer == NatureLayer::ThermalSteam) ? 0.055f
                                                               : 0.015f;
    for (int i = 0; i < n; ++i) {
        const float r = rng.NextSigned();
        lp1 += alpha * (r - lp1);
        lp2 += alpha * (lp1 - lp2);
        noise[static_cast<std::size_t>(i)] = lp2;
    }
    const int xf = static_cast<int>(0.35f * kSampleRate);
    for (int i = 0; i < xf; ++i) {
        const float w = static_cast<float>(i) / static_cast<float>(xf);
        const std::size_t tail_idx = static_cast<std::size_t>(n - xf + i);
        noise[tail_idx] = noise[tail_idx] * (1.0f - w) + noise[static_cast<std::size_t>(i)] * w;
    }

    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float nz = noise[static_cast<std::size_t>(i)];
        float sample = 0.0f;
        switch (layer) {
            case NatureLayer::AirBreeze: {
                const float mod = 0.72f + 0.28f * std::sin(kTwoPi * 0.25f * t);
                sample = nz * mod * 0.34f;
                break;
            }
            case NatureLayer::CanalWater: {
                const float ripple = 0.78f + 0.22f * std::sin(kTwoPi * 0.75f * t);
                sample = nz * ripple * 0.44f;
                break;
            }
            case NatureLayer::LaminarStream: {
                // Gorgoglio laminare limpido + lieve scintillio armonico di acqua risanata
                const float shimmer = 0.80f + 0.20f * std::sin(kTwoPi * 0.50f * t);
                const float bell =
                    std::sin(kTwoPi * 587.50f * t) * 0.028f * (0.5f + 0.5f * std::sin(kTwoPi * 0.25f * t)) +
                    std::sin(kTwoPi * 880.00f * t) * 0.018f * (0.5f + 0.5f * std::cos(kTwoPi * 0.50f * t));
                sample = nz * shimmer * 0.36f + bell;
                break;
            }
            case NatureLayer::PlateauWind: {
                const float gust = 0.68f + 0.32f * std::sin(kTwoPi * 0.50f * t);
                sample = nz * gust * 0.48f;
                break;
            }
            case NatureLayer::ThermalSteam: {
                const float pulse = 0.74f + 0.26f * std::sin(kTwoPi * 0.50f * t);
                sample = nz * pulse * 0.42f;
                break;
            }
            case NatureLayer::Count:
                break;
        }
        out[static_cast<std::size_t>(i)] = sample;
    }
    return out;
}

} // namespace

struct AudioEngine::Impl {
    std::array<Sound, static_cast<std::size_t>(SfxId::Count)> sfx{};
    std::array<Sound, static_cast<std::size_t>(MusicLayer::Count)> music{};
    std::array<float, static_cast<std::size_t>(MusicLayer::Count)> music_vol{};
    std::array<Sound, static_cast<std::size_t>(NatureLayer::Count)> nature{};
    std::array<float, static_cast<std::size_t>(NatureLayer::Count)> nature_vol{};

    // Stato osservato per attivare automaticamente i suoni di interazione
    bool has_prev = false;
    bool prev_modal_open = false;
    float prev_modal_value = 0.0f;
    float prev_stabilizer_value = 0.68f;
    float slider_tick_cooldown = 0.0f;
    bool prev_pause_open = false;
    bool prev_journal_open = false;
    int prev_journal_tab = 0;
    eco::GameMode prev_mode = eco::GameMode::Overworld;
    eco::StructureId prev_structure = eco::StructureId::HypogeanCisterns;
    eco::ZoneId prev_zone = eco::ZoneId::DawnTerraces;
    bool prev_interlude_active = false;
    eco::UiPhase prev_ui_phase = eco::UiPhase::Phase1_Minimal;
    bool prev_architect_recruited = false;
    bool prev_sim_completed = false;
    int prev_collected_count = 0;
    float prev_scan_timer = 0.0f;
    std::array<eco::MachineState, 6> prev_machine_states{};
    std::array<eco::CalibrationApproach, 6> prev_machine_approaches{};
    std::array<bool, 4> prev_npc_met{};
    std::array<bool, 4> prev_npc_recruited{};

    // Stato Vertical Slice (prove, osservazioni, innesti d'ulivo, prisma remoto, guado, cinguettii)
    int prev_total_trials = 0;
    int prev_observations_count = 0;
    int prev_planters_seeded = 0;
    int prev_prism_toggles = 0;
    bool prev_laminar_ford_open = false;
    bool prev_aux_stevino_open = false;
    std::vector<bool> prev_struct_switches{};
    float fauna_chirp_timer = 6.5f;

    // Stato osservato per le 6 meccaniche di zona
    std::vector<int> prev_mirror_dirs{};
    bool prev_solar_gate_open = false;
    std::vector<bool> prev_sluice_pulled{};
    bool prev_wind_gust_active = false;
    std::vector<bool> prev_resonance_active{};
    int prev_resonance_next_order = 0;
    bool prev_jade_bridge_unlocked = false;
    std::vector<eco::GeyserPhase> prev_geyser_phases{};
    std::vector<bool> prev_valve_open{};
    std::vector<bool> prev_seal_attuned{};

    // Tracciamento passi & salto P0
    eco::Vec2 prev_p0_pos{0.0f, 0.0f};
    bool prev_p0_grounded = true;
    float step_distance_acc = 0.0f;
    FastRng pitch_rng{0x7711u};
};

AudioEngine::~AudioEngine() {
    Shutdown();
}

void AudioEngine::Init(bool silent_mode) {
    if (impl_ != nullptr) {
        return;
    }
    impl_ = new Impl();
    muted_ = silent_mode;

    if (silent_mode) {
        ready_ = false;
        return;
    }

    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
        initialized_device_ = IsAudioDeviceReady();
    }
    if (!IsAudioDeviceReady()) {
        ready_ = false;
        return;
    }

    ::SetMasterVolume(muted_ ? 0.0f : master_volume_);

    auto load_sfx = [&](SfxId id, const std::vector<float>& pcm) {
        impl_->sfx[static_cast<std::size_t>(id)] = BuildSoundFromSamples(pcm);
    };

    load_sfx(SfxId::FootstepStone, SynthFootstepStone());
    load_sfx(SfxId::FootstepGrass, SynthFootstepGrass());
    load_sfx(SfxId::Jump, SynthJump());
    load_sfx(SfxId::Land, SynthLand());
    load_sfx(SfxId::ScanPulse, SynthScanPulse());
    load_sfx(SfxId::PortalEnter, SynthPortalEnter());
    load_sfx(SfxId::RelicCollect, SynthRelicCollect());
    load_sfx(SfxId::NpcInteract, SynthNpcInteract());
    load_sfx(SfxId::ModalOpen, SynthModalOpen());
    load_sfx(SfxId::ModalClose, SynthModalClose());
    load_sfx(SfxId::SliderTick, SynthSliderTick());
    load_sfx(SfxId::CalibrateClean, SynthCalibrateClean());
    load_sfx(SfxId::CalibrateBypass, SynthCalibrateBypass());
    load_sfx(SfxId::BridgeUnlock, SynthBridgeUnlock());
    load_sfx(SfxId::ZoneDiscover, SynthZoneDiscover());
    load_sfx(SfxId::PhaseShift, SynthPhaseShift());
    load_sfx(SfxId::UiClick, SynthUiClick());
    load_sfx(SfxId::EpilogueChord, SynthEpilogueChord());

    load_sfx(SfxId::SolarMirrorRotate, SynthSolarMirrorRotate());
    load_sfx(SfxId::SolarPlateUnlock, SynthSolarPlateUnlock());
    load_sfx(SfxId::SluiceLeverToggle, SynthSluiceLeverToggle());
    load_sfx(SfxId::WindGustWhoosh, SynthWindGustWhoosh());
    load_sfx(SfxId::ResonancePadNote, SynthResonancePadNote());
    load_sfx(SfxId::ResonanceSequenceFail, SynthResonanceSequenceFail());
    load_sfx(SfxId::JadeBridgeUnlock, SynthJadeBridgeUnlock());
    load_sfx(SfxId::GeyserErupt, SynthGeyserErupt());
    load_sfx(SfxId::ValveClose, SynthValveClose());
    load_sfx(SfxId::SanctuarySealTune, SynthSanctuarySealTune());
    load_sfx(SfxId::InterludeChime, SynthInterludeChime());
    load_sfx(SfxId::TrialPulse, SynthTrialPulse());
    load_sfx(SfxId::CalibrateRisky, SynthCalibrateRisky());
    load_sfx(SfxId::ObservationInspect, SynthObservationInspect());
    load_sfx(SfxId::BioPlanterBloom, SynthBioPlanterBloom());
    load_sfx(SfxId::PrismRemoteToggle, SynthPrismRemoteToggle());
    load_sfx(SfxId::FaunaChirp, SynthFaunaChirp());

    for (int i = 0; i < static_cast<int>(MusicLayer::Count); ++i) {
        const auto layer = static_cast<MusicLayer>(i);
        impl_->music[static_cast<std::size_t>(i)] = BuildSoundFromSamples(SynthMusicLoop(layer));
        impl_->music_vol[static_cast<std::size_t>(i)] = 0.0f;
    }

    for (int i = 0; i < static_cast<int>(NatureLayer::Count); ++i) {
        const auto layer = static_cast<NatureLayer>(i);
        impl_->nature[static_cast<std::size_t>(i)] = BuildSoundFromSamples(SynthNatureLoop(layer));
        impl_->nature_vol[static_cast<std::size_t>(i)] = 0.0f;
    }

    ready_ = true;
}

void AudioEngine::Shutdown() {
    if (g_audio_engine == this) {
        g_audio_engine = nullptr;
    }
    if (impl_ != nullptr) {
        if (ready_ && IsAudioDeviceReady()) {
            for (Sound& s : impl_->sfx) {
                if (s.frameCount > 0) {
                    StopSound(s);
                    UnloadSound(s);
                }
            }
            for (Sound& m : impl_->music) {
                if (m.frameCount > 0) {
                    StopSound(m);
                    UnloadSound(m);
                }
            }
            for (Sound& n : impl_->nature) {
                if (n.frameCount > 0) {
                    StopSound(n);
                    UnloadSound(n);
                }
            }
        }
        delete impl_;
        impl_ = nullptr;
    }
    if (initialized_device_ && IsAudioDeviceReady()) {
        CloseAudioDevice();
    }
    initialized_device_ = false;
    ready_ = false;
}

void AudioEngine::SetMasterVolume(float volume) noexcept {
    master_volume_ = std::clamp(volume, 0.0f, 1.0f);
    if (ready_ && IsAudioDeviceReady()) {
        ::SetMasterVolume(muted_ ? 0.0f : master_volume_);
    }
}

void AudioEngine::SetMuted(bool muted) noexcept {
    muted_ = muted;
    if (ready_ && IsAudioDeviceReady()) {
        ::SetMasterVolume(muted_ ? 0.0f : master_volume_);
    }
}

void AudioEngine::SetAmbientEnabled(bool enabled) noexcept {
    ambient_enabled_ = enabled;
}

void AudioEngine::SetMusicVolume(float volume) noexcept {
    music_volume_ = std::clamp(volume, 0.0f, 1.0f);
}

void AudioEngine::SetEffectsVolume(float volume) noexcept {
    effects_volume_ = std::clamp(volume, 0.0f, 1.0f);
}

void AudioEngine::SetAmbientVolume(float volume) noexcept {
    ambient_volume_ = std::clamp(volume, 0.0f, 1.0f);
}

void AudioEngine::SetUiSoundsEnabled(bool enabled) noexcept {
    ui_sounds_enabled_ = enabled;
}

void AudioEngine::PlaySfx(SfxId id, float volume_scale, float pitch) {
    if (!ready_ || muted_ || master_volume_ <= 0.001f || effects_volume_ <= 0.001f || impl_ == nullptr) {
        return;
    }
    const auto idx = static_cast<std::size_t>(id);
    if (idx >= impl_->sfx.size()) {
        return;
    }
    Sound& snd = impl_->sfx[idx];
    if (snd.frameCount == 0) {
        return;
    }
    SetSoundVolume(snd, std::clamp(volume_scale * effects_volume_, 0.0f, 1.0f));
    SetSoundPitch(snd, std::clamp(pitch, 0.55f, 1.85f));
    PlaySound(snd);
}

void AudioEngine::PlayUiSound(SfxId id, float volume_scale, float pitch) {
    if (!ui_sounds_enabled_) {
        return;
    }
    if (!ready_ || muted_ || master_volume_ <= 0.001f || impl_ == nullptr) {
        return;
    }
    const auto idx = static_cast<std::size_t>(id);
    if (idx >= impl_->sfx.size()) {
        return;
    }
    Sound& snd = impl_->sfx[idx];
    if (snd.frameCount == 0) {
        return;
    }
    // I suoni UI usano un pavimento minimo se effects_volume e' basso ma UI e' attiva
    const float gain = std::clamp(volume_scale * std::max(0.35f, effects_volume_), 0.0f, 1.0f);
    SetSoundVolume(snd, gain);
    SetSoundPitch(snd, std::clamp(pitch, 0.65f, 1.45f));
    PlaySound(snd);
}

void AudioEngine::PlayLivePreview(AudioPreviewChannel channel, float pitch) {
    if (!ready_ || muted_ || master_volume_ <= 0.001f || impl_ == nullptr) {
        return;
    }
    auto play_direct = [&](SfxId id, float gain, float p) {
        const auto idx = static_cast<std::size_t>(id);
        if (idx >= impl_->sfx.size()) {
            return;
        }
        Sound& snd = impl_->sfx[idx];
        if (snd.frameCount == 0 || gain <= 0.001f) {
            return;
        }
        SetSoundVolume(snd, std::clamp(gain, 0.0f, 1.0f));
        SetSoundPitch(snd, std::clamp(p, 0.65f, 1.45f));
        PlaySound(snd);
    };

    switch (channel) {
        case AudioPreviewChannel::Master:
            play_direct(SfxId::ResonancePadNote, 0.52f, pitch);
            break;
        case AudioPreviewChannel::Music:
            play_direct(SfxId::InterludeChime, 0.62f * music_volume_, pitch);
            break;
        case AudioPreviewChannel::Effects:
            play_direct(SfxId::SolarMirrorRotate, 0.72f * effects_volume_, pitch);
            break;
        case AudioPreviewChannel::Ambient:
            if (ambient_enabled_) {
                play_direct(SfxId::WindGustWhoosh, 0.55f * ambient_volume_, pitch);
            }
            break;
        case AudioPreviewChannel::Ui:
            PlayUiSound(SfxId::UiClick, 0.62f, pitch);
            break;
    }
}

void AudioEngine::SyncAndUpdate(
    float dt,
    const eco::WorldContext& world,
    std::string_view active_state_name,
    bool pause_open,
    bool journal_open,
    int journal_tab_index) {
    if (impl_ == nullptr) {
        return;
    }

    const float safe_dt = std::clamp(dt, 0.0f, 0.1f);
    impl_->slider_tick_cooldown = std::max(0.0f, impl_->slider_tick_cooldown - safe_dt);

    const bool in_gameplay =
        (active_state_name == "OverworldState" || active_state_name == "StructureState");

    // 1. Aggiornamento tappeti MUSICALI e AMBIENTALI con crossfade morbido (anche durante intramezzi)
    if (ready_ && IsAudioDeviceReady()) {
        std::array<float, static_cast<std::size_t>(MusicLayer::Count)> music_target{};
        std::array<float, static_cast<std::size_t>(NatureLayer::Count)> nature_target{};

        if (!muted_ && master_volume_ > 0.001f) {
            const float pause_duck = (pause_open || journal_open) ? 0.42f : 1.0f;
            float interlude_duck = 1.0f;
            if (world.interlude.active) {
                // Curva morbida a V che accompagna lo swap memoria al 50% dell'intramezzo
                const float p = world.interlude.Progress();
                const float dist_from_mid = std::abs(p - 0.5f) * 2.0f; // 1 -> 0 -> 1
                interlude_duck = 0.25f + 0.75f * dist_from_mid;
            }
            const float duck = pause_duck * interlude_duck;

            // Musica
            if (music_volume_ > 0.001f) {
                const eco::ZoneEcoStatus eco_st = world.ActiveSceneEcoStatus();
                bool risky_scene = false;
                if (active_state_name == "OverworldState") {
                    if (world.current_zone == eco::ZoneId::DawnTerraces) {
                        risky_scene = (world.GetMachine(eco::MachineType::SolarArray).approach ==
                                       eco::CalibrationApproach::RiskyEfficient);
                    } else if (world.current_zone == eco::ZoneId::CanalQuarter) {
                        risky_scene = (world.GetMachine(eco::MachineType::HydroTurbine).approach ==
                                       eco::CalibrationApproach::RiskyEfficient);
                    }
                } else if (active_state_name == "StructureState" &&
                           world.active_structure == eco::StructureId::HypogeanCisterns) {
                    risky_scene = (world.GetMachine(eco::MachineType::HydroTurbine).approach ==
                                   eco::CalibrationApproach::RiskyEfficient);
                }

                if (active_state_name == "TitleState" || active_state_name == "IntroState") {
                    music_target[static_cast<std::size_t>(MusicLayer::OverworldClean)] = 0.42f * duck;
                } else if (active_state_name == "OverworldState") {
                    const float poll = world.ecosystem.PollutionRatio();
                    const float bloom_boost = (eco_st.band == eco::EcoHealthBand::Flourishing) ? 0.12f : 0.0f;
                    music_target[static_cast<std::size_t>(MusicLayer::OverworldClean)] =
                        std::clamp(0.54f - 0.28f * poll + bloom_boost, 0.12f, 0.68f) * duck;
                    music_target[static_cast<std::size_t>(MusicLayer::OverworldSmog)] =
                        (0.06f + 0.44f * poll) * duck;
                } else if (active_state_name == "StructureState") {
                    switch (world.active_structure) {
                        case eco::StructureId::HypogeanCisterns:
                            music_target[static_cast<std::size_t>(MusicLayer::Cisterns)] = 0.50f * duck;
                            break;
                        case eco::StructureId::PendulumTower:
                            music_target[static_cast<std::size_t>(MusicLayer::Tower)] = 0.48f * duck;
                            break;
                        case eco::StructureId::GeothermalChamber:
                            music_target[static_cast<std::size_t>(MusicLayer::Geothermal)] = 0.52f * duck;
                            break;
                        case eco::StructureId::Count:
                            break;
                    }
                }
                if (risky_scene) {
                    music_target[static_cast<std::size_t>(MusicLayer::CriticalOverdrive)] = 0.34f * duck;
                }
            }

            // Natura / Ambiente (separato da musica!)
            if (ambient_enabled_ && ambient_volume_ > 0.001f) {
                const eco::ZoneEcoStatus eco_st = world.ActiveSceneEcoStatus();
                const bool healed = (eco_st.band == eco::EcoHealthBand::Flourishing ||
                                     eco_st.band == eco::EcoHealthBand::Recovering);
                if (active_state_name == "TitleState" || active_state_name == "IntroState") {
                    nature_target[static_cast<std::size_t>(NatureLayer::AirBreeze)] = 0.28f * duck;
                } else if (active_state_name == "OverworldState") {
                    nature_target[static_cast<std::size_t>(NatureLayer::AirBreeze)] = 0.24f * duck;
                    switch (world.current_zone) {
                        case eco::ZoneId::DawnTerraces:
                        case eco::ZoneId::HangingGardens:
                        case eco::ZoneId::HarmonySanctuary:
                            nature_target[static_cast<std::size_t>(NatureLayer::AirBreeze)] = 0.34f * duck;
                            if (eco_st.band == eco::EcoHealthBand::Flourishing) {
                                nature_target[static_cast<std::size_t>(NatureLayer::LaminarStream)] = 0.22f * duck;
                            }
                            break;
                        case eco::ZoneId::CanalQuarter:
                            if (world.zone_mechanics.laminar_flow_active || healed) {
                                nature_target[static_cast<std::size_t>(NatureLayer::LaminarStream)] = 0.44f * duck;
                                nature_target[static_cast<std::size_t>(NatureLayer::CanalWater)] = 0.18f * duck;
                            } else {
                                const float turb = std::clamp(eco_st.current_turbulence, 0.0f, 1.0f);
                                nature_target[static_cast<std::size_t>(NatureLayer::CanalWater)] =
                                    (0.42f + 0.18f * turb) * duck;
                            }
                            break;
                        case eco::ZoneId::WindPlateau: {
                            const float gust = world.zone_mechanics.wind.gust_intensity;
                            nature_target[static_cast<std::size_t>(NatureLayer::PlateauWind)] =
                                (0.28f + 0.48f * gust) * duck;
                            break;
                        }
                        case eco::ZoneId::FumaroleCaldera: {
                            float max_steam = 0.22f;
                            for (const auto& g : world.zone_mechanics.geysers) {
                                max_steam = std::max(max_steam, 0.22f + 0.45f * g.intensity);
                            }
                            nature_target[static_cast<std::size_t>(NatureLayer::ThermalSteam)] =
                                max_steam * duck;
                            break;
                        }
                        case eco::ZoneId::Count:
                            break;
                    }
                } else if (active_state_name == "StructureState") {
                    if (world.active_structure == eco::StructureId::HypogeanCisterns) {
                        if (healed) {
                            nature_target[static_cast<std::size_t>(NatureLayer::LaminarStream)] = 0.42f * duck;
                            nature_target[static_cast<std::size_t>(NatureLayer::CanalWater)] = 0.22f * duck;
                        } else {
                            nature_target[static_cast<std::size_t>(NatureLayer::CanalWater)] = 0.48f * duck;
                        }
                        if (world.GetMachine(eco::MachineType::HydroTurbine).approach ==
                            eco::CalibrationApproach::RiskyEfficient) {
                            nature_target[static_cast<std::size_t>(NatureLayer::ThermalSteam)] = 0.26f * duck;
                        }
                    } else if (world.active_structure == eco::StructureId::GeothermalChamber) {
                        nature_target[static_cast<std::size_t>(NatureLayer::ThermalSteam)] = 0.48f * duck;
                    } else {
                        nature_target[static_cast<std::size_t>(NatureLayer::AirBreeze)] = 0.26f * duck;
                    }
                }
            }
        }

        const float blend = std::min(1.0f, safe_dt * 3.8f);
        for (std::size_t i = 0; i < impl_->music.size(); ++i) {
            float& cur = impl_->music_vol[i];
            cur += (music_target[i] - cur) * blend;
            Sound& snd = impl_->music[i];
            if (snd.frameCount == 0) {
                continue;
            }
            const float final_vol = cur * music_volume_;
            if (final_vol > 0.008f) {
                SetSoundVolume(snd, std::clamp(final_vol, 0.0f, 1.0f));
                if (!IsSoundPlaying(snd)) {
                    PlaySound(snd);
                }
            } else if (IsSoundPlaying(snd)) {
                StopSound(snd);
            }
        }

        for (std::size_t i = 0; i < impl_->nature.size(); ++i) {
            float& cur = impl_->nature_vol[i];
            cur += (nature_target[i] - cur) * blend;
            Sound& snd = impl_->nature[i];
            if (snd.frameCount == 0) {
                continue;
            }
            const float final_vol = cur * ambient_volume_;
            if (final_vol > 0.008f) {
                SetSoundVolume(snd, std::clamp(final_vol, 0.0f, 1.0f));
                if (!IsSoundPlaying(snd)) {
                    PlaySound(snd);
                }
            } else if (IsSoundPlaying(snd)) {
                StopSound(snd);
            }
        }
    }

    const eco::Player* p0 = world.players.GetPlayer(0);

    auto snapshot_mechanics = [&]() {
        const auto& zm = world.zone_mechanics;
        impl_->prev_mirror_dirs.resize(zm.solar_mirrors.size());
        for (std::size_t i = 0; i < zm.solar_mirrors.size(); ++i) {
            impl_->prev_mirror_dirs[i] = zm.solar_mirrors[i].dir_index;
        }
        impl_->prev_solar_gate_open = zm.solar_gate_open;

        impl_->prev_sluice_pulled.resize(zm.sluice_levers.size());
        for (std::size_t i = 0; i < zm.sluice_levers.size(); ++i) {
            impl_->prev_sluice_pulled[i] = zm.sluice_levers[i].pulled;
        }
        impl_->prev_wind_gust_active = zm.wind.gust_active;

        impl_->prev_resonance_active.resize(zm.resonance_pads.size());
        for (std::size_t i = 0; i < zm.resonance_pads.size(); ++i) {
            impl_->prev_resonance_active[i] = zm.resonance_pads[i].activated;
        }
        impl_->prev_resonance_next_order = zm.resonance_next_order;
        impl_->prev_jade_bridge_unlocked = zm.jade_bridge_unlocked;

        impl_->prev_geyser_phases.resize(zm.geysers.size());
        for (std::size_t i = 0; i < zm.geysers.size(); ++i) {
            impl_->prev_geyser_phases[i] = zm.geysers[i].phase;
        }
        impl_->prev_valve_open.resize(zm.relief_valves.size());
        for (std::size_t i = 0; i < zm.relief_valves.size(); ++i) {
            impl_->prev_valve_open[i] = zm.relief_valves[i].open;
        }
        impl_->prev_seal_attuned.resize(zm.sanctuary_seals.size());
        for (std::size_t i = 0; i < zm.sanctuary_seals.size(); ++i) {
            impl_->prev_seal_attuned[i] = zm.sanctuary_seals[i].attuned;
        }
        impl_->prev_laminar_ford_open = zm.laminar_ford_open;
        impl_->prev_aux_stevino_open = world.structure.puzzle.aux_stevino_valve_open;
        const auto& sw = world.structure.puzzle.switches;
        impl_->prev_struct_switches.resize(sw.size());
        for (std::size_t i = 0; i < sw.size(); ++i) {
            impl_->prev_struct_switches[i] = sw[i].active;
        }
    };

    // 2. Primo frame: fotografa lo stato iniziale senza sparare suoni spuri
    if (!impl_->has_prev) {
        impl_->has_prev = true;
        impl_->prev_modal_open = world.calibration_modal.is_open;
        impl_->prev_modal_value = world.calibration_modal.slider_value;
        impl_->prev_stabilizer_value = world.calibration_modal.stabilizer_value;
        impl_->prev_pause_open = pause_open;
        impl_->prev_journal_open = journal_open;
        impl_->prev_journal_tab = journal_tab_index;
        impl_->prev_mode = world.current_mode;
        impl_->prev_structure = world.active_structure;
        impl_->prev_zone = world.current_zone;
        impl_->prev_interlude_active = world.interlude.active;
        impl_->prev_ui_phase = world.ecosystem.ui_phase;
        impl_->prev_architect_recruited = world.ecosystem.architect_recruited;
        impl_->prev_sim_completed = world.simulation_completed;
        impl_->prev_collected_count = world.collectibles.CollectedCount();
        impl_->prev_scan_timer = world.scan_pulse_timer;
        impl_->prev_total_trials = world.total_trials_run;
        impl_->prev_observations_count = world.observations_inspected_count;
        impl_->prev_planters_seeded = world.planters_seeded_count;
        impl_->prev_prism_toggles = world.prism_remote_toggles;
        for (std::size_t i = 0; i < world.machines.size(); ++i) {
            impl_->prev_machine_states[i] = world.machines[i].state;
            impl_->prev_machine_approaches[i] = world.machines[i].approach;
        }
        for (std::size_t i = 0; i < world.npcs.size(); ++i) {
            impl_->prev_npc_met[i] = world.npcs[i].met;
            impl_->prev_npc_recruited[i] = world.npcs[i].recruited;
        }
        snapshot_mechanics();
        if (p0 != nullptr) {
            impl_->prev_p0_pos = p0->pos;
            impl_->prev_p0_grounded = p0->isGrounded;
        }
        return;
    }

    // 3. Interazioni UI (Pausa, Diario, Modal di calibrazione — rispettano ui_sounds_enabled_)
    if (pause_open != impl_->prev_pause_open) {
        PlayUiSound(pause_open ? SfxId::ModalOpen : SfxId::ModalClose, 0.72f);
        impl_->prev_pause_open = pause_open;
    }
    if (journal_open != impl_->prev_journal_open) {
        PlayUiSound(journal_open ? SfxId::ModalOpen : SfxId::ModalClose, 0.72f);
        impl_->prev_journal_open = journal_open;
    } else if (journal_open && journal_tab_index != impl_->prev_journal_tab) {
        PlayUiSound(SfxId::UiClick, 0.65f, 1.08f);
    }
    impl_->prev_journal_tab = journal_tab_index;

    if (world.calibration_modal.is_open != impl_->prev_modal_open) {
        PlayUiSound(world.calibration_modal.is_open ? SfxId::ModalOpen : SfxId::ModalClose, 0.80f);
        impl_->prev_modal_open = world.calibration_modal.is_open;
        impl_->prev_modal_value = world.calibration_modal.slider_value;
        impl_->prev_stabilizer_value = world.calibration_modal.stabilizer_value;
    } else if (world.calibration_modal.is_open) {
        const bool main_changed = std::abs(world.calibration_modal.slider_value - impl_->prev_modal_value) > 1e-3f;
        const bool stab_changed = std::abs(world.calibration_modal.stabilizer_value - impl_->prev_stabilizer_value) > 1e-3f;
        if (main_changed || stab_changed) {
            if (impl_->slider_tick_cooldown <= 0.0f) {
                const float base_p = stab_changed ? 1.14f : 0.96f;
                PlayUiSound(SfxId::SliderTick, 0.55f, base_p + 0.07f * impl_->pitch_rng.NextSigned());
                impl_->slider_tick_cooldown = 0.048f;
            }
            impl_->prev_modal_value = world.calibration_modal.slider_value;
            impl_->prev_stabilizer_value = world.calibration_modal.stabilizer_value;
        }
    }

    // 4. Eventi fisici, intramezzi, indagine scientifica e 3 soluzioni
    if (world.interlude.active && !impl_->prev_interlude_active) {
        PlaySfx(SfxId::InterludeChime, 0.74f);
    }
    impl_->prev_interlude_active = world.interlude.active;

    if (world.prism_remote_toggles > impl_->prev_prism_toggles) {
        PlaySfx(SfxId::PrismRemoteToggle, 0.88f, 1.0f + 0.04f * impl_->pitch_rng.NextSigned());
        impl_->prev_prism_toggles = world.prism_remote_toggles;
        impl_->prev_scan_timer = world.scan_pulse_timer;
    } else if (world.scan_pulse_timer > impl_->prev_scan_timer + 0.1f) {
        PlaySfx(SfxId::ScanPulse, 0.85f);
        impl_->prev_scan_timer = world.scan_pulse_timer;
    } else {
        impl_->prev_scan_timer = world.scan_pulse_timer;
    }

    if (world.total_trials_run > impl_->prev_total_trials) {
        const float eff = std::clamp(world.calibration_modal.last_trial.measured_efficiency, 0.0f, 1.0f);
        PlaySfx(SfxId::TrialPulse, 0.86f, 0.86f + 0.28f * eff);
        impl_->prev_total_trials = world.total_trials_run;
    }

    if (world.observations_inspected_count > impl_->prev_observations_count) {
        PlaySfx(SfxId::ObservationInspect, 0.78f, 0.96f + 0.08f * impl_->pitch_rng.NextSigned());
        impl_->prev_observations_count = world.observations_inspected_count;
    }

    if (world.planters_seeded_count > impl_->prev_planters_seeded) {
        PlaySfx(SfxId::BioPlanterBloom, 0.90f);
        impl_->prev_planters_seeded = world.planters_seeded_count;
    }

    const int collected = world.collectibles.CollectedCount();
    if (collected > impl_->prev_collected_count) {
        PlaySfx(SfxId::RelicCollect, 0.88f);
    }
    impl_->prev_collected_count = collected;

    for (std::size_t i = 0; i < world.machines.size(); ++i) {
        const eco::MachineState st = world.machines[i].state;
        const eco::CalibrationApproach ap = world.machines[i].approach;
        if (st != impl_->prev_machine_states[i] || ap != impl_->prev_machine_approaches[i]) {
            if (ap == eco::CalibrationApproach::RiskyEfficient) {
                PlaySfx(SfxId::CalibrateRisky, 0.90f);
            } else if (ap == eco::CalibrationApproach::DirtyImmediate ||
                       st == eco::MachineState::BypassedDirty) {
                PlaySfx(SfxId::CalibrateBypass, 0.86f);
            } else if (st == eco::MachineState::CleanActive ||
                       ap == eco::CalibrationApproach::CleanSlow) {
                PlaySfx(SfxId::CalibrateClean, 0.90f);
            }
            impl_->prev_machine_states[i] = st;
            impl_->prev_machine_approaches[i] = ap;
        }
    }

    if (world.ecosystem.architect_recruited && !impl_->prev_architect_recruited) {
        PlaySfx(SfxId::BridgeUnlock, 0.85f);
    }
    impl_->prev_architect_recruited = world.ecosystem.architect_recruited;

    for (std::size_t i = 0; i < world.npcs.size(); ++i) {
        const bool newly_met = world.npcs[i].met && !impl_->prev_npc_met[i];
        const bool newly_rec = world.npcs[i].recruited && !impl_->prev_npc_recruited[i];
        if (newly_met || newly_rec) {
            PlaySfx(SfxId::NpcInteract, 0.72f);
        }
        impl_->prev_npc_met[i] = world.npcs[i].met;
        impl_->prev_npc_recruited[i] = world.npcs[i].recruited;
    }

    if (world.simulation_completed && !impl_->prev_sim_completed) {
        PlaySfx(SfxId::EpilogueChord, 0.92f);
    }
    impl_->prev_sim_completed = world.simulation_completed;

    if (world.ecosystem.ui_phase != impl_->prev_ui_phase) {
        PlaySfx(SfxId::PhaseShift, 0.72f);
        impl_->prev_ui_phase = world.ecosystem.ui_phase;
    }

    // Meccaniche top-down delle 6 zone (solo in Overworld attivo fuori da intramezzo)
    if (in_gameplay && world.current_mode == eco::GameMode::Overworld && !world.interlude.active) {
        const auto& zm = world.zone_mechanics;

        if (zm.solar_mirrors.size() == impl_->prev_mirror_dirs.size()) {
            for (std::size_t i = 0; i < zm.solar_mirrors.size(); ++i) {
                if (zm.solar_mirrors[i].dir_index != impl_->prev_mirror_dirs[i]) {
                    PlaySfx(SfxId::SolarMirrorRotate, 0.74f, 0.95f + 0.08f * static_cast<float>(zm.solar_mirrors[i].dir_index));
                    break;
                }
            }
        }
        if (zm.solar_gate_open && !impl_->prev_solar_gate_open) {
            PlaySfx(SfxId::SolarPlateUnlock, 0.86f);
        }

        if (zm.sluice_levers.size() == impl_->prev_sluice_pulled.size()) {
            for (std::size_t i = 0; i < zm.sluice_levers.size(); ++i) {
                if (zm.sluice_levers[i].pulled != impl_->prev_sluice_pulled[i]) {
                    PlaySfx(SfxId::SluiceLeverToggle, 0.78f, zm.sluice_levers[i].pulled ? 1.06f : 0.92f);
                    break;
                }
            }
        }

        if (world.current_zone == eco::ZoneId::WindPlateau &&
            zm.wind.gust_active && !impl_->prev_wind_gust_active) {
            PlaySfx(SfxId::WindGustWhoosh, 0.70f);
        }

        if (zm.resonance_pads.size() == impl_->prev_resonance_active.size()) {
            for (std::size_t i = 0; i < zm.resonance_pads.size(); ++i) {
                if (zm.resonance_pads[i].activated && !impl_->prev_resonance_active[i]) {
                    const float pitch = std::clamp(zm.resonance_pads[i].pitch_hz / 440.0f, 0.6f, 1.8f);
                    PlaySfx(SfxId::ResonancePadNote, 0.80f, pitch);
                    break;
                }
            }
        }
        if (!zm.jade_bridge_unlocked && zm.resonance_next_order == 0 && impl_->prev_resonance_next_order > 0) {
            PlaySfx(SfxId::ResonanceSequenceFail, 0.68f);
        }
        if (zm.jade_bridge_unlocked && !impl_->prev_jade_bridge_unlocked) {
            PlaySfx(SfxId::JadeBridgeUnlock, 0.88f);
        }

        if (world.current_zone == eco::ZoneId::FumaroleCaldera &&
            zm.geysers.size() == impl_->prev_geyser_phases.size()) {
            for (std::size_t i = 0; i < zm.geysers.size(); ++i) {
                if (zm.geysers[i].phase == eco::GeyserPhase::Erupting &&
                    impl_->prev_geyser_phases[i] != eco::GeyserPhase::Erupting) {
                    PlaySfx(SfxId::GeyserErupt, 0.65f, 0.94f + 0.12f * impl_->pitch_rng.NextSigned());
                    break;
                }
            }
        }
        if (zm.relief_valves.size() == impl_->prev_valve_open.size()) {
            for (std::size_t i = 0; i < zm.relief_valves.size(); ++i) {
                if (zm.relief_valves[i].open != impl_->prev_valve_open[i]) {
                    PlaySfx(SfxId::ValveClose, 0.78f);
                    break;
                }
            }
        }

        if (zm.sanctuary_seals.size() == impl_->prev_seal_attuned.size()) {
            for (std::size_t i = 0; i < zm.sanctuary_seals.size(); ++i) {
                if (zm.sanctuary_seals[i].attuned && !impl_->prev_seal_attuned[i]) {
                    PlaySfx(SfxId::SanctuarySealTune, 0.86f, 0.92f + 0.06f * static_cast<float>(i));
                    break;
                }
            }
        }

        if (zm.laminar_ford_open && !impl_->prev_laminar_ford_open) {
            PlaySfx(SfxId::JadeBridgeUnlock, 0.84f, 0.92f);
        }

        // Cinguettio leggero e vivo quando la fauna e' tornata in zona guarita
        const eco::ZoneEcoStatus eco_st = world.ActiveSceneEcoStatus();
        if (ambient_enabled_ && ambient_volume_ > 0.02f && !pause_open && !journal_open &&
            !world.calibration_modal.is_open && eco_st.fauna_count >= 5 &&
            world.current_zone != eco::ZoneId::FumaroleCaldera) {
            impl_->fauna_chirp_timer -= safe_dt;
            if (impl_->fauna_chirp_timer <= 0.0f) {
                const float chirp_pitch = 0.90f + 0.18f * (0.5f + 0.5f * impl_->pitch_rng.NextSigned());
                PlaySfx(SfxId::FaunaChirp, 0.34f * ambient_volume_, chirp_pitch);
                impl_->fauna_chirp_timer = 7.2f + 3.4f * (0.5f + 0.5f * impl_->pitch_rng.NextSigned());
            }
        }
    } else if (in_gameplay && world.current_mode == eco::GameMode::Structure && !world.interlude.active) {
        const auto& pz = world.structure.puzzle;
        if (pz.aux_stevino_valve_open && !impl_->prev_aux_stevino_open) {
            PlaySfx(SfxId::ValveClose, 0.82f, 1.08f);
        }
        if (pz.switches.size() == impl_->prev_struct_switches.size()) {
            for (std::size_t i = 0; i < pz.switches.size(); ++i) {
                if (pz.switches[i].active != impl_->prev_struct_switches[i]) {
                    PlaySfx(SfxId::SluiceLeverToggle, 0.76f, pz.switches[i].active ? 1.08f : 0.92f);
                    break;
                }
            }
        }
    }
    snapshot_mechanics();

    if (world.current_mode != impl_->prev_mode ||
        (world.current_mode == eco::GameMode::Structure &&
         world.active_structure != impl_->prev_structure)) {
        PlaySfx(SfxId::PortalEnter, 0.80f);
        impl_->prev_mode = world.current_mode;
        impl_->prev_structure = world.active_structure;
        impl_->step_distance_acc = 0.0f;
        if (p0 != nullptr) {
            impl_->prev_p0_pos = p0->pos;
            impl_->prev_p0_grounded = p0->isGrounded;
        }
    } else if (in_gameplay && world.current_mode == eco::GameMode::Overworld &&
               world.current_zone != impl_->prev_zone) {
        if (!world.interlude.active) {
            PlaySfx(SfxId::ZoneDiscover, 0.62f);
        }
        impl_->prev_zone = world.current_zone;
    }

    // 5. Passi e Salto/Atterraggio di P0
    if (in_gameplay && !pause_open && !journal_open && !world.calibration_modal.is_open &&
        !world.interlude.active && p0 != nullptr && p0->active) {
        if (world.current_mode == eco::GameMode::Structure) {
            if (impl_->prev_p0_grounded && !p0->isGrounded && p0->y_velocity < -100.0f) {
                PlaySfx(SfxId::Jump, 0.68f, 0.96f + 0.06f * impl_->pitch_rng.NextSigned());
            } else if (!impl_->prev_p0_grounded && p0->isGrounded) {
                PlaySfx(SfxId::Land, 0.65f, 0.95f + 0.08f * impl_->pitch_rng.NextSigned());
            }
        }

        const float dist = (p0->pos - impl_->prev_p0_pos).Length();
        const bool can_step = (world.current_mode == eco::GameMode::Overworld) || p0->isGrounded;
        if (can_step && dist > 0.25f && dist < 48.0f) {
            impl_->step_distance_acc += dist;
            if (impl_->step_distance_acc >= 54.0f) {
                impl_->step_distance_acc = 0.0f;
                bool stone_surface = (world.current_mode == eco::GameMode::Structure);
                if (!stone_surface) {
                    const eco::TileCoord tc = world.overworld.tilemap.WorldToTile(p0->Center());
                    const eco::TileId tid = world.overworld.tilemap.GetTile(tc.row, tc.col);
                    stone_surface = (tid == eco::TileId::PavedPath ||
                                     tid == eco::TileId::SanctuaryFloor ||
                                     tid == eco::TileId::HiddenBridge ||
                                     tid == eco::TileId::JadeBridge ||
                                     tid == eco::TileId::StructurePortal ||
                                     tid == eco::TileId::ZoneGateway);
                }
                const float pitch = 0.92f + 0.10f * impl_->pitch_rng.NextSigned();
                PlaySfx(stone_surface ? SfxId::FootstepStone : SfxId::FootstepGrass, 0.48f, pitch);
            }
        } else if (dist >= 48.0f) {
            impl_->step_distance_acc = 0.0f;
        }

        impl_->prev_p0_pos = p0->pos;
        impl_->prev_p0_grounded = p0->isGrounded;
    } else if (p0 != nullptr) {
        impl_->prev_p0_pos = p0->pos;
        impl_->prev_p0_grounded = p0->isGrounded;
    }
}

void SetGlobalAudioEngine(AudioEngine* engine) noexcept {
    g_audio_engine = engine;
}

void PlayUiSfx(SfxId id, float volume_scale, float pitch) noexcept {
    if (g_audio_engine != nullptr) {
        g_audio_engine->PlayUiSound(id, volume_scale, pitch);
    }
}

void PlayAudioPreview(AudioPreviewChannel channel, float pitch) noexcept {
    if (g_audio_engine != nullptr) {
        g_audio_engine->PlayLivePreview(channel, pitch);
    }
}

void ApplyGlobalAudioSettings(
    float master_volume,
    bool muted,
    bool ambient_enabled,
    float music_volume,
    float effects_volume,
    float ambient_volume,
    bool ui_sounds_enabled) noexcept {
    if (g_audio_engine != nullptr) {
        g_audio_engine->SetMasterVolume(master_volume);
        g_audio_engine->SetMuted(muted);
        g_audio_engine->SetAmbientEnabled(ambient_enabled);
        g_audio_engine->SetMusicVolume(music_volume);
        g_audio_engine->SetEffectsVolume(effects_volume);
        g_audio_engine->SetAmbientVolume(ambient_volume);
        g_audio_engine->SetUiSoundsEnabled(ui_sounds_enabled);
    }
}

} // namespace eco::app
