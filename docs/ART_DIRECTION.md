# ART DIRECTION — EQUILIBRIO: La Conca di Meridia

## 1. Identita' Visiva della Civilta' Perduta
**La Conca di Meridia** e' un altopiano monumentale scolpito secoli fa dai **Meridiani** — astronomi, idraulici e filosofi naturali — come ecosistema armonico alimentato esclusivamente da leggi fisiche, senza combustibili.
Tutta la grafica del gioco e' **procedurale in Raylib C++20** (senza asset esterni): la ricchezza estetica nasce da stratificazione geometrica, coerenza materica, luce direzionale e reattivita' ecologica continua.

### Pilastri Estetici
1. **Materiali Monumentali Antico-Scientifici**:
   - **Travertino e pietra calcarea avorio** per terrazze, colonnati, scalinate e piazze solari.
   - **Basalto scuro e ardesia** per l'Altopiano dei Venti e la Caldera delle Fumarole.
   - **Bronzo e ottone ossidato** per meridiani, quadranti, condotte, cornici e rilievi astronomici.
   - **Vetro-giada e acqua cristallina** per i canali, le lanterne armoniche e i cristalli geodetici.
2. **Luce Direzionale Coerente (Nord-Ovest -> Sud-Est)**:
   - Ogni volume architettonico (`Wall`, `RuinsWall`, colonne, stele, alberi) possiede una faccia superiore illuminata, spigoli chiari sul lato nord-ovest, una faccia frontale 2.5D in ombra propria (14 px) e un'ombra portata morbida proiettata verso sud-est.
3. **Transizione Cromatica "Inverno di Pece" <-> "Alba di Meridia"**:
   - Guidata da `EcoSystem::PollutionRatio()` tramite `pal::WorldColor(sick, healthy, poll)` (curva `pow(poll, 0.58)`).
   - Non cambia solo il colore: al calare dell'inquinamento spuntano muschi e rampicanti sulle rovine, si moltiplicano i fiori e i boccioli sugli alberi, le acque diventano trasparenti con riflessi e ninfee, gli intarsi in bronzo e giada del Santuario si illuminano e i fasci di luce zenitale scaldano gli interni.
4. **Gerarchia di Contrasto (Leggibilita' Immediata)**:
   - **Sfondo / Terreni / Acqua**: valore medio-basso e saturazione controllata per non affaticare la vista.
   - **Architettura 2.5D / Rovine / Flora**: rilievo netto, ombre portate morbide e silhouette leggibili.
   - **Primo Piano (Personaggi, Monumenti Fisici, Reliquie Collezionabili, VFX)**: contrasto alto, mantelli chiari, nuclei luminosi oro/giada/ciano e aureole animate.
5. **Zero Stile "Script/Debug"**:
   - Niente etichette tecniche artificiali nel mondo di gioco: solo stele incise diegetiche in pietra e bronzo con nomi naturali dei luoghi.

---

## 2. I 6 Distretti dell'Overworld (Griglia 36x50 tile)

| Distretto | Posizione | Era | Palette Ferita (`poll = 1`) | Palette Guarita (`poll = 0`) | Dettagli Architettonici & Flora |
|---|---|---|---|---|---|
| **Terrazze dell'Alba** (`DawnTerraces`) | Sud-Ovest | Era I | Ocra polverosa, travertino spento | Prato solare caldo, travertino mielato, bronzo dorato | Mosaici a meridiana solare, lastricati a grandi conci, colonne spezzate, ulivi monumentali, fiori gialli e avorio |
| **Quartiere dei Canali & Ponte degli Archi** (`CanalQuarter`) | Centro-Ovest | Era II | Acqua torbida/fangosa, pietra annerita | Acqua turchese-zaffiro riflettente, calcare azzurro | Canali con banchine a gradini, increspature animate, canneti e ninfee; **Ponte degli Archi** in pietra, archi di bronzo e guide di giada (bloccato: archi interrotti + sigillo d'ambra/pece) |
| **Altopiano dei Venti** (`WindPlateau`) | Nord-Ovest | Era II | Ardesia livida, roccia cenere | Prato salvia-azzurro, basalto blu-ardesia luminoso | Pavimenti con rose dei venti incise, colonnati d'altura, stele segnavento, cipressi slanciati inclinati dal vento e felci rupestri |
| **Giardini Sospesi** (`HangingGardens`) | Centro-Est | Era II | Humus secco e rami spogli | Smeraldo-giada rigoglioso, aiuole fiorite | Giardini terrazzati (`GardenFloor`), pergolati in pietra e bronzo, vasche riflettenti, alberi fioriti, siepi, fiori corallo/oro/turchese e bio-cristalli |
| **Caldera delle Fumarole** (`FumaroleCaldera`) | Nord-Est | Era III | Cenere nera, fessure rosso-pece | Basalto antracite pulito, vene di rame e ambra serena | Basalto esagonale, grate termali in rame, resti delle fornaci dell'Inverno di Pece, fessure termali con vapore limpido, flora termofila verde-oliva |
| **Santuario dell'Armonia** (`HarmonySanctuary`) | Sud-Est | Era IV | Marmo velato di cenere, ottone spento | Travertino avorio luminoso, intarsi oro e giada accesi | Grande mosaico astronomico a orbite ellittiche/concentriche, colonnato monumentale con capitelli, specchi d'acqua rituali, lanterne in vetro-giada |

---

## 3. Le 3 Strutture Interne Side-Scrolling

1. **Cisterne Ipogee (`HypogeanCisterns`)**:
   - **Parallasse**: grandi volte e arcate idrauliche in pietra calcarea su due piani di profondita', cascate interne luminose che scendono in un bacino d'acqua riflettente lungo il fondo.
   - **Piattaforme & Pareti**: passerelle in pietra scolpita con archi di rinforzo in bronzo e bordi di giada; pareti a conci sfalsati con muschio umido e lanterne idrauliche.
2. **Torre del Pendolo (`PendulumTower`)**:
   - **Parallasse**: grandi oculi e vetrate astronomiche ad arco con fasci di luce diagonali (che si scaldano in oro al guarire della Conca), quadranti circolari graduati in bronzo incisi sullo sfondo, pilastri slanciati.
   - **Piattaforme & Pareti**: ballatoi in pietra e ottone con balaustre incise, tacche graduate e nicchie per le reliquie.
3. **Camera Geotermica (`GeothermalChamber`)**:
   - **Parallasse**: colonne di basalto esagonali scolpite nella caverna vulcanica, grandi archi termali e condotte in rame ossidato, pozze geotermiche sul fondo con colonne di vapore ascendente (rosso-pece se inquinata -> turchese/ambra serena quando guarita).
   - **Piattaforme & Pareti**: passerelle in ferro/rame rivettato con grate termali e pareti di basalto con vene minerali luminose.

---

## 4. Tilemap Semantica e Allineamento al Pixel
Il motore core espone `TileId` semantici che garantiscono allineamento perfetto tra collisioni e arte procedurale:
- `Walkable`: terreno organico del distretto con variazioni deterministiche `Hash2(c, r, seed)` (ciottoli, fili d'erba animati dal vento, fiori multi-colore a salute alta, crepe termali nella Caldera).
- `PavedPath`: lastricato in conci di pietra del distretto con bordi smussati, fughe morbide e piccoli ciuffi d'erba tra le pietre.
- `SanctuaryFloor`: pavimentazione monumentale intarsiata con motivi geometrici/astronomici in bronzo e giada (meridiane nelle Terrazze, rose dei venti nell'Altopiano, mosaico orbitale nel Santuario).
- `GardenFloor`: aiuole terrazzate con bordure in pietra, fogliame fitto, felci e fioriture stagionali reattive allo smog.
- `Water`: canali e specchi d'acqua con sponde scolpite adiacenti alle tile camminabili, gradini d'approdo, riflessi luminosi, increspature animate, canneti lungo i bordi e ninfee nei Giardini/Canali.
- `HiddenBridge`: **Ponte degli Archi** sopra il canale centrale (col 24); bloccato = piloni spezzati con catene e sigillo ambrato; sbloccato = ponte monumentale ad archi in pietra e bronzo con lampioni e canali di luce giada.
- `Wall`: murature 2.5D perimetrali e terrazzamenti con conci di pietra, spigoli illuminati NW, ombra portata SE e rampicanti/edera.
- `RuinsWall`: rovine antiche 2.5D stratificate — colonne spezzate scanalate, frammenti di architravi, rilievi geometrici in bronzo e muschio fiorito.
- `StructurePortal` & `ExitPortal`: architettura dedicata per ciascuna delle 3 strutture (portale idraulico ad arco per le Cisterne, portale-torre con quadrante per la Torre del Pendolo, portale in basalto e rame per la Camera Geotermica) con stele diegetica posizionata dinamicamente da `world.overworld.portals`.

---

## 5. Decorazioni Procedurali, Flora e Prestazioni
- **Viewport Culling**: `DrawOverworldProceduralMap` e `DrawOverworldProceduralProps` calcolano il rettangolo visibile dalla `camera_target` (con margine di sicurezza per ombre e chiome) disegnando solo le tile e i prop in vista -> 60 FPS stabili sull'intera mappa 36x50.
- **Flora Diversificata**: 4 specie distinte per distretto (ulivi monumentali a chioma larga argentata/verde, cipressi slanciati inclinati dal vento, alberi fioriti dei Giardini Sospesi, alberi/arbusti termofili della Caldera) + bio-cristalli geodetici + colonnine/lanterne + stele diegetiche.
- **Anti-Sovrapposizione Automatica**: tutti i prop decorativi generati deterministicamente verificano distanza minima da macchine, NPC, portali, collezionabili e sentieri principali.

---

## 6. Personaggi, Reliquie e Interfaccia (Coordinamento con @artista-2 e @ingegnere-2)
- **Personaggi**: Elena Valli (mantello da cartografa + accento giocatore), Clara Sereni (ingegnera dei canali), Lorenzo Altieri (archivista), Miriam Ferro (botanica), Matteo Verrini (Sovrintendente della Compagnia della Pece).
- **Reliquie Collezionabili (16)**: 5 forme luminose (`Stele`, `Astrolabe`, `Prism`, `Cylinder`, `FossilSeed`) con colore d'era e particelle orbitali.
- **UI & Diario**: pannelli scuri in pietra/ardesia (`kUiPanelBg`) con filetti in bronzo (`kUiBorderBronze`) e accenti ciano/giada (`kUiBorderCyan`, `kJade`), testo avorio/pergamena (`kParchmentText`).
- **HUD senza sovrapposizioni**: Barra superiore (`y=0..52`), Obiettivo in alto a sinistra (`y=62..130`), Toast Zona/Reliquia in alto al centro (`y=62..108`), Scanner + Minimappa in alto a destra (`y=62..303`), Dialogue Box in basso (`y=598..704`), Diario `[H]`/`[J]` modale centrato.
