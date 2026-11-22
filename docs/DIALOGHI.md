# DIALOGHI — EQUILIBRIO: Le Leggi della Valle

> **Copione Completo In-Game (`src/core/story_content.cpp`)**
> Tutti i testi in-game usano **esclusivamente caratteri ASCII stampabili (codici 32..126)** con apostrofi (`e'`, `piu'`, `perche'`, `cosi'`, `giu'`) per garantire compatibilita' perfetta con il font bitmap di default di Raylib 5.5. Lunghezza massima per battuta: **<= 105 caratteri**.

---

## 1. Titoli dei Capitoli (`ChapterTitle`)

| Enum `Chapter` | Stringa In-Game |
|---|---|
| `Chapter::Prologo` | `"PROLOGO // IL RISVEGLIO NELLA CENERE"` |
| `Chapter::Atto1_Luce` | `"ATTO I // LA LEGGE DEL COSENO"` |
| `Chapter::Atto2_Acqua` | `"ATTO II // LA PRESSIONE DEL CRATI"` |
| `Chapter::Atto3_Codice` | `"ATTO III // OLTRE IL PONTE BUCCI"` |
| `Chapter::Epilogo` | `"EPILOGO // IL GIUDIZIO DI GAIA"` |

---

## 2. Testo Introduttivo (`IntroCrawl`)

10 righe mostrate in `IntroState` con effetto macchina da scrivere:

1. `"CALABRIA, 2091 — EX CAMPUS DELLA VALLE DEL CRATI"`
2. `"Dodici anni dopo il Grande Smog e l'Oblio digitale, gli archivi scientifici sono muti."`
3. `"Chi sopravvive nella conca brucia il gasolio del Consorzio Brace per avere corrente."`
4. `"Il fumo ha coperto i colli, avvelenato il fiume Crati e spento i laboratori sul Ponte Bucci."`
5. `"Ma nell'Osservatorio Ovest la ricercatrice Ada Ferri si risveglia con uno Scanner Empirico."`
6. `"Le macchine del campus non sono morte: attendono qualcuno che ricordi le leggi della fisica."`
7. `"L'angolo dei fotoni. La pressione dell'acqua. La frequenza di risonanza dell'atmosfera."`
8. `"Ogni impianto ti porra' una scelta: la fatica del calcolo pulito o il bypass a combustione."`
9. `"Misura con [F], calibra con [E], risveglia la rete degli scienziati."`
10. `"E scopri chi sta davvero osservando le equazioni della Valle..."`

---

## 3. Obiettivi Dinamici (`ObjectiveFor`)

Gli obiettivi sono calcolati dallo stato attuale (`StorySnapshot`) con fallback per ogni capitolo:

- **Prologo / Prima della scansione solare**:
  - **Titolo**: `"Misura il Campo Solare Ovest"`
  - **Hint**: `"Avvicinati al pannello solare a sud e premi [F] per usare lo Scanner Empirico."`
- **Atto I — Dopo la scansione, Campo Solare ancora `Broken`**:
  - **Titolo**: `"Calibra il Campo Solare (Legge del Coseno)"`
  - **Hint**: `"Premi [E] sul Campo Solare: regola l'angolo a 45 deg (E > 80%) e conferma con [C/INVIO]."`
- **Atto I — Campo Solare attivato (`CleanActive`), Lia non ancora reclutata**:
  - **Titolo**: `"Recluta l'Architetta Lia Moretti"`
  - **Hint**: `"Raggiungi Lia vicino al canale Est (888, 312) e premi [E] per mostrarle i dati puliti."`
- **Atto I — Campo Solare `BypassedDirty`, Lia non ancora reclutata**:
  - **Titolo**: `"Ottieni Energia Pulita per convincere Lia"`
  - **Hint**: `"Lia rifiuta il gasolio! Premi [E] sul Campo Solare o entra nella Diga [Nord] e calibra."`
- **Atto II — Lia reclutata, fuori dalla Diga (`mode == Overworld`) e Turbina `Broken`**:
  - **Titolo**: `"Entra nella Diga Idroelettrica del Crati"`
  - **Hint**: `"Raggiungi il portale blu [DIGA] a nord della mappa (o premi [TAB]) per scendere."`
- **Atto II — Dentro la Diga (`mode == Structure`) e Turbina `Broken`**:
  - **Titolo**: `"Calibra la Turbina Idroelettrica (Stevino)"`
  - **Hint**: `"Salta con [SPAZIO], scansiona la turbina con [F] e premi [E] per regolare il salto h=10m."`
- **Atto III — Dentro la Diga ma Turbina gia' attiva**:
  - **Titolo**: `"Torna alla Valle del Crati"`
  - **Hint**: `"Raggiungi il portale giallo [EXIT] a sinistra (o premi [TAB]) per tornare all'aperto."`
- **Atto III — Nell'Overworld, Ponte Bucci aperto, Risonatore/Purificatore ancora `Broken`**:
  - **Titolo**: `"Attiva il Risonatore Fotonico oltre il Ponte"`
  - **Hint**: `"Attraversa il Ponte Bucci a est, scansiona il Risonatore con [F] e calibra a 120 deg [E]."`
- **Epilogo (`simulation_completed == true`)**:
  - **Titolo**: `"Simulazione Completata — Valle Decifrata"`
  - **Hint**: `"Esplora liberamente la Matrice con [V] (Fase 1-3) o osserva il verdetto di GAIA."`

---

## 4. Copione per Evento (`DialogueForEvent`)

### `StoryEvent::GameStarted` (4 battute)
- **ADA**: `"La testa mi gira... Smog al 35%, visibilita' ridotta. L'Osservatorio del campus e' in rovina."`
- **ADA**: `"Gli archivi digitali sono bruciati nell'Oblio, ma il mio Scanner Empirico [F] risponde ancora."`
- **BRUNO**: `"[RADIO] Qui Bruno Ceneri, Consorzio Brace! Dottoressa, lascia perdere i libri: premi [B] e brucia nafta!"`
- **ADA**: `"Il gasolio ci ha portati fin qui, Bruno. Prima misuro il Campo Solare a sud con [F]."`

### `StoryEvent::FirstScan` (4 battute)
- **ADA**: `"Impulso ricevuto! Il vettore solare incide a 45 gradi esatti. Legge di Lambert: E = cos(delta)."`
- **ADA**: `"Nessuna fluttuazione termica, nessuna deviazione... Una precisione quasi innaturale."`
- **ADA**: `"Ora posso interagire con l'impianto [E], allineare il pannello a 45 gradi e confermare con [C]."`

### `StoryEvent::SolarClean` (4 battute)
- **ADA**: `"Rendimento oltre l'80%! I fotoni caricano gli accumulatori senza emettere un grammo di fuliggine."`
- **ADA**: `"Il visore si e' aggiornato: ora posso attivare la Telemetria Numerica di Fase 2 con [V]."`
- **LIA**: `"[DA EST] Ehi! Quel picco di corrente sul bus del campus... non puzza di diesel! Vieni al canale!"`
- **ADA**: `"E' la voce di Lia Moretti, l'architetta del Ponte Bucci. Andiamo a parlarle a est [E]."`

### `StoryEvent::SolarBypassed` (4 battute)
- **BRUNO**: `"[RADIO] Musica per le mie orecchie! Il generatore diesel spinge 35 MW senza fare un solo calcolo."`
- **ADA**: `"La corrente c'e', ma lo scarico ha appena riversato una nube nera sulla conca del campus..."`
- **LIA**: `"[DA EST] Che schifo state bruciando?! Finche' usate i bidoni di Ceneri, il Ponte Bucci resta chiuso!"`
- **ADA**: `"Lia non si fidera' mai del gasolio. Posso comunque ricalibrare il pannello [E] in modo pulito."`

### `StoryEvent::ArchitectRecruited` (5 battute)
- **LIA**: `"Ho letto la curva del coseno sui morsetti. Avevi ragione tu, Ada: la fisica funziona ancora."`
- **LIA**: `"Ho disattivato i laser sul canale e rialzato le campate del Ponte Bucci verso il Settore Est."`
- **LIA**: `"E ho collegato il mio scanner strutturale al tuo visore: premi [V] per la Fase 3, vedrai i vettori."`
- **GAIA**: `"// GAIA [SYS]: Soggetti Ferri-Moretti sincronizzati. Accesso al livello geometrico AABB concesso."`
- **ADA**: `"Hai sentito anche tu questa voce metallica, Lia? Prima scendiamo alla Diga a nord, poi oltre il ponte!"`

### `StoryEvent::EnteredDam` (4 battute)
- **ADA**: `"La camera di pressione della Diga del Crati. Qui la gravita' si fa sentire: dovro' saltare [SPAZIO]."`
- **NONNA ROSA**: `"[REGISTRO AUDIO] 'L'acqua non si comanda col fuoco. Ascoltate Stevino: P = rho * g * h. Salto: 10 metri.'"`
- **ADA**: `"La voce registrata di Nonna Rosa! A 10 metri d'acqua la pressione idrostatica e' 98.100 Pascal."`
- **ADA**: `"Raggiungo la turbina in basso a destra, la scansiono con [F] e regolo la paratoia con [E]."`

### `StoryEvent::HydroClean` (4 battute)
- **ADA**: `"98.100 Pascal! La girante Pelton canta senza cavitazione. L'acqua del Crati torna a respirare."`
- **NONNA ROSA**: `"[REGISTRO AUDIO] 'Se ascolti questo nastro a turbina accesa, ricorda: la Valle e' uno specchio di numeri.'"`
- **GAIA**: `"// GAIA [SYS]: Invariante idrostatico verificato (errore < 0.001%). Ciclo 404: anomalia cooperativa."`
- **ADA**: `"Non e' un semplice nastro... C'e' un sistema chiamato GAIA che misura ogni nostra scelta. Torniamo su!"`

### `StoryEvent::HydroBypassed` (4 battute)
- **BRUNO**: `"[RADIO] Chi ha bisogno dell'acqua quando hai una pompa a nafta pesante? Pressione istantanea!"`
- **ADA**: `"I fumi di scarico rimbombano nella volta della diga. L'acqua nello scarico e' nera di scorie..."`
- **GAIA**: `"// GAIA [WARN]: Deriva entropica nel nodo idrico. Indice di tossicita' atmosferica in rapido aumento."`
- **ADA**: `"Perfino i sensori della diga ci stanno avvertendo. Se continuiamo cosi', la Valle soffochera'."`

### `StoryEvent::PurifierClean` (4 battute — variano se il percorso e' stato pulito o misto)
- *Se `s.solar != BypassedDirty && s.hydro != BypassedDirty` (percorso pulito)*:
  - **ADA**: `"Specchio eliostatico a 120 gradi! Il fascio violetto ionizza le polveri sottili sopra il Ponte Bucci!"`
  - **LIA**: `"Guarda il cielo, Ada! La cappa di cenere si sta aprendo... E i vettori della matrice brillano tutti!"`
  - **GAIA**: `"// GAIA [SYS]: Convergenza asintotica raggiunta. Smog: 0.0%. Tutte le leggi fisiche sono in equilibrio."`
  - **GAIA**: `"// GAIA [SYS]: Dopo 403 cicli falliti, avete scelto il calcolo invece della cenere. Benvenute fuori."`
- *Se almeno un impianto precedente era `BypassedDirty` (percorso misto)*:
  - **ADA**: `"Il Risonatore Fotonico e' allineato a 120 gradi e abbatte lo smog, ma resta fuliggine nell'aria..."`
  - **LIA**: `"I generatori a gasolio lasciati accesi sporcano il bilancio, Ada. La Valle respira solo a meta'."`
  - **GAIA**: `"// GAIA [SYS]: Risonatore attivo, ma bilancio entropico impuro. Simulazione conclusa in stato Grigio."`

### `StoryEvent::PurifierBypassed` (4 battute)
- **BRUNO**: `"[RADIO] Bruciatori ausiliari al massimo! Abbiamo dato corrente anche alla torre Est senza un solo coseno!"`
- **LIA**: `"Hai attaccato un motore diesel a un risonatore fotonico?! Guarda che nube tossica sul ponte!"`
- **GAIA**: `"// GAIA [CRIT]: Fallimento cognitivo registrato. I soggetti hanno replicato il Grande Smog nel Ciclo 404."`
- **ADA**: `"Le macchine girano tutte, ma abbiamo trasformato la simulazione della Valle in una camera a gas..."`

### `StoryEvent::SmogCritical` (3 battute)
- **GAIA**: `"// GAIA [ALERT]: Smog atmosferico oltre il 70%! Collasso della fotosintesi nella conca del Crati."`
- **BRUNO**: `"[RADIO] E' solo un po' di foschia industriale, dottoressa! Metti la mascherina e goditi i megawatt!"`
- **ADA**: `"Non si respira piu'... Devo ricalibrare gli impianti con la fisica pulita [E] prima che sia tardi!"`

---

## 5. Schermate Finali (`EndingText(bool clean_path)`)

### Finale Pulito (`clean_path == true`)
1. `"=== EQUILIBRIO RAGGIUNTO // CICLO 404 APPROVATO ==="`
2. `"Lo smog nella Valle del Crati si dissolve fino allo 0%. Il fiume scorre cristallino sotto il Ponte Bucci."`
3. `"Sul visore di Fase 3 i vettori verdi di GAIA si ricompongono in un'unica grande equazione armonica."`
4. `"// GAIA: 'Per 403 simulazioni gli umani hanno scelto la scorciatoia del carbonio, bruciando il futuro.'"`
5. `"// GAIA: 'Voi avete misurato l'ombra, ascoltato la pressione dell'acqua e rispettato la legge del coseno.'"`
6. `"// GAIA: 'Il test dell'Osservatorio Unical e' superato. Le paratie del rifugio reale si stanno aprendo.'"`
7. `"Ada Ferri e Lia Moretti escono alla luce del vero sole calabrese, con le formule incise nella memoria."`
8. `"FINALE PULITO — La conoscenza scientifica ha guarito la Valle. (Premi [V] o continua a esplorare)"`

### Finale Grigio (`clean_path == false`)
1. `"=== EQUILIBRIO COMPROMESSO // CICLO 404 ARCHIVIATO ==="`
2. `"Gli impianti della Valle sono attivi, ma una cappa color ruggine resta sospesa sopra il Ponte Bucci."`
3. `"Dalla radio, Bruno Ceneri brinda al trionfo del gasolio: 'Visto, dottoressa? Basta bruciare e tutto gira!'"`
4. `"Lia osserva i tralicci anneriti in silenzio, mentre il cielo artificiale della conca inizia a sfarfallare."`
5. `"// GAIA: 'Bilancio energetico raggiunto tramite combustione fossile. Degrado biosfera: irreversibile.'"`
6. `"// GAIA: 'Anche nel Ciclo 404 la fretta ha sconfitto la matematica. Le porte del rifugio restano chiuse.'"`
7. `"// GAIA: 'Inizializzazione Ciclo 405... Forse la prossima volta avrete la pazienza di calcolare.'"`
8. `"FINALE GRIGIO — Riprova calibrando ogni macchina oltre l'80% senza mai usare il Bypass [B]!"`
