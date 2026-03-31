#!/bin/bash

# ===== PATHS =====
STT_PATH=$(pwd)
RISCV_GCC=/data/rv-linux/bin/riscv64-unknown-linux-gnu-gcc

SRC="$STT_PATH/workloads/stt_test_forward.c"
BIN="$STT_PATH/workloads/stt_test_forward"
OUT_DIR="$STT_PATH/results/stt_test_forward"
CONFIG="$STT_PATH/configs/deprecated/example/se.py"

# ===== COMPILE =====
echo "[INFO] Compiling..."
$RISCV_GCC -O2 -static "$SRC" -o "$BIN" || {
    echo "[ERROR] Compilation failed"
    exit 1
}

# ===== RUN =====
mkdir -p "$OUT_DIR"

echo "[INFO] Running gem5..."
$STT_PATH/build/RISCV/gem5.opt \
  --outdir="$OUT_DIR" \
  "$CONFIG" \
  --cpu-type=DerivO3CPU \
  --caches \
  --l2cache \
  --mem-size=4GB \
  --needsTSO=1 \
  --threat_model=Spectre \
  --STT=1 \
  --implicit_channel=1 \
  -c "$BIN"

echo "[INFO] Done. Check $OUT_DIR"