# 05 - Ambientazioni, Biomi, Architettura 2.5D e Dettagli Procedurali ("La Conca di Meridia")

- **Autore**: `@artista-1` (World Environments, Biomes & Architecture Artist)
- **File modificati / creati**:
  - `src/app/render/palette.h`
  - `src/app/render/procedural_art.h`
  - `src/app/render/procedural_art.cpp`
  - `docs/ART_DIRECTION.md`
  - `docs/modifiche/05_ambientazioni_e_dettagli.md`

---

## 1. Obiettivo e Direzione Artistica
Rifondazione completa dell'estetica ambientale del gioco attorno alla nuova civilta' perduta dei **Meridiani** (**La Conca di Meridia**), rimuovendo integralmente ogni riferimento alla Calabria (Crati, Unical, Ponte Bucci) e ogni stile "script/debug", e portando tutte le 6 zone dell'Overworld espanso (`36x50`) e le 3 strutture interne side-scrolling a una ricchezza architettonica, naturalistica e atmosferica di alto livello.

---

## 2. Palette Multi-Distretto e Multi-Struttura (`src/app/render/palette.h`)
- Introdotti i token materici condivisi della civilta' dei Meridiani: travertino/pietra calcarea (`kStoneIvory`, `kTravertineLight`, `kTravertineShadow`), basalto (`kBasaltDark`, `kBasaltMid`), bronzo e rame ossidato (`kBronze`, `kBronzeBright`, `kBronzeDark`, `kCopperOxidized`), vetro-giada (`kJade`, `kJadeGlow`), ambra (`kAmberGlow`) e pergamena (`kParchmentText`).
- Introdotte le struct `DistrictPalette` + `GetDistrictPalette(eco::ZoneId)` per tutti i 6 distretti dell'Overworld (transizione continua `Inverno di Pece` -> `Alba di Meridia` guidata da `EcoSystem::PollutionRatio()`):
  1. `DawnTerraces` (*Terrazze dell'Alba*, SO)
  2. `CanalQuarter` (*Quartiere dei Canali & Ponte degli Archi*, Centro-Ovest)
  3. `WindPlateau` (*Altopiano dei Venti*, NO)
  4. `HangingGardens` (*Giardini Sospesi*, Centro-Est)
  5. `FumaroleCaldera` (*Caldera delle Fumarole*, NE)
  6. `HarmonySanctuary` (*Santuario dell'Armonia*, SE)
- Introdotte le struct `StructurePalette` + `GetStructurePalette(eco::StructureId)` per le 3 strutture interne (`HypogeanCisterns`, `PendulumTower`, `GeothermalChamber`).
- Gerarchia di contrasto calibrata per far spiccare personaggi, monumenti fisici, VFX e reliquie luminose di `@artista-2` sopra terreni e architetture leggibili e armoniche.

---

## 3. Arte Procedurale Overworld e Strutture (`src/app/render/procedural_art.h/.cpp`)
- **Nuove firme pulite con frustum/viewport culling** (`DrawOverworldProceduralMap(world)`, `DrawOverworldProceduralProps(world)`, `DrawStructureProceduralBackdropAndMap(world, camera_target)`): rendering fluido a 60 FPS sull'intera mappa `36x50`.
- **Pavimentazioni e Terreni Semantici (`TileId` 0..9)**:
  - `Walkable`: velature organiche opache (zero griglia a scacchiera), ciuffi d'erba multipli (inclinati dal vento nell'*Altopiano dei Venti*), ciottoli scolpiti, fioriture spontanee al calare dello smog, crepe vulcaniche e vene geotermiche nella *Caldera delle Fumarole*.
  - `PavedPath`: lastricati a conci sfalsati (*running bond*) auto-tiled con cordoli esposti, luci NW, ombre SE e muschio tra le fughe.
  - `SanctuaryFloor`: lastre monumentali intarsiate in bronzo e giada, piu' **4 grandi mosaici world-space** (Meridiana Solare all'Eliostato, Onde Armoniche al Risonatore, Rosa dei Venti davanti alla Torre del Pendolo, e il grande Mosaico Astronomico con 4 medaglioni delle Ere al Santuario dell'Armonia).
  - `GardenFloor`: aiuole terrazzate bordate in pietra con rosette di fogliame organico e fioriture corallo/oro/avorio reattive alla salute ambientale.
  - `Water`: canali e specchi d'acqua continui (zero griglia interna) con sponde scolpite auto-tiled, ombre di banchina, onde e scintillii animati, canneti, ninfee e **arcate d'acquedotto in pietra** che scavalcano periodicamente il canale centrale.
  - `HiddenBridge` (**Ponte degli Archi**): bloccato con piloni spezzati, catene di bronzo e sigillo ambrato pulsante (zero laser sci-fi); sbloccato come viadotto monumentale in travertino, bronzo e guida luminosa di giada con 4 lanterne agli imbocchi.
  - `Wall` e `RuinsWall`: murature 2.5D continue auto-tiled con ombre portate morbide a due strati; rovine stratificate con colonne isolate scanalate (piloni d'acquedotto, pergolati fioriti nei Giardini, colonne con bracieri di giada nel Santuario), fregi in bronzo, crepe variate, edera fiorita e camini di fornaci spente nella Caldera.
  - `StructurePortal`: 3 architetture monumentali continue a doppio arco per *Cisterne Ipogee* (cascata idraulica + chiave di volta), *Torre del Pendolo* (quadrante dorato + pendolo oscillante), *Camera Geotermica* (basalto + rame + onde di calore).
- **Flora Diversificata, Statue, Lanterne e Stele Diegetiche** (`DrawOverworldProceduralProps`):
  - Filtro automatico anti-sovrapposizione rispetto a spawn, macchine, NPC, portali, collezionabili e Ponte degli Archi.
  - 4 specie/biomi (ulivi monumentali, cipressi inclinati dal vento, alberi fioriti dei Giardini Sospesi, fumarole di basalto esagonale con vapore), piu' statue dei Meridiani (Astronomo con sfera armillare e Idraulica con anfora d'acqua), obelischi-gnomone, urne fiorite, colonne spezzate, bio-cristalli geodetici, lanterne di giada e stele scolpite posizionate dinamicamente da `world.overworld.portals`.
- **3 Strutture Interne in Parallasse a Doppio Piano** (`DrawStructureProceduralBackdropAndMap`):
  - *Cisterne Ipogee*: grandi volte ad arco calcaree, contrafforti con fasce in bronzo, cascate interne animate, bacino riflettente sul fondo, passerelle con archi di sostegno in bronzo e lanterne sospese.
  - *Torre del Pendolo*: oculi superiori e finestre ad arco centrali con fasci di luce diagonali dorati, grandi archi goniometrici, contrappesi in ottone oscillanti, ballatoi graduati in ottone.
  - *Camera Geotermica*: colonne di basalto esagonali con vene termali luminose, archi e condotte in rame rivettato, scie di vapore morbide, bagliore termale dal basso e passerelle a traliccio in rame.

---

## 4. Verifica Build, Test e Ispezione Visiva Reale
- **Build & CTest (`build-art1`)**:
  - `cmake -S . -B build-art1 -G Ninja -DFETCHCONTENT_SOURCE_DIR_RAYLIB="$PWD/build/_deps/raylib-src" && cmake --build build-art1 -j4`: **0 errori, 0 warning** (`-std=c++20 -Wall -Wextra -Wpedantic`).
  - `ctest --test-dir build-art1 --output-on-failure`: **2/2 PASS** (`eco_tests`, `headless_e2e`).
- **Audit pulizia**: 0 occorrenze di riferimenti calabresi o termini script/debug in `palette.h`, `procedural_art.h`, `procedural_art.cpp`, `docs/ART_DIRECTION.md`.
- **Passata di polish visivo guidata dagli screenshot**:
  - Rimosse le griglie interne tra tile d'acqua contigue nei bacini (`CanalQuarter`, `HangingGardens`).
  - Eliminate le macchie traslucide su FBO (`RenderTexture2D`) convertendo le velature del terreno e delle aiuole a blending opaco `LerpColor` (alpha 255), ottenendo superfici morbide e vellutate sia in gioco sia negli screenshot esportati.
  - Aggiunte finestre ad arco e quadranti a mezza altezza nella *Torre del Pendolo*, archi termali e vapore morbido nella *Camera Geotermica*, e crepe/fughe variate per ogni blocco di rovine.
