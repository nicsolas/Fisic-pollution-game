# DEVLOG

## Coordinatore — analisi iniziale
- Build e ctest (eco_tests, headless_e2e) verdi prima delle modifiche.
- Grafica: rettangoli piatti + testo di debug; solo P0 disegnato (bug); HUD sovraccarico; nessuna storia/titolo/obiettivi.
- Due concept in conflitto in `Documents/`: uniti in `docs/VISION.md`.

## Coordinatore — Fase 0 completata
- Creati README, docs/*, stub (story.*, render/*, title/intro/story_ui) e inseriti in CMake.
- main.cpp: flag `--screenshot`, `--scene`, `--pollution`, `--skip-title`; finestra "EQUILIBRIO - Le Leggi della Valle".
- Build e ctest verdi (2/2). Screenshot di base: docs/screenshots/00_prima.png.

## Narratore — Fase 2 completata
- Scritto `docs/STORIA.md`: bibbia narrativa completa (premessa, cronologia 2048-2091, 4 pilastri, 5 personaggi, formule fisiche diegetiche di Lambert e Stevino, struttura in 5 capitoli e due finali).
- Scritto `docs/DIALOGHI.md`: copione completo per tutti gli `StoryEvent`, titoli dei capitoli, `IntroCrawl`, `ObjectiveFor` guidato dallo stato e i due finali (`EndingText`).
- Implementato `src/core/story_content.cpp` (100% ASCII puro per il font Raylib, max <= 98 caratteri per riga, dialoghi contestuali per percorso pulito/grigio).
- Compilato e verificato con `ctest` in `build-narratore` (2/2 test verdi, zero warning).

## Ingegnere — Fase 2 completata
- `src/core/story.cpp`: implementati `ChapterFor` (Prologo -> Atto1_Luce -> Atto2_Acqua -> Atto3_Codice -> Epilogo, con supporto sia al percorso pulito che al bypass della turbina per il Finale Grigio) e `StoryDirector` (`Reset`, `Observe`, `Notify`, `Tick` con durata `2.5s + 0.045s/char`, `AdvanceDialogue`, `CleanPath`).
- `src/core/simulation.h/.cpp`: integrato `StoryDirector story` in `WorldContext`; osservazione e tick a ogni step (anche con modal aperto); sblocco `simulation_completed` sia su percorso pulito sia su attivazione con bypass del Risonatore/Purificatore Est.
- `src/core/input_types.h` e `src/app/raylib_input.cpp`: aggiunto `advance_dialogue_pressed` mappato su `KEY_N` e `GAMEPAD_BUTTON_RIGHT_FACE_UP`, gestito in `LatchFrameInput` e `ConsumePressedEdges`.
- `src/app/title_state.h/.cpp` e `src/app/intro_state.h/.cpp`: schermata del titolo animata procedurale (Valle del Crati, Ponte Bucci, Diga, cenere->lucciole) e scena introduttiva con `IntroCrawl()` a macchina da scrivere (`--scene intro` aggiunto in `main.cpp`).
- `src/app/story_ui.cpp`: implementato `DrawStoryLayer` con Objective Panel in alto a sinistra (`x=16, y=62, w=410, h=64`), Dialogue Box in basso (`x=140, y=614, w=1000, h=90`, nascosto se il modal e' aperto, 5 speaker colorati e jitter per GAIA) e schermata finale Epilogo Pulito/Grigio.
- `tests/test_main.cpp`: aggiunto `TestStoryDirector()` (tutti i rami di `ChapterFor`, one-shot eventi, `CleanPath()`, `Tick()`, non-vuotezza contenuti di `story_content.cpp`). Build e `ctest` 2/2 verdi in `build-ingegnere` con zero warning.

## Artista — Fase 2 completata
- `docs/ART_DIRECTION.md`: definite palette "Valle Ferita" <-> "Valle Guarita", luce direzionale Nord-Ovest, rilievo 2.5D, regole di leggibilita' e layout HUD coordinato con `DrawStoryLayer`.
- `src/app/render/palette.h` e `src/app/render/ui_style.h/.cpp`: implementati `pal::WorldColor` con curva morbida, `PlayerAccentColor(0..3)`, `DrawPanel`, `DrawPanelWithHeader`, `DrawTextShadow` e `DrawProgressBar`.
- `src/app/render/procedural_art.h/.cpp`: hash deterministico `Hash2(x, y, seed)`, terreno organico con ciottoli/erba/fiori, sentiero lastricato nel campus, Canale del Crati animato, Ponte Bucci (crollato con laser / passerella reticolare attiva), rovine 2.5D con ombre portate e muschio, alberi procedurali, sfondo Diga in parallasse con cascate e passerelle a traliccio bullonate, cartelli diegetici compatti.
- `src/app/render/particles.h/.cpp`: sistema cosmetico zero-allocazioni per lucciole/polline/foglie (Overworld pulito), vapore/gocce (Diga), god-rays solari, coltre di smog, cenere obliqua e vignettatura.
- `src/app/calibration_ui.cpp`, `src/app/overworld_state.cpp`, `src/app/structure_state.cpp`: ridisegnati Ada e tutti i giocatori 1-4 (fix bug multiplayer), Lia l'Architetta con casco giallo e tubo portaprogetti, Torre del Risonatore Fotonico distinta dal Campo Solare, HUD superiore compatto con Scanner/Telemetria a destra e integrazione di `DrawStoryLayer(ctx_.world)`.
- Build e `ctest` 2/2 verdi in `build-artista` senza warning. Screenshot verificati (`03_valle_inquinata.png`, `04_valle_guarita.png`, `05_diga.png`).

## Coordinatore — Fase 3 (Integrazione, Screenshot Finali e Revisione Incrociata)
- Compilato il progetto integrato in `build/` con zero warning (`-Wall -Wextra -Wpedantic`).
- Eseguiti tutti i test: `ctest` (2/2 verdi: `eco_tests` incluso `TestStoryDirector` + `headless_e2e`) e `./tests/e2e/run_e2e.sh` (300/300 test E2E live passati). Verificato che `src/core` non ha alcuna dipendenza da Raylib.
- Generati e ispezionati gli screenshot finali in `docs/screenshots/`:
  - `01_titolo.png` (TitleState procedurale con Ponte Bucci e Diga)
  - `02_intro.png` (IntroState diario di bordo a macchina da scrivere)
  - `03_valle_inquinata.png` (Overworld con smog 35%, terreno ocra, cenere, Objective Panel + Dialogue Box)
  - `04_valle_guarita.png` (Overworld con smog 0%, prato verde, fiori, god-rays e lucciole)
  - `05_diga.png` (StructureState Diga del Crati con parallasse, cascate, passerelle a traliccio e turbina)
- Completata la revisione incrociata sul board tra `narratore` (verifica di tutti i rami di `ChapterFor`/`ObjectiveFor` incluso anti-softlock su bypass solare e esplorazione non lineare) e `ingegnere` (verifica geometrica zero-overlap tra Top-Bar, Objective Panel, Telemetria, Dialogue Box e Modal di Calibrazione).
- Terminati i worker dello swarm e rimosse le cartelle temporanee `build-narratore`, `build-ingegnere`, `build-artista`.

## Coordinatore — v0.2.0 (Git Versioning, Font TTF, Menu Pausa ESC, Impostazioni Grafiche e Schermo Intero)
- Inizializzato repository Git (`git init -b main`), creato `.gitignore` e taggata la prima versione completa come **`v0.1.0`**.
- Integrato il font ad alta leggibilita' **Noto Sans SemiBold TTF** (`assets/fonts/ui_font.ttf`) in `src/app/render/ui_style.*` con filtro bilineare (`InitUiFont`, `MeasureUiText`, `DrawTextShadow`), eliminando l'effetto pixelato confusionario del font bitmap di default (che resta selezionabile nelle impostazioni come opzione retro).
- Risolto il problema dello **Schermo Intero** e del ridimensionamento finestra in `src/app/main.cpp`:
  - Abilitato `FLAG_WINDOW_RESIZABLE` e `ToggleBorderlessWindowed()` (scorciatoia rapida **`F11`** o **`Alt+Invio`**, oppure dal menu Impostazioni).
  - Introdotto rendering su canvas virtuale `1280x720` con scaling letterbox 16:9 automatico e rimappatura precisa del mouse (`SetMouseOffset` / `SetMouseScale`), cosi' tutti i pulsanti e lo slider funzionano al pixel anche a schermo intero.
- Disabilitata l'uscita immediata con `ESC` (`SetExitKey(KEY_NULL)`): ora **`ESC` apre/chiude il Menu di Pausa** (`src/app/pause_settings_menu.h/.cpp`) con 5 voci (*Riprendi Esplorazione*, *Impostazioni Grafiche & Schermo*, *Ricomincia Partita*, *Torna al Titolo*, *Esci dal Gioco*).
- Creato il **Menu Impostazioni Grafiche & Interfaccia** (controllabile sia con tastiera `W/S/A/D/Frecce/Invio` sia con **Click del Mouse**) con 7 opzioni in tempo reale:
  1. Modalita' Schermo (`Finestra Ridimensionabile` <-> `Schermo Intero Borderless`)
  2. Risoluzione Finestra (`1280x720 HD`, `1600x900 HD+`, `1920x1080 Full HD`)
  3. Frequenza Fotogrammi (`60 FPS`, `120 FPS`, `144 FPS`, `Illimitato`)
  4. Qualita' Particelle & Atmosfera (`Alta 100%`, `Media 50%`, `Disattivata`)
  5. Stile Font Interfaccia (`Noto Sans HD Nitido` <-> `Pixel Bitmap Retro`)
  6. Contatore FPS a Schermo (`Attivo` <-> `Disattivato`)
  7. Ritmo Dialoghi & Tutorial (`Completo` <-> `Sintetico / Rapido`)
- Generati gli screenshot `06_menu_pausa.png` e `07_menu_impostazioni.png` e creato il tag Git **`v0.2.0`**.

## Narratore (narratore2) — v0.3.0 (Espansione Storia: Atto IV & Atto V + Debugging Graduale)
- Espansi `docs/STORIA.md` e `docs/DIALOGHI.md` con:
  - **Atto IV — "L'Archivio Sommerso" (`Chapter::Atto4_Archivio`)**: sblocco della paratia superiore della Diga del Crati dopo l'attivazione del Risonatore Fotonico, incontro fisico con **Nonna Rosa** (`rosa_npc` a `{1056, 240}`) sulla piattaforma alta a destra e consegna della chiave magnetica dello Scambiatore Geotermico (`StoryEvent::RosaRecruited`).
  - **Atto V — "Il Nucleo di GAIA" (`Chapter::Atto5_Inverno`)**: calibrazione dello **Scambiatore Geotermico di Carnot** a Nord-Est (`thermal_machine` a `{1128, 216}`, target `60 deg`, `StoryEvent::ThermalClean` / `StoryEvent::ThermalBypassed`) e decodifica finale del **Monolite / Terminale Sorgente di GAIA** a Sud-Est (`{1128, 648}`, `StoryEvent::GaiaTerminalDecoded`), con la rivelazione che Ada Ferri stessa ha programmato GAIA nel 2072.
- Aggiornato `src/core/story_content.cpp` con `ChapterTitle`, `DialogueForEvent` (4 nuovi eventi + rami `!s.demo_mode` su `PurifierClean` e `PurifierBypassed`) e `ObjectiveFor` per tutti i 7 capitoli, mantenendo 100% compatibilita' ASCII e zero warning in `story_content.cpp`.
- Compilato e verificato `ctest` (2/2 verdi) in `build-narratore2`.

## Artista (`artista2`) — v0.3.0 (Grafica Procedurale Atti IV e V)
- `src/app/calibration_ui.cpp`:
  - Implementato `DrawNonnaRosaNpc(world)` per disegnare Nonna Rosa sulla piattaforma superiore destra della Diga del Crati (`StructureState`) quando `!world.demo_mode` (capelli argento raccolti in uno chignon, occhiali tondi, cappotto tecnico blu idraulico con bordi ambra, tablet idrometrico, badge `"NONNA ROSA (IDRAULICA)"`, `"!"` flottante e prompt di interazione `[E]`).
  - Implementato `DrawGeothermalApparatus(world.thermal_machine, world)` per disegnare lo Scambiatore Geotermico di Carnot nel quadrante Nord-Est oltre il Ponte Bucci (pozzo incassato nella roccia, anelli in rame/ottone, nucleo termico pulsante, valvola rotante a `current_param` vs tacca a `target_param = 60 deg`, vapore pulito / fumo da bypass e indicatore di sblocco tramite Nonna Rosa).
  - Implementato `DrawGaiaSourceTerminal(world)` nel quadrante Sud-Est (monolite esagonale in ossidiana con anelli olografici orbitanti, schermo a onda sinusoidale verde matrice, etichetta `"NUCLEO SORGENTE GAIA"` e prompt `[E]`).
  - Aggiornato `DrawHudAndCalibrationModalScreenSpace` per supportare il titolo del modal di Carnot (`world.calibrating_thermal`) e il reset dei flag di calibrazione sui click del mouse.
- `docs/ART_DIRECTION.md`: documentata la sezione 9 sull'espansione visiva degli Atti IV e V.
- Compilazione e `ctest` (2/2) verdi in `build-artista2` con zero warning.

## Ingegnere (`ingegnere2`) — v0.3.0 (Atto IV & Atto V in `story.cpp`/`simulation.cpp`, Modalita' Demo vs Campagna Completa, Unit Test)
- Aggiornato `src/core/story.cpp`:
  - `ChapterFor(const StorySnapshot& s)` supporta `Chapter::Atto4_Archivio` (dopo l'attivazione del Risonatore Fotonico a Est quando `!s.demo_mode`) e `Chapter::Atto5_Inverno` (dopo il reclutamento di Nonna Rosa nella Diga).
  - `StoryDirector::Observe` emette gli eventi one-shot `RosaRecruited`, `ThermalClean`, `ThermalBypassed` e `GaiaTerminalDecoded`.
  - `StoryDirector::CleanPath()` include anche `!EventFired(StoryEvent::ThermalBypassed)`.
- Aggiornato `src/core/simulation.cpp`:
  - `WorldContext::Reset()` preserva `demo_mode` e inizializza `thermal_machine` (`{1128, 216}`, target `60 deg`), `rosa_npc` (`{1056, 240}` sulla piattaforma alta della Diga) e `gaia_terminal_pos` (`{1128, 648}`).
  - Gestita la scansione empirica (`TriggerScan`), il modal (`ModalTargetMachine`) e l'interazione `[E]` con Nonna Rosa nella Diga, con lo Scambiatore Geotermico di Carnot e col Terminale Sorgente di GAIA, mantenendo 100% compatibilita' con la Demo v0.2 quando `demo_mode == true`.
- Aggiornati `src/app/story_ui.cpp`, `src/app/pause_settings_menu.h/.cpp` e `src/app/main.cpp`:
  - Aggiunto flag CLI `--demo` e voce **"Modalita' Storia: `< Campagna Completa (Atti I-V) >` / `< Demo Classica (Atti I-III) >`"** nel Menu Impostazioni.
- Esteso `TestStoryDirector()` in `tests/test_main.cpp` con test completi per i 7 capitoli e simulazione E2E dell'Atto IV e Atto V su `WorldContext`. Build e `ctest` (2/2) verdi in `build-ingegnere2` con zero warning.

## Coordinatore — v0.3.1 (Fix Piattaforme Diga per Nonna Rosa & Fix Fullscreen Wayland)
- **Fix Piattaforme Diga (`src/core/tilemap.cpp` + `src/core/simulation.cpp`)**:
  - Calcolata l'altezza massima del salto (`h_max = v_0^2 / (2g) = 480^2 / 2400 = 96 px = 2 tile`): la prima piattaforma in `CreateStructureMap` era a `row 10` (`144 px = 3 tile` dal pavimento a `row 13`), quindi irraggiungibile dal suolo!
  - Abbassate le tre piattaforme di 1 tile (`row 11` a `528 px`, `row 9` a `432 px`, `row 7` a `336 px` -> ciascuna a esattamente `96 px` dalla precedente) e riposizionata Nonna Rosa sulla piattaforma superiore (`{1056, 322}` con `trigger_radius = 96 px`).
  - Aggiunto un test fisico automatico in `tests/test_main.cpp` che fa camminare e saltare realmente il giocatore dal pavimento fino alla piattaforma di Nonna Rosa e interagisce con `[E]`.
- **Fix Errore Fullscreen su Wayland (`src/app/pause_settings_menu.cpp`)**:
  - Sostituito `ToggleBorderlessWindowed()` (che in Raylib 5.5 chiama `glfwSetWindowPos`, non supportato dal protocollo Wayland con `WARNING: GLFW: Error: 65548`) con `SetWindowSize(mon_w, mon_h)` + `ToggleFullscreen()`, che su Wayland usa nativamente `xdg_toplevel_set_fullscreen` senza alcun errore di posizionamento finestra.

---

## 2026-11-22 — Milestone v0.4.0 "La Conca di Meridia" (Squadra a 6 Agenti)

- **`narratore-1`**: rifondata l'intera ambientazione (rimossa la Calabria e ogni tono da script) nella civilta' perduta dei **Meridiani** (*La Conca di Meridia*), 4 Ere storiche, 6 distretti, 3 strutture, 5 personaggi principali + Eco di Meridia, 7 capitoli e 3 finali (`docs/STORIA.md`, `docs/VISION.md`, `docs/modifiche/01_mondo_e_civilta.md`).
- **`narratore-2`**: scritti tutti i dialoghi letterari per i 7 capitoli e 32 eventi (`src/core/story_content.cpp`), le 16 reliquie `I.1`..`IV.4` con note di raccolta e schede Codex (`src/core/collectible_content.cpp`), e aggiornato `docs/DIALOGHI.md` (`docs/modifiche/02_dialoghi_e_reliquie.md`).
- **`ingegnere-1`**: espanso il motore core puro C++20 con Overworld `36x50` a 6 distretti, 10 `TileId`, 3 strutture side-scrolling con portali dedicati, 6 leggi fisiche autentiche (Lambert, Stevino, Pendolo di Galileo, Risonanza Lorentziana, Carnot, Interferenza di Fase), registro 16 collezionabili, 4 NPC e `StoryDirector::History()` (`docs/modifiche/03_motore_core_e_zone.md`).
- **`ingegnere-2`**: integrato il **Diario di Campo** (`[H]` Cronologia Dialoghi scorribile/filtrabile + `[J]` Codex Reliquie), banner diegetici di zona/reliquia, schermate Titolo/Intro/Pausa Meridia, flag CLI per screenshot, `CMakeLists.txt` e suite completa `eco_tests` + `headless_e2e` (`docs/modifiche/04_ui_cronologia_e_test.md`, `docs/modifiche/04b_fix_separatore_cronologia.md`).
- **`artista-1`**: realizzate le palette multi-bioma (`palette.h`), il rendering 2.5D con viewport culling dei 6 distretti (mosaici astronomici, acquedotti, Ponte degli Archi, rovine, flora dinamica, 3 portali) e i 3 fondali in parallasse delle strutture interne (`docs/ART_DIRECTION.md`, `docs/modifiche/05_ambientazioni_e_dettagli.md`).
- **`artista-2`**: disegnati Elena Valli + 4 NPC, i 6 monumenti fisici con diagrammi live nel modal di calibrazione, le 16 reliquie animate in 5 forme, la minimappa `MAPPA DI MERIDIA`, i VFX per bioma/struttura e gli helper condivisi in `ui_style.*` (`docs/modifiche/06_personaggi_collezionabili_vfx.md`).
- **`coordinatore`**: integrazione finale in `./build/` (0 errori, 0 warning, `ctest` 2/2 PASS), audit zero residui legacy, aggiornamento `README.md`/`docs/*` e raccolta 12 nuovi screenshot (`docs/modifiche/07_integrazione_finale_e_verifica.md`).

---

## 2026-11-22 — Milestone v0.4.1 "Audio Procedurale & Suoni di Interazione"

- **Audio Engine (`src/app/audio/audio_engine.h/.cpp`)**: sintetizzatore PCM procedurale in C++20 su Raylib/miniaudio (zero asset esterni, `src/core/` invariato) con **18 suoni di interazione e fisica** (passi su pietra/erba, salto/atterraggio, Lente Empirica, reliquie, calibrazione pulita/bypass, Ponte degli Archi, portali, cambio distretto/fase, menu/tab — **nessun suono o voce sui dialoghi**) e **5 loop ambientali continui** (Overworld pulito/foschia, Cisterne Ipogee, Torre del Pendolo, Camera Geotermica).
- **Impostazioni & Controlli (`pause_settings_menu.*`, `main.cpp`)**: aggiunte le voci `Volume Audio [M Muto]` e `Paesaggio Sonoro` (`Ambiente + Interazioni` / `Solo Interazioni`), scorciatoia `[M]` e flag `--mute`.
- **Verifica & Log**: build pulita (0 warning), `ctest` 2/2 PASS, verifica su PulseAudio/miniaudio, screenshot `docs/screenshots/20_impostazioni_audio.png` e log in `docs/modifiche/08_audio_procedurale_e_interazioni.md`.

---

## 2026-11-22 — Milestone v0.4.2 "Selezione Monitor, Selezione GPU & Salvataggio Partita"

- **Salvataggio & Menu Riprendi (`src/core/save_data.*`, `story.*`, `title_state.*`)**: salvataggio atomico in `saves/conca_di_meridia.sav` (posizione, zona/struttura, 6 monumenti, 4 NPC, 16 reliquie, Cronologia Dialoghi integrale), autosave su traguardi/cambi zona, pulsante `Salva Partita Ora` in pausa e menu del Titolo **"Riprendi da dove hai lasciato"** con **9 sfondi procedurali ridisegnati** in base alla zona o struttura salvata e al suo livello di foschia.
- **Selezione Monitor (`pause_settings_menu.*`)**: selettore `Monitor Schermo Intero` con `SetWindowMonitor` (fix Wayland/X11 multi-monitor) e persistenza in `saves/settings.cfg`.
- **Selezione GPU (`src/app/gpu_selector.*`, `main.cpp`)**: rilevazione GPU Linux DRM (`NVIDIA`, `Intel`, `AMD`), commutazione pre-`InitWindow()` via EGL/PRIME/DRI_PRIME (verificata dal vivo tra `NVIDIA GeForce RTX 3050` e `Mesa Intel(R) Graphics`), `GPU Attiva Ora` nelle Impostazioni, pulsante `Applica GPU & Riavvia` e flag CLI `--gpu` / `--monitor`.
- **Verifica & Log**: build pulita (0 warning), `ctest` 2/2 PASS (incluso `TestSaveAndLoadPersistence`), screenshot `docs/screenshots/21_impostazioni_monitor_gpu.png` e `22_titolo_riprendi_zona.png`, log in `docs/modifiche/09_monitor_gpu_e_salvataggio.md`.

---

## 2026-11-22 — Milestone v0.4.3 "Divisione Netta Zone, Memoria & Guida Giocatore" (`ingegnere-zone` + `artista-zone`)

- **`ingegnere-zone`**: confini fisici spessi 2 tile tra i 6 distretti con 5 `DistrictGateway` larghi 3 tile collegati da strade lastricate, `Tilemap` piatto contiguo `std::vector<std::uint8_t>` (1.800 B in 1 allocazione), helper zero-allocazione `ForEachMachineInScene`/`ForEachNpcInScene`, `CurrentObjectiveTarget()` e test BFS/memoria/guida (`docs/modifiche/10_divisione_zone_memoria_e_guida.md`).
- **`artista-zone`**: stacchi cromatici potenziati per i 6 distretti + tinta atmosferica interpolata, archi monumentali bicolore e stele direzionali sui 5 varchi, cache statica dei prop Overworld (zero scansioni/allocazioni per frame), bussola diegetica bronzo/giada con distanza in metri e `MAPPA DI MERIDIA` con distretto attivo, varchi e faro pulsante (`docs/modifiche/11_stacchi_visivi_portali_e_bussola.md`).
- **Verifica**: build `./build` a 0 warning, `ctest` 2/2 PASS, screenshot `23_varchi_bussola_alba.png`, `24_varchi_giardini_minimappa.png`, `25_stacco_caldera_arco.png`.
