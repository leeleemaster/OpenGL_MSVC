# Commit 08 Verification: Dear ImGui Viewer UI

Verified locally on 2026-08-16.

## Implemented interface

- Dear ImGui 1.92.8 with the vcpkg GLFW and OpenGL3 backend features
- Fixed 380-pixel Properties panel and a separately recorded Viewer rectangle
- DPI-aware conversion from window coordinates to framebuffer coordinates
- Direct Viewer rendering with `glViewport` and `glScissor`, without an early FBO dependency
- Model name, vertex/triangle counts, bounds, unit assumption, source meshes, load time, and FPS
- Solid, Wireframe, and Normal Color selection
- Base color, light position, and shininess controls
- Camera reset and a Windows `Load Model...` file dialog
- In-panel success/error status instead of console-only model loading errors
- Windows system Korean UI font when available, with the Dear ImGui default font as fallback

Model replacement is transactional at the application level: the newly loaded CPU mesh and GPU
mesh are constructed successfully before the current model is replaced. A failed selection leaves
the active model usable and reports the error in the Properties panel.

## Input and coordinate verification

The panel and Viewer use the same explicit boundary in rendering, interaction, and the automated
coordinate test. A scripted visible-window check applied a left-button drag and a wheel event inside
the Properties panel. The final camera statistics stayed unchanged:

```text
Camera input: 0 orbit, 0 pan, 0 zoom, 0 fit updates
Application loop exited cleanly.
```

The 1280x720 UI was captured and visually inspected. Model, Rendering, Status, and Controls sections
were all visible without scrolling or clipped control labels. The Viewer remained centered in the
remaining framebuffer rectangle.

The Release executable also loaded the external STL in 0.293 ms, rendered the UI and mesh on an
NVIDIA GeForce RTX 5070 Ti through OpenGL 3.3 with 4x MSAA, and exited cleanly after the timed run.

## Automated verification

The test suite now includes Unicode Windows model-path loading and exact panel/Viewer boundary cases:

```text
100% tests passed, 0 tests failed out of 16
```
