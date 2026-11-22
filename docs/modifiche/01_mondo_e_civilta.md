# Modifica 01 -- Rifondazione del Mondo, della Civilta' e della Bibbia Narrativa

- **Autore**: `@narratore-1` (Worldbuilder & Lore Architect)
- **File modificati / creati**:
  - `docs/STORIA.md` (riscritto integralmente)
  - `docs/VISION.md` (riscritto integralmente)
  - `docs/modifiche/01_mondo_e_civilta.md` (creato)

---

## 1. Obiettivo dell'Intervento

Superare integralmente l'impostazione iniziale da breve dimostrazione e rifondare l'identita' di **Equilibrio** attorno a una civilta' originale, letteraria e coerente con la fisica empirica, eliminando senza residui:
- la precedente ambientazione regionale e tutti i suoi riferimenti geografici reali;
- il precedente lessico artificiale da terminale informatico, prefissi di debug, nomi caricaturati e riferimenti a simulazioni da log tecnico.

## 2. Decisioni Condivise con il Team (Fase 1)

Prima di modificare i documenti, il design e' stato discusso e approvato collegialmente da tutti e 6 i membri del team (`@narratore-1`, `@narratore-2`, `@artista-1`, `@artista-2`, `@ingegnere-1`, `@ingegnere-2`):

1. **Nuova Civilta' e Mondo -- La Conca di Meridia**:
   - Altopiano monumentale di travertino, basalto, bronzo e vetro-giada edificato dai **Meridiani** (astronomi, idraulici e filosofi naturali).
   - **4 Ere Storiche** ricostruibili sul campo tramite 16 reliquie: *Era I -- L'Alba delle Ombre*, *Era II -- Il Secolo delle Acque e del Suono*, *Era III -- L'Inverno di Pece*, *Era IV -- Il Sigillo dell'Armonia*.
   - **Progressione percettiva diegetica** ripensata in chiave geodetica: *Sguardo Empirico* -> *Rilievo Metrico* -> *Trama Geodetica*.
2. **Geografia Espansa (6 Distretti Overworld + 3 Strutture Interne)**:
   - Overworld `36x50` tile diviso in 6 distretti: *Terrazze dell'Alba* (`DawnTerraces`), *Quartiere dei Canali & Ponte degli Archi* (`CanalQuarter`), *Altopiano dei Venti* (`WindPlateau`), *Giardini Sospesi* (`HangingGardens`), *Caldera delle Fumarole* (`FumaroleCaldera`) e *Santuario dell'Armonia* (`HarmonySanctuary`).
   - 3 strutture interne side-scrolling dedicate: *Cisterne Ipogee* (`HypogeanCisterns`), *Torre del Pendolo* (`PendulumTower`) e *Camera Geotermica* (`GeothermalChamber`).
3. **Cinque Leggi Fisiche + Sintesi Finale**:
   - Eliostato Solare (Legge di Lambert, $\theta = 45^\circ$), Turbina Idrostatica (Legge di Stevino, $h = 10.0\ \text{m}$), Pendolo Isochronos (Galileo, $L = 2.00\ \text{m}$), Risonatore Armonico (Lorentziana, $f_0 = 120\ \text{Hz}$, $Q = 3.0$), Scambiatore di Carnot ($T_h = 600\ \text{K}$, $\eta = 50\%$) e Astrolabio Centrale di Meridia.
4. **Nuovo Cast Naturale e Profondo**:
   - **Elena Valli** (protagonista, naturalista e cartografa), **Clara Sereni** (ingegnera dei canali), **Lorenzo Altieri** (archivista delle acque nelle Cisterne alte), **Miriam Ferro** (botanica nei Giardini Sospesi), **Matteo Verrini** (Sovrintendente realista della Compagnia della Pece, incontrabile nella Caldera) e **L'Eco di Meridia** (memoria epigrafica nei cristalli armonici).
5. **Catalogo delle 16 Reliquie e Tre Finali**:
   - 16 reliquie (`I.1`..`IV.4`) divise in 4 ere e 5 forme visive (`Stele`, `Astrolabe`, `Prism`, `Cylinder`, `FossilSeed`), 10 nell'Overworld e 6 nelle 3 strutture interne.
   - Campagna completa in **7 Capitoli** (`Prologo`, `Atto1_Luce`, `Atto2_Acqua`, `Atto3_Pendolo`, `Atto4_Giardini`, `Atto5_Fumarole`, `Epilogo`) con **3 Finali** (*L'Alba di Meridia* pulito con almeno 12/16 reliquie, *La Rinascita della Conca* pulito con meno di 12 reliquie, *Il Prezzo della Fretta* con bypass a fornace).
   - Supporto nativo nella narrazione a **Cronologia dei Dialoghi (`[H]`)** e **Codex dell'Archivio (`[J]`)**.

## 3. Verifiche Effettuate

- Controllo coerenza di tutti gli enum, nomi di distretti, strutture, formule fisiche, 16 reliquie e cast con i contratti congelati da `@ingegnere-1`, `@narratore-2`, `@artista-1`, `@artista-2` e `@ingegnere-2`.
- Controllo assenza totale di riferimenti geografici regionali precedenti e di lessico/prefissi artificiali da script di debug in tutti i documenti redatti.
