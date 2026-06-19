#!/usr/bin/env python3
"""
Minimal GEMM benchmark for ncu (NVIDIA Nsight Compute) profiling.

NOTE: You need to enable kernel-level permission to profile nvidia counters.

For nsight compute (kernel-level profiling):
`
sudo /usr/local/cuda/bin/ncu --set full -o gemm_fp32   /home/peru/repo/rlplays/game/thirdparty/puffer/bin/python    /home/peru/repo/rlplays/game/thirdparty/PufferLib/tests/test_cuda_flops.py --warmup 0 --iters 1
`

For nsys nsight systems (application-level profiling):
`
 sudo /usr/local/cuda/bin/nsys profile --trace=cuda --cudabacktrace=all  -o gemm_fp32_nsys   /home/peru/repo/rlplays/game/thirdparty/puffer/bin/python    /home/peru/repo/rlplays/game/thirdparty/PufferLib/tests/test_cuda_flops.py --warmup 1 --iters 2
` 

(See below for permanent)

Typical ncu invocation:
  ncu --set full -o gemm_profile python test_cuda_flops.py
  ncu --set full -o gemm_profile python test_cuda_flops.py --dtype fp16 --m 4096 --n 4096 --k 4096

Use --warmup 0 --iters 1 to profile exactly one kernel dispatch.
Use NVTX ranges (always enabled here) to isolate the measured region in ncu.

ncu tips:
  --kernel-name regex:gemm      # filter to cuBLAS GEMM kernels only
  --section SpeedOfLight         # quick roofline summary
  --set full                     # all metrics (slow replay)


Recipes:
# Quick roofline + speed-of-light summary
ncu --set roofline -o gemm_fp32 python test_cuda_flops.py --warmup 0 --iters 1

# Full metrics (slow due to replay passes)
ncu --set full -o gemm_fp32 python test_cuda_flops.py --warmup 0 --iters 1

# Filter to just the cuBLAS GEMM kernel (skips overhead kernels)
ncu --kernel-name regex:gemm --set full -o gemm_fp32 python test_cuda_flops.py --warmup 0 --iters 1

# FP16 run (uses tensor cores)
ncu --set full -o gemm_fp16 python test_cuda_flops.py --dtype fp16 --warmup 0 --iters 1  

NOTE: To enable the necessary permissions permanently:
```
echo 'options nvidia NVreg_RestrictProfilingToAdminUsers=0' | sudo tee /etc/modprobe.d/nvidia-perf.conf
sudo update-initramfs -u   # or: sudo dracut --force  (on RHEL/Fedora)
sudo reboot
```
"""

from __future__ import annotations

import argparse
import statistics
from typing import List, Tuple

import torch


# ---------------------------------------------------------------------------
# Helpers (copied from test_cuda_perf so this file is standalone)
# ---------------------------------------------------------------------------

def _stats(values: List[float]) -> Tuple[float, float, float]:
    mean = statistics.mean(values)
    median = statistics.median(values)
    stdev = statistics.pstdev(values) if len(values) > 1 else 0.0
    return mean, median, stdev


def _to_dtype(name: str) -> torch.dtype:
    name = name.lower()
    mapping = {"fp16": torch.float16, "bf16": torch.bfloat16, "fp32": torch.float32,
               "float16": torch.float16, "bfloat16": torch.bfloat16, "float32": torch.float32}
    if name not in mapping:
        raise ValueError(f"Unsupported dtype: {name}")
    return mapping[name]


# ---------------------------------------------------------------------------
# Core benchmark — identical logic to test_cuda_perf.bench_flops
# ---------------------------------------------------------------------------

@torch.no_grad()
def bench_flops(
    device: torch.device,
    dtype: torch.dtype,
    m: int,
    n: int,
    k: int,
    warmup: int,
    iters: int,
) -> None:
    if dtype == torch.float32:
        try:
            torch.set_float32_matmul_precision("high")
        except Exception:
            pass
        torch.backends.cuda.matmul.allow_tf32 = True
        torch.backends.cudnn.allow_tf32 = True

    print(f"\n== GEMM (FLOPs) on {device} ==")
    print(f"dtype={dtype}, M={m}, N={n}, K={k}, warmup={warmup}, iters={iters}")

    a = torch.randn((m, k), device=device, dtype=dtype)
    b = torch.randn((k, n), device=device, dtype=dtype)
    c = torch.empty((m, n), device=device, dtype=dtype)

    # Warmup — not profiled by ncu if you use --skip-kernel-count or delineate with NVTX
    for _ in range(warmup):
        torch.matmul(a, b, out=c)
    torch.cuda.synchronize()

    start_evt = torch.cuda.Event(enable_timing=True)
    end_evt = torch.cuda.Event(enable_timing=True)

    times_ms: List[float] = []
    for i in range(iters):
        # NVTX range so ncu / Nsight Systems can slice out individual iterations
        torch.cuda.nvtx.range_push(f"gemm_iter_{i}")
        start_evt.record()
        torch.matmul(a, b, out=c)
        end_evt.record()
        torch.cuda.synchronize()
        torch.cuda.nvtx.range_pop()
        times_ms.append(start_evt.elapsed_time(end_evt))

    mean_ms, median_ms, stdev_ms = _stats(times_ms)
    flops = 2.0 * m * n * k
    tflops = (flops / (mean_ms / 1e3)) / 1e12

    print(f"time: mean={mean_ms:.3f} ms  median={median_ms:.3f} ms  stdev={stdev_ms:.3f} ms")
    print(f"throughput: {tflops:.3f} TFLOPs  (2*M*N*K / mean_time)")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    p = argparse.ArgumentParser(description="Minimal GEMM benchmark for ncu profiling")
    p.add_argument("--dtype", default="fp32", choices=["fp16", "bf16", "fp32"])
    p.add_argument("--m", type=int, default=8192)
    p.add_argument("--n", type=int, default=8192)
    p.add_argument("--k", type=int, default=8192)
    p.add_argument("--warmup", type=int, default=5,
                   help="Warmup iterations (not timed; set to 0 when using ncu)")
    p.add_argument("--iters", type=int, default=10,
                   help="Measured iterations (set to 1 for ncu to keep replay fast)")
    p.add_argument("--device", type=str, default="cuda")
    args = p.parse_args()

    if not torch.cuda.is_available():
        raise SystemExit("CUDA not available.")

    device = torch.device(args.device)
    dtype = _to_dtype(args.dtype)

    torch.cuda.init()
    prop = torch.cuda.get_device_properties(device)
    print(f"device : {prop.name}  (cc {prop.major}.{prop.minor})")
    print(f"memory : {prop.total_memory / (1024**3):.2f} GiB")
    print(f"torch  : {torch.__version__}   cuda: {torch.version.cuda}")

    torch.manual_seed(0)
    torch.cuda.synchronize()

    bench_flops(
        device=device,
        dtype=dtype,
        m=args.m,
        n=args.n,
        k=args.k,
        warmup=args.warmup,
        iters=args.iters,
    )


if __name__ == "__main__":
    main()
