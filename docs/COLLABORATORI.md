# Collaboratori e regole di lavoro

## Ruoli e proprieta' dei file
| Ruolo | Possiede |
|---|---|
| **narratore** | `docs/STORIA.md`, `docs/DIALOGHI.md`, `src/core/story_content.cpp` |
| **ingegnere** | `src/core/story.cpp`, `src/core/simulation.*`, `src/core/npc.cpp`, `src/core/input_types.h`, `src/app/raylib_input.cpp`, `src/app/title_state.*`, `src/app/intro_state.*`, `src/app/story_ui.*`, `src/app/main.cpp`, `tests/test_main.cpp`, `CMakeLists.txt` |
| **artista** | `src/app/render/*`, `src/app/overworld_state.cpp`, `src/app/structure_state.cpp`, `src/app/calibration_ui.cpp`, `docs/ART_DIRECTION.md` |
| **coordinatore** | README, VISION, COLLABORATORI, ROADMAP, DISCUSSIONE, integrazione finale |

## Regole
1. Modifica solo i tuoi file. Se serve un cambio altrui, chiedilo sul board.
2. Ogni salvataggio deve compilare. Se la build si rompe per un file altrui, segnalalo sul board.
3. Ognuno compila nella PROPRIA cartella di build:
   `cmake -S . -B build-<nome> -G Ninja -DFETCHCONTENT_SOURCE_DIR_RAYLIB="$PWD/build/_deps/raylib-src" && cmake --build build-<nome> -j4 && (cd build-<nome> && ctest --output-on-failure)`
4. Ogni passo concluso va in `docs/DEVLOG.md` (append, con nome e cosa).
5. `src/core` non include mai raylib. Niente lettere accentate nei testi mostrati (font di default raylib): usa "e'".
6. I dialoghi non bloccano mai movimento/interazioni.
7. Verifica visiva: `cd docs/screenshots && ../../build-<nome>/eco_proto --skip-title --smoke-frames 90 --screenshot nome.png` (flag: `--pollution N`, `--scene title|overworld|structure`).

## Definizione di "fatto"
Build senza warning nuovi, ctest verde, screenshot guardato, nota nel DEVLOG.
