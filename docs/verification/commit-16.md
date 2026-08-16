# Commit 16 Verification: MiniShader Semantic Analyzer

Verified locally on 2026-08-16.

The semantic analyzer registers the three read-only built-in symbols, infers every expression and
declaration type, and leaves invalid expressions marked as `Invalid`. Its failed result is the
explicit gate that later GLSL generation and Runtime Compile & Apply must check.

Seven Catch2 cases cover the reference Dental material, source-ordered independent name errors,
built-in protection, unknown and non-callable functions, argument counts and signatures, rejected
operand pairs, every allowed binary operator form, all vector constructor result types, output
type mismatch, and the MVP exclusion of member access. Invalid child expressions suppress
secondary parent errors while independent statements continue to be analyzed.

MSVC Debug and Release builds completed with warnings treated as errors. All 50 Catch2 tests
passed in both configurations.
