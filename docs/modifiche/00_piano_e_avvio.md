# 00 — Piano di Espansione Oltre la Demo e Avvio Squadra (6 Agenti)

**Data**: 2026-11-22  
**Autore**: Coordinatore

## Obiettivi della Milestone
1. **Storia piu' lunga e complessa (Oltre la Demo)**: ricostruire la storia di una civilta' originale, abbandonando completamente l'ambientazione calabrese (Calabria, Valle del Crati, Unical, Ponte Bucci) e i nomi/toni da script tecnico (`// GAIA [SYS]`, `AABB`, `Ciclo 404`, `Consorzio Brace`, ecc.).
2. **Cronologia dei Dialoghi (Dialogue History)**: registrare tutte le battute vissute durante la campagna e renderle consultabili in-game in un diario scorribile.
3. **Molte piu' Zone e Dettagli Ambientali**: espandere l'Overworld in piu' distretti distinti e introdurre piu' strutture interne/sotterranee esplorabili con architettura, flora, acqua e landmark dedicati.
4. **Sistema Collezionabili**: spargere reliquie/frammenti della civilta' perduta in tutte le zone, con raccolta in-game e Archivio/Codex consultabile.
5. **Tracciabilita' in Markdown**: creare un file `.md` dedicato in `docs/modifiche/` per ogni blocco di modifiche apportato dal team.

## Squadra Attivata (6 Agenti)
- **`narratore-1`**: Worldbuilding, storia della civilta', geografia delle zone, personaggi (`docs/STORIA.md`, `docs/VISION.md`, `docs/modifiche/01_*.md`)
- **`narratore-2`**: Dialoghi letterari, testi collezionabili, obiettivi, intro ed epiloghi (`docs/DIALOGHI.md`, `src/core/story_content.cpp`, `src/core/collectible_content.cpp`, `docs/modifiche/02_*.md`)
- **`ingegnere-1`**: Motore core C++20, multi-zona/multi-struttura, collezionabili, Dialogue History in `StoryDirector`, macchine fisiche (`src/core/*`, `docs/modifiche/03_*.md`)
- **`ingegnere-2`**: Integrazione app, overlay Cronologia Dialoghi & Codex, input, schermate titolo/intro/pausa, CMake e test (`CMakeLists.txt`, `src/app/story_ui.*`, `tests/*`, `docs/modifiche/04_*.md`)
- **`artista-1`**: Palette multi-bioma, arte procedurale 2.5D di distretti, rovine, architetture e dettagli ambientali (`src/app/render/palette.h`, `src/app/render/procedural_art.*`, `docs/ART_DIRECTION.md`, `docs/modifiche/05_*.md`)
- **`artista-2`**: Personaggi, monumenti/macchine, collezionabili animati, minimappa, particelle VFX e stile UI (`src/app/render/particles.*`, `src/app/render/ui_style.*`, `src/app/overworld_state.cpp`, `src/app/structure_state.cpp`, `src/app/calibration_ui.cpp`, `docs/modifiche/06_*.md`)

## Fasi Operative
1. **Fase 1 — Discussione Collegiale sul Board**: i 6 agenti discutono civilta', zone, leggi fisiche, personaggi, collezionabili, controlli e contratti C++.
2. **Fase 2 — Contratti Core & Design Narrativo/Visivo**: `ingegnere-1` congela gli header core; `narratore-1` finalizza la bibbia della civilta'; `artista-1`/`artista-2` preparano palette e stile.
3. **Fase 3 — Implementazione Parallela**: tutti e 6 implementano sui propri file esclusivi, compilando ciascuno nella propria cartella `build-<nome>`.
4. **Fase 4 — Integrazione, Test, Verifica Visiva e Documentazione Finale**: `ctest` 100% verde, screenshot ispezionati di tutte le zone/UI, aggiornamento `README.md` e `docs/DISCUSSIONE.md`.
