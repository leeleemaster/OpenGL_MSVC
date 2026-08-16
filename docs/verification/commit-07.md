# Commit 07 Verification: Assimp Mesh Loading

Verified locally on 2026-08-16.

## Implemented loading path

- Assimp-based STL and OBJ import in a separate `dentalviz_io` target
- Scene and recursive node-transform validation
- Position transformation and inverse-transpose normal transformation
- Triangle extraction into the common 32-bit indexed `MeshData`
- Smooth normal generation when source normals are absent
- Empty mesh, missing file, invalid contents, invalid references, and overflow handling
- Vertex count, triangle count, bounds, source mesh count, and load-time reporting
- Bounds-based camera fit without changing source vertex coordinates
- `--model <path>` startup option with procedural fallback after load failure

No dental model is bundled because a source and redistribution license have not yet been
verified. The committed STL/OBJ files are tiny project-authored loader fixtures, not clinical
assets or patient data.

## Automated verification

The loader tests cover valid ASCII STL import, normal generation for an OBJ without normals,
missing paths, and invalid file contents. Together with the existing geometry and camera tests:

```text
100% tests passed, 0 tests failed out of 14
```

## Real graphics verification

The project-authored tetrahedron STL was loaded through Assimp, uploaded through the normal GPU
mesh path, fitted by its calculated bounds, and manipulated in a visible Debug window:

```text
Model: tetrahedron.stl
Mesh: 12 vertices, 4 triangles
Bounds size: 2, 2, 2
Source meshes: 1
Load time: 1.287 ms
Assumed unit: 1 model unit = 1 mm (STL has no unit metadata)
Camera input: 748 orbit, 59 pan, 11 zoom, 2 fit updates
Application loop exited cleanly.
```

An invalid STL produced an Assimp error, fell back to the procedural tooth, rendered normally,
and exited with code 0. This confirms a bad startup model does not terminate the viewer.

The Release executable was also verified in a visible window on an NVIDIA GeForce RTX 5070 Ti.
It loaded the same external STL in 0.366 ms, rendered through OpenGL 3.3 with 4x MSAA, and
exited cleanly after the timed smoke run.
