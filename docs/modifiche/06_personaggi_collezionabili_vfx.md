# Modifiche 06 — Personaggi, Monumenti Fisici, Collezionabili, VFX, Minimappa e Stile UI

**Autore:** `@artista-2`  
**Ambito:** `src/app/render/ui_style.*`, `src/app/render/particles.*`, `src/app/overworld_state.cpp`, `src/app/structure_state.cpp`, `src/app/calibration_ui.cpp`

## 1. Identita' UI della Conca di Meridia (`src/app/render/ui_style.h`, `src/app/render/ui_style.cpp`)
- Riprogettato il linguaggio visivo dei pannelli UI per evocare la civilta' dei Meridiani: ardesia profonda con gradiente verticale, filetto interno in ottone/bronzo, graffe geometriche agli angoli e losanghe ornamentali nei titoli.
- Aggiunti gli helper condivisi concordati con `@ingegnere-2` per Cronologia Dialoghi `[H]`, Codex Reliquie `[J]` e banner diegetici:
  - `DrawSectionCard(...)` per voci di elenco e schede evidenziate;
  - `MeasureKeyBadge(...)` e `DrawKeyBadge(...)` con rilievo metallico;
  - `DrawDivider(...)` con losanga centrale;
  - `DrawTabBar(...)` per la navigazione a schede del Diario;
  - `DrawScrollbar(...)` proporzionale;
  - `DrawRelicGlyph(kind_index, center, radius, accent, discovered, sim_time)` per i 5 glifi vettoriali delle reliquie (`Stele`, `Astrolabio`, `Prisma`, `Cilindro`, `Seme Fossile`);
  - `DrawBannerToast(...)` per ingressi di distretto/struttura e notifiche di raccolta reliquie.

## 2. Protagonista e 4 NPC distinti (`src/app/calibration_ui.cpp`)
- **Elena Valli (e colleghi P2..P4):** ridisegnata come naturalista e cartografa con mantello avorio, fibbia in bronzo, borsa da campo in cuoio con pergamena e gemma della fase percettiva, bussola/lente empirica in ottone e impulso radar geodetico su `[F]`.
- **Clara Sereni** (*Ingegnera dei Canali*, Ponte degli Archi): soprabito blu-ardesia, tubo portaprogetti a tracolla, occhiali da rilievo topografico e rosa dei venti rotante al reclutamento.
- **Lorenzo Altieri** (*Archivista di Meridia*, Cisterne Ipogee): mantello bordeaux/ocra, capelli e barba d'argento, occhiali in ottone e lanterna-astrolabio luminosa.
- **Miriam Ferro** (*Botanica dei Giardini*, Giardini Sospesi): tunica verde salvia, bisaccia di semi, fermaglio botanico e fiala di vetro-giada luminescente.
- **Matteo Verrini** (*Sovrintendente della Pece*, Caldera delle Fumarole): soprabito antracite con bottoni in ottone ambrato e monocolo fumee.

## 3. Tutti i 6 Monumenti Fisici e Modal di Calibrazione (`src/app/calibration_ui.cpp`)
- **Eliostato Solare** (*Ottica - Legge di Lambert*): basamento ottagonale in travertino, raggi solari con fotoni animati, specchio eliostatico orientabile e meridiana/gnomone laterale.
- **Turbina Idrostatica** (*Idrostatica - Legge di Stevino*): cisterna piezometrica graduata `0..20 m`, quota ottimale `h_opt`, condotta forzata in bronzo con flusso animato e rotore a 8 pale.
- **Pendolo Isochronos** (*Meccanica - Isocronismo di Galileo*): arco monumentale con quadrante graduato e pendolo oscillante dal vivo al periodo fisico reale `T = 2*pi*sqrt(L/g)`.
- **Risonatore Armonico** (*Acustica - Risonanza Armonica*): torre in pietra e giada con rebbi vibranti, cristallo centrale sospeso e onde sonore concentriche legate alla frequenza `f`.
- **Scambiatore di Carnot** (*Termodinamica - Ciclo di Carnot*): pozzo esagonale in basalto e rame, condotti caldo/freddo, nucleo termico reattivo a `T_calda`, ago graduato e vapore pulito.
- **Astrolabio Centrale** (*Sintesi - Interferenza di Fase*): grande sfera armillare su piedistallo a gradini circondata dai **4 anelli delle Ere**, ciascuno illuminato in proporzione alle reliquie raccolte nella rispettiva era.
- **Modal di Calibrazione aggiornato:** supporta tutte le 6 leggi fisiche con range/unita' reali (`deg`, `m`, `Hz`, `K`), formula esposta, soglia di rendimento visibile, blocco del bypass per l'Astrolabio Centrale e **6 diagrammi fisici live dedicati** (vettori Lambert, colonna Stevino `kPa`, pendolo oscillante `T` vs `T0`, curva di risonanza Lorentziana `R(f)` con cursore, serbatoi Carnot `Th/Tc` con `eta_C`, e onde interferenti risultanti per l'Astrolabio).

## 4. Collezionabili nel Mondo, Minimappa e HUD (`src/app/calibration_ui.cpp`)
- **16 Reliquie nel mondo di gioco (Overworld + Strutture):** ogni reliquia non raccolta brilla con colore legato alla propria Era, fascio verticale tenue, doppia aureola pulsante, glifo dedicato, 3 scintille orbitali e prompt diegetico `[E] Raccogli <codice> - <titolo>`.
- **Minimappa Overworld (`MAPPA DI MERIDIA`):** posizionata in alto a destra sotto il pannello Lente (`x=1064, y=158, w=200, h=146`), mostra i 6 distretti colorati secondo lo stato ecologico, il canale centrale e lo stato del Ponte degli Archi, i 3 portali delle strutture, i monumenti colorati per stato, le reliquie trovate/da trovare e il cursore pulsante di Elena.
- **HUD Superiore e Trama Geodetica:** barra superiore ripulita con nome del distretto/struttura corrente, fase percettiva (*Sguardo Empirico*, *Rilievo Metrico*, *Trama Geodetica*), barre *Foschia* ed *Energia*, contatore *Reliquie X/16* e *Monumenti X/5*, e badge tasti rapidi (`[F]`, `[E]`, `[H]`, `[J]`, `[V]`, `[ESC]`). Rimossa ogni traccia calabrese o da script (nessun riferimento a Calabria, Crati, Unical, Ponte Bucci, GAIA, AABB, diesel, Nonna Rosa).

## 5. Particelle per Bioma/Struttura, Burst Reliquie e Integrazione State (`particles.*`, `overworld_state.cpp`, `structure_state.cpp`)
- `particles.cpp` differenzia le particelle per ciascuno dei 6 distretti Overworld (lucciole/polline ai Giardini, scie di vento sull'Altopiano, vapore/braci nella Caldera, riflessi dorati su Terrazze e Santuario) e per ciascuna delle 3 strutture interne (nebbia idraulica nelle Cisterne, pulviscolo dorato nei fasci di luce della Torre del Pendolo, vapore e scintille ramate nella Camera Geotermica).
- Aggiunto l'effetto burst concentrico + 14 scintille radiali nel punto di raccolta quando una reliquia viene recuperata (`DrawCollectiblePickupBurst`).
- Aggiornati `overworld_state.cpp` e `structure_state.cpp` per il multi-struttura (`EnterStructure` / `ExitToOverworld`), il clamping della camera sulla mappa 36x50, gli sfondi per struttura tramite `StructurePaletteByIndex` e le nuove firme di `procedural_art.h` e `particles.h`.

## 6. Rifiniture post-ispezione visiva & Verifica Finale
- **Sincronizzazione camera/culling Overworld (`overworld_state.cpp`):** allineato temporaneamente `overworld.camera_target` alla camera clampata durante `DrawOverworldProceduralMap`/`DrawOverworldProceduralProps` (con ripristino immediato), eliminando una striscia vuota sul margine destro di *Terrazze dell'Alba* quando Elena e' vicina al bordo ovest.
- **Contrasto Targhette e Toast (`calibration_ui.cpp`, `ui_style.cpp`):** aggiunta `DrawWorldLabelTag` in ardesia scura con cornice bronzo/accento dietro i nomi dei 6 monumenti e dei 4 NPC per leggibilita' totale anche sui mosaici chiari; potenziato il contrasto di `DrawBannerToast` durante il fade.
- **Build & CTest (`build-art2`):** compilazione completa (`cmake -S . -B build-art2 -G Ninja ... && cmake --build build-art2 -j4`) con **0 errori e 0 warning** (`-std=c++20 -Wall -Wextra -Wpedantic`), `ctest --output-on-failure` **2/2 PASS** (`eco_tests` + `headless_e2e`).
- **Ispezione visiva completata (`read_file`)** su screenshot reali di tutti i monumenti, i 4 NPC (Clara, Lorenzo, Miriam, Matteo), Elena Valli, i collezionabili luminosi, la minimappa `MAPPA DI MERIDIA`, le 3 strutture interne e i modal di calibrazione con diagrammi fisici live (Risonatore Armonico, Pendolo Isochronos, Astrolabio Centrale).
