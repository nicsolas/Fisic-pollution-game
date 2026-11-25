# Modifica 13 -- Narrativa Vertical Slice: Indagine Empirica, Tre Soluzioni Reattive e Reliquie-Strumento

- **Autore**: `@narrativa-slice` (Narrative Designer & Writer per la Vertical Slice)
- **File esclusivi modificati / creati**:
  - `src/core/story_content.cpp` (riscritto ed esteso in C++20 puro, zero `raylib.h`, 100% ASCII `32..126`)
  - `src/core/collectible_content.cpp` (aggiornato per tutte le 16 reliquie come strumenti vivi, 100% ASCII `32..126`)
  - `docs/STORIA.md` (aggiornato con ciclo scientifico, 3 regimi, Vertical Slice e tabella strumenti)
  - `docs/DIALOGHI.md` (aggiornato con i 40 eventi narrativi, guida empirica, landmark e sintesi reattiva)
  - `docs/modifiche/13_narrativa_vertical_slice_ipotesi.md` (creato)

---

## 1. Obiettivo del Ridisegno Narrativo

Trasformare i primi 20-30 minuti giocabili (**Terrazze dell'Alba**, **Quartiere dei Canali**, **Cisterne Ipogee**) da una sequenza guidata con obiettivi espliciti ("Vai a X e imposta 45 gradi") in un'autentica **esperienza di osservazione scientifica e scelta sistemica**, dove:
1. Il giocatore impara a **leggere i segnali fisici del mondo** senza bussola iniziale ne' spoiler numerici gratuiti nei dialoghi d'apertura;
2. Ogni impianto supporta narrativamente il ciclo **`Osservare -> Ipotesi -> Sperimentare ([T]) -> Conseguenza -> Soluzione`** e risponde con **tre esiti distinti e reversibili** (*Armonia Lenta*, *Regime Critico Rischioso*, *Fornace a Pece Immediata*);
3. Le **reliquie** raccontano la memoria delle Quattro Ere **insegnando concretamente come usarle sul campo** come strumenti ottici, meccanici, idraulici e botanici.

---

## 2. Interventi Principali

### 2.1 Guida Ridotta e Taccuino d'Osservazione (`ObjectiveFor` in `src/core/story_content.cpp`)
- Rimossi tutti gli spoiler numerici diretti (`45 deg`, `10 m`, `98.100 Pa`) da `ObjectiveFor`, `FirstScan` ed `EnteredCisterns`.
- In `Prologo`, `Atto1_Luce`, `Atto2_Acqua` e nella fase iniziale di `Atto3_Pendolo`, `ObjectiveFor` reagisce dinamicamente a `observations_inspected`, `trials_completed`, `solar_approach`, `hydro_approach`, `has_hydraulic_diary`, `laminar_ford_open` e `slice_completed`, guidando l'occhio su ombre, condotti d'ottone, schiuma, idrometri e tacche di profondita' sommerse.

### 2.2 Landmark Fisici Diegetici (`ZoneLandmarks`)
- Espansi a **4 landmark ciascuno** i distretti della Vertical Slice (`DawnTerraces` e `CanalQuarter`), mantenendo gli ID storici e aggiungendo `dawn_crest` (*Cresta dei Rovi Vetrificati*) e `canal_ford` (*Soglia del Guado degli Idraulici*):
  - **Lambert ragionato**: quando l'ombra dello stilo eguaglia la sua altezza verticale, il raggio taglia a meta' l'angolo retto e lo specchio rende al massimo guardando la bisettrice;
  - **Codice cromatico dei condotti**: freddi da spenti, oro nell'armonia, ambra vibrante nel sovraccarico, neri di fuliggine sotto la pece;
  - **Idraulica dei Canali e di Stevino**: pendenza dolce e regime laminare per scoprire il Guado, soglia armonica nei pozzi di calma contro la spinta supercritica che spalanca gli sfioratori alti.

### 2.3 Nuovi Eventi del Ciclo d'Indagine, 3 Soluzioni e Sintesi Finale (`DialogueForEvent`)
Implementati tutti i **40 `StoryEvent`** concordati in `story.h` con `@core-sim`:
- **Ciclo Sperimentale**: `ObservationClueRead`, `SolarTrialRun` e `HydroTrialRun` raccontano l'effetto fisico tangibile dell'Impulso di Prova `[T]` prima della decisione finale.
- **Tre Regimi Distinti su Luce e Acqua**:
  - `CleanSlow` (`SolarClean`, `HydroClean`): ritorno di rondini, carpe e tritoni luminosi, dissoluzione dei Rovi di Pece, apertura del Passaggio degli Artigiani (Clara) e della Nicchia Sigillata (Lorenzo).
  - `RiskyEfficient` (`SolarRisky`, `HydroRisky` + rami dedicati in `ClaraRecruited` e `LorenzoRecruited`): vetrificazione della **Cresta Arsa** sulle Terrazze, apertura a pressione dello **Sfioratore Alto** nelle Cisterne, accelerazione del `+45%` nei Canali e postura d'allerta tecnica di Clara e Lorenzo.
  - `DirtyImmediate` (`SolarBypassed`, `HydroBypassed` + ramo in `LorenzoRecruited`): fumo acre, scomparsa totale della fauna, passaggi bassi chiusi dai Rovi di Pece, Clara dietro un panno filtrante e Lorenzo ritirato sulle volte alte per proteggere i cilindri dall'umidita' acida.
- **Strumenti sul Campo**: `BioPlanterSeeded` (innesto del Seme d'Ulivo Bianco contro i rovi) e `LaminarFordOpened` (apertura del Guado degli Idraulici col Diario di Valerio).
- **Bilancio Reattivo della Vertical Slice (`VerticalSliceSynthesis`)**: dialogo corale al Ponte degli Archi con **5 diramazioni complete** (`Clean+Clean`, `Risky+Risky`, `Mista senza pece`, `Divisa con pece`, `Doppia pece`) + nota dinamica sugli strumenti e sulle reliquie scoperte.

### 2.4 Le 16 Reliquie come Strumenti Vivi (`src/core/collectible_content.cpp`)
- Riscritti `pickup_line` e `codex_paragraphs` per tutte le 16 reliquie, con particolare cura per le **6 reliquie della Vertical Slice** (`Dawn_BrassGnomon`, `Dawn_FirstMeasureStele`, `Dawn_WhiteOliveSeed`, `Waters_HydraulicDiary`, `Waters_SluiceStele`, `Harmony_FarewellPrism`), integrando in ogni terzo paragrafo la guida pratica **`USO SUL CAMPO (...)`**.

---

## 3. Verifiche Tecniche

- **Puro ASCII `32..126`**: verificati via script `src/core/story_content.cpp` e `src/core/collectible_content.cpp` (`0` byte fuori intervallo, `0` token legacy vietati, `0` inclusioni di `raylib.h`).
- **Compilazione e Test dedicati (`build-narr`)**: compilazione C++20 con `-Wall -Wextra -Wpedantic` a **zero warning** ed esecuzione completa di `ctest --test-dir build-narr --output-on-failure`.
