#!/bin/bash
set -euo pipefail

SPEC_ROOT=~/STT/spec2017
CONFIG=gcc-linux-x86.cfg
LABEL=mytest-static-m64.0000
JOBS=8

# Pick the benchmarks you actually want
BENCHMARKS=(
  505.mcf_r
  519.lbm_r
  531.deepsjeng_r
  557.xz_r
)

cd "$SPEC_ROOT"
source ./shrc

build_one() {
    local bench="$1"
    local build_dir="benchspec/CPU/$bench/build/build_base_${LABEL}"

    runcpu --fake --config "$CONFIG" --define static=1 "$bench"

    if [ ! -d "$build_dir" ]; then
        echo "[ERROR] Build directory not found: $build_dir"
        exit 1
    fi

    echo "[INFO] Building $bench in $build_dir"
    cd "$SPEC_ROOT/$build_dir"
    specmake clean
    specmake -j"$JOBS"

    cd "$SPEC_ROOT"
}

for bench in "${BENCHMARKS[@]}"; do
    build_one "$bench"
done

echo
echo "[INFO] All benchmark builds completed."