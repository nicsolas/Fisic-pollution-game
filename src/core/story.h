#pragma once

#include <array>
#include <string>
#include <vector>

#include "core/math_types.h"

namespace eco {

enum class Chapter : int {
    Prologo = 0,
    Atto1_Luce = 1,
    Atto2_Acqua = 2,
    Atto3_Pendolo = 3,
    Atto4_Giardini = 4,
    Atto5_Fumarole = 5,
    Epilogo = 6
};

enum class StoryEvent : int {
    GameStarted = 0,
    FirstScan,
    SolarClean,
    SolarBypassed,
    ClaraRecruited,
    EnteredCanalQuarter,
    EnteredWindPlateau,
    EnteredHangingGardens,
    EnteredFumaroleCaldera,
    EnteredHarmonySanctuary,
    EnteredCisterns,
    EnteredPendulumTower,
    EnteredGeothermalChamber,
    HydroClean,
    HydroBypassed,
    LorenzoRecruited,
    PendulumClean,
    PendulumBypassed,
    MiriamMet,
    HarmonicClean,
    HarmonicBypassed,
    MatteoConfronted,
    ThermalClean,
    ThermalBypassed,
    AstrolabeUnlocked,
    AstrolabeActivated,
    SmogCritical,
    FirstRelic,
    FourRelics,
    HalfRelics,
    TwelveRelics,
    AllRelics,
    ObservationClueRead,
    SolarTrialRun,
    SolarRisky,
    HydroTrialRun,
    HydroRisky,
    BioPlanterSeeded,
    LaminarFordOpened,
    VerticalSliceSynthesis,
    Count
};

struct DialogueLine {
    std::string speaker{};
    std::string text{};
};

struct DialogueHistoryEntry {
    int sequence_id = 0;
    Chapter chapter = Chapter::Prologo;
    StoryEvent event = StoryEvent::GameStarted;
    bool is_ad_hoc = false;
    ZoneId zone = ZoneId::DawnTerraces;
    std::string location_name{};
    std::string speaker{};
    std::string text{};
};

struct Objective {
    std::string title{};
    std::string hint{};
};

struct ZoneInterludeText {
    const char* threshold_title = "";
    const char* direction_tag = "";
    const char* line1 = "";
    const char* line2 = "";
};

struct ZoneLandmarkCopy {
    const char* id = "";
    const char* title = "";
    const char* inscription = "";
    const char* mechanic_hint = "";
};

struct StorySnapshot {
    GameMode mode = GameMode::Overworld;
    LocationId location = LocationId::Overworld;
    ZoneId zone = ZoneId::DawnTerraces;
    bool has_scanned = false;
    MachineState solar = MachineState::Broken;
    MachineState hydro = MachineState::Broken;
    MachineState pendulum = MachineState::Broken;
    MachineState harmonic = MachineState::Broken;
    MachineState thermal = MachineState::Broken;
    MachineState astrolabe = MachineState::Broken;
    bool clara_recruited = false;
    bool lorenzo_recruited = false;
    bool miriam_met = false;
    bool matteo_confronted = false;
    bool astrolabe_unlocked = false;
    int relics_collected = 0;
    int total_relics = 16;
    float pollution_level = kInitialPollution;
    bool simulation_completed = false;
    CalibrationApproach solar_approach = CalibrationApproach::None;
    CalibrationApproach hydro_approach = CalibrationApproach::None;
    InquiryStage inquiry_stage = InquiryStage::Observe;
    int observations_inspected = 0;
    int trials_completed = 0;
    bool white_olive_planted = false;
    bool laminar_ford_open = false;
    bool slice_completed = false;
    bool has_brass_gnomon = false;
    bool has_first_measure_stele = false;
    bool has_white_olive_seed = false;
    bool has_hydraulic_diary = false;
    bool has_sluice_stele = false;
    bool has_farewell_prism = false;
};

// --- Contenuti narrativi (src/core/story_content.cpp, proprietario: @narratore) ---
[[nodiscard]] const char* ChapterTitle(Chapter c);
[[nodiscard]] std::vector<DialogueLine> DialogueForEvent(StoryEvent e, const StorySnapshot& s);
[[nodiscard]] Objective ObjectiveFor(Chapter c, const StorySnapshot& s);
[[nodiscard]] std::vector<std::string> IntroCrawl();
[[nodiscard]] std::vector<std::string> EndingText(
    bool clean_path, int relics_found, int total_relics);
[[nodiscard]] ZoneInterludeText ZoneInterludeCopy(ZoneId from, ZoneId to) noexcept;
[[nodiscard]] std::vector<ZoneLandmarkCopy> ZoneLandmarks(ZoneId zone);

// --- Logica narrativa e Cronologia Dialoghi (src/core/story.cpp, proprietario: @ingegnere-1) ---
[[nodiscard]] Chapter ChapterFor(const StorySnapshot& s);

class StoryDirector {
public:
    void Reset();
    void Observe(const StorySnapshot& s);
    void Notify(StoryEvent e, const StorySnapshot& s);
    void PushAdHocLine(
        const std::string& speaker,
        const std::string& text,
        ZoneId zone,
        const std::string& location_name);
    void Tick(float dt);
    void AdvanceDialogue();

    [[nodiscard]] bool HasActiveDialogue() const noexcept;
    [[nodiscard]] const DialogueLine* CurrentLine() const noexcept;
    [[nodiscard]] float LineElapsed() const noexcept;
    void SetTypewriterCharactersPerSecond(float characters_per_second) noexcept;
    [[nodiscard]] float TypewriterCharactersPerSecond() const noexcept;
    [[nodiscard]] Chapter CurrentChapter() const noexcept;
    [[nodiscard]] Objective CurrentObjective() const;
    [[nodiscard]] bool EventFired(StoryEvent e) const noexcept;
    [[nodiscard]] bool CleanPath() const noexcept;
    [[nodiscard]] int QueuedLineCount() const noexcept;
    [[nodiscard]] const std::vector<DialogueHistoryEntry>& History() const noexcept {
        return history_;
    }
    [[nodiscard]] const std::array<bool, static_cast<int>(StoryEvent::Count)>& FiredFlags() const noexcept {
        return fired_;
    }
    void RestoreSavedState(
        const std::array<bool, static_cast<int>(StoryEvent::Count)>& fired,
        std::vector<DialogueHistoryEntry> history,
        const StorySnapshot& current_snapshot);

private:
    struct QueuedEntry {
        DialogueLine line{};
        Chapter chapter = Chapter::Prologo;
        StoryEvent event = StoryEvent::GameStarted;
        bool is_ad_hoc = false;
        ZoneId zone = ZoneId::DawnTerraces;
        std::string location_name{};
        bool recorded = false;
    };

    void RecordFrontIfNeeded();

    std::array<bool, static_cast<int>(StoryEvent::Count)> fired_{};
    std::vector<QueuedEntry> queue_{};
    std::vector<DialogueHistoryEntry> history_{};
    int next_sequence_id_ = 1;
    float line_elapsed_ = 0.0f;
    StorySnapshot last_{};
    bool has_last_ = false;
    float typewriter_characters_per_second_ = 58.0f;
    Chapter chapter_ = Chapter::Prologo;
};

} // namespace eco
