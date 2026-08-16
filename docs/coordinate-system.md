# Coordinate and Unit Conventions

| Item | Convention |
|---|---|
| Handedness | Right-handed |
| Up axis | +Y |
| Camera forward | -Z |
| Matrix storage | GLM column-major |
| Clip-space depth | OpenGL NDC -1 to 1 |
| Model unit | Unspecified; UI reports `model units` unless reliable scale metadata is added |
| Measurement | Euclidean straight-line distance |
| Clipping plane | Model-space plane transformed consistently for rendering |
| Color | Linear values in the first release; sRGB is deferred |

## Invariants

- Source vertex positions are never destructively normalized to fit the screen.
- Bounds drive camera distance and near/far plane selection.
- Picking transforms the world ray into model space and transforms the selected hit back
  into the coordinate space used by measurement and display.
- Camera movement never changes the clipping plane relative to the model.
- Framebuffer size, window size, UI viewport bounds, and DPI scale are treated separately.

## Input policy

- Left drag: orbit.
- Middle drag: pan.
- Wheel: zoom.
- `F`: fit the model.
- Left click without exceeding a small drag threshold: picking.
- UI-captured input never changes the camera or selection.

The exact click/drag threshold will be fixed with the viewport implementation and tested
manually at 100% and high-DPI Windows scaling.
