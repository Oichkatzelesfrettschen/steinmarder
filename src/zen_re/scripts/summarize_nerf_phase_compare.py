#!/usr/bin/env python3
import csv
import statistics
import sys
from collections import defaultdict

if len(sys.argv) != 2:
    print("usage: summarize_nerf_phase_compare.py SUMMARY.csv", file=sys.stderr)
    sys.exit(1)

rows = list(csv.DictReader(open(sys.argv[1], newline="")))
by_mode = defaultdict(list)
for row in rows:
    by_mode[row["mode"]].append(row)

print("mode,count,render_ms_avg,render_ms_stdev,phase_ms_avg,hashgrid_ms_avg,mlp_ms_avg,hashgrid_ns_per_step_avg,mlp_ns_per_step_avg,composite_ns_per_step_avg")
for mode in sorted(by_mode):
    vals = by_mode[mode]
    render = [float(v["render_ms"]) for v in vals]
    phase = [float(v["total_phase_ms"]) for v in vals]
    hg_ms = [float(v["hashgrid_ms"]) for v in vals]
    mlp_ms = [float(v["mlp_ms"]) for v in vals]
    hg_ns = [float(v["hashgrid_ns_per_step"]) for v in vals]
    mlp_ns = [float(v["mlp_ns_per_step"]) for v in vals]
    comp_ns = [float(v["composite_ns_per_step"]) for v in vals]
    print(",".join([
        mode,
        str(len(vals)),
        f"{sum(render)/len(render):.2f}",
        f"{statistics.pstdev(render):.2f}",
        f"{sum(phase)/len(phase):.2f}",
        f"{sum(hg_ms)/len(hg_ms):.3f}",
        f"{sum(mlp_ms)/len(mlp_ms):.3f}",
        f"{sum(hg_ns)/len(hg_ns):.2f}",
        f"{sum(mlp_ns)/len(mlp_ns):.2f}",
        f"{sum(comp_ns)/len(comp_ns):.2f}",
    ]))
