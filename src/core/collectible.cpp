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
    int col) noexcept {
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
    return item;
}

} // namespace

void CollectibleRegistry::ResetDefault() {
    items_.clear();
    items_.reserve(static_cast<std::size_t>(kTotalCollectibles));

    // Era I - Alba delle Ombre (4 reliquie)
    items_.push_back(MakeRelic(
        CollectibleId::Dawn_BrassGnomon, CollectibleEra::Era1_Dawn, CollectibleKind::Astrolabe,
        LocationId::Overworld, ZoneId::DawnTerraces, 26, 18));
    items_.push_back(MakeRelic(
        CollectibleId::Dawn_FirstMeasureStele, CollectibleEra::Era1_Dawn, CollectibleKind::Stele,
        LocationId::Overworld, ZoneId::DawnTerraces, 33, 4));
    items_.push_back(MakeRelic(
        CollectibleId::Dawn_SolsticeCylinder, CollectibleEra::Era1_Dawn, CollectibleKind::Cylinder,
        LocationId::Overworld, ZoneId::WindPlateau, 8, 20));
    items_.push_back(MakeRelic(
        CollectibleId::Dawn_WhiteOliveSeed, CollectibleEra::Era1_Dawn, CollectibleKind::FossilSeed,
        LocationId::Overworld, ZoneId::CanalQuarter, 21, 4));

    // Era II - Secolo delle Acque e del Suono (4 reliquie)
    items_.push_back(MakeRelic(
        CollectibleId::Waters_HydraulicDiary, CollectibleEra::Era2_WatersAndSound, CollectibleKind::Cylinder,
        LocationId::Overworld, ZoneId::CanalQuarter, 14, 15));
    items_.push_back(MakeRelic(
        CollectibleId::Waters_SluiceStele, CollectibleEra::Era2_WatersAndSound, CollectibleKind::Stele,
        LocationId::HypogeanCisterns, ZoneId::CanalQuarter, 9, 10));
    items_.push_back(MakeRelic(
        CollectibleId::Waters_JadeTuningFork, CollectibleEra::Era2_WatersAndSound, CollectibleKind::Prism,
        LocationId::Overworld, ZoneId::HangingGardens, 14, 44));
    items_.push_back(MakeRelic(
        CollectibleId::Waters_ThreadChronometer, CollectibleEra::Era2_WatersAndSound, CollectibleKind::Astrolabe,
        LocationId::PendulumTower, ZoneId::WindPlateau, 5, 5));

    // Era III - Inverno di Pece (4 reliquie)
    items_.push_back(MakeRelic(
        CollectibleId::Pitch_SchismMinutes, CollectibleEra::Era3_PitchWinter, CollectibleKind::Cylinder,
        LocationId::Overworld, ZoneId::FumaroleCaldera, 4, 45));
    items_.push_back(MakeRelic(
        CollectibleId::Pitch_EmberStele, CollectibleEra::Era3_PitchWinter, CollectibleKind::Stele,
        LocationId::GeothermalChamber, ZoneId::FumaroleCaldera, 13, 29));
    items_.push_back(MakeRelic(
        CollectibleId::Pitch_CharredSeed, CollectibleEra::Era3_PitchWinter, CollectibleKind::FossilSeed,
        LocationId::Overworld, ZoneId::HangingGardens, 22, 42));
    items_.push_back(MakeRelic(
        CollectibleId::Pitch_BasaltThermometer, CollectibleEra::Era3_PitchWinter, CollectibleKind::Astrolabe,
        LocationId::GeothermalChamber, ZoneId::FumaroleCaldera, 5, 6));

    // Era IV - Sigillo dell'Armonia (4 reliquie)
    items_.push_back(MakeRelic(
        CollectibleId::Harmony_FarewellPrism, CollectibleEra::Era4_HarmonySeal, CollectibleKind::Prism,
        LocationId::HypogeanCisterns, ZoneId::CanalQuarter, 7, 31));
    items_.push_back(MakeRelic(
        CollectibleId::Harmony_CurrentsPrism, CollectibleEra::Era4_HarmonySeal, CollectibleKind::Prism,
        LocationId::PendulumTower, ZoneId::WindPlateau, 5, 25));
    items_.push_back(MakeRelic(
        CollectibleId::Harmony_AwakeningSeed, CollectibleEra::Era4_HarmonySeal, CollectibleKind::FossilSeed,
        LocationId::Overworld, ZoneId::HarmonySanctuary, 32, 29));
    items_.push_back(MakeRelic(
        CollectibleId::Harmony_AstrolabeStele, CollectibleEra::Era4_HarmonySeal, CollectibleKind::Stele,
        LocationId::Overworld, ZoneId::HarmonySanctuary, 32, 45));

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

} // namespace eco
