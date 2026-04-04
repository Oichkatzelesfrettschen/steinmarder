#!/usr/bin/env python3
"""r600_spill_heuristic.py — GPR Spill Candidate Heuristic Calculator.
Benefit = (dead_range * interference) / reload_cost
reload_cost: 1 (natural boundary) or 32 (new TEX clause: 5 switch + 27 fetch)
Min dead range threshold: 4 bundles (thrashing guard)."""
import sys
def calc(dr, ifc, boundary):
    if dr < 4: return 0.0, "PROTECTED"
    rc = 1.0 if boundary else 32.2
    s = (dr * ifc) / rc
    if s > 100: return s, "STRONG"
    elif s > 20: return s, "MODERATE"
    elif s > 5: return s, "WEAK"
    return s, "PROTECT"
if __name__ == "__main__":
    if len(sys.argv) > 3:
        s, c = calc(int(sys.argv[1]), int(sys.argv[2]), sys.argv[3] in ('y','1'))
        print(f"Score={s:.1f} [{c}]")
    else:
        print("Usage: r600_spill_heuristic.py <dead_range> <interference> <boundary:y/n>")
