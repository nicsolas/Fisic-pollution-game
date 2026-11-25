# DIALOGHI, LANDMARK E RELIQUIE -- EQUILIBRIO: Le Cronache di Meridia

> **Copione Reattivo In-Game (`src/core/story_content.cpp`) e Catalogo Strumenti Codex (`src/core/collectible_content.cpp`)**
> Tutti i testi destinati al rendering usano **esclusivamente caratteri ASCII stampabili (codici 32..126)** con apostrofi (`e'`, `piu'`, `civilta'`, `perche'`, `cosi'`) e doppi trattini (`--`).
> Ogni battuta viene trascritta nella **Cronologia Dialoghi (`[H]`)** e ogni manufatto sblocca storia + istruzioni d'uso sul campo nel **Codex (`[J]`)**.

---

## 1. Cast e Voci Reattive

| Parlante | Ruolo | Registro e Reattivita' alle Scelte |
|---|---|---|
| **Elena Valli** | Naturalista e Cartografa | Taccuino empirico: non anticipa numeri a memoria, ma deduce le leggi da ombre, condotti, schiuma e prove `[T]`. |
| **Clara Sereni** | Ingegnera dei Canali | Calorosa e fiduciosa in `CleanSlow` (apre Ponte + Passaggio Artigiani); vigile sui manometri in `RiskyEfficient`; amara dietro un panno filtrante in `DirtyImmediate`. |
| **Lorenzo Altieri** | Archivista delle Cisterne | Scende commosso al bacino aprendo la Nicchia in `CleanSlow`; ammira con cautela lo Sfioratore Alto in `RiskyEfficient`; ripara tossendo sulle volte alte in `DirtyImmediate`. |
| **Miriam Ferro** | Botanica dei Giardini Sospesi | Sensoriale, attenta a germogli, polveri sottili, semi fossili e risonanze di vetro-giada. |
| **Matteo Verrini** | Sovrintendente della Pece | Pragmatico e umano: rivendica la rapidita' delle fornaci, resta colpito dalla potenza del Regime Critico e depone le chiavi davanti al percorso pulito completo. |
| **Eco di Meridia** | Cristalli Armonici dell'Era IV | Voce epigrafica incisa nella pietra: celebra l'equilibrio paziente e ammonisce sulla tensione o sulle sorgenti ferite. |

---

## 2. Guida Ridotta e Appunti Empirici (`ObjectiveFor`)

Nelle fasi della Vertical Slice (`Prologo`, `Atto1_Luce`, `Atto2_Acqua` e sintesi d'apertura di `Atto3_Pendolo`), gli obiettivi non forniscono coordinate GPS ne' valori numerici precompilati, ma **note di lettura diegetica**:

- **`Prologo`**:
  - Prima delle osservazioni: *"Taccuino: Leggi le Ombre dell'Alba"* -- `"Osserva meridiane e condotti spenti nel travertino [E], poi usa [Q]."`
  - Dopo il primo rilievo: *"Taccuino: Segui i Condotti d'Ottone"* -- `"Segui le tracce metalliche fino al riflettore e sonda la luce [Q]."`
- **`Atto1_Luce`**:
  - Prima dell'esperimento: *"Ipotesi sulle Terrazze dell'Alba"* -- `"Orienta gli specchi [E], formula un'ipotesi sul banco e prova [T]."`
  - Dopo l'Impulso di Prova: *"Conseguenza e Scelta Solare"* -- `"Valuta l'esito sul banco [E]: Armonia, Regime Critico o Pece."`
  - Post-attivazione differenziato per `CleanSlow` (condotti dorati), `RiskyEfficient` (Cresta Arsa / cavi tesi) e `DirtyImmediate` (veto di Clara per il fumo).
- **`Atto2_Acqua`**:
  - Nei Canali: invita a leggere schiuma, sponde e idrometri `[E]` (o ad applicare il *Diario dell'Idraulico* per aprire le chiuse laminari) seguendo la caduta d'acqua verso i pozzi occidentali.
  - Nelle Cisterne: *"Indagine nei Pozzi di Calma"* (`"Osserva le tacche sommerse [E], formula un'ipotesi e prova con [T]."`) -> *"Il Peso dell'Acqua nella Cisterna"*.
- **Sintesi Vertical Slice (`Atto3_Pendolo` iniziale)**:
  - Invita a incontrare Lorenzo sotto le volte orientali e poi risalire al **Ponte degli Archi** per il bilancio complessivo con Clara Sereni (`VerticalSliceSynthesis`).

---

## 3. Copione Completo dei 40 Eventi (`DialogueForEvent`)

### 3.1 Ciclo Solare, Osservazione e Terrazze dell'Alba

- **`GameStarted`**: Elena nota l'assenza di una bussola automatica e osserva i condotti d'ottone spenti e l'ombra sulla meridiana di travertino; Matteo Verrini offre il bypass immediato a pece `[B]` per scaldare i borghi a valle; Elena sceglie di osservare `[E]`, sondare `[Q]` e mettere alla prova la propria ipotesi.
- **`FirstScan`**: attraverso la Lente `[Q]` Elena rileva che l'ombra dello gnomone eguaglia l'altezza dello stilo verticale, deducendo per la legge di Lambert che la normale dello specchio deve bisecare il quadrante solare prima di verificare con un Impulso di Prova `[T]`.
- **`ObservationClueRead`** *(Nuovo)*: primo appunto dalle stazioni diegetiche (`ObservationNode`) che esplicita il metodo dei Meridiani: *osservare i limiti incisi nella pietra -> scegliere l'ipotesi -> verificare la conseguenza con `[T]`*.
- **`SolarTrialRun`** *(Nuovo)*: l'Impulso di Prova `[T]` scuote la polvere dalle incisioni con un lampo nei condotti d'ottone e misura lo scarto ottico; Matteo commenta il tremito dei morsetti mentre Elena prepara la scelta fra `[INVIO]`, `[R]` e `[B]`.
- **`SolarClean`** *(con guardia reattiva su `solar_approach`)*: in `CleanSlow` i condotti pulsano d'oro caldo, i rovi di pece si sgretolano, tornano le rondini sugli ulivi, si sblocca il Rilievo Metrico `[V]` e Clara chiama entusiasta dai Canali.
- **`SolarRisky`** *(Nuovo -- `RiskyEfficient`)*: il raggio concentrato in sovraccarico fonde i rovi aprendo la **Scorciatoia della Cresta Arsa** tra rocce vetrificate; Matteo resta colpito dai 32 watt puliti ma tesi, mentre Clara chiama dai Canali avvertendo che i manometri vibrano al limite termico.
- **`SolarBypassed`** (`DirtyImmediate`): fumo acre di catrame sulle Terrazze, rondini scomparse e rovi neri che serrano i passaggi bassi; Clara parla tossendo dietro un panno bagnato e tiene chiuso il Ponte finche' non si ricalibra l'impianto `[E]` o si bonificano le vasche.

### 3.2 Quartiere dei Canali, Strumenti Vivi e Cisterne Ipogee

- **`EnteredCanalQuarter`** *(variante inquinato vs normale)*: lettura diegetica delle colonnine idrometriche, della schiuma bruna o chiara nei salti d'acqua e del Guado sommerso.
- **`ClaraRecruited`** *(variante `CleanSlow` vs `RiskyEfficient`)*:
  - In `CleanSlow`: Clara elogia l'assenza di fuliggine, apre il **Ponte degli Archi** e il **Passaggio degli Artigiani**, e condivide la **Trama Geodetica (`[V]`)** accompagnata dall'Eco di Meridia.
  - In `RiskyEfficient`: Clara ha la chiave inglese sui bulloni vibranti del collettore; ammira il coraggio tecnico, apre il Ponte e dona la Trama Geodetica `[V]` raccomandando prudenza nelle Cisterne (*"La corda tesa porta lontano la freccia, ma chiede mano ferma per non spezzare l'arco"*).
- **`BioPlanterSeeded`** *(Nuovo)*: il **Seme d'Ulivo Bianco** germoglia nella conca di pietra, spacca la crosta di catrame, dissolve i **Rovi di Pece** e rinfresca il sentiero sotto gli occhi stupiti di Clara.
- **`LaminarFordOpened`** *(Nuovo)*: applicando il **Diario dell'Idraulico** sulle leve delle chiuse, la piena diventa un velo laminare silenzioso e fa affiorare asciutto il **Guado degli Idraulici**.
- **`EnteredCisterns`**: Elena ascolta il rimbombo delle cascate e individua le tacche metriche in calce e bronzo sulle colonne sommerse di Stevino, preparando l'osservazione piezometrica prima di scendere alla Turbina.
- **`HydroTrialRun`** *(Nuovo)*: colpo d'ariete controllato `[T]` lungo la condotta forzata; i canali di scolo sussultano indicando lo scarto piezometrico e Lorenzo riconosce dall'alto l'antico "colpo d'assaggio" dei fontanieri.
- **`HydroClean`**: l'acqua schiarisce come vetro azzurro senza cavitazione, tornano i tritoni luminosi e Lorenzo scende al bacino aprendo la **Nicchia Sigillata dei Cilindri**.
- **`HydroRisky`** *(Nuovo -- `RiskyEfficient`)*: regime supercritico ad altissima resa; le tubazioni gocciolano vapore freddo, la pressione **spalanca lo Sfioratore Alto** verso il Prisma e Clara avverte che nei Canali la corrente e' accelerata del `+45%`.
- **`HydroBypassed`** (`DirtyImmediate`): patina oleosa sul bacino, lucciole di grotta spente e vapore corrosivo; Lorenzo si ritira tossendo sulle volte alte per coprire i cilindri d'archivio.
- **`LorenzoRecruited`** *(4 varianti reattive: `DirtyImmediate`, `RiskyEfficient`, `CleanSlow` con `>= 4` reliquie, `CleanSlow` base)*: consegna la chiave di bronzo adattando tono, posizione e commento allo stato delle Cisterne e agli strumenti raccolti.

### 3.3 Bilancio Reattivo della Vertical Slice (`VerticalSliceSynthesis`)

Quando il giocatore ha affrontato Terrazze, Canali e Cisterne e si confronta con Clara al **Ponte degli Archi**, il dialogo riflette tutte le **5 combinazioni di scelte** + il conteggio e l'uso degli strumenti (`white_olive_planted`, `laminar_ford_open`, `relics_collected`):
1. **Doppia Armonia (`CleanSlow` + `CleanSlow`)**: rondini sulle Terrazze dorate, carpe d'argento nei Canali limpidi, respiro di quarant'anni fa nelle Cisterne e bussola geodetica consacrata.
2. **Doppio Regime Critico (`RiskyEfficient` + `RiskyEfficient`)**: parapetto vibrante sul Ponte, Cresta Arsa fumante a sud e rapide supercritiche a ovest; Matteo ammette la luce pienissima a valle mentre Lorenzo avverte della pietra tesa come una corda di violino.
3. **Equilibrio Misto senza Pece (`CleanSlow` + `RiskyEfficient` o viceversa)**: contrasto vivo tra il distretto guarito in quiete e quello spinto in sovraccarico calcolato, celebrando il rifiuto totale del catrame.
4. **Conca Divisa a Meta' (una scelta Pulita/Rischiosa + una Fornace a Pece)**: specchio vivente dello Scisma dell'Era III, con meta' ecosistema rifiorito e meta' chiuso dai rovi neri, invitando a ricalibrare `[E]` l'impianto ferito.
5. **Doppia Fornace a Pece (`DirtyImmediate` + `DirtyImmediate`)**: Matteo rivendica i kilowatt immediati, ma Clara mostra dal Ponte il silenzio senza rondini ne' carpe e i varchi serrati dai rovi, ricordando che i banchi `[E]` restano sempre aperti al riscatto.

### 3.4 Atti III-V, Epilogo e Traguardi Codex
- Copertura integrale di `EnteredWindPlateau`, `EnteredPendulumTower`, `PendulumClean`/`PendulumBypassed`, `EnteredHangingGardens`, `MiriamMet`, `HarmonicClean`/`HarmonicBypassed`, `EnteredFumaroleCaldera`, `MatteoConfronted`, `EnteredGeothermalChamber`, `ThermalClean`/`ThermalBypassed`, `AstrolabeUnlocked`, `AstrolabeActivated` (3 rami), `SmogCritical` e dei 5 traguardi `FirstRelic`..`AllRelics`.

---

## 4. Landmark Diegetici della Vertical Slice (`ZoneLandmarks`)

### Terrazze dell'Alba (`DawnTerraces` -- 4 Landmark)
1. `dawn_sundial` -- **Meridiana Madre del Mezzogiorno**: uguaglianza tra ombra e altezza dello stilo verticale -> bisettrice del quadrante per il coseno di Lambert.
2. `dawn_gnomon` -- **Obelisco del Coseno e dei Condotti**: codice visivo delle vene d'ottone (fredde, oro, ambra vibrante, nere di pece) e invito all'Impulso di Prova `[T]`.
3. `dawn_olives` -- **Conca degli Ulivi Bianchi**: memoria botanica dell'Era I e indizio per innestare il *Seme d'Ulivo Bianco* nelle vasche aride `[E]` contro i rovi.
4. `dawn_crest` -- **Cresta dei Rovi Vetrificati**: indizio fisico sul Regime Critico `[R]` con stabilizzatore aperto per fondere la barriera rocciosa al prezzo di stress termico.

### Quartiere dei Canali (`CanalQuarter` -- 4 Landmark)
1. `canal_plaque` -- **Idrometro di Valerio degli Archi**: relazione tra pendenza dolce (flusso laminare + guado), spinta supercritica e vortici di pece.
2. `canal_amphora` -- **Pozzo di Calma di Stevino**: indizio ragionato sulle braccia metriche d'acqua quieta rispetto alla soglia supercritica che spalanca gli sfioratori alti nelle Cisterne.
3. `canal_arch` -- **Pilone del Ponte degli Archi**: architettura a conci serrati senza malta e punto di ritrovo con Clara per il bilancio dell'anello idraulico.
4. `canal_ford` -- **Soglia del Guado degli Idraulici**: lastroni sommersi tra Canali e Cisterne che emergono abbattendo l'inquinamento o attivando il *Codice delle Chiuse Laminari*.

---

## 5. Schede Codex delle 16 Reliquie-Strumento (`collectible_content.cpp`)

Ogni reliquia combina nel `pickup_line` e nei 3 `codex_paragraphs`:
1. **Origine storica** nell'arco delle Quattro Ere;
2. **Principio fisico o biologico** scoperto dai Meridiani;
3. **`USO SUL CAMPO (...)` esplicito**, spiegando al giocatore come impiegarla attivamente con `[Q]`, `[E]`, `[T]` o `[V]` (in piena sintonia con il riquadro *STRUMENTO & EFFETTO LUDICO* di `GetRelicGameplaySpec`).
