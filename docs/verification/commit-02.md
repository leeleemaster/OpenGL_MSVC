# Commit 02 Verification: OpenGL Window and Application Loop

Verified locally on 2026-08-16.

## Build and automated tests

Both configurations completed successfully:

```powershell
./scripts/build.ps1 -Configuration Debug
./scripts/build.ps1 -Configuration Release
```

The Catch2 smoke test passed in both configurations.

## Real graphics-context smoke run

The Debug executable rendered a visible window for five seconds and the Release executable
rendered one for three seconds. Both created an OpenGL context, swapped frames, processed
events, and exited normally.

```text
OpenGL vendor: NVIDIA Corporation
OpenGL renderer: NVIDIA GeForce RTX 5070 Ti/PCIe/SSE2
OpenGL version: 3.3.0 NVIDIA 591.86
GLSL version: 3.30 NVIDIA via Cg compiler
Application loop exited cleanly.
```

Commands used:

```powershell
./out/build/msvc/Debug/DentalViz.exe --smoke-seconds 5
./out/build/msvc/Release/DentalViz.exe --smoke-seconds 3
```

This confirms the current local MSVC, GLFW, GLAD, NVIDIA driver, and window/event loop work
together. It does not replace testing on other target machines or future mesh-rendering tests.
