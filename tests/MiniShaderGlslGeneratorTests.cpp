#include "minishader/GlslGenerator.h"
#include "minishader/Lexer.h"
#include "minishader/Parser.h"
#include "minishader/SemanticAnalyzer.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>

namespace {

using namespace dentalviz::minishader;

[[nodiscard]] ParseResult validatedMaterial(std::string_view source)
{
    LexResult lexResult = Lexer::scan(source);
    REQUIRE(lexResult.succeeded());
    ParseResult parseResult = Parser::parse(lexResult.tokens);
    REQUIRE(parseResult.succeeded());
    const SemanticResult semanticResult = SemanticAnalyzer::analyze(*parseResult.material);
    REQUIRE(semanticResult.succeeded());
    return parseResult;
}

constexpr std::string_view dentalSource =
    "material Dental {\n"
    "    let n = normalize(normal);\n"
    "    let l = normalize(lightDir);\n"
    "    let diffuse = max(dot(n, l), 0.0);\n"
    "    let intensity = 0.2 + diffuse;\n"
    "    output = baseColor * intensity;\n"
    "}";

constexpr std::string_view expectedDentalFragment = R"glsl(#version 330 core

in vec3 worldPosition;
in vec3 worldNormal;
in vec3 modelPosition;

uniform vec3 uBaseColor;
uniform vec3 uLightPosition;
uniform int uRenderMode;
uniform int uClipEnabled;
uniform vec3 uClipNormal;
uniform float uClipDistance;

out vec4 FragColor;

void main()
{
    if (uClipEnabled != 0 &&
        dot(modelPosition, uClipNormal) + uClipDistance > 0.0) {
        discard;
    }

    vec3 vNormal = normalize(worldNormal);
    if (uRenderMode == 2) {
        FragColor = vec4(vNormal * 0.5 + 0.5, 1.0);
        return;
    }

    vec3 uLightDir = normalize(uLightPosition - worldPosition);

    // MiniShader material Dental
    // MiniShader line 2, column 9
#line 2 1
    vec3 _ms_n = normalize(vNormal);
    // MiniShader line 3, column 9
#line 3 1
    vec3 _ms_l = normalize(uLightDir);
    // MiniShader line 4, column 9
#line 4 1
    float _ms_diffuse = max(dot(_ms_n, _ms_l), 0.0);
    // MiniShader line 5, column 9
#line 5 1
    float _ms_intensity = (0.2 + _ms_diffuse);

    // MiniShader line 6, column 14
#line 6 1
    FragColor = vec4((uBaseColor * _ms_intensity), 1.0);
}
)glsl";

} // namespace

TEST_CASE("MiniShader GLSL generator matches the Dental golden shader", "[minishader][glsl]")
{
    ParseResult material = validatedMaterial(dentalSource);

    const GlslGenerationResult result = GlslGenerator::generate(*material.material);

    REQUIRE(result.succeeded());
    CHECK(result.fragmentSource == expectedDentalFragment);
}

TEST_CASE("MiniShader GLSL generation is deterministic", "[minishader][glsl]")
{
    ParseResult material = validatedMaterial(dentalSource);

    const GlslGenerationResult first = GlslGenerator::generate(*material.material);
    const GlslGenerationResult second = GlslGenerator::generate(*material.material);

    REQUIRE(first.succeeded());
    REQUIRE(second.succeeded());
    CHECK(first.fragmentSource == second.fragmentSource);
}

TEST_CASE("MiniShader GLSL generator maps built-ins and mangles user names", "[minishader][glsl]")
{
    ParseResult material = validatedMaterial(
        "material Names { let FragColor = vec3(1, 0, 0); "
        "let lit = normalize(lightDir); output = FragColor + normal * dot(lit, baseColor); }");

    const GlslGenerationResult result = GlslGenerator::generate(*material.material);

    REQUIRE(result.succeeded());
    CHECK(result.fragmentSource.find("vec3 _ms_FragColor = vec3(1.0, 0.0, 0.0);") !=
          std::string::npos);
    CHECK(result.fragmentSource.find("normalize(uLightDir)") != std::string::npos);
    CHECK(result.fragmentSource.find("vNormal * dot(_ms_lit, uBaseColor)") != std::string::npos);
}

TEST_CASE("MiniShader GLSL generator rejects a semantically invalid AST", "[minishader][glsl]")
{
    LexResult lexResult = Lexer::scan("material Invalid { output = 1.0; }");
    REQUIRE(lexResult.succeeded());
    ParseResult parseResult = Parser::parse(lexResult.tokens);
    REQUIRE(parseResult.succeeded());
    const SemanticResult semanticResult = SemanticAnalyzer::analyze(*parseResult.material);
    REQUIRE_FALSE(semanticResult.succeeded());

    const GlslGenerationResult generation = GlslGenerator::generate(*parseResult.material);

    CHECK_FALSE(generation.succeeded());
    CHECK(generation.fragmentSource.empty());
    CHECK(generation.error ==
          "GLSL 생성 전에 MiniShader AST 의미 검증을 통과해야 합니다.");
}
