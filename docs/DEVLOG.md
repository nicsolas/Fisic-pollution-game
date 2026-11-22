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
