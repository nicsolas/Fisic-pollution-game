# 15 - Arte Ecosistema, Segnali Diegetici per Leggere il Mondo, Fauna/Flora Reattiva & Audio Procedurale

## 1. Obiettivo Artistico e Sistemico nella Vertical Slice (20-30 Minuti)
Nella Vertical Slice di **EQUILIBRIO - La Conca di Meridia** (*Terrazze dell'Alba*, *Quartiere dei Canali*, *Cisterne Ipogee*), il giocatore inizia nella **Fase 1: Sguardo Empirico** senza bussola né overlay telemetrici invadenti. Il mondo fisico stesso deve quindi insegnare a essere letto ad occhio nudo e deve restituire con immediatezza visiva e sonora le conseguenze dei tre approcci di calibrazione:
- **`CleanSlow` (Armonia Pulita)**: guarigione ecosistemica profonda, rifioritura argentea degli Ulivi Bianchi, apertura dei sentieri invasi dai rovi di pece, regime laminare trasparente nei canali e ritorno festoso della fauna.
- **`RiskyEfficient` (Sovraccarico Critico)**: massima resa energetica sul filo dell'instabilità, condotti incandescenti con scintille, correnti rapide ad alta pressione, tubazioni piezometriche vibranti con sbuffi di vapore, scorciatoie vetrificate/supercritiche, riverbero termico e micro-vibrazione della camera.
- **`DirtyImmediate` (Bypass a Pece)**: soluzione d'emergenza che annerisce di fuliggine i condotti, intorbida le acque in vortici bruno-pece, fa appassire la vegetazione in rami scheletrici e fa sparire completamente la fauna.

---

## 2. File Modificati (Proprietà Esclusiva `@arte-ecosistema`)
1. [`src/app/render/palette.h`](file:///home/nicholas/Scrivania/progetto%20gioco%20cpp%20fisica-mate%20capisdci/src/app/render/palette.h)
2. [`src/app/render/procedural_art.h`](file:///home/nicholas/Scrivania/progetto%20gioco%20cpp%20fisica-mate%20capisdci/src/app/render/procedural_art.h) & [`src/app/render/procedural_art.cpp`](file:///home/nicholas/Scrivania/progetto%20gioco%20cpp%20fisica-mate%20capisdci/src/app/render/procedural_art.cpp)
3. [`src/app/render/particles.h`](file:///home/nicholas/Scrivania/progetto%20gioco%20cpp%20fisica-mate%20capisdci/src/app/render/particles.h) & [`src/app/render/particles.cpp`](file:///home/nicholas/Scrivania/progetto%20gioco%20cpp%20fisica-mate%20capisdci/src/app/render/particles.cpp)
4. [`src/app/overworld_state.cpp`](file:///home/nicholas/Scrivania/progetto%20gioco%20cpp%20fisica-mate%20capisdci/src/app/overworld_state.cpp)
5. [`src/app/structure_state.cpp`](file:///home/nicholas/Scrivania/progetto%20gioco%20cpp%20fisica-mate%20capisdci/src/app/structure_state.cpp)
6. [`src/app/audio/audio_engine.h`](file:///home/nicholas/Scrivania/progetto%20gioco%20cpp%20fisica-mate%20capisdci/src/app/audio/audio_engine.h) & [`src/app/audio/audio_engine.cpp`](file:///home/nicholas/Scrivania/progetto%20gioco%20cpp%20fisica-mate%20capisdci/src/app/audio/audio_engine.cpp)

---

## 3. Segnali Diegetici per Imparare a Leggere il Mondo

### 3.1 Terrazze dell'Alba (`DawnTerraces`)
- **Meridiana Madre e Quadrante d'Ombra Gnomonica (`DrawSolarSundialMosaic`)**:
  - Incisioni goniometriche in ottone direttamente sul pavimento di travertino attorno all'*Eliostato Solare*, con tacche graduate ogni `15 deg` e `45 deg`.
  - **Ombra proiettata reale dello stilo gnomonico**: un cuneo d'ombra fisico proiettato sul quadrante indica visivamente l'azimut solare ottimale (`target_param = 45 deg`) secondo la Legge del Coseno di Lambert, leggibile a colpo d'occhio anche in Fase 1 senza HUD.
  - Durante o dopo un **Impulso di Prova (`RunTrial`)**, i glifi solari nascosti nel travertino si accendono in ciano-oro (`kAccentTrial`).
- **Condotti d'Ottone Incastonati nel Suolo (`DrawDawnBrassConduits`)**:
  - Collegano fisicamente l'emettitore solare e le 3 piastre fotosensibili alla *Barriera Fotonica (`SolarGate`)*, all'*Eliostato Solare* e al *Varco Nord*:
    - *Inattivo (`None`)*: ottone ossidato verde-bruno (`kConduitOxidized`).
    - *Guarito (`CleanSlow`)*: oro caldo pulsante con nodi armonici (`kConduitCleanGold`).
    - *Sovraccarico (`RiskyEfficient`)*: ambra incandescente ad alta frequenza con scintille elettriche (`kConduitRiskyAmber`).
    - *Bypass (`DirtyImmediate`)*: condotti anneriti di fuliggine e fumo di pece (`kConduitDirtySoot`).

### 3.2 Quartiere dei Canali (`CanalQuarter`)
- **Colonnine Idrometriche Scolpite (`DrawCanalHydrometerPillar`)**:
  - Pilastrini in pietra calcarea e bronzo lungo le sponde dei canali con tacche metriche di portata e un indicatore galleggiante il cui colore riflette in tempo reale il regime idrodinamico.
- **3 Regimi Visivi per le Correnti dei Canali**:
  - *Inquinato / `DirtyImmediate`*: schiuma densa bruno-pece e vortici erratici che ostacolano la traversata.
  - *Sovraccarico (`RiskyEfficient`)*: getti bianchi rapidissimi, frecce tese in ambra-ciano e spruzzi ad alta velocità (`current_speed_mult > 1.28`).
  - *Guarito (`CleanSlow`) o Regime Laminare del Diario Idraulico (`laminar_flow_active`)*: filetti fluidi zaffiro trasparenti (`kLaminarSapphire`), onde regolari e ninfee in fiore lungo i bordi.
- **Il Guado degli Idraulici (`PassageKind::LaminarFord`)**:
  - Quando bloccato da acque torbide e turbolente, i blocchi di pietra affiorano appena sotto il velo d'acqua scura; quando il regime laminare o la guarigione lo aprono, i conci di travertino emergono asciutti e calpestabili con riflessi zaffiro laterali.

### 3.3 Cisterne Ipogee (`HypogeanCisterns`)
- **Tacche Metriche di Stevino (`DrawCisternsBackdrop`)**:
  - Sulle colonne sommerse in parallasse sono dipinte in calce chiara e bronzo (`kStevinoChalk`) le quote idrostatiche `h = 0m`, `h = 5m`, `h = 10m`, `h = 12m`, `h = 15m` con righelli graduati intermedi per leggere la legge $P = P_0 + \rho g h$ direttamente sull'architettura sommersa.
- **Tubazioni Piezometriche Reattive (`DrawStructureProceduralBackdropAndMap`)**:
  - Lungo le colonne delle paratoie corrono tubazioni in bronzo con vetrini piezometrici graduati: in `RiskyEfficient` o durante il colpo d'ariete di `RunTrial` vibrano fisicamente (`sin(sim_time * 44.0f)`) ed emettono sbuffi di vapore in pressione.
- **Muschi Bioluminescenti, Sfioratore Supercritico, Nicchia dell'Archivista e Valvola di Stevino**:
  - Colonie di muschio bioluminescente azzurro-giada (`kBioMossCyan`, `kBioMossJade`) guidano lo sguardo verso la Galleria Alta del Prisma e la soglia di Lorenzo Altieri.
  - Disegno diegetico dello **Sfioratore Supercritico (`HighSpillway`)**, della **Nicchia d'Archivio (`ArchivistNiche`)** e del **Volantino in Ottone della Valvola Ausiliaria di Stevino (`aux_stevino_valve_pos`)**.

---

## 4. Inquinamento Sistemico: Flora a 3 Stati, Passaggi Dinamici, Stazioni & Vasche d'Ulivo

- **3 Stati Biologici della Flora (`ComputeLocalFloraVisual`, zero allocazioni heap per-frame)**:
  - La cache statica dei prop (`g_prop_cache`) resta invariata in memoria; ogni albero (`DrawOliveTree`, `DrawWindCypress`, `DrawFloweringGardenTree`) modula dinamicamente il proprio stato biologico combinando `ZoneEcoStatus` e la distanza dalle `BioPlanter` rigenerate:
    1. **Stato 0 - Inquinato/Critico**: rami scheletrici spogli (`kFloraDeadWood`), foglie secche cadute a terra (`kFloraDryLeaf`) e macchie di pece alla base (`kFloraPitchStain`).
    2. **Stato 1 - In Ripresa**: chioma verde salvia tenue (`kFloraRecoveringSage`) con primi germogli primaverili.
    3. **Stato 2 - Guarito / Ulivo Bianco (`CleanSlow` o nel raggio di una `BioPlanter`)**: chioma rigogliosa d'ulivo bianco-argenteo (`kFloraWhiteOliveSilver`, `kFloraWhiteOliveHi`), fiori dorati/azzurri e polline luminoso ascendente (`DrawHealedPollenMotes`).
- **Passaggi Dinamici (`DrawDynamicPassagesGround` / `DrawDynamicPassagesElevated`)**:
  - `PitchBramble`: masse spinose nero-ambra 2.5D con esalazioni scure quando chiuse; sentiero lastricato di petali d'ulivo bianco e polline dorato (con bonus di velocità `+12%`) quando la guarigione della zona le dissolve.
  - `ScorchedCrest`: barriera rocciosa quando chiusa; fenditura di roccia basaltica vetrificata con braci ambrate e fumo termico quando aperta da `RiskyEfficient`.
  - `ArtisanArchway`: arco con grata in bronzo quando chiuso, lastricato aperto con intarsi di giada quando sbloccato.
- **Stazioni di Osservazione (`DrawObservationNodeProp`) e Vasche Botaniche (`DrawBioPlanterProp`)**:
  - Gnomoni solari ottagonali nelle Terrazze, idrometri a colonna nei Canali e colonnine piezometriche nelle Cisterne.
  - Vasche ottagonali in pietra arida con terra screpolata prima dell'innesto vs giovane **Ulivo Bianco** radioso con anelli d'aura botanica dopo l'innesto del `Seme d'Ulivo Bianco`.

---

## 5. Fauna Sistemica Viva (`DrawAmbientFauna`)
Sincronizzata con `world.zone_mechanics.ambient_critters` e `ZoneEcoStatus::fauna_count`:
- **Terrazze dell'Alba**: rondini chiare (`CritterKind::Swallow`) in volo radente con battito d'ali animato e ombra proiettata sul travertino.
- **Quartiere dei Canali**: carpe argentate e dorate (`CritterKind::SilverCarp`) che guizzano sotto il pelo dell'acqua con piccole increspature concentriche, e libellule turchesi (`CritterKind::Dragonfly`) a doppia ala traslucida lungo le sponde.
- **Cisterne Ipogee**: tritoni/salamandre bioluminescenti (`CritterKind::BioNewt`) con alone ciano-giada e sciami di lucciole di grotta (`CritterKind::CaveFirefly`).
- Quando l'inquinamento è critico o si attiva `DirtyImmediate`, **`fauna_count` scende a 0** e la fauna sparisce completamente, rendendo tangibile il silenzio biologico del bypass sporco.

---

## 6. Atmosfera, Impulsi Sistemici & Audio Procedurale (`particles.cpp`, `audio_engine.cpp`)
- **Burst Visivi in World-Space (`DrawSystemicActionBursts` in `particles.cpp`)**:
  - **Impulso di Prova (`trial_pulse_timer`)**: 3 onde diagnostiche concentriche con tacche goniometriche/manometriche e 18 scintille empiriche radiali.
  - **Fioritura Vasca d'Ulivo Bianco (`planter_bloom_timer`)**: onda botanica argento-giada e 22 foglioline/petali ascendenti in spirale.
  - **Risonanza Remota del Prisma del Congedo (`prism_pulse_timer`)**: raggio cristallino multi-spettrale tra Elena e il nodo bersaglio con 7 nodi esagonali di rifrazione.
- **Atmosfera Screen-Space & Micro-Vibrazione Camera (`overworld_state.cpp`, `structure_state.cpp`)**:
  - Raggi d'alba dorati potenziati in `CleanSlow`, riverbero termico ambrato ai bordi dello schermo + micro-vibrazione smorzata della camera (`<= 1.85 px`) in `RiskyEfficient` e durante `RunTrial`, foschia acida e cenere pesante in `DirtyImmediate`.
- **Sintesi Audio Procedurale (`audio_engine.h`, `audio_engine.cpp`)**:
  - 6 nuovi SFX sintetizzati a zero allocazioni runtime: `TrialPulse`, `CalibrateRisky`, `ObservationInspect`, `BioPlanterBloom`, `PrismRemoteToggle`, `FaunaChirp`.
  - Nuovo layer musicale `MusicLayer::CriticalOverdrive` (pulsazione ambrata in tensione per `RiskyEfficient`) e nuovo layer naturalistico `NatureLayer::LaminarStream` (armoniche d'acqua cristallina e brezza tra le fronde d'ulivo in zona guarita).
