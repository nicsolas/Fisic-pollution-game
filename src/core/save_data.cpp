#include "core/save_data.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace eco {

namespace {

constexpr int kSaveFormatVersion = 1;

[[nodiscard]] std::string CurrentTimestampString() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &tt);
#else
    localtime_r(&tt, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M");
    return oss.str();
}

[[nodiscard]] std::string EscapeField(std::string_view input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (char c : input) {
        if (c == '\\') {
            out += "\\\\";
        } else if (c == '|') {
            out += "\\p";
        } else if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            // salta CR
        } else {
            out.push_back(c);
        }
    }
    return out;
}

[[nodiscard]] std::string UnescapeField(std::string_view input) {
    std::string out;
    out.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && i + 1 < input.size()) {
            const char nxt = input[++i];
            if (nxt == 'p') {
                out.push_back('|');
            } else if (nxt == 'n') {
                out.push_back('\n');
            } else {
                out.push_back(nxt);
            }
        } else {
            out.push_back(input[i]);
        }
    }
    return out;
}

[[nodiscard]] std::vector<std::string> SplitPipe(std::string_view line) {
    std::vector<std::string> parts;
    std::string cur;
    for (std::size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '|') {
            parts.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(line[i]);
        }
    }
    parts.push_back(cur);
    return parts;
}

[[nodiscard]] int ParseIntSafe(const std::string& s, int fallback = 0) noexcept {
    try {
        return std::stoi(s);
    } catch (...) {
        return fallback;
    }
}

[[nodiscard]] float ParseFloatSafe(const std::string& s, float fallback = 0.0f) noexcept {
    try {
        const float v = std::stof(s);
        return std::isfinite(v) ? v : fallback;
    } catch (...) {
        return fallback;
    }
}

[[nodiscard]] int CountActivePrimaryMonuments(const WorldContext& world) noexcept {
    int count = 0;
    for (int i = 0; i < 5; ++i) {
        if (world.machines[static_cast<std::size_t>(i)].state != MachineState::Broken) {
            ++count;
        }
    }
    return count;
}

} // namespace

bool SaveWorldToFile(
    const WorldContext& world,
    const std::string& path,
    const std::string& timestamp) {
    if (path.empty()) {
        return false;
    }

    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path target(path);
    if (target.has_parent_path()) {
        fs::create_directories(target.parent_path(), ec);
    }

    const fs::path tmp_path = target.string() + ".tmp";
    std::ofstream out(tmp_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    const std::string ts = timestamp.empty() ? CurrentTimestampString() : timestamp;
    const Player* p0 = world.players.GetPlayer(0);
    const Vec2 p0_pos = (p0 != nullptr) ? p0->pos : world.overworld.spawn_point;
    const std::string loc_name = LocationName(world.ActiveLocation(), world.current_zone);
    const int monuments_active = CountActivePrimaryMonuments(world);

    out << "format=" << kSaveFormatVersion << "\n";
    out << "timestamp=" << EscapeField(ts) << "\n";
    out << "location_name=" << EscapeField(loc_name) << "\n";
    out << "chapter=" << static_cast<int>(world.story.CurrentChapter()) << "\n";
    out << "chapter_title=" << EscapeField(ChapterTitle(world.story.CurrentChapter())) << "\n";
    out << "mode=" << static_cast<int>(world.current_mode) << "\n";
    out << "zone=" << static_cast<int>(world.current_zone) << "\n";
    out << "structure=" << static_cast<int>(world.active_structure) << "\n";
    out << "p0_x=" << std::fixed << std::setprecision(3) << p0_pos.x << "\n";
    out << "p0_y=" << std::fixed << std::setprecision(3) << p0_pos.y << "\n";
    out << "pollution=" << std::fixed << std::setprecision(3) << world.ecosystem.pollution_level << "\n";
    out << "clean_energy=" << std::fixed << std::setprecision(3) << world.ecosystem.clean_energy << "\n";
    out << "dirty_energy=" << std::fixed << std::setprecision(3) << world.ecosystem.dirty_energy << "\n";
    out << "ui_phase=" << static_cast<int>(world.ecosystem.ui_phase) << "\n";
    out << "architect_recruited=" << (world.ecosystem.architect_recruited ? 1 : 0) << "\n";
    out << "monuments_active=" << monuments_active << "\n";
    out << "relics_found=" << world.collectibles.CollectedCount() << "\n";
    out << "total_relics=" << world.collectibles.TotalCount() << "\n";
    out << "simulation_completed=" << (world.simulation_completed ? 1 : 0) << "\n";
    out << "demo_mode=" << (world.demo_mode ? 1 : 0) << "\n";
    out << "sim_time=" << std::fixed << std::setprecision(3) << world.sim_time << "\n";
    out << "has_scanned=" << (world.last_scan.active ? 1 : 0) << "\n";

    // Monumenti (6)
    for (std::size_t i = 0; i < world.machines.size(); ++i) {
        const Machine& m = world.machines[i];
        out << "machine_" << i << "="
            << static_cast<int>(m.state) << "|"
            << std::fixed << std::setprecision(3) << m.current_param << "|"
            << (m.unlocked ? 1 : 0) << "\n";
    }

    // NPC (4)
    for (std::size_t i = 0; i < world.npcs.size(); ++i) {
        const WorldNpc& n = world.npcs[i];
        out << "npc_" << i << "="
            << (n.met ? 1 : 0) << "|"
            << (n.recruited ? 1 : 0) << "\n";
    }

    // Reliquie (16 bit)
    std::string relic_bits;
    relic_bits.reserve(static_cast<std::size_t>(CollectibleId::Count));
    for (int i = 0; i < static_cast<int>(CollectibleId::Count); ++i) {
        relic_bits.push_back(
            world.collectibles.IsCollected(static_cast<CollectibleId>(i)) ? '1' : '0');
    }
    out << "relics_mask=" << relic_bits << "\n";

    // Eventi StoryFired
    const auto& fired = world.story.FiredFlags();
    std::string fired_bits;
    fired_bits.reserve(fired.size());
    for (bool f : fired) {
        fired_bits.push_back(f ? '1' : '0');
    }
    out << "story_fired=" << fired_bits << "\n";

    // Cronologia Dialoghi
    for (const DialogueHistoryEntry& entry : world.story.History()) {
        out << "hist="
            << entry.sequence_id << "|"
            << static_cast<int>(entry.chapter) << "|"
            << static_cast<int>(entry.event) << "|"
            << (entry.is_ad_hoc ? 1 : 0) << "|"
            << static_cast<int>(entry.zone) << "|"
            << EscapeField(entry.location_name) << "|"
            << EscapeField(entry.speaker) << "|"
            << EscapeField(entry.text) << "\n";
    }

    out.flush();
    if (!out.good()) {
        out.close();
        fs::remove(tmp_path, ec);
        return false;
    }
    out.close();

    fs::rename(tmp_path, target, ec);
    if (ec) {
        // Fallback copia+rimuovi se rename cross-device
        fs::copy_file(tmp_path, target, fs::copy_options::overwrite_existing, ec);
        fs::remove(tmp_path, ec);
        return !ec;
    }
    return true;
}

SaveSummary ReadSaveSummary(const std::string& path) {
    SaveSummary summary{};
    if (path.empty()) {
        return summary;
    }

    std::ifstream in(path);
    if (!in.is_open()) {
        return summary;
    }

    int format = 0;
    std::string line;
    while (std::getline(in, line)) {
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        const std::string key = line.substr(0, eq);
        const std::string val = line.substr(eq + 1);

        if (key == "format") {
            format = ParseIntSafe(val, 0);
        } else if (key == "timestamp") {
            summary.timestamp = UnescapeField(val);
        } else if (key == "location_name") {
            summary.location_name = UnescapeField(val);
        } else if (key == "chapter") {
            const int c = std::clamp(ParseIntSafe(val, 0), 0, static_cast<int>(Chapter::Epilogo));
            summary.chapter = static_cast<Chapter>(c);
        } else if (key == "chapter_title") {
            summary.chapter_title = UnescapeField(val);
        } else if (key == "mode") {
            summary.mode = (ParseIntSafe(val, 0) == 1) ? GameMode::Structure : GameMode::Overworld;
        } else if (key == "zone") {
            const int z = std::clamp(ParseIntSafe(val, 0), 0, static_cast<int>(ZoneId::Count) - 1);
            summary.zone = static_cast<ZoneId>(z);
        } else if (key == "structure") {
            const int s = std::clamp(ParseIntSafe(val, 0), 0, static_cast<int>(StructureId::Count) - 1);
            summary.structure = static_cast<StructureId>(s);
        } else if (key == "pollution") {
            summary.pollution_level = Clamp(ParseFloatSafe(val, kInitialPollution), kMinEcoValue, kMaxEcoValue);
        } else if (key == "clean_energy") {
            summary.clean_energy = std::max(0.0f, ParseFloatSafe(val, 0.0f));
        } else if (key == "monuments_active") {
            summary.monuments_active = std::clamp(ParseIntSafe(val, 0), 0, 5);
        } else if (key == "relics_found") {
            summary.relics_found = std::clamp(ParseIntSafe(val, 0), 0, 16);
        } else if (key == "total_relics") {
            summary.total_relics = std::max(1, ParseIntSafe(val, 16));
        } else if (key == "simulation_completed") {
            summary.simulation_completed = (ParseIntSafe(val, 0) != 0);
        }
    }

    if (format != kSaveFormatVersion) {
        return SaveSummary{};
    }

    summary.exists = true;
    if (summary.location_name.empty()) {
        if (summary.mode == GameMode::Structure) {
            summary.location_name = StructureName(summary.structure);
        } else {
            summary.location_name = ZoneName(summary.zone);
        }
    }
    if (summary.chapter_title.empty()) {
        summary.chapter_title = ChapterTitle(summary.chapter);
    }
    return summary;
}

bool LoadWorldFromFile(WorldContext& world, const std::string& path) {
    if (path.empty()) {
        return false;
    }

    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    int format = 0;
    GameMode saved_mode = GameMode::Overworld;
    ZoneId saved_zone = ZoneId::DawnTerraces;
    StructureId saved_structure = StructureId::HypogeanCisterns;
    Vec2 saved_p0{0.0f, 0.0f};
    bool has_p0_x = false;
    bool has_p0_y = false;
    float pollution = kInitialPollution;
    float clean_energy = 0.0f;
    float dirty_energy = 0.0f;
    UiPhase ui_phase = UiPhase::Phase1_Minimal;
    bool architect_recruited = false;
    bool simulation_completed = false;
    bool demo_mode = false;
    float sim_time = 0.0f;
    bool has_scanned = false;

    std::array<MachineState, 6> m_states{};
    std::array<float, 6> m_params{};
    std::array<bool, 6> m_unlocked{};
    std::array<bool, 6> m_seen{};

    std::array<bool, 4> npc_met{};
    std::array<bool, 4> npc_recruited{};
    std::string relics_mask{};
    std::string story_fired_mask{};
    std::vector<DialogueHistoryEntry> history{};

    std::string line;
    while (std::getline(in, line)) {
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        const std::string key = line.substr(0, eq);
        const std::string val = line.substr(eq + 1);

        if (key == "format") {
            format = ParseIntSafe(val, 0);
        } else if (key == "mode") {
            saved_mode = (ParseIntSafe(val, 0) == 1) ? GameMode::Structure : GameMode::Overworld;
        } else if (key == "zone") {
            const int z = std::clamp(ParseIntSafe(val, 0), 0, static_cast<int>(ZoneId::Count) - 1);
            saved_zone = static_cast<ZoneId>(z);
        } else if (key == "structure") {
            const int s = std::clamp(ParseIntSafe(val, 0), 0, static_cast<int>(StructureId::Count) - 1);
            saved_structure = static_cast<StructureId>(s);
        } else if (key == "p0_x") {
            saved_p0.x = ParseFloatSafe(val, 0.0f);
            has_p0_x = true;
        } else if (key == "p0_y") {
            saved_p0.y = ParseFloatSafe(val, 0.0f);
            has_p0_y = true;
        } else if (key == "pollution") {
            pollution = Clamp(ParseFloatSafe(val, kInitialPollution), kMinEcoValue, kMaxEcoValue);
        } else if (key == "clean_energy") {
            clean_energy = std::max(0.0f, ParseFloatSafe(val, 0.0f));
        } else if (key == "dirty_energy") {
            dirty_energy = std::max(0.0f, ParseFloatSafe(val, 0.0f));
        } else if (key == "ui_phase") {
            const int p = std::clamp(ParseIntSafe(val, 1), 1, 3);
            ui_phase = static_cast<UiPhase>(p);
        } else if (key == "architect_recruited") {
            architect_recruited = (ParseIntSafe(val, 0) != 0);
        } else if (key == "simulation_completed") {
            simulation_completed = (ParseIntSafe(val, 0) != 0);
        } else if (key == "demo_mode") {
            demo_mode = (ParseIntSafe(val, 0) != 0);
        } else if (key == "sim_time") {
            sim_time = std::max(0.0f, ParseFloatSafe(val, 0.0f));
        } else if (key == "has_scanned") {
            has_scanned = (ParseIntSafe(val, 0) != 0);
        } else if (key == "relics_mask") {
            relics_mask = val;
        } else if (key == "story_fired") {
            story_fired_mask = val;
        } else if (key.rfind("machine_", 0) == 0) {
            const int idx = ParseIntSafe(key.substr(8), -1);
            if (idx >= 0 && idx < 6) {
                const auto parts = SplitPipe(val);
                if (parts.size() >= 3) {
                    const int st = std::clamp(ParseIntSafe(parts[0], 0), 0, 2);
                    m_states[static_cast<std::size_t>(idx)] = static_cast<MachineState>(st);
                    m_params[static_cast<std::size_t>(idx)] = ParseFloatSafe(parts[1], 0.0f);
                    m_unlocked[static_cast<std::size_t>(idx)] = (ParseIntSafe(parts[2], 1) != 0);
                    m_seen[static_cast<std::size_t>(idx)] = true;
                }
            }
        } else if (key.rfind("npc_", 0) == 0) {
            const int idx = ParseIntSafe(key.substr(4), -1);
            if (idx >= 0 && idx < 4) {
                const auto parts = SplitPipe(val);
                if (parts.size() >= 2) {
                    npc_met[static_cast<std::size_t>(idx)] = (ParseIntSafe(parts[0], 0) != 0);
                    npc_recruited[static_cast<std::size_t>(idx)] = (ParseIntSafe(parts[1], 0) != 0);
                }
            }
        } else if (key == "hist") {
            const auto parts = SplitPipe(val);
            if (parts.size() >= 8) {
                DialogueHistoryEntry e{};
                e.sequence_id = std::max(1, ParseIntSafe(parts[0], 1));
                e.chapter = static_cast<Chapter>(
                    std::clamp(ParseIntSafe(parts[1], 0), 0, static_cast<int>(Chapter::Epilogo)));
                e.event = static_cast<StoryEvent>(
                    std::clamp(ParseIntSafe(parts[2], 0), 0, static_cast<int>(StoryEvent::Count) - 1));
                e.is_ad_hoc = (ParseIntSafe(parts[3], 0) != 0);
                e.zone = static_cast<ZoneId>(
                    std::clamp(ParseIntSafe(parts[4], 0), 0, static_cast<int>(ZoneId::Count) - 1));
                e.location_name = UnescapeField(parts[5]);
                e.speaker = UnescapeField(parts[6]);
                e.text = UnescapeField(parts[7]);
                history.push_back(std::move(e));
            }
        }
    }

    if (format != kSaveFormatVersion) {
        return false;
    }

    // Ricostruisci il mondo in modo pulito e deterministico
    world.Reset();
    world.EnsureInitialized();

    world.demo_mode = demo_mode;
    world.sim_time = sim_time;
    world.simulation_completed = simulation_completed;
    world.ecosystem.pollution_level = pollution;
    world.ecosystem.clean_energy = clean_energy;
    world.ecosystem.dirty_energy = dirty_energy;
    world.ecosystem.ui_phase = ui_phase;
    world.ecosystem.architect_recruited = architect_recruited;
    world.last_scan.active = has_scanned;

    for (std::size_t i = 0; i < world.machines.size(); ++i) {
        if (m_seen[i]) {
            world.machines[i].state = m_states[i];
            world.machines[i].current_param = Clamp(
                m_params[i],
                world.machines[i].min_param,
                world.machines[i].max_param);
            world.machines[i].unlocked = m_unlocked[i];
        }
    }

    for (std::size_t i = 0; i < world.npcs.size(); ++i) {
        world.npcs[i].met = npc_met[i];
        world.npcs[i].recruited = npc_recruited[i];
    }

    for (std::size_t i = 0; i < relics_mask.size() && i < static_cast<std::size_t>(CollectibleId::Count); ++i) {
        if (relics_mask[i] == '1') {
            (void)world.collectibles.Collect(static_cast<CollectibleId>(i));
        }
    }
    world.collectibles.ClearRecentBanner();

    world.current_mode = saved_mode;
    world.current_zone = saved_zone;
    world.active_structure = saved_structure;
    if (saved_mode == GameMode::Structure) {
        world.structure.SelectStructure(saved_structure);
        world.structure.OnEnter(world.players, architect_recruited);
    } else {
        world.overworld.OnEnter(world.players, false, architect_recruited);
    }

    if (has_p0_x && has_p0_y) {
        if (Player* p0 = world.players.GetPlayer(0); p0 != nullptr) {
            const Tilemap& map = world.ActiveTilemap();
            const Vec2 safe_pos = FindSafeSpawnNear(
                map,
                saved_p0,
                p0->size,
                (saved_mode == GameMode::Structure) ? world.structure.spawn_point
                                                    : world.overworld.SafeSpawnInZone(saved_zone, architect_recruited),
                architect_recruited);
            p0->pos = safe_pos;
            p0->y_velocity = 0.0f;
            if (saved_mode == GameMode::Structure) {
                world.structure.camera_target = p0->Center();
            } else {
                world.overworld.camera_target = p0->Center();
                world.current_zone = world.overworld.DistrictAt(p0->Center());
            }
        }
    }

    std::array<bool, static_cast<int>(StoryEvent::Count)> fired{};
    for (std::size_t i = 0; i < story_fired_mask.size() && i < fired.size(); ++i) {
        fired[i] = (story_fired_mask[i] == '1');
    }
    world.story.RestoreSavedState(fired, std::move(history), world.BuildStorySnapshot());
    world.loaded_from_save = true;
    world.UpdateProximityPrompts();
    return true;
}

bool DeleteSaveFile(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    std::error_code ec;
    return std::filesystem::remove(path, ec);
}

} // namespace eco
