# Piano di Sviluppo — Verso EQUILIBRIO v1.0

Questo documento raccoglie sia gli interventi tecnici/ludici prioritari sullo stato attuale della demo (*Primo Altopiano — La Conca di Meridia*), sia le espansioni pianificate per la versione **1.0** e oltre.

---

## 1. Sprint Corrente — Miglioramenti Immediati (Priorità Alta)
Interventi sequenziali e verificati con test automatici e versionamento Git dedicato (il multiplayer locale è volutamente congelato/rimandato a dopo la 1.0):

- [ ] **Stabilità & Test Suite**:
  - Ripristinare la compilazione e l'esecuzione della suite E2E (`tests/e2e/e2e_live_adapter.cpp` e `tests/e2e/run_e2e.sh`), attualmente rotta dopo il refactor da `architect_npc` a `npcs`.
  - Proteggere `InitWindow()` in `src/app/main.cpp` con verifica `IsWindowReady()` per evitare crash `SIGSEGV (139)` in ambienti senza display server.
- [ ] **Motore Fisico, Game Feel & Prestazioni Core**:
  - Eliminare le allocazioni heap per-frame in `PlayerManager::ActivePlayers()` (~30+ chiamate/frame) introducendo iterazione zero-allocazione.
  - Implementare il **salto ad altezza variabile** (*variable jump height* / jump-cut al rilascio di `jump_held`) nel side-scrolling (`src/core/physics2d.cpp`).
  - Aggiungere **inerzia morbida** (accelerazione, decelerazione e attrito controllato) sia in top-down che in side-scrolling senza rompere la precisione dei puzzle.
- [ ] **Architettura & Pulizia Codice**:
  - Separare il rendering world-space di protagonista, monumenti, NPC e reliquie da `src/app/calibration_ui.cpp` in un modulo dedicato (`src/app/render/world_entities.*`), riducendo i monoliti.
  - Migliorare il motore audio procedurale (`src/app/audio/audio_engine.cpp`) con inviluppi anti-click (fade-in/fade-out e ADSR morbido) e throttling su passi/slider.
- [ ] **Modalità Hard ("Fisica Pura") — Prima Implementazione Giocabile**:
  - Aggiungere un'opzione selezionabile (`Modalita' Fisica Pura / Hard`) nelle Impostazioni di Gioco e nello stato del mondo.
  - In modalità Hard, il banco di calibrazione rimuove le tacche del target, gli indicatori verdi live e l'anteprima di efficienza prima della prova/conferma: il giocatore deve studiare il principio fisico, leggere i dati sul campo e calcolare autonomamente il valore da impostare con la rotellina/slider.

---

## 2. Roadmap verso la Versione 1.0 (Espansione Contenuti)

### 2.1 Base del Giocatore (Rifugio / Laboratorio di Elena)
- [ ] Area hub persistente dedicata al giocatore (accessibile dall'Overworld).
- [ ] Banco da lavoro / lavagna di fisica dove consultare le leggi scoperte, rivedere le formule, testare strumenti e pianificare le spedizioni.
- [ ] Esposizione fisica delle reliquie raccolte (museo/archivio visivo oltre al Codex `[J]`).
- [ ] Punto di partenza per i viaggi rapidi verso le nuove macro-regioni.

### 2.2 Espansione del Mondo oltre il Primo Altopiano
La demo attuale rappresenta il **Primo Altopiano** (6 distretti + 3 strutture). Per la v1.0 sono previste nuove macro-aree con accesso legato a requisiti fisici e strumenti sbloccati:
- [ ] **Zone Sotterranee Profonde**: reti di caverne geologiche, falde acquifere ad alta pressione e condotti termici accessibili solo dopo aver padroneggiato idrostatica/termodinamica o costruito equipaggiamento adeguato.
- [ ] **Zone Sopraelevate / Picchi**: osservatori in quota, ponti aerei e stazioni eoliche/ottiche raggiungibili completando determinate catene di puzzle o attivando meccanismi di sollevamento nell'altopiano.
- [ ] **Progressione meno lineare**: più percorsi opzionali, scorciatoie sbloccabili e aree secondarie ricompensate con strumenti ludici reali.

### 2.3 Evoluzione della Modalità Hard ("Fisica Pura")
- [ ] Differenziare completamente la modalità **Standard** (accessibile, con feedback visivo e curve guidate) dalla modalità **Hard / Fisica Pura**:
  - Nessun indice di target sullo slider, nessuna barra verde live, nessun aiuto GPS/bussola automatica.
  - Dati ambientali da rilevare sul campo (ombre, altezze, temperature, frequenze) per calcolare a mano la soluzione con la legge fisica corrispondente.
  - Impostazione manuale pura tramite rotellina del mouse / slider fine, con riscontro reale sul monumento solo all'avvio della prova.
  - Badge / tracciamento dedicato nel salvataggio e nell'epilogo per chi completa la campagna in Fisica Pura.

### 2.4 Evoluzione Artistica & Gameplay
- [ ] Valutare l'introduzione di sprite sheet o animazioni 2D più espressive per Elena e gli NPC, o un sistema di rendering ad alta definizione (scaling nitido su 1440p/4K).
- [ ] Arricchire le meccaniche fisiche in tempo reale nel mondo (es. specchi con angoli continui $\theta_i = \theta_r$, accordatura acustica reale).

---

## 3. Post-1.0 (Dopo il Rilascio Stabile Desktop)
- [ ] **Porting Mobile (Android / iOS)**:
  - Interfaccia adattiva per schermi touch e aspect ratio variabili.
  - Controlli virtuali (joystick/tap-to-move, rotellina touch per calibrazione fine).
  - Ottimizzazione batteria e profilo grafico dedicato.
- [ ] **Multiplayer Cooperativo Locale (1-4 Giocatori)**:
  - Riprendere l'infrastruttura già predisposta (`PlayerManager`): separare WASD/Frecce su tastiera, camera con zoom dinamico/clamping e ruoli cooperativi nei puzzle.
