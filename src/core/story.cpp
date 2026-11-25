#include "core/story.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace eco {

namespace {

[[nodiscard]] constexpr bool IsValidStoryEvent(StoryEvent e) noexcept {
    const int idx = static_cast<int>(e);
    return idx >= 0 && idx < static_cast<int>(StoryEvent::Count);
}

[[nodiscard]] float ComputeLineDuration(const DialogueLine& line) noexcept {
    return 2.8f + 0.045f * static_cast<float>(line.text.size());
}

[[nodiscard]] bool IsActivated(MachineState state) noexcept {
    return state != MachineState::Broken;
}

} // namespace

Chapter ChapterFor(const StorySnapshot& s) {
    if (s.simulation_completed || IsActivated(s.astrolabe)) {
        return Chapter::Epilogo;
    }
    if (s.astrolabe_unlocked || IsActivated(s.thermal)) {
        return Chapter::Epilogo;
    }
    if (IsActivated(s.harmonic)) {
        return Chapter::Atto5_Fumarole;
    }
    if (s.clara_recruited && IsActivated(s.pendulum)) {
        return Chapter::Atto4_Giardini;
    }
    if (IsActivated(s.hydro)) {
        return Chapter::Atto3_Pendolo;
    }
    if (IsActivated(s.solar)) {
        return Chapter::Atto2_Acqua;
    }
    if (s.has_scanned) {
        return Chapter::Atto1_Luce;
    }
    return Chapter::Prologo;
}

void StoryDirector::Reset() {
    fired_.fill(false);
    queue_.clear();
    history_.clear();
    next_sequence_id_ = 1;
    line_elapsed_ = 0.0f;
    last_ = StorySnapshot{};
    has_last_ = false;
    chapter_ = Chapter::Prologo;
}

void StoryDirector::RestoreSavedState(
    const std::array<bool, static_cast<int>(StoryEvent::Count)>& fired,
    std::vector<DialogueHistoryEntry> history,
    const StorySnapshot& current_snapshot) {
    fired_ = fired;
    queue_.clear();
    history_ = std::move(history);
    int max_seq = 0;
    for (const auto& entry : history_) {
        max_seq = std::max(max_seq, entry.sequence_id);
    }
    next_sequence_id_ = max_seq + 1;
    line_elapsed_ = 0.0f;
    last_ = current_snapshot;
    has_last_ = true;
    chapter_ = ChapterFor(current_snapshot);
}

void StoryDirector::RecordFrontIfNeeded() {
    if (queue_.empty() || queue_.front().recorded) {
        return;
    }
    QueuedEntry& front = queue_.front();
    front.recorded = true;
    DialogueHistoryEntry entry{};
    entry.sequence_id = next_sequence_id_++;
    entry.chapter = front.chapter;
    entry.event = front.event;
    entry.is_ad_hoc = front.is_ad_hoc;
    entry.zone = front.zone;
    entry.location_name = front.location_name;
    entry.speaker = front.line.speaker;
    entry.text = front.line.text;
    history_.push_back(std::move(entry));
}

void StoryDirector::Notify(StoryEvent e, const StorySnapshot& s) {
    if (!IsValidStoryEvent(e)) {
        return;
    }
    const auto idx = static_cast<std::size_t>(e);
    if (fired_[idx]) {
        return;
    }
    fired_[idx] = true;

    const bool was_empty = queue_.empty();
    const Chapter snap_chapter = ChapterFor(s);
    const std::string loc_name = LocationName(s.location, s.zone);
    std::vector<DialogueLine> lines = DialogueForEvent(e, s);
    if (lines.empty()) {
        lines.push_back(DialogueLine{
            "Elena Valli",
            "Annoto l'osservazione sul taccuino da campo per verificare l'ipotesi."
        });
    }
    for (auto& line : lines) {
        if (!line.text.empty()) {
            QueuedEntry qe{};
            qe.line = std::move(line);
            qe.chapter = snap_chapter;
            qe.event = e;
            qe.is_ad_hoc = false;
            qe.zone = s.zone;
            qe.location_name = loc_name;
            qe.recorded = false;
            queue_.push_back(std::move(qe));
        }
    }
    if (was_empty && !queue_.empty()) {
        line_elapsed_ = 0.0f;
        RecordFrontIfNeeded();
    }
}

void StoryDirector::PushAdHocLine(
    const std::string& speaker,
    const std::string& text,
    ZoneId zone,
    const std::string& location_name) {
    if (text.empty()) {
        return;
    }
    const bool was_empty = queue_.empty();
    QueuedEntry qe{};
    qe.line = DialogueLine{speaker, text};
    qe.chapter = chapter_;
    qe.event = StoryEvent::GameStarted;
    qe.is_ad_hoc = true;
    qe.zone = zone;
    qe.location_name = location_name.empty() ? std::string(ZoneName(zone)) : location_name;
    qe.recorded = false;
    queue_.push_back(std::move(qe));
    if (was_empty) {
        line_elapsed_ = 0.0f;
        RecordFrontIfNeeded();
    }
}

void StoryDirector::Observe(const StorySnapshot& s) {
    chapter_ = ChapterFor(s);

    if (!has_last_) {
        has_last_ = true;
        last_ = s;
        Notify(StoryEvent::GameStarted, s);
    } else {
        last_ = s;
    }

    // Controlli idempotenti su stato, zone, strutture, monumenti, NPC e reliquie
    if (s.has_scanned) {
        Notify(StoryEvent::FirstScan, s);
    }

    if (s.observations_inspected >= 1) {
        Notify(StoryEvent::ObservationClueRead, s);
    }

    if (s.solar_approach == CalibrationApproach::RiskyEfficient) {
        Notify(StoryEvent::SolarRisky, s);
    } else if (s.solar == MachineState::CleanActive) {
        Notify(StoryEvent::SolarClean, s);
    } else if (s.solar == MachineState::BypassedDirty) {
        Notify(StoryEvent::SolarBypassed, s);
    }

    if (s.clara_recruited) {
        Notify(StoryEvent::ClaraRecruited, s);
    }

    // Ingressi nei distretti Overworld
    if (s.location == LocationId::Overworld) {
        switch (s.zone) {
            case ZoneId::CanalQuarter:
                Notify(StoryEvent::EnteredCanalQuarter, s);
                break;
            case ZoneId::WindPlateau:
                Notify(StoryEvent::EnteredWindPlateau, s);
                break;
            case ZoneId::HangingGardens:
                Notify(StoryEvent::EnteredHangingGardens, s);
                break;
            case ZoneId::FumaroleCaldera:
                Notify(StoryEvent::EnteredFumaroleCaldera, s);
                break;
            case ZoneId::HarmonySanctuary:
                Notify(StoryEvent::EnteredHarmonySanctuary, s);
                break;
            case ZoneId::DawnTerraces:
            case ZoneId::Count:
                break;
        }
    } else {
        switch (s.location) {
            case LocationId::HypogeanCisterns:
                Notify(StoryEvent::EnteredCisterns, s);
                break;
            case LocationId::PendulumTower:
                Notify(StoryEvent::EnteredPendulumTower, s);
                break;
            case LocationId::GeothermalChamber:
                Notify(StoryEvent::EnteredGeothermalChamber, s);
                break;
            case LocationId::Overworld:
                break;
        }
    }

    if (s.hydro_approach == CalibrationApproach::RiskyEfficient) {
        Notify(StoryEvent::HydroRisky, s);
    } else if (s.hydro == MachineState::CleanActive) {
        Notify(StoryEvent::HydroClean, s);
    } else if (s.hydro == MachineState::BypassedDirty) {
        Notify(StoryEvent::HydroBypassed, s);
    }

    if (s.white_olive_planted) {
        Notify(StoryEvent::BioPlanterSeeded, s);
    }

    if (s.laminar_ford_open) {
        Notify(StoryEvent::LaminarFordOpened, s);
    }

    if (s.slice_completed) {
        Notify(StoryEvent::VerticalSliceSynthesis, s);
    }

    if (s.lorenzo_recruited) {
        Notify(StoryEvent::LorenzoRecruited, s);
    }

    if (s.pendulum == MachineState::CleanActive) {
        Notify(StoryEvent::PendulumClean, s);
    } else if (s.pendulum == MachineState::BypassedDirty) {
        Notify(StoryEvent::PendulumBypassed, s);
    }

    if (s.miriam_met) {
        Notify(StoryEvent::MiriamMet, s);
    }

    if (s.harmonic == MachineState::CleanActive) {
        Notify(StoryEvent::HarmonicClean, s);
    } else if (s.harmonic == MachineState::BypassedDirty) {
        Notify(StoryEvent::HarmonicBypassed, s);
    }

    if (s.matteo_confronted) {
        Notify(StoryEvent::MatteoConfronted, s);
    }

    if (s.thermal == MachineState::CleanActive) {
        Notify(StoryEvent::ThermalClean, s);
    } else if (s.thermal == MachineState::BypassedDirty) {
        Notify(StoryEvent::ThermalBypassed, s);
    }

    if (s.astrolabe_unlocked) {
        Notify(StoryEvent::AstrolabeUnlocked, s);
    }

    if (IsActivated(s.astrolabe) || s.simulation_completed) {
        Notify(StoryEvent::AstrolabeActivated, s);
    }

    if (std::isfinite(s.pollution_level) && s.pollution_level >= 70.0f) {
        Notify(StoryEvent::SmogCritical, s);
    }

    // Traguardi Collezionabili
    if (s.relics_collected >= 1) {
        Notify(StoryEvent::FirstRelic, s);
    }
    if (s.relics_collected >= 4) {
        Notify(StoryEvent::FourRelics, s);
    }
    if (s.relics_collected >= 8) {
        Notify(StoryEvent::HalfRelics, s);
    }
    if (s.relics_collected >= 12) {
        Notify(StoryEvent::TwelveRelics, s);
    }
    if (s.total_relics > 0 && s.relics_collected >= s.total_relics) {
        Notify(StoryEvent::AllRelics, s);
    }
}

void StoryDirector::Tick(float dt) {
    if (queue_.empty() || !std::isfinite(dt) || dt <= 0.0f) {
        return;
    }
    RecordFrontIfNeeded();
    line_elapsed_ += dt;
    while (!queue_.empty()) {
        const float duration = ComputeLineDuration(queue_.front().line);
        if (line_elapsed_ < duration) {
            break;
        }
        line_elapsed_ -= duration;
        queue_.erase(queue_.begin());
        RecordFrontIfNeeded();
    }
    if (queue_.empty()) {
        line_elapsed_ = 0.0f;
    }
}

void StoryDirector::AdvanceDialogue() {
    if (queue_.empty()) {
        line_elapsed_ = 0.0f;
        return;
    }
    RecordFrontIfNeeded();
    queue_.erase(queue_.begin());
    line_elapsed_ = 0.0f;
    RecordFrontIfNeeded();
}

bool StoryDirector::HasActiveDialogue() const noexcept {
    return !queue_.empty();
}

const DialogueLine* StoryDirector::CurrentLine() const noexcept {
    if (queue_.empty()) {
        return nullptr;
    }
    return &queue_.front().line;
}

float StoryDirector::LineElapsed() const noexcept {
    return line_elapsed_;
}

void StoryDirector::SetTypewriterCharactersPerSecond(float characters_per_second) noexcept {
    typewriter_characters_per_second_ =
        std::isfinite(characters_per_second) ? std::clamp(characters_per_second, 20.0f, 120.0f) : 58.0f;
}

float StoryDirector::TypewriterCharactersPerSecond() const noexcept {
    return typewriter_characters_per_second_;
}

Chapter StoryDirector::CurrentChapter() const noexcept {
    return chapter_;
}

Objective StoryDirector::CurrentObjective() const {
    return ObjectiveFor(chapter_, last_);
}

bool StoryDirector::EventFired(StoryEvent e) const noexcept {
    if (!IsValidStoryEvent(e)) {
        return false;
    }
    return fired_[static_cast<std::size_t>(e)];
}

bool StoryDirector::CleanPath() const noexcept {
    if (has_last_) {
        return last_.solar != MachineState::BypassedDirty &&
               last_.hydro != MachineState::BypassedDirty &&
               last_.pendulum != MachineState::BypassedDirty &&
               last_.harmonic != MachineState::BypassedDirty &&
               last_.thermal != MachineState::BypassedDirty &&
               last_.astrolabe != MachineState::BypassedDirty;
    }
    return !EventFired(StoryEvent::SolarBypassed) &&
           !EventFired(StoryEvent::HydroBypassed) &&
           !EventFired(StoryEvent::PendulumBypassed) &&
           !EventFired(StoryEvent::HarmonicBypassed) &&
           !EventFired(StoryEvent::ThermalBypassed);
}

int StoryDirector::QueuedLineCount() const noexcept {
    return static_cast<int>(queue_.size());
}

} // namespace eco
