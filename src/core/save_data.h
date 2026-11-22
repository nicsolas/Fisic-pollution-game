#pragma once

#include <string>

#include "core/math_types.h"
#include "core/simulation.h"
#include "core/story.h"

namespace eco {

struct SaveSummary {
    bool exists = false;
    GameMode mode = GameMode::Overworld;
    ZoneId zone = ZoneId::DawnTerraces;
    StructureId structure = StructureId::HypogeanCisterns;
    Chapter chapter = Chapter::Prologo;
    float pollution_level = kInitialPollution;
    float clean_energy = 0.0f;
    int monuments_active = 0;
    int relics_found = 0;
    int total_relics = 16;
    bool simulation_completed = false;
    std::string location_name{};
    std::string chapter_title{};
    std::string timestamp{};
};

// Percorso predefinito del salvataggio della campagna (relativo alla directory di lavoro)
[[nodiscard]] inline const char* DefaultSaveFilePath() noexcept {
    return "saves/conca_di_meridia.sav";
}

// Salva lo stato completo del mondo (posizione, zona/struttura, ecosistema, 6 monumenti,
// 4 NPC, 16 reliquie, eventi narrativi e Cronologia Dialoghi integrale).
bool SaveWorldToFile(
    const WorldContext& world,
    const std::string& path = DefaultSaveFilePath(),
    const std::string& timestamp = "");

// Carica lo stato completo del mondo dal file di salvataggio.
bool LoadWorldFromFile(
    WorldContext& world,
    const std::string& path = DefaultSaveFilePath());

// Legge rapidamente il riepilogo del salvataggio per il menu "Riprendi da dove hai lasciato".
[[nodiscard]] SaveSummary ReadSaveSummary(
    const std::string& path = DefaultSaveFilePath());

bool DeleteSaveFile(const std::string& path = DefaultSaveFilePath());

} // namespace eco
