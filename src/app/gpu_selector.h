#pragma once

#include <string>
#include <vector>

namespace eco::app {

struct GpuOption {
    std::string id{};        // "auto", "nvidia", "intel", "amd", o "cardN"
    std::string label{};     // Es. "NVIDIA (Dedicata - nvidia)"
    std::string vendor{};    // "nvidia" | "intel" | "amd" | "auto" | "other"
    std::string pci_slot{};  // Es. "0000:01:00.0"
    int drm_index = -1;
};

// Scansiona le GPU disponibili sul sistema (Linux /sys/class/drm + opzione Auto).
[[nodiscard]] const std::vector<GpuOption>& EnumerateGpuOptions();

// Trova l'indice nella lista EnumerateGpuOptions() per un id (default 0 = Auto).
[[nodiscard]] int FindGpuOptionIndex(const std::string& gpu_id) noexcept;

// Imposta le variabili d'ambiente EGL/GLX/DRI_PRIME PRIMA di InitWindow().
// Su Linux Wayland/X11 commuta effettivamente tra NVIDIA (10_nvidia.json / PRIME) e Mesa Intel/AMD (50_mesa.json).
void ApplyGpuPreferenceBeforeInitWindow(const std::string& gpu_id);

// Interroga il renderer OpenGL effettivamente attivo dopo InitWindow() (es. "NVIDIA GeForce..." o "Mesa Intel...").
[[nodiscard]] std::string QueryActiveGpuRenderer();

// Riavvia il processo corrente (dopo aver salvato impostazioni/partita) per applicare la nuova GPU.
// Restituisce false se il riavvio non e' supportato sulla piattaforma.
bool RelaunchProcessForGpuChange(int argc, char** argv);

} // namespace eco::app
