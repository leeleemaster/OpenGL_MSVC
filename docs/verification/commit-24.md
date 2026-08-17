# Commit 24 Verification: Huvitz Application Portfolio

Verified locally on 2026-08-17.

## Portfolio PDF

`output/pdf/DentalViz_Huvitz_Portfolio.pdf` contains 12 A4 landscape pages generated from the
checked-in Release screenshots. SHA-256:

`5D2A951F6ECD9197C532F5588436D0D6B977D0CD585CD9A751F860526B3E1EA7`

The page sequence follows the submission plan: cover, background, feature overview, architecture,
rendering pipeline, camera/mesh, picking/measurement, clipping/UI, MiniShader, compiler/runtime,
engineering quality, and career/links. Eight pages primarily establish the Viewer/career narrative,
two cover the bounded DSL, and two cover engineering evidence and submission links. This keeps the
Viewer dominant and the MiniShader portion close to the planned 70:15 emphasis rather than making
the DSL the project headline.

Every page was rendered to a 1.6x PNG and reviewed as a 12-page contact sheet. Pages 1, 10, and 12
were additionally inspected at full rendered size. Korean/English text, screenshots, code panels,
page numbers, margins, and long URL wrapping have no visible clipping or overlap.

## Links and QR verification

The PDF contains six clickable link annotations with three unique public targets: repository, final
84-second Demo, and Windows CI. The GitHub and Demo QR codes were rendered from the final PDF at 4x,
cropped from their actual page coordinates, and decoded back to the exact expected URLs. This checks
the embedded PDF result rather than only the source QR images.

The public repository and raw Demo had already returned HTTP 200 after Commit 23. README local link
validation includes the new PDF target.

## Claim and application review

`docs/application/huvitz-application-kit.md` contains:

- a repository-backed career summary and DentalViz resume entry;
- a Huvitz-focused self-introduction draft;
- a C/C++, OpenGL, commercial Windows SW, architecture, CI/CD, and performance evidence matrix;
- interview answers that preserve the current implementation boundaries;
- a submission checklist for identity and prior-career facts unavailable in the repository.

The PDF and application text consistently use `3D straight-line distance`, `Clipping Preview`, and
explicit `Runtime Compile & Apply`. They do not claim geodesic measurement, generated clipping caps,
clinical validation, arbitrary GLSL support, BVH acceleration, or implemented hot reload.

The public GitHub handle `leeleemaster` is used as the portfolio identity because the repository and
GitHub profile do not expose a real name. The generator accepts `--applicant "..."` so the applicant
can create an otherwise identical final PDF with the verified real name before submission.
