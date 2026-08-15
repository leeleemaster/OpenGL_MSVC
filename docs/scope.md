# DentalViz Scope

## Project objective

DentalViz is a Windows x64 portfolio application that demonstrates a modern C++/OpenGL
rendering pipeline and geometry interaction on dental triangle meshes. The submission
must remain useful without the optional MiniShader work.

## P0 — submission viewer

- Reproducible CMake/MSVC build and headless unit tests
- OpenGL 3.3 Core window and application loop
- VAO/VBO/EBO indexed mesh rendering with RAII ownership
- Orbit, pan, zoom, and bounds-based camera fit
- Blinn–Phong, wireframe, and normal visualization modes
- STL loading through a common CPU mesh representation
- Dear ImGui properties and status UI
- Ray–AABB and Möller–Trumbore ray–triangle picking
- Two-point Euclidean distance measurement
- Fragment-shader plane clipping preview
- Windows release package, screenshots, demo, and documentation

P0 is complete only when a clean Release package runs outside the build directory.

## P1 — optional differentiation

- Minimal MiniShader grammar
- Lexer, parser, AST, and bounded semantic validation
- Deterministic GLSL generation
- Button-driven Runtime Compile & Apply
- Last Known Good shader preservation on failure

P1 starts only after the P0 release, README, and first demo are complete.

## P2 — explicitly excluded from the submission baseline

- Automatic file hot reload
- BVH acceleration
- GPU timer queries
- Clipping cap mesh generation
- Surface/geodesic distance
- Production medical-device claims or clinical validation

## Definition of done

Every milestone must have:

1. MSVC Debug and Release builds passing.
2. Relevant automated tests passing without an OpenGL context.
3. A documented manual visual check when rendering is involved.
4. Accurate terminology in UI and documentation.
5. No generated build output or unlicensed asset committed.

## Terminology boundaries

- Measurement means the Euclidean straight-line distance between two selected points.
- STL contains no reliable unit metadata. A displayed `mm` value is an explicit model-unit assumption.
- Clipping is a fragment-discard preview, not a generated cross-section or capped mesh.
- Runtime Compile & Apply is button-driven unless automatic file watching is actually implemented.
- GitHub Actions build and test constitutes CI; CD is claimed only if release publication is automated.

## Asset policy

No dental model is committed until its source, author, license, modification rights, and
redistribution rights are recorded in `assets/models/LICENSE.txt`. Until then, procedural
test geometry is the fallback and external STL files may be loaded locally.
