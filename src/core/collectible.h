#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/math_types.h"
#include "core/player.h"

namespace eco {

inline constexpr int kTotalCollectibles = 16;
inline constexpr int kCollectiblesPerEra = 4;
inline constexpr int kRelicsForTrueEnding = 12;
inline constexpr float kRelicBannerDuration = 4.2f;
inline constexpr float kDefaultRelicAutoPickupRadius = 38.0f;
inline constexpr float kDefaultRelicInteractRadius = 72.0f;

enum class CollectibleEra : int {
    Era1_Dawn = 0,           // Era I - Alba delle Ombre
    Era2_WatersAndSound = 1, // Era II - Secolo delle Acque e del Suono
    Era3_PitchWinter = 2,    // Era III - Inverno di Pece
    Era4_HarmonySeal = 3,    // Era IV - Sigillo dell'Armonia
    Count = 4
};

enum class CollectibleKind : int {
    Stele = 0,
    Astrolabe = 1,
    Prism = 2,
    Cylinder = 3,
    FossilSeed = 4,
    Count = 5
};

enum class CollectibleId : int {
    // Era I - Alba delle Ombre
    Dawn_BrassGnomon = 0,         // I.1 Gnomone d'Ottone (Astrolabe, Terrazze dell'Alba)
    Dawn_FirstMeasureStele = 1,   // I.2 Stele della Prima Misura (Stele, Terrazze dell'Alba)
    Dawn_SolsticeCylinder = 2,    // I.3 Cilindro dei Solstizi (Cylinder, Altopiano dei Venti)
    Dawn_WhiteOliveSeed = 3,      // I.4 Seme d'Ulivo Bianco (FossilSeed, Quartiere dei Canali)
    // Era II - Secolo delle Acque e del Suono
    Waters_HydraulicDiary = 4,    // II.1 Diario dell'Idraulico (Cylinder, Quartiere dei Canali)
    Waters_SluiceStele = 5,       // II.2 Stele delle Chiusure (Stele, Cisterne Ipogee)
    Waters_JadeTuningFork = 6,    // II.3 Diapason di Giada (Prism, Giardini Sospesi)
    Waters_ThreadChronometer = 7, // II.4 Cronometro a Filo (Astrolabe, Torre del Pendolo)
    // Era III - Inverno di Pece
    Pitch_SchismMinutes = 8,      // III.1 Verbale dello Scisma (Cylinder, Caldera delle Fumarole)
    Pitch_EmberStele = 9,         // III.2 Stele delle Braci (Stele, Camera Geotermica)
    Pitch_CharredSeed = 10,       // III.3 Seme Annerito (FossilSeed, Giardini Sospesi)
    Pitch_BasaltThermometer = 11, // III.4 Termometro di Basalto (Astrolabe, Camera Geotermica)
    // Era IV - Sigillo dell'Armonia
    Harmony_FarewellPrism = 12,   // IV.1 Prisma del Congedo (Prism, Cisterne Ipogee - ala alta)
    Harmony_CurrentsPrism = 13,   // IV.2 Prisma delle Correnti (Prism, Torre del Pendolo)
    Harmony_AwakeningSeed = 14,   // IV.3 Seme del Risveglio (FossilSeed, Santuario dell'Armonia)
    Harmony_AstrolabeStele = 15,  // IV.4 Stele dell'Astrolabio (Stele, Santuario dell'Armonia)
    Count = 16
};

[[nodiscard]] constexpr const char* EraName(CollectibleEra era) noexcept {
    switch (era) {
        case CollectibleEra::Era1_Dawn:
            return "Era I - Alba delle Ombre";
        case CollectibleEra::Era2_WatersAndSound:
            return "Era II - Secolo delle Acque e del Suono";
        case CollectibleEra::Era3_PitchWinter:
            return "Era III - Inverno di Pece";
        case CollectibleEra::Era4_HarmonySeal:
            return "Era IV - Sigillo dell'Armonia";
        case CollectibleEra::Count:
            break;
    }
    return "Era di Meridia";
}

[[nodiscard]] constexpr const char* CollectibleKindName(CollectibleKind kind) noexcept {
    switch (kind) {
        case CollectibleKind::Stele:
            return "Stele Incisa";
        case CollectibleKind::Astrolabe:
            return "Strumento Astronomico";
        case CollectibleKind::Prism:
            return "Prisma Armonico";
        case CollectibleKind::Cylinder:
            return "Cilindro d'Archivio";
        case CollectibleKind::FossilSeed:
            return "Seme Fossile";
        case CollectibleKind::Count:
            break;
    }
    return "Reliquia";
}

// Contenuto letterario (implementato in src/core/collectible_content.cpp da @narratore-2)
struct CollectibleMeta {
    CollectibleId id = CollectibleId::Dawn_BrassGnomon;
    CollectibleEra era = CollectibleEra::Era1_Dawn;
    CollectibleKind kind = CollectibleKind::Stele;
    std::string code{};
    std::string title{};
    std::string era_name{};
    std::string zone_name{};
    std::string pickup_speaker{};
    std::string pickup_line{};
    std::vector<std::string> codex_paragraphs{};
};

[[nodiscard]] const CollectibleMeta& CollectibleMetadata(CollectibleId id);

enum class RelicToolCategory : int {
    OpticalLens = 0,     // Lente
    Calibration = 1,     // Calibrazione
    WorldEcology = 2,    // Mondo
    MobilityFlow = 3     // Movimento
};

[[nodiscard]] constexpr const char* RelicToolCategoryName(RelicToolCategory cat) noexcept {
    switch (cat) {
        case RelicToolCategory::OpticalLens:
            return "Strumento Lente [Q]";
        case RelicToolCategory::Calibration:
            return "Banco Calibrazione";
        case RelicToolCategory::WorldEcology:
            return "Ecologia e Varchi";
        case RelicToolCategory::MobilityFlow:
            return "Flussi e Movimento";
    }
    return "Strumento di Campo";
}

struct RelicGameplaySpec {
    CollectibleId id = CollectibleId::Dawn_BrassGnomon;
    RelicToolCategory category = RelicToolCategory::OpticalLens;
    const char* tool_name = "";
    const char* active_or_passive_label = "";
    const char* effect_summary = "";
    const char* field_usage_hint = "";
    bool is_vertical_slice_core = false;
};

[[nodiscard]] RelicGameplaySpec GetRelicGameplaySpec(CollectibleId id) noexcept;

// Stato e collocazione spaziale nel mondo (implementato in src/core/collectible.cpp da @ingegnere-1)
struct CollectibleItem {
    CollectibleId id = CollectibleId::Dawn_BrassGnomon;
    CollectibleEra era = CollectibleEra::Era1_Dawn;
    CollectibleKind kind = CollectibleKind::Stele;
    LocationId location = LocationId::Overworld;
    ZoneId zone = ZoneId::DawnTerraces;
    Vec2 pos{0.0f, 0.0f};
    float auto_pickup_radius = kDefaultRelicAutoPickupRadius;
    float interact_radius = kDefaultRelicInteractRadius;
    bool collected = false;
    bool prompt_visible = false;
    bool hidden = false;
};

class CollectibleRegistry {
public:
    void ResetDefault();

    [[nodiscard]] const std::vector<CollectibleItem>& Items() const noexcept { return items_; }
    [[nodiscard]] std::vector<CollectibleItem>& Items() noexcept { return items_; }

    [[nodiscard]] const CollectibleItem* Find(CollectibleId id) const noexcept;
    [[nodiscard]] CollectibleItem* FindMutable(CollectibleId id) noexcept;
    [[nodiscard]] bool IsCollected(CollectibleId id) const noexcept;

    bool Collect(CollectibleId id) noexcept;
    void CollectAll() noexcept;

    void UpdateProximity(LocationId current_location, const PlayerManager& players) noexcept;
    [[nodiscard]] std::optional<CollectibleId> TryPickupNear(
        LocationId current_location,
        const PlayerManager& players,
        bool interact_pressed) noexcept;
    void Tick(float dt) noexcept;

    [[nodiscard]] int CollectedCount() const noexcept;
    [[nodiscard]] int TotalCount() const noexcept;
    [[nodiscard]] int CollectedInEra(CollectibleEra era) const noexcept;
    [[nodiscard]] int TotalInEra(CollectibleEra era) const noexcept;
    [[nodiscard]] int CollectedInZone(ZoneId zone) const noexcept;
    [[nodiscard]] int TotalInZone(ZoneId zone) const noexcept;
    [[nodiscard]] int CollectedInLocation(LocationId loc) const noexcept;
    [[nodiscard]] int TotalInLocation(LocationId loc) const noexcept;

    [[nodiscard]] bool HasRecentPickup() const noexcept { return has_last_collected_ && pickup_banner_timer_ > 0.0f; }
    void ClearRecentBanner() noexcept {
        has_last_collected_ = false;
        pickup_banner_timer_ = 0.0f;
    }
    [[nodiscard]] CollectibleId LastCollectedId() const noexcept { return last_collected_id_; }
    [[nodiscard]] float PickupBannerTimer() const noexcept { return pickup_banner_timer_; }
    [[nodiscard]] Vec2 LastCollectedPos() const noexcept { return last_collected_pos_; }
    [[nodiscard]] LocationId LastCollectedLocation() const noexcept { return last_collected_location_; }

private:
    std::vector<CollectibleItem> items_{};
    bool has_last_collected_ = false;
    CollectibleId last_collected_id_ = CollectibleId::Dawn_BrassGnomon;
    float pickup_banner_timer_ = 0.0f;
    Vec2 last_collected_pos_{0.0f, 0.0f};
    LocationId last_collected_location_ = LocationId::Overworld;
};

} // namespace eco
