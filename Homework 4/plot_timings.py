import os
import re
from pathlib import Path
from typing import Dict, List

# Force non-GUI backend and ensure matplotlib can write its cache inside the workspace
os.environ.setdefault("MPLBACKEND", "Agg")
mpl_dir = Path(__file__).resolve().parent / ".mplconfig"
mpl_dir.mkdir(exist_ok=True)
os.environ.setdefault("MPLCONFIGDIR", str(mpl_dir))

import matplotlib.pyplot as plt

PhaseData = Dict[str, Dict[int, Dict[str, float]]]  # impl -> threads -> {"insert": t, "retrieve": t}


def parse_timings(log_path: Path) -> PhaseData:
    pattern_header = re.compile(r"--- (original|mutex|spinlock) t=(\d+) ---")
    pattern_insert = re.compile(r"Inserted \d+ keys in ([0-9.]+) seconds")
    pattern_retrieve = re.compile(r"Retrieved \d+/\d+ keys in ([0-9.]+) seconds")

    results: PhaseData = {"original": {}, "mutex": {}, "spinlock": {}}
    current = None

    for line in log_path.read_text().splitlines():
        header_match = pattern_header.match(line.strip())
        if header_match:
            impl, threads = header_match.groups()
            current = (impl, int(threads))
            continue

        if current is None:
            continue

        impl, threads = current
        m_insert = pattern_insert.search(line)
        m_retrieve = pattern_retrieve.search(line)
        if m_insert:
            results[impl].setdefault(threads, {})["insert"] = float(m_insert.group(1))
        if m_retrieve:
            results[impl].setdefault(threads, {})["retrieve"] = float(m_retrieve.group(1))

    return results


def plot_series(
    threads: List[int],
    series: Dict[str, List[float]],
    title: str,
    ylabel: str,
    out_path: Path,
):
    plt.figure(figsize=(8, 4))
    colors = {"original": "#1f77b4", "mutex": "#d62728", "spinlock": "#2ca02c"}
    labels = {"original": "Original", "mutex": "Mutex", "spinlock": "Spinlock"}
    for impl, values in series.items():
        plt.plot(threads, values, marker="o", label=labels.get(impl, impl), color=colors.get(impl))
    plt.xlabel("Threads")
    plt.ylabel(ylabel)
    plt.title(title)
    plt.xticks(threads)
    plt.grid(True, linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_path, dpi=150)
    print(f"Saved {out_path}")


def main():
    log_path = Path("timing_output_with_spin.txt")
    if not log_path.exists():
        raise SystemExit(f"Missing log file: {log_path}")

    results = parse_timings(log_path)
    threads_sorted = sorted(
        set().union(*(results[impl].keys() for impl in results))
    )
    if not threads_sorted:
        raise SystemExit("No timing data found in log file.")

    insert_series: Dict[str, List[float]] = {}
    retrieve_series: Dict[str, List[float]] = {}
    total_series: Dict[str, List[float]] = {}
    for impl in results:
        insert_series[impl] = []
        retrieve_series[impl] = []
        total_series[impl] = []
        for t in threads_sorted:
            ins = results[impl].get(t, {}).get("insert", float("nan"))
            ret = results[impl].get(t, {}).get("retrieve", float("nan"))
            insert_series[impl].append(ins)
            retrieve_series[impl].append(ret)
            total_series[impl].append(ins + ret)

    plot_series(threads_sorted, insert_series, "Insertion time vs. threads", "Time (s)", Path("timing_insert.png"))
    plot_series(threads_sorted, retrieve_series, "Retrieval time vs. threads", "Time (s)", Path("timing_retrieve.png"))
    plot_series(threads_sorted, total_series, "Total time (insert+retrieve) vs. threads", "Time (s)", Path("timing_total.png"))


if __name__ == "__main__":
    main()
