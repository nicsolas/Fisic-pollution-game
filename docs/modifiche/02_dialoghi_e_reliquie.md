# Modifica 02 -- Copione Completo, Cronologia Dialoghi e Catalogo delle 16 Reliquie

- **Autore**: `@narratore-2` (Dialogue, Collectibles & Chronicle Writer)
- **File modificati / creati**:
  - `src/core/story_content.cpp` (riscritto integralmente in C++20 puro, zero `raylib.h`)
  - `src/core/collectible_content.cpp` (nuovo modulo C++20 puro, zero `raylib.h`)
  - `docs/DIALOGHI.md` (riscritto integralmente sul nuovo copione e catalogo reliquie)
  - `docs/modifiche/02_dialoghi_e_reliquie.md` (creato)

---

## 1. Obiettivo dell'Intervento

Dare voce letteraria, naturale e coerente all'intera campagna espansa de **La Conca di Meridia**, rendendo la **Cronologia dei Dialoghi (`[H]`)** piacevole da rileggere come un vero diario di campo e costruendo il **Codex dell'Archivio (`[J]`)** con 16 reliquie che ricompongono le quattro ere della civilta' dei Meridiani, eliminando completamente:
- ogni riferimento all'ambientazione calabrese precedente;
- ogni lessico artificiale da log tecnico o script di debug (`// GAIA [SYS]`, `AABB`, `Ciclo 404`, `[RADIO]`, `[REGISTRO]`, ecc.).

---

## 2. Contenuti Realizzati

### 2.1 Copione Narrativo (`src/core/story_content.cpp`)
- **7 Capitoli (`ChapterTitle`)**: `Prologo` (*La Nebbia sull'Altopiano*), `Atto1_Luce` (*La Misura della Luce*), `Atto2_Acqua` (*Il Peso dell'Acqua*), `Atto3_Pendolo` (*Il Tempo del Pendolo*), `Atto4_Giardini` (*L'Accordo dei Giardini*), `Atto5_Fumarole` (*Le Ceneri della Frattura*), `Epilogo` (*Il Cuore di Meridia*).
- **32 Eventi Narrativi (`DialogueForEvent`)**:
  - Prologo, prima scansione empirica e tutte le attivazioni pulite e bypass a fornace dei cinque monumenti fisici (Lambert `45 deg`, Stevino `10 m / 98.100 Pa`, Galileo `L=2.00 m / T=2.84 s`, Risonanza Armonica `120 Hz`, Carnot `600 K / 300 K, eta=50%`) piu' sblocco e attivazione dell'**Astrolabio Centrale** (`72 deg`).
  - **8 eventi di ingresso contestuale** per i distretti dell'Overworld (*Quartiere dei Canali*, *Altopiano dei Venti*, *Giardini Sospesi*, *Caldera delle Fumarole*, *Santuario dell'Armonia*) e per le 3 strutture interne (*Cisterne Ipogee*, *Torre del Pendolo*, *Camera Geotermica*).
  - **4 incontri approfonditi con gli NPC**: **Clara Sereni**, **Lorenzo Altieri** (con variante sensibile al numero di reliquie raccolte), **Miriam Ferro** (con variante legata alla qualita' dell'aria) e **Matteo Verrini** (confronto diretto e realistico nella Caldera, con variante pulita vs bypass).
  - **Allarme cenere critica** (`SmogCritical >= 70%`) e **5 traguardi reliquie** (`FirstRelic`, `FourRelics`, `HalfRelics`, `TwelveRelics`, `AllRelics`).
- **Obiettivi Dinamici (`ObjectiveFor`)**: copertura completa di tutti i 7 capitoli, gestione di Overworld vs Strutture interne, percorso pulito vs ricalibrazione post-bypass e guida al reclutamento di Clara Sereni e Lorenzo Altieri.
- **Introduzione (`IntroCrawl`)** e **3 Finali (`EndingText(clean_path, relics_found, total_relics)`)**:
  1. *L'Alba di Meridia -- Finale Rivelato* (percorso pulito + almeno 12/16 reliquie);
  2. *La Rinascita della Conca -- Finale Armonico* (percorso pulito + meno di 12 reliquie);
  3. *Il Prezzo della Fretta -- Finale Grigio* (almeno una fornace di bypass usata).

### 2.2 Catalogo delle 16 Reliquie (`src/core/collectible_content.cpp`)
- Implementazione completa di `CollectibleMetadata(CollectibleId)` per tutte le 16 reliquie (`I.1`..`IV.4`), 4 per ciascuna delle 4 ere storiche (*Era I - Alba delle Ombre*, *Era II - Secolo delle Acque e del Suono*, *Era III - Inverno di Pece*, *Era IV - Sigillo dell'Armonia*), bilanciate sulle 5 forme visive (`Stele`, `Astrolabe`, `Prism`, `Cylinder`, `FossilSeed`) e distribuite tra i 6 distretti Overworld (10) e le 3 strutture interne (6).
- Ogni reliquia include una **nota di ritrovamento parlata** (registrata nella Cronologia `[H]` alla raccolta) e **3 paragrafi letterari** per il Codex `[J]`.

### 2.3 Documentazione (`docs/DIALOGHI.md`)
- Aggiornato integralmente con il nuovo cast, i 7 capitoli, `IntroCrawl`, tutti i 32 eventi, la tabella delle 16 reliquie e i 3 finali.

---

## 3. Verifiche Eseguite

- **Audit ASCII 32..126 e contenuti**: 0 caratteri non-ASCII, 0 riferimenti calabresi, 0 termini da log/script, 0 inclusioni di `raylib.h` in `src/core/`.
- **Compilazione e Test (`build-nar2`, Ninja, C++20 `-Wall -Wextra -Wpedantic`)**:
  - `cmake -S . -B build-nar2 -G Ninja -DFETCHCONTENT_SOURCE_DIR_RAYLIB="$PWD/build/_deps/raylib-src"` -> **OK**
  - `cmake --build build-nar2 -j4` -> **0 errori, 0 warning**
  - `ctest --output-on-failure` -> **2/2 PASS (`eco_tests` + `headless_e2e`)**
- **Ispezione visiva in-game**: controllati gli screenshot di Cronologia Dialoghi `[H]`, Codex Reliquie `[J]` ed Epilogo (*Alba di Meridia*): testi leggibili, ben wrappati, senza troncamenti o sovrapposizioni.
