# Commit 25 Verification: v1.0 Submission Package

Verified locally on 2026-08-17.

## Version, build, and tests

The project and packaged executable report `DentalViz 1.0.0`. The MSVC Release configuration was
rebuilt with the `x64-windows-static-md-gl33` vcpkg triplet and `/W4 /WX`. All 63 Catch2 tests
passed.

The final actual-GPU checks ran on NVIDIA GeForce RTX 5070 Ti, OpenGL 3.3.0 NVIDIA 591.86:

- Runtime Hardening passed invalid mesh rejection, invalid GLSL rejection, Last Known Good
  retention, and moved-from OpenGL resource safety.
- MiniShader Runtime verification applied two valid programs. The default viewer SHA-256 was
  `313273ECD8944ABBA72242B42428BD69E685F63135F8B07B9D0733C2A551970F`; the modified and
  post-error Last Known Good viewer hashes both were
  `F7DD0A6E3E5370BFB236B60DD8CF8DE15E2337689C1B73B43B4B66113AFEF89A`.

## Windows package

`DentalViz-v1.0-submission-windows-x64.zip` contains 27 files under one root directory:

- `DentalViz.exe`;
- six required files under `assets/shaders/`;
- 17 dependency notices under `third-party-licenses/`;
- `README.txt`, `LICENSE`, and `MODEL-ASSET-NOTICE.txt`.

The archive was expanded into a new GUID-named Windows temporary directory. The executable was
launched with the extraction root as its working directory, resolved all six executable-adjacent
Shader files, rendered the procedural test mesh, reported `DentalViz 1.0.0`, and exited cleanly with
code 0 after five seconds. The temporary directory was then removed.

The exact smoke-tested ZIP is preserved under `output/release/`. The tag-triggered Release workflow
checks all three fixed assets against `SHA256SUMS.txt` before uploading them; it does not rebuild a
different ZIP for publication.

## Submission asset hashes

| Asset | SHA-256 |
|---|---|
| `DentalViz-v1.0-submission-windows-x64.zip` | `84D7FB2458C62160C2DF835D1A41BDFC266A6F1499A1D570EFEB4594195968BD` |
| `DentalViz_Huvitz_Portfolio.pdf` | `5D2A951F6ECD9197C532F5588436D0D6B977D0CD585CD9A751F860526B3E1EA7` |
| `DentalViz-v0.8-portfolio-demo.mp4` | `0A7EBEE6FCF96222214E29FB21628541FB376E143D01E56406B0DC5C8014AE39` |

## Public delivery

The source is fixed by the annotated `v1.0-submission` tag. The public GitHub Release publishes the
Windows ZIP, 12-page PDF, and 84-second MP4:

<https://github.com/leeleemaster/OpenGL_MSVC/releases/tag/v1.0-submission>

The repository README links the Release, source tag, final Demo, PDF, architecture, and performance
evidence. Public HTTP access is checked again after publishing the tag and Release.

## Applicant-controlled boundary

The repository package and public portfolio are submission-ready under the GitHub identity
`leeleemaster`. Real name, contact details, prior-employer facts, the final live job posting, portal
upload, and the submission-complete screen cannot be verified from the repository. They remain
explicitly unchecked in `docs/application/submission-checklist.md`; no personal fact or completed
application is fabricated here.
