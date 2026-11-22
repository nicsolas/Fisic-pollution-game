#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
GAME_BUILD_DIR="${ECO_BUILD_DIR:-${PROJECT_ROOT}/build}"

CONTRACT_CHECK=0
RUNNER_ARGS=()

for arg in "$@"; do
    if [[ "${arg}" == "--contract-check" ]]; then
        CONTRACT_CHECK=1
    else
        RUNNER_ARGS+=("${arg}")
    fi
done

GEN_ARGS=()
if command -v ninja >/dev/null 2>&1; then
    GEN_ARGS=(-G Ninja)
fi
JOBS="$(nproc 2>/dev/null || echo 4)"

if [[ "${CONTRACT_CHECK}" -eq 1 ]]; then
    E2E_BUILD_DIR="${SCRIPT_DIR}/.build_contract"
    cmake "${GEN_ARGS[@]}" \
        -S "${SCRIPT_DIR}" \
        -B "${E2E_BUILD_DIR}" \
        -DE2E_CONTRACT_CHECK_ONLY=ON \
        -DECO_GAME_BUILD_DIR="${GAME_BUILD_DIR}"
    cmake --build "${E2E_BUILD_DIR}" -j"${JOBS}"

    RUNNER_BIN="${E2E_BUILD_DIR}/eco_e2e_contract_check"
    "${RUNNER_BIN}" --list
    "${RUNNER_BIN}" --summary
    echo "[run_e2e.sh] Running CONTRACT-CHECK mode (NOT a live E2E pass)"
    if [[ ${#RUNNER_ARGS[@]} -gt 0 ]]; then
        "${RUNNER_BIN}" --contract-check "${RUNNER_ARGS[@]}"
    else
        "${RUNNER_BIN}" --contract-check
    fi
    exit 0
fi

# Default LIVE mode: fail non-zero if main build artifacts are missing (NEVER fall back to --contract-check)
GAME_CORE_FOUND=0
if [[ -f "${GAME_BUILD_DIR}/libgame_core.a" || -f "${GAME_BUILD_DIR}/src/core/libgame_core.a" ]]; then
    GAME_CORE_FOUND=1
fi

if [[ ! -x "${GAME_BUILD_DIR}/eco_proto" || "${GAME_CORE_FOUND}" -eq 0 ]]; then
    echo "[run_e2e.sh] ERROR: Live build artifacts missing in '${GAME_BUILD_DIR}'." >&2
    echo "[run_e2e.sh] Expected executable '${GAME_BUILD_DIR}/eco_proto' and 'libgame_core.a'." >&2
    echo "[run_e2e.sh] Build main project first (or pass --contract-check for pre-build contract checks)." >&2
    exit 1
fi

E2E_BUILD_DIR="${SCRIPT_DIR}/.build_live"
cmake "${GEN_ARGS[@]}" \
    -S "${SCRIPT_DIR}" \
    -B "${E2E_BUILD_DIR}" \
    -DE2E_CONTRACT_CHECK_ONLY=OFF \
    -DECO_GAME_BUILD_DIR="${GAME_BUILD_DIR}"
cmake --build "${E2E_BUILD_DIR}" -j"${JOBS}"

export E2E_ECO_PROTO_BIN="${GAME_BUILD_DIR}/eco_proto"
RUNNER_BIN="${E2E_BUILD_DIR}/eco_e2e_runner"

"${RUNNER_BIN}" --summary --repo-root "${PROJECT_ROOT}" --build-dir "${GAME_BUILD_DIR}"
if [[ ${#RUNNER_ARGS[@]} -gt 0 ]]; then
    "${RUNNER_BIN}" --repo-root "${PROJECT_ROOT}" --build-dir "${GAME_BUILD_DIR}" "${RUNNER_ARGS[@]}"
else
    "${RUNNER_BIN}" --repo-root "${PROJECT_ROOT}" --build-dir "${GAME_BUILD_DIR}"
fi

echo "[run_e2e.sh] Running '${GAME_BUILD_DIR}/eco_proto' --headless-e2e"
"${GAME_BUILD_DIR}/eco_proto" --headless-e2e

HAS_DISPLAY=0
HAS_WAYLAND=0
if [[ -n "${WAYLAND_DISPLAY:-}" && -n "${XDG_RUNTIME_DIR:-}" && -S "${XDG_RUNTIME_DIR}/${WAYLAND_DISPLAY}" ]]; then
    HAS_WAYLAND=1
fi

X11_SUPPORTED=1
WAYLAND_SUPPORTED=1
CMAKE_CACHE="${GAME_BUILD_DIR}/CMakeCache.txt"
if [[ -f "${CMAKE_CACHE}" ]]; then
    if grep -q '^GLFW_BUILD_X11:BOOL=OFF$' "${CMAKE_CACHE}"; then
        X11_SUPPORTED=0
    fi
    if grep -q '^GLFW_BUILD_WAYLAND:BOOL=OFF$' "${CMAKE_CACHE}"; then
        WAYLAND_SUPPORTED=0
    fi
fi

if [[ "${HAS_WAYLAND}" -eq 1 && "${WAYLAND_SUPPORTED}" -eq 1 ]]; then
    HAS_DISPLAY=1
elif [[ "${X11_SUPPORTED}" -eq 1 && -n "${DISPLAY:-}" ]]; then
    if [[ "${DISPLAY}" == :* ]]; then
        DNUM="${DISPLAY#:}"
        DNUM="${DNUM%%.*}"
        if [[ -n "${DNUM}" && "${DNUM}" =~ ^[0-9]+$ && -S "/tmp/.X11-unix/X${DNUM}" ]]; then
            HAS_DISPLAY=1
        fi
    else
        HAS_DISPLAY=1
    fi
fi

if [[ "${HAS_DISPLAY}" -eq 1 ]]; then
    echo "[run_e2e.sh] Running '${GAME_BUILD_DIR}/eco_proto' --smoke-frames 10"
    "${GAME_BUILD_DIR}/eco_proto" --smoke-frames 10
else
    echo "[run_e2e.sh] Skipping --smoke-frames 10 (no reachable Wayland/X11 display socket)"
fi
