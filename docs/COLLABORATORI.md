# Collaboratori e Regole di Lavoro (Squadra a 6 Agenti)

## Ruoli e Proprieta' Esclusiva dei File

| Agente | Ruolo | File di Proprieta' |
|---|---|---|
| **`narratore-1`** | Worldbuilder & Lore Architect | `docs/STORIA.md`, `docs/VISION.md`, `docs/modifiche/01_*.md` |
| **`narratore-2`** | Dialogue, Collectibles & Chronicle Writer | `docs/DIALOGHI.md`, `src/core/story_content.cpp`, `src/core/collectible_content.cpp`, `docs/modifiche/02_*.md` |
| **`ingegnere-1`** | Core Systems, World & Collectibles Architect | `src/core/math_types.h`, `tilemap.*`, `collectible.*`, `machine.*`, `npc.*`, `ecosystem.*`, `story.*`, `simulation.*`, `player.*`, `physics2d.*`, `docs/modifiche/03_*.md` |
| **`ingegnere-2`** | App Integration, History/Codex UI, Controls & QA | `CMakeLists.txt`, `src/core/input_types.h`, `src/app/raylib_input.*`, `story_ui.*`, `title_state.*`, `intro_state.*`, `pause_settings_menu.*`, `main.cpp`, `game_state_manager.cpp`, `tests/*`, `docs/modifiche/04_*.md` |
| **`artista-1`** | World Environments, Biomes & Architecture | `src/app/render/palette.h`, `src/app/render/procedural_art.*`, `docs/ART_DIRECTION.md`, `docs/modifiche/05_*.md` |
| **`artista-2`** | Characters, Monuments, Collectibles, VFX & UI Style | `src/app/render/particles.*`, `src/app/render/ui_style.*`, `src/app/overworld_state.cpp`, `structure_state.cpp`, `calibration_ui.cpp`, `docs/modifiche/06_*.md` |
| **`coordinatore`** | Integrazione, Documentazione Generale & QA Finale | `README.md`, `docs/DISCUSSIONE.md`, `docs/COLLABORATORI.md`, `docs/ROADMAP.md`, `docs/DEVLOG.md`, `docs/modifiche/00_*.md`, `docs/modifiche/07_*.md` |

## Regole Operative
1. **Proprieta' esclusiva**: ogni agente modifica solo i propri file; qualsiasi modifica a interfacce condivise passa prima dal board.
2. **Core puro C++20**: `src/core/` non include mai `raylib.h`. Tutti i testi renderizzati usano puro ASCII 32..126 (`e'`, `piu'`, `civilta'`) per compatibilita' universale.
3. **Zero warning & Test verdi**: build con `-Wall -Wextra -Wpedantic` e `ctest --output-on-failure` sempre verdi.
4. **Tracciabilita' in Markdown**: ogni blocco di modifiche produce un documento `.md` dedicato in `docs/modifiche/`.
5. **Verifica visiva obbligatoria**: ogni modifica grafica o UI e' verificata ispezionando screenshot reali generati da `eco_proto`.
