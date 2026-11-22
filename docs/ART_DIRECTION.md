# ART DIRECTION — EQUILIBRIO: Le Leggi della Valle

## 1. Visione Visiva
La Valle del Crati nel 2091 non deve sembrare un foglio di carta millimetrata con rettangoli colorati, ma un **paesaggio eco-industriale ferito che rinasce**. Poiché tutta la grafica è **procedurale via codice Raylib** (senza texture esterne), costruiamo profondità, atmosfera e leggibilità tramite:
- **Luce direzionale coerente** dall'alto a sinistra (nord-ovest): ogni volume ha una faccia superiore illuminata, una faccia frontale in ombra propria (sporgenza pseudo-3D di ~12 px) e un'ombra portata morbida a terra verso sud-est.
- **Transizione cromatica continua ("Valle Ferita" ↔ "Valle Guarita")**: guidata da `EcoSystem::PollutionRatio()` tramite `pal::WorldColor(sick, healthy, poll)`.
- **Variazione deterministica per tile**: una funzione hash pura `Hash2(col, row, seed)` genera ciottoli, crepe, fili d'erba, muschio e fiori senza alcuno stato mutabile e senza cali di FPS.
- **Separazione netta tra Mondo e Interfaccia**: rimuoviamo le scritte di debug fluttuanti nel mondo ("PIATTAFORME DI ISPEZIONE IDROSTATICA...", ecc.), sostituendole con cartelli diegetici compatti e icone visive chiare.

---

## 2. Palette Cromatica (`src/app/render/palette.h`)

| Elemento | Valle Ferita (`poll = 1.0`) | Valle Guarita (`poll = 0.0`) | Note |
|---|---|---|---|
| **Cielo / Sfondo** | Ocra/Cenere scuro `(54, 42, 34)` | Blu notte/Crepuscolo sereno `(20, 44, 62)` | Usato per `ClearBackground` e nebbia |
| **Terreno Ovest (Campus)** | Terra arida/ruggine `(82, 70, 48)` | Prato mediterraneo `(46, 108, 64)` | Con variazioni ±8 RGB per tile |
| **Terreno Est (Oltre Ponte)** | Sabbia sulfurea `(76, 64, 52)` | Giardino bio-tecnologico `(38, 116, 82)` | Fiori turchesi/gialli quando `poll < 0.35` |
| **Rovine / Muri (Top)** | Cemento fuligginoso `(92, 86, 80)` | Pietra chiara e muschio `(116, 134, 138)` | Faccia superiore |
| **Rovine / Muri (Fronte)** | Ombra ruggine `(52, 46, 44)` | Ardesia profonda `(58, 72, 82)` | Faccia frontale pseudo-3D (12 px) |
| **Canale del Crati (Acqua)** | Fango acido `(68, 74, 52)` | Acqua cristallina `(36, 148, 196)` | Onde sinusoidali + riflessi bianchi/ciano |
| **Ponte Bucci** | Travatura crollata + barriera laser rossa | Passerella reticolare Unical illuminata | Omaggio al cubo/ponte dell'Unical |
| **Accenti UI & Macchine** | Ambra/Ruggine `(235, 135, 45)` (Bypass/Smog) | Turchese/Smeraldo `(65, 240, 190)` (Pulito) | Blu ciano `(80, 195, 245)` per telemetria |

---

## 3. Regole di Profondità, Tilemap e Decorazioni (`src/app/render/procedural_art.*`)

1. **Terreno (`TileId::Walkable`)**:
   - Nessuna griglia artificiale su ogni tile. I bordi sono morbidi; `Hash2(c, r, 1)` sceglie micro-dettagli: ciottoli sparsi, macchie di terra, ciuffi d'erba (più alti e verdi al diminuire dello smog) e piccoli fiori quando la salute supera il 60%.
   - Un sentiero lastricato collega visivamente la piazza di spawn, il Campo Solare, la postazione di Lia (Architetta), il Ponte Bucci, l'ingresso della Diga e il Purificatore/Risonatore Est.
2. **Muri e Rovine (`TileId::Wall`)**:
   - Il canale centrale (`col == 20`) è il **Canale del Crati**: sponde in pietra viva, corrente animata con linee d'onda e schiuma.
   - I muri perimetrali e i pilastri interni sono disegnati con **rilievo 2.5D**: base scura nei 12 px inferiori, piano superiore più chiaro, crepe procedurali e rampicanti/muschio che crescono quando `poll` scende.
3. **Elementi Architettonici Iconici**:
   - **Ponte Bucci (`TileId::HiddenBridge`)**: quando bloccato, mostra piloni spezzati sul canale e due fasci laser rossi pulsanti; quando sbloccato da Lia, disegna la caratteristica travatura reticolare metallica sospesa con luci guida ciano.
   - **Ingresso Diga (`TileId::StructurePortal`)**: portale in cemento armato incassato nella roccia con paratoia idraulica, spruzzi alla base e cartello diegetico `[DIGA]`.
4. **Flora Procedurale e Alberi Decorativi**:
   - Oltre ai nodi di Bio-Cristallo, posizioniamo alberi e cespugli procedurali (tronco + chioma a strati che passa da rami secchi/cenere a chioma frondosa verde-azzurra) solo in coordinate sicure lontane da spawn, macchine, portali e NPC.
5. **Diga del Crati (`StructureState`, Side-Scrolling)**:
   - Sfondo stratificato in parallasse: grandi arcate in cemento della diga, condotte forzate verticali con acqua che scorre dietro, griglia tecnica attenuata.
   - Piattaforme metalliche con travi a traliccio, rivetti/bulloni agli angoli e striscia antiscivolo superiore.
   - Cartelli diegetici compatti (`[USCITA]`, frecce direzionali) al posto delle lunghe frasi di tutorial nel mondo.

---

## 4. Personaggi e Macchine

- **Ada Ferri e Squadra (1-4 Giocatori)**:
  - Correggiamo il bug per cui solo P0 veniva disegnato: iteriamo su tutti i `players.ActivePlayers()`.
  - Ogni `player_id` (0..3) ha un colore d'accento distintivo su sciarpa/visore/zaino:
    - P0 (Ada): Ciano scientifico `(55, 220, 245)`
    - P1: Ambra solare `(255, 185, 55)`
    - P2: Verde smeraldo `(80, 235, 145)`
    - P3: Corallo/Magenta `(245, 105, 165)`
  - Silhouette riconoscibile: ombra a terra, stivali animati con il passo, camice da laboratorio con pieghe e bordo colorato, zaino telemetrico con LED di fase UI, testa con capelli (treccia/caschetto) e occhiali/visore luminoso orientato nella direzione del movimento.
- **Lia Moretti, l'Architetta**:
  - Silhouette distinta da Ada: giacca tecnica blu scuro/ocra, casco giallo da cantiere, tavolo/leggio olografico o tubo portaprogetti, e indicatore `!` flottante sopra la testa quando non è ancora reclutata (che diventa un'icona ponte/matrice attiva dopo il reclutamento).
- **Tre Macchine Distinte**:
  - **Campo Solare (Ovest)**: basamento ottagonale in pietra e acciaio, pannello fotovoltaico a celle blu con riflesso speculare orientato secondo la normale, gnomone/meridiana empirica a fianco; se in bypass, scarico laterale che sbuffa fumo nero.
  - **Turbina Idroelettrica (Diga)**: serbatoio graduato in vetro blindato con colonna d'acqua dinamica e tacca `h_opt`, condotta forzata bullonata, cassa a chiocciola con rotore a pale curve che accelera con l'efficienza.
  - **Torre di Purificazione / Risonatore Atmosferico (Est)**: non più un clone piatto del pannello solare, ma una **torre di filtraggio e risonanza** con anelli concentrici controrotanti, nucleo energetico centrale e bocchette di aspirazione dello smog.

---

## 5. Particelle Cosmetiche e Atmosfera (`src/app/render/particles.*`)

- Pool fisso pre-allocato (`constexpr int kMaxParticles = 96`), zero allocazioni dinamiche per frame, aggiornato in sola lettura rispetto alla simulazione:
  - **Inquinamento alto (`poll > 0.35`)**: fiocchi di cenere grigio-ocra che cadono obliquamente e foschia/smog stratificata.
  - **Valle in guarigione (`poll < 0.65`)**: spore/polline dorato e lucciole turchesi che fluttuano dolcemente verso l'alto nell'Overworld.
  - **Diga (`StructureState`)**: goccioline di condensa e spruzzi d'acqua nebulizzata vicino alla turbina e sul fondo.
- **Post-processing leggero in screen-space**:
  - Raggi di sole obliqui morbidi (god-rays) nell'Overworld la cui intensità e calore crescono con la salute della valle.
  - Vignettatura perimetrale morbida per dare profondità cinematografica senza scurire l'HUD.

---

## 6. Layout HUD e Coordinamento con `DrawStoryLayer`

Per evitare qualsiasi sovrapposizione con `story_ui.cpp` (dell'ingegnere):
- **Barra Superiore Compatta (altezza `48 px`, `y = 0..48`)**:
  - Sinistra: Nome settore + indicatore compatto Fase UI (`[V] Fase 1/2/3`).
  - Centro: Barra **Smog Atmosferico** con percentuale e Barra **Energia Pulita / Sporca**.
  - Destra: promemoria sintetico dei tasti principali (`[F] Scan  [E] Azione  [V] UI  [N] Dialogo`).
- **Angolo Alto-Sinistra (`x = 16, y = 58`)**: **LASCIATO LIBERO** per il pannello Capitolo + Obiettivo di `DrawStoryLayer`.
- **Angolo Alto-Destra (`x = kWindowWidth - 360, y = 58`)**: Pannello compatto **Scanner & Telemetria** (compare quando lo scanner è attivo o in Fase 2/3, così non collide mai con l'obiettivo a sinistra).
- **Fascia Inferiore (`y = kWindowHeight - 130 .. kWindowHeight`)**: **LASCIATA LIBERA** per il box dialoghi a macchina da scrivere di `DrawStoryLayer` (rimossa la vecchia barra `architect_npc.last_dialogue` e rimosso il vecchio banner di vittoria centrale, ora gestiti da `story_ui`).
- **Modal di Calibrazione**: centrato con sfondo oscurato, ridisegnato con `DrawPanel` e `DrawTextShadow`, mantenendo **identici** tasti, slider, pulsanti e gestione dei click mouse.


---

## 9. Espansione Visiva v0.3.0 (Atti IV e V: Nonna Rosa, Scambiatore Geotermico e Nucleo GAIA)

Quando la partita gira in modalita' **Campagna Completa (Atti I-V)** (`!world.demo_mode`), il renderer procedurale aggiunge tre nuovi elementi diegetici:
1. **Nonna Rosa (`DrawNonnaRosaNpc`)** sulla piattaforma superiore destra della Diga del Crati (`{1056, 240}`): capelli bianchi/argento raccolti in uno chignon, occhiali tondi azzurri, cappotto tecnico blu idraulico con colletto ambra e tablet idrometrico.
2. **Scambiatore Geotermico di Carnot (`DrawGeothermalApparatus`)** nel quadrante Nord-Est oltre il Ponte Bucci (`{1128, 216}`): pozzo geotermico circolare con anelli in rame/ottone, nucleo termico pulsante, valvola rotante orientata rispetto al gradiente ottimale (`60 deg`) e vapore pulito (o fumo denso in caso di bypass).
3. **Terminale Sorgente di GAIA (`DrawGaiaSourceTerminal`)** nel quadrante Sud-Est (`{1128, 648}`): monolite esagonale in ossidiana con anelli olografici orbitanti e schermo a onda sinusoidale verde matrice, collegato da condotte energetiche al Risonatore Fotonico.
