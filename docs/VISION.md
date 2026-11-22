# VISION -- EQUILIBRIO: Le Cronache di Meridia

## Logline
Sull'antico altopiano monumentale della **Conca di Meridia**, una civilta' perduta di astronomi, idraulici e filosofi naturali costrui' un ecosistema governato unicamente da leggi fisiche, senza combustibili. Secoli dopo la frattura provocata dalle fornaci a pece, la naturalista e cartografa **Elena Valli** risale la Conca con il proprio **Diario di Campo** e una **Lente Empirica**: misurando la luce, la pressione dell'acqua, il periodo del pendolo, la risonanza armonica e il ciclo di Carnot, riaccende i cinque monumenti della Conca, raccoglie sedici reliquie delle quattro ere perdute e sceglie tra la pazienza della scienza e la scorciatoia del combustibile.

## Pilastri di Design
1. **La matematica e' il mestiere della cura**: si osserva con la Lente Empirica (`[F]`), si legge la legge fisica reale, si calibra lo strumento (`[E]`).
2. **Pulito costa pazienza, sporco costa alla terra**: ogni impianto offre il compromesso immediato della Fornace di Bypass (`[B]`), ma il mondo ne porta subito le cicatrici visive ed etiche.
3. **Un mondo vasto che guarisce visibilmente**: 6 distretti distinti nell'Overworld (`36x50` tile) e 3 strutture interne side-scrolling con gravita', ciascuno con architettura, flora, luci e palette dedicate che si trasformano dal grigio-cenere alla pietra viva, al turchese e al verde giada.
4. **Percezione stratificata e memoria viva**: la Lente evolve in 3 fasi diegetiche (*Sguardo Empirico* -> *Rilievo Metrico* -> *Trama Geodetica*), mentre il **Diario di Campo** permette di rileggere in qualsiasi momento l'intera **Cronologia dei Dialoghi (`[H]`)** e il **Codex delle 16 Reliquie (`[J]`)**.
5. **Campagna Completa come cuore dell'esperienza**: superata la vecchia struttura da breve dimostrazione, il gioco offre di default la campagna integrale in 7 capitoli, 5 leggi fisiche autentiche + Astrolabio Centrale, 4 personaggi incontrabili nel mondo e 3 finali modellati sia dalle scelte ecologiche sia dalla ricostruzione storica.

## Architettura del Mondo
- **Overworld (`36x50` tile, 6 distretti)**: *Terrazze dell'Alba* (SO), *Quartiere dei Canali & Ponte degli Archi* (Centro-Ovest), *Altopiano dei Venti* (NO), *Giardini Sospesi* (Centro-Est), *Caldera delle Fumarole* (NE), *Santuario dell'Armonia* (SE).
- **3 Strutture Interne Side-Scrolling**: *Cisterne Ipogee* (Stevino + Lorenzo Altieri), *Torre del Pendolo* (Galileo), *Camera Geotermica* (Carnot).
- **Cast**: Elena Valli (protagonista), Clara Sereni (ingegnera dei canali), Lorenzo Altieri (archivista), Miriam Ferro (botanica), Matteo Verrini (Sovrintendente della Compagnia della Pece) e l'**Eco di Meridia** (memoria epigrafica nei cristalli armonici).
- **Collezionabili**: 16 reliquie divise in 4 Ere Storiche (*Alba delle Ombre*, *Secolo delle Acque e del Suono*, *Inverno di Pece*, *Sigillo dell'Armonia*) e 5 forme materiali (`Stele`, `Astrolabe`, `Prism`, `Cylinder`, `FossilSeed`).

## Direzione Tecnica e Vincoli
- **Core puro C++20 (`src/core/`)**: simulazione fisica, tilemap multi-distretto, strutture, collezionabili e direzione narrativa totalmente indipendenti dalla grafica (`raylib.h` confinato in `src/app/`).
- **Grafica procedurale 2.5D**: estetica antico-scientifica in pietra calcarea, basalto, bronzo ossidato, vetro-giada e acqua riflettente, con minimappa diegetica, particelle per bioma e diagrammi fisici dal vivo nei pannelli di calibrazione.
- **Testi ASCII-safe (`32..126`)**: compatibilita' garantita su tutti i font con word-wrap automatico in Dialogue Box, Cronologia `[H]` e Codex `[J]`.
- **Co-op locale**: predisposto per 1-4 esploratori sullo stesso schermo.

## Fuori Ambito Attuale
- Multiplayer in rete.
- Asset sprite esterni pre-renderizzati (tutta la resa resta procedurale in Raylib).
- Motore audio completo.
