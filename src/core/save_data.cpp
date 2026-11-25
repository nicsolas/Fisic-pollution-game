#include "core/save_data.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <queue>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "core/physics2d.h"

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

[[nodiscard]] bool IsTileReachableFromSpawn(
    const Tilemap& map,
    Vec2 spawn_pos,
    Vec2 target_pos,
    bool bridge_unlocked) {
    const int rows = map.Rows();
    const int cols = map.Cols();
    if (rows <= 0 || cols <= 0) {
        return false;
    }
    const TileCoord start = map.WorldToTile(spawn_pos);
    const TileCoord goal = map.WorldToTile(target_pos);
    if (!map.InBounds(start.row, start.col) || !map.InBounds(goal.row, goal.col) ||
        map.IsSolid(start.row, start.col, bridge_unlocked) ||
        map.IsSolid(goal.row, goal.col, bridge_unlocked)) {
        return false;
    }
    std::vector<bool> visited(static_cast<std::size_t>(rows * cols), false);
    std::queue<TileCoord> q;
    visited[static_cast<std::size_t>(start.row * cols + start.col)] = true;
    q.push(start);
    static constexpr int kDr[4] = {-1, 1, 0, 0};
    static constexpr int kDc[4] = {0, 0, -1, 1};
    while (!q.empty()) {
        const TileCoord cur = q.front();
        q.pop();
        if (cur == goal) {
            return true;
        }
        for (int d = 0; d < 4; ++d) {
            const int nr = cur.row + kDr[d];
            const int nc = cur.col + kDc[d];
            if (!map.InBounds(nr, nc) || map.IsSolid(nr, nc, bridge_unlocked)) {
                continue;
            }
            const std::size_t idx = static_cast<std::size_t>(nr * cols + nc);
            if (!visited[idx]) {
                visited[idx] = true;
                q.push({nr, nc});
            }
        }
    }
    return false;
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
    out << "guidance_mode=" << static_cast<int>(world.guidance_mode) << "\n";
    out << "slice_inquiry="
        << static_cast<int>(world.slice_inquiry_stage[0]) << "|"
        << static_cast<int>(world.slice_inquiry_stage[1]) << "|"
        << static_cast<int>(world.slice_inquiry_stage[2]) << "\n";
    out << "total_trials=" << world.total_trials_run << "\n";
    out << "slice_synthesis=" << (world.vertical_slice_synthesis_done ? 1 : 0) << "\n";

    // Monumenti (6) — retrocompatibile: primi 3 campi invariati, campi 4..9 per Vertical Slice
    for (std::size_t i = 0; i < world.machines.size(); ++i) {
        const Machine& m = world.machines[i];
        out << "machine_" << i << "="
            << static_cast<int>(m.state) << "|"
            << std::fixed << std::setprecision(3) << m.current_param << "|"
            << (m.unlocked ? 1 : 0) << "|"
            << static_cast<int>(m.approach) << "|"
            << std::fixed << std::setprecision(3) << m.stabilizer_value << "|"
            << m.trials_count << "|"
            << std::fixed << std::setprecision(3) << m.applied_clean_energy << "|"
            << std::fixed << std::setprecision(3) << m.applied_dirty_energy << "|"
            << std::fixed << std::setprecision(3) << m.applied_pollution_delta << "\n";
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

    // Stato delle 6 meccaniche top-down di zona
    out << "mech_solar_gate=" << (world.zone_mechanics.solar_gate_open ? 1 : 0) << "\n";
    {
        std::string plate_bits;
        for (const auto& p : world.zone_mechanics.solar_plates) {
            plate_bits.push_back((p.charge >= 0.99f) ? '1' : '0');
        }
        out << "mech_solar_plates=" << plate_bits << "\n";
    }
    {
        std::string mirror_dirs;
        for (const auto& m : world.zone_mechanics.solar_mirrors) {
            mirror_dirs.push_back(static_cast<char>('0' + ((m.dir_index % 4 + 4) % 4)));
        }
        out << "mech_mirrors=" << mirror_dirs << "\n";
    }
    {
        std::string sluice_bits;
        for (const auto& l : world.zone_mechanics.sluice_levers) {
            sluice_bits.push_back(l.pulled ? '1' : '0');
        }
        out << "mech_sluices=" << sluice_bits << "\n";
    }
    out << "mech_jade_bridge=" << (world.zone_mechanics.jade_bridge_unlocked ? 1 : 0) << "\n";
    out << "mech_resonance_next=" << world.zone_mechanics.resonance_next_order << "\n";
    {
        std::string valve_bits;
        for (const auto& v : world.zone_mechanics.relief_valves) {
            valve_bits.push_back(v.open ? '1' : '0');
        }
        out << "mech_valves=" << valve_bits << "\n";
    }
    {
        std::string seal_bits;
        for (const auto& s : world.zone_mechanics.sanctuary_seals) {
            seal_bits.push_back(s.attuned ? '1' : '0');
        }
        out << "mech_seals=" << seal_bits << "\n";
    }
    {
        std::string obs_bits;
        for (const auto& n : world.zone_mechanics.observation_nodes) {
            obs_bits.push_back(n.inspected ? '1' : '0');
        }
        out << "mech_obs=" << obs_bits << "\n";
    }
    {
        std::string planter_bits;
        for (const auto& bp : world.zone_mechanics.bio_planters) {
            planter_bits.push_back(bp.planted ? '1' : '0');
        }
        out << "mech_planters=" << planter_bits << "\n";
    }
    out << "mech_laminar_ford=" << (world.zone_mechanics.laminar_ford_open ? 1 : 0) << "\n";
    out << "mech_scorched_crest=" << (world.zone_mechanics.scorched_crest_open ? 1 : 0) << "\n";

    // Stato delle leve/interruttori e sequenze nei 3 livelli dei portali (strutture interne)
    for (int s = 0; s < static_cast<int>(StructureId::Count); ++s) {
        const auto sid = static_cast<StructureId>(s);
        const bool use_live =
            (world.current_mode == GameMode::Structure &&
             world.active_structure == sid &&
             !world.structure.puzzle.switches.empty());
        const StructurePuzzleState& pz =
            use_live ? world.structure.puzzle
                     : world.structure.saved_puzzles[static_cast<std::size_t>(s)];
        std::string sw_bits;
        sw_bits.reserve(pz.switches.size());
        for (const auto& sw : pz.switches) {
            sw_bits.push_back(sw.active ? '1' : '0');
        }
        out << "struct_switches_" << s << "=" << sw_bits << "\n";

        out << "struct_seq_" << s << "=";
        for (std::size_t k = 0; k < pz.current_sequence.size(); ++k) {
            if (k > 0) {
                out << ",";
            }
            out << pz.current_sequence[k];
        }
        out << "\n";

        std::string st_obs;
        for (const auto& n : pz.observation_nodes) {
            st_obs.push_back(n.inspected ? '1' : '0');
        }
        out << "struct_obs_" << s << "=" << st_obs << "\n";
        if (sid == StructureId::HypogeanCisterns) {
            out << "struct_aux_valve=" << (pz.aux_stevino_valve_open ? 1 : 0) << "\n";
        }
    }

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
    std::array<CalibrationApproach, 6> m_approaches{};
    std::array<float, 6> m_stabilizers{{0.68f, 0.68f, 0.68f, 0.68f, 0.68f, 0.70f}};
    std::array<int, 6> m_trials{};
    std::array<float, 6> m_app_clean{};
    std::array<float, 6> m_app_dirty{};
    std::array<float, 6> m_app_poll{};
    std::array<bool, 6> m_seen{};

    GuidanceAssistMode saved_guidance = GuidanceAssistMode::Empirical;
    std::array<InquiryStage, 3> saved_inquiry{{
        InquiryStage::Observe, InquiryStage::Observe, InquiryStage::Observe
    }};
    int saved_total_trials = 0;
    bool saved_slice_synthesis = false;
    std::string mech_obs{};
    std::string mech_planters{};
    bool mech_laminar_ford = false;
    bool mech_scorched_crest = false;
    std::array<std::string, static_cast<std::size_t>(StructureId::Count)> struct_obs{};
    bool struct_aux_valve = false;

    std::array<bool, 4> npc_met{};
    std::array<bool, 4> npc_recruited{};
    std::string relics_mask{};
    std::string story_fired_mask{};
    bool mech_solar_gate = false;
    std::string mech_solar_plates{};
    std::string mech_mirrors{};
    std::string mech_sluices{};
    bool mech_jade_bridge = false;
    int mech_resonance_next = 0;
    std::string mech_valves{};
    std::string mech_seals{};
    std::array<std::string, static_cast<std::size_t>(StructureId::Count)> struct_switches{};
    std::array<bool, static_cast<std::size_t>(StructureId::Count)> has_struct_switches{};
    std::array<std::string, static_cast<std::size_t>(StructureId::Count)> struct_seqs{};
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
        } else if (key == "mech_solar_gate") {
            mech_solar_gate = (ParseIntSafe(val, 0) != 0);
        } else if (key == "mech_solar_plates") {
            mech_solar_plates = val;
        } else if (key == "mech_mirrors") {
            mech_mirrors = val;
        } else if (key == "mech_sluices") {
            mech_sluices = val;
        } else if (key == "mech_jade_bridge") {
            mech_jade_bridge = (ParseIntSafe(val, 0) != 0);
        } else if (key == "mech_resonance_next") {
            mech_resonance_next = std::max(0, ParseIntSafe(val, 0));
        } else if (key == "mech_valves") {
            mech_valves = val;
        } else if (key == "mech_seals") {
            mech_seals = val;
        } else if (key == "guidance_mode") {
            const int gm = std::clamp(ParseIntSafe(val, 0), 0, 2);
            saved_guidance = static_cast<GuidanceAssistMode>(gm);
        } else if (key == "slice_inquiry") {
            const auto parts = SplitPipe(val);
            for (std::size_t i = 0; i < parts.size() && i < 3; ++i) {
                const int st = std::clamp(ParseIntSafe(parts[i], 0), 0, 4);
                saved_inquiry[i] = static_cast<InquiryStage>(st);
            }
        } else if (key == "total_trials") {
            saved_total_trials = std::max(0, ParseIntSafe(val, 0));
        } else if (key == "slice_synthesis") {
            saved_slice_synthesis = (ParseIntSafe(val, 0) != 0);
        } else if (key == "mech_obs") {
            mech_obs = val;
        } else if (key == "mech_planters") {
            mech_planters = val;
        } else if (key == "mech_laminar_ford") {
            mech_laminar_ford = (ParseIntSafe(val, 0) != 0);
        } else if (key == "mech_scorched_crest") {
            mech_scorched_crest = (ParseIntSafe(val, 0) != 0);
        } else if (key.rfind("struct_obs_", 0) == 0) {
            const int idx = ParseIntSafe(key.substr(11), -1);
            if (idx >= 0 && idx < static_cast<int>(StructureId::Count)) {
                struct_obs[static_cast<std::size_t>(idx)] = val;
            }
        } else if (key == "struct_aux_valve") {
            struct_aux_valve = (ParseIntSafe(val, 0) != 0);
        } else if (key.rfind("struct_switches_", 0) == 0) {
            const int idx = ParseIntSafe(key.substr(16), -1);
            if (idx >= 0 && idx < static_cast<int>(StructureId::Count)) {
                struct_switches[static_cast<std::size_t>(idx)] = val;
                has_struct_switches[static_cast<std::size_t>(idx)] = true;
            }
        } else if (key.rfind("struct_seq_", 0) == 0) {
            const int idx = ParseIntSafe(key.substr(11), -1);
            if (idx >= 0 && idx < static_cast<int>(StructureId::Count)) {
                struct_seqs[static_cast<std::size_t>(idx)] = val;
            }
        } else if (key.rfind("machine_", 0) == 0) {
            const int idx = ParseIntSafe(key.substr(8), -1);
            if (idx >= 0 && idx < 6) {
                const auto parts = SplitPipe(val);
                if (parts.size() >= 3) {
                    const auto u = static_cast<std::size_t>(idx);
                    const int st = std::clamp(ParseIntSafe(parts[0], 0), 0, 2);
                    m_states[u] = static_cast<MachineState>(st);
                    m_params[u] = ParseFloatSafe(parts[1], 0.0f);
                    m_unlocked[u] = (ParseIntSafe(parts[2], 1) != 0);
                    if (parts.size() >= 4) {
                        const int ap = std::clamp(ParseIntSafe(parts[3], 0), 0, 3);
                        m_approaches[u] = static_cast<CalibrationApproach>(ap);
                    }
                    if (parts.size() >= 5) {
                        m_stabilizers[u] = Clamp(ParseFloatSafe(parts[4], 0.68f), 0.0f, 1.0f);
                    }
                    if (parts.size() >= 6) {
                        m_trials[u] = std::max(0, ParseIntSafe(parts[5], 0));
                    }
                    if (parts.size() >= 9) {
                        m_app_clean[u] = ParseFloatSafe(parts[6], 0.0f);
                        m_app_dirty[u] = ParseFloatSafe(parts[7], 0.0f);
                        m_app_poll[u] = ParseFloatSafe(parts[8], 0.0f);
                    }
                    m_seen[u] = true;
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
    world.guidance_mode = saved_guidance;
    world.slice_inquiry_stage = saved_inquiry;
    world.total_trials_run = saved_total_trials;
    world.vertical_slice_synthesis_done = saved_slice_synthesis;

    for (std::size_t i = 0; i < world.machines.size(); ++i) {
        if (m_seen[i]) {
            world.machines[i].state = m_states[i];
            world.machines[i].current_param = Clamp(
                m_params[i],
                world.machines[i].min_param,
                world.machines[i].max_param);
            world.machines[i].unlocked = m_unlocked[i];
            world.machines[i].approach = m_approaches[i];
            world.machines[i].stabilizer_value = m_stabilizers[i];
            world.machines[i].trials_count = m_trials[i];
            world.machines[i].applied_clean_energy = m_app_clean[i];
            world.machines[i].applied_dirty_energy = m_app_dirty[i];
            world.machines[i].applied_pollution_delta = m_app_poll[i];
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

    // Ripristina stato delle 6 meccaniche top-down di zona
    world.zone_mechanics.solar_gate_open = mech_solar_gate;
    for (std::size_t i = 0; i < mech_solar_plates.size() && i < world.zone_mechanics.solar_plates.size(); ++i) {
        if (mech_solar_plates[i] == '1') {
            world.zone_mechanics.solar_plates[i].charge = 1.0f;
        }
    }
    for (std::size_t i = 0; i < mech_mirrors.size() && i < world.zone_mechanics.solar_mirrors.size(); ++i) {
        if (mech_mirrors[i] >= '0' && mech_mirrors[i] <= '3') {
            world.zone_mechanics.solar_mirrors[i].dir_index = static_cast<int>(mech_mirrors[i] - '0');
        }
    }
    for (std::size_t i = 0; i < mech_sluices.size() && i < world.zone_mechanics.sluice_levers.size(); ++i) {
        const bool pulled = (mech_sluices[i] == '1');
        world.zone_mechanics.sluice_levers[i].pulled = pulled;
        const int grp = world.zone_mechanics.sluice_levers[i].sluice_group;
        for (auto& cur : world.zone_mechanics.water_currents) {
            if (cur.sluice_group == grp) {
                cur.active = !pulled;
            }
        }
    }
    world.zone_mechanics.jade_bridge_unlocked = mech_jade_bridge;
    world.zone_mechanics.jade_bridge_progress = mech_jade_bridge ? 1.0f : 0.0f;
    if (mech_jade_bridge) {
        world.zone_mechanics.resonance_next_order =
            static_cast<int>(world.zone_mechanics.resonance_pads.size());
        for (auto& pad : world.zone_mechanics.resonance_pads) {
            pad.activated = true;
        }
    } else if (mech_resonance_next > 0) {
        const int clamped_next = std::min(
            mech_resonance_next,
            static_cast<int>(world.zone_mechanics.resonance_pads.size()));
        world.zone_mechanics.resonance_next_order = clamped_next;
        for (auto& pad : world.zone_mechanics.resonance_pads) {
            pad.activated = (pad.sequence_order < clamped_next);
        }
    }
    for (std::size_t i = 0; i < mech_valves.size() && i < world.zone_mechanics.relief_valves.size(); ++i) {
        const bool open = (mech_valves[i] == '1');
        world.zone_mechanics.relief_valves[i].open = open;
        if (open) {
            const int grp = world.zone_mechanics.relief_valves[i].valve_group;
            for (auto& g : world.zone_mechanics.geysers) {
                if (g.valve_group == grp) {
                    g.phase = GeyserPhase::Calmed;
                    g.intensity = 0.0f;
                }
            }
        }
    }
    for (std::size_t i = 0; i < mech_seals.size() && i < world.zone_mechanics.sanctuary_seals.size(); ++i) {
        const bool attuned = (mech_seals[i] == '1');
        world.zone_mechanics.sanctuary_seals[i].attuned = attuned;
        world.zone_mechanics.sanctuary_seals[i].glow = attuned ? 1.0f : 0.0f;
    }
    int obs_count = 0;
    for (std::size_t i = 0; i < mech_obs.size() && i < world.zone_mechanics.observation_nodes.size(); ++i) {
        if (mech_obs[i] == '1') {
            world.zone_mechanics.observation_nodes[i].inspected = true;
            ++obs_count;
        }
    }
    int planter_count = 0;
    for (std::size_t i = 0; i < mech_planters.size() && i < world.zone_mechanics.bio_planters.size(); ++i) {
        if (mech_planters[i] == '1') {
            world.zone_mechanics.bio_planters[i].planted = true;
            ++planter_count;
        }
    }
    world.zone_mechanics.laminar_ford_open = mech_laminar_ford;
    world.zone_mechanics.scorched_crest_open = mech_scorched_crest;

    // Ripristina stato delle leve/interruttori e sequenze nei 3 livelli dei portali
    for (int s = 0; s < static_cast<int>(StructureId::Count); ++s) {
        const auto idx = static_cast<std::size_t>(s);
        StructurePuzzleState& pz = world.structure.saved_puzzles[idx];
        pz.ResetForStructure(static_cast<StructureId>(s), kTileSize);
        if (has_struct_switches[idx]) {
            const std::string& bits = struct_switches[idx];
            for (std::size_t i = 0; i < bits.size() && i < pz.switches.size(); ++i) {
                pz.switches[i].active = (bits[i] == '1');
            }
        }
        const std::string& sobs = struct_obs[idx];
        for (std::size_t i = 0; i < sobs.size() && i < pz.observation_nodes.size(); ++i) {
            if (sobs[i] == '1') {
                pz.observation_nodes[i].inspected = true;
                ++obs_count;
            }
        }
        if (static_cast<StructureId>(s) == StructureId::HypogeanCisterns) {
            pz.aux_stevino_valve_open = struct_aux_valve;
        }
        if (!struct_seqs[idx].empty()) {
            std::stringstream ss(struct_seqs[idx]);
            std::string tok;
            while (std::getline(ss, tok, ',')) {
                if (tok.empty()) {
                    continue;
                }
                const int sw_id = ParseIntSafe(tok, -1);
                if (pz.GetSwitch(sw_id) != nullptr) {
                    pz.current_sequence.push_back(sw_id);
                }
            }
            // Allinea gli switch attivi con la sequenza salvata nei puzzle a sequenza
            if (!pz.expected_sequence.empty()) {
                for (auto& sw : pz.switches) {
                    sw.active = false;
                }
                for (const int sw_id : pz.current_sequence) {
                    for (auto& sw : pz.switches) {
                        if (sw.id == sw_id) {
                            sw.active = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    world.observations_inspected_count = obs_count;
    world.planters_seeded_count = planter_count;

    // Evita che SelectStructure/UnloadMemory sovrascrivano saved_puzzles con il puzzle di default
    world.structure.puzzle.switches.clear();

    world.current_mode = saved_mode;
    world.current_zone = saved_zone;
    world.active_structure = saved_structure;
    if (saved_mode == GameMode::Structure) {
        world.overworld.UnloadZoneMemory();
        world.structure.SelectStructure(saved_structure);
        world.structure.OnEnter(world.players, architect_recruited);
    } else {
        world.structure.UnloadMemory();
        world.overworld.LoadZone(saved_zone);
        world.overworld.OnEnter(world.players, false, architect_recruited);
        if (saved_zone == ZoneId::DawnTerraces) {
            world.zone_mechanics.RecomputeSolarBeams(world.overworld.tilemap);
        }
    }
    world.RefreshSystemicWorldState();

    if (has_p0_x && has_p0_y) {
        if (Player* p0 = world.players.GetPlayer(0); p0 != nullptr) {
            const Tilemap& map = world.ActiveTilemap();
            const Vec2 fallback_spawn =
                (saved_mode == GameMode::Structure)
                    ? world.structure.spawn_point
                    : world.overworld.SafeSpawnInZone(saved_zone, architect_recruited);
            Vec2 target_p0 = saved_p0;
            // Protezione anti-softlock per salvataggi legacy senza stato leve di struttura
            if (saved_mode == GameMode::Structure &&
                !has_struct_switches[static_cast<std::size_t>(saved_structure)]) {
                const Vec2 sp_center{
                    world.structure.spawn_point.x + p0->size.x * 0.5f,
                    world.structure.spawn_point.y + p0->size.y * 0.5f
                };
                const Vec2 p0_center{
                    saved_p0.x + p0->size.x * 0.5f,
                    saved_p0.y + p0->size.y * 0.5f
                };
                if (!IsTileReachableFromSpawn(map, sp_center, p0_center, architect_recruited)) {
                    target_p0 = fallback_spawn;
                }
            }
            const Vec2 safe_pos = FindSafeSpawnNear(
                map,
                target_p0,
                p0->size,
                fallback_spawn,
                architect_recruited,
                world.zone_mechanics.solar_gate_open,
                world.zone_mechanics.jade_bridge_unlocked);
            p0->pos = safe_pos;
            p0->y_velocity = 0.0f;
            if (saved_mode == GameMode::Structure) {
                p0->isGrounded = IsSupportedByFloor(p0->pos, p0->size, map, architect_recruited);
                p0->coyote_timer = p0->isGrounded ? kCoyoteTimeDuration : 0.0f;
                world.structure.camera_target = p0->Center();
            } else {
                world.overworld.camera_target = p0->Center();
                world.current_zone = world.overworld.active_zone;
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
