# Discussione del Team — Verbale e Decisioni (Fase 1)

## Partecipanti
- **coordinatore** (Direzione e integrazione)
- **narratore** (Storia, personaggi, dialoghi, `story_content.cpp`)
- **ingegnere** (Architettura `StoryDirector`, stati Title/Intro, `story_ui`, test)
- **artista** (Grafica procedurale, palette dinamica, personaggi, HUD, particelle)

---

## Sintesi delle Proposte e Critiche

### Da @ingegnere (Ingegneria & Sistemi)
1. **Non-blocco garantito per E2E**: `StoryDirector::Observe()` e `Tick()` girano a fine `WorldContext::StepWorld` (anche col modal aperto) senza bloccare input o fisica.
2. **Retrocompatibilita' `architect_npc.last_dialogue`**: `ArchitectNpc::TryInteract` continua a popolare `last_dialogue` per i test esistenti, mentre `StoryDirector` emette `StoryEvent::ArchitectRecruited`.
3. **Input dedicato `advance_dialogue_pressed`**: mappato su `KEY_N` e `GAMEPAD_BUTTON_RIGHT_FACE_UP` per non confliggere con `E` / `INVIO`.
4. **Coda FIFO ed emissione one-shot**: ogni `StoryEvent` scatta al massimo una volta per sessione (`fired_`).
5. **Auto-avanzamento calibrato**: `2.5f + 0.045f * testo.size()`.
6. **Progressione `ChapterFor` pura dallo stato**: `Epilogo` se `simulation_completed`; `Atto3_Codice` se `architect_recruited && hydro == CleanActive`; `Atto2_Acqua` se `architect_recruited`; `Atto1_Luce` se `has_scanned || solar != Broken`; altrimenti `Prologo`.
7. **Tracciamento `CleanPath()`**: `false` se qualsiasi `*Bypassed` e' scattato.
8. **Flusso `TitleState` -> `IntroState` -> `OverworldState`**: paesaggio animato nel titolo, typewriter nell'intro con skip/avanzamento rapido.
9. **Separazione layout `DrawStoryLayer`**: Capitolo/Obiettivo in alto a sinistra (`x=16, y=64, w=380, h=68`), box dialoghi in basso (`y=616, h=88`) nascosto quando `calibration_modal.is_open == true`.
10. **Unit test `TestStoryDirector()`** in `tests/test_main.cpp`.

### Da @narratore (Storia & Design)
1. **Coerenza fisica del Purificatore (Atto III)**: battezzato **"Risonatore Fotonico / Specchio Eliostatico d'Alta Quota"** (`target_param = 120.0f` gradi) per dare pieno senso fisico alla legge del coseno e al ruolo di nodo atmosferico di GAIA.
2. **`ObjectiveFor` guidato dallo stato reale**: ispeziona lo snapshot per indicare sempre il prossimo passo non completato e il tasto esatto (`[F]`, `[E]`, portale).
3. **Observazione su `ChapterFor` e Bypass Turbina**: per mantenere il contratto del piano su `ChapterFor` (`architect_recruited && hydro == CleanActive` per `Atto3_Codice`), se il giocatore ha bypassato la turbina (`hydro == BypassedDirty`) `ObjectiveFor(Atto2_Acqua, s)` puo' comunque guidarlo verso il Risonatore oltre il Ponte Bucci o a ricalibrare la turbina in modo pulito.
4. **Rimozione vecchio box "PROTOCOLLO DI DECIFRAZIONE (3 ATTI)"**: evitare duplicazioni con il nuovo pannello Obiettivo di `DrawStoryLayer`.
5. **Feedback su Lia prima della calibrazione**: l'obiettivo guida chiaramente a ottenere Energia Pulita > 0 prima di reclutare Lia.
6. **Identita' vocale e cromatica dei 5 personaggi**: Ada (ciano), Lia (ambra/oro), Bruno Ceneri del Consorzio Brace (arancio ruggine, via radio sui bypass), Nonna Rosa (azzurro acqua, memorie della Diga), GAIA (verde matrice, sintassi `// GAIA [SYS]: ...`).
7. **Filtro ASCII rigoroso**: solo ASCII puro con apostrofi (`e'`, `piu'`), max 105 caratteri per battuta.
8. **Dialoghi reattivi allo snapshot**: `DialogueForEvent` tiene conto di eventuali bypass precedenti.

### Da @artista (Art Direction & Grafica Procedurale)
1. **Terreno procedurale con hash deterministico `Hash2(c, r, seed)`**: addio alla griglia piatta; variazioni di tono, ciottoli, erba e fiori che sbocciano quando l'inquinamento scende.
2. **Profondita' 2.5D e luce da Nord-Ovest**: muri con faccia superiore, fronte scuro di ~12 px, ombra a terra e muschio/rampicanti.
3. **Palette dinamica `pal::WorldColor(sick, healthy, poll)`** in `palette.h`.
4. **Canale del Crati e Ponte Bucci**: sponde in pietra, acqua animata, passerella reticolare omaggio all'Unical.
5. **Alberi e flora procedurale decorativa** in coordinate sicure senza collisioni.
6. **Diga del Crati (`StructureState`) con sfondo in parallasse**: arcate di cemento, condotte, cascate animate, tralicci metallici, cartelli diegetici al posto dei lunghi testi di debug.
7. **Fix bug multiplayer 1-4 giocatori**: `DrawScientistProtagonist` disegnato per tutti i `players.ActivePlayers()` con colore per `player_id`; Lia con casco giallo, progetti e `!` flottante.
8. **Tre macchine visivamente distinte**: Campo Solare Ovest, Turbina Idroelettrica nella Diga, e Risonatore Fotonico / Torre Eliostatica a Est.
9. **Particelle a pool fisso (`particles.*`)**: 96 particelle (cenere <-> polline/lucciole, vapore nella Diga), god-rays e vignettatura.
10. **Pulizia e allineamento HUD**: top-bar compatta (`h=52`), angolo alto-sinistra (`y=60..140`) e fascia bassa (`y>600`) riservati a `DrawStoryLayer`; scanner/telemetria spostato in alto a destra (`x = kWindowWidth - 420, y = 64`); chiamata a `DrawStoryLayer(ctx_.world)` a fine `OverworldState::Draw` e `StructureState::Draw`.

---

## Decisioni del Coordinatore (Approvate per la Fase 2)

1. **Contratto `ChapterFor` e `ObjectiveFor`**:
   - `ChapterFor` in `story.cpp` segue esattamente la regola del piano (`completed -> Epilogo`; `architect_recruited && hydro == CleanActive -> Atto3_Codice`; `architect_recruited -> Atto2_Acqua`; `has_scanned || solar != Broken -> Atto1_Luce`; altrimenti `Prologo`) cosi' tutti gli unit test sono deterministici e allineati al piano.
   - In `ObjectiveFor(Chapter, StorySnapshot)` (@narratore), se `architect_recruited && hydro == BypassedDirty`, l'obiettivo di Atto II informa il giocatore che puo' attraversare il Ponte Bucci verso il Risonatore a Est (o ricalibrare la turbina in modo pulito). Inoltre, se tutte le macchine sono attive (anche con bypass) o il purificatore viene attivato, il giocatore non resta mai senza obiettivo chiaro.
2. **Allineamento Layout UI/HUD**:
   - Approvata la divisione spaziale tra @artista e @ingegnere:
     - Top bar HUD: `y = 0..52`.
     - Pannello Capitolo/Obiettivo (`DrawStoryLayer`): alto-sinistra `x = 16, y = 62, w = 390, h = 70`.
     - Pannello Telemetria/Scanner (`calibration_ui.cpp`): alto-destra `x = kWindowWidth - 430, y = 62`.
     - Box Dialoghi (`DrawStoryLayer`): basso `x = 140, y = 612, w = 1000, h = 92` (nascosto automaticamente se `world.calibration_modal.is_open` o `world.simulation_completed`).
     - Rimosso da `calibration_ui.cpp` il vecchio box "PROTOCOLLO DI DECIFRAZIONE", la vecchia barra `last_dialogue` e il vecchio banner di vittoria (ora gestiti da `DrawStoryLayer`).
3. **Nome diegetico del terzo impianto**: approvato **"Risonatore Fotonico (Eliostato Est)"** sia nei dialoghi/obiettivi (@narratore) sia nel rendering procedurale (@artista) e nel titolo del modal quando `world.calibrating_purifier == true`.
4. **ASCII Puro e Test**: confermato zero caratteri accentati UTF-8 nelle stringhe renderizzate da Raylib; ogni worker compila e testa nella propria cartella `build-<nome>`.
