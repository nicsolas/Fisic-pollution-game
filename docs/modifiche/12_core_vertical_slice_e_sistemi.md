# 12 — Architettura Core della Vertical Slice (20-30 min): Ciclo Scientifico, Calibrazione a 3 Soluzioni, Reliquie-Strumento ed Ecologia Sistemica

## 1. Sintesi Architetturale

Questo modulo evolve `game_core` (`src/core/`, C++20 puro a zero dipendenze Raylib e zero warning con `-Wall -Wextra -Wpedantic`) per sostenere una **Vertical Slice giocabile di 20-30 minuti** articolata sulle prime tre aree interconnesse della Conca di Meridia:
1. **Terrazze dell'Alba (`ZoneId::DawnTerraces`)** — Ottica e Legge del Coseno di Lambert (`Eliostato Solare`).
2. **Quartiere dei Canali & Ponte degli Archi (`ZoneId::CanalQuarter`)** — Idrodinamica delle chiuse, correnti direzionali e snodo di Clara Sereni.
3. **Cisterne Ipogee (`StructureId::HypogeanCisterns`)** — Idrostatica e Legge di Stevino (`Turbina Idrostatica`) e archivio di Lorenzo Altieri.

L'intera implementazione preserva il 100% della retrocompatibilita' con la campagna completa a 6 distretti e 3 strutture interne, mantenendo verdi tutti i test unitari, di raggiungibilita' BFS, fisica 60 Hz e persistenza.

---

## 2. Ciclo Scientifico a 5 Fasi (`Osserva -> Ipotesi -> Sperimenta -> Conseguenza -> Soluzione`)

In `src/core/math_types.h`, `src/core/structure_puzzles.h` e `src/core/simulation.h` sono stati introdotti:
- `enum class InquiryStage : int { Observe = 0, Hypothesize = 1, Experimented = 2, ConsequenceSeen = 3, Resolved = 4 }`.
- `enum class HypothesisKind : int { HarmonicEquilibrium = 0, CriticalOverdrive = 1, EmergencyBypass = 2 }`.
- **9 Stazioni di Osservazione Diegetica (`ObservationNode`)** distribuite nelle 3 aree della Vertical Slice (`zone_mechanics.observation_nodes` e `structure.puzzle.observation_nodes`), ispezionabili con `[E]` o scansionabili con la Lente `[Q]`:
  - *Terrazze dell'Alba*: **Meridiana Madre di Travertino** `(11, 17)` (rapporto ombra/altezza $h = L$), **Condotto d'Ottone Ossidato** `(7, 13)` (soglia di vetrificazione focale), **Vasca d'Ulivo Spenta** `(17, 16)` (sensibilita' biologica dei rovi di pece).
  - *Quartiere dei Canali*: **Idrometro del Ponte** `(10, 17)`, **Chiusa Incrostata dell'Acquedotto** `(7, 10)`, **Conca dei Rami Secchi** `(14, 27)`.
  - *Cisterne Ipogee*: **Colonna Piezometrica di Stevino** `(17, 13)`, **Griglia di Sfogo Supercritico** `(17, 19)`, **Soglia dell'Archivista** `(17, 31)`.
- Ogni osservazione fornisce indizi fisici empirici ragionati (senza regalare il numero target a schermo) e fa avanzare `slice_inquiry_stage` per l'area attiva.

---

## 3. Calibrazione Multi-Soluzione + Impulso di Prova (`RunTrial`) & Ricalibrazione Reversibile

In `src/core/machine.h` e `src/core/machine.cpp`:
- Ogni monumento espone `CalibrationApproach approach` (`None`, `CleanSlow`, `RiskyEfficient`, `DirtyImmediate`) accanto a `MachineState state` (`CleanSlow` e `RiskyEfficient` impostano `state = MachineState::CleanActive` per piena compatibilita', mentre `DirtyImmediate` imposta `state = MachineState::BypassedDirty`), piu' un secondo asse fisico `stabilizer_value` (`[0..1]`, target armonico `0.70 +/- 0.16`, regime critico `< 0.40`).
- **Impulso di Prova (`CalibrationModalState::RunTrial` / `WorldContext::ExecuteModalTrial`)**:
  - Misura l'efficienza fisica, lo stress termico/idraulico (`0..1`), la direzione dell'errore (`-1`, `0`, `+1`) e verifica l'ingresso nella banda armonica o supercritica.
  - Scatena conseguenze fisiche tangibili nel mondo: sulle Terrazze un lampo solare svela i glifi della meridiana (`hidden_sundial_glyphs_revealed`) e aggiunge `+1.0..1.5` di stress smog se fuori fase; nelle Cisterne un colpo d'ariete (`trial_water_hammer_timer = 4.5s`) scuote le paratoie aprendo temporaneamente lo Sfioratore Alto.
- **3 Soluzioni con Requisiti e Conseguenze Sistemiche Distinte**:
  1. **Soluzione Pulita Lenta (`ConfirmClean` -> `CalibrationApproach::CleanSlow`)**:
     - *Requisiti*: efficienza $\ge$ soglia (allargata del `+30%` con `Dawn_FirstMeasureStele`) **E** `stabilizer_value` nella finestra armonica (`0.70 +/- 0.16`, allargata del `+30%` con la Stele).
     - *Impatto*: `+18` energia pulita, `-16` inquinamento (`-22` con `Dawn_WhiteOliveSeed`), purifica i rovi di pece aprendo i sentieri fioriti (`PitchBramble`), calma le correnti (`0.62x`), porta flora e fauna al massimo (`10..12` creature) e sblocca la piena fiducia di Clara (`Passaggio degli Artigiani`) e Lorenzo (`Nicchia d'Archivio`).
  2. **Soluzione Rischiosa ma Efficiente (`ConfirmRisky` -> `CalibrationApproach::RiskyEfficient`)**:
     - *Requisiti*: parametro nella banda supercritica/focale (`48..68 deg` o alta concentrazione su `SolarArray`; battente `12.5..15.0 m` su `HydroTurbine`) con `stabilizer_value < 0.40`.
     - *Impatto*: **massima energia (`+32`)**, lieve stress ambientale (`+4` inquinamento) e **trasformazione fisica della mappa**:
       - Su `SolarArray`: il fascio sovraccarico vetrifica la barriera rocciosa nord aprendo la **Scorciatoia della Cresta Arsa** `(3..4, 10..11)` in `DawnTerraces`.
       - Su `HydroTurbine`: lo scarico ad alta pressione **accelera del `+45%` le correnti di `CanalQuarter`** e spalanca lo **Sfioratore del Prisma / Galleria Alta** `(col 28, righe 7..10)` in `HypogeanCisterns`, portando Clara e Lorenzo in stato `NpcMood::TenseOverdrive`.
  3. **Soluzione Sporca Immediata (`TriggerBypass` -> `CalibrationApproach::DirtyImmediate`)**:
     - *Requisiti*: attivazione istantanea senza mira (`+22` energia sporca, `+22` inquinamento).
     - *Impatto*: intasa i sentieri bassi coi rovi di pece (`PitchBramble` chiusi), azzera la fauna (`0` creature), rende le correnti torbide e turbolente (`1.22x`) e costringe Clara e Lorenzo a ripararsi dai fumi (`NpcMood::ShelteredPolluted`).
- **Ricalibrazione Reversibile**: ogni monumento traccia `applied_clean_energy`, `applied_dirty_energy` e `applied_pollution_delta`. Riaprire il banco `[E]` su un monumento gia' attivo e cambiare approccio annulla in modo esatto il contributo precedente prima di applicare il nuovo regime.

---

## 4. Reliquie come Strumenti di Campo (`RelicGameplaySpec`)

In `src/core/collectible.h` e `src/core/collectible.cpp`, `GetRelicGameplaySpec(CollectibleId)` definisce categoria, titolo strumento ed effetto attivo/passivo per tutte le 16 reliquie, con integrazione diretta in `WorldContext` per le 6 reliquie della Vertical Slice:
1. `Dawn_BrassGnomon` (**Mirino Gnomonico della Lente**): raggio scan `[Q]` `+45%`, proiezione multi-rimbalzo dei fasci solari (`PredictGnomonBeams()`) e attivazione della Bussola Empirica temporanea (`7.0s`).
2. `Dawn_FirstMeasureStele` (**Nonio della Prima Misura**): passo fine `0.25x` su `AdjustParameter`/`AdjustStabilizer`, finestra di tolleranza pulita e stabilizzatore `+30%`, rivelazione della banda armonica dopo `1` sola prova.
3. `Dawn_WhiteOliveSeed` (**Seme Vivo d'Ulivo Bianco**): innestabile con `[E]` nelle vasche `BioPlanter` in `DawnTerraces` `(17, 18)` e `CanalQuarter` `(5, 13)` -> `-6` inquinamento immediato, rimozione dei rovi di pece vicini, potenziamento della cura `ConfirmClean` a `-22` e bonus `+12%` velocita' di movimento sui sentieri rigenerati.
4. `Waters_HydraulicDiary` (**Codice delle Chiuse Laminari**): azionare una leva `SluiceLever` `[E]` in `CanalQuarter` attiva il regime laminare (`current_speed_mult = 0.35x`) e prosciuga il **Guado degli Idraulici** `(15..16, 7..8)`.
5. `Waters_SluiceStele` (**Tavola Piezometrica di Stevino**): telemetria piezometrica live nelle Cisterne e sblocco della **Valvola di Equilibrio Ausiliaria** `(17, 26)` che apre a secco la Chiusa Est con `[E]`.
6. `Harmony_FarewellPrism` (**Prisma di Risonanza Remota**): l'impulso `[Q]` della Lente aggancia e commuta a distanza (`<= 220 px`, `FindRemotePrismTarget`) lo specchio solare, la leva idraulica o l'interruttore delle Cisterne piu' vicino.

---

## 5. Ecologia Sistemica, Fauna Zero-Heap, NPC Dinamici e Guida Empirica

- **`ZoneEcoStatus` & `AmbientCritter`**: `ComputeZoneEcoStatus(ZoneId)` calcola `EcoHealthBand` (`Critical`, `Polluted`, `Recovering`, `Flourishing`), vitalita', fioritura, moltiplicatore corrente e `fauna_count` (`0..12`). Lo sciame fisso `std::array<AmbientCritter, 12>` anima senza allocazioni heap rondini (`Swallow`) in Terrazze, carpe argentate e libellule (`SilverCarp`, `Dragonfly`) nei Canali, e tritoni/lucciole (`GlowNewt`, `CaveFirefly`) nelle Cisterne.
- **NPC Dinamici (`NpcMood`)**: `RefreshSystemicWorldState()` aggiorna umore, nota diegetica, scorciatoia associata e posizione (sempre su tile camminabili sicuri) di Clara Sereni `(12,29) / (12,30) / (11,27) / (14,24)` e Lorenzo Altieri `(17,30) / (17,31) / (16,34) / (16,35)`.
- **Riduzione Temporanea della Guida Automatica (`ComputeGuidanceDisplay()`)**: in modalita' predefinita `GuidanceAssistMode::Empirical`, bussola, distanza metrica, freccia a bordo schermo e beacon sul monumento sono nascosti all'avvio finche' il giocatore non esplora le Terrazze; dopo aver raccolto `Dawn_BrassGnomon` o calibrato `SolarArray`, l'impulso `[Q]` rivela la Bussola Empirica per `7.0s`, mentre la bussola stabile si sblocca al completamento della sintesi della Vertical Slice con Clara (con override `AlwaysOn` e `MinimalOff` sempre disponibili).
- **Persistenza (`save_data.cpp`)**: salvataggio e caricamento completi e retrocompatibili con il formato v1 di approcci, stabilizzatori, prove, stadi d'indagine, stazioni ispezionate, vasche innestate, guado laminare, valvola ausiliaria di Stevino e modalita' guida.
