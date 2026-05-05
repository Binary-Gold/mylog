#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="${SCRIPT_DIR}/.."
BIN="${SCRIPT_DIR}/build_test_main"

# ---- 确保 libmylog.a 已编译 ----
MYLOG_LIB="${PROJECT_DIR}/build/libmylog.a"
if [ ! -f "${MYLOG_LIB}" ]; then
    echo "[*] Building libmylog.a..."
    cmake -B "${PROJECT_DIR}/build" -DMYLOG_BUILD_TESTS=OFF -S "${PROJECT_DIR}"
    cmake --build "${PROJECT_DIR}/build" -j"$(nproc)"
fi

echo "[*] Compiling test/main.cpp..."
g++ -std=c++20 -O2 -Wall -Wextra \
    -I"${PROJECT_DIR}/include" \
    "${SCRIPT_DIR}/main.cpp" \
    "${MYLOG_LIB}" \
    -lz -pthread \
    -o "${BIN}"

echo "[*] Running..."
echo "----"
"${BIN}"
echo "----"
echo "[*] done"
