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
    // Sonar cristallino della Lente Empirica (D5 + A5 + D6)
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
    // Risonanza d'arco in pietra + soffio d'aria
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
    // Arpeggio ascendente a 3 campane di giada (D5 -> F#5 -> D6)
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
    // Singolo segnale d'interazione caldo in legno/ottone (NON voce/dialogo)
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
    // Accordo armonico Dmaj9 (D4, A4, C#5, E5, F#5) luminoso e bilanciato
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
    // Accensione grave della fornace a pece
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
            std::sin(kTwoPi * 155.56f * t) * 0.16f; // leggera dissonanza scura
        out[static_cast<std::size_t>(i)] =
            (rumble * 0.55f + lp * 0.95f) * std::exp(-t * 1.9f) * env * 0.58f;
    }
    return out;
}

[[nodiscard]] std::vector<float> SynthBridgeUnlock() {
    // Risonanza monumentale del Ponte degli Archi
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
    // Soave eco di pietra al passaggio di distretto
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
    // Scatto ottico tra Sguardo Empirico / Rilievo Metrico / Trama Geodetica
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
    // Grande accordo armonico dell'Astrolabio Centrale
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

// --- Tappeti ambientali loopabili perfettamente continui (4.0 secondi, cicli interi) ---

enum class AmbientLayer : int {
    OverworldClean = 0,
    OverworldSmog,
    Cisterns,
    Tower,
    Geothermal,
    Count
};

[[nodiscard]] std::vector<float> SynthAmbientLoop(AmbientLayer layer) {
    constexpr float kDur = 4.0f;
    const int n = static_cast<int>(kDur * kSampleRate);
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    FastRng rng{0xA000u + static_cast<std::uint32_t>(layer) * 97u};

    // Frequenze arrotondate a multipli di 1/kDur (0.25 Hz) per continuita' perfetta di fase sul loop
    auto snap = [](float hz) noexcept -> float {
        return std::round(hz * kDur) / kDur;
    };

    std::vector<float> noise(static_cast<std::size_t>(n), 0.0f);
    float lp1 = 0.0f;
    float lp2 = 0.0f;
    const float alpha = (layer == AmbientLayer::Cisterns) ? 0.032f
                        : (layer == AmbientLayer::Geothermal) ? 0.022f
                        : (layer == AmbientLayer::OverworldSmog) ? 0.018f
                                                                  : 0.014f;
    for (int i = 0; i < n; ++i) {
        const float r = rng.NextSigned();
        lp1 += alpha * (r - lp1);
        lp2 += alpha * (lp1 - lp2);
        noise[static_cast<std::size_t>(i)] = lp2;
    }
    // Crossfade coda->testa di 0.35s per rendere anche la componente di rumore 100% senza giunture
    const int xf = static_cast<int>(0.35f * kSampleRate);
    for (int i = 0; i < xf; ++i) {
        const float w = static_cast<float>(i) / static_cast<float>(xf);
        const std::size_t tail_idx = static_cast<std::size_t>(n - xf + i);
        noise[tail_idx] = noise[tail_idx] * (1.0f - w) + noise[static_cast<std::size_t>(i)] * w;
    }

    const float f_d2 = snap(73.42f);
    const float f_a2 = snap(110.00f);
    const float f_d3 = snap(146.83f);
    const float f_e3 = snap(164.81f);
    const float f_a3 = snap(220.00f);
    const float f_f4 = snap(369.99f);
    const float breath_hz = 0.25f; // esattamente 1 ciclo in 4s

    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float breath = 0.78f + 0.22f * std::sin(kTwoPi * breath_hz * t);
        const float nz = noise[static_cast<std::size_t>(i)];
        float sample = 0.0f;

        switch (layer) {
            case AmbientLayer::OverworldClean: {
                const float pad =
                    std::sin(kTwoPi * f_d3 * t) * 0.24f +
                    std::sin(kTwoPi * f_a3 * t) * 0.16f +
                    std::sin(kTwoPi * f_f4 * t) * 0.06f * (0.5f + 0.5f * std::sin(kTwoPi * 0.5f * t));
                sample = (pad * breath + nz * 0.55f) * 0.26f;
                break;
            }
            case AmbientLayer::OverworldSmog: {
                const float low_drone =
                    std::sin(kTwoPi * f_d2 * t) * 0.26f +
                    std::sin(kTwoPi * f_e3 * t) * 0.08f;
                sample = (low_drone * breath + nz * 0.95f) * 0.25f;
                break;
            }
            case AmbientLayer::Cisterns: {
                const float cavern =
                    std::sin(kTwoPi * f_a2 * t) * 0.22f +
                    std::sin(kTwoPi * f_d3 * t) * 0.14f;
                const float ripple = 0.80f + 0.20f * std::sin(kTwoPi * 0.75f * t);
                sample = (cavern * breath + nz * 1.25f * ripple) * 0.28f;
                break;
            }
            case AmbientLayer::Tower: {
                // Risonanza della torre + battito morbido e lento del pendolo (ogni 2.0s)
                const float hum =
                    std::sin(kTwoPi * f_d3 * t) * 0.18f +
                    std::sin(kTwoPi * f_a3 * t) * 0.10f;
                const float cycle_t = std::fmod(t, 2.0f);
                const float tick_env = std::exp(-cycle_t * 28.0f);
                const float pend_tick =
                    (std::sin(kTwoPi * 440.0f * cycle_t) * 0.18f +
                     std::sin(kTwoPi * 660.0f * cycle_t) * 0.08f) * tick_env;
                sample = (hum * breath + pend_tick + nz * 0.35f) * 0.28f;
                break;
            }
            case AmbientLayer::Geothermal: {
                const float magma =
                    std::sin(kTwoPi * f_d2 * t) * 0.32f +
                    std::sin(kTwoPi * f_a2 * t) * 0.18f;
                const float steam_mod = 0.72f + 0.28f * std::sin(kTwoPi * 0.5f * t);
                sample = (magma * breath + nz * 1.15f * steam_mod) * 0.29f;
                break;
            }
            case AmbientLayer::Count:
                break;
        }
        out[static_cast<std::size_t>(i)] = sample;
    }
    return out;
}

} // namespace

struct AudioEngine::Impl {
    std::array<Sound, static_cast<std::size_t>(SfxId::Count)> sfx{};
    std::array<Sound, static_cast<std::size_t>(AmbientLayer::Count)> ambient{};
    std::array<float, static_cast<std::size_t>(AmbientLayer::Count)> ambient_vol{};

    // Stato osservato per attivare automaticamente i suoni di interazione
    bool has_prev = false;
    bool prev_modal_open = false;
    float prev_modal_value = 0.0f;
    float slider_tick_cooldown = 0.0f;
    bool prev_pause_open = false;
    bool prev_journal_open = false;
    int prev_journal_tab = 0;
    eco::GameMode prev_mode = eco::GameMode::Overworld;
    eco::StructureId prev_structure = eco::StructureId::HypogeanCisterns;
    eco::ZoneId prev_zone = eco::ZoneId::DawnTerraces;
    eco::UiPhase prev_ui_phase = eco::UiPhase::Phase1_Minimal;
    bool prev_architect_recruited = false;
    bool prev_sim_completed = false;
    int prev_collected_count = 0;
    float prev_scan_timer = 0.0f;
    std::array<eco::MachineState, 6> prev_machine_states{};
    std::array<bool, 4> prev_npc_met{};
    std::array<bool, 4> prev_npc_recruited{};

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

    for (int i = 0; i < static_cast<int>(AmbientLayer::Count); ++i) {
        const auto layer = static_cast<AmbientLayer>(i);
        impl_->ambient[static_cast<std::size_t>(i)] = BuildSoundFromSamples(SynthAmbientLoop(layer));
        impl_->ambient_vol[static_cast<std::size_t>(i)] = 0.0f;
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
            for (Sound& a : impl_->ambient) {
                if (a.frameCount > 0) {
                    StopSound(a);
                    UnloadSound(a);
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

void AudioEngine::PlaySfx(SfxId id, float volume_scale, float pitch) {
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
    SetSoundVolume(snd, std::clamp(volume_scale, 0.0f, 1.0f));
    SetSoundPitch(snd, std::clamp(pitch, 0.65f, 1.45f));
    PlaySound(snd);
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

    // 1. Aggiornamento tappeto ambientale con crossfade morbido
    if (ready_ && IsAudioDeviceReady()) {
        std::array<float, static_cast<std::size_t>(AmbientLayer::Count)> target{};
        if (ambient_enabled_ && !muted_ && master_volume_ > 0.001f) {
            const float duck = (pause_open || journal_open) ? 0.42f : 1.0f;
            if (active_state_name == "TitleState" || active_state_name == "IntroState") {
                target[static_cast<std::size_t>(AmbientLayer::OverworldClean)] = 0.42f * duck;
            } else if (active_state_name == "OverworldState") {
                const float poll = world.ecosystem.PollutionRatio();
                target[static_cast<std::size_t>(AmbientLayer::OverworldClean)] =
                    (0.55f - 0.30f * poll) * duck;
                target[static_cast<std::size_t>(AmbientLayer::OverworldSmog)] =
                    (0.10f + 0.45f * poll) * duck;
            } else if (active_state_name == "StructureState") {
                switch (world.active_structure) {
                    case eco::StructureId::HypogeanCisterns:
                        target[static_cast<std::size_t>(AmbientLayer::Cisterns)] = 0.52f * duck;
                        break;
                    case eco::StructureId::PendulumTower:
                        target[static_cast<std::size_t>(AmbientLayer::Tower)] = 0.50f * duck;
                        break;
                    case eco::StructureId::GeothermalChamber:
                        target[static_cast<std::size_t>(AmbientLayer::Geothermal)] = 0.54f * duck;
                        break;
                    case eco::StructureId::Count:
                        break;
                }
            }
        }

        const float blend = std::min(1.0f, safe_dt * 3.5f);
        for (std::size_t i = 0; i < impl_->ambient.size(); ++i) {
            float& cur = impl_->ambient_vol[i];
            cur += (target[i] - cur) * blend;
            Sound& snd = impl_->ambient[i];
            if (snd.frameCount == 0) {
                continue;
            }
            if (cur > 0.01f) {
                SetSoundVolume(snd, cur);
                if (!IsSoundPlaying(snd)) {
                    PlaySound(snd);
                }
            } else if (IsSoundPlaying(snd)) {
                StopSound(snd);
            }
        }
    }

    const eco::Player* p0 = world.players.GetPlayer(0);

    // 2. Primo frame: fotografa lo stato iniziale senza sparare suoni spuri
    if (!impl_->has_prev) {
        impl_->has_prev = true;
        impl_->prev_modal_open = world.calibration_modal.is_open;
        impl_->prev_modal_value = world.calibration_modal.slider_value;
        impl_->prev_pause_open = pause_open;
        impl_->prev_journal_open = journal_open;
        impl_->prev_journal_tab = journal_tab_index;
        impl_->prev_mode = world.current_mode;
        impl_->prev_structure = world.active_structure;
        impl_->prev_zone = world.current_zone;
        impl_->prev_ui_phase = world.ecosystem.ui_phase;
        impl_->prev_architect_recruited = world.ecosystem.architect_recruited;
        impl_->prev_sim_completed = world.simulation_completed;
        impl_->prev_collected_count = world.collectibles.CollectedCount();
        impl_->prev_scan_timer = world.scan_pulse_timer;
        for (std::size_t i = 0; i < world.machines.size(); ++i) {
            impl_->prev_machine_states[i] = world.machines[i].state;
        }
        for (std::size_t i = 0; i < world.npcs.size(); ++i) {
            impl_->prev_npc_met[i] = world.npcs[i].met;
            impl_->prev_npc_recruited[i] = world.npcs[i].recruited;
        }
        if (p0 != nullptr) {
            impl_->prev_p0_pos = p0->pos;
            impl_->prev_p0_grounded = p0->isGrounded;
        }
        return;
    }

    // 3. Interazioni UI (Pausa, Diario, Modal di calibrazione — MAI dialoghi)
    if (pause_open != impl_->prev_pause_open) {
        PlaySfx(pause_open ? SfxId::ModalOpen : SfxId::ModalClose, 0.72f);
        impl_->prev_pause_open = pause_open;
    }
    if (journal_open != impl_->prev_journal_open) {
        PlaySfx(journal_open ? SfxId::ModalOpen : SfxId::ModalClose, 0.72f);
        impl_->prev_journal_open = journal_open;
    } else if (journal_open && journal_tab_index != impl_->prev_journal_tab) {
        PlaySfx(SfxId::UiClick, 0.65f, 1.08f);
    }
    impl_->prev_journal_tab = journal_tab_index;

    if (world.calibration_modal.is_open != impl_->prev_modal_open) {
        PlaySfx(world.calibration_modal.is_open ? SfxId::ModalOpen : SfxId::ModalClose, 0.80f);
        impl_->prev_modal_open = world.calibration_modal.is_open;
        impl_->prev_modal_value = world.calibration_modal.slider_value;
    } else if (world.calibration_modal.is_open &&
               std::abs(world.calibration_modal.slider_value - impl_->prev_modal_value) > 1e-3f) {
        if (impl_->slider_tick_cooldown <= 0.0f) {
            PlaySfx(SfxId::SliderTick, 0.55f, 0.96f + 0.08f * impl_->pitch_rng.NextSigned());
            impl_->slider_tick_cooldown = 0.048f;
        }
        impl_->prev_modal_value = world.calibration_modal.slider_value;
    }

    // 4. Eventi fisici e di mondo (solo in gameplay attivo)
    if (world.scan_pulse_timer > impl_->prev_scan_timer + 0.1f) {
        PlaySfx(SfxId::ScanPulse, 0.85f);
    }
    impl_->prev_scan_timer = world.scan_pulse_timer;

    const int collected = world.collectibles.CollectedCount();
    if (collected > impl_->prev_collected_count) {
        PlaySfx(SfxId::RelicCollect, 0.88f);
    }
    impl_->prev_collected_count = collected;

    for (std::size_t i = 0; i < world.machines.size(); ++i) {
        const eco::MachineState st = world.machines[i].state;
        if (st != impl_->prev_machine_states[i]) {
            if (st == eco::MachineState::CleanActive) {
                PlaySfx(SfxId::CalibrateClean, 0.90f);
            } else if (st == eco::MachineState::BypassedDirty) {
                PlaySfx(SfxId::CalibrateBypass, 0.86f);
            }
            impl_->prev_machine_states[i] = st;
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
        PlaySfx(SfxId::ZoneDiscover, 0.62f);
        impl_->prev_zone = world.current_zone;
    }

    // 5. Passi e Salto/Atterraggio di P0
    if (in_gameplay && !pause_open && !journal_open && !world.calibration_modal.is_open &&
        p0 != nullptr && p0->active) {
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
                                     tid == eco::TileId::StructurePortal);
                }
                const float pitch = 0.92f + 0.10f * impl_->pitch_rng.NextSigned();
                PlaySfx(stone_surface ? SfxId::FootstepStone : SfxId::FootstepGrass, 0.48f, pitch);
            }
        } else if (dist >= 48.0f) {
            // Teletrasporto / cambio scena: resetta l'accumulatore
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
        g_audio_engine->PlaySfx(id, volume_scale, pitch);
    }
}

void ApplyGlobalAudioSettings(float master_volume, bool muted, bool ambient_enabled) noexcept {
    if (g_audio_engine != nullptr) {
        g_audio_engine->SetMasterVolume(master_volume);
        g_audio_engine->SetMuted(muted);
        g_audio_engine->SetAmbientEnabled(ambient_enabled);
    }
}

} // namespace eco::app
