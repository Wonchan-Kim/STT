#!/bin/bash
set -euo pipefail
trap 'echo; echo "[ERROR] Interrupted"; exit 130' INT TERM

# ===== PATHS =====
STT_PATH=$(pwd)
GEM5_BIN="$STT_PATH/build/X86/gem5.opt"
CONFIG="$STT_PATH/configs/deprecated/example/se.py"
SPEC_ROOT="$STT_PATH/spec2017/benchspec/CPU"

RUNS_DIR="$STT_PATH/runs"
LOG_DIR="$RUNS_DIR/logs"

mkdir -p "$RUNS_DIR" "$LOG_DIR"

# ===== SETTINGS =====
INST_LIMIT=15000000

# ===== CHECK FILES =====
if [ ! -f "$GEM5_BIN" ]; then
    echo "[ERROR] gem5 binary not found: $GEM5_BIN"
    exit 1
fi

if [ ! -f "$CONFIG" ]; then
    echo "[ERROR] config file not found: $CONFIG"
    exit 1
fi

if [ ! -d "$SPEC_ROOT" ]; then
    echo "[ERROR] SPEC root not found: $SPEC_ROOT"
    exit 1
fi

# ===== BENCHMARK LIST =====
BENCHMARKS=(
  "505.mcf_r|mcf_r|inp.in"
  "519.lbm_r|lbm_r|1000 reference.dat 0 0 100_100_130_cf_a.of"
  "531.deepsjeng_r|deepsjeng_r|ref.txt"
  "557.xz_r|xz_r|cld.tar.xz 160 19 cf30.xz 200 200 4.0 0"
)

# ===== RUN ONE BENCHMARK =====
run_benchmark() {
    local CASE_NAME="$1"
    local STT_FLAG="$2"
    local IMPLICIT_FLAG="$3"
    local EXPLICIT_FLAG="$4"
    local FUTURISTIC_FLAG="$5"
    local BENCH="$6"
    local BIN_NAME="$7"
    local OPTS="$8"

    local RUN_DIR="$SPEC_ROOT/$BENCH/run/run_base_refrate_mytest-static-m64.0000"
    local BIN_PATH="$SPEC_ROOT/$BENCH/build/build_base_mytest-static-m64.0000/$BIN_NAME"
    local OUTDIR="$RUNS_DIR/${CASE_NAME}_${BENCH}_m5out"
    local LOGFILE="$LOG_DIR/${CASE_NAME}_${BENCH}.out"

    if [ ! -d "$RUN_DIR" ]; then
        echo "[WARN] run directory not found for $BENCH, skipping"
        return 0
    fi

    if [ ! -f "$BIN_PATH" ]; then
        echo "[WARN] benchmark binary not found for $BENCH, skipping"
        return 0
    fi

    rm -f "$LOGFILE"
    rm -rf "$OUTDIR"
    mkdir -p "$OUTDIR"

    cd "$RUN_DIR"

    if [ -n "$OPTS" ]; then
        "$GEM5_BIN" \
            --outdir="$OUTDIR" \
            "$CONFIG" \
            --num-cpus=1 \
            --mem-size=4GB \
            --caches \
            --l2cache \
            --cpu-type=DerivO3CPU \
            -I "$INST_LIMIT" \
            -c "$BIN_PATH" \
            -o "$OPTS" \
            --STT="$STT_FLAG" \
            --implicit_channel="$IMPLICIT_FLAG" \
            --explicit_channel="$EXPLICIT_FLAG" \
            --futuristic_model="$FUTURISTIC_FLAG" \
            > "$LOGFILE" 2>&1
    else
        "$GEM5_BIN" \
            --outdir="$OUTDIR" \
            "$CONFIG" \
            --num-cpus=1 \
            --mem-size=4GB \
            --caches \
            --l2cache \
            --cpu-type=DerivO3CPU \
            -I "$INST_LIMIT" \
            -c "$BIN_PATH" \
            --STT="$STT_FLAG" \
            --implicit_channel="$IMPLICIT_FLAG" \
            --explicit_channel="$EXPLICIT_FLAG" \
            --futuristic_model="$FUTURISTIC_FLAG" \
            > "$LOGFILE" 2>&1
    fi

    local rc=$?
    echo "[INFO] gem5 exit code for $CASE_NAME / $BENCH : $rc"

    if [ $rc -ne 0 ]; then
        echo "[ERROR] Failed case=$CASE_NAME benchmark=$BENCH"
        tail -n 80 "$LOGFILE" || true
        exit $rc
    fi

    if ! grep -q "Exiting @ tick" "$LOGFILE"; then
        echo "[ERROR] No normal exit line for case=$CASE_NAME benchmark=$BENCH"
        tail -n 80 "$LOGFILE" || true
        exit 1
    fi

    echo "[INFO] Finished case=$CASE_NAME benchmark=$BENCH"
    cd "$STT_PATH"
}

# ===== RUN ONE CASE =====
run_case() {
    local NAME="$1"
    local STT_FLAG="$2"
    local IMPLICIT_FLAG="$3"
    local EXPLICIT_FLAG="$4"
    local FUTURISTIC_FLAG="$5"

    for entry in "${BENCHMARKS[@]}"; do
        IFS='|' read -r BENCH BIN OPTS <<< "$entry"
        run_benchmark "$NAME" "$STT_FLAG" "$IMPLICIT_FLAG" "$EXPLICIT_FLAG" "$FUTURISTIC_FLAG" \
            "$BENCH" "$BIN" "$OPTS"
    done
}

# ===== EXPERIMENTS =====
echo "[INFO] Running Baseline Case"
run_case "baseline" 0 0 0 0

# echo "[INFO] Running STT only Case"
# run_case "stt_only" 1 0 0 0

echo "[INFO] Running Futuristic Case"
run_case "futuristic_on" 1 1 0 1

echo "[INFO] All runs completed."