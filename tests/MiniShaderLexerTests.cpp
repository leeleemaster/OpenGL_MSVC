#include "minishader/Lexer.h"

#include <catch2/catch_test_macros.hpp>

#include <string_view>
#include <vector>

namespace {

using dentalviz::minishader::LexResult;
using dentalviz::minishader::Lexer;
using dentalviz::minishader::SourceLocation;
using dentalviz::minishader::TokenKind;
using dentalviz::minishader::formatDiagnostic;

[[nodiscard]] std::vector<TokenKind> tokenKinds(const LexResult& result)
{
    std::vector<TokenKind> kinds;
    kinds.reserve(result.tokens.size());
    for (const auto& token : result.tokens) {
        kinds.push_back(token.kind);
    }
    return kinds;
}

} // namespace

TEST_CASE("MiniShader lexer tokenizes a complete material", "[minishader][lexer]")
{
    constexpr std::string_view source =
        "material Dental { let diffuse = 0.2 + 1.0; output = baseColor * diffuse; }";

    const LexResult result = Lexer::scan(source);

    REQUIRE(result.succeeded());
    CHECK(tokenKinds(result) == std::vector<TokenKind>{
                                    TokenKind::Material,
                                    TokenKind::Identifier,
                                    TokenKind::LeftBrace,
                                    TokenKind::Let,
                                    TokenKind::Identifier,
                                    TokenKind::Equal,
                                    TokenKind::Number,
                                    TokenKind::Plus,
                                    TokenKind::Number,
                                    TokenKind::Semicolon,
                                    TokenKind::Output,
                                    TokenKind::Equal,
                                    TokenKind::Identifier,
                                    TokenKind::Star,
                                    TokenKind::Identifier,
                                    TokenKind::Semicolon,
                                    TokenKind::RightBrace,
                                    TokenKind::EndOfFile,
                                });
}

TEST_CASE("MiniShader lexer preserves nested function call tokens", "[minishader][lexer]")
{
    const LexResult result = Lexer::scan("output = max(dot(normal, lightDir), 0.0);");

    REQUIRE(result.succeeded());
    REQUIRE(result.tokens.size() == 15);
    CHECK(result.tokens[2].lexeme == "max");
    CHECK(result.tokens[3].kind == TokenKind::LeftParen);
    CHECK(result.tokens[4].lexeme == "dot");
    CHECK(result.tokens[5].kind == TokenKind::LeftParen);
    CHECK(result.tokens[7].kind == TokenKind::Comma);
    CHECK(result.tokens[9].kind == TokenKind::RightParen);
    CHECK(result.tokens[10].kind == TokenKind::Comma);
    CHECK(result.tokens[12].kind == TokenKind::RightParen);
}

TEST_CASE("MiniShader lexer recognizes integer-form and decimal float numbers", "[minishader][lexer]")
{
    const LexResult result = Lexer::scan("0 1.0 42.125");

    REQUIRE(result.succeeded());
    REQUIRE(result.tokens.size() == 4);
    CHECK(result.tokens[0].lexeme == "0");
    CHECK(result.tokens[1].lexeme == "1.0");
    CHECK(result.tokens[2].lexeme == "42.125");
    CHECK(result.tokens[0].kind == TokenKind::Number);
    CHECK(result.tokens[1].kind == TokenKind::Number);
    CHECK(result.tokens[2].kind == TokenKind::Number);
}

TEST_CASE("MiniShader lexer ignores line comments", "[minishader][lexer]")
{
    constexpr std::string_view source =
        "material Dental { // material declaration\r\n"
        "  let n = normal; // source normal\n"
        "  output = n; // no final newline";

    const LexResult result = Lexer::scan(source);

    REQUIRE(result.succeeded());
    REQUIRE(result.tokens.size() == 13);
    CHECK(result.tokens[3].kind == TokenKind::Let);
    CHECK(result.tokens[3].location == SourceLocation{45, 2, 3});
    CHECK(result.tokens[8].kind == TokenKind::Output);
    CHECK(result.tokens[8].location.line == 3);
    CHECK(result.tokens[8].location.column == 3);
}

TEST_CASE("MiniShader lexer reports unknown characters and keeps advancing", "[minishader][lexer]")
{
    const LexResult result = Lexer::scan("@# output");

    REQUIRE_FALSE(result.succeeded());
    REQUIRE(result.diagnostics.size() == 2);
    CHECK(result.diagnostics[0].location == SourceLocation{0, 1, 1});
    CHECK(result.diagnostics[0].message == "Unknown character: '@'");
    CHECK(formatDiagnostic(result.diagnostics[0]) ==
          "MiniShader Lexical Error\nline 1, column 1\nUnknown character: '@'");
    CHECK(result.diagnostics[1].location == SourceLocation{1, 1, 2});
    CHECK(result.diagnostics[1].message == "Unknown character: '#'");
    REQUIRE(result.tokens.size() == 2);
    CHECK(result.tokens[0].kind == TokenKind::Output);
    CHECK(result.tokens[1].kind == TokenKind::EndOfFile);
}

TEST_CASE("MiniShader lexer tracks one-based offset line and column", "[minishader][lexer]")
{
    const LexResult result = Lexer::scan("\tlet value = 7.5;\r\n  output = value;");

    REQUIRE(result.succeeded());
    REQUIRE(result.tokens.size() == 10);
    CHECK(result.tokens[0].location == SourceLocation{1, 1, 2});
    CHECK(result.tokens[1].location == SourceLocation{5, 1, 6});
    CHECK(result.tokens[3].location == SourceLocation{13, 1, 14});
    CHECK(result.tokens[5].location == SourceLocation{21, 2, 3});
    CHECK(result.tokens.back().location == SourceLocation{36, 2, 18});
}
