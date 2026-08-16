# Commit 21 Verification: Code Quality and Error Hardening

Verified locally on 2026-08-17.

## Audit and changes

The RAII, OpenGL deletion, move semantics, const correctness, error propagation, asset-path,
duplication, naming, header dependency, warning, invalid-input, and Last Known Good checklist was
reviewed across the application, renderer, mesh loader, geometry, and MiniShader pipeline.

- `GpuMesh`, `ShaderProgram`, and `SelectionMarker` now reject failed OpenGL object allocation and
  keep deletion idempotent. Moved-from meshes render as a safe no-op, while move assignment releases
  the previous object first.
- Buffer, mesh-file, Shader-file, Shader-source, and procedural-index size conversions are checked
  before narrowing or allocation.
- Render and Picking entry points reject NaN/Inf positions or normals. Bounds and normal generation
  also reject non-finite positions instead of propagating invalid floating-point state.
- MiniShader source is limited to 65,536 bytes and expression nesting to 128 levels, producing a
  normal diagnostic instead of unbounded token allocation or parser recursion.
- UTF-8 path conversion was consolidated into `core/PathText`; Shader asset discovery checks every
  required file with non-throwing filesystem probes and still prioritizes the executable directory.
- Invalid programmatic run options, empty uniform names, zero OpenGL object handles, and oversized
  OpenGL buffer/source lengths now fail with explicit errors.
- The MSVC preset explicitly enables `/W4` plus `/WX` for clean Debug, Release, and CI builds.

## Automated verification

Both MSVC Debug and Release completed with warnings treated as errors. All 63 Catch2 tests passed in
both configurations. The suite includes Geometry, Picking, MeshLoader, Lexer, Parser, Semantic, and
GLSL Generator coverage plus new non-finite mesh, index overflow, oversized MiniShader, and deep
nesting rejection cases.

Invalid command lines also returned exit code 1 with an explanatory error, without opening the
application or terminating abnormally.

## Real OpenGL and invalid-input verification

The Release `dentalviz_benchmark --self-check` ran on NVIDIA GeForce RTX 5070 Ti with OpenGL 3.3.0
NVIDIA 591.86. It verified all of the following in one real driver context:

- a deliberately broken GLSL fragment fails compilation;
- the active OpenGL program ID remains the Last Known Good program after that failure;
- a NaN mesh is rejected before GPU upload;
- move construction and move assignment of `GpuMesh` and `ShaderProgram` release prior ownership;
- calls on moved-from objects are safe and the check exits with no OpenGL error.

The actual Release Viewer was then launched with `tests/fixtures/invalid.stl`. Assimp reported the
damage, the application retained the procedural test model, rendered, and exited cleanly with code
0. MiniShader lexical, syntax, semantic, size, and depth failures all produce diagnostics without
GLSL replacement; the real UI Last Known Good transition remains documented in Commit 18.

Finally, a fresh Windows package was launched from `C:\Windows\Temp`. It resolved all six Shader
files under the executable-adjacent `assets/shaders` directory and exited cleanly. The final
`DentalViz-v0.8-minishader-windows-x64.zip` SHA-256 is
`FD90AA616958D1F75FCB9DE8D1B058AB6A249CADFF675E07D4A3AF7737C453D7`.
