#!/usr/bin/env bash
set -euo pipefail

# Configura e compila in Release (con parallelismo automatico)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
# Esegui unit test e test headless E2E
ctest --test-dir build --output-on-failure
# Avvia il gioco (Campagna Completa attiva di default)
./build/eco_proto "$@"
