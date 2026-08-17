#include "minishader/Ast.h"
#include "minishader/Lexer.h"
#include "minishader/Parser.h"

#include <catch2/catch_test_macros.hpp>

#include <string_view>

namespace {

using namespace dentalviz::minishader;

[[nodiscard]] ParseResult parseSource(std::string_view source)
{
    LexResult lexResult = Lexer::scan(source);
    REQUIRE(lexResult.succeeded());
    return Parser::parse(lexResult.tokens);
}

template <typename ExpressionType>
[[nodiscard]] const ExpressionType& requireExpression(const Expression& expression)
{
    const auto* converted = dynamic_cast<const ExpressionType*>(&expression);
    REQUIRE(converted != nullptr);
    return *converted;
}

} // namespace

TEST_CASE("MiniShader parser builds a basic material AST", "[minishader][parser]")
{
    constexpr std::string_view source =
        "material Dental {\n"
        "  let intensity = 0.2;\n"
        "  output = baseColor * intensity;\n"
        "}";

    const ParseResult result = parseSource(source);

    REQUIRE(result.succeeded());
    REQUIRE(result.material != nullptr);
    CHECK(result.material->name == "Dental");
    REQUIRE(result.material->variables.size() == 1);
    CHECK(result.material->variables[0].name == "intensity");
    const auto& literal =
        requireExpression<LiteralExpression>(*result.material->variables[0].initializer);
    CHECK(literal.lexeme == "0.2");
    REQUIRE(result.material->output != nullptr);
    const auto& output =
        requireExpression<BinaryExpression>(*result.material->output->expression);
    CHECK(output.operatorKind == TokenKind::Star);
}

TEST_CASE("MiniShader parser applies multiplication before addition", "[minishader][parser]")
{
    const ParseResult result =
        parseSource("material Order { output = 1.0 + 2.0 * 3.0; }");

    REQUIRE(result.succeeded());
    const auto& addition =
        requireExpression<BinaryExpression>(*result.material->output->expression);
    CHECK(addition.operatorKind == TokenKind::Plus);
    CHECK(addition.left->kind == ExpressionKind::Literal);
    const auto& multiplication = requireExpression<BinaryExpression>(*addition.right);
    CHECK(multiplication.operatorKind == TokenKind::Star);
}

TEST_CASE("MiniShader parser honors parentheses and nested calls", "[minishader][parser]")
{
    const ParseResult result = parseSource(
        "material Nested { output = max(dot(normal, lightDir), (0.1 + 0.2) * 3.0); }");

    REQUIRE(result.succeeded());
    const auto& maximum =
        requireExpression<CallExpression>(*result.material->output->expression);
    CHECK(maximum.callee == "max");
    REQUIRE(maximum.arguments.size() == 2);
    const auto& dot = requireExpression<CallExpression>(*maximum.arguments[0]);
    CHECK(dot.callee == "dot");
    REQUIRE(dot.arguments.size() == 2);
    const auto& product = requireExpression<BinaryExpression>(*maximum.arguments[1]);
    CHECK(product.operatorKind == TokenKind::Star);
    const auto& parenthesizedAddition = requireExpression<BinaryExpression>(*product.left);
    CHECK(parenthesizedAddition.operatorKind == TokenKind::Plus);
}

TEST_CASE("MiniShader parser reports an unclosed parenthesis", "[minishader][parser]")
{
    const ParseResult result =
        parseSource("material Broken { output = max(1.0, 2.0; }");

    REQUIRE_FALSE(result.succeeded());
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(result.diagnostics[0].phase == DiagnosticPhase::Syntax);
    CHECK(result.diagnostics[0].message.find("필요: ')'") != std::string::npos);
    CHECK(result.diagnostics[0].location.line == 1);
    CHECK(result.diagnostics[0].location.column == 40);
}

TEST_CASE("MiniShader parser requires a declaration semicolon", "[minishader][parser]")
{
    constexpr std::string_view source =
        "material Broken {\n"
        "  let value = 1.0\n"
        "  output = value;\n"
        "}";

    const ParseResult result = parseSource(source);

    REQUIRE_FALSE(result.succeeded());
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(result.diagnostics[0].message.find("필요: ';'") != std::string::npos);
    CHECK(result.diagnostics[0].message.find("'output' ('output')") != std::string::npos);
    CHECK(result.diagnostics[0].location.line == 3);
    CHECK(result.diagnostics[0].location.column == 3);
}

TEST_CASE("MiniShader parser rejects statements outside a material", "[minishader][parser]")
{
    const ParseResult result = parseSource("let value = 1.0;");

    REQUIRE_FALSE(result.succeeded());
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(result.diagnostics[0].message.find("필요: 'material'") != std::string::npos);
    CHECK(result.diagnostics[0].location.line == 1);
    CHECK(result.diagnostics[0].location.column == 1);
}
