DentalViz v1.0 Submission - Windows x64
=======================================

DentalViz is a C++20/OpenGL 3.3 desktop viewer for exploring and measuring
triangle meshes. This package includes the stable P0 viewer and the optional
P1 MiniShader Runtime Compile & Apply milestone.

Run
---

Double-click DentalViz.exe. The bundled procedural tooth opens without an
external model. Use Load Model... or the command line below for STL/OBJ files:

    DentalViz.exe --model "C:\Models\dental.stl"

Controls
--------

Left click       Select measurement points A and B
Left drag        Orbit camera
Middle drag      Pan camera
Mouse wheel      Zoom
F                Fit model to the viewer
1 / 2 / 3        Solid / Wireframe / Normal Color
Escape           Close

The Properties panel provides model information, rendering settings,
point-to-point 3D straight-line measurement, and model-space Clipping Preview.
Measurements use model units because STL/OBJ scale metadata is not assumed.
Clipping uses fragment discard and does not create or fill a cut surface.

The MiniShader tab contains a bounded material editor. Compile & Apply runs the
lexer, parser, semantic validator, GLSL generator, and OpenGL compiler only when
the button is selected. An error retains the Last Known Good shader and reports
its line and column; no automatic file watching is performed.

Requirements
------------

- Windows 10/11 x64
- GPU and driver supporting OpenGL 3.3 Core
- Microsoft Visual C++ 2015-2022 Redistributable (x64)

Asset and safety notice
-----------------------

No clinical or patient model is bundled. The procedural tooth is
project-authored test geometry and is not clinical data. DentalViz is a
portfolio/visualization project, not a medical device or diagnostic tool.

Source: https://github.com/leeleemaster/OpenGL_MSVC
License: MIT; third-party notices are under third-party-licenses/.
