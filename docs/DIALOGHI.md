# DIALOGHI E RELIQUIE -- EQUILIBRIO: Le Cronache di Meridia

> **Copione Completo In-Game (`src/core/story_content.cpp`) e Catalogo Codex (`src/core/collectible_content.cpp`)**
> Tutti i testi destinati al rendering usano **esclusivamente caratteri ASCII stampabili (codici 32..126)** con apostrofi (`e'`, `piu'`, `civilta'`, `perche'`, `cosi'`) per garantire compatibilita' universale con font TTF e bitmap.
> Ogni battuta viene registrata automaticamente nella **Cronologia Dialoghi (`[H]`)** e ogni reliquia viene trascritta nel **Codex dell'Archivio (`[J]`)**.

---

## 1. Cast e Voci Narranti

| Parlante | Ruolo | Registro e Carattere |
|---|---|---|
| **Elena Valli** | Naturalista e Cartografa (Protagonista) | Prosa lucida da taccuino di campo; unisce rigore empirico e meraviglia davanti alla pietra antica. |
| **Clara Sereni** | Ingegnera dei Canali | Asciutta, concreta, priva di retorica; si fida solo dell'energia pulita e apre il Ponte degli Archi. |
| **Lorenzo Altieri** | Archivista delle Cisterne Ipogee | Cadenza misurata da cronista antico; custodisce la memoria delle quattro ere e la chiave geotermica. |
| **Miriam Ferro** | Botanica dei Giardini Sospesi | Sensoriale, attenta a profumi, radici, semi dormienti e risonanze di vetro-giada. |
| **Matteo Verrini** | Sovrintendente della Compagnia della Pece | Pragmatico e stanco, mai caricaturale; difende le fornaci rapide per proteggere i villaggi dal freddo. |
| **Eco di Meridia** | Cristalli Armonici dei Monumenti | Prosa breve, epigrafica e poetica incisa dagli ultimi Custodi dell'Era IV; zero gergo tecnico. |

---

## 2. Titoli dei Capitoli (`ChapterTitle`)

| Enum `Chapter` | Titolo In-Game |
|---|---|
| `Chapter::Prologo` | `"PROLOGO - La Nebbia sull'Altopiano"` |
| `Chapter::Atto1_Luce` | `"ATTO I - La Misura della Luce"` |
| `Chapter::Atto2_Acqua` | `"ATTO II - Il Peso dell'Acqua"` |
| `Chapter::Atto3_Pendolo` | `"ATTO III - Il Tempo del Pendolo"` |
| `Chapter::Atto4_Giardini` | `"ATTO IV - L'Accordo dei Giardini"` |
| `Chapter::Atto5_Fumarole` | `"ATTO V - Le Ceneri della Frattura"` |
| `Chapter::Epilogo` | `"EPILOGO - Il Cuore di Meridia"` |

---

## 3. Testo Introduttivo (`IntroCrawl`)

1. `"LA CONCA DI MERIDIA - CRONACHE DELL'ALTOPIANO"`
2. `"Secoli fa, una civilta' di astronomi, idraulici e botanici scolpi' questo altopiano nella pietra calcarea e nel basalto."`
3. `"I Meridiani non bruciavano carbone ne' pece: alimentavano la loro citta' con la luce del sole, il peso dell'acqua, il pendolo, il suono e il calore profondo."`
4. `"Poi venne la Grande Siccita' e con essa l'Inverno di Pece: le fornaci d'emergenza oscurarono gli specchi e dispersero gli archivi."`
5. `"Oggi i villaggi a valle respirano cenere e dipendono dai fumi della Compagnia della Pece."`
6. `"La naturalista Elena Valli sale sull'altopiano con una Lente Empirica e un Diario di Campo ancora bianco."`
7. `"I cinque grandi monumenti della Conca sono muti, ma le leggi della fisica attendono mani pazienti che sappiano misurarle."`
8. `"Ogni impianto ti porra' una scelta reale: la pazienza del calcolo pulito o la scorciatoia immediata della fornace."`
9. `"Esplora i sei distretti e le tre camere ipogee, raccogli le sedici reliquie delle quattro ere nel Codex [J] e rileggi il viaggio nel Diario [H]."`
10. `"Misura con [F], calibra con [E], e scopri cosa sigillarono gli ultimi custodi al cuore di Meridia..."`

---

## 4. Copione per Evento (`DialogueForEvent`)

### 4.1 Inizio, Luce e Canali (Prologo e Atto I)

- **`GameStarted`**:
  - **Elena Valli**: `"L'aria sulle Terrazze dell'Alba sa di cenere e pietra bagnata. Ecco finalmente la Conca di Meridia."`
  - **Elena Valli**: `"Gli specchi dell'Eliostato sono spenti da anni, ma la mia Lente Empirica [F] puo' ancora misurare la luce."`
  - **Matteo Verrini**: `"Dottoressa Valli, la vedo salire dalle terrazze. Se vuole corrente subito senza perdere la giornata, apra la fornace con [B]."`
  - **Elena Valli**: `"Sono salita fin qui per fermare il fumo che scende sui villaggi, Matteo. Prima misuro l'Eliostato con [F]."`

- **`FirstScan`**:
  - **Elena Valli**: `"Lo gnomone di bronzo non mente: il raggio solare incide a 45 gradi esatti sulle lastre di travertino."`
  - **Elena Valli**: `"E' la legge del coseno di Lambert: il rendimento dipende dall'angolo tra lo specchio e il fascio di luce."`
  - **Elena Valli**: `"Mi avvicino al meccanismo [E], allineo l'Eliostato a 45 gradi e confermo la misura con [C]."`

- **`SolarClean`**:
  - **Elena Valli**: `"Oltre l'ottanta per cento di rendimento! La pietra si scalda di luce pulita, senza un solo filo di fumo."`
  - **Elena Valli**: `"La Lente Empirica si e' tarata sul campo: ora posso attivare il Rilievo Metrico di Fase 2 con [V]."`
  - **Clara Sereni**: `"Ehi, laggiu' sulle terrazze! Ho visto accendersi gli specchi senza una sbuffata di pece. Raggiungimi al Quartiere dei Canali!"`
  - **Elena Valli**: `"Quella dev'essere Clara Sereni, l'ingegnera delle chiuse. Saliamo verso nord al Quartiere dei Canali per parlarle [E]."`

- **`SolarBypassed`**:
  - **Matteo Verrini**: `"Vede, dottoressa? Con la fornace di emergenza i condotti si scaldano in un istante, senza stare a inseguire le ombre."`
  - **Elena Valli**: `"La corrente scorre, ma una coltre scura sta gia' calando sugli ulivi delle terrazze..."`
  - **Clara Sereni**: `"Chi ha acceso di nuovo la pece sulle terrazze?! Finche' bruciate scorie, il Ponte degli Archi resta chiuso!"`
  - **Elena Valli**: `"Clara non aprira' mai il passaggio con la fornace accesa. Posso tornare all'Eliostato [E] e calibrarlo con la luce."`

- **`ClaraRecruited`**:
  - **Clara Sereni**: `"Ho controllato il flusso sui morsetti di bronzo. Niente fuliggine, solo angolo e pazienza. Avevi ragione tu, Elena."`
  - **Clara Sereni**: `"Tolgo il sigillo dal Ponte degli Archi: ora tutta la meta' orientale di Meridia, dai Giardini al Santuario, e' aperta."`
  - **Clara Sereni**: `"Tieni la mia lente strutturale. Premi [V] per la Trama Geodetica: vedrai le linee di forza che i Meridiani incisero nella pietra."`
  - **Eco di Meridia**: `"Quando due sguardi misurano senza fretta, il ponte torna a unire cio' che l'inverno aveva diviso."`
  - **Elena Valli**: `"Hai sentito quel canto salire dai cristalli, Clara? Scendiamo nelle Cisterne Ipogee a nord-ovest, poi esploreremo oltre il ponte."`

### 4.2 Ingressi Contestuali nei Distretti e nelle Strutture

- **`EnteredCanalQuarter`**:
  - **Elena Valli**: `"Il Quartiere dei Canali... Le vasche a gradini portano ancora i segni dell'acqua limpida che un tempo nutriva l'altopiano."`
  - **Elena Valli**: `"A est vedo il grande Ponte degli Archi, e tra le volte a ovest l'imbocco delle Cisterne Ipogee."`

- **`EnteredWindPlateau`**:
  - **Elena Valli**: `"Sull'Altopiano dei Venti il basalto taglia la brezza e i cipressi si piegano tutti lungo la stessa rosa scolpita nella roccia."`
  - **Elena Valli**: `"In fondo alla cresta si staglia la Torre del Pendolo: i suoi oculi di pietra sembrano ancora attendere il mezzogiorno."`

- **`EnteredHangingGardens`**:
  - **Elena Valli**: `"Oltre il Ponte degli Archi si aprono i Giardini Sospesi. Anche sotto la polvere si avverte il profumo dei semi dormienti."`
  - **Elena Valli**: `"Tra le vasche di giada si intravedono gli anelli del Risonatore Armonico e qualcuno che si prende cura delle aiuole."`

- **`EnteredFumaroleCaldera`**:
  - **Elena Valli**: `"La Caldera delle Fumarole... Qui nacque l'Inverno di Pece. Le vecchie fornaci hanno segnato il basalto come cicatrici."`
  - **Elena Valli**: `"Vicino all'ingresso della Camera Geotermica scorgo la figura di Matteo Verrini, immerso nel vapore."`

- **`EnteredHarmonySanctuary`**:
  - **Elena Valli**: `"Il Santuario dell'Armonia. Sotto i miei passi un intero mosaico astronomico di travertino, bronzo e giada."`
  - **Elena Valli**: `"Al centro della piazza riposa l'Astrolabio di Meridia: i suoi quattro anelli attendono che l'intera Conca ritrovi l'equilibrio."`

- **`EnteredCisterns`**:
  - **Elena Valli**: `"Le Cisterne Ipogee. L'eco delle cascate riempie la volta, e lungo queste passerelle dovro' muovermi con attenzione [SPAZIO]."`
  - **Elena Valli**: `"La legge di Stevino governa questo bacino: a dieci metri di colonna d'acqua la pressione tocca 98.100 Pascal."`
  - **Elena Valli**: `"Sul fondo del bacino mi attende la Turbina Idrostatica; piu' in alto, verso est, intravedo il bagliore di una lanterna."`

- **`EnteredPendulumTower`**:
  - **Elena Valli**: `"Dentro la Torre del Pendolo la luce piove dagli oculi come polvere d'oro. Ogni ballatoio sale verso il grande braccio d'ottone."`
  - **Elena Valli**: `"Galileo insegno' che il periodo del pendolo dipende solo dalla sua lunghezza: due metri esatti per 2,84 secondi di cadenza."`

- **`EnteredGeothermalChamber`**:
  - **Elena Valli**: `"Colonne esagonali di basalto e archi di rame ossidato: la Camera Geotermica custodisce il calore naturale della faglia."`
  - **Elena Valli**: `"Con la sorgente fredda delle cisterne a 300 Kelvin, portare lo scambiatore di Carnot a 600 Kelvin garantisce il 50% di rendimento pulito."`

### 4.3 Acqua, Pendolo, Giardini, Caldera e Astrolabio (Atti II-V ed Epilogo)

- **`HydroClean` / `HydroBypassed`**:
  - *Pulito*: Elena celebra i `98.100 Pascal` senza cavitazione; `Eco di Meridia` richiama la misura dell'acqua; `Lorenzo Altieri` la invita a salire sul ballatoio alto.
  - *Bypass*: Matteo Verrini difende la pompa a pece; un velo scuro macchia le cisterne; `Eco di Meridia` lamenta la sorgente ferita.

- **`LorenzoRecruited`** *(variante se `relics_collected >= 6`)*:
  - Lorenzo Altieri accoglie Elena nell'ala alta delle Cisterne Ipogee, racconta lo Scisma dell'Era III e le consegna la chiave profonda per la Camera Geotermica, sottolineando il valore delle reliquie nel Codex `[J]`.

- **`PendulumClean` / `PendulumBypassed`**:
  - *Pulito*: `L = 2.00 m`, `T = 2.84 s`; Clara sente la sincronizzazione delle chiuse nei canali; `Eco di Meridia` ricorda che il tempo non si forza.
  - *Bypass*: il volano a combustione fa vibrare le vetrate della torre.

- **`MiriamMet`** *(variante in base a `pollution_level <= 25%`)*:
  - Miriam Ferro accoglie Elena nei Giardini Sospesi, parlando di semi dormienti, profumo d'ulivi e del Risonatore Armonico a `120 Hz`.

- **`HarmonicClean` / `HarmonicBypassed`**:
  - *Pulito*: `120 Hz` esatti, la foschia si dissolve e le terrazze tornano a fiorire sotto lo sguardo commosso di Miriam Ferro.
  - *Bypass*: i soffianti a pece costringono Miriam a richiudere i semi sotto vetro.

- **`MatteoConfronted`** *(variante pulito vs con almeno un bypass)*:
  - Confronto umano e diretto nella Caldera delle Fumarole: Matteo racconta i vent'anni d'inverno in cui la pece ha tenuto accese le case a valle; Elena gli riconosce l'assenza di malizia ma gli promette di dimostrare sul campo il rendimento pulito dello Scambiatore di Carnot (`600 K`).

- **`ThermalClean` / `ThermalBypassed`**:
  - *Pulito*: `600 K / 300 K` (`eta = 50%`), solo vapore bianco; Matteo stesso resta stupito dal calore pulito verso valle; Clara segnala il risveglio dell'Astrolabio nel Santuario dell'Armonia.
  - *Bypass*: iniezione di pece e carbone nella faglia basaltica; Lorenzo piange la ferita alle falde profonde.

- **`AstrolabeUnlocked` / `AstrolabeActivated`**:
  - `AstrolabeUnlocked`: l'Eco di Meridia annuncia lo scioglimento del sigillo nel Santuario dell'Armonia.
  - `AstrolabeActivated`: 3 diramazioni distinte in base a percorso pulito e reliquie (`>= 12`, `< 12`, oppure bypass).

- **`SmogCritical`**:
  - Oltre il 70% di cenere, Miriam e Clara avvertono Elena del collasso della flora; Elena decide di ricalibrare gli impianti forzati.

- **Traguardi Collezionabili (`FirstRelic`, `FourRelics`, `HalfRelics`, `TwelveRelics`, `AllRelics`)**:
  - Commenti progressivi di Elena, Lorenzo e dell'Eco di Meridia che accompagnano la ricostruzione delle quattro ere nel Codex `[J]`.

---

## 5. Catalogo Completo delle 16 Reliquie (`src/core/collectible_content.cpp`)

| Indice | Titolo | Era | Forma | Luogo | Parlante alla Raccolta |
|---|---|---|---|---|---|
| `I.1` | **Gnomone d'Ottone** | Era I - Alba delle Ombre | `Astrolabe` | Terrazze dell'Alba | Elena Valli |
| `I.2` | **Stele della Prima Misura** | Era I - Alba delle Ombre | `Stele` | Terrazze dell'Alba | Elena Valli |
| `I.3` | **Cilindro dei Solstizi** | Era I - Alba delle Ombre | `Cylinder` | Altopiano dei Venti | Elena Valli |
| `I.4` | **Seme d'Ulivo Bianco** | Era I - Alba delle Ombre | `FossilSeed` | Quartiere dei Canali | Elena Valli |
| `II.1` | **Diario dell'Idraulico** | Era II - Secolo delle Acque e del Suono | `Cylinder` | Quartiere dei Canali | Clara Sereni |
| `II.2` | **Stele delle Chiusure** | Era II - Secolo delle Acque e del Suono | `Stele` | Cisterne Ipogee | Elena Valli |
| `II.3` | **Diapason di Giada** | Era II - Secolo delle Acque e del Suono | `Prism` | Giardini Sospesi | Miriam Ferro |
| `II.4` | **Cronometro a Filo** | Era II - Secolo delle Acque e del Suono | `Astrolabe` | Torre del Pendolo | Elena Valli |
| `III.1` | **Verbale dello Scisma** | Era III - Inverno di Pece | `Cylinder` | Caldera delle Fumarole | Elena Valli |
| `III.2` | **Stele delle Braci** | Era III - Inverno di Pece | `Stele` | Camera Geotermica | Lorenzo Altieri |
| `III.3` | **Seme Annerito** | Era III - Inverno di Pece | `FossilSeed` | Giardini Sospesi | Miriam Ferro |
| `III.4` | **Termometro di Basalto** | Era III - Inverno di Pece | `Astrolabe` | Camera Geotermica | Elena Valli |
| `IV.1` | **Prisma del Congedo** | Era IV - Sigillo dell'Armonia | `Prism` | Cisterne Ipogee | Eco di Meridia |
| `IV.2` | **Prisma delle Correnti** | Era IV - Sigillo dell'Armonia | `Prism` | Torre del Pendolo | Eco di Meridia |
| `IV.3` | **Seme del Risveglio** | Era IV - Sigillo dell'Armonia | `FossilSeed` | Santuario dell'Armonia | Miriam Ferro |
| `IV.4` | **Stele dell'Astrolabio** | Era IV - Sigillo dell'Armonia | `Stele` | Santuario dell'Armonia | Eco di Meridia |

Ogni scheda nel Codex contiene 3 paragrafi letterari che ricostruiscono l'ascesa scientifica dei Meridiani, la siccita' e lo Scisma dell'Era III, fino al congedo e alla chiusura della Valle dei Semi.

---

## 6. I Tre Finali (`EndingText(clean_path, relics_found, total_relics)`)

1. **`L'ALBA DI MERIDIA - FINALE RIVELATO`** (`clean_path == true && relics_found >= 12`):
   - Cielo limpido, quattro anelli dell'Astrolabio accesi, apertura del passo verso la Valle dei Semi, riconciliazione di Matteo Verrini che depone le chiavi delle fornaci.
2. **`LA RINASCITA DELLA CONCA - FINALE ARMONICO`** (`clean_path == true && relics_found < 12`):
   - I cinque monumenti guariscono l'altopiano senza fumo, ma alcuni anelli dell'Astrolabio restano in penombra in attesa di raggiungere almeno 12 reliquie nel Codex `[J]`.
3. **`IL PREZZO DELLA FRETTA - FINALE GRIGIO`** (`clean_path == false`):
   - Le fornaci forniscono energia ma lasciano una coltre plumbea sul Ponte degli Archi e silenziano l'Eco di Meridia; il giocatore puo' tornare a ricalibrare i monumenti forzati in modo pulito.
