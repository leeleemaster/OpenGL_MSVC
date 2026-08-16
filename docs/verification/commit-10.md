# Commit 10 Verification: 3D Point-to-Point Measurement

Verified locally on 2026-08-16.

## Measurement behavior

- The first valid surface click sets Point A.
- The second valid surface click sets Point B and calculates their Euclidean 3D straight-line
  distance.
- A third valid surface click consistently starts a new measurement at Point A and clears Point B.
- Clicking empty Viewer space, loading a replacement model, or pressing `Reset Measurement` clears
  both points and the distance.

The viewer renders Point A in orange, Point B in cyan, a yellow segment between them, and a distance
label at the projected segment midpoint. The Properties panel repeats both coordinates and the
distance.

## Unit policy

STL and the current common mesh representation do not provide reliable real-world scale metadata.
DentalViz therefore labels the result as `model units` and does not claim millimeters. The UI and
README explicitly call the result a `3D 직선거리`.

## Automated coverage

Catch2 verifies the A-to-B state transition, Euclidean distance, zero-length measurement, the
third-click restart rule, and explicit reset behavior. Existing picking and click-gesture coverage
continues to protect the Viewer boundary, click/drag threshold, AABB rejection, triangle hit rules,
and closest positive hit.

Both Debug and Release completed with all 27 tests passing while MSVC warnings were treated as
errors. A visible Release run used an NVIDIA GeForce RTX 5070 Ti, OpenGL 3.3, and 4x MSAA. Two
surface points produced a `0.962 model units` result; visual inspection confirmed the orange A and
cyan B markers, yellow segment, midpoint label, Properties values, and a clean 1280x720 layout.
