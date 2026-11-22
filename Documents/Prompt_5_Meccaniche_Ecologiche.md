# PROMPT 5: Il "Core Loop" Ecologico e la Matematica Empirica

**Contesto:** Dobbiamo tradurre le meccaniche ecologiche nel gameplay fisico.

**Richiesta Operativa:**

1. **Global State:** Crea un file/classe `EcoSystem` globale che tracci due valori chiave: `pollution_level` (inquinamento) e `clean_energy` (energia pulita).  
2. **Classe Machine:** Crea l'entità `Machine` (es. un impianto solare rovinato o una turbina) posizionata nella tilemap. Implementa un "trigger radius" tramite calcolo della distanza: quando uno dei player entra nel raggio, stampa a schermo l'UI prompt "Premi Interagisci per riavviare l'impianto".  
3. **Matematica Empirica Giocata:** All'interazione, si apre un pop-up UI gestito da Raylib. Il player non deve inserire roba a caso, ma "calibrare" fisicamente l'impianto (es. regolare l'inclinazione in gradi del pannello tramite slider).  
4. **Algoritmo di Efficienza:** Scrivi una formula matematica nel codice (es. il rendimento energetico \$E\$ si calcola con il coseno dell'angolo d'errore rispetto al vettore di irraggiamento solare virtuale calcolato per la mappa). Se il risultato supera l'80% di efficienza ottimale, eroga energia pulita.  
5. **Il Dilemma Inquinante:** Nel pop-up dell'interazione aggiungi una seconda opzione rapida: "Usa Generatore di Emergenza a Combustione (Bypass)". Questa opzione eroga energia subito, bypassando i calcoli matematici, ma aumenta il `pollution_level`.  
6. **Feedback Visivo:** Lega la variabile `pollution_level` al rendering del gioco. All'aumentare dell'inquinamento, applica un `ColorTint` globale a tutte le texture (o uno shader base) per far apparire lo schermo progressivamente più cupo, smog-style e saturo di grigio.