# Commit 11 Verification: Clipping Preview

Verified locally on 2026-08-16.

## Plane behavior

- `Clipping Preview` exposes an enable switch, +X/+Y/+Z model-space normal selection, a bounded
  plane-distance slider, and `Reset Plane`.
- The shader keeps `dot(modelPosition, normal) + distance <= 0` and discards the positive
  half-space before applying any render mode.
- Changing the camera affects only view/projection matrices; it does not change the model-space
  position used by clipping.
- Reset selects +X and places the plane through the loaded model's bounds center.

This is a fragment-discard preview. It does not create geometry at the cut, cap the opening, or
claim that the cut surface is filled.

## Automated coverage

Catch2 verifies center reset, axis normals and bounds-derived slider ranges, distance clamping,
disabled behavior, the plane boundary, and positive-half-space discard behavior. Debug and Release
completed with all 31 tests passing while MSVC warnings were treated as errors.

A visible 1280x720 Release run used an NVIDIA GeForce RTX 5070 Ti, OpenGL 3.3, and 4x MSAA.
Visual inspection confirmed that the model changed immediately when clipping was enabled and the
distance slider moved. The open cut remained tied to the selected +Z model-space plane after an
orbit-camera drag. The final UI inspection confirmed complete `Normal axis` and `Distance d` labels,
the reset control, and the explicit warning that the cut is not capped or filled. The application
then exited cleanly without an OpenGL error.
