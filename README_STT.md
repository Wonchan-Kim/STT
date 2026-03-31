# Speculative Taint Tracking (STT) in gem5

## Project Structure
```text
.
├── configs/             # Modified gem5 configs
├── workloads/           # Benchmark programs
├── scripts/             # Experiment scripts
├── results/             # Output (not included in repo)
└── (other project files)
```

## Build
### 1. Load environment (ARCH-750 server)
```bash
source /data/.local/modules/init/zsh
module load rv-newlib
```

### Build gem5 (RISCV)
```bash
scons build/RISCV/gem5.opt -j16
```

## Running the Experiments

All experiment scripts must be run from the **root directory**.

**Note:** If you encounter a "Permission denied" error, run the following command first:
```bash
chmod +x *.sh
```

```bash
./scripts/run_stt_benchmarks.sh
```
