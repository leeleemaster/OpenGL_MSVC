# Commit 03 Verification: First Indexed Mesh Render

Verified locally on 2026-08-16.

## Implemented rendering path

- Procedural, redistributable tooth-shaped CPU test mesh
- Indexed VAO/VBO/EBO upload with move-only RAII ownership
- Smooth area-weighted vertex normals and axis-aligned bounds
- OpenGL 3.3 Core vertex and fragment shaders with checked compile/link results
- Blinn–Phong-style diffuse, specular, and rim lighting
- Perspective camera, depth testing, 4x MSAA, framebuffer-aware projection, and rotation

The procedural mesh is intentionally test geometry, not a clinical or anatomically validated
dental asset.

## Automated verification

Debug and Release builds passed all five Catch2 tests:

```text
Test mesh: 578 vertices, 1152 triangles
Bounds size: 1.55971, 2.67, 1.8568
100% tests passed, 0 tests failed out of 5
```

The tests cover index validity, finite unit normals, bounds preservation, invalid generator
input, and project metadata without creating an OpenGL context.

## Real graphics verification

The rotating, lit tooth test mesh was rendered in visible Debug and Release windows on:

```text
OpenGL vendor: NVIDIA Corporation
OpenGL renderer: NVIDIA GeForce RTX 5070 Ti/PCIe/SSE2
OpenGL version: 3.3.0 NVIDIA 591.86
GLSL version: 3.30 NVIDIA via Cg compiler
MSAA samples: 4
Application loop exited cleanly.
```

Commands used:

```powershell
./scripts/build.ps1 -Configuration Debug
./out/build/msvc/Debug/DentalViz.exe --smoke-seconds 8
./scripts/build.ps1 -Configuration Release
./out/build/msvc/Release/DentalViz.exe --smoke-seconds 5
```

The application also checks the OpenGL error state before reporting a clean loop exit.
