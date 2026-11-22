# PROMPT 1: Architettura, Finestra e Game Loop

**Contesto:** Stiamo costruendo un gioco in C++ con Raylib come progetto didattico per l'università. Il Mega Prompt principale è già stato fornito e funge da linea guida.

**Richiesta Operativa:** Scrivi il file `main.cpp` e l'infrastruttura di base (GameStateManager).

1. Inizializza una finestra Raylib (es. 1280x720) a 60 FPS, attivando l'Anti-Aliasing.  
2. Implementa un **Game Loop con Fixed Timestep** (pattern con accumulatore di tempo delta) per garantire che la fisica resti deterministica e invariata indipendentemente dalle oscillazioni del framerate.  
3. Crea un'interfaccia astratta o una classe base `GameState` con metodi virtuali puri per `Update(float dt)` e `Draw()`.  
4. Implementa uno State Manager che gestisca la transizione sicura tra due stati (per ora vuoti): `OverworldState` (per la visuale Top-Down) e `StructureState` (per il Side-Scrolling).  
5. Fornisci un file `CMakeLists.txt` pulito per configurare e compilare agevolmente il progetto (includendo Raylib) su Linux e Windows.