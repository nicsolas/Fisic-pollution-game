# Roadmap — EQUILIBRIO: Le Leggi della Valle

## Ciclo Attuale (Rifondazione & Slice Verticale)
- [x] **Fase 0 — Coordinatore**: documentazione di base (`VISION.md`, `COLLABORATORI.md`, `ROADMAP.md`, `DEVLOG.md`), flag di verifica CLI in `main.cpp` (`--screenshot`, `--scene`, `--pollution`, `--skip-title`), stub compilabili e screenshot di baseline (`00_prima.png`).
- [x] **Fase 1 — Discussione del Team**: revisione critica incrociata sul board tra `narratore`, `ingegnere` e `artista` e decisioni di design verbalizzate in [`docs/DISCUSSIONE.md`](DISCUSSIONE.md).
- [x] **Fase 2 — Implementazione Parallela**:
  - **Narratore**: bibbia narrativa ([`docs/STORIA.md`](STORIA.md)), copione integrale ([`docs/DIALOGHI.md`](DIALOGHI.md)) e contenuti C++ in `src/core/story_content.cpp` (5 capitoli, 11 eventi, 5 personaggi, obiettivi guidati dallo stato, intro e due finali).
  - **Ingegnere**: logica `StoryDirector` in `src/core/story.cpp`, integrazione non-bloccante in `WorldContext`, input `advance_dialogue_pressed` (`[N]`), `TitleState` procedurale animato, `IntroState` a macchina da scrivere, `DrawStoryLayer` in `src/app/story_ui.cpp` e unit test in `tests/test_main.cpp`.
  - **Artista**: direzione artistica ([`docs/ART_DIRECTION.md`](ART_DIRECTION.md)), palette dinamica `pal::WorldColor` (`palette.h`), terreno/rovine 2.5D/Ponte Bucci/Diga in parallasse (`procedural_art.*`), particelle a pool fisso (`particles.*`), fix multiplayer 1-4 giocatori, restyling delle 3 macchine e dell'HUD (`calibration_ui.cpp`).
- [x] **Fase 3 — Integrazione, Verifica & Revisione Incrociata**: build pulita zero warning, `ctest` + 300 test E2E verdi, screenshot finali (`01_titolo.png`..`05_diga.png`), verifica anti-softlock narrativa e anti-overlap UI (**Tag Git `v0.1.0`**).
- [x] **Fase 4 (v0.2.0) — Git Versioning, Font TTF HD, Schermo Intero, Menu di Pausa (`ESC`) e Impostazioni Grafiche**: repository Git locale con tag `v0.1.0` e `v0.2.0`, font `Noto Sans SemiBold` in `assets/fonts/ui_font.ttf`, fullscreen borderless (`F11`) con letterbox 16:9 e mouse scaling automatico, menu di pausa su `ESC` e menu impostazioni grafiche interattivo.
- [x] **Fase 5 (v0.3.0) — Fix Risoluzione Dinamica, Demo Giocabile Congelata (`demo/`) ed Espansione Storia (Atto IV & Atto V)**:
  - Fix del cambio risoluzione 1280x720 -> 1920x1080 (`GetRenderWidth()`/`GetRenderHeight()` + `rlViewport`/`rlOrtho` e auto-borderless su monitor piu' piccoli).
  - Congelata la Demo giocabile (Atti I-III) in `demo/equilibrio_demo_v0.2`, sul branch Git `demo-v0.2` e selezionabile in-game (`--demo` o voce *Modalita' Storia* nel menu Impostazioni).
  - **Atto IV ("L'Archivio Sommerso")**: Nonna Rosa interagibile dentro la Diga del Crati (`rosa_npc` sulla piattaforma alta).
  - **Atto V ("Il Nucleo di GAIA")**: Scambiatore Geotermico di Carnot a Nord-Est (`thermal_machine` a 60 deg) e Terminale Sorgente di GAIA a Sud-Est con la rivelazione finale del 2048.

---

## Prossimi Passi (Sviluppi Futuri per il Team)
- [ ] **Atto II Esteso — Nonna Rosa in carne e ossa**: aggiungere Nonna Rosa come secondo NPC interagibile dentro la Diga del Crati con una sfida di idrodinamica avanzata (portata e teorema di Bernoulli).
- [ ] **Anomalie Quantistiche Sfruttabili**: trasformare gli arrotondamenti fisici di Fase 3 ("Visione della Matrice") in meccaniche di puzzle ambientali per raggiungere aree segrete del campus.
- [ ] **Integrazione Sprite Artista 2D**: sostituire o affiancare le primitive procedurali in `src/app/render/procedural_art.cpp` con le texture/spritesheet disegnate a mano, mantenendo il sistema di illuminazione e particelle.
- [ ] **Font Personalizzato & Audio Dinamico**: caricare un font bitmap/TTF con supporto esteso e aggiungere paesaggio sonoro reattivo allo smog (vento/ronzio industriale -> acqua del Crati e fauna).
