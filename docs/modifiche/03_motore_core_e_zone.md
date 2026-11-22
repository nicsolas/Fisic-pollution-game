# Modifica 03 — Motore Core, Multi-Zona, Multi-Struttura, Collezionabili e Cronologia Dialoghi

**Autore:** @ingegnere-1 (Core Systems, World & Collectibles Architect)  
**Tema:** Rifondazione completa del motore puro C++20 (`src/core/`) su **La Conca di Meridia** (zero Raylib, zero riferimenti calabresi o "script", campagna completa attiva di default).

---

## 1. Riepilogo dei file creati e modificati

- `src/core/math_types.h`:
  - Esteso `TileId` (0..9): `Walkable`, `Wall`, `StructurePortal`, `ExitPortal`, `HiddenBridge`, `Water`, `PavedPath`, `SanctuaryFloor`, `RuinsWall`, `GardenFloor`.
  - Aggiunti `ZoneId` (6 distretti), `StructureId` (3 strutture interne), `LocationId` (`Overworld` + 3 strutture) e `MachineType` (6 monumenti fisici).
  - Aggiunti helper puri `constexpr`: `ZoneName`, `ZoneSubtitle`, `StructureName`, `StructureSubtitle`, `LocationName`, `UiPhaseName` (*Sguardo Empirico*, *Rilievo Metrico*, *Trama Geodetica*), `LocationFromStructure`, `StructureFromLocation`.
- `src/core/tilemap.h`, `src/core/tilemap.cpp`:
  - Overworld espanso da `18x28` a **`36x50` tile** (`1728x2400 px`, quasi 4x l'area precedente), diviso in una griglia 3x2 di 6 distretti con canale centrale a `col 24` e **Ponte degli Archi** (`HiddenBridge`, righe `16..18`).
  - Aggiunte `DistrictAtTile(row, col)`, `DistrictAt(world_pos)` e `DistrictBounds(ZoneId)`.
  - Implementate **3 mappe side-scrolling distinte** in `Tilemap::CreateStructureMap(StructureId)`:
    1. `HypogeanCisterns` (*Cisterne Ipogee*, `16x34`) — volte idrauliche e galleria alta dell'archivista.
    2. `PendulumTower` (*Torre del Pendolo*, `18x30`) — ballatoi ascendenti a spirale e doppio campanile superiore.
    3. `GeothermalChamber` (*Camera Geotermica*, `16x34`) — passerelle in basalto e piattaforma di Carnot.
  - Tutte le piattaforme verticali hanno salto massimo `<= 2 tile (96 px)`, pienamente raggiungibili con `kJumpForce = 480`, `kGravityAccel = 1200`.
- `src/core/collectible.h`, `src/core/collectible.cpp` *(nuovi)*:
  - Sistema puro C++20 per **16 reliquie** (`CollectibleId` 0..15) divise in **4 Ere** (`CollectibleEra`) e **5 categorie visive** (`CollectibleKind`: `Stele`, `Astrolabe`, `Prism`, `Cylinder`, `FossilSeed`).
  - Distribuzione bilanciata: 10 nell'Overworld (tutti i 6 distretti) + 6 nelle 3 Strutture interne (2 per struttura).
  - `CollectibleRegistry`: raccolta tramite auto-pickup ravvicinato (`<= 38 px`) o interazione `[E]` (`<= 72 px`), contatori totali/per era/per distretto/per luogo, stato recente (`HasRecentPickup`, `LastCollectedId`, `PickupBannerTimer`, `LastCollectedPos`) e `CollectAll()` per test/screenshot.
  - Contratto `CollectibleMeta` + `CollectibleMetadata(CollectibleId)` disaccoppiato dai testi narrativi (`src/core/collectible_content.cpp`).
- `src/core/machine.h`, `src/core/machine.cpp`:
  - Implementate **6 leggi fisiche autentiche** e calibrabili:
    1. `SolarArray` (*Eliostato Solare*): Ottica — Legge di Lambert `eta = cos(theta - 45 deg)`.
    2. `HydroTurbine` (*Turbina Idrostatica*): Idrostatica — Legge di Stevino `P = rho * g * h`, target `10.0 m`.
    3. `PendulumRegulator` (*Pendolo Isochronos*): Meccanica — Isocronismo di Galileo `T = 2*pi*sqrt(L/g)`, target `L = 2.00 m` (`T0 = 2.84 s`).
    4. `HarmonicResonator` (*Risonatore Armonico*): Acustica — Curva di risonanza `R = 1 / sqrt(1 + Q^2 * (f/f0 - f0/f)^2)` con `f0 = 120 Hz`, `Q = 3.0`.
    5. `ThermalExchanger` (*Scambiatore di Carnot*): Termodinamica — Rendimento `eta_C = 1 - Tc/Th` (`Tc = 300 K`) con punto operativo ottimale `Th = 600 K` (`eta_C = 50%`).
    6. `CentralAstrolabe` (*Astrolabio Centrale*): Sintesi — Interferenza di fase `I = cos^2((phi - 72 deg)/2)`, soglia `0.85`, sbloccato dopo l'attivazione dei 5 monumenti principali (`allow_bypass = false`).
  - Esposti range, step, unita', etichette di legge/formula e metadati di posizione su `Machine`.
- `src/core/npc.h`, `src/core/npc.cpp`:
  - Introdotto `NpcId` e `WorldNpc` per i **4 personaggi nel mondo**:
    - `ClaraSereni` (*Quartiere dei Canali*, presso il Ponte degli Archi)
    - `LorenzoAltieri` (*Cisterne Ipogee*, galleria alta)
    - `MiriamFerro` (*Giardini Sospesi*)
    - `MatteoVerrini` (*Caldera delle Fumarole*)
  - Mantenuto `ArchitectNpc` per compatibilita' con i test unitari, con testi interamente ripuliti e coerenti con Meridia.
- `src/core/story.h`, `src/core/story.cpp`:
  - Arco narrativo esteso a **7 Capitoli** (`Prologo`, `Atto1_Luce`, `Atto2_Acqua`, `Atto3_Pendolo`, `Atto4_Giardini`, `Atto5_Fumarole`, `Epilogo`) e **32 `StoryEvent`** (ingressi distretti/strutture, 4 NPC, 6 monumenti clean/bypass, smog critico, traguardi reliquie 1/4/8/12/16).
  - Implementata la **Cronologia dei Dialoghi (`StoryDirector::History()`)** con `DialogueHistoryEntry { sequence_id, chapter, event, is_ad_hoc, zone, location_name, speaker, text }`. Ogni battuta viene registrata esattamente una volta non appena diventa attiva sullo schermo (e mai persa in caso di avanzamento manuale con `AdvanceDialogue()`).
  - Aggiunto `StoryDirector::PushAdHocLine(...)` per registrare in cronologia anche i commenti alla raccolta delle reliquie e le conversazioni ripetute con gli NPC.
  - Aggiornata firma `EndingText(bool clean_path, int relics_found, int total_relics)` per i 3 finali.
- `src/core/simulation.h`, `src/core/simulation.cpp`:
  - `OverworldSim`: gestione di **3 `PortalLink` distinti** (ognuno con struttura target, distretto, rect portale e spawn di uscita sicuro), `DistrictAt`, `DistrictBounds`, `SafeSpawnInZone`.
  - `StructureSim`: gestione di tutte e 3 le mappe side-scrolling (`SelectStructure`, `OnEnter(StructureId, ...)`).
  - `WorldContext`: array uniforme delle 6 macchine, 4 NPC, `CollectibleRegistry`, `demo_mode = false` di default, rilevazione automatica del cambio distretto con banner (`zone_banner_timer`), raccolta reliquie integrata nel tick di simulazione, scanner empirico multi-monumento sul bersaglio piu' vicino nella scena attiva, `EnterStructure`, `ExitToOverworld` e `TeleportToZone`.

---

## 2. Verifiche eseguite

- Compilazione diretta di tutti gli 11 `.cpp` di `src/core/` con `-std=c++20 -Wall -Wextra -Wpedantic`: **0 errori, 0 warning**.
- Verifica dipendenze: **0 inclusioni di `raylib.h`** in `src/core/`.
- Audit lessicale su `src/core/`: **0 riferimenti residui** a Calabria, Crati, Unical, Ponte Bucci, GAIA, Ciclo 404, Nonna Rosa, Bruno Ceneri o Consorzio Brace.
- Build completa CMake/Ninja in `build-ing1` (`-std=c++20 -Wall -Wextra -Wpedantic`): **106/106 target compilati con 0 errori e 0 warning**.
- Suite CTest in `build-ing1` (`ctest --output-on-failure`): **2/2 PASS (100%)** (`eco_tests` + `headless_e2e`).
- Verifica dedicata di raggiungibilita' BFS e collisioni su Overworld `36x50` (prima e dopo lo sblocco del Ponte degli Archi) e sulle 3 strutture interne: tutti gli spawn, i 3 portali, i 6 monumenti, i 4 NPC e le 16 reliquie giacciono su tile camminabili e raggiungibili.
