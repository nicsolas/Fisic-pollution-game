#include "app/pause_settings_menu.h"

#include <raylib.h>
#include <rlgl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>

#include "app/audio/audio_engine.h"
#include "app/game_state_manager.h"
#include "app/gpu_selector.h"
#include "app/overworld_state.h"
#include "app/render/palette.h"
#include "app/render/particles.h"
#include "app/render/ui_style.h"
#include "app/story_ui.h"
#include "app/title_state.h"
#include "core/math_types.h"
#include "core/save_data.h"
#include "core/simulation.h"

namespace eco::app {

namespace {

constexpr int kCategoryCount = 4;

struct ResolutionOption {
    int width;
    int height;
    const char* label;
};

constexpr std::array<ResolutionOption, 4> kResolutions = {{
    {1024, 576, "1024 x 576"},
    {1280, 720, "1280 x 720 (HD)"},
    {1600, 900, "1600 x 900 (HD+)"},
    {1920, 1080, "1920 x 1080 (FHD)"}
}};

struct FpsOption {
    int target_fps;
    const char* label;
};

constexpr std::array<FpsOption, 4> kFpsOptions = {{
    {60, "60 FPS"},
    {120, "120 FPS"},
    {144, "144 FPS"},
    {0, "Illimitato"}
}};

struct DialogueSpeedOption {
    float characters_per_second;
    const char* label;
};

constexpr std::array<DialogueSpeedOption, 3> kDialogueSpeedOptions = {{
    {38.0f, "Lento (38 cps)"},
    {58.0f, "Normale (58 cps)"},
    {84.0f, "Rapido (84 cps)"}
}};

constexpr std::array<float, 5> kLegacyVolumeLevels = {0.0f, 0.25f, 0.50f, 0.75f, 1.00f};

struct SettingsPanelLayout {
    int box_w = 880;
    int box_h = 612;
    int bx = (eco::kWindowWidth - 880) / 2;
    int by = (eco::kWindowHeight - 612) / 2;

    [[nodiscard]] Rectangle PanelRect() const noexcept {
        return Rectangle{
            static_cast<float>(bx),
            static_cast<float>(by),
            static_cast<float>(box_w),
            static_cast<float>(box_h)
        };
    }

    [[nodiscard]] Rectangle TabsArea() const noexcept {
        return Rectangle{
            static_cast<float>(bx + 24),
            static_cast<float>(by + 54),
            static_cast<float>(box_w - 48),
            36.0f
        };
    }

    [[nodiscard]] Rectangle TabRect(int idx) const noexcept {
        const Rectangle area = TabsArea();
        const float gap = 8.0f;
        const float tab_w =
            (area.width - gap * static_cast<float>(kCategoryCount - 1)) / static_cast<float>(kCategoryCount);
        return Rectangle{
            area.x + static_cast<float>(idx) * (tab_w + gap),
            area.y,
            tab_w,
            area.height
        };
    }

    [[nodiscard]] Rectangle RowRect(int row) const noexcept {
        return Rectangle{
            static_cast<float>(bx + 24),
            static_cast<float>(by + 104 + row * 48),
            static_cast<float>(box_w - 48),
            42.0f
        };
    }

    [[nodiscard]] Rectangle BackButtonRect() const noexcept {
        return Rectangle{
            static_cast<float>(bx + 24),
            static_cast<float>(by + box_h - 72),
            224.0f,
            38.0f
        };
    }

    [[nodiscard]] Rectangle ResetButtonRect() const noexcept {
        return Rectangle{
            static_cast<float>(bx + 264),
            static_cast<float>(by + box_h - 72),
            234.0f,
            38.0f
        };
    }

    [[nodiscard]] Rectangle ApplyGpuButtonRect() const noexcept {
        return Rectangle{
            static_cast<float>(bx + box_w - 24 - 246),
            static_cast<float>(by + box_h - 72),
            246.0f,
            38.0f
        };
    }
};

[[nodiscard]] int OptionCountForCategory(PauseSettingsMenu::SettingsCategory cat) noexcept {
    switch (cat) {
        case PauseSettingsMenu::SettingsCategory::Game:
            return 3;
        case PauseSettingsMenu::SettingsCategory::Graphics:
            return 6;
        case PauseSettingsMenu::SettingsCategory::Environment:
            return 6;
        case PauseSettingsMenu::SettingsCategory::Audio:
            return 7;
    }
    return 3;
}

[[nodiscard]] bool IsSliderRow(PauseSettingsMenu::SettingsCategory cat, int row) noexcept {
    if (cat == PauseSettingsMenu::SettingsCategory::Environment) {
        return row >= 1 && row <= 5;
    }
    if (cat == PauseSettingsMenu::SettingsCategory::Audio) {
        return row >= 1 && row <= 4;
    }
    return false;
}

[[nodiscard]] float* GetSliderValuePtr(
    GraphicsSettings& settings,
    PauseSettingsMenu::SettingsCategory cat,
    int row) noexcept {
    if (cat == PauseSettingsMenu::SettingsCategory::Environment) {
        switch (row) {
            case 1: return &settings.particle_density;
            case 2: return &settings.fog_haze_intensity;
            case 3: return &settings.sun_rays_intensity;
            case 4: return &settings.vignette_intensity;
            case 5: return &settings.zone_weather_intensity;
            default: return nullptr;
        }
    }
    if (cat == PauseSettingsMenu::SettingsCategory::Audio) {
        switch (row) {
            case 1: return &settings.master_volume;
            case 2: return &settings.music_volume;
            case 3: return &settings.effects_volume;
            case 4: return &settings.ambient_volume;
            default: return nullptr;
        }
    }
    return nullptr;
}

[[nodiscard]] std::string FormatPercent(float v) {
    const int pct = static_cast<int>(std::lround(std::clamp(v, 0.0f, 1.0f) * 100.0f));
    return std::to_string(pct) + "%";
}

[[nodiscard]] bool IsTitleStateActive(const AppContext& ctx) noexcept {
    if (!ctx.state_manager.HasActiveState()) {
        return false;
    }
    return std::string_view(ctx.state_manager.CurrentState()->Name()) == "TitleState";
}

[[nodiscard]] bool IsGameplayStateActive(const AppContext& ctx) noexcept {
    if (!ctx.state_manager.HasActiveState()) {
        return false;
    }
    const std::string_view name(ctx.state_manager.CurrentState()->Name());
    return name == "OverworldState" || name == "StructureState";
}

[[nodiscard]] std::string FormatMonitorLabel(int monitor_index) {
    const int count = IsWindowReady() ? std::max(1, GetMonitorCount()) : 1;
    if (monitor_index < 0 || monitor_index >= count) {
        return "Auto (Corrente)";
    }
    const int mw = IsWindowReady() ? GetMonitorWidth(monitor_index) : 1280;
    const int mh = IsWindowReady() ? GetMonitorHeight(monitor_index) : 720;
    return "Monitor " + std::to_string(monitor_index + 1) + "/" + std::to_string(count) +
           " (" + std::to_string(mw) + "x" + std::to_string(mh) + ")";
}

[[nodiscard]] std::string FormatGpuLabel(const std::string& gpu_id) {
    const auto& list = EnumerateGpuOptions();
    const int idx = FindGpuOptionIndex(gpu_id);
    std::string label = (idx >= 0 && idx < static_cast<int>(list.size()))
                            ? list[static_cast<std::size_t>(idx)].label
                            : std::string("Auto (Sistema)");
    if (label.size() > 24) {
        label = label.substr(0, 22) + "..";
    }
    return label;
}

void PlayAudioFeedbackForRow(
    PauseSettingsMenu::SettingsCategory category,
    int row,
    int dir) {
    const float pitch = (dir > 0) ? 1.05f : ((dir < 0) ? 0.95f : 1.0f);
    if (category == PauseSettingsMenu::SettingsCategory::Audio) {
        switch (row) {
            case 0: // Mute
            case 6: // UI sounds
                PlayAudioPreview(AudioPreviewChannel::Ui, pitch);
                return;
            case 1: // Master
                PlayAudioPreview(AudioPreviewChannel::Master, pitch);
                return;
            case 2: // Music
                PlayAudioPreview(AudioPreviewChannel::Music, pitch);
                return;
            case 3: // Effects
                PlayAudioPreview(AudioPreviewChannel::Effects, pitch);
                return;
            case 4: // Ambient volume
            case 5: // Ambient enabled
                PlayAudioPreview(AudioPreviewChannel::Ambient, pitch);
                return;
            default:
                break;
        }
    }
    PlayUiSfx(SfxId::UiClick, 0.56f, pitch);
}

void SyncLegacyParticleQuality(GraphicsSettings& settings) noexcept {
    if (settings.particle_density <= 0.05f) {
        settings.particle_quality = ParticleQuality::Off;
    } else if (settings.particle_density < 0.75f) {
        settings.particle_quality = ParticleQuality::Medium;
    } else {
        settings.particle_quality = ParticleQuality::High;
    }
}

void ResetCategoryDefaults(
    GraphicsSettings& settings,
    PauseSettingsMenu::SettingsCategory category) {
    switch (category) {
        case PauseSettingsMenu::SettingsCategory::Game:
            settings.compact_tutorial = false;
            settings.show_fps = false;
            settings.dialogue_speed_index = 1;
            break;
        case PauseSettingsMenu::SettingsCategory::Graphics:
            settings.monitor_index = -1;
            settings.resolution_index = 1;
            settings.fps_index = 0;
            settings.clean_ttf_font = true;
            settings.gpu_preference = "auto";
            if (settings.fullscreen) {
                ToggleAppFullscreen(settings);
            } else {
                ApplySafeWindowResolution(settings);
            }
            break;
        case PauseSettingsMenu::SettingsCategory::Environment:
            settings.atmosphere_enabled = true;
            settings.particle_density = 1.0f;
            settings.fog_haze_intensity = 1.0f;
            settings.sun_rays_intensity = 1.0f;
            settings.vignette_intensity = 1.0f;
            settings.zone_weather_intensity = 1.0f;
            SyncLegacyParticleQuality(settings);
            break;
        case PauseSettingsMenu::SettingsCategory::Audio:
            settings.audio_muted = false;
            settings.master_volume = 0.75f;
            settings.music_volume = 0.85f;
            settings.effects_volume = 0.90f;
            settings.ambient_volume = 0.85f;
            settings.ambient_audio = true;
            settings.ui_sounds_enabled = true;
            break;
    }
    ApplyGraphicsSettingsToRuntime(settings);
    (void)SaveGraphicsSettings(settings);
    PlayUiSfx(SfxId::UiClick, 0.65f, 1.12f);
}

void AdjustSettingValue(
    GraphicsSettings& settings,
    PauseSettingsMenu::SettingsCategory category,
    int row,
    int dir) {
    if (dir == 0) {
        return;
    }

    if (float* slider = GetSliderValuePtr(settings, category, row); slider != nullptr) {
        constexpr float kStep = 0.05f;
        const float stepped = std::round((*slider + static_cast<float>(dir) * kStep) * 20.0f) / 20.0f;
        *slider = std::clamp(stepped, 0.0f, 1.0f);
        if (category == PauseSettingsMenu::SettingsCategory::Environment && row == 1) {
            SyncLegacyParticleQuality(settings);
        }
        if (category == PauseSettingsMenu::SettingsCategory::Audio && row == 1 && *slider > 0.001f) {
            settings.audio_muted = false;
        }
        ApplyGraphicsSettingsToRuntime(settings);
        (void)SaveGraphicsSettings(settings);
        PlayAudioFeedbackForRow(category, row, dir);
        return;
    }

    switch (category) {
        case PauseSettingsMenu::SettingsCategory::Game:
            if (row == 0) {
                settings.compact_tutorial = !settings.compact_tutorial;
            } else if (row == 1) {
                settings.show_fps = !settings.show_fps;
            } else if (row == 2) {
                const int n = static_cast<int>(kDialogueSpeedOptions.size());
                settings.dialogue_speed_index = (settings.dialogue_speed_index + dir + n) % n;
            }
            break;
        case PauseSettingsMenu::SettingsCategory::Graphics:
            switch (row) {
                case 0:
                    ToggleAppFullscreen(settings);
                    break;
                case 1: {
                    const int count = IsWindowReady() ? std::max(1, GetMonitorCount()) : 1;
                    const int total_slots = count + 1;
                    int slot = (settings.monitor_index < 0 || settings.monitor_index >= count)
                                   ? 0
                                   : (settings.monitor_index + 1);
                    slot = (slot + dir + total_slots) % total_slots;
                    settings.monitor_index = (slot == 0) ? -1 : (slot - 1);
                    ApplySelectedMonitor(settings);
                    break;
                }
                case 2: {
                    const auto& gpus = EnumerateGpuOptions();
                    const int n = std::max(1, static_cast<int>(gpus.size()));
                    int idx = FindGpuOptionIndex(settings.gpu_preference);
                    idx = (idx + dir + n) % n;
                    settings.gpu_preference = gpus[static_cast<std::size_t>(idx)].id;
                    break;
                }
                case 3: {
                    const int n = static_cast<int>(kResolutions.size());
                    settings.resolution_index = (settings.resolution_index + dir + n) % n;
                    ApplySafeWindowResolution(settings);
                    break;
                }
                case 4: {
                    const int n = static_cast<int>(kFpsOptions.size());
                    settings.fps_index = (settings.fps_index + dir + n) % n;
                    ApplyGraphicsSettingsToRuntime(settings);
                    break;
                }
                case 5:
                    settings.clean_ttf_font = !settings.clean_ttf_font;
                    ApplyGraphicsSettingsToRuntime(settings);
                    break;
                default:
                    break;
            }
            break;
        case PauseSettingsMenu::SettingsCategory::Environment:
            if (row == 0) {
                settings.atmosphere_enabled = !settings.atmosphere_enabled;
                ApplyGraphicsSettingsToRuntime(settings);
            }
            break;
        case PauseSettingsMenu::SettingsCategory::Audio:
            if (row == 0) {
                settings.audio_muted = !settings.audio_muted;
                ApplyGraphicsSettingsToRuntime(settings);
            } else if (row == 5) {
                settings.ambient_audio = !settings.ambient_audio;
                ApplyGraphicsSettingsToRuntime(settings);
            } else if (row == 6) {
                settings.ui_sounds_enabled = !settings.ui_sounds_enabled;
                ApplyGraphicsSettingsToRuntime(settings);
            }
            break;
    }

    (void)SaveGraphicsSettings(settings);
    PlayAudioFeedbackForRow(category, row, dir);
}

[[nodiscard]] int ParseIntSetting(const std::string& s, int fallback) noexcept {
    try {
        return std::stoi(s);
    } catch (...) {
        return fallback;
    }
}

[[nodiscard]] float ParseFloatSetting(const std::string& s, float fallback) noexcept {
    try {
        const float v = std::stof(s);
        return std::isfinite(v) ? v : fallback;
    } catch (...) {
        return fallback;
    }
}

} // namespace

bool LoadGraphicsSettings(GraphicsSettings& settings, const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }
    bool has_particle_density = false;
    bool has_master_float = false;
    bool has_music_float = false;
    bool has_effects_float = false;

    std::string line;
    while (std::getline(in, line)) {
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        const std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        while (!val.empty() && (val.back() == '\r' || val.back() == '\n')) {
            val.pop_back();
        }

        if (key == "fullscreen") {
            settings.fullscreen = (ParseIntSetting(val, 0) != 0);
        } else if (key == "monitor_index") {
            settings.monitor_index = ParseIntSetting(val, -1);
        } else if (key == "resolution_index") {
            settings.resolution_index =
                std::clamp(ParseIntSetting(val, 1), 0, static_cast<int>(kResolutions.size()) - 1);
        } else if (key == "fps_index") {
            settings.fps_index =
                std::clamp(ParseIntSetting(val, 0), 0, static_cast<int>(kFpsOptions.size()) - 1);
        } else if (key == "particle_quality") {
            const int q = std::clamp(ParseIntSetting(val, 0), 0, 2);
            settings.particle_quality = static_cast<ParticleQuality>(q);
            if (!has_particle_density) {
                settings.particle_density = (q == 0) ? 1.0f : ((q == 1) ? 0.5f : 0.0f);
            }
        } else if (key == "clean_ttf_font") {
            settings.clean_ttf_font = (ParseIntSetting(val, 1) != 0);
        } else if (key == "show_fps") {
            settings.show_fps = (ParseIntSetting(val, 0) != 0);
        } else if (key == "compact_tutorial") {
            settings.compact_tutorial = (ParseIntSetting(val, 0) != 0);
        } else if (key == "dialogue_speed_index") {
            settings.dialogue_speed_index =
                std::clamp(ParseIntSetting(val, 1), 0, static_cast<int>(kDialogueSpeedOptions.size()) - 1);
        } else if (key == "gpu_preference") {
            if (!val.empty()) {
                settings.gpu_preference = val;
            }
        } else if (key == "particle_density") {
            settings.particle_density = std::clamp(ParseFloatSetting(val, 1.0f), 0.0f, 1.0f);
            has_particle_density = true;
            SyncLegacyParticleQuality(settings);
        } else if (key == "fog_haze_intensity") {
            settings.fog_haze_intensity = std::clamp(ParseFloatSetting(val, 1.0f), 0.0f, 1.0f);
        } else if (key == "sun_rays_intensity") {
            settings.sun_rays_intensity = std::clamp(ParseFloatSetting(val, 1.0f), 0.0f, 1.0f);
        } else if (key == "vignette_intensity") {
            settings.vignette_intensity = std::clamp(ParseFloatSetting(val, 1.0f), 0.0f, 1.0f);
        } else if (key == "zone_weather_intensity") {
            settings.zone_weather_intensity = std::clamp(ParseFloatSetting(val, 1.0f), 0.0f, 1.0f);
        } else if (key == "atmosphere_enabled") {
            settings.atmosphere_enabled = (ParseIntSetting(val, 1) != 0);
        } else if (key == "audio_muted") {
            settings.audio_muted = (ParseIntSetting(val, 0) != 0);
        } else if (key == "master_volume") {
            settings.master_volume = std::clamp(ParseFloatSetting(val, 0.75f), 0.0f, 1.0f);
            has_master_float = true;
        } else if (key == "music_volume") {
            settings.music_volume = std::clamp(ParseFloatSetting(val, 0.85f), 0.0f, 1.0f);
            has_music_float = true;
        } else if (key == "effects_volume") {
            settings.effects_volume = std::clamp(ParseFloatSetting(val, 0.90f), 0.0f, 1.0f);
            has_effects_float = true;
        } else if (key == "ambient_volume") {
            settings.ambient_volume = std::clamp(ParseFloatSetting(val, 0.85f), 0.0f, 1.0f);
        } else if (key == "ambient_audio") {
            settings.ambient_audio = (ParseIntSetting(val, 1) != 0);
        } else if (key == "ui_sounds_enabled") {
            settings.ui_sounds_enabled = (ParseIntSetting(val, 1) != 0);
        } else if (key == "audio_volume_index" && !has_master_float) {
            const int idx = std::clamp(ParseIntSetting(val, 3), 0, static_cast<int>(kLegacyVolumeLevels.size()) - 1);
            settings.master_volume = kLegacyVolumeLevels[static_cast<std::size_t>(idx)];
            settings.audio_muted = (idx == 0);
        } else if (key == "music_volume_index" && !has_music_float) {
            const int idx = std::clamp(ParseIntSetting(val, 4), 0, static_cast<int>(kLegacyVolumeLevels.size()) - 1);
            settings.music_volume = kLegacyVolumeLevels[static_cast<std::size_t>(idx)];
        } else if (key == "effects_volume_index" && !has_effects_float) {
            const int idx = std::clamp(ParseIntSetting(val, 4), 0, static_cast<int>(kLegacyVolumeLevels.size()) - 1);
            settings.effects_volume = kLegacyVolumeLevels[static_cast<std::size_t>(idx)];
        }
    }
    return true;
}

bool SaveGraphicsSettings(const GraphicsSettings& settings, const std::string& path) {
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path target(path);
    if (target.has_parent_path()) {
        fs::create_directories(target.parent_path(), ec);
    }
    std::ofstream out(target, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    out << "fullscreen=" << (settings.fullscreen ? 1 : 0) << "\n";
    out << "monitor_index=" << settings.monitor_index << "\n";
    out << "resolution_index=" << settings.resolution_index << "\n";
    out << "fps_index=" << settings.fps_index << "\n";
    out << "particle_quality=" << static_cast<int>(settings.particle_quality) << "\n";
    out << "clean_ttf_font=" << (settings.clean_ttf_font ? 1 : 0) << "\n";
    out << "show_fps=" << (settings.show_fps ? 1 : 0) << "\n";
    out << "compact_tutorial=" << (settings.compact_tutorial ? 1 : 0) << "\n";
    out << "dialogue_speed_index=" << settings.dialogue_speed_index << "\n";
    out << "gpu_preference=" << settings.gpu_preference << "\n";
    out << "particle_density=" << settings.particle_density << "\n";
    out << "fog_haze_intensity=" << settings.fog_haze_intensity << "\n";
    out << "sun_rays_intensity=" << settings.sun_rays_intensity << "\n";
    out << "vignette_intensity=" << settings.vignette_intensity << "\n";
    out << "zone_weather_intensity=" << settings.zone_weather_intensity << "\n";
    out << "atmosphere_enabled=" << (settings.atmosphere_enabled ? 1 : 0) << "\n";
    out << "audio_muted=" << (settings.audio_muted ? 1 : 0) << "\n";
    out << "master_volume=" << settings.master_volume << "\n";
    out << "music_volume=" << settings.music_volume << "\n";
    out << "effects_volume=" << settings.effects_volume << "\n";
    out << "ambient_volume=" << settings.ambient_volume << "\n";
    out << "ambient_audio=" << (settings.ambient_audio ? 1 : 0) << "\n";
    out << "ui_sounds_enabled=" << (settings.ui_sounds_enabled ? 1 : 0) << "\n";
    return out.good();
}

int ResolveTargetMonitor(const GraphicsSettings& settings) noexcept {
    if (!IsWindowReady()) {
        return 0;
    }
    const int count = GetMonitorCount();
    if (count <= 0) {
        return 0;
    }
    if (settings.monitor_index >= 0 && settings.monitor_index < count) {
        return settings.monitor_index;
    }
    const int cur = GetCurrentMonitor();
    return (cur >= 0 && cur < count) ? cur : 0;
}

void ApplyGraphicsSettingsToRuntime(const GraphicsSettings& settings) {
    SetUiFontCleanMode(settings.clean_ttf_font);

    EnvironmentEffectsConfig env_cfg{};
    env_cfg.particle_density = std::clamp(settings.particle_density, 0.0f, 1.0f);
    env_cfg.fog_haze_intensity = std::clamp(settings.fog_haze_intensity, 0.0f, 1.0f);
    env_cfg.sun_rays_intensity = std::clamp(settings.sun_rays_intensity, 0.0f, 1.0f);
    env_cfg.vignette_intensity = std::clamp(settings.vignette_intensity, 0.0f, 1.0f);
    env_cfg.zone_weather_intensity = std::clamp(settings.zone_weather_intensity, 0.0f, 1.0f);
    env_cfg.atmosphere_enabled = settings.atmosphere_enabled;
    SetEnvironmentEffectsConfig(env_cfg);

    ApplyGlobalAudioSettings(
        std::clamp(settings.master_volume, 0.0f, 1.0f),
        settings.audio_muted,
        settings.ambient_audio,
        std::clamp(settings.music_volume, 0.0f, 1.0f),
        std::clamp(settings.effects_volume, 0.0f, 1.0f),
        std::clamp(settings.ambient_volume, 0.0f, 1.0f),
        settings.ui_sounds_enabled);

    if (IsWindowReady()) {
        const int idx = std::clamp(settings.fps_index, 0, static_cast<int>(kFpsOptions.size()) - 1);
        SetTargetFPS(kFpsOptions[static_cast<std::size_t>(idx)].target_fps);
    }
}

void ApplySelectedMonitor(GraphicsSettings& settings) {
    if (!IsWindowReady()) {
        return;
    }
    const int target_mon = ResolveTargetMonitor(settings);
    SetWindowMonitor(target_mon);
}

void ApplySafeWindowResolution(GraphicsSettings& settings) {
    if (!IsWindowReady()) {
        return;
    }

    const int idx = std::clamp(settings.resolution_index, 0, static_cast<int>(kResolutions.size()) - 1);
    const auto& res = kResolutions[static_cast<std::size_t>(idx)];

    const int mon = ResolveTargetMonitor(settings);
    const int mon_w = GetMonitorWidth(mon);
    const int mon_h = GetMonitorHeight(mon);

    if (mon_w > 0 && mon_h > 0 && (res.width >= mon_w || res.height >= mon_h - 32)) {
        if (!IsWindowFullscreen()) {
            ToggleFullscreen();
        }
        SetWindowMonitor(mon);
        settings.fullscreen = true;
        return;
    }

    if (IsWindowFullscreen()) {
        ToggleFullscreen();
    }
    if (IsWindowMaximized()) {
        RestoreWindow();
    }
    settings.fullscreen = false;
    SetWindowSize(res.width, res.height);
    if (settings.monitor_index >= 0) {
        SetWindowMonitor(mon);
    }
}

void ToggleAppFullscreen(GraphicsSettings& settings) {
    settings.fullscreen = !settings.fullscreen;
    if (!IsWindowReady()) {
        return;
    }

    const int mon = ResolveTargetMonitor(settings);
    const int mon_w = GetMonitorWidth(mon);
    const int mon_h = GetMonitorHeight(mon);

    if (settings.fullscreen) {
        if (!IsWindowFullscreen()) {
            ToggleFullscreen();
        }
        SetWindowMonitor(mon);
    } else {
        if (IsWindowFullscreen()) {
            ToggleFullscreen();
        }
        const int idx = std::clamp(settings.resolution_index, 0, static_cast<int>(kResolutions.size()) - 1);
        const auto& res = kResolutions[static_cast<std::size_t>(idx)];
        const int safe_w = (mon_w > 0 && res.width >= mon_w) ? 1280 : res.width;
        const int safe_h = (mon_h > 0 && res.height >= mon_h - 32) ? 720 : res.height;
        SetWindowSize(safe_w, safe_h);
        if (settings.monitor_index >= 0) {
            SetWindowMonitor(mon);
        }
    }
    (void)SaveGraphicsSettings(settings);
}

void PauseSettingsMenu::OpenPause() noexcept {
    is_open_ = true;
    in_settings_ = false;
    opened_from_title_ = false;
    pause_cursor_ = 0;
    dragging_slider_row_ = -1;
}

void PauseSettingsMenu::OpenSettingsDirect() noexcept {
    is_open_ = true;
    in_settings_ = true;
    opened_from_title_ = true;
    settings_cursor_ = 0;
    dragging_slider_row_ = -1;
}

void PauseSettingsMenu::Close() noexcept {
    is_open_ = false;
    in_settings_ = false;
    opened_from_title_ = false;
    dragging_slider_row_ = -1;
}

void PauseSettingsMenu::TriggerSaveFeedback(const char* message) noexcept {
    save_feedback_timer_ = 2.6f;
    save_feedback_msg_ = (message != nullptr) ? std::string(message) : std::string("Partita salvata!");
}

bool PauseSettingsMenu::UpdateAndHandleInput(
    AppContext& ctx,
    GraphicsSettings& settings,
    JournalOverlay* journal) {
    const int dialogue_speed_idx = std::clamp(
        settings.dialogue_speed_index,
        0,
        static_cast<int>(kDialogueSpeedOptions.size()) - 1);
    ctx.world.story.SetTypewriterCharactersPerSecond(
        kDialogueSpeedOptions[static_cast<std::size_t>(dialogue_speed_idx)].characters_per_second);

    if (save_feedback_timer_ > 0.0f) {
        save_feedback_timer_ = std::max(0.0f, save_feedback_timer_ - GetFrameTime());
    }

    if (ctx.open_settings_requested) {
        ctx.open_settings_requested = false;
        OpenSettingsDirect();
        return true;
    }

    // Scorciatoia globale F11 o Alt+Invio per Schermo Intero sul monitor scelto
    if (IsKeyPressed(KEY_F11) ||
        ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER))) {
        ToggleAppFullscreen(settings);
    }

    // Scorciatoia globale M per silenziare/riattivare preservando il volume impostato
    if (IsKeyPressed(KEY_M)) {
        settings.audio_muted = !settings.audio_muted;
        ApplyGraphicsSettingsToRuntime(settings);
        (void)SaveGraphicsSettings(settings);
        if (!settings.audio_muted) {
            PlayAudioPreview(AudioPreviewChannel::Ui, 1.08f);
        }
    }

    // Avanzamento esplicito dei dialoghi (non durante intramezzi o menu)
    if (!is_open_ &&
        !ctx.world.interlude.active &&
        (journal == nullptr || !journal->IsOpen()) &&
        !ctx.world.calibration_modal.is_open &&
        ctx.world.story.HasActiveDialogue() &&
        (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) ||
         IsKeyPressed(KEY_E) || IsKeyPressed(KEY_SPACE)) &&
        ctx.world.story.LineElapsed() > 0.12f) {
        ctx.world.story.AdvanceDialogue();
        PlayUiSfx(SfxId::UiClick, 0.35f, 1.12f);
    }

    // Avanzamento rapido dei dialoghi se il giocatore attiva il ritmo sintetico
    if (!is_open_ &&
        !ctx.world.interlude.active &&
        (journal == nullptr || !journal->IsOpen()) &&
        settings.compact_tutorial && ctx.world.story.HasActiveDialogue() &&
        ctx.world.story.LineElapsed() > 3.2f) {
        ctx.world.story.AdvanceDialogue();
    }

    const bool on_title = IsTitleStateActive(ctx);

    // Priorita' ESC: Intramezzo (salta) -> Calibrazione -> Diario [H/J] -> Menu di Pausa / Impostazioni
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (!is_open_) {
            if (ctx.world.interlude.active) {
                if (ctx.world.interlude.CanSkip()) {
                    ctx.world.SkipZoneInterlude();
                    PlayUiSfx(SfxId::UiClick, 0.45f, 1.0f);
                }
                return false;
            }
            if (ctx.world.calibration_modal.is_open) {
                ctx.world.calibration_modal.Cancel();
                return false;
            }
            if (journal != nullptr && journal->IsOpen()) {
                journal->Close();
                return false;
            }
            if (on_title) {
                OpenSettingsDirect();
            } else {
                OpenPause();
            }
            return true;
        }

        dragging_slider_row_ = -1;
        if (in_settings_) {
            if (opened_from_title_) {
                Close();
                return false;
            }
            in_settings_ = false;
            return true;
        }

        Close();
        return false;
    }

    if (!is_open_ && on_title && IsKeyPressed(KEY_O)) {
        OpenSettingsDirect();
        return true;
    }

    if (!is_open_) {
        return false;
    }

    const Vector2 mouse = GetMousePosition();
    const bool mouse_clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    const bool mouse_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    const bool mouse_released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    const bool mouse_moved = (GetMouseDelta().x != 0.0f || GetMouseDelta().y != 0.0f);

    const bool up_pressed = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
    const bool down_pressed = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
    const bool left_pressed = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
    const bool right_pressed = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
    const bool confirm_pressed =
        IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE);

    if (!in_settings_) {
        // --- MENU DI PAUSA (7 Voci) ---
        constexpr int kPauseItemCount = 7;
        if (up_pressed) {
            pause_cursor_ = (pause_cursor_ - 1 + kPauseItemCount) % kPauseItemCount;
            PlayUiSfx(SfxId::UiClick, 0.45f, 1.04f);
        }
        if (down_pressed) {
            pause_cursor_ = (pause_cursor_ + 1) % kPauseItemCount;
            PlayUiSfx(SfxId::UiClick, 0.45f, 0.96f);
        }

        const int box_w = 560;
        const int box_h = 480;
        const int bx = (eco::kWindowWidth - box_w) / 2;
        const int by = (eco::kWindowHeight - box_h) / 2;

        hovered_pause_row_ = -1;
        int activated_action = -1;
        for (int i = 0; i < kPauseItemCount; ++i) {
            const Rectangle btn_rect{
                static_cast<float>(bx + 36),
                static_cast<float>(by + 68 + i * 52),
                static_cast<float>(box_w - 72),
                42.0f
            };
            if (CheckCollisionPointRec(mouse, btn_rect)) {
                hovered_pause_row_ = i;
                if (mouse_moved || mouse_clicked) {
                    pause_cursor_ = i;
                }
                if (mouse_clicked) {
                    activated_action = i;
                }
            }
        }

        if (confirm_pressed || IsKeyPressed(KEY_E)) {
            activated_action = pause_cursor_;
        }

        switch (activated_action) {
            case 0: // Riprendi Esplorazione
                Close();
                return false;
            case 1: // Salva Partita Ora
                if (eco::SaveWorldToFile(ctx.world)) {
                    TriggerSaveFeedback("Partita salvata nella Conca di Meridia!");
                    PlayUiSfx(SfxId::UiClick, 0.72f, 1.16f);
                } else {
                    TriggerSaveFeedback("Errore durante il salvataggio.");
                }
                break;
            case 2: // Diario: Cronologia & Codex [H / J]
                Close();
                if (journal != nullptr) {
                    journal->OpenHistory();
                }
                return false;
            case 3: // Impostazioni
                in_settings_ = true;
                settings_cursor_ = 0;
                dragging_slider_row_ = -1;
                PlayUiSfx(SfxId::UiClick, 0.55f, 1.06f);
                break;
            case 4: // Ricomincia Viaggio (Reset Conca)
                ctx.world.Reset();
                ctx.world.EnsureInitialized();
                ctx.state_manager.RequestTransition(std::make_unique<OverworldState>(ctx));
                Close();
                return false;
            case 5: // Salva e Torna alla Schermata del Titolo
                if (IsGameplayStateActive(ctx)) {
                    (void)eco::SaveWorldToFile(ctx.world);
                }
                ctx.state_manager.RequestTransition(std::make_unique<TitleState>(ctx));
                Close();
                return false;
            case 6: // Salva ed Esci dal Gioco
                if (IsGameplayStateActive(ctx)) {
                    (void)eco::SaveWorldToFile(ctx.world);
                }
                ctx.quit_requested = true;
                Close();
                return false;
            default:
                break;
        }
        return is_open_;
    }

    // --- SOTTOMENU IMPOSTAZIONI A 4 SCHEDE (GIOCO / GRAFICA / AMBIENTE / AUDIO) ---
    const SettingsPanelLayout layout{};

    // Cambio scheda pulito: [Q/E], [TAB]/[SHIFT+TAB], [1-4], oppure click mouse (mai A/D!)
    auto switch_category = [&](int new_cat) {
        const int clamped = ((new_cat % kCategoryCount) + kCategoryCount) % kCategoryCount;
        if (clamped != static_cast<int>(settings_category_)) {
            settings_category_ = static_cast<SettingsCategory>(clamped);
            settings_cursor_ = 0;
            dragging_slider_row_ = -1;
            PlayUiSfx(SfxId::UiClick, 0.48f, 1.04f);
        }
    };

    if (IsKeyPressed(KEY_Q)) {
        switch_category(static_cast<int>(settings_category_) - 1);
    } else if (IsKeyPressed(KEY_E)) {
        switch_category(static_cast<int>(settings_category_) + 1);
    } else if (IsKeyPressed(KEY_TAB)) {
        const bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        switch_category(static_cast<int>(settings_category_) + (shift ? -1 : 1));
    } else if (IsKeyPressed(KEY_ONE)) {
        switch_category(0);
    } else if (IsKeyPressed(KEY_TWO)) {
        switch_category(1);
    } else if (IsKeyPressed(KEY_THREE)) {
        switch_category(2);
    } else if (IsKeyPressed(KEY_FOUR)) {
        switch_category(3);
    }

    // Aggiorna conteggi se la categoria e' appena cambiata
    const int cur_option_count = OptionCountForCategory(settings_category_);
    const bool cur_gpu_changed =
        settings_category_ == SettingsCategory::Graphics &&
        settings.gpu_preference != settings.initial_gpu_preference;
    const int cur_back_idx = cur_option_count;
    const int cur_reset_idx = cur_option_count + 1;
    const int cur_apply_idx = cur_option_count + 2;
    const int cur_total_rows = cur_gpu_changed ? (cur_option_count + 3) : (cur_option_count + 2);
    settings_cursor_ = std::clamp(settings_cursor_, 0, cur_total_rows - 1);

    // Gestione drag continuo dello slider con mouse
    if (dragging_slider_row_ >= 0) {
        if (mouse_down && dragging_slider_row_ < cur_option_count) {
            if (float* slider_val = GetSliderValuePtr(settings, settings_category_, dragging_slider_row_);
                slider_val != nullptr) {
                const Rectangle track = SliderTrackRect(layout.RowRect(dragging_slider_row_));
                const float ratio = std::clamp((mouse.x - track.x) / std::max(1.0f, track.width), 0.0f, 1.0f);
                const float snapped = std::round(ratio * 20.0f) / 20.0f;
                if (std::abs(snapped - *slider_val) > 1e-4f) {
                    *slider_val = snapped;
                    if (settings_category_ == SettingsCategory::Environment && dragging_slider_row_ == 1) {
                        SyncLegacyParticleQuality(settings);
                    }
                    if (settings_category_ == SettingsCategory::Audio && dragging_slider_row_ == 1 && snapped > 0.001f) {
                        settings.audio_muted = false;
                    }
                    ApplyGraphicsSettingsToRuntime(settings);
                }
            }
        }
        if (mouse_released || !mouse_down) {
            (void)SaveGraphicsSettings(settings);
            PlayAudioFeedbackForRow(settings_category_, dragging_slider_row_, 0);
            dragging_slider_row_ = -1;
        }
        return is_open_;
    }

    // Navigazione tastiera righe
    if (up_pressed) {
        settings_cursor_ = (settings_cursor_ - 1 + cur_total_rows) % cur_total_rows;
        PlayUiSfx(SfxId::UiClick, 0.45f, 1.04f);
    }
    if (down_pressed) {
        settings_cursor_ = (settings_cursor_ + 1) % cur_total_rows;
        PlayUiSfx(SfxId::UiClick, 0.45f, 0.96f);
    }

    // Regolazione tastiera opzione / pulsanti in basso
    if (settings_cursor_ < cur_option_count) {
        if (left_pressed) {
            AdjustSettingValue(settings, settings_category_, settings_cursor_, -1);
        } else if (right_pressed || confirm_pressed) {
            AdjustSettingValue(settings, settings_category_, settings_cursor_, +1);
        }
    } else {
        // Navigazione orizzontale sui pulsanti in basso
        if (left_pressed) {
            if (settings_cursor_ > cur_back_idx) {
                --settings_cursor_;
                PlayUiSfx(SfxId::UiClick, 0.42f, 0.96f);
            }
        } else if (right_pressed) {
            if (settings_cursor_ + 1 < cur_total_rows) {
                ++settings_cursor_;
                PlayUiSfx(SfxId::UiClick, 0.42f, 1.04f);
            }
        } else if (confirm_pressed) {
            if (settings_cursor_ == cur_back_idx) {
                PlayUiSfx(SfxId::UiClick, 0.55f, 0.94f);
                if (opened_from_title_) {
                    Close();
                    return false;
                }
                in_settings_ = false;
                return true;
            }
            if (settings_cursor_ == cur_reset_idx) {
                ResetCategoryDefaults(settings, settings_category_);
                TriggerSaveFeedback("Predefiniti della scheda ripristinati.");
                return true;
            }
            if (cur_gpu_changed && settings_cursor_ == cur_apply_idx) {
                (void)SaveGraphicsSettings(settings);
                if (IsGameplayStateActive(ctx)) {
                    (void)eco::SaveWorldToFile(ctx.world);
                }
                ctx.relaunch_gpu_requested = true;
                return false;
            }
        }
    }

    // Mouse sulle schede
    hovered_tab_ = -1;
    for (int tab = 0; tab < kCategoryCount; ++tab) {
        if (CheckCollisionPointRec(mouse, layout.TabRect(tab))) {
            hovered_tab_ = tab;
            if (mouse_clicked) {
                switch_category(tab);
                return true;
            }
        }
    }

    // Mouse sulle righe delle opzioni
    hovered_settings_row_ = -1;
    for (int i = 0; i < cur_option_count; ++i) {
        const Rectangle row_rect = layout.RowRect(i);
        if (CheckCollisionPointRec(mouse, row_rect)) {
            hovered_settings_row_ = i;
            if (mouse_moved || mouse_clicked) {
                settings_cursor_ = i;
            }
            if (mouse_clicked) {
                if (IsSliderRow(settings_category_, i)) {
                    const Rectangle track = SliderTrackRect(row_rect);
                    const Rectangle grab_rect{
                        track.x - 10.0f,
                        row_rect.y + 4.0f,
                        track.width + 20.0f,
                        row_rect.height - 8.0f
                    };
                    if (CheckCollisionPointRec(mouse, grab_rect)) {
                        dragging_slider_row_ = i;
                        if (float* slider_val = GetSliderValuePtr(settings, settings_category_, i);
                            slider_val != nullptr) {
                            const float ratio = std::clamp((mouse.x - track.x) / std::max(1.0f, track.width), 0.0f, 1.0f);
                            *slider_val = std::round(ratio * 20.0f) / 20.0f;
                            if (settings_category_ == SettingsCategory::Environment && i == 1) {
                                SyncLegacyParticleQuality(settings);
                            }
                            if (settings_category_ == SettingsCategory::Audio && i == 1 && *slider_val > 0.001f) {
                                settings.audio_muted = false;
                            }
                            ApplyGraphicsSettingsToRuntime(settings);
                        }
                    } else {
                        const float mid_x = row_rect.x + row_rect.width * 0.5f;
                        AdjustSettingValue(settings, settings_category_, i, (mouse.x < mid_x) ? -1 : +1);
                    }
                } else {
                    const float choice_mid = row_rect.x + row_rect.width - 136.0f;
                    AdjustSettingValue(settings, settings_category_, i, (mouse.x < choice_mid) ? -1 : +1);
                }
            }
        }
    }

    // Mouse sui pulsanti in basso (Indietro, Predefiniti, Applica GPU)
    const Rectangle back_rect = layout.BackButtonRect();
    if (CheckCollisionPointRec(mouse, back_rect)) {
        hovered_settings_row_ = cur_back_idx;
        if (mouse_moved || mouse_clicked) {
            settings_cursor_ = cur_back_idx;
        }
        if (mouse_clicked) {
            PlayUiSfx(SfxId::UiClick, 0.55f, 0.94f);
            if (opened_from_title_) {
                Close();
                return false;
            }
            in_settings_ = false;
            return true;
        }
    }

    const Rectangle reset_rect = layout.ResetButtonRect();
    if (CheckCollisionPointRec(mouse, reset_rect)) {
        hovered_settings_row_ = cur_reset_idx;
        if (mouse_moved || mouse_clicked) {
            settings_cursor_ = cur_reset_idx;
        }
        if (mouse_clicked) {
            ResetCategoryDefaults(settings, settings_category_);
            TriggerSaveFeedback("Predefiniti della scheda ripristinati.");
            return true;
        }
    }

    if (cur_gpu_changed) {
        const Rectangle apply_rect = layout.ApplyGpuButtonRect();
        if (CheckCollisionPointRec(mouse, apply_rect)) {
            hovered_settings_row_ = cur_apply_idx;
            if (mouse_moved || mouse_clicked) {
                settings_cursor_ = cur_apply_idx;
            }
            if (mouse_clicked) {
                (void)SaveGraphicsSettings(settings);
                if (IsGameplayStateActive(ctx)) {
                    (void)eco::SaveWorldToFile(ctx.world);
                }
                ctx.relaunch_gpu_requested = true;
                return false;
            }
        }
    }

    return is_open_;
}

void PauseSettingsMenu::DrawOverlay(const AppContext& ctx, const GraphicsSettings& settings) const {
    // 1. Contatore FPS opzionale in alto a destra
    if (settings.show_fps) {
        const char* fps_text = TextFormat("FPS: %d", GetFPS());
        const int tw = MeasureUiText(fps_text, 13);
        const Rectangle fps_box{
            static_cast<float>(eco::kWindowWidth - tw - 28),
            8.0f,
            static_cast<float>(tw + 16),
            24.0f
        };
        DrawRectangleRec(fps_box, Color{10, 22, 32, 230});
        DrawRectangleLinesEx(fps_box, 1.2f, pal::kAccentClean);
        DrawTextShadow(fps_text, static_cast<int>(fps_box.x) + 8, 12, 13, pal::kAccentClean);
    }

    // Toast di conferma salvataggio / predefiniti
    if (save_feedback_timer_ > 0.0f && !save_feedback_msg_.empty()) {
        const float alpha = std::clamp(save_feedback_timer_ * 1.4f, 0.0f, 1.0f);
        DrawBannerToast(
            Rectangle{430.0f, 16.0f, 420.0f, 46.0f},
            "ARCHIVIO CONCA",
            save_feedback_msg_.c_str(),
            nullptr,
            pal::kJadeGlow,
            alpha);
    }

    if (!is_open_) {
        return;
    }

    // 2. Oscuramento morbido dello sfondo di gioco in pausa
    DrawRectangle(0, 0, eco::kWindowWidth, eco::kWindowHeight, Color{5, 10, 18, 202});

    if (!in_settings_) {
        // --- DISEGNO MENU DI PAUSA ---
        const int box_w = 560;
        const int box_h = 480;
        const int bx = (eco::kWindowWidth - box_w) / 2;
        const int by = (eco::kWindowHeight - box_h) / 2;

        DrawPanelWithHeader(
            Rectangle{
                static_cast<float>(bx),
                static_cast<float>(by),
                static_cast<float>(box_w),
                static_cast<float>(box_h)},
            44,
            "PAUSA  |  LA CONCA DI MERIDIA",
            pal::kUiBorderBronze);

        constexpr std::array<const char*, 7> kLabels = {
            "Riprendi Esplorazione [ESC]",
            "Salva Partita Ora",
            "Diario: Cronologia & Codex [H / J]",
            "Impostazioni (Grafica, Ambiente & Audio)",
            "Ricomincia Viaggio (Reset Conca)",
            "Salva e Torna al Titolo",
            "Salva ed Esci dal Gioco"
        };

        for (int i = 0; i < static_cast<int>(kLabels.size()); ++i) {
            const bool selected = (pause_cursor_ == i);
            const bool hovered = (hovered_pause_row_ == i);
            const Rectangle btn{
                static_cast<float>(bx + 36),
                static_cast<float>(by + 68 + i * 52),
                static_cast<float>(box_w - 72),
                42.0f
            };
            const Color accent = (i == 6)
                                     ? Color{236, 108, 76, 255}
                                     : ((i == 1) ? pal::kJadeGlow
                                                 : ((i == 2) ? pal::kBronzeBright : pal::kAccentClean));
            DrawActionButton(btn, kLabels[static_cast<std::size_t>(i)], accent, selected, hovered, i == 0);
        }

        const char* hint = "[W/S] o Mouse Scegli  |  [INVIO] Conferma  |  [M] Muto  |  [F11] Schermo Intero";
        const int hw = MeasureUiText(hint, 12);
        DrawTextShadow(hint, bx + (box_w - hw) / 2, by + box_h - 24, 12, Color{154, 192, 208, 225});
        return;
    }

    // --- DISEGNO SOTTOMENU IMPOSTAZIONI A 4 SCHEDE ---
    const SettingsPanelLayout layout{};
    DrawPanelWithHeader(
        layout.PanelRect(),
        44,
        "IMPOSTAZIONI  |  SCHERMO, AMBIENTE & AUDIO",
        pal::kAccentClean);

    const char* tab_labels[kCategoryCount] = {
        "1. GIOCO",
        "2. GRAFICA",
        "3. AMBIENTE",
        "4. AUDIO"
    };
    DrawCategoryTabs(
        layout.TabsArea(),
        tab_labels,
        kCategoryCount,
        static_cast<int>(settings_category_),
        hovered_tab_,
        pal::kAccentClean);

    const int res_idx = std::clamp(settings.resolution_index, 0, static_cast<int>(kResolutions.size()) - 1);
    const int fps_idx = std::clamp(settings.fps_index, 0, static_cast<int>(kFpsOptions.size()) - 1);
    const int dialogue_speed_idx = std::clamp(
        settings.dialogue_speed_index,
        0,
        static_cast<int>(kDialogueSpeedOptions.size()) - 1);

    const std::string monitor_str = FormatMonitorLabel(settings.monitor_index);
    const std::string gpu_str = FormatGpuLabel(settings.gpu_preference);

    auto draw_toggle = [&](int row, const char* label, bool enabled, const char* state_text, Color accent) {
        DrawToggleRow(
            layout.RowRect(row),
            label,
            enabled,
            state_text,
            accent,
            settings_cursor_ == row,
            hovered_settings_row_ == row);
    };

    auto draw_choice = [&](int row, const char* label, const char* val_text, Color accent) {
        DrawChoiceRow(
            layout.RowRect(row),
            label,
            val_text,
            accent,
            settings_cursor_ == row,
            hovered_settings_row_ == row);
    };

    auto draw_slider = [&](int row, const char* label, float val01, Color accent) {
        const std::string pct = FormatPercent(val01);
        DrawSliderRow(
            layout.RowRect(row),
            label,
            val01,
            pct.c_str(),
            accent,
            settings_cursor_ == row,
            hovered_settings_row_ == row || dragging_slider_row_ == row);
    };

    if (settings_category_ == SettingsCategory::Game) {
        draw_toggle(
            0,
            "Avanzamento Rapido Dialoghi",
            settings.compact_tutorial,
            settings.compact_tutorial ? "Sintetico (3,2 s)" : "Manuale",
            pal::kAccentClean);
        draw_toggle(
            1,
            "Contatore FPS in Sovrimpressione",
            settings.show_fps,
            settings.show_fps ? "Visibile" : "Nascosto",
            pal::kAccentClean);
        draw_choice(
            2,
            "Velocita' Scrittura Dialoghi",
            kDialogueSpeedOptions[static_cast<std::size_t>(dialogue_speed_idx)].label,
            pal::kBronzeBright);

        // Info leggera memoria zone separate (diagnostica utile)
        const eco::MemoryMetrics mem = ctx.world.GetMemoryMetrics();
        const char* mem_info = TextFormat(
            "Memoria Mappe Residente: %zu B  (Risparmiati: %zu B  |  Cambi Zona: %d)",
            mem.total_resident_map_bytes,
            mem.saved_unloaded_bytes,
            mem.zone_swaps_count);
        DrawTextShadow(mem_info, layout.bx + 26, layout.by + layout.box_h - 104, 13, Color{146, 198, 212, 225});
    } else if (settings_category_ == SettingsCategory::Graphics) {
        draw_toggle(
            0,
            "Modalita' Schermo Intero [F11]",
            settings.fullscreen,
            settings.fullscreen ? "Schermo Intero" : "Finestra",
            pal::kAccentClean);
        draw_choice(1, "Monitor Schermo Intero", monitor_str.c_str(), pal::kBronzeBright);
        draw_choice(2, "GPU / Scheda Grafica", gpu_str.c_str(), pal::kBronzeBright);
        draw_choice(
            3,
            "Risoluzione Finestra",
            kResolutions[static_cast<std::size_t>(res_idx)].label,
            pal::kBronzeBright);
        draw_choice(
            4,
            "Limite Fotogrammi (FPS)",
            kFpsOptions[static_cast<std::size_t>(fps_idx)].label,
            pal::kBronzeBright);
        draw_toggle(
            5,
            "Font Interfaccia Nitido (TTF)",
            settings.clean_ttf_font,
            settings.clean_ttf_font ? "Noto Sans HD" : "Pixel Retro",
            pal::kJadeGlow);

        const bool gpu_changed = settings.gpu_preference != settings.initial_gpu_preference;
        const std::string gpu_status =
            std::string("GPU Attiva Ora: ") +
            (settings.active_gpu_renderer.empty() ? std::string("OpenGL") : settings.active_gpu_renderer) +
            (gpu_changed ? "  [Premi 'Applica GPU & Riavvia']" : "");
        DrawTextShadow(
            gpu_status.c_str(),
            layout.bx + 26,
            layout.by + layout.box_h - 104,
            13,
            gpu_changed ? pal::kAmberGlow : Color{148, 206, 214, 230});
    } else if (settings_category_ == SettingsCategory::Environment) {
        draw_toggle(
            0,
            "Effetti Atmosferici Screen-Space",
            settings.atmosphere_enabled,
            settings.atmosphere_enabled ? "Attivi" : "Disattivati",
            pal::kJadeGlow);
        draw_slider(1, "Densita' Particelle Globali", settings.particle_density, pal::kAccentClean);
        draw_slider(2, "Intensita' Foschia & Nebbia", settings.fog_haze_intensity, pal::kAmberGlow);
        draw_slider(3, "Raggi Solari & Luce Volumetrica", settings.sun_rays_intensity, pal::kBronzeBright);
        draw_slider(4, "Vignettatura Atmosferica", settings.vignette_intensity, pal::kAccentClean);
        draw_slider(5, "Meteo & Effetti di Zona (Vento / Vapore)", settings.zone_weather_intensity, pal::kJadeGlow);

        DrawTextShadow(
            "Le modifiche agli effetti ambientali si applicano in tempo reale sulla Conca di Meridia.",
            layout.bx + 26,
            layout.by + layout.box_h - 104,
            13,
            Color{154, 206, 214, 225});
    } else {
        draw_toggle(
            0,
            "Audio Silenziato [M]",
            settings.audio_muted,
            settings.audio_muted ? "Muto" : "Attivo",
            settings.audio_muted ? Color{236, 114, 82, 255} : pal::kJadeGlow);
        draw_slider(1, "Volume Generale", settings.master_volume, pal::kAccentClean);
        draw_slider(2, "Volume Musica", settings.music_volume, pal::kBronzeBright);
        draw_slider(3, "Volume Effetti & Meccaniche", settings.effects_volume, pal::kJadeGlow);
        draw_slider(4, "Volume Ambiente & Natura", settings.ambient_volume, pal::kAccentClean);
        draw_toggle(
            5,
            "Paesaggio Sonoro Ambientale",
            settings.ambient_audio,
            settings.ambient_audio ? "Attivo" : "Solo Effetti",
            pal::kJadeGlow);
        draw_toggle(
            6,
            "Suoni Interfaccia (UI)",
            settings.ui_sounds_enabled,
            settings.ui_sounds_enabled ? "Attivi" : "Silenziosi",
            pal::kAccentClean);

        DrawTextShadow(
            "Regola uno slider o clicca la barra per ascoltare un'anteprima sonora in tempo reale.",
            layout.bx + 26,
            layout.by + layout.box_h - 104,
            13,
            Color{154, 206, 214, 225});
    }

    // Pulsanti in basso
    const int option_count = OptionCountForCategory(settings_category_);
    const bool gpu_changed =
        settings_category_ == SettingsCategory::Graphics &&
        settings.gpu_preference != settings.initial_gpu_preference;
    const int back_row_index = option_count;
    const int reset_row_index = option_count + 1;
    const int apply_gpu_row_index = option_count + 2;

    const char* back_label = IsTitleStateActive(ctx) ? "[ESC] Torna al Titolo" : "[ESC] Torna alla Pausa";
    DrawActionButton(
        layout.BackButtonRect(),
        back_label,
        pal::kAccentClean,
        settings_cursor_ == back_row_index,
        hovered_settings_row_ == back_row_index,
        false);

    DrawActionButton(
        layout.ResetButtonRect(),
        "Ripristina Predefiniti",
        pal::kBronzeBright,
        settings_cursor_ == reset_row_index,
        hovered_settings_row_ == reset_row_index,
        false);

    if (gpu_changed) {
        DrawActionButton(
            layout.ApplyGpuButtonRect(),
            "Applica GPU & Riavvia",
            pal::kJadeGlow,
            settings_cursor_ == apply_gpu_row_index,
            hovered_settings_row_ == apply_gpu_row_index,
            true);
    }

    const char* hint = "[Q/E] o [TAB] Scheda  |  [A/D] o Trascina Regola  |  [M] Muto  |  [ESC] Indietro";
    const int hw = MeasureUiText(hint, 12);
    DrawTextShadow(hint, layout.bx + (layout.box_w - hw) / 2, layout.by + layout.box_h - 24, 12, Color{155, 195, 215, 225});
}

} // namespace eco::app
