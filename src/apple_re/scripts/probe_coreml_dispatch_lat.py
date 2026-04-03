#!/usr/bin/env python3
"""
F6: CoreML ANE dispatch latency probe.

Creates a minimal CoreML model (single linear layer, weight=I, no bias) and
measures the wall-clock dispatch latency for warm subsequent predictions.

The model is intentionally trivial to isolate dispatch overhead from compute
time. Both CPU and ANE targets are measured.

Usage:
    python3 probe_coreml_dispatch_lat.py [--n-iters N] [--warmup N] [--size N]

Requirements:
    pip install coremltools numpy

Output:
    JSON to stdout + human-readable summary.
"""

import argparse
import json
import sys
import time

import numpy as np

try:
    import coremltools as ct
    from coremltools.models.neural_network import NeuralNetworkBuilder
except ImportError:
    print("ERROR: coremltools not installed. Run: pip install coremltools", file=sys.stderr)
    sys.exit(1)

try:
    import torch
    import torch.nn as nn
    _has_torch = True
except ImportError:
    _has_torch = False


def build_minimal_coreml_model(input_size: int, compute_unit: str) -> "ct.models.MLModel":
    """Build an identity-weight linear model in CoreML format.

    Produces a model that computes output = input @ I = input.
    This minimises compute time, isolating dispatch overhead.

    Args:
        input_size: Size of the 1D input vector.
        compute_unit: 'ALL' (ANE preferred), 'CPU_ONLY', or 'CPU_AND_GPU'.
    """
    # Build via the NeuralNetwork builder (spec v4, broadly compatible).
    input_name = "input_features"
    output_name = "output_features"

    builder = NeuralNetworkBuilder(
        input_features=[(input_name, ct.models.datatypes.Array(input_size))],
        output_features=[(output_name, ct.models.datatypes.Array(input_size))],
    )

    # Identity weight matrix (no-op linear transform).
    weight_matrix = np.eye(input_size, dtype=np.float32)
    builder.add_inner_product(
        name="identity_fc",
        W=weight_matrix,
        b=None,
        input_channels=input_size,
        output_channels=input_size,
        has_bias=False,
        input_name=input_name,
        output_name=output_name,
    )

    raw_model = ct.models.MLModel(builder.spec)

    # Re-load with the chosen compute unit to target ANE/CPU/GPU.
    compute_unit_map = {
        "ALL":         ct.ComputeUnit.ALL,
        "CPU_ONLY":    ct.ComputeUnit.CPU_ONLY,
        "CPU_AND_GPU": ct.ComputeUnit.CPU_AND_GPU,
    }
    cu = compute_unit_map.get(compute_unit.upper(), ct.ComputeUnit.ALL)

    model = ct.models.MLModel(raw_model.get_spec(), compute_units=cu)
    return model


def percentile(sorted_values: list, p: float) -> float:
    """p in [0, 100]."""
    idx = int(len(sorted_values) * p / 100.0)
    idx = min(idx, len(sorted_values) - 1)
    return sorted_values[idx]


def measure_dispatch_latency(
    model,
    input_array: np.ndarray,
    input_name: str,
    n_iters: int,
    n_warmup: int,
) -> dict:
    """Run the model n_warmup + n_iters times and return latency statistics."""
    input_dict = {input_name: input_array}

    # Warmup: ignored in statistics.
    for _ in range(n_warmup):
        _ = model.predict(input_dict)

    latencies_ns = []
    for _ in range(n_iters):
        t0 = time.perf_counter_ns()
        _ = model.predict(input_dict)
        t1 = time.perf_counter_ns()
        latencies_ns.append(t1 - t0)

    latencies_ns.sort()
    mean_ns = sum(latencies_ns) / len(latencies_ns)

    return {
        "n_iters":   n_iters,
        "n_warmup":  n_warmup,
        "min_ns":    latencies_ns[0],
        "p50_ns":    percentile(latencies_ns, 50),
        "p90_ns":    percentile(latencies_ns, 90),
        "p95_ns":    percentile(latencies_ns, 95),
        "p99_ns":    percentile(latencies_ns, 99),
        "max_ns":    latencies_ns[-1],
        "mean_ns":   mean_ns,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description="F6: CoreML dispatch latency probe")
    parser.add_argument("--n-iters",  type=int, default=200,
                        help="Number of timed prediction calls (default: 200)")
    parser.add_argument("--warmup",   type=int, default=20,
                        help="Warmup calls before timing (default: 20)")
    parser.add_argument("--size",     type=int, default=64,
                        help="Linear layer input/output size (default: 64)")
    parser.add_argument("--targets",  type=str, default="ALL,CPU_ONLY",
                        help="Comma-separated compute targets (default: ALL,CPU_ONLY)")
    parser.add_argument("--json",     action="store_true",
                        help="Emit JSON output only (for machine parsing)")
    args = parser.parse_args()

    input_size  = args.size
    n_iters     = args.n_iters
    n_warmup    = args.warmup
    targets     = [t.strip().upper() for t in args.targets.split(",")]
    input_array = np.ones((input_size,), dtype=np.float32) * 0.5
    input_name  = "input_features"

    results = {
        "probe": "probe_coreml_dispatch_lat",
        "date":  time.strftime("%Y-%m-%d"),
        "model_description": f"single identity FC layer, input_size={input_size}",
        "measurements": [],
    }

    for target in targets:
        if not args.json:
            print(f"\n--- target={target} ---", flush=True)
            print(f"Building model (input_size={input_size})...", flush=True)

        try:
            model = build_minimal_coreml_model(input_size, target)
        except Exception as exc:
            msg = f"FAILED to build model for target={target}: {exc}"
            if not args.json:
                print(msg, file=sys.stderr)
            results["measurements"].append({"target": target, "error": str(exc)})
            continue

        if not args.json:
            print(f"Warming up ({n_warmup} calls)...", flush=True)

        try:
            stats = measure_dispatch_latency(
                model=model,
                input_array=input_array,
                input_name=input_name,
                n_iters=n_iters,
                n_warmup=n_warmup,
            )
        except Exception as exc:
            msg = f"FAILED to measure latency for target={target}: {exc}"
            if not args.json:
                print(msg, file=sys.stderr)
            results["measurements"].append({"target": target, "error": str(exc)})
            continue

        stats["target"] = target
        results["measurements"].append(stats)

        if not args.json:
            p50_us  = stats["p50_ns"] / 1000.0
            p95_us  = stats["p95_ns"] / 1000.0
            mean_us = stats["mean_ns"] / 1000.0
            min_us  = stats["min_ns"] / 1000.0
            max_us  = stats["max_ns"] / 1000.0
            print(f"  n={n_iters}  warmup={n_warmup}")
            print(f"  min={min_us:.0f} µs  p50={p50_us:.0f} µs  mean={mean_us:.0f} µs"
                  f"  p95={p95_us:.0f} µs  max={max_us:.0f} µs")

    if args.json:
        print(json.dumps(results, indent=2))
    else:
        print("\n--- Summary ---")
        for meas in results["measurements"]:
            target = meas["target"]
            if "error" in meas:
                print(f"  {target}: ERROR — {meas['error']}")
            else:
                p50_us = meas["p50_ns"] / 1000.0
                print(f"  {target}: p50={p50_us:.0f} µs"
                      f"  (min={meas['min_ns']/1000:.0f}µs"
                      f"  mean={meas['mean_ns']/1000:.0f}µs"
                      f"  p99={meas['p99_ns']/1000:.0f}µs"
                      f"  max={meas['max_ns']/1000:.0f}µs)")


if __name__ == "__main__":
    main()
