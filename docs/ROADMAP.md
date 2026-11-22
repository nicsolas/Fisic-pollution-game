# Roadmap — EQUILIBRIO: La Conca di Meridia

## Milestone Completate
- [x] **v0.1.0 — Prototipo Fondamentale**: fisica 2D top-down e side-scrolling, primi 3 impianti fisici, primi dialoghi e grafica procedurale.
- [x] **v0.2.0 — Leggibilita' e UX Base (Demo congelata in `demo/`)**: font TTF Noto Sans, schermo intero borderless `F11`, menu di pausa `ESC` e impostazioni grafiche.
- [x] **v0.3.x — Espansione Iniziale**: selettore modalita', fix risoluzioni e piattaforme.
- [x] **v0.4.0 — Oltre la Demo: La Conca di Meridia (Squadra a 6 Agenti)**:
  - [x] Rifondazione completa del mondo e della civilta' dei **Meridiani** (rimozione totale ambientazione calabrese e terminologia da script).
  - [x] Overworld espanso **`36x50`** diviso in **6 Distretti** con identita' architettonica, mosaici, acquedotti, flora dinamica e minimappa `MAPPA DI MERIDIA`.
  - [x] **3 Strutture Interne side-scrolling** dedicate (*Cisterne Ipogee*, *Torre del Pendolo*, *Camera Geotermica*) con parallasse e 3 portali distinti.
  - [x] **6 Monumenti Fisici Autentici** (Lambert, Stevino, Pendolo di Galileo, Risonanza Lorentziana, Carnot, Interferenza di Fase dell'Astrolabio) con diagrammi live nel modal di calibrazione.
  - [x] **4 NPC fisici** nel mondo (Clara Sereni, Lorenzo Altieri, Miriam Ferro, Matteo Verrini) + **Eco di Meridia**.
  - [x] **16 Collezionabili** in 4 Ere e 5 forme visive con animazioni, VFX di raccolta e **Codex Reliquie `[J]`**.
  - [x] **Cronologia Dialoghi `[H]`** completa, scorribile e filtrabile per capitolo, integrata nel Diario di Campo di Elena Valli.
  - [x] **Campagna in 7 Capitoli e 3 Finali** (*L'Alba di Meridia*, *La Rinascita della Conca*, *Il Prezzo della Fretta*).
  - [x] Suite test completa (`eco_tests` + `headless_e2e`) e log `.md` puntuali in `docs/modifiche/`.
- [x] **v0.4.1 — Audio Procedurale & Suoni di Interazione**:
  - [x] Sintetizzatore PCM procedurale (`src/app/audio/audio_engine.*`) su Raylib/miniaudio con 18 effetti d'interazione (passi su pietra/erba, salto/atterraggio, Lente Empirica, reliquie, calibrazione pulita/fornace, Ponte degli Archi, portali, cambio fase, UI — zero voci sui dialoghi) e 5 paesaggi sonori continui (Overworld pulito/foschia, Cisterne, Torre del Pendolo, Camera Geotermica).
  - [x] Controlli Volume Audio (`0%..100%`), modalita' `Ambiente + Interazioni` / `Solo Interazioni`, tasto rapido `[M]` e flag `--mute`.
- [x] **v0.4.2 — Selezione Monitor, Selezione GPU & Salvataggio Partita**:
  - [x] Selettore `Monitor Schermo Intero` con `SetWindowMonitor` (fix schermo intero sul monitor corretto su Wayland/X11) e persistenza in `saves/settings.cfg`.
  - [x] Selettore `GPU / Scheda Grafica` (rilevazione DRM NVIDIA/Intel/AMD, `GPU Attiva Ora`, `Applica GPU & Riavvia`, `--gpu`).
  - [x] Salvataggio/Caricamento completo (`src/core/save_data.*`, autosave + salvataggio manuale) e menu **"Riprendi da dove hai lasciato"** con 9 sfondi procedurali ridisegnati per zona/struttura.
- [x] **v0.4.3 — Divisione Netta Zone, Ottimizzazione Memoria & Guida Giocatore (`ingegnere-zone` + `artista-zone`)**:
  - [x] Muri/scarpate di confine spessi 2 tile tra i 6 distretti con **5 `DistrictGateway` monumentali** e stele direzionali (`N -> CANALI`, `E -> GIARDINI`, `S -> SANTUARIO`, ecc.).
  - [x] Ottimizzazione memoria: `Tilemap` piatto contiguo (`std::vector<std::uint8_t>`, 1 sola allocazione heap, 1.800 B), prop Overworld pre-cachati una sola volta, zero allocazioni per-frame (`ForEachMachineInScene`, `ForEachNpcInScene`, viewport culling).
  - [x] Guida giocatore: `WorldContext::CurrentObjectiveTarget()`, bussola diegetica bronzo/giada con distanza in metri e faro pulsante + distretto attivo evidenziato su `MAPPA DI MERIDIA`.

## Prossimi Orizzonti (v0.5+)
- [ ] **Nuove Sfide Fisiche Opzionali**: puzzle secondari legati all'ottica a prismi, ottiche acustiche nei Giardini e modalita' Co-op 2-4 giocatori con ruoli asimmetrici.
- [ ] **Espansione della Valle dei Semi (Post-Epilogo)**: area sbloccabile oltre il Santuario dopo aver raggiunto il finale *L'Alba di Meridia*.
