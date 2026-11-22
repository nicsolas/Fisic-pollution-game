# PROMPT 4: Fisica Side-Scrolling e Gravità

**Contesto:** Stiamo sviluppando lo stato `StructureState`, usato per i puzzle interni ai macchinari in cui si richiede la gravità (Platformer 2D).

**Richiesta Operativa:**

1. **Fisica Platform:** Aggiungi alla classe `Player` le variabili specifiche per la fisica verticale: `y_velocity`, `gravity_accel`, `jump_force` e uno stato booleano `isGrounded`. Disabilita l'input sull'asse Y in questo stato, e applica invece la costante accelerazione di gravità alla velocità Y.  
2. **Collisioni Verticali Avanzate:** Riadatta le collisioni AABB. Assicurati di gestire il sub-stepping per impedire che una caduta a velocità elevata faccia trapassare i blocchi del pavimento. Quando il player atterra su un blocco, azzera la sua `y_velocity` e imposta `isGrounded = true`.  
3. **Game Feel (Coyote Time & Jump Buffer):** Non fare un salto banale e rigido.  
   - Implementa il **Coyote Time**: concedi al giocatore una tolleranza di \~0.15 secondi in cui può ancora saltare subito dopo essere sceso (o caduto) da una sporgenza.  
   - Implementa il **Jump Buffer**: se il giocatore preme il tasto salto pochi frame prima di toccare il suolo, registra l'input e fallo saltare non appena tocca terra.  
4. **Transizione di Stato:** Aggiungi la logica di transizione nel GameStateManager: se in Top-Down un giocatore tocca un TileID speciale (es. l'ingresso di una fabbrica), il gioco cambia stato in Side-Scrolling posizionando i giocatori all'ingresso della nuova mappa.