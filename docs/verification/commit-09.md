# Commit 09 Verification: Ray Picking

Verified locally on 2026-08-16.

## Implemented pipeline

The picking path keeps window, Viewer, framebuffer, and model coordinates explicit:

```text
window cursor -> Viewer-relative normalized coordinate -> OpenGL NDC
-> inverse projection/view -> world ray -> mesh AABB -> indexed triangles
-> closest positive Moller-Trumbore hit
```

Viewer-relative normalization makes the cursor mapping independent of the Windows DPI scale while
the camera projection continues to use the actual framebuffer aspect ratio. The current model matrix
is identity; the core also provides ray transformation for future non-identity model transforms.

## Input and selection behavior

- A left press/release moving no more than four logical pixels requests a selection.
- Motion beyond the threshold becomes orbit input and never also selects on release.
- An interaction must start and finish inside the Viewer without Dear ImGui mouse capture.
- Clicking empty space clears the previous selection.
- Loading a replacement model clears the previous selection.
- A selected hit stores its model-space position, interpolated normal, barycentric coordinates,
  positive distance, and indexed triangle number.
- The Properties panel displays the position, normal, and triangle number; a circular orange marker
  identifies the point in the Viewer.

## Automated coverage

Catch2 covers center-screen world-ray construction, AABB hit/miss, triangle hit/miss, a triangle
behind the ray origin, and closest-positive-hit selection across multiple triangles. The click
gesture is tested separately for short clicks, movement beyond the four-pixel threshold, an
interaction beginning in the Properties panel, and a release outside the Viewer.

Both Debug and Release configurations completed with all 24 tests passing. A visible Release run
used an NVIDIA GeForce RTX 5070 Ti, OpenGL 3.3, and 4x MSAA. Clicking the procedural tooth reported
triangle 608 at `(-0.015, -0.056, 0.692)`, and visual inspection confirmed the matching orange
surface marker plus the position, normal, and triangle fields in the Properties panel. The process
then exited cleanly without an OpenGL error.
