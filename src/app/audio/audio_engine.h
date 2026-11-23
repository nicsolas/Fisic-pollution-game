#pragma once

#include <string_view>

#include "core/simulation.h"

namespace eco::app {

enum class SfxId : int {
    FootstepStone = 0,
    FootstepGrass,
    Jump,
    Land,
    ScanPulse,
    PortalEnter,
    RelicCollect,
    NpcInteract,
    ModalOpen,
    ModalClose,
    SliderTick,
    CalibrateClean,
    CalibrateBypass,
    BridgeUnlock,
    ZoneDiscover,
    PhaseShift,
    UiClick,
    EpilogueChord,
    // Nuovi suoni procedurali per le 6 meccaniche top-down e gli intramezzi
    SolarMirrorRotate,
    SolarPlateUnlock,
    SluiceLeverToggle,
    WindGustWhoosh,
    ResonancePadNote,
    ResonanceSequenceFail,
    JadeBridgeUnlock,
    GeyserErupt,
    ValveClose,
    SanctuarySealTune,
    InterludeChime,
    Count
};

enum class AudioPreviewChannel : int {
    Master = 0,
    Music = 1,
    Effects = 2,
    Ambient = 3,
    Ui = 4
};

// Motore audio procedurale (solo Raylib/miniaudio lato src/app, zero dipendenze in src/core).
// Sintetizza suoni ambientali, meccaniche di zona e interazioni senza mai riprodurre voci sui dialoghi.
class AudioEngine {
public:
    AudioEngine() = default;
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    // Inizializza il dispositivo audio e sintetizza tutte le forme d'onda procedurali.
    // Se silent_mode == true o il dispositivo non e' disponibile, opera in sicurezza come no-op.
    void Init(bool silent_mode = false);
    void Shutdown();

    [[nodiscard]] bool IsReady() const noexcept { return ready_; }

    void SetMasterVolume(float volume) noexcept;  // 0.0f .. 1.0f
    void SetMuted(bool muted) noexcept;
    void SetAmbientEnabled(bool enabled) noexcept;
    void SetMusicVolume(float volume) noexcept;   // 0.0f .. 1.0f
    void SetEffectsVolume(float volume) noexcept; // 0.0f .. 1.0f
    void SetAmbientVolume(float volume) noexcept; // 0.0f .. 1.0f
    void SetUiSoundsEnabled(bool enabled) noexcept;

    [[nodiscard]] float MasterVolume() const noexcept { return master_volume_; }
    [[nodiscard]] bool IsMuted() const noexcept { return muted_; }
    [[nodiscard]] bool IsAmbientEnabled() const noexcept { return ambient_enabled_; }
    [[nodiscard]] float MusicVolume() const noexcept { return music_volume_; }
    [[nodiscard]] float EffectsVolume() const noexcept { return effects_volume_; }
    [[nodiscard]] float AmbientVolume() const noexcept { return ambient_volume_; }
    [[nodiscard]] bool IsUiSoundsEnabled() const noexcept { return ui_sounds_enabled_; }

    void PlaySfx(SfxId id, float volume_scale = 1.0f, float pitch = 1.0f);
    void PlayUiSound(SfxId id, float volume_scale = 1.0f, float pitch = 1.0f);
    void PlayLivePreview(AudioPreviewChannel channel, float pitch = 1.0f);

    // Osserva le variazioni di stato di mondo, meccaniche di zona, intramezzi, modal, diario e menu
    // per attivare suoni di interazione e modulare il tappeto musicale/ambientale con crossfade morbido.
    // Non genera mai alcun suono per l'avanzamento o la scrittura dei dialoghi.
    void SyncAndUpdate(
        float dt,
        const eco::WorldContext& world,
        std::string_view active_state_name,
        bool pause_open,
        bool journal_open,
        int journal_tab_index);

private:
    struct Impl;
    Impl* impl_ = nullptr;

    bool ready_ = false;
    bool initialized_device_ = false;
    float master_volume_ = 0.75f;
    bool muted_ = false;
    bool ambient_enabled_ = true;
    float music_volume_ = 0.85f;
    float effects_volume_ = 0.90f;
    float ambient_volume_ = 0.85f;
    bool ui_sounds_enabled_ = true;
};

// Accesso leggero per piccoli suoni di interazione UI, anteprime live e sincronizzazione impostazioni audio.
void SetGlobalAudioEngine(AudioEngine* engine) noexcept;
void PlayUiSfx(SfxId id, float volume_scale = 1.0f, float pitch = 1.0f) noexcept;
void PlayAudioPreview(AudioPreviewChannel channel, float pitch = 1.0f) noexcept;
void ApplyGlobalAudioSettings(
    float master_volume,
    bool muted,
    bool ambient_enabled,
    float music_volume,
    float effects_volume,
    float ambient_volume = 0.85f,
    bool ui_sounds_enabled = true) noexcept;

} // namespace eco::app
