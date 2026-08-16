# Commit 20 Verification: Performance Benchmark

Verified locally on 2026-08-17.

## Reproducible benchmark

The dedicated `dentalviz_benchmark` target creates deterministic procedural-tooth OBJ fixtures,
loads them through the production Assimp path, uploads the resulting `MeshData` to real OpenGL
buffers, renders with the production mesh shader, and runs the production AABB-gated nearest
ray/triangle picker. `scripts/run-benchmark.ps1` performs the Release build and tests before running
the benchmark.

Fixed conditions were Release, 1280x720, 4x MSAA, VSync off, Solid render mode, and the same
bounds-fit camera policy for every model. Frame timings used 120 warm-up frames and 600 samples;
load timings used one warm-up and three warm-cache samples; upload and Picking each used ten
measured samples after warm-up.

## Measured machine and results

- CPU identifier: AMD64 Family 26 Model 68 Stepping 0, AuthenticAMD (16 logical threads)
- GPU: NVIDIA GeForce RTX 5070 Ti/PCIe/SSE2
- OpenGL: 3.3.0 NVIDIA 591.86

| Model | Triangles | Load median ms | Upload median ms | CPU frame median/p95 ms | FPS | Picking median/p95 ms |
|---|---:|---:|---:|---:|---:|---:|
| 100k | 100008 | 81.500 | 0.851 | 0.286 / 0.476 | 3155.895 | 0.663 / 0.684 |
| 500k | 500004 | 477.794 | 3.337 | 0.516 / 1.012 | 1681.784 | 3.321 / 3.413 |

These are measured values, not estimates. Upload is CPU-observed wall time through `glFinish`, not
a GPU timer query. CPU frame time covers clear, uniform and draw submission, event polling, and
VSync-off buffer swap without per-frame `glFinish`; it is not GPU execution time. Picking uses the
current brute-force triangle path after the AABB gate.

The checked-in summary is `docs/performance/benchmark-summary.md`; all 1,246 individual measured
samples are in `docs/performance/benchmark-raw.csv`.

## Build verification

MSVC Debug and Release builds completed with warnings treated as errors. All 58 Catch2 tests passed
in both configurations. The benchmark completed without an OpenGL error on the real NVIDIA driver.
