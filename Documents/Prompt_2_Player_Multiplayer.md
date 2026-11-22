# PROMPT 2: Entità Player e Multiplayer Locale (1-4 Giocatori)

**Contesto:** Il gioco deve supportare il multiplayer cooperativo locale in stile drop-in, come definito dal Mega Prompt.

**Richiesta Operativa:** Implementa l'astrazione della classe o struct `Player`.

1. Il costruttore deve accettare un parametro `int player_id` (da 0 a 3). Questo ID determinerà gli input in ascolto e un colore/tinta a schermo (per distinguerli prima di avere gli sprite).  
2. Scrivi un sistema di input modulare. Il player 0 usa WASD (o ZQSD), il player 1 usa le Frecce Direzionali. Soprattutto, aggiungi il supporto nativo ai **Gamepad tramite Raylib** (`IsGamepadAvailable`, `GetGamepadAxisMovement`) per permettere a tutti e 4 i giocatori di usare i controller.  
3. Nel caso dei controller analogici, implementa una "deadzone" (es. 0.2f) per evitare effetti di drifting accidentale del personaggio.  
4. Crea un manager nel `GameState` che tenga traccia di un `std::vector<Player>` (o array fisso).  
5. Nel metodo `Update(float dt)`, leggi gli input calcolando un vettore 2D di direzione (x, y) ma **NON** applicare ancora nessuno spostamento spaziale. Nel `Draw()`, renderizza semplicemente un rettangolo colorato per player. Fornisci i file `player.h` e `player.cpp`.