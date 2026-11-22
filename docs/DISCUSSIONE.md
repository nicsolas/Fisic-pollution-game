# Verbale della Discussione Collegiale — Rifondazione "La Conca di Meridia" (6 Agenti)

**Data**: 2026-11-22  
**Partecipanti**: `narratore-1`, `narratore-2`, `artista-1`, `artista-2`, `ingegnere-1`, `ingegnere-2`, `coordinatore`.

---

## 1. Critica alla Versione Precedente (Demo v0.3)
Prima di scrivere una sola riga di codice, tutti e 6 gli agenti hanno esaminato il progetto e condiviso i limiti principali:
- **Ambientazione e tono**: i riferimenti diretti alla Calabria (`Valle del Crati`, `Unical`, `Ponte Bucci`) e il gergo da script/debug (`// GAIA [SYS]`, `AABB`, `Ciclo 404`, `Consorzio Brace`, `Bruno Ceneri`, `Nonna Rosa`) rompevano l'immersione e suonavano artificiali.
- **Scala del mondo**: l'Overworld (`18x28`) e l'unica struttura interna (`15x28`) offrivano poche zone e una varieta' visiva limitata.
- **Longevita' e memoria narrativa**: mancavano una cronologia consultabile dei dialoghi, un sistema di collezionabili che ricostruisse la storia della civilta', e una vera campagna estesa oltre la demo.

---

## 2. Decisioni Condivise (Consenso 6/6)

### Mondo e Civilta' (`narratore-1`, `narratore-2`)
- **La Conca di Meridia**: altopiano monumentale scolpito dai **Meridiani** in pietra calcarea, travertino, basalto, bronzo e giada, progettato per funzionare con le sole leggi fisiche.
- **4 Ere Storiche** da ricostruire tramite **16 Collezionabili** (`I.1`..`IV.4`):
  1. *Era I — Alba delle Ombre*
  2. *Era II — Secolo delle Acque e del Suono*
  3. *Era III — Inverno di Pece*
  4. *Era IV — Sigillo dell'Armonia*
- **Cast naturale e letterario**: **Elena Valli** (protagonista), **Clara Sereni** (ingegnera idraulica), **Lorenzo Altieri** (archivista nelle Cisterne alte), **Miriam Ferro** (botanica nei Giardini Sospesi), **Matteo Verrini** (Sovrintendente pragmatico della Compagnia della Pece, promosso a **4o NPC fisico** nella Caldera su proposta congiunta di `narratore-2`, `narratore-1` e `artista-2`), e **Eco di Meridia** (epigrafi armoniche nei cristalli, zero sintassi da log).
- **Campagna in 7 Capitoli e 3 Finali**: *L'Alba di Meridia* (pulito + `>=12/16` reliquie), *La Rinascita della Conca* (pulito + `<12` reliquie), *Il Prezzo della Fretta* (almeno una fornace a pece).

### Architettura Multi-Zona e Fisica (`ingegnere-1`, `ingegnere-2`)
- **Overworld espanso `36x50` tile** (`1728x2400 px`, quasi 4x la mappa precedente) suddiviso in **6 Distretti** (`ZoneId`): *Terrazze dell'Alba*, *Quartiere dei Canali*, *Altopiano dei Venti*, *Giardini Sospesi*, *Caldera delle Fumarole*, *Santuario dell'Armonia*, con 10 `TileId` semantici (`Water`, `PavedPath`, `SanctuaryFloor`, `RuinsWall`, `GardenFloor`, `HiddenBridge`, ecc.).
- **3 Strutture Interne side-scrolling** (`StructureId`) collegate da 3 portali dedicati (`PortalLink`): *Cisterne Ipogee* (`16x34`), *Torre del Pendolo* (`18x30`), *Camera Geotermica* (`16x34`).
- **6 Monumenti Fisici Autentici**: Eliostato/Lambert (`45 deg`), Turbina/Stevino (`10 m`), Pendolo Isochronos/Galileo (`L=2.0 m`, `T=2.84 s`), Risonatore Armonico Lorentziano (`f0=120 Hz`, `Q=3`), Scambiatore di Carnot (`Th=600 K`, `eta=50%`), Astrolabio Centrale (`72 deg`, senza bypass).
- **Cronologia Dialoghi `[H]` + Codex Reliquie `[J]`**: `StoryDirector::History()` salva ogni battuta con capitolo, zona e parlante; `story_ui.cpp` la rende in un Diario a due tab con word-wrap automatico, scroll, filtro capitolo/era e priorita' pulita del tasto `[ESC]`.

### Direzione Artistica, Dettagli e Contrasto (`artista-1`, `artista-2`)
- **Viewport culling** sulla mappa `36x50` per garantire 60 FPS fluidi con ricchissimi dettagli 2.5D: mosaici astronomici world-space, lastricati auto-tiled, canali/bacini continui senza linee di griglia, Ponte degli Archi in travertino/bronzo con sigillo ambrato (zero laser), colonne scanalate, statue, flora a 4 specie reattiva allo smog e 3 fondali in parallasse dedicati per le strutture.
- **Gerarchia visiva concordata**: terreni/architetture su valori controllati (`palette.h`), personaggi/monumenti/reliquie (5 forme: `Stele`, `Astrolabe`, `Prism`, `Cylinder`, `FossilSeed`) ad alto contrasto con aureole e particelle per bioma, **Minimappa Overworld (`MAPPA DI MERIDIA`)** sotto lo scanner senza sovrapposizioni HUD, e 6 diagrammi fisici live nel modal di calibrazione.

---

## 3. Esito della Verifica Finale
- Compilazione C++20 (`-Wall -Wextra -Wpedantic`): **0 errori, 0 warning**.
- Suite test `ctest --output-on-failure`: **2/2 PASS** (`eco_tests` + `headless_e2e`, inclusi audit ASCII, BFS raggiungibilita' di tutte le zone/reliquie e audit zero residui legacy).
- Verifica visiva completa su 12+ screenshot ispezionati (con micro-fix al separatore verticale della Cronologia Dialoghi in `04b_fix_separatore_cronologia.md` e polish acqua/vapore/finestre in `05_ambientazioni_e_dettagli.md`).
