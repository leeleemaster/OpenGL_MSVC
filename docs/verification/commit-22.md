# Commit 22 Verification: README, Architecture, and Windows Release

Verified locally on 2026-08-17.

## Thirty-second repository entry

The README now opens with the project name, the `C++ Dental 3D Visualization with MiniShader
Runtime` description, CI/milestone/license badges, direct Demo/Architecture/Performance links, and
an actual-execution animated preview. The preview contains 120 frames at 720 pixels wide and is
reproducibly generated from the checked-in P0 MP4 by `scripts/create-readme-preview.py`.

The remaining README follows the planned Overview, Demo, Features, Architecture, Engineering
Decisions, Build, Tests, Performance, Limitations, and License/Asset Attribution sequence. It links
only to existing artifacts. The public portfolio PDF and downloadable v1.0 asset are explicitly
deferred to their planned commits instead of exposing placeholder links.

## Architecture decisions

`docs/architecture.md` documents and diagrams:

- the context-free Viewer/Shader Compiler boundary;
- canonical CPU `MeshData` versus move-only GPU VAO/VBO/EBO ownership;
- candidate replacement and Last Known Good Shader policy;
- DPI-aware Viewer coordinates through NDC/world ray/AABB/triangle Picking;
- P0 Viewer, P1 MiniShader, and excluded P2 boundaries;
- why VTK and a general GLSL reimplementation are outside this portfolio scope.

The README and Architecture keep the terminology boundaries for model units, straight-line rather
than surface distance, Clipping Preview rather than cap generation, and button-driven Runtime
Compile & Apply rather than automatic Hot Reload.

## Build and package verification

The project version is `0.8.0`. MSVC Debug and Release completed with `/W4 /WX`; all 63 Catch2 tests
passed in both configurations.

The Release package contains:

```text
DentalViz.exe
assets/shaders/              (six required shader files)
third-party-licenses/        (17 notices)
README.txt
LICENSE
MODEL-ASSET-NOTICE.txt
```

`DentalViz-v0.8-minishader-windows-x64.zip` SHA-256:
`A652A6DD0F47969876C824D7595A4DD171EE973F031300378B691A54009031FB`.

The packaged executable was launched from `C:\Windows\Temp`, reported `DentalViz 0.8.0`, resolved
the executable-adjacent six Shader assets, rendered on NVIDIA GeForce RTX 5070 Ti/OpenGL 3.3.0
NVIDIA 591.86, and exited cleanly with code 0.
