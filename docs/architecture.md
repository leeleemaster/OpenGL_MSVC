# Architecture

## Dependency direction

```text
DentalViz executable
  -> app / ui / renderer / scene / io
  -> dentalviz_core (geometry and shared domain types)
  -> Dear ImGui GLFW/OpenGL3 backends

dentalviz_io
  -> dentalviz_core / Assimp

dentalviz_tests
  -> dentalviz_core / dentalviz_io
```

`dentalviz_core` must not require an OpenGL context. Geometry intersection, measurement,
bounds, model-space clipping-plane math, camera math that can be isolated, and the optional
MiniShader compiler belong in testable code. OpenGL handles remain inside renderer-side RAII
types.

`dentalviz_io` converts Assimp scenes, node transforms, positions, normals, and triangle
indices into the same `MeshData` used by procedural geometry. File parsing remains independent
of the OpenGL context and is tested with project-authored non-clinical fixtures.

`ViewerUi` owns the Dear ImGui context and backend lifetime, emits UI actions, and stores the
window-space plus framebuffer-space viewer rectangle. The application remains responsible for
model loading, GPU replacement, camera fitting, and rendering state. The Windows native file
dialog is isolated under `src/platform`.

## Viewport decision

P0 uses a fixed properties sidebar and renders the scene directly into the remaining
framebuffer rectangle with `glViewport` and `glScissor`. This avoids an early framebuffer
object dependency. Picking uses the same recorded rectangle and DPI conversion.

Camera input is accepted only while the GLFW cursor is inside that viewer rectangle and Dear
ImGui does not request capture. This prevents panel drags and wheel events from changing the
camera while preserving the exact coordinate rectangle needed by the later picking stage.

An FBO-backed ImGui image viewport is a later refactor only if docking or multiple viewports
becomes necessary.

## Resource ownership

- CPU mesh data owns positions, normals, indices, and bounds.
- GPU mesh data owns VAO/VBO/EBO handles through move-only RAII objects.
- Upload and draw are separate operations.
- Runtime shaders are loaded from `assets/shaders` and copied beside each built executable.
- A shader replacement becomes active only after compile and link succeed.
- UI communicates commands and state; it does not own renderer resources.
- Clipping state stores a model-space axis normal and plane distance. The mesh vertex shader
  forwards the original model position and the fragment shader discards only the positive
  half-space, so camera changes cannot move the plane relative to the model. No cap geometry
  is generated.

## Release linkage

The baseline vcpkg triplet is `x64-windows-static-md-gl33`: third-party libraries are
statically linked while the MSVC runtime remains dynamically linked. The custom triplet also
generates GLAD for the OpenGL 3.3 Core profile. This minimizes application DLL packaging while
remaining compatible with the normal Visual C++ runtime deployment model.
