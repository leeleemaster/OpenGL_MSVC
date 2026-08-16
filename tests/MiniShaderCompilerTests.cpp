#include "minishader/Compiler.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>

namespace {

using dentalviz::minishader::CompilationResult;
using dentalviz::minishader::Compiler;
using dentalviz::minishader::DiagnosticPhase;
using dentalviz::minishader::formatDiagnostics;

constexpr std::string_view validSource =
    "material Dental {\n"
    "  let n = normalize(normal);\n"
    "  let diffuse = max(dot(n, normalize(lightDir)), 0.0);\n"
    "  output = baseColor * (0.2 + diffuse);\n"
    "}";

} // namespace

TEST_CASE("MiniShader compiler runs the complete source to GLSL pipeline", "[minishader][compiler]")
{
    const CompilationResult result = Compiler::compile(validSource);

    REQUIRE(result.succeeded());
    CHECK(result.fragmentSource.find("#version 330 core") != std::string::npos);
    CHECK(result.fragmentSource.find("FragColor = vec4") != std::string::npos);
}

TEST_CASE("MiniShader compiler stops after lexical failure", "[minishader][compiler]")
{
    const CompilationResult result = Compiler::compile("material Broken { @ output = baseColor; }");

    REQUIRE_FALSE(result.succeeded());
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(result.diagnostics[0].phase == DiagnosticPhase::Lexical);
    CHECK(result.fragmentSource.empty());
}

TEST_CASE("MiniShader compiler stops after syntax failure", "[minishader][compiler]")
{
    const CompilationResult result =
        Compiler::compile("material Broken { output = baseColor }");

    REQUIRE_FALSE(result.succeeded());
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(result.diagnostics[0].phase == DiagnosticPhase::Syntax);
    CHECK(result.fragmentSource.empty());
}

TEST_CASE("MiniShader compiler returns multiple semantic diagnostics without GLSL", "[minishader][compiler]")
{
    const CompilationResult result = Compiler::compile(
        "material Broken { let a = unknown; let b = missing; output = 1.0; }");

    REQUIRE_FALSE(result.succeeded());
    REQUIRE(result.diagnostics.size() == 3);
    CHECK(result.diagnostics[0].phase == DiagnosticPhase::Semantic);
    CHECK(result.diagnostics[1].phase == DiagnosticPhase::Semantic);
    CHECK(result.diagnostics[2].phase == DiagnosticPhase::Semantic);
    CHECK(result.fragmentSource.empty());
    CHECK(formatDiagnostics(result.diagnostics).find("line 1, column") != std::string::npos);
}

TEST_CASE("MiniShader compiler rejects oversized source before token allocation", "[minishader][compiler][invalid-input]")
{
    const CompilationResult result = Compiler::compile(std::string(65'537, 'a'));

    REQUIRE_FALSE(result.succeeded());
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(result.diagnostics[0].phase == DiagnosticPhase::Lexical);
    CHECK(result.diagnostics[0].message.find("65536-byte") != std::string::npos);
    CHECK(result.fragmentSource.empty());
}

TEST_CASE("MiniShader compiler rejects excessive expression nesting", "[minishader][compiler][invalid-input]")
{
    std::string source = "material Deep { output = ";
    source.append(129, '(');
    source += '1';
    source.append(129, ')');
    source += "; }";

    const CompilationResult result = Compiler::compile(source);

    REQUIRE_FALSE(result.succeeded());
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(result.diagnostics[0].phase == DiagnosticPhase::Syntax);
    CHECK(result.diagnostics[0].message.find("nesting") != std::string::npos);
    CHECK(result.fragmentSource.empty());
}
