# 14 - UI Banco di Calibrazione Multi-Soluzione, Codex Strumenti, Guida Progressiva e QA Test

## 1. Obiettivo dell'Intervento
Trasformare la calibrazione dei monumenti, l'esplorazione delle prime tre aree della Vertical Slice (*Terrazze dell'Alba -> Quartiere dei Canali -> Cisterne Ipogee -> Sintesi con Clara e Lorenzo*) e l'interfaccia delle reliquie in un **ciclo di indagine scientifica diegetico, sistemico e rigiocabile**:
- Eliminazione dello spoiler immediato `Target: 45.0 deg` all'apertura del modal in favore di un **Banco d'Indagine Scientifica** a 5 fasi (`1.OSSERVA -> 2.IPOTESI -> 3.SPERIMENTA -> 4.CONSEGUENZA -> 5.SOLUZIONE`).
- Introduzione di **due parametri fisici accoppiati** (Parametro Principale + Stabilizzatore Armonico / Valvola di Smorzamento), **Impulso di Prova (`[T]` / `[SPAZIO]`)** con report empirico live e **3 Schede Soluzione Giocabili** fianco a fianco (`Pulita Lenta`, `Rischiosa ma Efficiente`, `Sporca Immediata`), con **ricalibrazione reversibile `[E]`** su qualsiasi monumento gia' attivato.
- **Guida Esplorazione Progressiva (`Empirical` di default)**: rimozione di bussola fissa, metri esatti, freccia a bordo schermo e anello pulsante all'inizio del gioco per premiare la lettura del mondo (ombre, condotti d'ottone, correnti, tacche di Stevino), con **Rilievo Empirico temporaneo su `[Q]` Lente** e opzione ciclabile nel menu Impostazioni.
- **Reliquie come Strumenti Ludici Attivi** in HUD, World-Space, Toast di raccolta e **`[J] Codex`**.
- **Suite completa di test unitari ed E2E** (18/18 unit test in `tests/test_main.cpp` + verifica Vertical Slice in `RunHeadlessE2E()`).

---

## 2. File Modificati
- `src/app/raylib_input.h`, `src/app/raylib_input.cpp`
- `src/app/calibration_ui.cpp`
- `src/app/story_ui.h`, `src/app/story_ui.cpp`
- `src/app/pause_settings_menu.h`, `src/app/pause_settings_menu.cpp`
- `src/app/main.cpp`
- `tests/test_main.cpp`
- `docs/modifiche/14_ui_calibrazione_guida_e_test.md`

---

## 3. Dettaglio delle Modifiche

### 3.1 Controlli del Banco di Calibrazione e Lente/Prisma `[Q]` (`src/app/raylib_input.h/.cpp`)
- **Separazione del tasto `[Q]`**: rimosso `KEY_Q` dallo spostamento a sinistra (`left`), cosi' la pressione di `[Q]` per la **Lente Empirica** o la **Risonanza Remota del Prisma del Congedo** non sposta mai Elena involontariamente.
- **Mappatura completa del Banco di Calibrazione** (Tastiera + Gamepad + Mouse):
  - **Slider Principale** (`slider_delta`): `A / D` o `FRECCIA SX / DX` (con passo fine tenendo premuto `SHIFT`), analogico sinistro X o `LB / RB` su gamepad, oppure drag diretto col mouse sulla barra.
  - **Stabilizzatore Armonico / Apertura Secondaria** (`aux_slider_delta`): `W / S` o `FRECCIA SU / GIU`, D-Pad verticale o analogico Y su gamepad, oppure drag col mouse sulla seconda barra.
  - **Selezione Ipotesi Fisica** (`hypothesis_delta`): tasti rapidi `[1]`, `[2]`, `[3]`, tasto `[I]` / `LT` per ciclare, oppure click diretto sui 3 bottoni ipotesi nel modal.
  - **Impulso di Prova / Sperimenta** (`trial_pressed`): `[T]` o `[SPAZIO]` (su gamepad `FaceLeft / X`), oppure click sul pulsante `[T / SPAZIO] SPERIMENTA`.
  - **Conferma Soluzione Pulita Lenta** (`confirm_pressed`): `[INVIO]` o `[C]` (su gamepad `FaceDown / A`), oppure click sulla Scheda 1.
  - **Conferma Soluzione Rischiosa Efficiente** (`risky_pressed`): `[R]` (su gamepad `RT`), oppure click sulla Scheda 2.
  - **Conferma Soluzione Sporca Immediata** (`bypass_pressed`): `[B]` (su gamepad `FaceRight / B`), oppure click sulla Scheda 3.

### 3.2 Banco di Indagine Scientifica & Calibrazione Multi-Soluzione (`src/app/calibration_ui.cpp`)
- **Layout `1120x640` centrato in `1280x720`** (`x=80, y=40`), progettato senza alcuna sovrapposizione di testo:
  1. **Header & Striscia a 5 Passi del Metodo Scientifico**: mostra `1.OSSERVA -> 2.IPOTESI -> 3.SPERIMENTA -> 4.CONSEGUENZA -> 5.SOLUZIONE`, evidenziando lo stadio corrente (`InquiryStage`) della zona e lo stato attuale del monumento (inclusa l'etichetta di ricalibrazione se gia' attivo).
  2. **Riquadro Sinistro (`w=526`) - Osservazione, Ipotesi & Controlli Accoppiati**:
     - Indizio diegetico d'osservazione (senza rivelare il target numerico all'apertura).
     - Selettore interattivo delle **3 Ipotesi Fisiche** con descrizione contestuale per ciascun monumento.
     - **Slider Parametro Principale** (con banda verde/ambra svelata **solo** dopo un Impulso di Prova `[T]` o se il giocatore possiede la reliquia strumento `Dawn_FirstMeasureStele` o `Waters_SluiceStele`).
     - **Slider Stabilizzatore Armonico / Valvola di Smorzamento** con indicazione visiva della fascia armonica (`>= 0.55`, ideale `~0.70`) e della fascia di spinta supercritica (`<= 0.40`).
     - Badge degli **Strumenti Reliquia Attivi** nel banco (`Stele della Prima Misura`, `Stele delle Chiusure`, `Seme d'Ulivo Bianco`).
  3. **Riquadro Destro (`w=538`) - Diagramma Vettoriale Live & Sperimentazione Empirica `[T]`**:
     - Simulazione grafica in tempo reale del monumento (raggi solari/specchio, colonna di Stevino/turbina, pendolo di Galileo, onde di risonanza, ciclo di Carnot, anelli dell'Astrolabio).
     - Pulsante cliccabile **`[T / SPAZIO] SPERIMENTA - IMPULSO DI PROVA`** e **Report Empirico dell'Esperimento** che mostra rendimento misurato (`eta`), stress termico/idraulico, direzione fisica suggerita e **conseguenza ambientale innescata nella scena** (es. rivelazione dei glifi solari sulla meridiana o colpo d'ariete che apre temporaneamente lo Sfioratore nelle Cisterne).
  4. **3 Schede Soluzione Giocabili Fianco a Fianco (`y=432, h=174`)**:
     - **Scheda 1 - `[INVIO] PULITA LENTA (ARMONIA)`**: richiede mira accurata + stabilizzatore armonico; `0%` pece, guarisce flora/fauna, apre sentieri verdi e conquista la fiducia di Clara e Lorenzo.
     - **Scheda 2 - `[R] RISCHIOSA EFFICIENTE (CRITICA)`**: finestra supercritica ad alta spinta; massima energia (`+32..+38 kW`), lieve stress ambientale, apre scorciatoie ad alta energia (*Cresta Arsa*, *Sfioratore del Prisma*), accelera le correnti dei canali (`+45%`) e mette gli NPC in allerta sui manometri.
     - **Scheda 3 - `[B] SPORCA IMMEDIATA (PECE)`**: bypass immediato a fornace di pece; alza l'inquinamento, serra i sentieri bassi coi rovi di pece, azzera la fauna e costringe Clara e Lorenzo a ripararsi dai fumi.
- **Ricalibrazione Reversibile `[E]`**: `DrawMonumentPrompt` e la logica del modal permettono di riaprire `[E] Ricalibra ...` su qualsiasi monumento gia' attivato (es. in modo Sporco o Rischioso) per convertirlo in soluzione Pulita (o viceversa) e osservare il mondo guarire in tempo reale.

### 3.3 Riduzione della Guida Automatica & Bussola Empirica (`src/app/calibration_ui.cpp`, `src/app/pause_settings_menu.h/.cpp`)
- In `calibration_ui.cpp`, `DrawOverworldMinimap`, `DrawObjectiveCompass` e `DrawMonumentPrompt` interrogano `world.ComputeGuidanceDisplay()`:
  - All'avvio in modalita' **`Empirica` (default)**: bussola inferiore, distanza in metri, freccia a bordo schermo, anello pulsante sul monumento e marker obiettivo sulla minimappa sono **nascosti**. Una discreta pillola diegetica invita a osservare ombre, condotti d'ottone e correnti.
  - Dopo aver raccolto lo **Gnomone d'Ottone (`Dawn_BrassGnomon`)** o calibrato l'Eliostato, premere **`[Q]` Lente Empirica** attiva per `7.0s` il **Rilievo Empirico Gnomonico** con bussola e indicatori che sfumano dolcemente.
  - Una volta completata la sintesi della Vertical Slice (o in modalita' `Sempre Attiva`), il Rilievo Geodetico diventa stabile.
- In `pause_settings_menu.h/.cpp`:
  - Aggiunta in `SettingsCategory::Game` (riga 0) l'opzione ciclabile **`Guida Esplorazione: Empirica / Sempre Attiva / Spenta`**, persistita in `saves/settings.cfg` (`guidance_mode_index`) e sincronizzata in tempo reale con `world.guidance_mode`.

### 3.4 Reliquie come Strumenti in World-Space, HUD e `[J] Codex` (`src/app/calibration_ui.cpp`, `src/app/story_ui.cpp`)
- **World-Space (`DrawSceneMachineAndOverlaysWorldSpace` in `calibration_ui.cpp`)**:
  - Prompt contestuali per **Stazioni di Osservazione (`ObservationNode`)** (`[E] Osserva` / `[E] Rileggi osservazione`), **Vasche Botaniche (`BioPlanter`)** (`[E] Innesta Seme d'Ulivo Bianco`) e **Valvola Ausiliaria di Stevino** nelle Cisterne Ipogee.
  - **`Dawn_BrassGnomon`**: quando la Lente `[Q]` e' attiva sulle Terrazze, traccia i **vettori ottici multi-rimbalzo previsti** (`world.PredictGnomonBeams()`) e il goniometro d'ombra `theta_ombra = 45.0 deg (h = L)`.
  - **`Waters_SluiceStele`**: nelle Cisterne Ipogee mostra il cartellino piezometrico live `Stevino: h = ... m | P = ... kPa` accanto al bacino e alla turbina.
  - **`Harmony_FarewellPrism`**: evidenzia con mirino ciano e prompt `[Q] Prisma: ...` lo specchio solare, la chiusa idraulica o l'interruttore di struttura agganciabile a distanza (`<= 220 px`), disegnando il raggio prismatico all'attivazione.
- **Toast di Raccolta & `[J] Codex` (`story_ui.cpp`)**:
  - Il toast di raccolta reliquia e la scheda dettaglio del **`[J] Codex`** includono un riquadro dedicato **"STRUMENTO & EFFETTO LUDICO"** che espone `tool_badge`, `gameplay_effect` e `where_to_use` da `eco::GetRelicGameplaySpec(id)`.
- **Riepilogo Primo Ciclo Vertical Slice (`story_ui.cpp`)**:
  - Aggiunto in alto a sinistra (`DrawVerticalSliceSummaryCard`, comprimibile con un click) il riepilogo non invasivo del Primo Ciclo quando Terrazze, Canali e Cisterne sono stati affrontati, con sintesi delle scelte (`SolarArray`, `HydroTurbine`), stato ecologico (`EcoHealthBand`), vitalita', osservazioni, esperimenti, reliquie-strumento e varchi sbloccati.

---

## 4. Test Unitari ed End-to-End (`tests/test_main.cpp`, `src/app/main.cpp`)
- Aggiunto `TestVerticalSliceInquiryMultiSolutionRelicsAndSystemicEco()` in `tests/test_main.cpp` (18esimo test della suite), che verifica:
  1. Guida progressiva (`ComputeGuidanceDisplay()` nascosta all'avvio in `Empirical`, attivata da `[Q]` dopo aver raccolto `Dawn_BrassGnomon`, e override `AlwaysOn` / `MinimalOff`).
  2. Tutte le 6 reliquie della Vertical Slice con `GetRelicGameplaySpec()` valido e in puro ASCII.
  3. Ciclo `Observe -> Hypothesize -> Experiment -> ConsequenceSeen -> Resolved` sulle Terrazze dell'Alba, con `[T]` Impulso di Prova che rivela i glifi solari.
  4. Tutte e 3 le soluzioni (`DirtyImmediate`, `RiskyEfficient`, `CleanSlow`) + ricalibrazione su `SolarArray` e `HydroTurbine`, verificando apertura/blocco di *Sentiero degli Ulivi Bassi*, *Cresta Arsa*, *Guado degli Idraulici*, *Passaggio degli Artigiani*, *Sfioratore del Prisma* e *Nicchia d'Archivio*.
  5. Uso attivo di `Dawn_WhiteOliveSeed` su `BioPlanter`, `Harmony_FarewellPrism` da remoto con `[Q]` su specchi/interruttori, `Waters_HydraulicDiary` sulle chiuse laminari e `Waters_SluiceStele` sulla valvola ausiliaria di Stevino.
  6. Reazioni dinamiche di umore, posizione e dialogo di Clara Sereni e Lorenzo Altieri (`ShelteredPolluted`, `TenseOverdrive`, `FlourishingTrust`) fino al completamento di `ComputeVerticalSliceSummary()`.
- Aggiornato `RunHeadlessE2E()` in `src/app/main.cpp` per validare anche il flusso E2E della Vertical Slice.
- Tutti i target compilano in `build-ui` con `-Wall -Wextra -Wpedantic` a **zero warning** e **100% dei test verdi** (`eco_tests` e `headless_e2e`).
