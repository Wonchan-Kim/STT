#!/bin/bash

set -e

# ===== PATHS =====
STT_PATH=$(pwd)
RISCV_GCC=/data/rv-linux/bin/riscv64-unknown-linux-gnu-gcc
CONFIG="$STT_PATH/configs/deprecated/example/se.py"
GEM5_BIN="$STT_PATH/build/RISCV/gem5.opt"
WORKLOAD_DIR="$STT_PATH/workloads"
RESULTS_DIR="$STT_PATH/results"

# ===== WORKLOAD LIST =====
WORKLOADS=(
    stt_test_forward
    # Add more workloads later
)

# ===== CHECK FILES =====
if [ ! -f "$CONFIG" ]; then
    echo "[ERROR] Config file not found: $CONFIG"
    exit 1
fi

if [ ! -f "$GEM5_BIN" ]; then
    echo "[ERROR] gem5 binary not found: $GEM5_BIN"
    exit 1
fi

if [ ! -d "$WORKLOAD_DIR" ]; then
    echo "[ERROR] Workload directory not found: $WORKLOAD_DIR"
    exit 1
fi

mkdir -p "$RESULTS_DIR"

# ===== COMPILE FUNCTION =====
compile_workload() {
    local NAME="$1"
    local SRC="$WORKLOAD_DIR/${NAME}.c"
    local BIN="$WORKLOAD_DIR/${NAME}"

    if [ ! -f "$SRC" ]; then
        echo "[ERROR] Source file not found: $SRC"
        exit 1
    fi

    echo "[INFO] Compiling $NAME ..."
    "$RISCV_GCC" -O2 -static "$SRC" -o "$BIN"

    echo "[INFO] Built $BIN"
    file "$BIN"
    echo
}

# ===== RUN FUNCTION =====
run_case() {
    local NAME="$1"
    local MODE_NAME="$2"
    local STT_FLAG="$3"
    local IC_FLAG="$4"

    local BIN="$WORKLOAD_DIR/${NAME}"
    local OUT_DIR="$RESULTS_DIR/${NAME}_${MODE_NAME}"

    mkdir -p "$OUT_DIR"

    echo "[INFO] Running $NAME ($MODE_NAME)"
    "$GEM5_BIN" \
        --outdir="$OUT_DIR" \
        "$CONFIG" \
        --cpu-type=DerivO3CPU \
        --caches \
        --l2cache \
        --mem-size=4GB \
        --needsTSO=1 \
        --threat_model=Spectre \
        --STT="$STT_FLAG" \
        --implicit_channel="$IC_FLAG" \
        -c "$BIN"

    echo "[INFO] Finished $NAME ($MODE_NAME)"
    echo "[INFO] Output dir: $OUT_DIR"
    echo
}

# ===== MAIN LOOP =====
for NAME in "${WORKLOADS[@]}"; do
    compile_workload "$NAME"
    run_case "$NAME" "off" 0 0
    run_case "$NAME" "on" 1 1
done

# ===== SUMMARY =====
echo
echo "[INFO] Quick stats summary"
echo "===================================================="

for NAME in "${WORKLOADS[@]}"; do
    for MODE in off on; do
        STATS_FILE="$RESULTS_DIR/${NAME}_${MODE}/stats.txt"
        echo "[INFO] ${NAME}_${MODE}"
        if [ -f "$STATS_FILE" ]; then
            grep -E "simTicks|simInsts|system.cpu.numCycles|system.cpu.ipc" "$STATS_FILE" || true
        else
            echo "[WARN] stats.txt not found"
        fi
        echo "----------------------------------------------------"
    done
done