# Commit 13 Verification: MiniShader MVP Specification

Verified locally on 2026-08-16.

The normative language document was reviewed against the Commit 13 completion criteria. It fixes:

- lexical and syntactic EBNF, including precedence and statement order
- the complete `float`/`vec2`/`vec3`/`vec4` type set
- immutable declaration, scope, output, and no-conversion rules
- all built-in symbols, GLSL mappings, and function signatures
- the exact supported binary-operator matrix
- one-based source locations, diagnostic phases, and pipeline failure behavior

The document also lists unsupported syntax and types so later lexer, parser, semantic, and GLSL
work cannot accidentally expand the MVP. Commit 13 changes no runtime code; the 31-test Debug and
Release baseline verified for the P0 release remains unchanged.
