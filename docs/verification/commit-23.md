# Commit 23 Verification: Final Portfolio Demo

Verified locally on 2026-08-17.

## Artifact

`docs/demo/DentalViz-v0.8-portfolio-demo.mp4` is an 84-second capture of the actual 1280x720
Release application at 12 FPS. All 1,008 encoded frames were decoded successfully. SHA-256:

`0A7EBEE6FCF96222214E29FB21628541FB376E143D01E56406B0DC5C8014AE39`

The capture script brings the DentalViz window to the foreground and confirms it before recording.
Frames sampled at 0.0, 0.5, 1.0, 1.5, and 2.0 seconds contain only the application; no build,
terminal, browser, or unrelated desktop window appears.

## Demonstrated flow

| Time | Actual Release interaction |
|---:|---|
| 0–18 s | Generated dental test mesh, Orbit, Pan, Zoom, and bounds-based Fit |
| 18–30 s | Wireframe, Normal Color, and Blinn–Phong Solid modes |
| 30–39 s | nearest-triangle Ray Picking and two-point 3D straight-line Measurement |
| 39–50 s | model-space Clipping Preview and camera-independent clipping plane |
| 50–66 s | default and edited MiniShader sources compiled and applied explicitly |
| 66–75 s | invalid identifier rejected while the Last Known Good program remains active |
| 75–84 s | compiler/resource/test architecture summary over the running application |

The script captures the process output as part of verification. It requires exactly two successful
`MiniShader revision` activations and the expected `Unknown identifier: missing.` semantic error for
the invalid source. A mismatch fails capture instead of producing an apparently successful demo.

The complete timeline was also reviewed from a 19-frame contact sheet. The captions describe only
implemented behavior, and all application transitions are visible in the recorded UI.
