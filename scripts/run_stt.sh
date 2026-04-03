#!/bin/bash

set -e

# ===== PATHS =====
STT_PATH=$(pwd)
GEM5_BIN="$STT_PATH/build/X86/gem5.opt"
CONFIG="$STT_PATH/configs/deprecated/example/se.py"

WORKLOAD_SRC="$STT_PATH/workloads/stt_test_forward.c"
WORKLOAD_BIN="$STT_PATH/workloads/stt_test_forward"

RUNS_DIR="$STT_PATH/runs"
LOG_DIR="$RUNS_DIR/logs"

mkdir -p "$RUNS_DIR" "$LOG_DIR"

# ===== CHECK FILES =====
if [ ! -f "$GEM5_BIN" ]; then
    echo "[ERROR] gem5 binary not found: $GEM5_BIN"
    exit 1
fi

if [ ! -f "$CONFIG" ]; then
    echo "[ERROR] config file not found: $CONFIG"
    exit 1
fi

if [ ! -f "$WORKLOAD_SRC" ]; then
    echo "[ERROR] workload source not found: $WORKLOAD_SRC"
    exit 1
fi

# ===== COMPILE WORKLOAD =====
echo "[INFO] Compiling workload..."
gcc -O2 "$WORKLOAD_SRC" -o "$WORKLOAD_BIN"

if [ $? -ne 0 ]; then
    echo "[ERROR] Workload compilation failed"
    exit 1
fi

echo "[INFO] Built workload: $WORKLOAD_BIN"
file "$WORKLOAD_BIN"
echo

# ===== RUN FUNCTION =====
run_case() {
    local NAME="$1"
    local STT_FLAG="$2"
    local IMPLICIT_FLAG="$3"
    local EXPLICIT_FLAG="$4"
    local FUTURISTIC_FLAG="$5"

    local OUTDIR="$RUNS_DIR/${NAME}_m5out"
    local LOGFILE="$LOG_DIR/${NAME}.out"

    mkdir -p "$OUTDIR"

    echo "[INFO] Running $NAME ..."
    "$GEM5_BIN" \
        --outdir="$OUTDIR" \
        "$CONFIG" \
        --cpu-type=DerivO3CPU \
        --caches \
        --cmd="$WORKLOAD_BIN" \
        --STT="$STT_FLAG" \
        --implicit_channel="$IMPLICIT_FLAG" \
        --explicit_channel="$EXPLICIT_FLAG" \
        --futuristic_model="$FUTURISTIC_FLAG" \
        > "$LOGFILE" 2>&1

    if [ $? -ne 0 ]; then
        echo "[ERROR] gem5 run failed for $NAME"
        exit 1
    fi

    echo "[INFO] Finished $NAME"
    echo "[INFO] gem5 outdir: $OUTDIR"
    echo "[INFO] log file   : $LOGFILE"
    echo
}

# ===== EXPERIMENTS =====

# Baseline
run_case "baseline" 0 0 0 0

# STT + futuristic model
run_case "futuristic_on" 1 1 0 1

echo "[INFO] All runs completed."