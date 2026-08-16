# MiniShader MVP Language Specification

Status: normative for the P1 MiniShader implementation.

This document fixes the smallest language accepted by DentalViz before the lexer, parser,
semantic analyzer, and GLSL generator are implemented. An implementation change that accepts
syntax or types outside this document must update this specification deliberately; it must not
grow the language as an incidental parser or generator behavior.

## Design boundary

MiniShader is a deterministic expression language for producing one RGB material color. It is
not general GLSL and source text is never copied directly into a generated shader. The compiler
pipeline consumes an AST, validates it, and generates a fragment shader only when every phase
succeeds.

The MVP source character set is ASCII. Keywords and identifiers are case-sensitive.

## Example

```text
material Dental {
    let n = normalize(normal);
    let l = normalize(lightDir);
    let diffuse = max(dot(n, l), 0.0);
    let intensity = 0.2 + diffuse;
    output = baseColor * intensity;
}
```

## Lexical grammar

The following EBNF is normative. `{ X }` means zero or more occurrences, `[ X ]` means an
optional occurrence, and quoted text is a literal token.

```ebnf
letter          = "A" | "B" | ... | "Z" | "a" | "b" | ... | "z" | "_" ;
digit           = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
identifier      = letter, { letter | digit } ;
number          = digit, { digit }, [ ".", digit, { digit } ] ;

whitespace      = " " | "\t" | line-break ;
line-comment    = "//", { any-character-except-line-break }, [ line-break ] ;
line-break      = "\n" | "\r\n" ;
```

`material`, `let`, and `output` are reserved keywords and cannot be identifiers. A number is
always a `float`; exponent notation, a leading decimal point, and a numeric suffix are not in
the MVP. Therefore `1`, `1.0`, and `0.25` are valid, while `.5`, `1.`, `1e3`, and `1.0f` are not.

Whitespace and line comments separate tokens and otherwise have no meaning. A comment ends at
the next line break or end of file. Block comments are not supported.

## Syntactic grammar

```ebnf
source                  = material-declaration, end-of-file ;
material-declaration    = "material", identifier, "{",
                          { variable-declaration }, output-statement, "}" ;
variable-declaration    = "let", identifier, "=", expression, ";" ;
output-statement        = "output", "=", expression, ";" ;

expression              = additive-expression ;
additive-expression     = multiplicative-expression,
                          { ( "+" | "-" ), multiplicative-expression } ;
multiplicative-expression = primary-expression,
                            { ( "*" | "/" ), primary-expression } ;
primary-expression      = number
                        | identifier
                        | function-call
                        | "(", expression, ")" ;
function-call           = identifier, "(", [ argument-list ], ")" ;
argument-list           = expression, { ",", expression } ;
```

Only one material declaration is allowed per source. It contains zero or more `let`
declarations followed by exactly one `output` statement. `output` must be the final statement.

Binary `*` and `/` bind more tightly than `+` and `-`; operators at the same precedence are
left-associative. Parentheses override precedence. Unary operators, assignment to variables,
and trailing commas are not supported.

## Types and names

The complete MVP type set is:

```text
float
vec2
vec3
vec4
```

Every number literal has type `float`. A `let` binding is immutable and takes the type of its
initializer; explicit type annotations and implicit conversions do not exist. A binding is in
scope only after its declaration, and it remains in scope through the material's `output`.

A user binding cannot duplicate an earlier user binding or shadow an MVP built-in symbol.
Keywords cannot be used as names. The material name labels the declaration and is not an
expression symbol.

The `output` expression must have type `vec3`, representing linear RGB. It is not a variable and
cannot be read from another expression.

## Built-in symbols

Built-in symbols are read-only.

| MiniShader symbol | Type | Generated GLSL symbol |
|---|---|---|
| `normal` | `vec3` | `vNormal` |
| `lightDir` | `vec3` | `uLightDir` |
| `baseColor` | `vec3` | `uBaseColor` |

## Built-in functions

`N` means one matching vector width: 2, 3, or 4. Arguments must match a listed signature
exactly; there are no implicit scalar/vector conversions or constructor splats.

```text
normalize(vecN)                         -> vecN
dot(vecN, vecN)                         -> float
max(float, float)                       -> float
min(float, float)                       -> float
clamp(float, float, float)              -> float
vec2(float, float)                      -> vec2
vec3(float, float, float)               -> vec3
vec4(float, float, float, float)        -> vec4
```

For `dot`, both arguments must have the same vector width. Calling an identifier that names a
`let` binding is an error; first-class functions are not part of the MVP.

## Binary operator rules

`op` below means any one of `+`, `-`, `*`, or `/` only where the corresponding row exists.
No other operand combination is accepted.

| Left | Operator | Right | Result |
|---|---|---|---|
| `float` | `+ - * /` | `float` | `float` |
| `vecN` | `+ -` | same `vecN` | same `vecN` |
| `vecN` | `*` | `float` | same `vecN` |
| `float` | `*` | `vecN` | same `vecN` |
| `vecN` | `/` | `float` | same `vecN` |

Vector-by-vector multiplication or division, scalar divided by vector, mixed vector widths,
comparisons, logical operations, and implicit promotion are errors.

## Diagnostics and failure policy

Locations are one-based. `line 1, column 1` is the first character. A tab advances one source
column, not to a display tab stop, and `\r\n` counts as one line break. A diagnostic points at
the first character of the token or expression that caused it. End-of-file errors point one
column after the last source character.

Every diagnostic has a phase, location, and actionable message:

```text
MiniShader Semantic Error
line 4, column 25
Unknown identifier: ligthDir
```

The phase is one of `Lexical`, `Syntax`, or `Semantic`. Unknown characters are lexical errors;
missing or unexpected tokens are syntax errors; name, call, operand, and output type failures
are semantic errors.

The lexer must always advance past an unknown character so malformed input cannot cause an
infinite loop. Lexical errors prevent parsing, syntax errors prevent semantic analysis, and any
semantic error prevents GLSL generation and runtime application. Semantic analysis reports as
many independent errors as it can safely collect, in source order, while suppressing secondary
errors whose only cause is an already-invalid subexpression.

## Explicitly outside the MVP

- `bool`, integer, matrix, texture, sampler, and user-defined types
- unary operators, comparisons, conditions, loops, blocks, and user-defined functions
- mutable variables, reassignment, member access, indexing, and swizzles
- additional GLSL functions or direct GLSL source insertion
- multiple materials, imports, preprocessor directives, and block comments

These exclusions keep parsing, validation, generated GLSL, and Runtime Compile & Apply bounded
and testable. A future extension requires an intentional specification and test update first.
