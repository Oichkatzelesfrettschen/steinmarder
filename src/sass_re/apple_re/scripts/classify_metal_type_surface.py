#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import json
import re
import shutil
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[1]
COMPILE_SCRIPT = ROOT_DIR / "scripts" / "compile_metal_probe.sh"
BUILD_HOST_SCRIPT = ROOT_DIR / "scripts" / "build_metal_probe_host.sh"
DEFAULT_HOST_BIN = ROOT_DIR / "results" / "metal_type_surface_host" / "sm_apple_metal_probe_host"
LLVM_DIS_BIN = shutil.which("llvm-dis") or (
    "/opt/homebrew/opt/llvm/bin/llvm-dis" if Path("/opt/homebrew/opt/llvm/bin/llvm-dis").exists() else None
)


@dataclass(frozen=True)
class ProbeCase:
    format: str
    bit_width: str
    classification_mode: str
    support_tier: str
    msl_type: str
    kernel_name: str
    note: str
    source: str


DIRECT_CASES = [
    ProbeCase(
        format="float32",
        bit_width="32",
        classification_mode="native_surface",
        support_tier="native_or_lowered",
        msl_type="float",
        kernel_name="probe_float32_surface",
        note="Public MSL float surface compiled; still needs AIR review to split native from lowered.",
        source=r"""
#include <metal_stdlib>
using namespace metal;

kernel void probe_float32_surface(device uint *out [[buffer(0)]],
                                  uint gid [[thread_position_in_grid]]) {
    float a = float((gid & 255u) + 1u) * 0.25f;
    float b = float(((gid * 17u) & 255u) + 3u) * 0.125f;
    float c = fma(a, 0.75f, b);
    out[gid] = as_type<uint>(c);
}
""".strip(),
    ),
    ProbeCase(
        format="float16",
        bit_width="16",
        classification_mode="native_surface",
        support_tier="native_or_lowered",
        msl_type="half",
        kernel_name="probe_float16_surface",
        note="Public MSL half surface compiled; timing remains a host-visible dispatch proxy.",
        source=r"""
#include <metal_stdlib>
using namespace metal;

kernel void probe_float16_surface(device uint *out [[buffer(0)]],
                                  uint gid [[thread_position_in_grid]]) {
    half a = half(float((gid & 127u) + 1u) * 0.5f);
    half b = half(float(((gid * 5u) & 127u) + 3u) * 0.25f);
    half c = a * half(0.75f) + b;
    out[gid] = as_type<uint>(float(c));
}
""".strip(),
    ),
    ProbeCase(
        format="bfloat16",
        bit_width="16",
        classification_mode="native_surface",
        support_tier="native_or_lowered",
        msl_type="bfloat",
        kernel_name="probe_bfloat16_surface",
        note="Public MSL bfloat surface compiled on this machine; keep classification honest until AIR/native lowering is separated.",
        source=r"""
#include <metal_stdlib>
using namespace metal;

kernel void probe_bfloat16_surface(device uint *out [[buffer(0)]],
                                   uint gid [[thread_position_in_grid]]) {
    bfloat a = bfloat(float((gid & 255u) + 1u) * 0.25f);
    bfloat b = bfloat(float(((gid * 9u) & 255u) + 5u) * 0.125f);
    bfloat c = a * bfloat(0.5f) + b;
    out[gid] = as_type<uint>(float(c));
}
""".strip(),
    ),
    ProbeCase(
        format="float64",
        bit_width="64",
        classification_mode="native_surface",
        support_tier="native_or_lowered",
        msl_type="double",
        kernel_name="probe_float64_surface",
        note="Public MSL double surface compiled; native-vs-lowered still needs AIR/disassembly evidence.",
        source=r"""
#include <metal_stdlib>
using namespace metal;

kernel void probe_float64_surface(device uint *out [[buffer(0)]],
                                  uint gid [[thread_position_in_grid]]) {
    double a = double((gid & 255u) + 1u) * 0.25;
    double b = double(((gid * 17u) & 255u) + 3u) * 0.125;
    double c = fma(a, 0.75, b);
    ulong bits = as_type<ulong>(c);
    out[gid] = uint(bits ^ (bits >> 32));
}
""".strip(),
    ),
    ProbeCase(
        format="int32",
        bit_width="32",
        classification_mode="native_surface",
        support_tier="native_or_lowered",
        msl_type="int",
        kernel_name="probe_int32_surface",
        note="Public MSL int surface compiled.",
        source=r"""
#include <metal_stdlib>
using namespace metal;

kernel void probe_int32_surface(device uint *out [[buffer(0)]],
                                uint gid [[thread_position_in_grid]]) {
    int a = int(gid) * 13 + 7;
    int b = int(gid) ^ 0x55aa55aa;
    int c = (a + b) ^ (a << 1);
    out[gid] = as_type<uint>(c);
}
""".strip(),
    ),
    ProbeCase(
        format="int16",
        bit_width="16",
        classification_mode="native_surface",
        support_tier="native_or_lowered",
        msl_type="short",
        kernel_name="probe_int16_surface",
        note="Packed 16-bit signed surface compiled through public MSL short.",
        source=r"""
#include <metal_stdlib>
using namespace metal;

kernel void probe_int16_surface(device uint *out [[buffer(0)]],
                                uint gid [[thread_position_in_grid]]) {
    short a = short((gid & 0x7fffu) - 1024u);
    short b = short(((gid * 5u) & 0x7fffu) - 256u);
    short c = short(a + b);
    out[gid] = uint(ushort(c));
}
""".strip(),
    ),
    ProbeCase(
        format="uint32",
        bit_width="32",
        classification_mode="native_surface",
        support_tier="native_or_lowered",
        msl_type="uint",
        kernel_name="probe_uint32_surface",
        note="Public MSL uint surface compiled.",
        source=r"""
#include <metal_stdlib>
using namespace metal;

kernel void probe_uint32_surface(device uint *out [[buffer(0)]],
                                 uint gid [[thread_position_in_grid]]) {
    uint a = gid * 13u + 7u;
    uint b = gid ^ 0x55aa55aau;
    out[gid] = (a + b) ^ (a << 1);
}
""".strip(),
    ),
    ProbeCase(
        format="uint16",
        bit_width="16",
        classification_mode="native_surface",
        support_tier="native_or_lowered",
        msl_type="ushort",
        kernel_name="probe_uint16_surface",
        note="Packed 16-bit unsigned surface compiled through public MSL ushort.",
        source=r"""
#include <metal_stdlib>
using namespace metal;

kernel void probe_uint16_surface(device uint *out [[buffer(0)]],
                                 uint gid [[thread_position_in_grid]]) {
    ushort a = ushort(gid & 0xffffu);
    ushort b = ushort((gid * 7u) & 0xffffu);
    ushort c = ushort(a + b);
    out[gid] = uint(c);
}
""".strip(),
    ),
    ProbeCase(
        format="int8",
        bit_width="8",
        classification_mode="native_surface",
        support_tier="native_or_lowered",
        msl_type="char",
        kernel_name="probe_int8_surface",
        note="Packed 8-bit signed surface compiled through public MSL char.",
        source=r"""
#include <metal_stdlib>
using namespace metal;

kernel void probe_int8_surface(device uint *out [[buffer(0)]],
                               uint gid [[thread_position_in_grid]]) {
    char a = char((gid & 0x7fu) - 64u);
    char b = char(((gid * 3u) & 0x7fu) - 32u);
    char c = char(a + b);
    out[gid] = uint(uchar(c));
}
""".strip(),
    ),
    ProbeCase(
        format="int64",
        bit_width="64",
        classification_mode="native_surface",
        support_tier="native_or_lowered",
        msl_type="long",
        kernel_name="probe_int64_surface",
        note="Public MSL long surface compiled; native-vs-lowered still needs AIR/disassembly evidence.",
        source=r"""
#include <metal_stdlib>
using namespace metal;

kernel void probe_int64_surface(device uint *out [[buffer(0)]],
                                uint gid [[thread_position_in_grid]]) {
    long a = long(gid) * 13l + 7l;
    long b = long(gid) ^ 0x55aa55aal;
    long c = (a + b) ^ (a << 1);
    ulong bits = as_type<ulong>(c);
    out[gid] = uint(bits ^ (bits >> 32));
}
""".strip(),
    ),
    ProbeCase(
        format="uint8",
        bit_width="8",
        classification_mode="native_surface",
        support_tier="native_or_lowered",
        msl_type="uchar",
        kernel_name="probe_uint8_surface",
        note="Packed 8-bit unsigned surface compiled through public MSL uchar.",
        source=r"""
#include <metal_stdlib>
using namespace metal;

kernel void probe_uint8_surface(device uint *out [[buffer(0)]],
                                uint gid [[thread_position_in_grid]]) {
    uchar a = uchar(gid & 0xffu);
    uchar b = uchar((gid * 7u) & 0xffu);
    uchar c = uchar(a + b);
    out[gid] = uint(c);
}
""".strip(),
    ),
    ProbeCase(
        format="uint64",
        bit_width="64",
        classification_mode="native_surface",
        support_tier="native_or_lowered",
        msl_type="ulong",
        kernel_name="probe_uint64_surface",
        note="Public MSL ulong surface compiled; native-vs-lowered still needs AIR/disassembly evidence.",
        source=r"""
#include <metal_stdlib>
using namespace metal;

kernel void probe_uint64_surface(device uint *out [[buffer(0)]],
                                 uint gid [[thread_position_in_grid]]) {
    ulong a = ulong(gid) * 13ul + 7ul;
    ulong b = ulong(gid) ^ 0x55aa55aaul;
    ulong c = (a + b) ^ (a << 1);
    out[gid] = uint(c ^ (c >> 32));
}
""".strip(),
    ),
]


FRONTIER_HELPERS = r"""
#include <metal_stdlib>
using namespace metal;

inline float quantize_tf32_proxy(float x) {
    uint bits = as_type<uint>(x);
    bits &= 0xffffe000u;
    return as_type<float>(bits);
}

inline float quantize_minifloat_proxy(float x, uint exp_bits, uint mant_bits, int exp_bias) {
    if (x == 0.0f) {
        return 0.0f;
    }
    float sign = x < 0.0f ? -1.0f : 1.0f;
    float ax = fabs(x);
    int exponent = 0;
    float mant = frexp(ax, exponent);
    (void)mant;
    int normal_exp = exponent - 1;
    int min_normal = 1 - exp_bias;
    int max_normal = int((1u << exp_bits) - 2u) - exp_bias;
    if (normal_exp < min_normal) {
        return 0.0f;
    }
    if (normal_exp > max_normal) {
        normal_exp = max_normal;
    }
    float normalized = ldexp(ax, -normal_exp);
    float frac = clamp(normalized - 1.0f, 0.0f, 1.0f);
    float steps = float(1u << mant_bits);
    float qfrac = floor(frac * steps + 0.5f) / steps;
    qfrac = min(qfrac, (steps - 1.0f) / steps);
    return sign * ldexp(1.0f + qfrac, normal_exp);
}

inline float quantize_mxint8_proxy(float x, float scale) {
    float normalized = clamp(x / scale, -127.0f, 127.0f);
    float q = floor(normalized + (normalized >= 0.0f ? 0.5f : -0.5f));
    return q * scale;
}

inline float threadgroup_scale_proxy(uint gid,
                                     uint lid,
                                     uint tg_size,
                                     threadgroup float *scratch) {
    float base = float((gid & 255u) + 1u) * 0.0625f;
    float alt = float(((gid * 17u) & 255u) + 3u) * 0.03125f;
    float x = (float(int(gid & 1u) * 2 - 1) * base) + alt;
    scratch[lid] = fabs(x);
    threadgroup_barrier(mem_flags::mem_threadgroup);
    for (uint stride = tg_size >> 1; stride > 0; stride >>= 1) {
        if (lid < stride) {
            scratch[lid] = max(scratch[lid], scratch[lid + stride]);
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    return max(scratch[0], 1.0f);
}
""".strip()


FRONTIER_CASES = [
    ProbeCase(
        format="int4",
        bit_width="4",
        classification_mode="frontier_proxy",
        support_tier="host_proxy",
        msl_type="int4_proxy",
        kernel_name="probe_int4_proxy",
        note="No public MSL int4 scalar exists; this kernel uses signed nibble quantization as an honest packed proxy.",
        source=FRONTIER_HELPERS + r"""

kernel void probe_int4_proxy(device uint *out [[buffer(0)]],
                             uint gid [[thread_position_in_grid]]) {
    int raw = int((gid * 5u) & 0x1fu) - 8;
    int q = clamp(raw, -8, 7);
    out[gid] = uint(q & 0xf);
}
""".strip(),
    ),
    ProbeCase(
        format="uint4",
        bit_width="4",
        classification_mode="frontier_proxy",
        support_tier="host_proxy",
        msl_type="uint4_proxy",
        kernel_name="probe_uint4_proxy",
        note="No public MSL uint4 scalar exists; this kernel uses unsigned nibble quantization as an honest packed proxy.",
        source=FRONTIER_HELPERS + r"""

kernel void probe_uint4_proxy(device uint *out [[buffer(0)]],
                              uint gid [[thread_position_in_grid]]) {
    uint raw = (gid * 7u) & 0x1fu;
    uint q = min(raw, 15u);
    out[gid] = q & 0xfu;
}
""".strip(),
    ),
    ProbeCase(
        format="fp4",
        bit_width="4",
        classification_mode="frontier_proxy",
        support_tier="host_proxy",
        msl_type="fp4_proxy",
        kernel_name="probe_fp4_proxy",
        note="No public MSL FP4 scalar exists; this kernel uses a 2e1m-style minifloat proxy without microscaling.",
        source=FRONTIER_HELPERS + r"""

kernel void probe_fp4_proxy(device uint *out [[buffer(0)]],
                            uint gid [[thread_position_in_grid]]) {
    float x = float((gid & 63u) + 1u) * 0.03125f;
    float q = quantize_minifloat_proxy(x, 2u, 1u, 1);
    out[gid] = as_type<uint>(q);
}
""".strip(),
    ),
    ProbeCase(
        format="tf32",
        bit_width="19",
        classification_mode="frontier_proxy",
        support_tier="host_proxy",
        msl_type="float_proxy",
        kernel_name="probe_tf32_proxy",
        note="No public MSL TF32 scalar exists; this kernel uses mantissa truncation as an honest TF32-style proxy.",
        source=FRONTIER_HELPERS + r"""

kernel void probe_tf32_proxy(device uint *out [[buffer(0)]],
                             uint gid [[thread_position_in_grid]]) {
    float a = float((gid & 1023u) + 1u) * 0.25f;
    float b = float(((gid * 9u) & 1023u) + 5u) * 0.125f;
    float qa = quantize_tf32_proxy(a);
    float qb = quantize_tf32_proxy(b);
    float qc = quantize_tf32_proxy(fma(qa, qb, qa + qb));
    out[gid] = as_type<uint>(qc);
}
""".strip(),
    ),
    ProbeCase(
        format="fp8_e4m3",
        bit_width="8",
        classification_mode="frontier_proxy",
        support_tier="host_proxy",
        msl_type="fp8_proxy_e4m3",
        kernel_name="probe_fp8_e4m3_proxy",
        note="No public MSL FP8 e4m3 scalar exists; this is a software minifloat proxy.",
        source=FRONTIER_HELPERS + r"""

kernel void probe_fp8_e4m3_proxy(device uint *out [[buffer(0)]],
                                 uint gid [[thread_position_in_grid]]) {
    float a = float((gid & 255u) + 1u) * 0.125f;
    float b = float(((gid * 13u) & 255u) + 7u) * 0.0625f;
    float qa = quantize_minifloat_proxy(a, 4u, 3u, 7);
    float qb = quantize_minifloat_proxy(b, 4u, 3u, 7);
    float qc = quantize_minifloat_proxy(qa * qb + qa, 4u, 3u, 7);
    out[gid] = as_type<uint>(qc);
}
""".strip(),
    ),
    ProbeCase(
        format="fp8_e5m2",
        bit_width="8",
        classification_mode="frontier_proxy",
        support_tier="host_proxy",
        msl_type="fp8_proxy_e5m2",
        kernel_name="probe_fp8_e5m2_proxy",
        note="No public MSL FP8 e5m2 scalar exists; this is a software minifloat proxy.",
        source=FRONTIER_HELPERS + r"""

kernel void probe_fp8_e5m2_proxy(device uint *out [[buffer(0)]],
                                 uint gid [[thread_position_in_grid]]) {
    float a = float((gid & 511u) + 1u) * 0.0625f;
    float b = float(((gid * 11u) & 511u) + 9u) * 0.03125f;
    float qa = quantize_minifloat_proxy(a, 5u, 2u, 15);
    float qb = quantize_minifloat_proxy(b, 5u, 2u, 15);
    float qc = quantize_minifloat_proxy(qa * qb + qb, 5u, 2u, 15);
    out[gid] = as_type<uint>(qc);
}
""".strip(),
    ),
    ProbeCase(
        format="mxfp8",
        bit_width="8",
        classification_mode="frontier_proxy",
        support_tier="host_proxy",
        msl_type="mx_proxy_fp8",
        kernel_name="probe_mxfp8_proxy",
        note="MXFP8 is approximated as threadgroup-scale microscaling plus FP8 e4m3-style quantization.",
        source=FRONTIER_HELPERS + r"""

kernel void probe_mxfp8_proxy(device uint *out [[buffer(0)]],
                              uint3 gid3 [[thread_position_in_grid]],
                              uint3 lid3 [[thread_position_in_threadgroup]],
                              uint3 threads_per_tg [[threads_per_threadgroup]]) {
    threadgroup float scratch[128];
    uint gid = gid3.x;
    uint lid = lid3.x;
    uint tg_size = min(threads_per_tg.x, 128u);
    float scale = threadgroup_scale_proxy(gid, lid, tg_size, scratch);
    float x = float((gid & 255u) + 1u) * 0.0625f;
    float q = quantize_minifloat_proxy(x / scale, 4u, 3u, 7) * scale;
    out[gid] = as_type<uint>(q);
}
""".strip(),
    ),
    ProbeCase(
        format="mxint8",
        bit_width="8",
        classification_mode="frontier_proxy",
        support_tier="host_proxy",
        msl_type="mx_proxy_int8",
        kernel_name="probe_mxint8_proxy",
        note="MXINT8 is approximated as threadgroup-scale quantize/dequantize with signed int8 range.",
        source=FRONTIER_HELPERS + r"""

kernel void probe_mxint8_proxy(device uint *out [[buffer(0)]],
                               uint3 gid3 [[thread_position_in_grid]],
                               uint3 lid3 [[thread_position_in_threadgroup]],
                               uint3 threads_per_tg [[threads_per_threadgroup]]) {
    threadgroup float scratch[128];
    uint gid = gid3.x;
    uint lid = lid3.x;
    uint tg_size = min(threads_per_tg.x, 128u);
    float scale = threadgroup_scale_proxy(gid, lid, tg_size, scratch) / 127.0f;
    float x = float((gid & 255u) + 1u) * 0.5f;
    float q = quantize_mxint8_proxy(x, scale);
    out[gid] = as_type<uint>(q);
}
""".strip(),
    ),
    ProbeCase(
        format="mxfp6",
        bit_width="6",
        classification_mode="frontier_proxy",
        support_tier="host_proxy",
        msl_type="mx_proxy_fp6",
        kernel_name="probe_mxfp6_proxy",
        note="MXFP6 is approximated as threadgroup-scale microscaling plus a 3e2m-style minifloat proxy.",
        source=FRONTIER_HELPERS + r"""

kernel void probe_mxfp6_proxy(device uint *out [[buffer(0)]],
                              uint3 gid3 [[thread_position_in_grid]],
                              uint3 lid3 [[thread_position_in_threadgroup]],
                              uint3 threads_per_tg [[threads_per_threadgroup]]) {
    threadgroup float scratch[128];
    uint gid = gid3.x;
    uint lid = lid3.x;
    uint tg_size = min(threads_per_tg.x, 128u);
    float scale = threadgroup_scale_proxy(gid, lid, tg_size, scratch);
    float x = float((gid & 127u) + 1u) * 0.0625f;
    float q = quantize_minifloat_proxy(x / scale, 3u, 2u, 3) * scale;
    out[gid] = as_type<uint>(q);
}
""".strip(),
    ),
    ProbeCase(
        format="mxfp4",
        bit_width="4",
        classification_mode="frontier_proxy",
        support_tier="host_proxy",
        msl_type="mx_proxy_fp4",
        kernel_name="probe_mxfp4_proxy",
        note="MXFP4 is approximated as threadgroup-scale microscaling plus a 2e1m-style minifloat proxy.",
        source=FRONTIER_HELPERS + r"""

kernel void probe_mxfp4_proxy(device uint *out [[buffer(0)]],
                              uint3 gid3 [[thread_position_in_grid]],
                              uint3 lid3 [[thread_position_in_threadgroup]],
                              uint3 threads_per_tg [[threads_per_threadgroup]]) {
    threadgroup float scratch[128];
    uint gid = gid3.x;
    uint lid = lid3.x;
    uint tg_size = min(threads_per_tg.x, 128u);
    float scale = threadgroup_scale_proxy(gid, lid, tg_size, scratch);
    float x = float((gid & 63u) + 1u) * 0.03125f;
    float q = quantize_minifloat_proxy(x / scale, 2u, 1u, 1) * scale;
    out[gid] = as_type<uint>(q);
}
""".strip(),
    ),
]


def ensure_host_binary(host_bin: Path) -> Path:
    if host_bin.exists():
        return host_bin
    host_bin.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [str(BUILD_HOST_SCRIPT), str(ROOT_DIR / "host" / "metal_probe_host.m"), str(host_bin.parent)],
        check=True,
        capture_output=True,
        text=True,
    )
    return host_bin


def write_shader_source(path: Path, source: str) -> None:
    path.write_text(source + "\n", encoding="utf-8")


def disassemble_air(air_path: Path) -> str | None:
    if LLVM_DIS_BIN is None or not air_path.exists():
        return None
    proc = subprocess.run(
        [LLVM_DIS_BIN, str(air_path), "-o", "-"],
        capture_output=True,
        text=True,
    )
    if proc.returncode != 0:
        return None
    return proc.stdout


def classify_air_support(case: ProbeCase, air_text: str | None) -> tuple[str, str]:
    if case.classification_mode != "native_surface":
        return case.support_tier, case.note
    if air_text is None:
        return case.support_tier, f"{case.note} AIR evidence unavailable on this host."

    native_patterns = {
        "float32": (r"@air\.fma\.f32", r"fmul fast float", r"store float"),
        "float16": (r"fmul fast half", r"fadd fast half", r"fpext half .* to float"),
        "bfloat16": (r"fmul fast bfloat", r"fadd fast bfloat", r"fpext bfloat .* to float"),
        "int32": (r"mul(?: [a-z]+)? i32", r"add(?: [a-z]+)? i32", r"store i32"),
        "uint32": (r"mul i32", r"add i32", r"store i32"),
        "int64": (r"mul .* i64", r"xor i64", r"trunc i64 .* to i32"),
        "uint64": (r"mul .* i64", r"xor i64", r"trunc i64 .* to i32"),
    }
    lowered_patterns = {
        "int16": (r"shl i32", r"lshr exact i32", r"and i32 .*65535"),
        "uint16": (r"shl i32", r"and i32 .*65528|65535"),
        "int8": (r"shl i32", r"lshr exact i32", r"and i32 .*255"),
        "uint8": (r"shl i32", r"and i32 .*248|255"),
    }

    if case.format in native_patterns and all(re.search(pattern, air_text) for pattern in native_patterns[case.format]):
        return "native", f"{case.note} AIR evidence shows direct {case.msl_type}-typed arithmetic."
    if case.format in lowered_patterns and all(re.search(pattern, air_text) for pattern in lowered_patterns[case.format]):
        return "compiler_lowered", f"{case.note} AIR evidence shows the public {case.msl_type} surface widened into i32 operations."
    return case.support_tier, f"{case.note} AIR evidence was inconclusive."


def compile_case(case: ProbeCase, build_dir: Path) -> tuple[str, str, Path | None, Path | None, str]:
    source_path = build_dir / f"{case.kernel_name}.metal"
    write_shader_source(source_path, case.source)
    proc = subprocess.run(
        [str(COMPILE_SCRIPT), str(source_path), str(build_dir)],
        capture_output=True,
        text=True,
    )
    if proc.returncode != 0:
        note = (proc.stderr or proc.stdout or "").strip().replace("\n", " ")
        return "failed", note[:800], None, None, "unsupported_or_unknown"
    metallib_path = build_dir / f"{case.kernel_name}.metallib"
    if not metallib_path.exists():
        return "failed", "compile script returned success but metallib was not found", None, None, "unsupported_or_unknown"
    air_path = build_dir / f"{case.kernel_name}.air"
    air_text = disassemble_air(air_path)
    support_tier, note = classify_air_support(case, air_text)
    return "compiled", note, metallib_path, (air_path if air_path.exists() else None), support_tier


def parse_host_csv(stdout: str) -> dict[str, str] | None:
    lines = [line.strip() for line in stdout.splitlines() if line.strip()]
    if len(lines) < 2:
        return None
    header = lines[-2].split(",")
    values = lines[-1].split(",")
    if len(header) != len(values):
        return None
    return dict(zip(header, values))


def time_case(
    case: ProbeCase,
    host_bin: Path,
    metallib: Path,
    width: int,
    iters: int,
) -> tuple[str, dict[str, str] | None, str]:
    proc = subprocess.run(
        [
            str(host_bin),
            "--metallib",
            str(metallib),
            "--kernel",
            case.kernel_name,
            "--width",
            str(width),
            "--iters",
            str(iters),
            "--csv",
        ],
        capture_output=True,
        text=True,
    )
    if proc.returncode != 0:
        note = (proc.stderr or proc.stdout or "").strip().replace("\n", " ")
        return "failed", None, note[:800]
    parsed = parse_host_csv(proc.stdout)
    if parsed is None:
        return "failed", None, "host harness did not return parsable CSV timing output"
    return "timed", parsed, case.note


def build_rows(cases: list[ProbeCase], host_bin: Path, build_dir: Path, width: int, iters: int, do_timing: bool) -> tuple[list[dict[str, str]], list[dict[str, str]]]:
    boundary_rows: list[dict[str, str]] = []
    timing_rows: list[dict[str, str]] = []
    for case in cases:
        compile_status, notes, metallib, air_path, support_tier = compile_case(case, build_dir)
        timing_status = "not_run"
        if compile_status == "compiled" and do_timing and metallib is not None:
            timing_status, timing_data, timing_notes = time_case(case, host_bin, metallib, width, iters)
            if timing_status == "timed" and timing_data is not None:
                timing_rows.append({
                    "format": case.format,
                    "bit_width": case.bit_width,
                    "classification_mode": case.classification_mode,
                    "kernel_name": case.kernel_name,
                    "support_tier": support_tier,
                    "measurement_backend": "cpu_wall",
                    "iters": timing_data.get("iters", str(iters)),
                    "width": timing_data.get("width", str(width)),
                    "elapsed_ns": timing_data.get("elapsed_ns", ""),
                    "ns_per_iter": timing_data.get("ns_per_iter", ""),
                    "ns_per_element": timing_data.get("ns_per_element", ""),
                    "checksum": timing_data.get("checksum", ""),
                    "notes": timing_notes,
                })
            else:
                notes = f"{notes} Timing failed: {timing_notes}".strip()
        boundary_rows.append({
            "format": case.format,
            "bit_width": case.bit_width,
            "classification_mode": case.classification_mode,
            "msl_type": case.msl_type,
            "kernel_name": case.kernel_name,
            "compile_status": compile_status,
            "support_tier": support_tier if compile_status == "compiled" else "unsupported_or_unknown",
            "timing_status": timing_status,
            "notes": notes if air_path is None else f"{notes} air={air_path.name}",
        })
    return boundary_rows, timing_rows


def write_csv(path: Path, rows: list[dict[str, str]], fieldnames: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(fh, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True, help="CSV output path for type-surface rows")
    parser.add_argument("--json-out", help="JSON output path for type-surface rows")
    parser.add_argument("--timing-out", help="CSV output path for timing rows")
    parser.add_argument("--build-dir", help="Directory for generated probe sources and metallibs")
    parser.add_argument("--host-bin", help="Existing metal probe host binary")
    parser.add_argument("--width", type=int, default=4096)
    parser.add_argument("--iters", type=int, default=256)
    parser.add_argument("--skip-timing", action="store_true")
    args = parser.parse_args()

    out_path = Path(args.out).resolve()
    json_path = Path(args.json_out).resolve() if args.json_out else None
    timing_path = Path(args.timing_out).resolve() if args.timing_out else None
    build_dir = Path(args.build_dir).resolve() if args.build_dir else Path(tempfile.mkdtemp(prefix="steinmarder_metal_type_surface_"))
    build_dir.mkdir(parents=True, exist_ok=True)

    host_bin = Path(args.host_bin).resolve() if args.host_bin else DEFAULT_HOST_BIN
    host_bin = ensure_host_binary(host_bin)

    cases = DIRECT_CASES + FRONTIER_CASES
    boundary_rows, timing_rows = build_rows(
        cases=cases,
        host_bin=host_bin,
        build_dir=build_dir,
        width=args.width,
        iters=args.iters,
        do_timing=not args.skip_timing,
    )

    boundary_fields = [
        "format",
        "bit_width",
        "classification_mode",
        "msl_type",
        "kernel_name",
        "compile_status",
        "support_tier",
        "timing_status",
        "notes",
    ]
    timing_fields = [
        "format",
        "bit_width",
        "classification_mode",
        "kernel_name",
        "support_tier",
        "measurement_backend",
        "iters",
        "width",
        "elapsed_ns",
        "ns_per_iter",
        "ns_per_element",
        "checksum",
        "notes",
    ]
    write_csv(out_path, boundary_rows, boundary_fields)
    if timing_path is not None:
        write_csv(timing_path, timing_rows, timing_fields)
    if json_path is not None:
        json_path.parent.mkdir(parents=True, exist_ok=True)
        json_path.write_text(json.dumps(boundary_rows, indent=2), encoding="utf-8")

    print(json.dumps({
        "boundary_out": str(out_path),
        "timing_out": str(timing_path) if timing_path else None,
        "rows": len(boundary_rows),
        "timed_rows": len(timing_rows),
        "build_dir": str(build_dir),
        "host_bin": str(host_bin),
    }, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
