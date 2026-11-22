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
    return 2.5f + 0.045f * static_cast<float>(line.text.size());
}

} // namespace

Chapter ChapterFor(const StorySnapshot& s) {
    if (s.simulation_completed) {
        return Chapter::Epilogo;
    }
    if (!s.demo_mode) {
        if (s.rosa_recruited) {
            return Chapter::Atto5_Inverno;
        }
        if (s.architect_recruited && s.hydro != MachineState::Broken && s.purifier != MachineState::Broken) {
            return Chapter::Atto4_Archivio;
        }
    }
    if (s.architect_recruited && s.hydro != MachineState::Broken) {
        return Chapter::Atto3_Codice;
    }
    if (s.architect_recruited) {
        return Chapter::Atto2_Acqua;
    }
    if (s.has_scanned || s.solar != MachineState::Broken) {
        return Chapter::Atto1_Luce;
    }
    return Chapter::Prologo;
}

void StoryDirector::Reset() {
    fired_.fill(false);
    queue_.clear();
    line_elapsed_ = 0.0f;
    last_ = StorySnapshot{};
    has_last_ = false;
    chapter_ = Chapter::Prologo;
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
    std::vector<DialogueLine> lines = DialogueForEvent(e, s);
    for (auto& line : lines) {
        if (!line.text.empty()) {
            queue_.push_back(std::move(line));
        }
    }
    if (was_empty && !queue_.empty()) {
        line_elapsed_ = 0.0f;
    }
}

void StoryDirector::Observe(const StorySnapshot& s) {
    chapter_ = ChapterFor(s);

    if (!has_last_) {
        has_last_ = true;
        last_ = s;
        Notify(StoryEvent::GameStarted, s);
    } else {
        if (!last_.has_scanned && s.has_scanned) {
            Notify(StoryEvent::FirstScan, s);
        }
        if (last_.solar != s.solar) {
            if (s.solar == MachineState::CleanActive) {
                Notify(StoryEvent::SolarClean, s);
            } else if (s.solar == MachineState::BypassedDirty) {
                Notify(StoryEvent::SolarBypassed, s);
            }
        }
        if (!last_.architect_recruited && s.architect_recruited) {
            Notify(StoryEvent::ArchitectRecruited, s);
        }
        if (last_.mode != GameMode::Structure && s.mode == GameMode::Structure) {
            Notify(StoryEvent::EnteredDam, s);
        }
        if (last_.hydro != s.hydro) {
            if (s.hydro == MachineState::CleanActive) {
                Notify(StoryEvent::HydroClean, s);
            } else if (s.hydro == MachineState::BypassedDirty) {
                Notify(StoryEvent::HydroBypassed, s);
            }
        }
        if (last_.purifier != s.purifier) {
            if (s.purifier == MachineState::CleanActive) {
                Notify(StoryEvent::PurifierClean, s);
            } else if (s.purifier == MachineState::BypassedDirty) {
                Notify(StoryEvent::PurifierBypassed, s);
            }
        }
        if (!last_.rosa_recruited && s.rosa_recruited) {
            Notify(StoryEvent::RosaRecruited, s);
        }
        if (last_.thermal != s.thermal) {
            if (s.thermal == MachineState::CleanActive) {
                Notify(StoryEvent::ThermalClean, s);
            } else if (s.thermal == MachineState::BypassedDirty) {
                Notify(StoryEvent::ThermalBypassed, s);
            }
        }
        if (!last_.gaia_terminal_unlocked && s.gaia_terminal_unlocked) {
            Notify(StoryEvent::GaiaTerminalDecoded, s);
        }
        last_ = s;
    }

    // Controlli idempotenti basati su stato/soglia
    if (s.has_scanned && !EventFired(StoryEvent::FirstScan)) {
        Notify(StoryEvent::FirstScan, s);
    }
    if (s.solar == MachineState::CleanActive && !EventFired(StoryEvent::SolarClean)) {
        Notify(StoryEvent::SolarClean, s);
    } else if (s.solar == MachineState::BypassedDirty && !EventFired(StoryEvent::SolarBypassed)) {
        Notify(StoryEvent::SolarBypassed, s);
    }
    if (s.architect_recruited && !EventFired(StoryEvent::ArchitectRecruited)) {
        Notify(StoryEvent::ArchitectRecruited, s);
    }
    if (s.mode == GameMode::Structure && !EventFired(StoryEvent::EnteredDam)) {
        Notify(StoryEvent::EnteredDam, s);
    }
    if (s.hydro == MachineState::CleanActive && !EventFired(StoryEvent::HydroClean)) {
        Notify(StoryEvent::HydroClean, s);
    } else if (s.hydro == MachineState::BypassedDirty && !EventFired(StoryEvent::HydroBypassed)) {
        Notify(StoryEvent::HydroBypassed, s);
    }
    if (s.purifier == MachineState::CleanActive && !EventFired(StoryEvent::PurifierClean)) {
        Notify(StoryEvent::PurifierClean, s);
    } else if (s.purifier == MachineState::BypassedDirty && !EventFired(StoryEvent::PurifierBypassed)) {
        Notify(StoryEvent::PurifierBypassed, s);
    }
    if (s.rosa_recruited && !EventFired(StoryEvent::RosaRecruited)) {
        Notify(StoryEvent::RosaRecruited, s);
    }
    if (s.thermal == MachineState::CleanActive && !EventFired(StoryEvent::ThermalClean)) {
        Notify(StoryEvent::ThermalClean, s);
    } else if (s.thermal == MachineState::BypassedDirty && !EventFired(StoryEvent::ThermalBypassed)) {
        Notify(StoryEvent::ThermalBypassed, s);
    }
    if (s.gaia_terminal_unlocked && !EventFired(StoryEvent::GaiaTerminalDecoded)) {
        Notify(StoryEvent::GaiaTerminalDecoded, s);
    }
    if (std::isfinite(s.pollution_level) && s.pollution_level >= 70.0f) {
        Notify(StoryEvent::SmogCritical, s);
    }
}

void StoryDirector::Tick(float dt) {
    if (queue_.empty() || !std::isfinite(dt) || dt <= 0.0f) {
        return;
    }
    line_elapsed_ += dt;
    while (!queue_.empty()) {
        const float duration = ComputeLineDuration(queue_.front());
        if (line_elapsed_ < duration) {
            break;
        }
        line_elapsed_ -= duration;
        queue_.erase(queue_.begin());
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
    queue_.erase(queue_.begin());
    line_elapsed_ = 0.0f;
}

bool StoryDirector::HasActiveDialogue() const noexcept {
    return !queue_.empty();
}

const DialogueLine* StoryDirector::CurrentLine() const noexcept {
    if (queue_.empty()) {
        return nullptr;
    }
    return &queue_.front();
}

float StoryDirector::LineElapsed() const noexcept {
    return line_elapsed_;
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
    return !EventFired(StoryEvent::SolarBypassed) &&
           !EventFired(StoryEvent::HydroBypassed) &&
           !EventFired(StoryEvent::PurifierBypassed) &&
           !EventFired(StoryEvent::ThermalBypassed);
}

} // namespace eco
