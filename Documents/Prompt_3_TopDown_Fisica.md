# PROMPT 3: Fisica Top-Down, Mappa e Camera

**Contesto:** Stiamo sviluppando lo stato `OverworldState`, ovvero l'esplorazione larga con visuale dall'alto stile vecchi Zelda.

**Richiesta Operativa:**

1. **Movimento:** Nel metodo di Update, applica il vettore di input alla posizione. **Normalizza il vettore spaziale** per evitare il classico bug in cui il movimento diagonale risulta più veloce di quello sugli assi (Pitatura).  
2. **Tilemap:** Implementa una classe `Tilemap` basata su una griglia 2D di interi (es. `std::vector<std::vector<int>>`). Crea una piccola mappa hardcoded d'esempio dove `1` rappresenta un muro invalicabile e `0` lo spazio camminabile.  
3. **Collisioni Sliding:** Scrivi un algoritmo di collisione AABB custom. Nessuna libreria esterna. Quando il player urta un tile "muro", non bloccare completamente il suo movimento spaziale: annulla solo la velocità sull'asse interessato (X o Y) permettendogli di "scivolare" (slide collision) lungo le pareti. Questo migliora drasticamente il game feel.  
4. **Camera:** Usa `Camera2D` di Raylib. Fai in modo che il target della telecamera si centri automaticamente sulla **posizione media** (il punto medio) di tutti i giocatori attivi nella mappa, con un leggero lerp (interpolazione lineare) per rendere il movimento della telecamera fluido.