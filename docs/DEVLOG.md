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
