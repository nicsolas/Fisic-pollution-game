# Riassunto Progetto Videogioco: Simulazione e Matematica (C++)

## 1. Concept Principale
Un gioco 2D in stile Immersive Sim basato sulla fisica e sulla matematica. Il protagonista è uno scienziato in un mondo apparentemente normale che, attraverso il calcolo e l'osservazione, si rende progressivamente conto di vivere all'interno di un videogioco (una simulazione). Il mondo non si può "hackerare" modificando magicamente le variabili, ma si può **decifrare** e manipolare sfruttando le rigide leggi matematiche del motore grafico.

## 2. Meccaniche di Gioco (Gameplay)
*   **Decifrazione e Calcolo:** Il giocatore deve calcolare le forze in gioco. Se una roccia ha un punto di rottura `x`, il giocatore deve costruire uno strumento o calcolare un impatto (es. massa per velocità) che generi una forza `>x`.
*   **Strumenti Empirici:** Si parte senza informazioni. Il giocatore deve costruire strumenti in-game per misurare massa, attrito, resistenza e velocità degli oggetti e dei nemici.
*   **Progressione dell'UI:** 
    *   *Fase 1:* Nessuna interfaccia (realismo totale).
    *   *Fase 2:* Iniziano a comparire numeri nudi e crudi a schermo mentre si analizzano i fenomeni.
    *   *Fase 3:* Sovrapposizione del motore di gioco (si vedono vettori di movimento, hitbox, coni di visione).
*   **Filosofia alla "Goat Simulator":** I bug logici del codice (es. errori nei cicli o di arrotondamento) non vengono corretti, ma diventano "anomalie quantistiche" o caratteristiche fisiche del mondo che il giocatore può scoprire e sfruttare.

## 3. Gestione degli NPC (Il Network)
*   **Reclutamento Matematico:** Gli altri personaggi non si convincono con il carisma, ma con dimostrazioni empiriche (es. mostrando loro un errore di calcolo nel raffreddamento di un metallo).
*   **Fazione dei Consapevoli:** Gli NPC reclutati mettono a disposizione le loro specializzazioni (es. architetti per vedere le strutture invisibili, balistici per calcolare traiettorie complesse) aumentando la potenza di calcolo della squadra.

## 4. Dettagli Tecnici e Sviluppo
*   **Linguaggio:** C++ (ideale per imparare la gestione della memoria, i puntatori e l'architettura base del PC).
*   **Motore Grafico / Librerie:** Niente motori enormi. Verrà usata una libreria snella per il 2D come **Raylib** o **SFML**.
*   **Grafica:** 2D, con asset e personaggi disegnati da un amico.
*   **Distribuzione:** Open Source (su GitHub), free-to-play, sviluppato principalmente come progetto didattico personale e portfolio.
*   **Team & Tempi:** Sviluppatore Solista (studente). Impegno previsto: ~1/1.5 ore al giorno.
*   **Roadmap Iniziale:** Partire dal terminale. Scrivere prima funzioni matematiche testuali (es. input/output su forze e pesi) per poi passare al rendering del primo quadrato 2D a schermo.