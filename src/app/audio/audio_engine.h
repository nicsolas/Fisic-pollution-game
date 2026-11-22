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
    Count
};

// Motore audio procedurale (solo Raylib/miniaudio lato src/app, zero dipendenze in src/core).
// Sintetizza suoni ambientali e di interazione senza mai riprodurre voci o suoni sui dialoghi.
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

    void SetMasterVolume(float volume) noexcept; // 0.0f .. 1.0f
    void SetMuted(bool muted) noexcept;
    void SetAmbientEnabled(bool enabled) noexcept;

    [[nodiscard]] float MasterVolume() const noexcept { return master_volume_; }
    [[nodiscard]] bool IsMuted() const noexcept { return muted_; }
    [[nodiscard]] bool IsAmbientEnabled() const noexcept { return ambient_enabled_; }

    void PlaySfx(SfxId id, float volume_scale = 1.0f, float pitch = 1.0f);

    // Osserva le variazioni di stato di mondo, modal, diario e menu per attivare suoni di
    // interazione e modulare il tappeto ambientale in base a scena, struttura e foschia.
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
};

// Accesso leggero per piccoli suoni di interazione UI e sincronizzazione impostazioni audio.
void SetGlobalAudioEngine(AudioEngine* engine) noexcept;
void PlayUiSfx(SfxId id, float volume_scale = 1.0f, float pitch = 1.0f) noexcept;
void ApplyGlobalAudioSettings(float master_volume, bool muted, bool ambient_enabled) noexcept;

} // namespace eco::app
