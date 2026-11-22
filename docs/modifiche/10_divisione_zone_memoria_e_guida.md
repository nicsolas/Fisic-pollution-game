# 10 — Divisione Fisica delle Zone, Ottimizzazione Memoria Piatta e Sistema di Guida

- **Autore**: `@ingegnere-zone` (in collaborazione 1-a-1 con `@artista-zone`)
- **Ambito**: `src/core/tilemap.h`, `src/core/tilemap.cpp`, `src/core/simulation.h`, `src/core/simulation.cpp`, `tests/test_main.cpp`
- **Obiettivo**: Separare fisicamente i 6 distretti della Conca di Meridia con scarpate di confine e varchi monumentali collegati da strade lastricate, ridurre drasticamente l'impronta RAM e le allocazioni heap di `Tilemap` e dei percorsi caldi in `WorldContext`, ed esporre un sistema di guida deterministico (`ObjectiveTarget`) per accompagnare il giocatore lungo la campagna senza farlo vagare a caso.

---

## 1. Divisione Fisica dei 6 Distretti e Metadati `DistrictGateway`

In `src/core/tilemap.h` e `src/core/tilemap.cpp`:
- Introdotte **scarpate/muri di confine spessi 2 tile** lungo gli assi orizzontali:
  - **Righe 11..12**: separazione netta tra banda Nord (`Altopiano dei Venti`, `Caldera delle Fumarole`) e banda Centrale (`Quartiere dei Canali`, `Giardini Sospesi`).
  - **Righe 23..24**: separazione netta tra banda Centrale e banda Sud (`Terrazze dell'Alba`, `Santuario dell'Armonia`).
  - L'asse verticale resta delimitato dal canale centrale a **colonna 24** (`kBridgeDividerCol`).
- Aperti **5 varchi monumentali (`DistrictGateway`)** larghi 3 tile, collegati da strade lastricate (`TileId::PavedPath`) che formano un percorso guidato naturale:
  1. `DawnTerraces <-> CanalQuarter` (righe 23..24, col 10..12 — `N -> CANALI` / `S -> TERRAZZE`)
  2. `CanalQuarter <-> WindPlateau` (righe 11..12, col 10..12 — `N -> VENTO` / `S -> CANALI`)
  3. `CanalQuarter <-> HangingGardens` — **Ponte degli Archi** (col 24, righe 16..18 — `E -> GIARDINI` / `O -> CANALI`, `requires_bridge_unlock = true`)
  4. `HangingGardens <-> FumaroleCaldera` (righe 11..12, col 35..37 — `N -> CALDERA` / `S -> GIARDINI`)
  5. `HangingGardens <-> HarmonySanctuary` (righe 23..24, col 35..37 — `S -> SANTUARIO` / `N -> GIARDINI`)
- Aggiunta una diramazione lastricata (`righe 15..16, col 5..6`) fino al portale delle **Cisterne Ipogee**.
- Esposto `Tilemap::OverworldGateways(float tile_size)` e `OverworldSim::gateways` (`std::array<DistrictGateway, 5>`), cosi' il renderer puo' disegnare archi monumentali, lanterne bicolore e cartelli direzionali senza alcuna coordinata hardcoded.

---

## 2. Ottimizzazione Memoria e Zero Allocazioni Per-Frame

- **Buffer piatto contiguo in `Tilemap`**:
  - Convertito lo storage interno da `std::vector<std::vector<int>>` a `std::vector<std::uint8_t>` (`rows_ * cols_`).
  - Per l'Overworld (`36 x 50`), le allocazioni heap passano da **37 a 1** e l'impronta dei dati scende da ~8.1 KB (+ overhead allocatore) a **1.800 byte contigui** (~4.5x in meno, massima cache locality).
  - Aggiunto costruttore diretto piatto `Tilemap(int rows, int cols, std::vector<std::uint8_t> cells, float tile_size)` usato da tutte le factory (`CreateOverworldMap`, `CreateStructureMap`), preservando la piena compatibilita' con il costruttore legacy `std::vector<std::vector<int>>` per test ed E2E.
  - Esposti `RawData()` (`std::span<const std::uint8_t>`) e `MemoryBytes()`.
- **Percorsi caldi zero-allocazione in `WorldContext`**:
  - Aggiunti gli helper template zero-allocazione `ForEachMachineInScene(Fn&&)` e `ForEachNpcInScene(Fn&&)` su `WorldContext`.
  - Aggiunto `reserve()` su `OverworldSim::portals`, `Tilemap::OverlappingTiles` e `WorldContext::NpcsInCurrentScene`.

---

## 3. Sistema di Guida del Giocatore (`ObjectiveTarget`)

In `src/core/simulation.h` e `src/core/simulation.cpp`:
- Introdotta la struct POD `ObjectiveTarget` (`valid`, `location`, `zone`, `world_pos`, `label`, `is_portal_step`) e il metodo zero-allocazione `WorldContext::CurrentObjectiveTarget() const noexcept`.
- Segue la progressione della campagna:
  1. `Eliostato Solare` (Terrazze dell'Alba)
  2. `Cisterne Ipogee` (portale) -> `Turbina Idrostatica`
  3. `Torre del Pendolo` (portale) -> `Pendolo Isochronos`
  4. `Clara Sereni (Ponte)` (se il Ponte degli Archi e' ancora bloccato)
  5. `Risonatore Armonico` (Giardini Sospesi)
  6. `Camera Geotermica` (portale) -> `Scambiatore di Carnot`
  7. `Astrolabio Centrale` (Santuario dell'Armonia)
  - Se il giocatore si trova dentro una struttura diversa da quella dell'obiettivo corrente, punta automaticamente all'`ExitPortal` (`"Uscita verso Meridia"`, `is_portal_step = true`).

---

## 4. Verifica e Test Automatici

Aggiunto `TestZoneBordersFlatMemoryAndGuidance()` in `tests/test_main.cpp`:
- Verifica buffer piatto contiguo (`1.800 B` per Overworld, `rows * cols` byte per tutte e 3 le strutture).
- Verifica solidita' integrale delle scarpate a righe 11..12 e 23..24 fuori dai varchi e percorribilita' di tutti e 5 i `DistrictGateway`.
- Verifica tramite **BFS** la raggiungibilita' al 100% da spawn di tutti e 6 i distretti (con Est bloccato prima di Clara e sbloccato dopo), tutti i 6 monumenti, i 4 NPC, i 3 portali e tutte le 16 reliquie (senza dover spostare alcuna reliquia in `collectible.cpp`).
- Verifica l'intera sequenza di `CurrentObjectiveTarget()` (inclusi i passaggi Overworld <-> Struttura e struttura errata -> `ExitPortal`) e gli helper `ForEachMachineInScene` / `ForEachNpcInScene`.

### Risultati Build & Test (`build-ingz`)
- Compilazione C++20 (`-Wall -Wextra -Wpedantic`): **0 errori, 0 warning**.
- Nessun `#include "raylib.h"` in `src/core/`.
- `ctest --output-on-failure`: **2/2 PASS** (`eco_tests`, `headless_e2e`).
