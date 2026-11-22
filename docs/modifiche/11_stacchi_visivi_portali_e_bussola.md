# 11 — Stacchi visivi, portali di distretto e bussola obiettivo

Autore: @artista-zone (in coppia con @ingegnere-zone, vedi `10_divisione_zone_memoria_e_guida.md`).

## Stacchi visivi
- `palette.h`: i 6 distretti ricolorati in modo molto piu' distinto (Terrazze oro/travertino, Canali blu-pietra e acqua zaffiro, Altopiano ardesia fredda, Giardini smeraldo/fioriti, Caldera basalto/brace, Santuario avorio/turchese). Nuovo campo `atmosphere_tint` per distretto.
- `particles.cpp`: tinta atmosferica di distretto (gradiente alto/basso) interpolata con smorzamento esponenziale al cambio zona (stato statico, zero allocazioni).
- `procedural_art.cpp`: `DrawDistrictGateway` disegna archi monumentali sui 5 varchi (`overworld.gateways`): pilastri scolpiti con fasce in bronzo, lanterne col colore dei distretti collegati, architrave a gradiente, stele direzionali (`N -> CANALI`, `E -> GIARDINI`, `S -> SANTUARIO`...) sul lato da cui si parte, ritorno sul lato opposto. Ponte degli Archi in ambra finche' chiuso. I prop non vengono mai piazzati vicino ai varchi.

## Memoria / rendering
- Prop statici dell'Overworld calcolati una sola volta (`RebuildPropCacheIfNeeded`) e poi solo filtrati per viewport (prima: scansione mappa + controlli distanza ogni frame).
- Strutture: tile disegnate solo nella viewport; posizione della stele d'uscita in cache per struttura (niente `FindTiles()` con `std::vector` per frame).
- Particelle delle strutture scartate fuori viewport; nessuna allocazione dinamica nel loop.

## Guida al giocatore (`calibration_ui.cpp`)
- Bussola bronzo/giada in basso: ago che ruota verso la meta, `-> Nome (N m)` (1 m = mezzo tile), freccia + distanza sul bordo schermo quando la meta e' fuori vista, anello/freccia sopra la meta se inquadrata. Usa `world.CurrentObjectiveTarget()`.
- `MAPPA DI MERIDIA`: contorno giada del distretto attivo, varchi come archetti in bronzo (ambra se Ponte chiuso), faro pulsante sull'obiettivo.
