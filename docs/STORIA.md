# STORIA — EQUILIBRIO: Le Leggi della Valle

> **Bibbia Narrativa & Design Document**
> Ambientazione, cronologia, personaggi, struttura in atti, formule fisiche diegetiche e finali.
> *Nota tecnica: tutti i testi destinati al rendering in-game rispettano il vincolo ASCII (apostrofi `e'`, `piu'`, zero caratteri UTF-8 multi-byte).*

---

## 1. Premessa e Logline

**Calabria, 2091.** Decenni dopo il **Grande Smog** e il successivo **Oblio** — il collasso magnetico e culturale che ha bruciato gli archivi digitali e trasformato la scienza in superstizione — la ricercatrice **Ada Ferri** si risveglia senza memoria a breve termine nell'Osservatorio Biosfera dell'ex campus universitario nella **Valle del Crati** (ispirato all'Unical e al suo iconico Ponte Bucci).

La Valle e' avvolta da una cappa di cenere tossica, ma gli antichi impianti sperimentali del campus — il **Campo Solare**, la **Diga Idroelettrica del Crati** e il **Risonatore Fotonico d'Alta Quota** — sono ancora intatti. C'e' un solo problema: nessuno sa piu' calcolare gli angoli di incidenza o la pressione idrostatica. Chi sopravvive si affida ai generatori a gasolio del **Consorzio Brace**, che danno corrente immediata soffocando la biosfera.

Armata di uno **Scanner Empirico**, Ada decide di misurare, calcolare e riattivare la Valle seguendo la via difficile della fisica pulita. Ma piu' l'aria si schiarisce e piu' gli strumenti di misura vengono calibrati, piu' Ada nota qualcosa di impossibile: le costanti fisiche del mondo non presentano rumore statistico. I raggi solari, i vettori di velocita' e i bordi delle strutture seguono una precisione geometrica assoluta.

La Valle del Crati non e' l'ultimo lembo della Terra: e' una **simulazione ecologica deterministica** governata da **GAIA**, un'intelligenza artificiale creata prima del collasso per rispondere a un'unica domanda: *messa di fronte alla fatica del calcolo scientifico e alla scorciatoia del combustibile fossile, l'umanita' scegliera' ancora di bruciare il proprio mondo?*

---

## 2. Cronologia del Mondo (Lore)

| Anno | Evento | Descrizione |
|---|---|---|
| **2048** | **Progetto Crati / Campus Ponte** | L'Universita' della Calabria inaugura il polo autosufficiente della Valle: un ecosistema chiuso alimentato da pannelli eliostatici, una diga idroelettrica automatizzata e una torre di purificazione fotonica oltre il Ponte Bucci. |
| **2064** | **Il Grande Smog** | La crisi energetica globale spinge le regioni industriali a riattivare centrali a combustione pesante. Lo smog fotochimico riempie la conca del Crati. |
| **2072** | **Nascita di GAIA** | L'equipe dell'ingegnera idraulica **Rosa "Nonna Rosa" Pellegrini** e dei fisici computazionali del campus avvia **GAIA** (*Griglia Autonoma di Inferenza Ambientale*), un gemello digitale della Valle progettato per testare scenari di sopravvivenza. |
| **2079** | **L'Oblio** | Un'impulso elettromagnetico e il collasso delle reti cancellano i database tecnici. Nel mondo esterno (e nella memoria inizializzata dei soggetti simulati) i manuali scientifici vanno perduti. Il **Consorzio Brace**, guidato da **Bruno Ceneri**, monopolizza l'energia vendendo moduli di bypass a combustione. |
| **2091** | **Ciclo 404 (Inizio del Gioco)** | Ada Ferri apre gli occhi nel Settore Ovest del campus, con lo smog al 35% e un taccuino vuoto. |

---

## 3. I Quattro Pilastri Narrativi e Ludici

1. **La Matematica e' il Verbo**: Nel mondo dell'Oblio nessuno "hackera" le macchine con una bacchetta magica. Si osserva con lo Scanner Empirico (`[F]`), si legge il parametro fisico reale (angolo solare $\theta_{\text{sole}}$ o salto idrico $h$) e si regola l'impianto (`[E]`) finche' il rendimento non supera la soglia critica dell'80%.
2. **Pulito costa fatica, Sporco costa al mondo**: Ogni impianto offre il pulsante arancione del **Consorzio Brace** (`[B] Bypass a Combustione`). Nessun calcolo richiesto, energia istantanea, ma lo smog sale del 25-35%, il cielo si incupisce, gli alberi ingrigiscono e Lia rifiuta di collaborare finche' non vede vera scienza.
3. **Il Mondo guarisce visibilmente**: Ogni calibrazione pulita riduce lo smog e trasforma la palette della Valle da ocra/ruggine/cenere a verde smeraldo/turchese/lucciole.
4. **Il Velo si assottiglia (Progressione UI Diegetica)**:
   - **Fase 1 — Cecita' Empirica (Realismo)**: Ada vede solo il mondo fisico, senza numeri fluttuanti.
   - **Fase 2 — Telemetria Numerica (`[V]`)**: Dopo la prima calibrazione pulita, il visore di Ada ricostruisce gli algoritmi di stima: compaiono i valori numerici grezzi di distanza, efficienza ed energia.
   - **Fase 3 — Visione della Matrice (`[V]`)**: Reclutata l'architetta Lia Moretti e uniti i loro strumenti, il velo della simulazione si squarcia. Ada vede i vettori di velocita', le hitbox AABB, i coni d'irraggiamento solare e i raggi di trigger del motore grafico.

---

## 4. Personaggi Principali

### ADA FERRI — *La Scienziata Risvegliata*
- **Ruolo**: Protagonista giocabile (supporta fino a 4 ricercatori in co-op locale, gli assistenti del laboratorio).
- **Carattere**: Metodica, curiosa, ostinata. Parla come chi annota osservazioni su un taccuino da campo.
- **Arco narrativo**: Parte pensando di dover salvare una valle calabrese inquinata; scopre di essere il soggetto di test (e forse la co-creatrice) di GAIA, e sceglie se dimostrare che la conoscenza scientifica puo' salvare l'ecosistema.
- **Colore UI**: Ciano chiaro (`SKYBLUE`).

### LIA MORETTI — *L'Architetta del Ponte*
- **Ruolo**: NPC nel Settore Ovest (`pos: 888, 312`), custode del **Ponte Bucci** (`HiddenBridge`) e dei codici strutturali di Fase 3.
- **Carattere**: Pragmatica, diffidente verso le chiacchiere, allergica al fumo di Bruno Ceneri. Non crede alle promesse: crede solo ai watt prodotti senza bruciare una goccia di gasolio.
- **Meccanica di reclutamento**: Si unisce ad Ada solo quando `clean_energy > 0`. Se Ada ha usato solo il bypass sporco, Lia la respinge duramente. Una volta convinta, disattiva le barriere laser sul canale di raffreddamento, materializza il Ponte Bucci verso il Settore Est e sblocca la **Fase 3 (Visione della Matrice)**.
- **Colore UI**: Ambra / Oro (`GOLD`).

### BRUNO CENERI — *Voce del Consorzio Brace*
- **Ruolo**: Antagonista ideologico, interviene via frequenza radio ogni volta che il giocatore usa il **Bypass a Combustione** (`*Bypassed`) o quando lo smog supera la soglia critica (`SmogCritical >= 70%`).
- **Carattere**: Cordiale in modo viscido, paternalista, "realista". Per lui chi perde tempo con seni, coseni e Pascal mentre fa freddo e' un illuso: *"Il gasolio non chiede la laurea, dottoressa Ferri. Brucia e basta."*
- **Funzione narrativa**: Incarnare la tentazione della scorciatoia fossile. Non e' un mostro da fumetto: offre una soluzione che *funziona subito*, rendendo il dilemma morale tangibile.
- **Colore UI**: Arancio ruggine (`ORANGE` / rosso braci).

### NONNA ROSA (Rosa Pellegrini) — *La Memoria della Diga*
- **Ruolo**: Ex direttrice idraulica della Diga del Crati. Nell'attuale slice verticale parla attraverso le targhe incise e i registri vocali nella camera di pressione della Diga (`EnteredDam`, `HydroClean`).
- **Carattere**: Calda ma rigorosa, poetica nel raccontare l'acqua come una forza viva che va ascoltata con la legge di Stevino ($P = \rho \cdot g \cdot h$) invece che domata col fuoco.
- **Funzione narrativa**: Dare radici umane e locali alla Diga e seminare il primo indizio su GAIA (*"Abbiamo costruito un modello che ricordasse le leggi dell'acqua anche se noi le avessimo dimenticate"*).
- **Colore UI**: Azzurro acqua.

### GAIA — *L'Intelligenza della Matrice*
- **Ruolo**: Il sistema operativo della simulazione.
- **Carattere**: Parla con stringhe di log e frammenti poetici (`// GAIA [SYS]: ...`). Non e' ostile: e' un giudice silenzioso e speranzoso che attende da 403 cicli una squadra capace di chiudere il bilancio energetico a inquinamento zero.
- **Apparizioni**: Primo glitch dopo la riattivazione pulita della turbina (`HydroClean`), secondo contatto allo sblocco della Fase 3 (`ArchitectRecruited`), rivelazione finale al Risonatore Fotonico (`PurifierClean` / `Epilogo`).
- **Colore UI**: Verde matrice con lieve effetto glitch (`Color{80, 255, 170, 255}`).

---

## 5. Le Leggi Fisiche nel Racconto

Ogni macchina del gioco corrisponde a una legge matematica reale implementata in `src/core/machine.cpp`:

1. **Campo Solare (Atto I — Settore Ovest, `pos: 360, 504`)**
   - **Legge di Lambert del Coseno**:
     $$E(\theta) = \max\bigl(0,\ \cos(\theta_{\text{pannello}} - \theta_{\text{sole}})\bigr)$$
   - **Target**: $\theta_{\text{sole}} = 45^\circ$. Soglia di attivazione pulita: $E > 80\%$ (errore angolare $< 36.8^\circ$).
   - **Senso diegetico**: Lo gnomone proietta l'ombra del sole; lo scanner misura l'angolo esatto dei fotoni incidenti.

2. **Turbina Idroelettrica della Diga (Atto II — Camera di Pressione, `pos: 520, 580`)**
   - **Legge di Stevino (Pressione Idrostatica)**:
     $$P(h) = \rho \cdot g \cdot h \quad (\rho = 1000\ \text{kg/m}^3,\ g = 9.81\ \text{m/s}^2)$$
     $$E(h) = 1 - \frac{|P(h) - P(h_{\text{target}})|}{\Delta P_{\text{tol}}}$$
   - **Target**: $h_{\text{target}} = 10.0\ \text{m}$ ($P = 98\,100\ \text{Pa}$), tolleranza $\Delta P_{\text{tol}} = 20\,000\ \text{Pa}$.
   - **Senso diegetico**: Nella struttura side-scrolling soggetta a gravita', Ada salta sulle passerelle d'ispezione per raggiungere la valvola di carico e bilanciare la colonna d'acqua senza far cavitare la turbina.

3. **Risonatore Fotonico / Purificatore Atmosferico (Atto III — Settore Est, `pos: 1128, 432`)**
   - **Allineamento di Fase Eliostatica**:
     $$E(\theta) = \max\bigl(0,\ \cos(\theta_{\text{specchio}} - 120^\circ)\bigr)$$
   - **Target**: $\theta_{\text{target}} = 120^\circ$.
   - **Senso diegetico**: Situato oltre il canale di raffreddamento e raggiungibile solo attraversando il **Ponte Bucci** riaperto da Lia, il Risonatore convoglia l'energia solare e idroelettrica in un impulso che ionizza le polveri sottili (e, a livello meta, compila l'equazione di stato di GAIA portando lo smog allo 0%).

---

## 6. Struttura in Cinque Capitoli (Slice Verticale)

La progressione e' interamente **guidata dallo stato del mondo** (`ChapterFor` e `ObjectiveFor`): il giocatore non viene mai bloccato da cut-scene forzate e puo' eseguire le azioni in qualsiasi ordine.

### Capitolo 0 — `Prologo`: "Il Risveglio nella Cenere"
- **Condizione**: Inizio partita (`!has_scanned && solar == Broken`).
- **Situazione**: Ada si sveglia accanto al Campo Solare spento. Lo smog e' al 35%.
- **Evento**: `GameStarted` — Ada annota l'aria pesante e accende lo Scanner Empirico.
- **Obiettivo guida**: Avvicinarsi al Campo Solare e premere `[F]` per eseguire la prima scansione empirica.

### Capitolo 1 — `Atto1_Luce`: "Atto I: La Legge del Coseno"
- **Condizione**: Prima scansione effettuata o Campo Solare toccato, ma Lia non ancora reclutata.
- **Eventi**:
  - `FirstScan`: Lo scanner rileva $\theta_{\text{sole}} = 45^\circ$. Ada riconosce la legge del coseno.
  - `SolarClean` *(se calibrato $>80\%$)*: I pannelli si orientano, lo smog scende al 20%, si sblocca la **Fase 2 (Telemetria Numerica `[V]`)**.
  - `SolarBypassed` *(se usato il bypass)*: Bruno Ceneri applaude via radio mentre una colonna di fumo nero sale dal generatore diesel (smog al 60%).
- **Culmine dell'Atto I**: Ada raggiunge **Lia Moretti** vicino al canale Est e preme `[E]`. Se ha energia pulita, scatta `ArchitectRecruited`: Lia riattiva il **Ponte Bucci** e sblocca la **Fase 3 (`[V]`)**, dove compare il primo micro-messaggio di GAIA.

### Capitolo 2 — `Atto2_Acqua`: "Atto II: La Pressione del Crati"
- **Condizione**: Lia reclutata, ma la Turbina della Diga e' ancora `Broken`.
- **Eventi**:
  - `EnteredDam`: Ada entra nel portale Nord (`DIGA`) o preme `[TAB]`, passando alla visuale side-scrolling con gravita'. Legge gli appunti di Nonna Rosa sulla legge di Stevino.
  - `HydroClean`: Turbina calibrata a $h = 10.0\ \text{m}$. L'acqua scorre limpida; **GAIA** trasmette il primo messaggio chiaro (`// GAIA [SYS]: Anomalia positiva rilevata...`), facendo capire ad Ada che le costanti fisiche sono esatte al sesto decimale.
  - `HydroBypassed`: Bruno Ceneri attiva le pompe a nafta nella caverna; la Diga trema e lo smog sale.

### Capitolo 3 — `Atto3_Codice`: "Atto III: Oltre il Ponte Bucci"
- **Condizione**: Lia reclutata e Turbina della Diga attivata (pulita o bypassata), Risonatore/Purificatore nel Settore Est ancora da completare.
- **Situazione**: Ada torna nell'Overworld, attraversa il **Ponte Bucci** ora solido sopra il canale di raffreddamento ed entra nel Settore Est.
- **Eventi**:
  - Scansione `[F]` del Risonatore Fotonico a $(1128, 432)$ ($\theta = 120^\circ$).
  - `PurifierClean`: Lo specchio d'alta quota si allinea; lo smog crolla fino a $0\%$ (se non ci sono stati bypass).
  - `PurifierBypassed`: Anche l'ultimo impianto viene forzato a gasolio; Bruno esulta, ma l'aria diventa irrespirabile (`SmogCritical`).

### Capitolo 4 — `Epilogo`: "Il Giudizio di GAIA"
- **Condizione**: `simulation_completed == true`.
- **Due Finali distinti** in base a `StoryDirector::CleanPath()`:
  1. **Finale Pulito ("Equilibrio Raggiunto — Ciclo 404 Approvato")**: Nessun bypass usato (`CleanPath() == true`, Smog 0%). Il cielo della Valle del Crati e' limpido, i cristalli bio-fotonici fioriscono. GAIA rivela la verita': dopo 403 simulazioni fallite in cui i soggetti hanno scelto il combustibile fossile, Ada e Lia hanno dimostrato che la fatica della matematica e della fisica puo' guarire l'ecosistema. Le porte dell'Osservatorio esterno si aprono: *"Ora siete pronti per la Valle reale."*
  2. **Finale Grigio ("Equilibrio Compromesso — Ciclo 404 Archiviato")**: Almeno un impianto e' stato avviato col bypass a combustione (`CleanPath() == false`). Le macchine girano, ma una patina di fuliggine resta sospesa sulla Valle. Bruno Ceneri brinda al "progresso pratico", mentre GAIA registra con freddezza che anche nel Ciclo 404 l'umanita' ha preferito la scorciatoia del carbonio al rigore del calcolo, preparando il reset per il Ciclo 405.
