# Commit 18 Verification: MiniShader Runtime Compile & Apply

Verified locally on 2026-08-17.

## Compiler and UI

The `MiniShader` tab provides an editable source area, an explicit `Compile & Apply` button,
generated GLSL preview, phase/location diagnostics, source reset, and active Last Known Good state.
Editing does not trigger compilation. The headless compiler facade gates Lexer, Parser, Semantic
Analyzer, and GLSL Generator in order and produces no GLSL after an earlier phase fails.

The application compiles and links generated GLSL into a temporary OpenGL program. Only a complete
candidate replaces the active move-only `ShaderProgram`; any MiniShader or driver failure leaves the
previous program alive. Driver-optimized optional fragment uniforms are handled without weakening
the required vertex-transform uniforms.

## Real GPU verification

`scripts/verify-minishader-runtime.ps1` launched the visible Release application and automated the
actual editor and button. The runtime used:

- NVIDIA GeForce RTX 5070 Ti
- OpenGL 3.3.0 NVIDIA 591.86
- GLSL 3.30 NVIDIA via Cg compiler
- 4x MSAA

The default source compiled and linked on the driver. Changing the example intensity from `0.2` to
`0.5` and applying again visibly changed the material. A third attempt using unknown identifier
`missing` produced a Semantic diagnostic with line/column and retained revision 2.

Viewer-only capture hashes prove the state transition:

| Capture | SHA-256 |
|---|---|
| Default MiniShader | `313273ECD8944ABBA72242B42428BD69E685F63135F8B07B9D0733C2A551970F` |
| Modified `0.5` MiniShader | `F7DD0A6E3E5370BFB236B60DD8CF8DE15E2337689C1B73B43B4B66113AFEF89A` |
| Invalid source after revision 2 | `F7DD0A6E3E5370BFB236B60DD8CF8DE15E2337689C1B73B43B4B66113AFEF89A` |

The modified and post-error captures are byte-identical, while the default and modified captures
differ. The application then closed cleanly without an OpenGL error. Actual UI evidence is stored
in `docs/screenshots/06_minishader.png` and `docs/screenshots/07_minishader_error.png`.

## Automated and package verification

MSVC Debug and Release builds completed with warnings treated as errors. All 58 Catch2 tests passed
in both configurations, including pipeline phase gates and multi-diagnostic formatting.

The Windows x64 package is `DentalViz-v0.8-minishader-windows-x64.zip`. Its SHA-256 is
`BEE4B963215F936825F51E2726CEC7766FCCA70CB32F5AF9F44DC56C17EE320B`.
The packaged executable was launched with an unrelated `C:\Windows\Temp` working directory,
resolved its bundled shaders beside the executable, rendered on the same GPU, and exited cleanly.
