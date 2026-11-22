# 09 — Selezione Monitor, Selezione GPU, Salvataggio Partita e Menu "Riprendi"

**Data**: 2026-11-22  
**Autore**: Coordinatore / Ingegneria Sistemi & UI

## Obiettivi
1. **Selezione Schermo (Monitor Schermo Intero)**: risolvere il problema del passaggio al secondo monitor su sistemi multi-monitor (Wayland/X11) permettendo di scegliere esplicitamente il monitor su cui andare a schermo intero.
2. **Selezione GPU / Scheda Grafica**: rilevare automaticamente le GPU fisiche presenti (es. NVIDIA dedicata + Intel/AMD integrata), mostrare la GPU OpenGL effettivamente attiva e consentire la commutazione con riavvio rapido (`Applica GPU & Riavvia`) o da CLI (`--gpu`).
3. **Salvataggio & Ripresa Partita**: salvare l'intero stato della campagna (posizione, zona/struttura, 6 monumenti, 4 NPC, 16 reliquie, eventi narrativi e Cronologia Dialoghi integrale) con salvataggio manuale e automatico, e offrire nel menu del Titolo **"Riprendi da dove hai lasciato"** con uno **sfondo procedurale ridisegnato dinamicamente in base alla zona o struttura salvata**.

## File Creati e Modificati
- **Nuovi**:
  - `src/core/save_data.h`, `src/core/save_data.cpp` — Serializzazione pura C++20 (zero Raylib) con scrittura atomica in `saves/conca_di_meridia.sav`, `ReadSaveSummary`, `SaveWorldToFile`, `LoadWorldFromFile`, `DeleteSaveFile`.
  - `src/app/gpu_selector.h`, `src/app/gpu_selector.cpp` — Scansione `/sys/class/drm/card*`, selezione variabili EGL/GLX/DRI_PRIME (`__EGL_VENDOR_LIBRARY_FILENAMES`, `__NV_PRIME_RENDER_OFFLOAD`, `__GLX_VENDOR_LIBRARY_NAME`, `DRI_PRIME`) prima di `InitWindow()`, lettura live `GL_RENDERER` (`QueryActiveGpuRenderer`) e riavvio pulito `RelaunchProcessForGpuChange`.
- **Modificati**:
  - `src/core/story.h`, `src/core/story.cpp` — Aggiunti `FiredFlags()` e `RestoreSavedState(...)` in `StoryDirector` per ripristinare senza duplicati eventi e Cronologia Dialoghi.
  - `src/core/collectible.h` — Aggiunto `ClearRecentBanner()`.
  - `src/core/simulation.h`, `src/app/overworld_state.cpp`, `src/app/structure_state.cpp` — Aggiunto flag `loaded_from_save` per preservare esattamente posizione e camera al caricamento della partita.
  - `src/app/game_state.h` — Aggiunti `open_settings_requested` e `relaunch_gpu_requested` in `AppContext`.
  - `src/app/title_state.h`, `src/app/title_state.cpp` — Menu interattivo con **"Riprendi da dove hai lasciato"**, scheda riepilogo salvataggio (zona, capitolo, monumenti, reliquie, foschia, data) e **9 sfondi procedurali dedicati** (6 distretti Overworld + 3 strutture interne) tinteggiati in base alla foschia reale salvata.
  - `src/app/pause_settings_menu.h`, `src/app/pause_settings_menu.cpp` — Persistenza `saves/settings.cfg`, selettore **Monitor Schermo Intero** con `SetWindowMonitor(target)` (fix Wayland/X11 multi-monitor), selettore **GPU / Scheda Grafica** + riga `GPU Attiva Ora` + pulsante `Applica GPU & Riavvia`, voce **"Salva Partita Ora"** nel menu di pausa e salvataggio automatico al ritorno al Titolo o all'uscita.
  - `src/app/main.cpp` — Caricamento preferenze prima di `InitWindow()`, applicazione GPU prima del contesto OpenGL, applicazione monitor/fullscreen salvato, autosave su traguardi/cambi zona, flag `--gpu` e `--monitor`.
  - `CMakeLists.txt`, `.gitignore`, `tests/test_main.cpp` — Aggiunti `save_data.cpp` e `gpu_selector.cpp`, escluso `saves/` dal repository, aggiunto unit test `TestSaveAndLoadPersistence()`.

## Verifiche Eseguite
- `cmake --build build -j8`: **0 errori, 0 warning** (`-Wall -Wextra -Wpedantic`).
- `cd build && ctest --output-on-failure`: **2/2 PASS** (incluso `TestSaveAndLoadPersistence` per Overworld + Struttura + Reliquie + Cronologia Dialoghi).
- **Verifica reale commutazione GPU su Wayland**:
  - `--gpu nvidia` -> `Vendor: NVIDIA Corporation | Renderer: NVIDIA GeForce RTX 3050/PCIe/SSE2`
  - `--gpu intel` -> `Vendor: Intel | Renderer: Mesa Intel(R) Graphics (RKL GT1)`
- **Verifica visiva**:
  - `docs/screenshots/21_impostazioni_monitor_gpu.png` — Pannello Impostazioni con Monitor, GPU, `GPU Attiva Ora` e Audio.
  - `docs/screenshots/22_titolo_riprendi_zona.png` — Schermata Titolo con scheda salvataggio, menu `Riprendi da dove hai lasciato` e sfondo ridisegnato dei *Giardini Sospesi*.
