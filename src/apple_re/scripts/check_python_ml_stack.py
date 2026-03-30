#!/usr/bin/env python3

from __future__ import annotations

import importlib
import os
import sys


def try_import(name: str):
    try:
        module = importlib.import_module(name)
        version = getattr(module, "__version__", "unknown")
        return True, version, module
    except Exception as exc:
        return False, str(exc), None


def main() -> int:
    print(f"python: {sys.version.split()[0]}")

    ok, version, _ = try_import("coremltools")
    print(f"coremltools: {'ok' if ok else 'missing'} ({version})")

    ok, version, _ = try_import("mlx.core")
    print(f"mlx.core: {'ok' if ok else 'missing'} ({version})")

    ok, version, torch = try_import("torch")
    if ok and torch is not None:
        mps_available = getattr(torch.backends, "mps", None)
        has_mps = bool(mps_available and torch.backends.mps.is_available())
        print(f"torch: ok ({version}), mps_available={has_mps}")
    else:
        print(f"torch: missing ({version})")

    ok, version, jax = try_import("jax")
    if ok and jax is not None:
        try:
            os.environ.setdefault("ENABLE_PJRT_COMPATIBILITY", "1")
            devices = [str(device) for device in jax.devices()]
        except Exception as exc:
            devices = [f"error: {exc}"]
        print(f"jax: ok ({version}), devices={devices}")
    else:
        print(f"jax: missing ({version})")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
