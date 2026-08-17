#include "minishader/Ast.h"
#include "minishader/Lexer.h"
#include "minishader/Parser.h"
#include "minishader/SemanticAnalyzer.h"

#include <catch2/catch_test_macros.hpp>

#include <string_view>
#include <utility>

namespace {

using namespace dentalviz::minishader;

struct AnalysisFixture {
    ParseResult parseResult;
    SemanticResult semanticResult;
};

[[nodiscard]] AnalysisFixture analyzeSource(std::string_view source)
{
    LexResult lexResult = Lexer::scan(source);
    REQUIRE(lexResult.succeeded());

    ParseResult parseResult = Parser::parse(lexResult.tokens);
    REQUIRE(parseResult.succeeded());

    SemanticResult semanticResult = SemanticAnalyzer::analyze(*parseResult.material);
    return AnalysisFixture{std::move(parseResult), std::move(semanticResult)};
}

} // namespace

TEST_CASE("MiniShader semantic analyzer validates the Dental material", "[minishader][semantic]")
{
    constexpr std::string_view source =
        "material Dental {\n"
        "  let n = normalize(normal);\n"
        "  let l = normalize(lightDir);\n"
        "  let diffuse = max(dot(n, l), 0.0);\n"
        "  let bounded = min(diffuse, 1.0);\n"
        "  let intensity = clamp(0.2 + bounded, 0.0, 1.0);\n"
        "  output = baseColor * intensity;\n"
        "}";

    AnalysisFixture fixture = analyzeSource(source);

    REQUIRE(fixture.semanticResult.succeeded());
    REQUIRE(fixture.parseResult.material->variables.size() == 5);
    CHECK(fixture.parseResult.material->variables[0].inferredType == ValueType::Vec3);
    CHECK(fixture.parseResult.material->variables[1].inferredType == ValueType::Vec3);
    CHECK(fixture.parseResult.material->variables[2].inferredType == ValueType::Float);
    CHECK(fixture.parseResult.material->variables[3].inferredType == ValueType::Float);
    CHECK(fixture.parseResult.material->variables[4].inferredType == ValueType::Float);
    CHECK(fixture.parseResult.material->output->expression->inferredType == ValueType::Vec3);
}

TEST_CASE("MiniShader semantic analyzer reports independent name errors in source order", "[minishader][semantic]")
{
    constexpr std::string_view source =
        "material Names {\n"
        "  let early = later;\n"
        "  let later = 1.0;\n"
        "  let missing = mysteryValue;\n"
        "  let same = 1.0;\n"
        "  let same = 2.0;\n"
        "  output = 1.0;\n"
        "}";

    AnalysisFixture fixture = analyzeSource(source);

    REQUIRE_FALSE(fixture.semanticResult.succeeded());
    REQUIRE(fixture.semanticResult.diagnostics.size() == 4);
    CHECK(fixture.semanticResult.diagnostics[0].message == "선언 전에 사용한 식별자: later.");
    CHECK(fixture.semanticResult.diagnostics[1].message == "알 수 없는 식별자: mysteryValue.");
    CHECK(fixture.semanticResult.diagnostics[2].message == "중복 변수: same.");
    CHECK(fixture.semanticResult.diagnostics[3].message ==
          "Output 타입 불일치: vec3가 필요하지만 실제 타입은 float입니다.");
    CHECK(fixture.semanticResult.diagnostics[0].location.line == 2);
    CHECK(fixture.semanticResult.diagnostics[3].location.line == 7);
}

TEST_CASE("MiniShader semantic analyzer protects built-in symbols", "[minishader][semantic]")
{
    AnalysisFixture fixture = analyzeSource(
        "material Builtin { let normal = vec3(1.0, 0.0, 0.0); output = baseColor; }");

    REQUIRE_FALSE(fixture.semanticResult.succeeded());
    REQUIRE(fixture.semanticResult.diagnostics.size() == 1);
    CHECK(fixture.semanticResult.diagnostics[0].message ==
          "내장 Symbol을 다시 선언할 수 없습니다: normal.");
}

TEST_CASE("MiniShader semantic analyzer validates function names counts and signatures", "[minishader][semantic]")
{
    constexpr std::string_view source =
        "material Calls {\n"
        "  let unknown = mystery(1.0);\n"
        "  let count = dot(normal);\n"
        "  let signature = max(normal, lightDir);\n"
        "  let scalar = 1.0;\n"
        "  let called = scalar();\n"
        "  output = baseColor;\n"
        "}";

    AnalysisFixture fixture = analyzeSource(source);

    REQUIRE_FALSE(fixture.semanticResult.succeeded());
    REQUIRE(fixture.semanticResult.diagnostics.size() == 4);
    CHECK(fixture.semanticResult.diagnostics[0].message == "알 수 없는 함수: mystery.");
    CHECK(fixture.semanticResult.diagnostics[1].message ==
          "dot 함수의 인자 개수가 잘못되었습니다: 필요 2개, 입력 1개입니다.");
    CHECK(fixture.semanticResult.diagnostics[2].message ==
          "지원하지 않는 인자 타입: max(vec3, vec3).");
    CHECK(fixture.semanticResult.diagnostics[3].message ==
          "호출할 수 없는 식별자: scalar.");
}

TEST_CASE("MiniShader semantic analyzer rejects unsupported binary operands", "[minishader][semantic]")
{
    constexpr std::string_view source =
        "material Operators {\n"
        "  let vectorProduct = baseColor * normal;\n"
        "  let mixedWidth = vec2(1.0, 2.0) + baseColor;\n"
        "  let scalarDivision = 1.0 / baseColor;\n"
        "  output = baseColor;\n"
        "}";

    AnalysisFixture fixture = analyzeSource(source);

    REQUIRE_FALSE(fixture.semanticResult.succeeded());
    REQUIRE(fixture.semanticResult.diagnostics.size() == 3);
    CHECK(fixture.semanticResult.diagnostics[0].message ==
          "연산자 '*'에서 지원하지 않는 타입: vec3, vec3.");
    CHECK(fixture.semanticResult.diagnostics[1].message ==
          "연산자 '+'에서 지원하지 않는 타입: vec2, vec3.");
    CHECK(fixture.semanticResult.diagnostics[2].message ==
          "연산자 '/'에서 지원하지 않는 타입: float, vec3.");
}

TEST_CASE("MiniShader semantic analyzer accepts the complete binary operator matrix", "[minishader][semantic]")
{
    constexpr std::string_view source =
        "material Operators {\n"
        "  let first = vec3(1.0, 2.0, 3.0);\n"
        "  let second = vec3(0.5, 1.0, 1.5);\n"
        "  let sum = first + second;\n"
        "  let difference = sum - second;\n"
        "  let leftScaled = difference * 2.0;\n"
        "  let rightScaled = 0.5 * leftScaled;\n"
        "  let divided = rightScaled / 2.0;\n"
        "  output = divided;\n"
        "}";

    AnalysisFixture fixture = analyzeSource(source);

    REQUIRE(fixture.semanticResult.succeeded());
    for (const VariableDeclaration& variable : fixture.parseResult.material->variables) {
        CHECK(variable.inferredType == ValueType::Vec3);
    }
}

TEST_CASE("MiniShader semantic analyzer supports all MVP constructor result types", "[minishader][semantic]")
{
    constexpr std::string_view source =
        "material Constructors {\n"
        "  let uv = vec2(0.0, 1.0);\n"
        "  let rgb = vec3(uv.x, 0.5, 1.0);\n"
        "  let rgba = vec4(0.0, 0.5, 1.0, 1.0);\n"
        "  output = rgb;\n"
        "}";

    LexResult lexResult = Lexer::scan(source);

    REQUIRE_FALSE(lexResult.succeeded());
    REQUIRE(lexResult.diagnostics.size() == 1);
    CHECK(lexResult.diagnostics[0].message == "알 수 없는 문자: '.'");

    AnalysisFixture validFixture = analyzeSource(
        "material Constructors { let uv = vec2(0.0, 1.0); let rgb = vec3(0.0, 0.5, 1.0); "
        "let rgba = vec4(0.0, 0.5, 1.0, 1.0); output = rgb; }");
    REQUIRE(validFixture.semanticResult.succeeded());
    CHECK(validFixture.parseResult.material->variables[0].inferredType == ValueType::Vec2);
    CHECK(validFixture.parseResult.material->variables[1].inferredType == ValueType::Vec3);
    CHECK(validFixture.parseResult.material->variables[2].inferredType == ValueType::Vec4);
}
