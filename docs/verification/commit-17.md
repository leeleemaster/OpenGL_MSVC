# Commit 17 Verification: MiniShader GLSL Generator

Verified locally on 2026-08-17.

The generator accepts only an AST whose expressions and output carry successful semantic types.
It maps the three language built-ins to the fixed shader symbols, emits typed declarations, wraps
the RGB output in `FragColor`, and retains the viewer's model-space clipping and normal-color
paths. User identifiers are deterministically prefixed with `_ms_` to prevent collisions with GLSL
or template names, and integer-form MiniShader floats are emitted with a `.0` suffix.

Generated declarations include MiniShader line/column comments and GLSL `#line` directives so a
driver compile log can be related to the editor source. Four Catch2 cases cover a full golden
fragment shader, repeatability, built-in mapping and name mangling, and rejection after semantic
failure. The generator serializes only typed AST nodes; it never inserts arbitrary source text.

MSVC Debug and Release builds completed with warnings treated as errors. All 54 Catch2 tests
passed in both configurations. Actual driver compilation and Last Known Good replacement are the
next Runtime Compile & Apply milestone.
