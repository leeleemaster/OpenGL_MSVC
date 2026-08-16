# Commit 15 Verification: MiniShader Parser and AST

Verified locally on 2026-08-16.

The recursive-descent parser accepts exactly one material with zero or more immutable declarations
and one final output statement. AST ownership uses `std::unique_ptr`; binary, call, identifier, and
literal expressions retain the source locations needed by later semantic diagnostics.

Six parser cases cover the basic material structure, multiplication-before-addition precedence,
parentheses and nested calls, an unclosed parenthesis, a missing declaration semicolon, and a
statement outside a material. Failure diagnostics identify the unexpected token and the expected
token at its one-based source location.

MSVC Debug and Release builds completed with warnings treated as errors. All 43 Catch2 tests
passed in both configurations.
