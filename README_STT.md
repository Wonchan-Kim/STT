# Speculative Taint Tracking (STT) in gem5

## Project Structure
```text
.
├── configs/             # Modified gem5 configs
├── scripts/             # Experiment + plotting scripts
├── runs/                # Simulation outputs (not included in repo)
├── plots/               # Generated figures
└── (other project files)
```

## Build gem5 (X86)

```bash
scons build/X86/gem5.opt -j16
```

## Benchmark Setup (SPEC CPU2017)

This project uses SPEC CPU2017 benchmarks.

A local copy is used in this project directory. Users must have their own SPEC CPU2017 to reproduce the results.

Before building benchmarks, copy and modify the configuration file:

```bash
cp Example-gcc-linux-x86.cfg gcc-linux-x86.cfh
```

The configuration file may need to be edited to match your system environment(e.g., gcc path)

### Expected Directory Structure
```text
.
├── spec2017/               # Local SPEC CPU2017 installation 
├── configs/ 
├── scripts/ 
...
```

## Build Benchmarks

Run:

```bash
./scripts/build_benchmark.sh
```

## Run Simulation

Run:

```bash
./scripts/run_benchmark.sh
```

Simulation ouputs will be stored in `runs/`

## Generate Plots

Run:

```bash
python3 scripts/plots.py
```

Figures will be stored in `plots/`