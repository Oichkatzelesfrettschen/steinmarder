#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import json
import math
import os
import statistics
import time
from pathlib import Path


DEFAULT_WARMUP_RUNS = 10
DEFAULT_MEASURED_RUNS = 30

WORKLOADS = (
    {"id": "gemm_256", "kind": "gemm", "m": 256, "n": 256, "k": 256},
    {"id": "gemm_1024", "kind": "gemm", "m": 1024, "n": 1024, "k": 1024},
    {"id": "gemm_2048", "kind": "gemm", "m": 2048, "n": 2048, "k": 2048},
    {"id": "reduce_1m", "kind": "reduce", "size": 1_000_000},
)

TORCH_DTYPES = {
    "float16": "float16",
    "float32": "float32",
    "float64": "float64",
    "bfloat16": "bfloat16",
    "int8": "int8",
    "uint8": "uint8",
    "int16": "int16",
    "int32": "int32",
    "int64": "int64",
    "uint16": "uint16",
    "uint32": "uint32",
    "uint64": "uint64",
}

TORCH_PROXY_FORMATS = (
    "tf32",
    "fp8_e4m3",
    "fp8_e5m2",
    "mxfp8",
    "mxfp6",
    "mxfp4",
    "mxint8",
    "fp4",
    "int4",
    "uint4",
)

PLANNED_FRONTIER_FORMATS = ("nf4",)


def torch_quantize_tf32_proxy(torch_mod, tensor):
    return torch_mod.round(tensor * 1024.0) / 1024.0


def torch_quantize_minifloat_proxy(torch_mod, tensor, mantissa_bits: int):
    scale = float(1 << mantissa_bits)
    return torch_mod.round(tensor * scale) / scale


def torch_quantize_mx_proxy(torch_mod, tensor, mantissa_bits: int):
    max_abs = torch_mod.amax(torch_mod.abs(tensor), dim=-1, keepdim=True)
    max_abs = torch_mod.clamp(max_abs, min=1.0)
    normalized = tensor / max_abs
    quantized = torch_quantize_minifloat_proxy(torch_mod, normalized, mantissa_bits)
    return quantized * max_abs


def torch_quantize_mxint8_proxy(torch_mod, tensor):
    max_abs = torch_mod.amax(torch_mod.abs(tensor), dim=-1, keepdim=True)
    scale = torch_mod.clamp(max_abs / 127.0, min=1.0 / 127.0)
    quantized = torch_mod.round(torch_mod.clamp(tensor / scale, -127.0, 127.0))
    return quantized * scale


def mlx_quantize_tf32_proxy(mx_mod, tensor):
    return mx_mod.round(tensor * 1024.0) / 1024.0


def mlx_quantize_minifloat_proxy(mx_mod, tensor, mantissa_bits: int):
    scale = float(1 << mantissa_bits)
    return mx_mod.round(tensor * scale) / scale


def mlx_quantize_mx_proxy(mx_mod, tensor, mantissa_bits: int):
    max_abs = mx_mod.max(mx_mod.abs(tensor), axis=-1, keepdims=True)
    max_abs = mx_mod.maximum(max_abs, 1.0)
    normalized = tensor / max_abs
    quantized = mlx_quantize_minifloat_proxy(mx_mod, normalized, mantissa_bits)
    return quantized * max_abs


def mlx_quantize_mxint8_proxy(mx_mod, tensor):
    max_abs = mx_mod.max(mx_mod.abs(tensor), axis=-1, keepdims=True)
    scale = mx_mod.maximum(max_abs / 127.0, 1.0 / 127.0)
    quantized = mx_mod.round(mx_mod.clip(tensor / scale, -127.0, 127.0))
    return quantized * scale


# ── fp4 (E2M1): 1 sign, 2 exponent, 1 mantissa bit; max magnitude = 6.0 ──────
def torch_quantize_fp4e2m1_proxy(torch_mod, tensor):
    clamped = torch_mod.clamp(tensor, -6.0, 6.0)
    return torch_quantize_minifloat_proxy(torch_mod, clamped, 1)


def mlx_quantize_fp4e2m1_proxy(mx_mod, tensor):
    clamped = mx_mod.clip(tensor, -6.0, 6.0)
    return mlx_quantize_minifloat_proxy(mx_mod, clamped, 1)


# ── int4 (signed symmetric, range −7..7): per-row max-abs scale ──────────────
def torch_quantize_int4_proxy(torch_mod, tensor):
    max_abs = torch_mod.amax(torch_mod.abs(tensor), dim=-1, keepdim=True)
    scale = torch_mod.clamp(max_abs / 7.0, min=1.0 / 7.0)
    quantized = torch_mod.round(torch_mod.clamp(tensor / scale, -7.0, 7.0))
    return quantized * scale


def mlx_quantize_int4_proxy(mx_mod, tensor):
    max_abs = mx_mod.max(mx_mod.abs(tensor), axis=-1, keepdims=True)
    scale = mx_mod.maximum(max_abs / 7.0, 1.0 / 7.0)
    quantized = mx_mod.round(mx_mod.clip(tensor / scale, -7.0, 7.0))
    return quantized * scale


# ── uint4 (unsigned, range 0..15): per-row min/max affine scale ──────────────
def torch_quantize_uint4_proxy(torch_mod, tensor):
    min_val = torch_mod.amin(tensor, dim=-1, keepdim=True)
    max_val = torch_mod.amax(tensor, dim=-1, keepdim=True)
    scale = torch_mod.clamp((max_val - min_val) / 15.0, min=1.0 / 15.0)
    quantized = torch_mod.round(torch_mod.clamp((tensor - min_val) / scale, 0.0, 15.0))
    return quantized * scale + min_val


def mlx_quantize_uint4_proxy(mx_mod, tensor):
    min_val = mx_mod.min(tensor, axis=-1, keepdims=True)
    max_val = mx_mod.max(tensor, axis=-1, keepdims=True)
    scale = mx_mod.maximum((max_val - min_val) / 15.0, 1.0 / 15.0)
    quantized = mx_mod.round(mx_mod.clip((tensor - min_val) / scale, 0.0, 15.0))
    return quantized * scale + min_val


def time_mlx_proxy_backend(
    mx_mod,
    dtype_name: str,
    workload: dict[str, object],
    warmup_runs: int,
    measured_runs: int,
) -> dict[str, object]:
    try:
        if workload["kind"] == "gemm":
            shape_a = (int(workload["m"]), int(workload["k"]))
            shape_b = (int(workload["k"]), int(workload["n"]))
            a = mx_mod.random.normal(shape=shape_a, dtype=mx_mod.float32)
            b = mx_mod.random.normal(shape=shape_b, dtype=mx_mod.float32)

            if dtype_name == "tf32":
                qa = mlx_quantize_tf32_proxy(mx_mod, a)
                qb = mlx_quantize_tf32_proxy(mx_mod, b)

                def run_once():
                    out = mlx_quantize_tf32_proxy(mx_mod, qa @ qb)
                    mx_mod.eval(out)
                    return float(out[0, 0].item())

            elif dtype_name == "fp8_e4m3":
                qa = mlx_quantize_minifloat_proxy(mx_mod, a, 3)
                qb = mlx_quantize_minifloat_proxy(mx_mod, b, 3)

                def run_once():
                    out = mlx_quantize_minifloat_proxy(mx_mod, qa @ qb, 3)
                    mx_mod.eval(out)
                    return float(out[0, 0].item())

            elif dtype_name == "fp8_e5m2":
                qa = mlx_quantize_minifloat_proxy(mx_mod, a, 2)
                qb = mlx_quantize_minifloat_proxy(mx_mod, b, 2)

                def run_once():
                    out = mlx_quantize_minifloat_proxy(mx_mod, qa @ qb, 2)
                    mx_mod.eval(out)
                    return float(out[0, 0].item())

            elif dtype_name == "mxfp8":
                qa = mlx_quantize_mx_proxy(mx_mod, a, 3)
                qb = mlx_quantize_mx_proxy(mx_mod, b, 3)

                def run_once():
                    out = mlx_quantize_mx_proxy(mx_mod, qa @ qb, 3)
                    mx_mod.eval(out)
                    return float(out[0, 0].item())

            elif dtype_name == "mxfp6":
                qa = mlx_quantize_mx_proxy(mx_mod, a, 2)
                qb = mlx_quantize_mx_proxy(mx_mod, b, 2)

                def run_once():
                    out = mlx_quantize_mx_proxy(mx_mod, qa @ qb, 2)
                    mx_mod.eval(out)
                    return float(out[0, 0].item())

            elif dtype_name == "mxfp4":
                qa = mlx_quantize_mx_proxy(mx_mod, a, 1)
                qb = mlx_quantize_mx_proxy(mx_mod, b, 1)

                def run_once():
                    out = mlx_quantize_mx_proxy(mx_mod, qa @ qb, 1)
                    mx_mod.eval(out)
                    return float(out[0, 0].item())

            elif dtype_name == "mxint8":
                qa = mlx_quantize_mxint8_proxy(mx_mod, a)
                qb = mlx_quantize_mxint8_proxy(mx_mod, b)

                def run_once():
                    out = mlx_quantize_mxint8_proxy(mx_mod, qa @ qb)
                    mx_mod.eval(out)
                    return float(out[0, 0].item())

            elif dtype_name == "fp4":
                qa = mlx_quantize_fp4e2m1_proxy(mx_mod, a)
                qb = mlx_quantize_fp4e2m1_proxy(mx_mod, b)

                def run_once():
                    out = mlx_quantize_fp4e2m1_proxy(mx_mod, qa @ qb)
                    mx_mod.eval(out)
                    return float(out[0, 0].item())

            elif dtype_name == "int4":
                qa = mlx_quantize_int4_proxy(mx_mod, a)
                qb = mlx_quantize_int4_proxy(mx_mod, b)

                def run_once():
                    out = mlx_quantize_int4_proxy(mx_mod, qa @ qb)
                    mx_mod.eval(out)
                    return float(out[0, 0].item())

            elif dtype_name == "uint4":
                qa = mlx_quantize_uint4_proxy(mx_mod, a)
                qb = mlx_quantize_uint4_proxy(mx_mod, b)

                def run_once():
                    out = mlx_quantize_uint4_proxy(mx_mod, qa @ qb)
                    mx_mod.eval(out)
                    return float(out[0, 0].item())

            else:
                return {
                    "backend": "mlx_proxy",
                    "format": dtype_name,
                    "workload": workload["id"],
                    "status": "planned",
                    "support_tier": "framework_proxy",
                    "notes": "MLX proxy timing path not implemented for this format yet.",
                }
        else:
            x = mx_mod.random.normal(shape=(int(workload["size"]),), dtype=mx_mod.float32)
            if dtype_name == "tf32":
                qx = mlx_quantize_tf32_proxy(mx_mod, x)

                def run_once():
                    out = mlx_quantize_tf32_proxy(mx_mod, mx_mod.sum(qx))
                    mx_mod.eval(out)
                    return float(out.item())

            elif dtype_name in {"fp8_e4m3", "fp8_e5m2"}:
                mantissa_bits = 3 if dtype_name == "fp8_e4m3" else 2
                qx = mlx_quantize_minifloat_proxy(mx_mod, x, mantissa_bits)

                def run_once():
                    out = mlx_quantize_minifloat_proxy(mx_mod, mx_mod.sum(qx), mantissa_bits)
                    mx_mod.eval(out)
                    return float(out.item())

            elif dtype_name in {"mxfp8", "mxfp6", "mxfp4"}:
                mantissa_bits = {"mxfp8": 3, "mxfp6": 2, "mxfp4": 1}[dtype_name]
                qx = mlx_quantize_mx_proxy(mx_mod, x.reshape(1, -1), mantissa_bits).reshape(-1)

                def run_once():
                    out = mlx_quantize_mx_proxy(mx_mod, qx.reshape(1, -1), mantissa_bits)
                    summed = mx_mod.sum(out)
                    mx_mod.eval(summed)
                    return float(summed.item())

            elif dtype_name == "mxint8":
                qx = mlx_quantize_mxint8_proxy(mx_mod, x.reshape(1, -1)).reshape(-1)

                def run_once():
                    out = mlx_quantize_mxint8_proxy(mx_mod, qx.reshape(1, -1))
                    summed = mx_mod.sum(out)
                    mx_mod.eval(summed)
                    return float(summed.item())

            elif dtype_name == "fp4":
                qx = mlx_quantize_fp4e2m1_proxy(mx_mod, x.reshape(1, -1)).reshape(-1)

                def run_once():
                    out = mlx_quantize_fp4e2m1_proxy(mx_mod, qx.reshape(1, -1))
                    summed = mx_mod.sum(out)
                    mx_mod.eval(summed)
                    return float(summed.item())

            elif dtype_name == "int4":
                qx = mlx_quantize_int4_proxy(mx_mod, x.reshape(1, -1)).reshape(-1)

                def run_once():
                    out = mlx_quantize_int4_proxy(mx_mod, qx.reshape(1, -1))
                    summed = mx_mod.sum(out)
                    mx_mod.eval(summed)
                    return float(summed.item())

            elif dtype_name == "uint4":
                qx = mlx_quantize_uint4_proxy(mx_mod, x.reshape(1, -1)).reshape(-1)

                def run_once():
                    out = mlx_quantize_uint4_proxy(mx_mod, qx.reshape(1, -1))
                    summed = mx_mod.sum(out)
                    mx_mod.eval(summed)
                    return float(summed.item())

            else:
                return {
                    "backend": "mlx_proxy",
                    "format": dtype_name,
                    "workload": workload["id"],
                    "status": "planned",
                    "support_tier": "framework_proxy",
                    "notes": "MLX proxy timing path not implemented for this format yet.",
                }

        for _ in range(warmup_runs):
            run_once()

        timings: list[float] = []
        checksum = 0.0
        for _ in range(measured_runs):
            start = time.perf_counter()
            checksum = run_once()
            timings.append((time.perf_counter() - start) * 1000.0)

        return {
            "backend": "mlx_proxy",
            "format": dtype_name,
            "workload": workload["id"],
            "status": "measured",
            "support_tier": "framework_proxy",
            "median_ms": statistics.median(timings),
            "p90_ms": percentile(timings, 0.9),
            "warmup_runs": warmup_runs,
            "measured_runs": measured_runs,
            "checksum": checksum,
            "notes": f"default_device={mx_mod.default_device()}, proxy_backend=mlx_float32",
        }
    except Exception as exc:  # pragma: no cover - hardware dependent
        return {
            "backend": "mlx_proxy",
            "format": dtype_name,
            "workload": workload["id"],
            "status": "error",
            "support_tier": "unsupported_or_proxy",
            "notes": f"{type(exc).__name__}: {exc}",
        }


def time_torch_proxy_backend(
    torch_mod,
    backend: str,
    device: str,
    dtype_name: str,
    workload: dict[str, object],
    warmup_runs: int,
    measured_runs: int,
) -> dict[str, object]:
    try:
        if workload["kind"] == "gemm":
            shape_a = (int(workload["m"]), int(workload["k"]))
            shape_b = (int(workload["k"]), int(workload["n"]))
            a = torch_mod.randn(shape_a, dtype=torch_mod.float32, device=device)
            b = torch_mod.randn(shape_b, dtype=torch_mod.float32, device=device)

            if dtype_name == "tf32":
                qa = torch_quantize_tf32_proxy(torch_mod, a)
                qb = torch_quantize_tf32_proxy(torch_mod, b)

                def run_once():
                    out = torch_quantize_tf32_proxy(torch_mod, qa @ qb)
                    return float(out.reshape(-1)[0].item())

            elif dtype_name == "fp8_e4m3":
                qa = torch_quantize_minifloat_proxy(torch_mod, a, 3)
                qb = torch_quantize_minifloat_proxy(torch_mod, b, 3)

                def run_once():
                    out = torch_quantize_minifloat_proxy(torch_mod, qa @ qb, 3)
                    return float(out.reshape(-1)[0].item())

            elif dtype_name == "fp8_e5m2":
                qa = torch_quantize_minifloat_proxy(torch_mod, a, 2)
                qb = torch_quantize_minifloat_proxy(torch_mod, b, 2)

                def run_once():
                    out = torch_quantize_minifloat_proxy(torch_mod, qa @ qb, 2)
                    return float(out.reshape(-1)[0].item())

            elif dtype_name == "mxfp8":
                qa = torch_quantize_mx_proxy(torch_mod, a, 3)
                qb = torch_quantize_mx_proxy(torch_mod, b, 3)

                def run_once():
                    out = torch_quantize_mx_proxy(torch_mod, qa @ qb, 3)
                    return float(out.reshape(-1)[0].item())

            elif dtype_name == "mxfp6":
                qa = torch_quantize_mx_proxy(torch_mod, a, 2)
                qb = torch_quantize_mx_proxy(torch_mod, b, 2)

                def run_once():
                    out = torch_quantize_mx_proxy(torch_mod, qa @ qb, 2)
                    return float(out.reshape(-1)[0].item())

            elif dtype_name == "mxfp4":
                qa = torch_quantize_mx_proxy(torch_mod, a, 1)
                qb = torch_quantize_mx_proxy(torch_mod, b, 1)

                def run_once():
                    out = torch_quantize_mx_proxy(torch_mod, qa @ qb, 1)
                    return float(out.reshape(-1)[0].item())

            elif dtype_name == "mxint8":
                qa = torch_quantize_mxint8_proxy(torch_mod, a)
                qb = torch_quantize_mxint8_proxy(torch_mod, b)

                def run_once():
                    out = torch_quantize_mxint8_proxy(torch_mod, qa @ qb)
                    return float(out.reshape(-1)[0].item())

            elif dtype_name == "fp4":
                qa = torch_quantize_fp4e2m1_proxy(torch_mod, a)
                qb = torch_quantize_fp4e2m1_proxy(torch_mod, b)

                def run_once():
                    out = torch_quantize_fp4e2m1_proxy(torch_mod, qa @ qb)
                    return float(out.reshape(-1)[0].item())

            elif dtype_name == "int4":
                qa = torch_quantize_int4_proxy(torch_mod, a)
                qb = torch_quantize_int4_proxy(torch_mod, b)

                def run_once():
                    out = torch_quantize_int4_proxy(torch_mod, qa @ qb)
                    return float(out.reshape(-1)[0].item())

            elif dtype_name == "uint4":
                qa = torch_quantize_uint4_proxy(torch_mod, a)
                qb = torch_quantize_uint4_proxy(torch_mod, b)

                def run_once():
                    out = torch_quantize_uint4_proxy(torch_mod, qa @ qb)
                    return float(out.reshape(-1)[0].item())

            else:
                return {
                    "backend": backend,
                    "format": dtype_name,
                    "workload": workload["id"],
                    "status": "planned",
                    "support_tier": "framework_proxy",
                    "notes": "Proxy timing path not implemented for this format yet.",
                }
        else:
            x = torch_mod.randn((int(workload["size"]),), dtype=torch_mod.float32, device=device)
            if dtype_name == "tf32":
                qx = torch_quantize_tf32_proxy(torch_mod, x)

                def run_once():
                    out = torch_quantize_tf32_proxy(torch_mod, qx.sum().reshape(1))
                    return float(out.item())

            elif dtype_name in {"fp8_e4m3", "fp8_e5m2"}:
                mantissa_bits = 3 if dtype_name == "fp8_e4m3" else 2
                qx = torch_quantize_minifloat_proxy(torch_mod, x, mantissa_bits)

                def run_once():
                    out = torch_quantize_minifloat_proxy(torch_mod, qx.sum().reshape(1), mantissa_bits)
                    return float(out.item())

            elif dtype_name in {"mxfp8", "mxfp6", "mxfp4"}:
                mantissa_bits = {"mxfp8": 3, "mxfp6": 2, "mxfp4": 1}[dtype_name]
                qx = torch_quantize_mx_proxy(torch_mod, x.reshape(1, -1), mantissa_bits).reshape(-1)

                def run_once():
                    out = torch_quantize_mx_proxy(torch_mod, qx.reshape(1, -1), mantissa_bits).sum()
                    return float(out.item())

            elif dtype_name == "mxint8":
                qx = torch_quantize_mxint8_proxy(torch_mod, x.reshape(1, -1)).reshape(-1)

                def run_once():
                    out = torch_quantize_mxint8_proxy(torch_mod, qx.reshape(1, -1)).sum()
                    return float(out.item())

            elif dtype_name == "fp4":
                qx = torch_quantize_fp4e2m1_proxy(torch_mod, x.reshape(1, -1)).reshape(-1)

                def run_once():
                    out = torch_quantize_fp4e2m1_proxy(torch_mod, qx.reshape(1, -1)).sum()
                    return float(out.item())

            elif dtype_name == "int4":
                qx = torch_quantize_int4_proxy(torch_mod, x.reshape(1, -1)).reshape(-1)

                def run_once():
                    out = torch_quantize_int4_proxy(torch_mod, qx.reshape(1, -1)).sum()
                    return float(out.item())

            elif dtype_name == "uint4":
                qx = torch_quantize_uint4_proxy(torch_mod, x.reshape(1, -1)).reshape(-1)

                def run_once():
                    out = torch_quantize_uint4_proxy(torch_mod, qx.reshape(1, -1)).sum()
                    return float(out.item())

            else:
                return {
                    "backend": backend,
                    "format": dtype_name,
                    "workload": workload["id"],
                    "status": "planned",
                    "support_tier": "framework_proxy",
                    "notes": "Proxy timing path not implemented for this format yet.",
                }

        for _ in range(warmup_runs):
            run_once()

        timings: list[float] = []
        checksum = 0.0
        for _ in range(measured_runs):
            start = time.perf_counter()
            checksum = run_once()
            timings.append((time.perf_counter() - start) * 1000.0)

        return {
            "backend": backend,
            "format": dtype_name,
            "workload": workload["id"],
            "status": "measured",
            "support_tier": "framework_proxy",
            "median_ms": statistics.median(timings),
            "p90_ms": percentile(timings, 0.9),
            "warmup_runs": warmup_runs,
            "measured_runs": measured_runs,
            "checksum": checksum,
            "notes": f"proxy_backend=torch_float32_{device}",
        }
    except Exception as exc:  # pragma: no cover - hardware dependent
        return {
            "backend": backend,
            "format": dtype_name,
            "workload": workload["id"],
            "status": "error",
            "support_tier": "unsupported_or_proxy",
            "notes": f"{type(exc).__name__}: {exc}",
        }


def expected_row_count(
    workloads: tuple[dict[str, object], ...],
    include_torch_native: bool = True,
    include_torch_proxy: bool = True,
    include_mlx: bool = True,
    include_coreml: bool = True,
) -> int:
    torch_native_rows = (2 * len(TORCH_DTYPES) * len(workloads)) if include_torch_native else 0
    torch_proxy_rows = ((2 * len(TORCH_PROXY_FORMATS) * len(workloads)) + (len(PLANNED_FRONTIER_FORMATS) * len(workloads))) if include_torch_proxy else 0
    torch_rows = torch_native_rows + torch_proxy_rows
    mlx_rows = ((2 * len(workloads)) + (len(TORCH_PROXY_FORMATS) * len(workloads)) + (5 * len(workloads))) if include_mlx else 0
    coreml_rows = ((4 * 2) + (4 * 9)) if include_coreml else 0
    return torch_rows + mlx_rows + coreml_rows


def percentile(values: list[float], q: float) -> float:
    if not values:
        return math.nan
    ordered = sorted(values)
    if len(ordered) == 1:
        return ordered[0]
    idx = (len(ordered) - 1) * q
    low = math.floor(idx)
    high = math.ceil(idx)
    if low == high:
        return ordered[low]
    frac = idx - low
    return ordered[low] * (1.0 - frac) + ordered[high] * frac


def torch_tensor(torch_mod, dtype_name: str, shape: tuple[int, ...]):
    dtype = getattr(torch_mod, TORCH_DTYPES[dtype_name])
    integer_types = {"int8", "uint8", "int16", "int32", "int64", "uint16", "uint32", "uint64"}
    if dtype_name in integer_types:
        unsigned_types = {"uint8", "uint16", "uint32", "uint64"}
        low = 0 if dtype_name in unsigned_types else -64
        high = 128 if dtype_name in unsigned_types else 64
        return torch_mod.randint(low, high, shape, dtype=dtype)
    return torch_mod.randn(shape, dtype=dtype)


def time_torch_backend(
    torch_mod,
    backend: str,
    dtype_name: str,
    workload: dict[str, object],
    warmup_runs: int,
    measured_runs: int,
) -> dict[str, object]:
    if backend == "pytorch_mps" and not torch_mod.backends.mps.is_available():
        return {
            "backend": backend,
            "format": dtype_name,
            "workload": workload["id"],
            "status": "unavailable",
            "support_tier": "unsupported",
            "notes": "torch.backends.mps.is_available() is false",
        }

    if dtype_name not in TORCH_DTYPES:
        return {
            "backend": backend,
            "format": dtype_name,
            "workload": workload["id"],
            "status": "planned",
            "support_tier": "framework_proxy",
            "notes": "No direct torch dtype mapping; keep as frontier proxy row.",
        }

    device = "cpu" if backend == "pytorch_cpu" else "mps"

    try:
        if workload["kind"] == "gemm":
            shape_a = (int(workload["m"]), int(workload["k"]))
            shape_b = (int(workload["k"]), int(workload["n"]))
            a = torch_tensor(torch_mod, dtype_name, shape_a).to(device)
            b = torch_tensor(torch_mod, dtype_name, shape_b).to(device)

            def run_once():
                out = a @ b
                if device == "mps":
                    torch_mod.mps.synchronize()
                return float(out.reshape(-1)[0].item())

        else:
            size = int(workload["size"])
            x = torch_tensor(torch_mod, dtype_name, (size,)).to(device)

            def run_once():
                out = x.sum()
                if device == "mps":
                    torch_mod.mps.synchronize()
                return float(out.item())

        for _ in range(warmup_runs):
            run_once()

        timings: list[float] = []
        checksum = 0.0
        for _ in range(measured_runs):
            start = time.perf_counter()
            checksum = run_once()
            timings.append((time.perf_counter() - start) * 1000.0)

        return {
            "backend": backend,
            "format": dtype_name,
            "workload": workload["id"],
            "status": "measured",
            "support_tier": "native" if dtype_name in {"float16", "float32", "float64", "int32", "int64"} else "native_or_proxy",
            "median_ms": statistics.median(timings),
            "p90_ms": percentile(timings, 0.9),
            "warmup_runs": warmup_runs,
            "measured_runs": measured_runs,
            "checksum": checksum,
            "notes": f"device={device}",
        }
    except Exception as exc:  # pragma: no cover - hardware dependent
        return {
            "backend": backend,
            "format": dtype_name,
            "workload": workload["id"],
            "status": "error",
            "support_tier": "unsupported_or_proxy",
            "notes": f"{type(exc).__name__}: {exc}",
        }


def probe_torch(
    workloads: tuple[dict[str, object], ...],
    warmup_runs: int,
    measured_runs: int,
    include_native: bool = True,
    include_proxy: bool = True,
    rows: list[dict[str, object]] | None = None,
    on_row_start=None,
    on_row=None,
) -> list[dict[str, object]]:
    rows = [] if rows is None else rows
    try:
        import torch
    except Exception as exc:  # pragma: no cover - environment dependent
        fallback_rows = [{
            "backend": "pytorch_cpu",
            "format": "float32",
            "workload": "environment",
            "status": "missing_dependency",
            "support_tier": "unknown",
            "notes": f"{type(exc).__name__}: {exc}",
        }]
        rows.extend(fallback_rows)
        if on_row is not None:
            for row in fallback_rows:
                on_row("torch", row)
        return rows

    if include_native:
        for backend in ("pytorch_cpu", "pytorch_mps"):
            for dtype_name in ("float16", "float32", "float64", "bfloat16", "int8", "uint8",
                               "int16", "int32", "int64", "uint16", "uint32", "uint64"):
                for workload in workloads:
                    if on_row_start is not None:
                        on_row_start("torch", backend, dtype_name, str(workload["id"]))
                    row = time_torch_backend(torch, backend, dtype_name, workload, warmup_runs, measured_runs)
                    rows.append(row)
                    if on_row is not None:
                        on_row("torch", row)

    if include_proxy:
        for backend, device in (("pytorch_proxy_cpu", "cpu"), ("pytorch_proxy_mps", "mps")):
            if backend == "pytorch_proxy_mps" and not torch.backends.mps.is_available():
                for dtype_name in TORCH_PROXY_FORMATS + PLANNED_FRONTIER_FORMATS:
                    for workload in workloads:
                        if on_row_start is not None:
                            on_row_start("torch", backend, dtype_name, str(workload["id"]))
                        row = {
                            "backend": backend,
                            "format": dtype_name,
                            "workload": workload["id"],
                            "status": "unavailable",
                            "support_tier": "unsupported_or_proxy",
                            "notes": "torch.backends.mps.is_available() is false",
                        }
                        rows.append(row)
                        if on_row is not None:
                            on_row("torch", row)
                continue

            for dtype_name in TORCH_PROXY_FORMATS:
                for workload in workloads:
                    if on_row_start is not None:
                        on_row_start("torch", backend, dtype_name, str(workload["id"]))
                    row = time_torch_proxy_backend(torch, backend, device, dtype_name, workload, warmup_runs, measured_runs)
                    rows.append(row)
                    if on_row is not None:
                        on_row("torch", row)

            for dtype_name in PLANNED_FRONTIER_FORMATS:
                for workload in workloads:
                    if on_row_start is not None:
                        on_row_start("torch", backend, dtype_name, str(workload["id"]))
                    row = {
                        "backend": backend,
                        "format": dtype_name,
                        "workload": workload["id"],
                        "status": "planned",
                        "support_tier": "framework_proxy",
                        "notes": f"Reserved frontier row for future torch proxy coverage on {device}.",
                    }
                    rows.append(row)
                    if on_row is not None:
                        on_row("torch", row)
    return rows


def probe_mlx(
    workloads: tuple[dict[str, object], ...],
    warmup_runs: int,
    measured_runs: int,
    rows: list[dict[str, object]] | None = None,
    on_row_start=None,
    on_row=None,
) -> list[dict[str, object]]:
    rows = [] if rows is None else rows
    try:
        import mlx.core as mx
    except Exception as exc:  # pragma: no cover - environment dependent
        fallback_rows = [{
            "backend": "mlx_gpu",
            "format": "float32",
            "workload": "environment",
            "status": "missing_dependency",
            "support_tier": "unknown",
            "notes": f"{type(exc).__name__}: {exc}",
        }]
        rows.extend(fallback_rows)
        if on_row is not None:
            for row in fallback_rows:
                on_row("mlx", row)
        return rows

    dtype_map = {
        "float16": mx.float16,
        "float32": mx.float32,
    }

    for dtype_name, dtype in dtype_map.items():
        for workload in workloads:
            try:
                if on_row_start is not None:
                    on_row_start("mlx", "mlx_gpu", dtype_name, str(workload["id"]))
                if workload["kind"] == "gemm":
                    a = mx.random.normal(shape=(int(workload["m"]), int(workload["k"])), dtype=dtype)
                    b = mx.random.normal(shape=(int(workload["k"]), int(workload["n"])), dtype=dtype)

                    def run_once():
                        out = a @ b
                        mx.eval(out)
                        return float(out[0, 0].item())

                else:
                    x = mx.random.normal(shape=(int(workload["size"]),), dtype=dtype)

                    def run_once():
                        out = mx.sum(x)
                        mx.eval(out)
                        return float(out.item())

                for _ in range(warmup_runs):
                    run_once()

                timings: list[float] = []
                checksum = 0.0
                for _ in range(measured_runs):
                    start = time.perf_counter()
                    checksum = run_once()
                    timings.append((time.perf_counter() - start) * 1000.0)

                row = {
                    "backend": "mlx_gpu",
                    "format": dtype_name,
                    "workload": workload["id"],
                    "status": "measured",
                    "support_tier": "native",
                    "median_ms": statistics.median(timings),
                    "p90_ms": percentile(timings, 0.9),
                    "warmup_runs": warmup_runs,
                    "measured_runs": measured_runs,
                    "checksum": checksum,
                    "notes": f"default_device={mx.default_device()}",
                }
                rows.append(row)
                if on_row is not None:
                    on_row("mlx", row)
            except Exception as exc:  # pragma: no cover - hardware dependent
                row = {
                    "backend": "mlx_gpu",
                    "format": dtype_name,
                    "workload": workload["id"],
                    "status": "error",
                    "support_tier": "unsupported_or_proxy",
                    "notes": f"{type(exc).__name__}: {exc}",
                }
                rows.append(row)
                if on_row is not None:
                    on_row("mlx", row)

    for dtype_name in TORCH_PROXY_FORMATS:
        for workload in workloads:
            if on_row_start is not None:
                on_row_start("mlx", "mlx_proxy", dtype_name, str(workload["id"]))
            row = time_mlx_proxy_backend(mx, dtype_name, workload, warmup_runs, measured_runs)
            rows.append(row)
            if on_row is not None:
                on_row("mlx", row)

    for dtype_name in ("bfloat16", "int8", "uint8") + PLANNED_FRONTIER_FORMATS:
        for workload in workloads:
            if on_row_start is not None:
                on_row_start("mlx", "mlx_proxy", dtype_name, str(workload["id"]))
            row = {
                "backend": "mlx_proxy",
                "format": dtype_name,
                "workload": workload["id"],
                "status": "planned",
                "support_tier": "framework_proxy",
                "notes": "Reserved frontier row for future MLX coverage.",
            }
            rows.append(row)
            if on_row is not None:
                on_row("mlx", row)
    return rows


def probe_coreml(
    workloads: tuple[dict[str, object], ...],
    rows: list[dict[str, object]] | None = None,
    on_row_start=None,
    on_row=None,
) -> list[dict[str, object]]:
    rows = [] if rows is None else rows
    try:
        import coremltools as ct
    except Exception as exc:  # pragma: no cover - environment dependent
        fallback_rows = [{
            "backend": "coreml_all",
            "format": "float32",
            "workload": "environment",
            "status": "missing_dependency",
            "support_tier": "unknown",
            "notes": f"{type(exc).__name__}: {exc}",
        }]
        rows.extend(fallback_rows)
        if on_row is not None:
            for row in fallback_rows:
                on_row("coreml", row)
        return rows

    compute_units = (
        ("coreml_cpu_only", "CPU_ONLY"),
        ("coreml_cpu_and_gpu", "CPU_AND_GPU"),
        ("coreml_cpu_and_ne", "CPU_AND_NE"),
        ("coreml_all", "ALL"),
    )
    for backend, unit_name in compute_units:
        if on_row_start is not None:
            on_row_start("coreml", backend, "float16", "tiny_linear")
        row = {
            "backend": backend,
            "format": "float16",
            "workload": "tiny_linear",
            "status": "availability_only",
            "support_tier": "placement_only",
            "notes": f"ComputeUnit.{unit_name} present in coremltools {ct.__version__}.",
        }
        rows.append(row)
        if on_row is not None:
            on_row("coreml", row)
        if on_row_start is not None:
            on_row_start("coreml", backend, "float32", "tiny_linear")
        row = {
            "backend": backend,
            "format": "float32",
            "workload": "tiny_linear",
            "status": "availability_only",
            "support_tier": "placement_only",
            "notes": f"ComputeUnit.{unit_name} present in coremltools {ct.__version__}.",
        }
        rows.append(row)
        if on_row is not None:
            on_row("coreml", row)

    for dtype_name in ("int8", "bfloat16", "tf32", "mxfp8", "mxfp6", "mxfp4", "mxint8", "nf4", "fp4"):
        for backend, _unit_name in compute_units:
            if on_row_start is not None:
                on_row_start("coreml", backend, dtype_name, "typed_proxy")
            row = {
                "backend": backend,
                "format": dtype_name,
                "workload": "typed_proxy",
                "status": "planned",
                "support_tier": "framework_proxy",
                "notes": "Keep frontier family visible for placement-only Core ML sweeps.",
            }
            rows.append(row)
            if on_row is not None:
                on_row("coreml", row)
    return rows


def write_csv(path: Path, rows: list[dict[str, object]]) -> None:
    fieldnames = [
        "backend",
        "format",
        "workload",
        "status",
        "support_tier",
        "median_ms",
        "p90_ms",
        "warmup_runs",
        "measured_runs",
        "checksum",
        "notes",
    ]
    with path.open("w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow({key: row.get(key, "") for key in fieldnames})


def persist_outputs(
    out_dir: Path,
    workloads: tuple[dict[str, object], ...],
    warmup_runs: int,
    measured_runs: int,
    rows_by_group: dict[str, list[dict[str, object]]],
    progress: dict[str, object],
) -> None:
    all_rows = rows_by_group["torch"] + rows_by_group["mlx"] + rows_by_group["coreml"]
    timing_rows = [row for row in all_rows if row.get("status") == "measured"]
    payload = {
        "python": {
            "executable": os.sys.executable,
            "version": os.sys.version.split()[0],
        },
        "warmup_runs": warmup_runs,
        "measured_runs": measured_runs,
        "workloads": list(workloads),
        "progress": progress,
        "rows": rows_by_group,
    }
    (out_dir / "neural_typed_matrix.json").write_text(json.dumps(payload, indent=2, sort_keys=True))
    write_csv(out_dir / "neural_backend_matrix.csv", all_rows)
    write_csv(out_dir / "neural_timing_matrix.csv", timing_rows)
    (out_dir / "neural_progress.json").write_text(json.dumps(progress, indent=2, sort_keys=True))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out-dir", required=True)
    parser.add_argument("--warmup-runs", type=int, default=DEFAULT_WARMUP_RUNS)
    parser.add_argument("--measured-runs", type=int, default=DEFAULT_MEASURED_RUNS)
    parser.add_argument("--workloads", default="all", help="Comma-separated workload ids or 'all'")
    parser.add_argument("--skip-torch-native", action="store_true", help="Skip the native PyTorch dtype sweep and run only proxy rows.")
    parser.add_argument("--skip-torch-proxy", action="store_true", help="Skip the PyTorch frontier proxy sweep.")
    parser.add_argument("--skip-mlx", action="store_true", help="Skip MLX rows entirely.")
    parser.add_argument("--skip-coreml", action="store_true", help="Skip Core ML rows entirely.")
    args = parser.parse_args()

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    if args.workloads == "all":
        workloads = WORKLOADS
    else:
        wanted = {item.strip() for item in args.workloads.split(",") if item.strip()}
        workloads = tuple(workload for workload in WORKLOADS if workload["id"] in wanted)
        if not workloads:
            raise SystemExit(f"no workloads matched: {args.workloads}")

    include_torch_native = not args.skip_torch_native
    include_torch_proxy = not args.skip_torch_proxy
    include_mlx = not args.skip_mlx
    include_coreml = not args.skip_coreml

    total_expected = expected_row_count(
        workloads,
        include_torch_native=include_torch_native,
        include_torch_proxy=include_torch_proxy,
        include_mlx=include_mlx,
        include_coreml=include_coreml,
    )
    rows = {"torch": [], "mlx": [], "coreml": []}
    progress = {
        "status": "running",
        "expected_rows": total_expected,
        "completed_rows": 0,
        "completed_groups": [],
        "last_group": "",
        "last_backend": "",
        "last_format": "",
        "last_workload": "",
    }

    def on_row(group_name: str, row: dict[str, object]) -> None:
        progress["completed_rows"] = sum(len(group_rows) for group_rows in rows.values())
        progress["completed_groups"] = [name for name, group_rows in rows.items() if group_rows]
        progress["last_group"] = group_name
        progress["last_backend"] = str(row.get("backend", ""))
        progress["last_format"] = str(row.get("format", ""))
        progress["last_workload"] = str(row.get("workload", ""))
        persist_outputs(out_dir, workloads, args.warmup_runs, args.measured_runs, rows, progress)

    def on_row_start(group_name: str, backend: str, dtype_name: str, workload_id: str) -> None:
        progress["last_group"] = group_name
        progress["last_backend"] = backend
        progress["last_format"] = dtype_name
        progress["last_workload"] = workload_id
        persist_outputs(out_dir, workloads, args.warmup_runs, args.measured_runs, rows, progress)

    persist_outputs(out_dir, workloads, args.warmup_runs, args.measured_runs, rows, progress)
    probe_torch(
        workloads,
        args.warmup_runs,
        args.measured_runs,
        include_native=include_torch_native,
        include_proxy=include_torch_proxy,
        rows=rows["torch"],
        on_row_start=on_row_start,
        on_row=on_row,
    )
    if include_mlx:
        probe_mlx(workloads, args.warmup_runs, args.measured_runs, rows=rows["mlx"], on_row_start=on_row_start, on_row=on_row)
    if include_coreml:
        probe_coreml(workloads, rows=rows["coreml"], on_row_start=on_row_start, on_row=on_row)
    progress["status"] = "completed"
    progress["completed_rows"] = sum(len(group_rows) for group_rows in rows.values())
    progress["completed_groups"] = [name for name, group_rows in rows.items() if group_rows]
    persist_outputs(out_dir, workloads, args.warmup_runs, args.measured_runs, rows, progress)
    all_rows = rows["torch"] + rows["mlx"] + rows["coreml"]
    timing_rows = [row for row in all_rows if row.get("status") == "measured"]
    print(json.dumps({
        "json": str(out_dir / "neural_typed_matrix.json"),
        "backend_csv": str(out_dir / "neural_backend_matrix.csv"),
        "timing_csv": str(out_dir / "neural_timing_matrix.csv"),
        "progress_json": str(out_dir / "neural_progress.json"),
        "row_count": len(all_rows),
        "timing_row_count": len(timing_rows),
    }, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
