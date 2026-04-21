#!/usr/bin/env python3
"""
Generate professional report graphs from pool metrics.
Produces PNG charts suitable for the CA2 project report.

Usage:
    python3 scripts/generate_report_graphs.py
"""

import json
import csv
import os
import sys
from pathlib import Path

try:
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    from matplotlib.patches import FancyBboxPatch
except ImportError:
    print("ERROR: matplotlib not installed. Run: pip install matplotlib")
    sys.exit(1)

ROOT       = Path(__file__).resolve().parents[1]
METRICS    = ROOT / "build" / "metrics"
OUT_DIR    = ROOT / "report" / "graphs"
OUT_DIR.mkdir(parents=True, exist_ok=True)

# ── Style ──
plt.rcParams.update({
    'figure.dpi': 120,
    'savefig.dpi': 150,
    'font.family': 'DejaVu Sans',
    'font.size': 11,
    'axes.edgecolor': '#30363d',
    'axes.labelcolor': '#c9d1d9',
    'xtick.color': '#8b949e',
    'ytick.color': '#8b949e',
    'axes.grid': True,
    'grid.color': '#21262d',
    'grid.linestyle': '--',
    'grid.alpha': 0.6,
    'figure.facecolor': '#0d1117',
    'axes.facecolor': '#161b22',
    'savefig.facecolor': '#0d1117',
    'axes.titlecolor': '#58a6ff',
    'axes.titleweight': 'bold',
})

COLORS = {
    'blue':   '#58a6ff',
    'green':  '#3fb950',
    'yellow': '#d29922',
    'red':    '#f85149',
    'purple': '#bc8cff',
    'cyan':   '#39c5cf',
    'orange': '#ffa657',
}

def load_history():
    """Load time-series CSV."""
    csv_path = METRICS / "time_series.csv"
    if not csv_path.exists():
        return None
    with open(csv_path) as f:
        reader = csv.DictReader(f)
        return list(reader)

def load_json(name):
    path = METRICS / name
    if not path.exists():
        return None
    with open(path) as f:
        return json.load(f)

# ── Graph 1: Throughput over time ──
def graph_throughput(data):
    if not data: return
    t = [float(r['elapsed_sec']) for r in data]
    thr = [float(r['throughput']) for r in data]

    fig, ax = plt.subplots(figsize=(10, 5))
    ax.plot(t, thr, color=COLORS['blue'], linewidth=2.5, marker='o', markersize=3)
    ax.fill_between(t, thr, alpha=0.2, color=COLORS['blue'])
    ax.set_title('Thread Pool Throughput Over Time', fontsize=14, pad=15)
    ax.set_xlabel('Elapsed Time (seconds)')
    ax.set_ylabel('Throughput (tasks/second)')
    fig.tight_layout()
    fig.savefig(OUT_DIR / "01_throughput.png")
    plt.close(fig)
    print(f"  ✓ 01_throughput.png")

# ── Graph 2: Queue depth + active threads (dual axis) ──
def graph_queue_threads(data):
    if not data: return
    t = [float(r['elapsed_sec']) for r in data]
    q = [int(r['queue_depth']) for r in data]
    a = [int(r['active_threads']) for r in data]
    live = [int(r['live_threads']) for r in data]

    fig, ax1 = plt.subplots(figsize=(10, 5))
    ax1.plot(t, q, color=COLORS['red'], linewidth=2.2, label='Queue Depth')
    ax1.fill_between(t, q, alpha=0.15, color=COLORS['red'])
    ax1.set_xlabel('Elapsed Time (seconds)')
    ax1.set_ylabel('Queue Depth', color=COLORS['red'])
    ax1.tick_params(axis='y', labelcolor=COLORS['red'])

    ax2 = ax1.twinx()
    ax2.plot(t, a, color=COLORS['green'], linewidth=2.2, label='Active Threads')
    ax2.plot(t, live, color=COLORS['yellow'], linewidth=2, linestyle='--', label='Live Threads')
    ax2.set_ylabel('Threads', color=COLORS['green'])
    ax2.tick_params(axis='y', labelcolor=COLORS['green'])
    ax2.grid(False)

    ax1.set_title('Queue Depth vs Thread Utilization', fontsize=14, pad=15)

    # Combined legend
    lines1, labels1 = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    ax2.legend(lines1 + lines2, labels1 + labels2, loc='upper right',
               facecolor='#161b22', edgecolor='#30363d', labelcolor='#c9d1d9')

    fig.tight_layout()
    fig.savefig(OUT_DIR / "02_queue_vs_threads.png")
    plt.close(fig)
    print(f"  ✓ 02_queue_vs_threads.png")

# ── Graph 3: Tasks completed over time ──
def graph_completed(data):
    if not data: return
    t = [float(r['elapsed_sec']) for r in data]
    c = [int(r['completed']) for r in data]

    fig, ax = plt.subplots(figsize=(10, 5))
    ax.plot(t, c, color=COLORS['green'], linewidth=2.8)
    ax.fill_between(t, c, alpha=0.25, color=COLORS['green'])
    ax.set_title('Cumulative Tasks Completed Over Time', fontsize=14, pad=15)
    ax.set_xlabel('Elapsed Time (seconds)')
    ax.set_ylabel('Tasks Completed')
    fig.tight_layout()
    fig.savefig(OUT_DIR / "03_completed.png")
    plt.close(fig)
    print(f"  ✓ 03_completed.png")

# ── Graph 4: Average wait time ──
def graph_wait(data):
    if not data: return
    t = [float(r['elapsed_sec']) for r in data]
    w = [float(r['avg_wait_ms']) for r in data]

    fig, ax = plt.subplots(figsize=(10, 5))
    ax.plot(t, w, color=COLORS['purple'], linewidth=2.5)
    ax.fill_between(t, w, alpha=0.2, color=COLORS['purple'])
    ax.set_title('Average Task Wait Time (Queue Latency)', fontsize=14, pad=15)
    ax.set_xlabel('Elapsed Time (seconds)')
    ax.set_ylabel('Average Wait Time (ms)')
    fig.tight_layout()
    fig.savefig(OUT_DIR / "04_wait_time.png")
    plt.close(fig)
    print(f"  ✓ 04_wait_time.png")

# ── Graph 5: Comparison bar chart (sequential vs pool) ──
def graph_comparison():
    cmp = load_json("comparison.json")
    if not cmp: return

    categories = ['Sequential\nExecution', 'Thread Pool\n(16 threads)']
    values = [cmp['sequential_sec'], cmp['pool_sec']]
    colors = [COLORS['red'], COLORS['green']]

    fig, ax = plt.subplots(figsize=(9, 6))
    bars = ax.bar(categories, values, color=colors, width=0.55, edgecolor='white', linewidth=2)

    for bar, val in zip(bars, values):
        ax.text(bar.get_x() + bar.get_width()/2, val + max(values)*0.02,
                f'{val:.3f}s', ha='center', va='bottom',
                color='#c9d1d9', fontsize=13, fontweight='bold')

    ax.set_title(f'Execution Time — Sequential vs Thread Pool  (Speedup: {cmp["speedup"]:.2f}×)',
                 fontsize=14, pad=15)
    ax.set_ylabel('Execution Time (seconds)')
    ax.set_ylim(0, max(values) * 1.25)

    fig.tight_layout()
    fig.savefig(OUT_DIR / "05_comparison.png")
    plt.close(fig)
    print(f"  ✓ 05_comparison.png")

# ── Graph 6: Final summary card ──
def graph_summary():
    m = load_json("latest_metrics.json")
    if not m: return

    fig, ax = plt.subplots(figsize=(11, 6))
    ax.axis('off')
    ax.set_title('Thread Pool — Final Performance Summary', fontsize=16, pad=20, color=COLORS['blue'])

    tiles = [
        ("Tasks Submitted",   f"{m['submitted']}",        COLORS['blue']),
        ("Tasks Completed",   f"{m['completed']}",        COLORS['green']),
        ("Tasks Cancelled",   f"{m.get('cancelled', 0)}", COLORS['yellow']),
        ("Tasks Failed",      f"{m['failed']}",           COLORS['red']),
        ("Live Threads",      f"{m['live_threads']}",     COLORS['cyan']),
        ("Queue Depth",       f"{m['queue_depth']}",      COLORS['orange']),
        ("Avg Wait (ms)",     f"{m['avg_wait_ms']:.2f}",  COLORS['purple']),
        ("Avg Exec (ms)",     f"{m['avg_exec_ms']:.2f}",  COLORS['blue']),
        ("Throughput (t/s)",  f"{m['throughput']:.1f}",   COLORS['green']),
        ("Elapsed (sec)",     f"{m['elapsed_sec']:.2f}",  COLORS['yellow']),
    ]

    cols = 5
    rows = 2
    w, h = 0.185, 0.38
    sx, sy = 0.035, 0.08
    gap_x, gap_y = 0.01, 0.04

    for i, (label, val, color) in enumerate(tiles):
        r = i // cols
        c = i % cols
        x = sx + c * (w + gap_x)
        y = 0.55 - r * (h + gap_y)

        box = FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.015",
                             facecolor='#161b22', edgecolor='#30363d',
                             linewidth=1.2, transform=ax.transAxes)
        ax.add_patch(box)
        ax.text(x + w/2, y + h*0.72, val, ha='center', va='center',
                fontsize=18, fontweight='bold', color=color,
                transform=ax.transAxes)
        ax.text(x + w/2, y + h*0.25, label, ha='center', va='center',
                fontsize=10, color='#8b949e', transform=ax.transAxes)

    fig.tight_layout()
    fig.savefig(OUT_DIR / "06_summary_card.png")
    plt.close(fig)
    print(f"  ✓ 06_summary_card.png")

# ── Main ──
def main():
    print("╔══════════════════════════════════════════╗")
    print("║  Generating CA2 Report Graphs (PNG)      ║")
    print("╚══════════════════════════════════════════╝")
    print(f"  Metrics dir: {METRICS}")
    print(f"  Output dir:  {OUT_DIR}")
    print()

    history = load_history()
    if not history:
        print("  WARNING: time_series.csv not found — run './thread_library' first.")

    if history:
        graph_throughput(history)
        graph_queue_threads(history)
        graph_completed(history)
        graph_wait(history)

    graph_comparison()
    graph_summary()

    print()
    print(f"✓ All graphs saved to: {OUT_DIR}")
    print(f"  Use them in your CA2 report as figures!")

if __name__ == "__main__":
    main()
