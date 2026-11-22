# 08 — Componente Audio Procedurale (Suoni Ambientali e Interazioni)

**Data**: 2026-11-22  
**Autore**: Coordinatore / Ingegneria Audio

## Obiettivo
Aggiungere una componente audio completa al progetto **senza voci né effetti sui dialoghi**, focalizzata su **paesaggio sonoro ambientale** e **suoni di interazione fisica e d'interfaccia**, mantenendo `src/core/` al 100% privo di dipendenze Raylib e senza asset binari esterni.

## File Creati e Modificati
- **Nuovi**:
  - `src/app/audio/audio_engine.h` — Interfaccia `AudioEngine`, enum `SfxId` (18 suoni d'interazione), controllo volume/muto/ambiente e helper globali leggeri per l'UI.
  - `src/app/audio/audio_engine.cpp` — Sintetizzatore PCM procedurale in C++20 (44.1 kHz, 16-bit mono via Raylib `miniaudio` / `LoadSoundFromWave`) e osservatore automatico di stato (`SyncAndUpdate`).
- **Modificati**:
  - `CMakeLists.txt` — Aggiunto `src/app/audio/audio_engine.cpp` al target `eco_proto` (lasciando `game_core` e `eco_tests` puri e zero-dipendenze).
  - `src/app/pause_settings_menu.h`, `src/app/pause_settings_menu.cpp` — Aggiunte le voci **Volume Audio (`0%..100%`, `[M]` Muto rapido)** e **Paesaggio Sonoro (`Ambiente + Interazioni` / `Solo Interazioni`)** nelle Impostazioni + feedback `UiClick` discreto su navigazione/toggle menu.
  - `src/app/main.cpp` — Ciclo di vita completo di `AudioEngine` (`Init`, `SyncAndUpdate`, `Shutdown`), opzione CLI `--mute` e isolamento totale da `--headless-e2e`.

## Catalogo Sonoro Sintetizzato

### 1. Suoni di Interazione e Fisica (ZERO dialoghi / ZERO voci)
- **Movimento**: `FootstepStone` / `FootstepGrass` (selezionati dinamicamente dal `TileId` sotto i piedi con lieve variazione di pitch), `Jump` e `Land` nelle 3 strutture side-scrolling.
- **Fisica & Monumenti**: `ScanPulse` (sonar armonico della Lente Empirica `[F]`), `ModalOpen` / `ModalClose`, `SliderTick` (regolazione parametri con rate-limit anti-ronzio), `CalibrateClean` (accordo Dmaj9 luminoso), `CalibrateBypass` (accensione grave della fornace a pece), `BridgeUnlock` (risonanza monumentale del Ponte degli Archi), `EpilogueChord` (accordo finale dell'Astrolabio Centrale).
- **Esplorazione & Interazioni**: `RelicCollect` (arpeggio a 3 campane di giada alla raccolta delle 16 reliquie), `NpcInteract` (singolo segnale d'interazione caldo in legno/ottone all'incontro con un personaggio, **mai** durante le battute di dialogo), `PortalEnter` (transizione portali), `ZoneDiscover` (cambio distretto), `PhaseShift` (cambio Fase Percettiva `[V]`), `UiClick` (navigazione menu/tab Diario).

### 2. Paesaggio Sonoro Ambientale (5 Loop Continui senza Giunture da 4.0s)
- `OverworldClean` & `OverworldSmog`: crossfade continuo guidato in tempo reale dalla foschia (`PollutionRatio()`) della Conca di Meridia.
- `Cisterns`: eco idraulico e flusso d'acqua filtrato nelle *Cisterne Ipogee*.
- `Tower`: risonanza della *Torre del Pendolo* con lento battito isocrono d'ottone.
- `Geothermal`: bordone di basalto e soffio morbido di vapore nella *Camera Geotermica*.
- Attenuazione automatica (`ducking` al 42%) quando il Diario `[H/J]` o il Menu di Pausa `[ESC]` sono aperti; disattivabile per tenere `Solo Interazioni`.

## Verifiche Eseguite
- `cmake --build build -j8`: **0 errori, 0 warning** (`-Wall -Wextra -Wpedantic`).
- `cd build && ctest --output-on-failure`: **2/2 PASS** (`eco_tests` + `headless_e2e`).
- Smoke-test reale su backend `miniaudio | PulseAudio`: `AUDIO: Device initialized successfully` -> `AUDIO: Device closed successfully`, sia con audio attivo che con `--mute`.
- Verifica visiva del menu Impostazioni aggiornato in `docs/screenshots/20_impostazioni_audio.png`.
