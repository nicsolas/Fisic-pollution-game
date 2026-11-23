# EQUILIBRIO — La Conca di Meridia

Videogioco 2D in **C++20 e Raylib 5.5** basato sulla fisica empirica, sulla matematica applicata, sull'ecologia e sull'archeologia di una civilta' perduta.

Ambientato sull'altopiano monumentale de **La Conca di Meridia**, segue la naturalista e cartografa **Elena Valli** mentre esplora **6 distretti separati in superficie** e **3 strutture interne**, raccoglie **16 reliquie** distribuite in **4 Ere storiche** e riattiva **5 monumenti fisici + l'Astrolabio Centrale** (Ottica di Lambert, Idrostatica di Stevino, Pendolo Isocrono di Galileo, Risonanza Armonica Lorentziana, Termodinamica di Carnot e Interferenza di Fase), scegliendo se guarire la Conca con il calcolo o cedere alle fornaci a pece di Matteo Verrini.

Include:
- **6 Zone Sequenziali a Mappa Dedicata (`26x36` tile)** caricate on-demand in memoria (`Terrazze dell'Alba -> Quartiere dei Canali -> Altopiano dei Venti -> Giardini Sospesi -> Caldera delle Fumarole -> Santuario dell'Armonia`), con **varchi di avanzamento vincolati al completamento della task principale** della zona corrente e ritorno sempre aperto.
- **3 Livelli dei Portali Ridisegnati come Test di Logica Side-Scrolling**: *Cisterne Ipogee* (`20x40`, **Equilibrio delle Paratoie** a logica booleana), *Torre del Pendolo* (`22x38`, **Sequenza Isocrona** a 4 campane ordinate) e *Camera Geotermica* (`20x42`, **Ciclo di Carnot** a 4 valvole accoppiate), tutti verificati da test automatici di fisica reale a 60 Hz.
- **6 Meccaniche Top-Down Uniche**: specchi e fasci solari con piastre fotosensibili e muro `SolarGate` completamente sigillato, correnti d'acqua fisiche con chiuse a leva, raffiche di vento periodiche con monoliti frangivento, piastre acustiche di risonanza armonica con sblocco del Ponte di Giada, geyser termici ciclici con valvole di sfogo, e accordatura dei sigilli elementali dell'Astrolabio.
- **16 Reliquie Lore-Accurate (di cui 7 Nascoste in Nicchie Segrete)**: posizionate in punti coerenti con l'architettura e la storia, con luccichio discreto in prossimita', rivelazione in Fase 3 (*Trama Geodetica*) e Codex `[J]`.
- **Intramezzi Panoramici tra Zone**: transizioni visive e narrative dipinte proceduralmente ad ogni passaggio di soglia, con parallasse, versi storici dedicati e skip rapido (`SPAZIO`).
- **Menu di Pausa & Impostazioni Riprogettato**: 4 schede intuitive (**Gioco**, **Grafica**, **Ambiente**, **Audio**), slider trascinabili col mouse, mute rapido `[M]`, reset predefiniti per scheda.
- **Controlli Effetti Ambientali & Audio Granulari**: intensita' regolabile in tempo reale per atmosfera, particelle, foschia/nebbia, raggi solari, vignettatura e meteo di zona; volumi separati per Master, Musica, Effetti e Ambiente naturale con anteprima live.
- **Cronologia dei Dialoghi (`[H]`)** e **Salvataggio Completo Anti-Softlock** ("Riprendi da dove hai lasciato") che persiste anche lo stato di tutte le leve, sequenze, paratoie e ponti dentro i livelli dei portali.
- **Selezione del Monitor** e **Selezione GPU OpenGL** (NVIDIA / Intel / AMD) su Linux e multi-schermo.

---

## Galleria Visiva — La Conca di Meridia

### Nuovi Livelli dei Portali a Logica & Zone Sequenziali

| Cisterne Ipogee — Equilibrio delle Paratoie (`27_cisterne_enigma_paratoie.png`) | Torre del Pendolo — Sequenza Isocrona (`28_torre_pendolo_sequenza.png`) | Camera Geotermica — Ciclo di Carnot (`29_camera_geotermica_carnot.png`) |
|---|---|---|
| ![Cisterne Ipogee Enigma](docs/screenshots/27_cisterne_enigma_paratoie.png) | ![Torre del Pendolo Sequenza](docs/screenshots/28_torre_pendolo_sequenza.png) | ![Camera Geotermica Carnot](docs/screenshots/29_camera_geotermica_carnot.png) |

| Terrazze dell'Alba — SolarGate & Varco Sigillato (`26_terrazze_solargate_varco.png`) | Quartiere dei Canali — Progressione Sequenziale (`30_canali_sequenziale_varco.png`) | Titolo — Riprendi Partita (`22_titolo_riprendi_zona.png`) |
|---|---|---|
| ![Terrazze SolarGate](docs/screenshots/26_terrazze_solargate_varco.png) | ![Canali Sequenziale](docs/screenshots/30_canali_sequenziale_varco.png) | ![Riprendi Partita](docs/screenshots/22_titolo_riprendi_zona.png) |

### Distretti, Interfacce e Impostazioni

| Impostazioni Schermo, GPU & Audio (`21_impostazioni_monitor_gpu.png`) | Giardini Sospesi (`10_giardini_sospesi.png`) | Caldera delle Fumarole (`11_caldera_fumarole.png`) |
|---|---|---|
| ![Impostazioni Monitor & GPU](docs/screenshots/21_impostazioni_monitor_gpu.png) | ![Giardini Sospesi](docs/screenshots/10_giardini_sospesi.png) | ![Caldera delle Fumarole](docs/screenshots/11_caldera_fumarole.png) |

| Santuario dell'Armonia (`12_santuario_astrolabio.png`) | Cronologia Dialoghi `[H]` (`16_cronologia_dialoghi.png`) | Codex Reliquie `[J]` (`17_codex_reliquie.png`) |
|---|---|---|
| ![Santuario dell'Armonia](docs/screenshots/12_santuario_astrolabio.png) | ![Cronologia Dialoghi](docs/screenshots/16_cronologia_dialoghi.png) | ![Codex Reliquie](docs/screenshots/17_codex_reliquie.png) |

| Modal Risonatore (`18_modal_risonatore.png`) | Epilogo — Alba su Meridia (`19_epilogo_alba_meridia.png`) | Impostazioni Audio (`20_impostazioni_audio.png`) |
|---|---|---|
| ![Modal Risonatore](docs/screenshots/18_modal_risonatore.png) | ![Epilogo Alba su Meridia](docs/screenshots/19_epilogo_alba_meridia.png) | ![Impostazioni Audio](docs/screenshots/20_impostazioni_audio.png) |

---

## Compilazione, Esecuzione e Test

```bash
# Configura e compila
cmake -S . -B build
cmake --build build -j8

# Avvia il gioco (Campagna Completa attiva di default)
./build/eco_proto

# Esegui unit test, test di fisica/logica a 60 Hz e test headless E2E
ctest --test-dir build --output-on-failure
```

### Opzioni da Riga di Comando (`eco_proto`)
- `--gpu auto|nvidia|intel|amd`: seleziona la GPU OpenGL prima dell'avvio (su Linux commuta tra NVIDIA PRIME/EGL e Mesa Intel/AMD; impostabile anche in-game nelle Impostazioni con pulsante `Applica GPU & Riavvia`).
- `--monitor -1|0|1|...`: sceglie il monitor per lo schermo intero (`-1` = monitor corrente, `0` = Monitor 1, `1` = Monitor 2, ecc.; regolabile anche dal menu Impostazioni e salvato in `saves/settings.cfg`).
- `--mute`: avvia il gioco con l'audio silenziato.
- `--skip-title`: salta il titolo e avvia direttamente nell'Overworld.
- `--scene title|intro|overworld|structure`: avvia nella scena indicata.
- `--zone dawn|canals|wind|gardens|caldera|sanctuary`: posiziona Elena nel distretto scelto.
- `--structure cisterns|pendulum|geothermal` (o `0|1|2`): entra direttamente nella struttura interna indicata.
- `--pollution N`: imposta la foschia iniziale (`0..100`).
- `--open-history` / `--open-codex`: apre subito il Diario sul tab Cronologia Dialoghi (`[H]`) o Codex Reliquie (`[J]`).
- `--open-pause` / `--open-settings`: apre direttamente il menu di pausa o le impostazioni.
- `--collect-all` / `--clean-all` / `--epilogue`: preset utili per ispezionare archivio completo, monumenti calibrati o epilogo.
- `--smoke-frames N` + `--screenshot FILE`: chiude dopo `N` frame salvando uno screenshot (supporta percorsi assoluti e relativi).
- `--headless-e2e`: esegue la suite end-to-end deterministica senza finestra grafica (include la validazione completa delle 6 zone separate, metriche di memoria residenti e swap intramezzi).

---

## Controlli di Gioco

| Tasto / Input | Azione |
|---|---|
| `WASD` / `Frecce` / Stick Sinistro | Movimento (Top-Down nelle 6 zone, Side-Scrolling nelle 3 Strutture) |
| `SPAZIO` / `W` / `SU` | Salto nelle Strutture interne / **Salta Intramezzo** durante la transizione tra zone |
| `F` / `Q` / `.` | **Lente Empirica** (misura il parametro fisico del monumento o dell'elemento piu' vicino) |
| `E` / `INVIO` | **Interagisci** con monumenti, personaggi (Clara, Lorenzo, Miriam, Matteo), leve, specchi, valvole, reliquie o conferma menu |
| `A`/`D` / `Frecce` / Mouse (nel modal) | Regola il parametro fisico del monumento |
| `C` / `INVIO` (nel modal) | **Conferma Calibrazione Pulita** |
| `B` (nel modal) | **Fornace a Pece di Emergenza** (energia immediata ma +Foschia; disabilitata sull'Astrolabio) |
| `X` / `BACKSPACE` | Chiude modal / Salta il diario introduttivo |
| `H` | **Diario di Campo — Cronologia Dialoghi** (scorribile con `W/S`, Frecce, rotellina; filtra capitoli con `A/D`) |
| `J` | **Diario di Campo — Codex Reliquie** (16 reliquie divise per 4 Ere storiche dei Meridiani) |
| `V` | Cambia **Fase Percettiva** (Fase 1 Sguardo Empirico -> Fase 2 Rilievo Metrico -> Fase 3 Trama Geodetica) |
| `N` | **Avanza Dialogo** / Completa battuta |
| `TAB` | Cambio rapido Overworld <-> Struttura attiva (o cambia tab dentro il Diario e nelle Impostazioni) |
| `ESC` | Chiude Calibrazione -> Chiude Diario -> Apre/Chiude **Menu di Pausa & Impostazioni** |
| `O` (dal Titolo) | Apre direttamente le **Impostazioni (Gioco, Grafica, Ambiente, Audio)** |
| `M` | Attiva/Disattiva rapidamente **Audio (Muto)** preservando i livelli di volume |
| `F11` / `Alt+INVIO` | Attiva/Disattiva **Schermo Intero** sul **Monitor selezionato** |

---

## Funzionalita' Principali e Novita'

### 1. Progressione Sequenziale, Varchi Vincolati & Architettura a Zone Separate
- **6 Mappe Indipendenti (`26x36` tile) in Catena Sequenziale (`1 <-> 2 <-> 3 <-> 4 <-> 5 <-> 6`)**:
  1. *Terrazze dell'Alba* (`N -> Canali`, sbloccato dopo l'attivazione dell'**Eliostato Solare**; muro orientale del `SolarGate` sigillato da bordo a bordo).
  2. *Quartiere dei Canali* (`E -> Venti` sul Ponte degli Archi, sbloccato dopo la **Turbina Idrostatica** + reclutamento di **Clara Sereni**).
  3. *Altopiano dei Venti* (`E -> Giardini`, sbloccato dopo la calibrazione del **Pendolo Isochronos**).
  4. *Giardini Sospesi* (`N -> Caldera`, sbloccato dopo l'accordatura del **Risonatore Armonico**).
  5. *Caldera delle Fumarole* (`E -> Santuario`, sbloccato dopo la calibrazione dello **Scambiatore di Carnot**).
  6. *Santuario dell'Armonia* (piazza cerimoniale dell'**Astrolabio Centrale** e dei 5 sigilli elementali).
- **Varchi Sigillati & Ritorno Libero**: ogni varco verso la zona successiva mostra barre energetiche sigillate, targa dedicata e suggerimento contestuale finche' la task principale corrente non viene completata; i varchi all'indietro restano sempre aperti per l'esplorazione.
- **Ottimizzazione Memoria Reale**:
  - Viene mantenuta residente in RAM **solo la mappa della zona attualmente attiva** (`936 B` di griglia per zona, oltre `7.1 KB` risparmiati rispetto al caricamento simultaneo di zone e strutture).
  - Al varco tra zone, la vecchia tilemap e le cache procedurali (affreschi e prop) vengono liberate con deallocazione esplicita (`ReleaseOverworldProceduralCache()`), rigenerando on-demand solo gli asset della nuova destinazione.
  - Minimappa filtrata sulla sola zona attiva con stato live dei varchi e bussola guidata.

### 2. Livelli dei Portali Ridisegnati come Test di Logica Side-Scrolling
Ogni struttura interna (`src/core/structure_puzzles.h/.cpp`) e' un vero enigma di logica e platforming, calibrato sulla fisica reale di salto (`kJumpForce = 480`, `kGravityAccel = 1200`) e verificato da playthrough automatici a 60 Hz in `tests/test_main.cpp`:
1. **Cisterne Ipogee (`20x40`) — *Equilibrio delle Paratoie***:
   - 3 leve (`A`, `B`, `C`) governano una logica booleana senza softlock: la Paratoia Centrale si apre con `A || C`, il Ponte Acquedotto si estende con `A && B`, mentre la Chiusa della Turbina (con **Lorenzo Altieri** e la **Turbina Idrostatica**) si apre solo con `!A && B && C`.
2. **Torre del Pendolo (`22x38`) — *Sequenza Isocrona***:
   - 4 campane armoniche (`I`, `II`, `III`, `IV`) su livelli ascendenti da percuotere in ordine strettamente crescente (un ordine errato resetta la sequenza), estendendo 2 ponti armonici fino al Portale Isocrono del **Pendolo**.
3. **Camera Geotermica (`20x42`) — *Ciclo di Carnot***:
   - 4 valvole termiche accoppiate (`V1..V4`): `V1 && !V2` estende il Ponte di Basalto verso `V3`, `V2` apre il condotto verso `V4`, `V3 && V4` estende la Passerella Termica e tutte e 4 attive aprono la Camera di Carnot dello **Scambiatore**.
- **Salvataggio Persistente delle Leve & Anti-Softlock**: lo stato di leve, sequenze, paratoie e ponti di tutte e 3 le strutture (oltre alle piastre solari e risonanze in Overworld) viene salvato automaticamente in `saves/conca_di_meridia.sav` e ripristinato prima del posizionamento di Elena, impedendo di rimanere bloccati alla riapertura del gioco.

### 3. Meccaniche Top-Down Uniche per Zona
Ogni livello 2D possiede ora un gameplay e una fisica propria:
1. **Terrazze dell'Alba — Emettitore & Specchi Solari**:
   - Fascio luminoso orientabile ruotando 3 specchi a prisma con `[E]`.
   - Caricamento delle piastre fotosensibili all'incidenza del raggio per disattivare la barriera energetica (*SolarGate*) e sbloccare passaggi e reliquie.
2. **Quartiere dei Canali — Correnti Idrauliche & Chiuse**:
   - I canali bassi (`ShallowChannel`) esercitano una reale forza idrodinamica sul giocatore.
   - Leve di manovra (`[E]`) deviano il flusso e abbassano le chiuse per rendere sicuro il transito.
3. **Altopiano dei Venti — Raffiche Dinamiche & Monoliti Frangivento**:
   - Cicli periodici di vento con telegraphing visivo e sonoro che spingono fisicamente Elena.
   - 5 grandi monoliti in basalto creano aree d'ombra aerodinamica (*shelter zone*) dietro cui ripararsi durante la raffica.
4. **Giardini Sospesi — Nodi di Risonanza Acustica**:
   - 4 piastre armoniche intonate (accordo di Do Maggiore: C4, E4, G4, C5) che emettono onde sonore e particelle intonate.
   - Calpestare la sequenza corretta materializza gradualmente il *Ponte di Giada* sospeso sulle acque.
5. **Caldera delle Fumarole — Geyser Termici & Valvole di Sfogo**:
   - 6 crateri geotermici ciclici a 4 fasi (*Dormant*, *Telegraphing*, *Erupting*, *Calmed*) che respingono in sicurezza chi si avvicina durante l'eruzione.
   - 2 valvole di sfogo azionabili per canalizzare il vapore e raffreddare stabilmente i getti.
6. **Santuario dell'Armonia — Sigilli Elementali dell'Astrolabio**:
   - 5 sigilli radiali corrispondenti ai 5 distretti periferici, che si accordano e illuminano all'attivazione delle rispettive macchine.

### 4. Intramezzi di Soglia Narrativi e Visivi
- Attraversando uno dei 5 varchi monumentali tra zone si attiva una sequenza di intramezzo procedurale con dissolvenza, parallasse dello sfondo e polvere atmosferica.
- Ogni passaggio propone un testo poetico e storico scritto per la specifica direzione del viaggio (`ZoneInterludeCopy()`).
- Possibilita' di saltare istantaneamente la transizione premendo `SPAZIO`, `INVIO`, `E` o `ESC` (istantanea in test automatizzati `--headless-e2e`).

### 5. Menu di Pausa, Regolazioni Ambientali e Audio
- **Layout a 4 Schede** navigabile sia da tastiera (`[Q/E]`, `[TAB]`, `[1-4]`, frecce) sia interamente col mouse:
  1. *Gioco*: velocita' dialoghi, tutorial compatto, mostra FPS.
  2. *Grafica*: modalita' finestra/fullscreen, selettore monitor dedicato, risoluzione sicura, FPS target, selettore GPU fisica con riavvio automatico.
  3. *Ambiente*: toggle generale atmosfera, densita' particelle (0..100%), intensita' foschia/nebbia, raggi solari volumetrici, intensita' vignetta cinematografica, intensita' effetti meteo di zona (raffiche, vapore, pollini).
  4. *Audio*: toggle mute rapido, volume principale, volume musica procedurale, volume effetti/interazioni, volume atmosfera/natura, toggle ambiente, toggle suoni interfaccia, con campionamento sonoro in tempo reale durante la regolazione.
- Salvataggio persistente e automatico in `saves/settings.cfg`.

### 6. Collezionabili, Reliquie Nascoste & Codex Storico
- 16 reliquie archeologiche divise in 4 Ere storiche dei Meridiani (10 in Overworld + 6 nelle 3 Strutture), riposizionate in punti coerenti con la storia e l'architettura.
- **7 Reliquie Nascoste** in nicchie e passaggi segreti (dietro archi d'acquedotto, sottovento ai monoliti, dietro le fornaci in rovina, sull'ala alta delle Cisterne, sulla guglia della Torre e contro la parete d'ossidiana): invisibili sulla minimappa standard, brillano discretamente finche' Elena non si avvicina (o non attiva la Fase 3 *Trama Geodetica*) e restano sempre 100% raggiungibili.
- Consultabili in qualsiasi momento con il tasto `[J]` (Codex Reliquie) con analisi scientifiche, estratti di diario e contributi narrativi degli NPC.

---

## Documentazione e Diario delle Modifiche (`docs/`)

- [`docs/VISION.md`](docs/VISION.md) — Visione creativa, pilastri di design e progressione percettiva.
- [`docs/STORIA.md`](docs/STORIA.md) — Bibbia narrativa de *La Conca di Meridia*: 4 Ere, cast, geografia, 16 reliquie e 3 finali.
- [`docs/DIALOGHI.md`](docs/DIALOGHI.md) — Copione integrale dei 7 capitoli, 32 eventi, 16 schede Codex ed epiloghi.
- [`docs/ART_DIRECTION.md`](docs/ART_DIRECTION.md) — Direzione artistica multi-bioma, architetture 2.5D, monumenti, reliquie e stile UI.
- [`docs/DISCUSSIONE.md`](docs/DISCUSSIONE.md) — Verbale della discussione collegiale a 6 agenti (2 narratori, 2 artisti, 2 ingegneri).
- [`docs/COLLABORATORI.md`](docs/COLLABORATORI.md) — Squadra a 6 agenti, proprieta' dei file e regole di verifica.
- [`docs/ROADMAP.md`](docs/ROADMAP.md) — Milestone completate e prossimi orizzonti.
- [`docs/DEVLOG.md`](docs/DEVLOG.md) — Registro cronologico degli sviluppi.
- **Log puntuali delle modifiche in `docs/modifiche/`**:
  - [`00_piano_e_avvio.md`](docs/modifiche/00_piano_e_avvio.md)
  - [`01_mondo_e_civilta.md`](docs/modifiche/01_mondo_e_civilta.md)
  - [`02_dialoghi_e_reliquie.md`](docs/modifiche/02_dialoghi_e_reliquie.md)
  - [`03_motore_core_e_zone.md`](docs/modifiche/03_motore_core_e_zone.md)
  - [`04_ui_cronologia_e_test.md`](docs/modifiche/04_ui_cronologia_e_test.md)
  - [`04b_fix_separatore_cronologia.md`](docs/modifiche/04b_fix_separatore_cronologia.md)
  - [`05_ambientazioni_e_dettagli.md`](docs/modifiche/05_ambientazioni_e_dettagli.md)
  - [`06_personaggi_collezionabili_vfx.md`](docs/modifiche/06_personaggi_collezionabili_vfx.md)
  - [`07_integrazione_finale_e_verifica.md`](docs/modifiche/07_integrazione_finale_e_verifica.md)
  - [`08_audio_procedurale_e_interazioni.md`](docs/modifiche/08_audio_procedurale_e_interazioni.md)
  - [`09_monitor_gpu_e_salvataggio.md`](docs/modifiche/09_monitor_gpu_e_salvataggio.md)
  - [`10_divisione_zone_memoria_e_guida.md`](docs/modifiche/10_divisione_zone_memoria_e_guida.md)
  - [`11_stacchi_visivi_portali_e_bussola.md`](docs/modifiche/11_stacchi_visivi_portali_e_bussola.md)
