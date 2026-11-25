# Configura e compila
cmake -S . -B build
cmake --build build -j8
# Avvia il gioco (Campagna Completa attiva di default)
./build/eco_proto
# Esegui unit test e test headless E2E
ctest --test-dir build --output-on-failure
