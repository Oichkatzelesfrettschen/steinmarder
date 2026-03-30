#!/usr/bin/env python3

from __future__ import annotations

import json
import os
import tempfile
from pathlib import Path


def probe_torch() -> dict:
    import torch

    result: dict[str, object] = {
        "version": torch.__version__,
        "mps_available": bool(torch.backends.mps.is_available()),
    }

    x = torch.arange(16, dtype=torch.float32).reshape(4, 4)
    y = x @ x.T
    result["cpu_sum"] = float(y.sum().item())

    if torch.backends.mps.is_available():
        xm = x.to("mps")
        ym = (xm @ xm.T).sum().cpu().item()
        result["mps_sum"] = float(ym)

    return result


def probe_mlx() -> dict:
    import mlx.core as mx

    x = mx.arange(16, dtype=mx.float32).reshape(4, 4)
    y = x @ x.T
    return {
        "version": getattr(mx, "__version__", "unknown"),
        "sum": float(mx.sum(y).item()),
        "default_device": str(mx.default_device()),
    }


def probe_jax() -> dict:
    os.environ.setdefault("ENABLE_PJRT_COMPATIBILITY", "1")

    import jax
    import jax.numpy as jnp

    devices = [str(device) for device in jax.devices()]
    x = jnp.arange(16, dtype=jnp.float32).reshape(4, 4)
    y = x @ x.T
    return {
        "version": jax.__version__,
        "devices": devices,
        "sum": float(y.sum().item()),
        "enable_pjrt_compatibility": os.environ.get("ENABLE_PJRT_COMPATIBILITY"),
    }


def probe_coremltools() -> dict:
    import coremltools as ct
    import torch

    class TinyLinear(torch.nn.Module):
        def __init__(self) -> None:
            super().__init__()
            self.linear = torch.nn.Linear(4, 4)

        def forward(self, x):
            return self.linear(x).relu()

    model = TinyLinear().eval()
    example = torch.rand(1, 4)
    traced = torch.jit.trace(model, example)

    mlmodel = ct.convert(
        traced,
        inputs=[ct.TensorType(shape=example.shape)],
        convert_to="mlprogram",
    )

    spec = mlmodel.get_spec()
    with tempfile.TemporaryDirectory() as tmpdir:
        package_path = Path(tmpdir) / "tiny_linear.mlpackage"
        mlmodel.save(str(package_path))
        saved = package_path.exists()

    return {
        "version": ct.__version__,
        "model_type": spec.WhichOneof("Type"),
        "saved_mlpackage": saved,
    }


def main() -> int:
    probes: dict[str, object] = {}

    probes["python"] = {
        "executable": os.sys.executable,
        "version": os.sys.version.split()[0],
    }

    for name, fn in (
        ("torch", probe_torch),
        ("mlx", probe_mlx),
        ("jax", probe_jax),
        ("coremltools", probe_coremltools),
    ):
        try:
            probes[name] = {"ok": True, "details": fn()}
        except Exception as exc:
            probes[name] = {"ok": False, "error": f"{type(exc).__name__}: {exc}"}

    print(json.dumps(probes, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
