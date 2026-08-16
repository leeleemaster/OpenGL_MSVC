# Commit 14 Verification: MiniShader Lexer

Verified locally on 2026-08-16.

The lexer converts owned source text into an owned token stream with an explicit end-of-file
token. Each token records a byte offset and one-based line and column. CRLF is treated as one line
break, while tabs advance one source column as fixed by the MVP specification.

Six Catch2 cases cover a complete material, nested calls, integer-form and decimal numbers,
line comments including CRLF and end-of-file comments, repeated unknown characters, and exact
source locations. The unknown-character case confirms that scanning advances and still returns
the following valid token. It also verifies the normative three-line diagnostic format.

MSVC Debug and Release builds completed with warnings treated as errors. All 37 Catch2 tests
passed in both configurations.
