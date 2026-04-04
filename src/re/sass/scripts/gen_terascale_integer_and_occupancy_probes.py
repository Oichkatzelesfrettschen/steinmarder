#!/usr/bin/env python3
"""Generate plain integer baseline and occupancy ladder probes for TeraScale.

These probes are intentionally simple and repetitive because we want stable
cross-lane comparisons, not hand-tuned one-off kernels.
"""

from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OPENCL_DIR = ROOT / "probes" / "r600" / "opencl"
VK_DIR = ROOT / "probes" / "r600" / "vulkan_compute"


DEPCHAIN_SPECS = [
    ("u8", "uchar", "uint", "0xffu", "convert_float(acc)"),
    ("i8", "char", "int", "0x7f", "(float)acc"),
    ("u16", "ushort", "uint", "0xffffu", "convert_float(acc)"),
    ("i16", "short", "int", "0x7fff", "(float)acc"),
    ("u32", "uint", "uint", "0xffffffffu", "convert_float(acc)"),
    ("i32", "int", "int", "0x7fffffff", "(float)acc"),
    ("u64", "ulong", "ulong", "0xfffffffffffffffful", "convert_float(acc)"),
    ("i64", "long", "long", "0x7fffffffffffffffl", "(float)acc"),
]

OCCUPANCY_COUNTS = [4, 8, 16, 24, 32, 48, 64, 96]


def write(path: Path, text: str) -> None:
    path.write_text(text.rstrip() + "\n")


def cl_depchain_probe(name: str, cl_type: str, seed_type: str, mask: str, out_expr: str) -> str:
    add_literal = "3" + ("ul" if cl_type in {"ulong", "long"} else "")
    start_literal = "1" + ("ul" if cl_type in {"ulong", "long"} else "u")
    gid_expr = "gid"
    if seed_type == "int":
        gid_init = f"(({seed_type})({gid_expr} & {mask}) - 31)"
        cast_expr = f"({cl_type})"
    elif seed_type == "long":
        gid_init = f"(({seed_type})({gid_expr} & {mask}) - 31l)"
        cast_expr = f"({cl_type})"
    else:
        gid_init = f"(({seed_type})({gid_expr} & {mask}) + {start_literal})"
        cast_expr = f"({cl_type})"

    return f"""__kernel void depchain_{name}_add(__global float *out)
{{
    const uint gid = get_global_id(0);
    {cl_type} acc = {cast_expr}({gid_init});

    for (int i = 0; i < 512; ++i) {{
        acc = {cast_expr}(acc + ({cl_type}){add_literal});
    }}

    out[gid] = {out_expr};
}}
"""


def vk_depchain_probe(name: str) -> str:
    if name == "u8":
        body = """
uint narrow_u8(uint x) { return x & 0xffu; }

void main(void) {
    uint index = gl_GlobalInvocationID.x;
    uint acc = narrow_u8(index + 1u);
    for (int i = 0; i < 512; ++i) {
        acc = narrow_u8(acc + 3u);
    }
    outbuf.values[index] = float(acc);
}
"""
    elif name == "i8":
        body = """
int narrow_i8(int x) { return (x << 24) >> 24; }

void main(void) {
    uint index = gl_GlobalInvocationID.x;
    int acc = narrow_i8(int(index & 0xffu) - 31);
    for (int i = 0; i < 512; ++i) {
        acc = narrow_i8(acc + 3);
    }
    outbuf.values[index] = float(acc);
}
"""
    elif name == "u16":
        body = """
uint narrow_u16(uint x) { return x & 0xffffu; }

void main(void) {
    uint index = gl_GlobalInvocationID.x;
    uint acc = narrow_u16(index + 1u);
    for (int i = 0; i < 512; ++i) {
        acc = narrow_u16(acc + 3u);
    }
    outbuf.values[index] = float(acc);
}
"""
    elif name == "i16":
        body = """
int narrow_i16(int x) { return (x << 16) >> 16; }

void main(void) {
    uint index = gl_GlobalInvocationID.x;
    int acc = narrow_i16(int(index & 0xffffu) - 31);
    for (int i = 0; i < 512; ++i) {
        acc = narrow_i16(acc + 3);
    }
    outbuf.values[index] = float(acc);
}
"""
    elif name == "u32":
        body = """
void main(void) {
    uint index = gl_GlobalInvocationID.x;
    uint acc = index + 1u;
    for (int i = 0; i < 512; ++i) {
        acc = acc + 3u;
    }
    outbuf.values[index] = float(acc);
}
"""
    elif name == "i32":
        body = """
void main(void) {
    uint index = gl_GlobalInvocationID.x;
    int acc = int(index) - 31;
    for (int i = 0; i < 512; ++i) {
        acc = acc + 3;
    }
    outbuf.values[index] = float(acc);
}
"""
    elif name == "u64":
        body = """
#extension GL_ARB_gpu_shader_int64 : require

void main(void) {
    uint index = gl_GlobalInvocationID.x;
    uint64_t acc = uint64_t(index) + uint64_t(1);
    for (int i = 0; i < 512; ++i) {
        acc = acc + uint64_t(3);
    }
    outbuf.values[index] = float(acc & uint64_t(0xffffu));
}
"""
    elif name == "i64":
        body = """
#extension GL_ARB_gpu_shader_int64 : require

void main(void) {
    uint index = gl_GlobalInvocationID.x;
    int64_t acc = int64_t(index) - int64_t(31);
    for (int i = 0; i < 512; ++i) {
        acc = acc + int64_t(3);
    }
    outbuf.values[index] = float(acc & int64_t(0xffff));
}
"""
    else:
        raise ValueError(name)

    return f"""#version 450

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer OutputBuffer {{
    float values[];
}} outbuf;
{body.strip()}
"""


def occupancy_body_float(count: int, lang: str) -> str:
    float_t = "float"
    literal = lambda i: f"{1.0 + (i % 7) * 0.03125:.8f}f"
    bias = lambda i: f"{0.125 + (i % 5) * 0.015625:.8f}f"
    lines: list[str] = []
    for i in range(count):
        scalar = f"({float_t})({i + 1})" if lang == "opencl" else f"{float_t}({i + 1})"
        lines.append(f"    {float_t} a{i} = {scalar} + seed * {0.0001 * (i + 1):.7f}f;")
    lines.append("")
    lines.append("    for (int iter = 0; iter < 256; ++iter) {")
    for i in range(count):
        nxt = (i + 1) % count
        lines.append(
            f"        a{i} = a{i} * {literal(i)} + {bias(i)} + a{nxt} * 0.0009765625f;"
        )
    lines.append("    }")
    lines.append("")
    if lang == "opencl":
        lines.append("    float sum = 0.0f;")
    else:
        lines.append("    float sum = 0.0;")
    for i in range(count):
        lines.append(f"    sum += a{i};")
    return "\n".join(lines)


def cl_occupancy_probe(count: int) -> str:
    body = occupancy_body_float(count, "opencl")
    return f"""__kernel void occupancy_gpr_{count:02d}(__global float *out)
{{
    const uint gid = get_global_id(0);
    const float seed = (float)gid;
{body}
    out[gid] = sum;
}}
"""


def vk_occupancy_probe(count: int) -> str:
    body = occupancy_body_float(count, "vulkan")
    return f"""#version 450

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer OutputBuffer {{
    float values[];
}} outbuf;

void main(void) {{
    uint index = gl_GlobalInvocationID.x;
    float seed = float(index);
{body}
    outbuf.values[index] = sum;
}}
"""


def main() -> None:
    OPENCL_DIR.mkdir(parents=True, exist_ok=True)
    VK_DIR.mkdir(parents=True, exist_ok=True)

    for name, cl_type, seed_type, mask, cast_prefix in DEPCHAIN_SPECS:
        write(OPENCL_DIR / f"depchain_{name}_add.cl", cl_depchain_probe(name, cl_type, seed_type, mask, cast_prefix))
        write(VK_DIR / f"depchain_{name}_add.comp", vk_depchain_probe(name))

    for count in OCCUPANCY_COUNTS:
        write(OPENCL_DIR / f"occupancy_gpr_{count:02d}.cl", cl_occupancy_probe(count))
        write(VK_DIR / f"occupancy_gpr_{count:02d}.comp", vk_occupancy_probe(count))


if __name__ == "__main__":
    main()
