#include "core/collectible.h"

#include <algorithm>
#include <cmath>

namespace eco {

namespace {

[[nodiscard]] constexpr Vec2 TileCenterPos(int row, int col) noexcept {
    return {
        (static_cast<float>(col) + 0.5f) * kTileSize,
        (static_cast<float>(row) + 0.5f) * kTileSize
    };
}

[[nodiscard]] CollectibleItem MakeRelic(
    CollectibleId id,
    CollectibleEra era,
    CollectibleKind kind,
    LocationId location,
    ZoneId zone,
    int row,
    int col,
    bool hidden = false) noexcept {
    CollectibleItem item{};
    item.id = id;
    item.era = era;
    item.kind = kind;
    item.location = location;
    item.zone = zone;
    item.pos = TileCenterPos(row, col);
    item.auto_pickup_radius = kDefaultRelicAutoPickupRadius;
    item.interact_radius = kDefaultRelicInteractRadius;
    item.collected = false;
    item.prompt_visible = false;
    item.hidden = hidden;
    return item;
}

} // namespace

void CollectibleRegistry::ResetDefault() {
    items_.clear();
    items_.reserve(static_cast<std::size_t>(kTotalCollectibles));

    // Era I - Alba delle Ombre (4 reliquie - coordinate locali nelle zone 26x36)
    items_.push_back(MakeRelic(
        CollectibleId::Dawn_BrassGnomon, CollectibleEra::Era1_Dawn, CollectibleKind::Astrolabe,
        LocationId::Overworld, ZoneId::DawnTerraces, 7, 5, false));
    items_.push_back(MakeRelic(
        CollectibleId::Dawn_FirstMeasureStele, CollectibleEra::Era1_Dawn, CollectibleKind::Stele,
        LocationId::Overworld, ZoneId::DawnTerraces, 13, 32, false));
    items_.push_back(MakeRelic(
        CollectibleId::Dawn_SolsticeCylinder, CollectibleEra::Era1_Dawn, CollectibleKind::Cylinder,
        LocationId::Overworld, ZoneId::WindPlateau, 9, 11, true));
    items_.push_back(MakeRelic(
        CollectibleId::Dawn_WhiteOliveSeed, CollectibleEra::Era1_Dawn, CollectibleKind::FossilSeed,
        LocationId::Overworld, ZoneId::CanalQuarter, 2, 13, true));

    // Era II - Secolo delle Acque e del Suono (4 reliquie)
    items_.push_back(MakeRelic(
        CollectibleId::Waters_HydraulicDiary, CollectibleEra::Era2_WatersAndSound, CollectibleKind::Cylinder,
        LocationId::Overworld, ZoneId::CanalQuarter, 16, 29, false));
    items_.push_back(MakeRelic(
        CollectibleId::Waters_SluiceStele, CollectibleEra::Era2_WatersAndSound, CollectibleKind::Stele,
        LocationId::HypogeanCisterns, ZoneId::CanalQuarter, 17, 25, false));
    items_.push_back(MakeRelic(
        CollectibleId::Waters_JadeTuningFork, CollectibleEra::Era2_WatersAndSound, CollectibleKind::Prism,
        LocationId::Overworld, ZoneId::HangingGardens, 12, 29, false));
    items_.push_back(MakeRelic(
        CollectibleId::Waters_ThreadChronometer, CollectibleEra::Era2_WatersAndSound, CollectibleKind::Astrolabe,
        LocationId::PendulumTower, ZoneId::WindPlateau, 6, 34, false));

    // Era III - Inverno di Pece (4 reliquie)
    items_.push_back(MakeRelic(
        CollectibleId::Pitch_SchismMinutes, CollectibleEra::Era3_PitchWinter, CollectibleKind::Cylinder,
        LocationId::Overworld, ZoneId::FumaroleCaldera, 2, 29, true));
    items_.push_back(MakeRelic(
        CollectibleId::Pitch_EmberStele, CollectibleEra::Era3_PitchWinter, CollectibleKind::Stele,
        LocationId::GeothermalChamber, ZoneId::FumaroleCaldera, 9, 25, true));
    items_.push_back(MakeRelic(
        CollectibleId::Pitch_CharredSeed, CollectibleEra::Era3_PitchWinter, CollectibleKind::FossilSeed,
        LocationId::Overworld, ZoneId::HangingGardens, 21, 4, true));
    items_.push_back(MakeRelic(
        CollectibleId::Pitch_BasaltThermometer, CollectibleEra::Era3_PitchWinter, CollectibleKind::Astrolabe,
        LocationId::GeothermalChamber, ZoneId::FumaroleCaldera, 9, 36, false));

    // Era IV - Sigillo dell'Armonia (4 reliquie)
    items_.push_back(MakeRelic(
        CollectibleId::Harmony_FarewellPrism, CollectibleEra::Era4_HarmonySeal, CollectibleKind::Prism,
        LocationId::HypogeanCisterns, ZoneId::CanalQuarter, 9, 3, true));
    items_.push_back(MakeRelic(
        CollectibleId::Harmony_CurrentsPrism, CollectibleEra::Era4_HarmonySeal, CollectibleKind::Prism,
        LocationId::PendulumTower, ZoneId::WindPlateau, 4, 3, true));
    items_.push_back(MakeRelic(
        CollectibleId::Harmony_AwakeningSeed, CollectibleEra::Era4_HarmonySeal, CollectibleKind::FossilSeed,
        LocationId::Overworld, ZoneId::HarmonySanctuary, 13, 30, false));
    items_.push_back(MakeRelic(
        CollectibleId::Harmony_AstrolabeStele, CollectibleEra::Era4_HarmonySeal, CollectibleKind::Stele,
        LocationId::Overworld, ZoneId::HarmonySanctuary, 19, 18, false));

    has_last_collected_ = false;
    last_collected_id_ = CollectibleId::Dawn_BrassGnomon;
    pickup_banner_timer_ = 0.0f;
    last_collected_pos_ = {0.0f, 0.0f};
    last_collected_location_ = LocationId::Overworld;
}

const CollectibleItem* CollectibleRegistry::Find(CollectibleId id) const noexcept {
    for (const auto& item : items_) {
        if (item.id == id) {
            return &item;
        }
    }
    return nullptr;
}

CollectibleItem* CollectibleRegistry::FindMutable(CollectibleId id) noexcept {
    for (auto& item : items_) {
        if (item.id == id) {
            return &item;
        }
    }
    return nullptr;
}

bool CollectibleRegistry::IsCollected(CollectibleId id) const noexcept {
    const CollectibleItem* item = Find(id);
    return item != nullptr && item->collected;
}

bool CollectibleRegistry::Collect(CollectibleId id) noexcept {
    CollectibleItem* item = FindMutable(id);
    if (item == nullptr || item->collected) {
        return false;
    }
    item->collected = true;
    item->prompt_visible = false;
    has_last_collected_ = true;
    last_collected_id_ = item->id;
    pickup_banner_timer_ = kRelicBannerDuration;
    last_collected_pos_ = item->pos;
    last_collected_location_ = item->location;
    return true;
}

void CollectibleRegistry::CollectAll() noexcept {
    for (auto& item : items_) {
        item.collected = true;
        item.prompt_visible = false;
    }
    if (!items_.empty()) {
        has_last_collected_ = true;
        last_collected_id_ = items_.back().id;
        last_collected_pos_ = items_.back().pos;
        last_collected_location_ = items_.back().location;
    }
}

void CollectibleRegistry::UpdateProximity(
    LocationId current_location,
    const PlayerManager& players) noexcept {
    for (auto& item : items_) {
        item.prompt_visible = false;
        if (item.collected || item.location != current_location) {
            continue;
        }
        for (const Player* p : players.ActivePlayers()) {
            if (p == nullptr) {
                continue;
            }
            const float dist = p->Center().DistanceTo(item.pos);
            if (std::isfinite(dist) && dist <= item.interact_radius) {
                item.prompt_visible = true;
                break;
            }
        }
    }
}

std::optional<CollectibleId> CollectibleRegistry::TryPickupNear(
    LocationId current_location,
    const PlayerManager& players,
    bool interact_pressed) noexcept {
    for (auto& item : items_) {
        if (item.collected || item.location != current_location) {
            continue;
        }
        for (const Player* p : players.ActivePlayers()) {
            if (p == nullptr) {
                continue;
            }
            const float dist = p->Center().DistanceTo(item.pos);
            if (!std::isfinite(dist)) {
                continue;
            }
            const bool auto_hit = dist <= item.auto_pickup_radius;
            const bool interact_hit = interact_pressed && dist <= item.interact_radius;
            if (auto_hit || interact_hit) {
                if (Collect(item.id)) {
                    return item.id;
                }
            }
        }
    }
    return std::nullopt;
}

void CollectibleRegistry::Tick(float dt) noexcept {
    if (!std::isfinite(dt) || dt <= 0.0f) {
        return;
    }
    if (pickup_banner_timer_ > 0.0f) {
        pickup_banner_timer_ = std::max(0.0f, pickup_banner_timer_ - dt);
    }
}

int CollectibleRegistry::CollectedCount() const noexcept {
    int count = 0;
    for (const auto& item : items_) {
        if (item.collected) {
            ++count;
        }
    }
    return count;
}

int CollectibleRegistry::TotalCount() const noexcept {
    return static_cast<int>(items_.size());
}

int CollectibleRegistry::CollectedInEra(CollectibleEra era) const noexcept {
    int count = 0;
    for (const auto& item : items_) {
        if (item.era == era && item.collected) {
            ++count;
        }
    }
    return count;
}

int CollectibleRegistry::TotalInEra(CollectibleEra era) const noexcept {
    int count = 0;
    for (const auto& item : items_) {
        if (item.era == era) {
            ++count;
        }
    }
    return count;
}

int CollectibleRegistry::CollectedInZone(ZoneId zone) const noexcept {
    int count = 0;
    for (const auto& item : items_) {
        if (item.zone == zone && item.collected) {
            ++count;
        }
    }
    return count;
}

int CollectibleRegistry::TotalInZone(ZoneId zone) const noexcept {
    int count = 0;
    for (const auto& item : items_) {
        if (item.zone == zone) {
            ++count;
        }
    }
    return count;
}

int CollectibleRegistry::CollectedInLocation(LocationId loc) const noexcept {
    int count = 0;
    for (const auto& item : items_) {
        if (item.location == loc && item.collected) {
            ++count;
        }
    }
    return count;
}

int CollectibleRegistry::TotalInLocation(LocationId loc) const noexcept {
    int count = 0;
    for (const auto& item : items_) {
        if (item.location == loc) {
            ++count;
        }
    }
    return count;
}

RelicGameplaySpec GetRelicGameplaySpec(CollectibleId id) noexcept {
    switch (id) {
        case CollectibleId::Dawn_BrassGnomon:
            return {
                id,
                RelicToolCategory::OpticalLens,
                "Mirino Gnomonico della Lente",
                "Attivo su Lente [Q] + Passivo Ottico",
                "Estende del +45% il raggio della Lente [Q], proietta i rimbalzi completi degli specchi solari e svela la Bussola Empirica.",
                "Premi [Q] sulle Terrazze per leggere l'azimut solare esatto e tracciare i fasci verso le piastre.",
                true
            };
        case CollectibleId::Dawn_FirstMeasureStele:
            return {
                id,
                RelicToolCategory::Calibration,
                "Nonio della Prima Misura",
                "Passivo su Banco di Calibrazione",
                "Attiva il passo fine (0.25x), allarga del +30% la finestra armonica pulita e svela la banda ottima dopo 1 sola prova.",
                "Apri un monumento [E]: regola con precisione micrometrica e usa [T] Sperimenta per fissare la banda armonica.",
                true
            };
        case CollectibleId::Dawn_SolsticeCylinder:
            return {
                id,
                RelicToolCategory::MobilityFlow,
                "Giroscopio dei Solstizi",
                "Passivo su Altopiano dei Venti",
                "Riduce del 35% la spinta laterale delle raffiche cicliche sull'Altopiano dei Venti.",
                "Attraversa i varchi ventosi con maggiore stabilita' tra un monolite frangivento e l'altro.",
                false
            };
        case CollectibleId::Dawn_WhiteOliveSeed:
            return {
                id,
                RelicToolCategory::WorldEcology,
                "Seme Vivo d'Ulivo Bianco",
                "Attivo su Vasche Botaniche [E] + Bonus Guarigione",
                "Innestabile nelle vasche aride [E]: dissolve i rovi di pece vicini, riduce -6 smog, potenzia la cura Clean (-22) e dona +12% passo sui sentieri fioriti.",
                "Cerca le conche botaniche spente in Terrazze e Canali e premi [E] per far rifiorire la scorciatoia.",
                true
            };
        case CollectibleId::Waters_HydraulicDiary:
            return {
                id,
                RelicToolCategory::MobilityFlow,
                "Codice delle Chiuse Laminari",
                "Attivo su Leve Idrauliche [E]",
                "Potenzia le chiuse dei Canali in regime laminare: calma i vortici e prosciuga il Guado degli Idraulici.",
                "Aziona una leva idraulica [E] nel Quartiere dei Canali per far emergere la scorciatoia di pietra.",
                true
            };
        case CollectibleId::Waters_SluiceStele:
            return {
                id,
                RelicToolCategory::Calibration,
                "Tavola Piezometrica di Stevino",
                "Passivo Telemetria + Attivo Valvola Ausiliaria [E]",
                "Mostra pressione idrostatica live (kPa) nelle Cisterne e sblocca la Valvola di Equilibrio Ausiliaria davanti alla chiusa Est.",
                "Nelle Cisterne Ipogee premi [E] sulla valvola ausiliaria presso la colonna 26 per aprire un passaggio asciutto.",
                true
            };
        case CollectibleId::Waters_JadeTuningFork:
            return {
                id,
                RelicToolCategory::OpticalLens,
                "Diapason Armonico di Giada",
                "Passivo nei Giardini Sospesi",
                "Evidenzia la prossima piastra musicale nella sequenza armonica e allarga il raggio di risonanza.",
                "Segui il battito armonico nei Giardini Sospesi per stendere senza errori il Ponte di Giada.",
                false
            };
        case CollectibleId::Waters_ThreadChronometer:
            return {
                id,
                RelicToolCategory::Calibration,
                "Scappamento a Filo Isocrono",
                "Passivo su Pendolo Isochronos",
                "Stabilizza l'oscillazione del Pendolo mostrando il periodo esatto in secondi durante le prove.",
                "Usa i rintocchi della Torre del Pendolo per accordare la lunghezza L a 2.00 metri.",
                false
            };
        case CollectibleId::Pitch_SchismMinutes:
            return {
                id,
                RelicToolCategory::WorldEcology,
                "Registro delle Valvole Nere",
                "Passivo su Ricalibrazione Fornaci",
                "Riduce del 25% lo stress residuo durante le prove [T] su impianti precedentemente forzati a pece.",
                "Utile per riconvertire in sicurezza i monumenti lasciati in bypass sporco.",
                false
            };
        case CollectibleId::Pitch_EmberStele:
            return {
                id,
                RelicToolCategory::Calibration,
                "Griglia Refrattaria delle Braci",
                "Passivo Termico",
                "Contiene la dispersione termica riducendo di -1.0 lo smog generato dagli impulsi di prova fuori fase.",
                "Permette piu' esperimenti consecutivi nel banco di calibrazione senza saturare l'aria.",
                false
            };
        case CollectibleId::Pitch_CharredSeed:
            return {
                id,
                RelicToolCategory::WorldEcology,
                "Seme Pirolitico Resiliente",
                "Passivo Ecologico",
                "Mantiene viva una riserva minima di flora anche quando l'inquinamento supera la soglia critica.",
                "Accelera la ripresa biologica appena un monumento viene riportato in regime pulito.",
                false
            };
        case CollectibleId::Pitch_BasaltThermometer:
            return {
                id,
                RelicToolCategory::MobilityFlow,
                "Termometro Differenziale di Basalto",
                "Passivo nella Caldera delle Fumarole",
                "Anticpa il preavviso dei geyser termici e riduce del 30% l'impulso d'urto del vapore.",
                "Osserva gli anelli termici prima dell'eruzione per attraversare la Caldera in sicurezza.",
                false
            };
        case CollectibleId::Harmony_FarewellPrism:
            return {
                id,
                RelicToolCategory::OpticalLens,
                "Prisma di Risonanza Remota",
                "Attivo con Impulso Lente [Q] entro 220 px",
                "L'impulso [Q] commuta a distanza lo specchio solare, la chiusa idraulica o l'interruttore delle Cisterne piu' vicino!",
                "Premi [Q] entro 220 px da specchi, leve o switch oltre una paratoia per azionarli senza toccarli.",
                true
            };
        case CollectibleId::Harmony_CurrentsPrism:
            return {
                id,
                RelicToolCategory::MobilityFlow,
                "Prisma Idrodinamico delle Correnti",
                "Passivo nei Canali",
                "Riduce del 25% la resistenza controcorrente quando guadi i canali rapidi.",
                "Permette di risalire anche i tratti accelerati dal regime supercritico.",
                false
            };
        case CollectibleId::Harmony_AwakeningSeed:
            return {
                id,
                RelicToolCategory::WorldEcology,
                "Germoglio del Risveglio",
                "Passivo Aura Vitale",
                "Aumenta la fauna ambientale e porta la vitalita' locale al massimo nelle zone purificate.",
                "Amplifica la rifioritura attorno al Santuario dell'Armonia.",
                false
            };
        case CollectibleId::Harmony_AstrolabeStele:
            return {
                id,
                RelicToolCategory::Calibration,
                "Chiave di Fase dell'Astrolabio",
                "Passivo Sintesi Finale",
                "Rivela l'angolo aureo di 72 gradi e stabilizza l'interferenza costruttiva dell'Astrolabio Centrale.",
                "Usala nel Santuario dell'Armonia per completare la sintesi delle cinque leggi.",
                false
            };
        case CollectibleId::Count:
            break;
    }
    return {};
}

} // namespace eco
