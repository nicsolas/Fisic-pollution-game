#include "app/gpu_selector.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#if defined(__linux__)
#include <dlfcn.h>
#include <unistd.h>
#endif

namespace eco::app {

namespace {

[[nodiscard]] std::string TrimLine(std::string s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ' || s.back() == '\t')) {
        s.pop_back();
    }
    std::size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t')) {
        ++start;
    }
    return s.substr(start);
}

[[nodiscard]] std::string ToLowerAscii(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

[[nodiscard]] std::string ReadFirstLine(const std::filesystem::path& p) {
    std::ifstream in(p);
    if (!in.is_open()) {
        return {};
    }
    std::string line;
    std::getline(in, line);
    return TrimLine(line);
}

[[nodiscard]] std::string DriPrimeSlotValue(std::string slot) {
    for (char& c : slot) {
        if (c == ':' || c == '.') {
            c = '_';
        }
    }
    return "pci-" + slot;
}

} // namespace

const std::vector<GpuOption>& EnumerateGpuOptions() {
    static std::vector<GpuOption> cached;
    static bool initialized = false;
    if (initialized) {
        return cached;
    }
    initialized = true;

    cached.push_back(GpuOption{
        "auto",
        "Auto (Predefinita di Sistema)",
        "auto",
        "",
        -1
    });

#if defined(__linux__)
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path drm_dir("/sys/class/drm");
    if (fs::exists(drm_dir, ec) && fs::is_directory(drm_dir, ec)) {
        std::vector<int> card_indices;
        for (const auto& entry : fs::directory_iterator(drm_dir, ec)) {
            const std::string name = entry.path().filename().string();
            if (name.rfind("card", 0) != 0) {
                continue;
            }
            const std::string rest = name.substr(4);
            if (rest.empty() || !std::all_of(rest.begin(), rest.end(), [](unsigned char ch) {
                    return std::isdigit(ch) != 0;
                })) {
                continue;
            }
            card_indices.push_back(std::stoi(rest));
        }
        std::sort(card_indices.begin(), card_indices.end());

        int nvidia_count = 0;
        int intel_count = 0;
        int amd_count = 0;

        for (int idx : card_indices) {
            const fs::path dev_dir = drm_dir / ("card" + std::to_string(idx)) / "device";
            if (!fs::exists(dev_dir / "vendor", ec)) {
                continue;
            }
            const std::string vendor_hex = ToLowerAscii(ReadFirstLine(dev_dir / "vendor"));
            std::string driver;
            std::string pci_slot;
            std::ifstream uevent(dev_dir / "uevent");
            std::string line;
            while (std::getline(uevent, line)) {
                line = TrimLine(line);
                if (line.rfind("DRIVER=", 0) == 0) {
                    driver = line.substr(7);
                } else if (line.rfind("PCI_SLOT_NAME=", 0) == 0) {
                    pci_slot = line.substr(14);
                }
            }

            GpuOption opt{};
            opt.drm_index = idx;
            opt.pci_slot = pci_slot;

            if (vendor_hex == "0x10de" || driver == "nvidia" || driver == "nouveau") {
                opt.vendor = "nvidia";
                opt.id = (nvidia_count == 0) ? "nvidia" : ("nvidia" + std::to_string(nvidia_count));
                opt.label = "NVIDIA (Dedicata" + (driver.empty() ? std::string() : " - " + driver) + ")";
                ++nvidia_count;
            } else if (vendor_hex == "0x8086" || driver == "i915" || driver == "xe") {
                opt.vendor = "intel";
                opt.id = (intel_count == 0) ? "intel" : ("intel" + std::to_string(intel_count));
                opt.label = "Intel (Integrata" + (driver.empty() ? std::string() : " - " + driver) + ")";
                ++intel_count;
            } else if (vendor_hex == "0x1002" || driver == "amdgpu" || driver == "radeon") {
                opt.vendor = "amd";
                opt.id = (amd_count == 0) ? "amd" : ("amd" + std::to_string(amd_count));
                opt.label = "AMD (" + (driver.empty() ? std::string("Mesa") : driver) + ")";
                ++amd_count;
            } else {
                opt.vendor = "other";
                opt.id = "card" + std::to_string(idx);
                opt.label = "GPU card" + std::to_string(idx) +
                            (driver.empty() ? std::string() : " (" + driver + ")");
            }
            cached.push_back(std::move(opt));
        }
    }
#endif

    return cached;
}

int FindGpuOptionIndex(const std::string& gpu_id) noexcept {
    const auto& list = EnumerateGpuOptions();
    const std::string lowered = ToLowerAscii(gpu_id);
    for (std::size_t i = 0; i < list.size(); ++i) {
        if (ToLowerAscii(list[i].id) == lowered ||
            (lowered == "dedicated" && list[i].vendor == "nvidia") ||
            (lowered == "integrated" && list[i].vendor == "intel")) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

void ApplyGpuPreferenceBeforeInitWindow(const std::string& gpu_id) {
    const auto& list = EnumerateGpuOptions();
    const int idx = FindGpuOptionIndex(gpu_id);
    if (idx <= 0 || idx >= static_cast<int>(list.size())) {
        return; // "auto": rispetta le variabili dell'ambiente corrente
    }

    const GpuOption& chosen = list[static_cast<std::size_t>(idx)];
#if defined(__linux__)
    namespace fs = std::filesystem;
    std::error_code ec;
    constexpr const char* kNvidiaEglJson = "/usr/share/glvnd/egl_vendor.d/10_nvidia.json";
    constexpr const char* kMesaEglJson = "/usr/share/glvnd/egl_vendor.d/50_mesa.json";

    if (chosen.vendor == "nvidia") {
        setenv("__NV_PRIME_RENDER_OFFLOAD", "1", 1);
        setenv("__GLX_VENDOR_LIBRARY_NAME", "nvidia", 1);
        setenv("__VK_LAYER_NV_optimus", "NVIDIA_only", 1);
        unsetenv("DRI_PRIME");
        if (fs::exists(kNvidiaEglJson, ec)) {
            setenv("__EGL_VENDOR_LIBRARY_FILENAMES", kNvidiaEglJson, 1);
        }
    } else if (chosen.vendor == "intel" || chosen.vendor == "amd" || chosen.vendor == "other") {
        unsetenv("__NV_PRIME_RENDER_OFFLOAD");
        unsetenv("__GLX_VENDOR_LIBRARY_NAME");
        unsetenv("__VK_LAYER_NV_optimus");
        if (fs::exists(kMesaEglJson, ec)) {
            setenv("__EGL_VENDOR_LIBRARY_FILENAMES", kMesaEglJson, 1);
        }
        if (!chosen.pci_slot.empty()) {
            const std::string slot_val = DriPrimeSlotValue(chosen.pci_slot);
            setenv("DRI_PRIME", slot_val.c_str(), 1);
        }
    }
#else
    (void)chosen;
#endif
}

std::string QueryActiveGpuRenderer() {
#if defined(__linux__)
    using GlGetStringFn = const unsigned char* (*)(unsigned int);
    constexpr unsigned int kGlRenderer = 0x1F01u;
    void* sym = dlsym(RTLD_DEFAULT, "glGetString");
    if (sym != nullptr) {
        auto fn = reinterpret_cast<GlGetStringFn>(sym);
        const unsigned char* raw = fn(kGlRenderer);
        if (raw != nullptr && raw[0] != '\0') {
            std::string s(reinterpret_cast<const char*>(raw));
            if (s.size() > 44) {
                s = s.substr(0, 41) + "...";
            }
            return s;
        }
    }
#endif
    return "OpenGL 3.3 (Predefinita)";
}

bool RelaunchProcessForGpuChange(int argc, char** argv) {
#if defined(__linux__)
    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(std::max(1, argc)));
    if (argc > 0 && argv != nullptr && argv[0] != nullptr) {
        args.emplace_back(argv[0]);
    } else {
        args.emplace_back("/proc/self/exe");
    }
    for (int i = 1; i < argc; ++i) {
        const std::string_view a(argv[i]);
        // Non trascinare un vecchio --gpu CLI che sovrascriverebbe la nuova scelta salvata
        if (a == "--gpu" && i + 1 < argc) {
            ++i;
            continue;
        }
        args.emplace_back(argv[i]);
    }

    std::vector<char*> c_argv;
    c_argv.reserve(args.size() + 1);
    for (auto& s : args) {
        c_argv.push_back(s.data());
    }
    c_argv.push_back(nullptr);

    execv("/proc/self/exe", c_argv.data());
    return false;
#else
    (void)argc;
    (void)argv;
    return false;
#endif
}

} // namespace eco::app
