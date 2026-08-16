# Commit 12 Verification: P0 Viewer Release

Verified locally on 2026-08-16.

## Release package

`scripts/package.ps1` performs the Release build and tests before creating
`DentalViz-v0.5-viewer-windows-x64.zip`. The package contains:

- `DentalViz.exe`
- all runtime GLSL files under `assets/shaders/`
- package usage and model-asset notices
- the project MIT license
- vcpkg copyright notices under `third-party-licenses/`

The verified archive SHA-256 is
`82CF7840AA4B3A058925E85B3313B677916967FD1EAD86EBF5CC07B0B3C31B78`.

The ZIP was extracted to a unique Windows temporary directory. DentalViz was then launched while
the current working directory was the parent temporary directory, not the package or repository.
The executable resolved all shaders from its own `assets/shaders` directory, rendered through an
NVIDIA GeForce RTX 5070 Ti with OpenGL 3.3 and 4x MSAA, and exited cleanly with code 0.

## Resource lifetime and tests

The executable directory is resolved with Unicode-safe `GetModuleFileNameW`. Runtime assets now use
that directory before development fallbacks. The local RAII review confirmed that UI, marker,
shader, and GPU mesh resources are destroyed before the `Application` tears down the current GLFW
context.

MSVC Debug and Release builds completed with warnings treated as errors. All 31 Catch2 tests passed
in both configurations.

## Portfolio evidence

Five project-authored screenshots record the actual Release viewer:

1. Overview
2. Wireframe
3. Ray Picking with Point A
4. Two-point 3D straight-line measurement
5. Model-space Clipping Preview

The first Viewer Demo is 50 seconds, 600 frames at 12 FPS, and 1280x720. It shows Orbit, Pan, Zoom,
fit, all three render modes, Picking, Measurement, Clipping distance adjustment, and camera movement
with clipping enabled. Its SHA-256 is
`41840D2667C453EFDB2B438A7B9C39AA0D643C317C199AE085FFA7738201F962`.

No clinical or patient model is presented. The procedural tooth is explicitly labeled
project-authored test geometry.
