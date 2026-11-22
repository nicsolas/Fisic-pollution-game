#pragma once
#include <array>
#include <string>
#include <vector>
#include "core/math_types.h"   // GameMode, MachineState
namespace eco {
enum class Chapter : int {
    Prologo = 0,
    Atto1_Luce,
    Atto2_Acqua,
    Atto3_Codice,
    Atto4_Archivio,
    Atto5_Inverno,
    Epilogo
};
enum class StoryEvent : int {
    GameStarted = 0,
    FirstScan,
    SolarClean,
    SolarBypassed,
    ArchitectRecruited,
    EnteredDam,
    HydroClean,
    HydroBypassed,
    PurifierClean,
    PurifierBypassed,
    SmogCritical,
    RosaRecruited,
    ThermalClean,
    ThermalBypassed,
    GaiaTerminalDecoded,
    Count
};
struct DialogueLine { std::string speaker; std::string text; };
struct Objective { std::string title; std::string hint; };
struct StorySnapshot {
    GameMode mode = GameMode::Overworld;
    bool has_scanned = false;
    MachineState solar = MachineState::Broken, hydro = MachineState::Broken, purifier = MachineState::Broken;
    bool architect_recruited = false;
    float pollution_level = 0.0f;
    bool simulation_completed = false;
    bool rosa_recruited = false;
    MachineState thermal = MachineState::Broken;
    bool gaia_terminal_unlocked = false;
    bool demo_mode = true; // true = Demo Atti I-III (retrocompatibile con test v0.1/v0.2); false = Campagna Completa Atti I-V
};
// --- Contenuti (story_content.cpp, proprietario: narratore) ---
[[nodiscard]] const char* ChapterTitle(Chapter c);
[[nodiscard]] std::vector<DialogueLine> DialogueForEvent(StoryEvent e, const StorySnapshot& s);
[[nodiscard]] Objective ObjectiveFor(Chapter c, const StorySnapshot& s);
[[nodiscard]] std::vector<std::string> IntroCrawl();
[[nodiscard]] std::vector<std::string> EndingText(bool clean_path);
// --- Logica (story.cpp, proprietario: ingegnere) ---
[[nodiscard]] Chapter ChapterFor(const StorySnapshot& s);
class StoryDirector {
public:
    void Reset();
    void Observe(const StorySnapshot& s);   // confronta con lo snapshot precedente, emette eventi (ognuno una sola volta)
    void Notify(StoryEvent e, const StorySnapshot& s); // accoda DialogueForEvent(e) se l'evento non e' gia' scattato
    void Tick(float dt);                    // auto-avanzamento: durata = 2.5 s + 0.045 s per carattere
    void AdvanceDialogue();
    [[nodiscard]] bool HasActiveDialogue() const noexcept;
    [[nodiscard]] const DialogueLine* CurrentLine() const noexcept;
    [[nodiscard]] float LineElapsed() const noexcept;   // per l'effetto macchina da scrivere nella UI
    [[nodiscard]] Chapter CurrentChapter() const noexcept;
    [[nodiscard]] Objective CurrentObjective() const;
    [[nodiscard]] bool EventFired(StoryEvent e) const noexcept;
    [[nodiscard]] bool CleanPath() const noexcept;      // false se e' scattato un qualsiasi evento *Bypassed
private:
    std::array<bool, static_cast<int>(StoryEvent::Count)> fired_{};
    std::vector<DialogueLine> queue_{};
    float line_elapsed_ = 0.0f;
    StorySnapshot last_{};
    bool has_last_ = false;
    Chapter chapter_ = Chapter::Prologo;
};
}
