#!/usr/bin/env python3
import re
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# ===== PATHS =====
BASE_DIR = Path(__file__).resolve().parent.parent
RUNS_DIR = BASE_DIR / "runs"

PLOTS_DIR = BASE_DIR / "plots"
PLOTS_DIR.mkdir(exist_ok=True)

# ===== BENCHMARKS =====
BENCHMARKS = [
    ("505.mcf_r", "mcf"),
    ("519.lbm_r", "lbm"),
    ("531.deepsjeng_r", "deepsjeng"),
    ("557.xz_r", "xz"),
]

CASES = ["baseline", "futuristic_on"]

# ===== REGEX =====
CYCLE_RE = re.compile(r"system\.cpu\.numCycles\s+(\d+)")
IPC_RE = re.compile(r"system\.cpu\.ipc\s+([0-9.]+)")
INST_RE = re.compile(r"simInsts\s+(\d+)")


def parse_stats(stats_path):
    text = stats_path.read_text()

    cycles = float(CYCLE_RE.search(text).group(1))
    ipc = float(IPC_RE.search(text).group(1))
    insts = float(INST_RE.search(text).group(1))

    return cycles, ipc, insts


def get_data():
    rows = []

    for full, short in BENCHMARKS:
        for case in CASES:
            stats_path = RUNS_DIR / f"{case}_{full}_m5out" / "stats.txt"

            if not stats_path.exists():
                raise FileNotFoundError(f"Missing {stats_path}")

            cycles, ipc, insts = parse_stats(stats_path)

            rows.append({
                "benchmark": short,
                "full_name": full,  # Benchmark full name
                "case": case,
                "cycles": cycles,
                "ipc": ipc,
                "insts": insts,
            })

    df = pd.DataFrame(rows)
    return df


def compute_normalized(df):

    pivot = df.pivot(
        index="benchmark", 
        columns="case", 
        values="cycles"
    )

    pivot["normalized"] = pivot["futuristic_on"] / pivot["baseline"]

    return pivot.reset_index()


def plot_normalized(pivot_df):
    labels = pivot_df["benchmark"]
    values = pivot_df["normalized"]

    fig, ax = plt.subplots(figsize=(10, 6))

    bar_container = ax.bar(labels, values, width=0.5, alpha=0.7)

    ax.axhline(1.0, color="red", linestyle="--", linewidth=1.5)

    # 🔹 Add labels on bars
    ax.bar_label(bar_container, fmt="%.2f")

    ax.set_xlabel("Benchmark")
    ax.set_ylabel("Normalized Execution Time (Cycles / Baseline)")
    ax.set_title("Normalized STT Performance")

    plt.tight_layout()
    plt.savefig(PLOTS_DIR / "normalized_execution_time.png", dpi=300)

def plot_ipc(df):
    pivot = df.pivot(
        index="benchmark",
        columns="case",
        values="ipc"
    ).reset_index()

    labels = pivot["benchmark"]
    x = np.arange(len(labels))
    width = 0.35

    plt.figure(figsize=(10, 6))

    plt.bar(x - width/2, pivot["baseline"], width=width, label="Baseline")
    plt.bar(x + width/2, pivot["futuristic_on"], width=width, label="Futuristic", alpha=0.7)

    plt.ylabel("IPC")
    plt.xlabel("Benchmark")
    plt.title("IPC Comparison")

    plt.xticks(x, labels)
    plt.legend()
    plt.tight_layout()
    plt.savefig(PLOTS_DIR / "ipc.png", dpi=300)

if __name__ == "__main__":
    df = get_data()
    plot_ipc(df)
    print("==== Raw data ====")
    print(df)
    
    pivot_df = compute_normalized(df)
    print("")
    print("==== Norm data ====")
    print(pivot_df)
    plot_normalized(pivot_df)