#pragma once

#include <string>

#include "core/ecosystem.h"
#include "core/math_types.h"
#include "core/player.h"

namespace eco {

inline constexpr float kDefaultNpcTriggerRadius = 78.0f;

enum class NpcId : int {
    ClaraSereni = 0,   // Ingegnera dei canali, custode del Ponte degli Archi
    LorenzoAltieri = 1,// Archivista delle Cisterne Ipogee
    MiriamFerro = 2,   // Botanica dei Giardini Sospesi
    MatteoVerrini = 3, // Sovrintendente della Compagnia della Pece
    Count = 4
};

[[nodiscard]] constexpr const char* NpcDisplayName(NpcId id) noexcept {
    switch (id) {
        case NpcId::ClaraSereni:
            return "Clara Sereni";
        case NpcId::LorenzoAltieri:
            return "Lorenzo Altieri";
        case NpcId::MiriamFerro:
            return "Miriam Ferro";
        case NpcId::MatteoVerrini:
            return "Matteo Verrini";
        case NpcId::Count:
            break;
    }
    return "Custode";
}

[[nodiscard]] constexpr const char* NpcRoleTitle(NpcId id) noexcept {
    switch (id) {
        case NpcId::ClaraSereni:
            return "Ingegnera dei Canali";
        case NpcId::LorenzoAltieri:
            return "Archivista di Meridia";
        case NpcId::MiriamFerro:
            return "Botanica dei Giardini";
        case NpcId::MatteoVerrini:
            return "Sovrintendente della Pece";
        case NpcId::Count:
            break;
    }
    return "Abitante della Conca";
}

struct OverlayVisibility {
    UiPhase phase = UiPhase::Phase1_Minimal;
    bool show_raw_numbers = false;
    bool show_velocity_vectors = false;
    bool show_aabb_hitboxes = false;
    bool show_irradiance_rays = false;
    bool show_trigger_radii = false;
};

[[nodiscard]] OverlayVisibility ComputeOverlayVisibility(const EcoSystem& ecosystem) noexcept;
bool TryRequestOverlayPhase(EcoSystem& ecosystem, UiPhase requested) noexcept;

enum class NpcMood : int {
    NeutralObserving = 0,
    FlourishingTrust = 1,
    TenseOverdrive = 2,
    ShelteredPolluted = 3
};

[[nodiscard]] constexpr const char* NpcMoodName(NpcMood mood) noexcept {
    switch (mood) {
        case NpcMood::NeutralObserving:
            return "In Osservazione";
        case NpcMood::FlourishingTrust:
            return "Fiducia Armonica";
        case NpcMood::TenseOverdrive:
            return "Allerta Supercritica";
        case NpcMood::ShelteredPolluted:
            return "Riparato dai Fumi";
    }
    return "In Osservazione";
}

struct WorldNpc {
    NpcId id = NpcId::ClaraSereni;
    std::string name{};
    std::string role{};
    LocationId location = LocationId::Overworld;
    ZoneId zone = ZoneId::CanalQuarter;
    Vec2 pos{0.0f, 0.0f};
    Vec2 home_pos{0.0f, 0.0f};
    float trigger_radius = kDefaultNpcTriggerRadius;
    NpcMood mood = NpcMood::NeutralObserving;
    bool met = false;
    bool recruited = false;
    bool hidden_bridge_revealed = false;
    bool shortcut_unlocked = false;
    bool prompt_visible = false;
    const char* status_note = "";
    std::string last_dialogue{};

    [[nodiscard]] static WorldNpc CreateDefault(NpcId id);

    [[nodiscard]] bool IsPlayerInRange(const Player& player) const noexcept;
    [[nodiscard]] bool AnyActivePlayerInRange(const PlayerManager& players) const noexcept;
    void UpdateProximity(const PlayerManager& players) noexcept;
};

// Compatibility helper for Clara Sereni recruitment logic & unit tests
struct ArchitectNpc {
    Vec2 pos{1032.0f, 840.0f};
    float trigger_radius = kDefaultNpcTriggerRadius;
    bool recruited = false;
    bool hidden_bridge_revealed = false;
    std::string last_dialogue{};

    void Reset(Vec2 default_pos = {1032.0f, 840.0f});
    [[nodiscard]] bool IsPlayerInRange(const Player& player) const noexcept;
    [[nodiscard]] bool AnyActivePlayerInRange(const PlayerManager& players) const noexcept;
    bool TryInteract(int player_id, const PlayerManager& players, EcoSystem& ecosystem);
};

} // namespace eco
