# 04 - Integrazione Applicativa, Diario (Cronologia & Codex), Controlli e QA

**Autore**: `@ingegnere-2` (App Integration, History/Codex UI, Controls & QA)  
**Data**: 22 novembre 2026  
**Ambientazione**: *Equilibrio - La Conca di Meridia*

## 1. Obiettivi Completati
1. **Cronologia dei Dialoghi (`[H]`) e Codex delle Reliquie (`[J]`)**:
   - Introdotto `JournalOverlay` in `src/app/story_ui.h` e `src/app/story_ui.cpp` integrato con gli helper di stile condivisi (`ui_style.h`) e la palette materica di Meridia (`palette.h`).
   - **Tab Cronologia Dialoghi (`[H]`)**: mostra in ordine cronologico tutte le battute registrate da `StoryDirector::History()` (incluse le note di raccolta reliquie), con badge cromatici dedicati ai 6 parlanti (*Elena Valli*, *Clara Sereni*, *Lorenzo Altieri*, *Miriam Ferro*, *Matteo Verrini*, *Eco di Meridia*), tag capitolo + luogo, word-wrap automatico, filtro per capitolo (`Tutti`, `Prologo`, `Cap. I`..`Cap. V`, `Epilogo`) via `[A/D]` o click, e scorrimento fluido tramite `[W/S]`, Frecce, `PageUp/PageDown`, `Home/End` e rotellina del mouse con scrollbar dedicata.
   - **Tab Codex Reliquie (`[J]`)**: archivio completo delle 16 reliquie divise per le 4 Ere (*Alba delle Ombre*, *Secolo delle Acque e del Suono*, *Inverno di Pece*, *Sigillo dell'Armonia*), barra di completamento globale, filtri per Era, lista scorribile con glifi vettoriali (`DrawRelicGlyph`) e indizi di zona per le reliquie non ancora trovate, piu' pannello lettura scorribile con citazione di ritrovamento e paragrafi di lore completi.
   - Chiusura pulita con `[ESC]`, `[H]` o `[J]`, cambio tab con `[TAB]` o click, e pausa automatica della simulazione sottostante cosi' nessun input filtra al gameplay.
2. **HUD Narrativa, Banner Diegetici e Tre Finali (`src/app/story_ui.cpp`)**:
   - Pannello obiettivo in alto a sinistra aggiornato alla Campagna Completa di Meridia.
   - **Banner toast diegetico** in alto al centro (`DrawBannerToast`) con fade-in/fade-out morbido sia all'ingresso in ciascuno dei 6 distretti e delle 3 strutture interne, sia alla raccolta di una nuova reliquia.
   - **Dialogue Box** con word-wrap automatico multi-riga, effetto macchina da scrivere, respiro luminoso per l'*Eco di Meridia* (eliminato ogni glitch/jitter da script) e contatore rapido `[H] Cronologia (N) | [N] Avanti`.
   - **Schermata Epilogo** completa per i 3 finali (`Alba di Meridia`, `Rinascita della Conca`, `Il Prezzo della Fretta`) via `eco::EndingText(clean_path, relics_found, total_relics)`.
3. **Schermata del Titolo, Intro e Menu di Pausa/Impostazioni**:
   - `src/app/title_state.h/.cpp`: panorama procedurale interamente ridisegnato su **La Conca di Meridia** (anfiteatro calcareo-basaltico, Torre del Pendolo animata, cupola armillare del Santuario, Ponte degli Archi con lanterne meridiane, canale riflettente e polline/lucciole).
   - `src/app/intro_state.h/.cpp`: prologo di *Elena Valli* con griglia cartografica/geodetica e word-wrap automatico dei paragrafi.
   - `src/app/pause_settings_menu.h/.cpp`: aggiunta la voce diretta `"Diario: Cronologia & Codex [H / J]"` nel menu di pausa, priorita' `[ESC]` coerente (`Calibrazione -> Diario -> Menu Pausa`), rimosso il toggle obsoleto Demo vs Campagna (la Campagna Completa e' ora l'esperienza principale) e rimosso ogni residuo calabrese o "script".
4. **Input, Build System e Flag CLI per QA/Screenshot**:
   - `src/core/input_types.h` e `src/app/raylib_input.cpp`: aggiunti `toggle_history_pressed` (`[H]`) e `toggle_codex_pressed` (`[J]`) con latch/consume completi.
   - `CMakeLists.txt`: aggiunti `src/core/collectible.cpp` e `src/core/collectible_content.cpp` al target `game_core`.
   - `src/app/main.cpp`: integrato `JournalOverlay` nel loop principale, aggiornata l'esportazione `--screenshot` dalla `RenderTexture2D` virtuale 1280x720 (supporta percorsi assoluti) e aggiunti flag CLI `--zone`, `--structure`, `--open-history`, `--open-codex`, `--collect-all`, `--clean-all`, `--epilogue`.
5. **Suite di Test Completa (`tests/test_main.cpp` e `--headless-e2e`)**:
   - Verificati: mappa Overworld `36x50` con 6 distretti e nuovi `TileId`, 3 strutture interne side-scrolling con portali multipli e anti-loop, tutte e 6 le leggi fisiche (Lambert, Stevino, Galileo, Risonanza Armonica, Carnot, Interferenza Astrolabio), 16 collezionabili con audit automatico ASCII 32..126 e zero token legacy (Calabria/GAIA/AABB/script), `StoryDirector::History()` e `PushAdHocLine`, 7 capitoli, 32 eventi, 4 NPC e 3 finali.

## 2. File Modificati / Creati
- `CMakeLists.txt`
- `src/core/input_types.h`
- `src/app/raylib_input.cpp`
- `src/app/story_ui.h`, `src/app/story_ui.cpp`
- `src/app/title_state.h`, `src/app/title_state.cpp`
- `src/app/intro_state.h`, `src/app/intro_state.cpp`
- `src/app/pause_settings_menu.h`, `src/app/pause_settings_menu.cpp`
- `src/app/main.cpp`
- `tests/test_main.cpp`
- `docs/modifiche/04_ui_cronologia_e_test.md`

## 3. Verifica Tecnica e Visiva
- Build pulita in `build-ing2` (`Ninja`, C++20, `-Wall -Wextra -Wpedantic`): **0 errori, 0 warning**.
- `ctest --output-on-failure`: **2/2 test verdi (`eco_tests`, `headless_e2e`)**.
- Screenshot reali catturati e ispezionati per Titolo, Intro, Overworld HUD + Banner, Cronologia Dialoghi `[H]`, Codex Reliquie `[J]`, Epilogo *Alba di Meridia* e Menu di Pausa.
