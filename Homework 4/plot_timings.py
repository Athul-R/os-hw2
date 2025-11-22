import re
from pathlib import Path
from typing import Dict, List, Tuple


PointMap = Dict[str, Dict[int, float]]


def parse_timings(log_path: Path, key_string: str) -> PointMap:
    pattern_header = re.compile(r"--- (original|mutex) t=(\d+) ---")
    pattern_time = re.compile(f".* {key_string} .* in ([0-9.]+) seconds")

    results: PointMap = {"original": {}, "mutex": {}}
    current = None

    for line in log_path.read_text().splitlines():
        header_match = pattern_header.match(line.strip())
        if header_match:
            impl, threads = header_match.groups()
            current = (impl, int(threads))
            continue

        if current is None:
            continue

        time_match = pattern_time.search(line)
        if time_match:
            impl, threads = current
            elapsed = float(time_match.group(1))
            # Sum insert + retrieve time for each run
            results[impl].setdefault(threads, 0.0)
            results[impl][threads] += elapsed

    return results


def main():
    log_path = Path("timing_output.txt")
    if not log_path.exists():
        raise SystemExit(f"Missing log file: {log_path}")
    key_string = "Retrieved"
    results = parse_timings(log_path, key_string)
    print(results)
    threads_sorted = sorted(set(results["original"]) | set(results["mutex"]))
    if not threads_sorted:
        raise SystemExit("No timing data found in log file.")

    orig_times = [results["original"].get(t, float("nan")) for t in threads_sorted]
    mutex_times = [results["mutex"].get(t, float("nan")) for t in threads_sorted]

    create_svg(threads_sorted, orig_times, mutex_times, Path("timing_plot.svg"), key_string)


def create_svg(threads: List[int], orig: List[float], mutex: List[float], out_path: Path, key_string="Retrieve"):
    width, height = 720, 420
    margin = 60
    plot_w = width - 2 * margin
    plot_h = height - 2 * margin

    max_time = max([t for t in orig + mutex if t == t])  # filter nan
    min_time = min([t for t in orig + mutex if t == t])
    # Avoid zero range
    if max_time == min_time:
        max_time += 1.0

    def x_pos(idx: int) -> float:
        return margin + (plot_w * idx / (len(threads) - 1 if len(threads) > 1 else 1))

    def y_pos(val: float) -> float:
        # Flip y (SVG origin is top-left)
        return margin + plot_h - ((val - min_time) / (max_time - min_time)) * plot_h

    def polyline(points: List[Tuple[float, float]], color: str) -> str:
        coords = " ".join(f"{x:.1f},{y:.1f}" for x, y in points)
        return f'<polyline fill="none" stroke="{color}" stroke-width="2" points="{coords}"/>\n'

    def markers(points: List[Tuple[float, float]], color: str) -> str:
        return "\n".join(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="4" fill="{color}"/>' for x, y in points)

    # Build axes and labels
    svg = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">',
        f'<rect width="100%" height="100%" fill="white"/>',
        f'<text x="{width/2:.1f}" y="28" text-anchor="middle" font-family="Arial" font-size="18">Hashtable {key_string} runtime vs. thread count</text>',
        # Y axis
        f'<line x1="{margin}" y1="{margin}" x2="{margin}" y2="{height - margin}" stroke="black" />',
        # X axis
        f'<line x1="{margin}" y1="{height - margin}" x2="{width - margin}" y2="{height - margin}" stroke="black" />',
        f'<text x="{margin - 10}" y="{margin - 10}" text-anchor="end" font-family="Arial" font-size="12">Time (s)</text>',
        f'<text x="{width/2:.1f}" y="{height - 10}" text-anchor="middle" font-family="Arial" font-size="12">Threads</text>',
    ]

    # y ticks (5 ticks)
    for i in range(6):
        val = min_time + (max_time - min_time) * i / 5
        y = y_pos(val)
        svg.append(f'<line x1="{margin-5}" y1="{y:.1f}" x2="{margin}" y2="{y:.1f}" stroke="black" />')
        svg.append(f'<text x="{margin-8}" y="{y+4:.1f}" text-anchor="end" font-family="Arial" font-size="10">{val:.2f}</text>')
        svg.append(f'<line x1="{margin}" y1="{y:.1f}" x2="{width - margin}" y2="{y:.1f}" stroke="#ddd" />')

    # x ticks
    for idx, t in enumerate(threads):
        x = x_pos(idx)
        svg.append(f'<line x1="{x:.1f}" y1="{height - margin}" x2="{x:.1f}" y2="{height - margin + 5}" stroke="black" />')
        svg.append(f'<text x="{x:.1f}" y="{height - margin + 18}" text-anchor="middle" font-family="Arial" font-size="10">{t}</text>')

    orig_points = [(x_pos(i), y_pos(orig[i])) for i in range(len(threads))]
    mutex_points = [(x_pos(i), y_pos(mutex[i])) for i in range(len(threads))]

    svg.append(polyline(orig_points, "#1f77b4"))
    svg.append(polyline(mutex_points, "#d62728"))
    svg.append(markers(orig_points, "#1f77b4"))
    svg.append(markers(mutex_points, "#d62728"))

    # Legend
    legend_x = width - margin - 170
    legend_y = margin
    svg.append(f'<rect x="{legend_x}" y="{legend_y}" width="170" height="50" fill="white" stroke="black"/>')
    svg.append(f'<line x1="{legend_x + 10}" y1="{legend_y + 15}" x2="{legend_x + 30}" y2="{legend_y + 15}" stroke="#1f77b4" stroke-width="2"/>')
    svg.append(f'<text x="{legend_x + 40}" y="{legend_y + 19}" font-family="Arial" font-size="12">Original (unsafe)</text>')
    svg.append(f'<line x1="{legend_x + 10}" y1="{legend_y + 35}" x2="{legend_x + 30}" y2="{legend_y + 35}" stroke="#d62728" stroke-width="2"/>')
    svg.append(f'<text x="{legend_x + 40}" y="{legend_y + 39}" font-family="Arial" font-size="12">Mutex</text>')

    svg.append("</svg>")

    out_path.write_text("\n".join(svg))
    print(f"Saved {out_path}")


if __name__ == "__main__":
    main()
