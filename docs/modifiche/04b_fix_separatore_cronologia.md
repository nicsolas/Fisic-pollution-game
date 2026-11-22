# 04b - Fix Separatore & Nitidezza Tag Cronologia Dialoghi

**Autore**: `@ingegnere-2`  
**Data**: 22 novembre 2026

## Modifica Applicata
- In `src/app/story_ui.cpp` (intestazione a destra di ogni card nella Cronologia Dialoghi `[H]`): sostituito il separatore sottile `"-"` a dimensione 12px (che in downsampling dal TTF 48px risultava poco nitido) con il separatore verticale ASCII `"|"` (`"#%02d  |  %s  |  %s"`) a dimensione 13px e contrasto leggermente migliorato.

## Verifica
- Build pulita in `build-ing2` (`0 errori`, `0 warning`).
- `ctest --output-on-failure`: `eco_tests` e `headless_e2e` verdi al 100%.
- Screenshot `shot_history.png` ricatturato e ispezionato visivamente.
