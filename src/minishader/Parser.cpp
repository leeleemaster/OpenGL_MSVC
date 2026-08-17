#include "minishader/Parser.h"

#include <string>
#include <string_view>
#include <utility>

namespace dentalviz::minishader {
namespace {

struct ParseAbort {
};

constexpr std::size_t maximumExpressionDepth = 128U;

[[nodiscard]] std::string expectedTokenText(TokenKind kind)
{
    switch (kind) {
    case TokenKind::Identifier:
        return "식별자";
    case TokenKind::Number:
        return "숫자";
    case TokenKind::Material:
        return "'material'";
    case TokenKind::Let:
        return "'let'";
    case TokenKind::Output:
        return "'output'";
    case TokenKind::Equal:
        return "'='";
    case TokenKind::Plus:
        return "'+'";
    case TokenKind::Minus:
        return "'-'";
    case TokenKind::Star:
        return "'*'";
    case TokenKind::Slash:
        return "'/'";
    case TokenKind::LeftParen:
        return "'('";
    case TokenKind::RightParen:
        return "')'";
    case TokenKind::LeftBrace:
        return "'{'";
    case TokenKind::RightBrace:
        return "'}'";
    case TokenKind::Comma:
        return "','";
    case TokenKind::Semicolon:
        return "';'";
    case TokenKind::EndOfFile:
        return "파일 끝";
    }

    return "토큰";
}

[[nodiscard]] std::string foundTokenText(const Token& token)
{
    if (token.kind == TokenKind::EndOfFile) {
        return "파일 끝";
    }
    return expectedTokenText(token.kind) + " ('" + token.lexeme + "')";
}

class RecursiveDescentParser final {
public:
    explicit RecursiveDescentParser(const std::vector<Token>& tokens)
        : tokens_(tokens)
    {
    }

    [[nodiscard]] ParseResult run()
    {
        ParseResult result;
        try {
            result.material = parseMaterial();
            consume(TokenKind::EndOfFile, "Material 선언 뒤");
        } catch (const ParseAbort&) {
            result.material.reset();
        }
        result.diagnostics = std::move(diagnostics_);
        return result;
    }

private:
    [[nodiscard]] const Token& peek() const noexcept
    {
        return tokens_[current_];
    }

    [[nodiscard]] const Token& previous() const noexcept
    {
        return tokens_[current_ - 1U];
    }

    [[nodiscard]] bool isAtEnd() const noexcept
    {
        return peek().kind == TokenKind::EndOfFile;
    }

    [[nodiscard]] bool check(TokenKind kind) const noexcept
    {
        return peek().kind == kind;
    }

    const Token& advance() noexcept
    {
        const Token& token = peek();
        if (!isAtEnd()) {
            ++current_;
        }
        return token;
    }

    bool match(TokenKind kind) noexcept
    {
        if (!check(kind)) {
            return false;
        }
        static_cast<void>(advance());
        return true;
    }

    [[noreturn]] void failExpected(TokenKind expected, std::string_view context)
    {
        const Token& found = peek();
        std::string message = "필요: " + expectedTokenText(expected);
        if (!context.empty()) {
            message += " ";
            message += context;
        }
        message += "; 발견: ";
        message += foundTokenText(found);
        message += ".";
        diagnostics_.push_back(Diagnostic{DiagnosticPhase::Syntax, found.location, std::move(message)});
        throw ParseAbort{};
    }

    const Token& consume(TokenKind kind, std::string_view context)
    {
        if (check(kind)) {
            return advance();
        }
        failExpected(kind, context);
    }

    [[nodiscard]] std::unique_ptr<MaterialDeclaration> parseMaterial()
    {
        const Token& materialToken = consume(TokenKind::Material, "소스 시작 위치");
        const Token& name = consume(TokenKind::Identifier, "'material' 뒤");
        consume(TokenKind::LeftBrace, "Material 이름 뒤");

        std::vector<VariableDeclaration> variables;
        while (match(TokenKind::Let)) {
            variables.push_back(parseVariableDeclaration());
        }

        auto output = parseOutputStatement();
        consume(TokenKind::RightBrace, "Output 문장 뒤");

        auto material = std::make_unique<MaterialDeclaration>();
        material->location = materialToken.location;
        material->nameLocation = name.location;
        material->name = name.lexeme;
        material->variables = std::move(variables);
        material->output = std::move(output);
        return material;
    }

    [[nodiscard]] VariableDeclaration parseVariableDeclaration()
    {
        const Token& name = consume(TokenKind::Identifier, "'let' 뒤");
        consume(TokenKind::Equal, "변수 이름 뒤");
        auto initializer = parseExpression();
        consume(TokenKind::Semicolon, "변수 선언 뒤");
        return VariableDeclaration{name.location, name.lexeme, std::move(initializer)};
    }

    [[nodiscard]] std::unique_ptr<OutputStatement> parseOutputStatement()
    {
        const Token& outputToken = consume(TokenKind::Output, "변수 선언 뒤");
        consume(TokenKind::Equal, "'output' 뒤");
        auto expression = parseExpression();
        consume(TokenKind::Semicolon, "Output 표현식 뒤");

        auto output = std::make_unique<OutputStatement>();
        output->location = outputToken.location;
        output->expression = std::move(expression);
        return output;
    }

    [[nodiscard]] std::unique_ptr<Expression> parseExpression()
    {
        if (expressionDepth_ >= maximumExpressionDepth) {
            diagnostics_.push_back(Diagnostic{
                DiagnosticPhase::Syntax,
                peek().location,
                "표현식 중첩이 안전 제한 128단계를 초과했습니다.",
            });
            throw ParseAbort{};
        }
        ++expressionDepth_;
        auto expression = parseAdditive();
        --expressionDepth_;
        return expression;
    }

    [[nodiscard]] std::unique_ptr<Expression> parseAdditive()
    {
        auto expression = parseMultiplicative();

        while (check(TokenKind::Plus) || check(TokenKind::Minus)) {
            const Token operation = advance();
            auto right = parseMultiplicative();
            expression = std::make_unique<BinaryExpression>(
                std::move(expression), operation.kind, operation.location, std::move(right));
        }
        return expression;
    }

    [[nodiscard]] std::unique_ptr<Expression> parseMultiplicative()
    {
        auto expression = parsePrimary();

        while (check(TokenKind::Star) || check(TokenKind::Slash)) {
            const Token operation = advance();
            auto right = parsePrimary();
            expression = std::make_unique<BinaryExpression>(
                std::move(expression), operation.kind, operation.location, std::move(right));
        }
        return expression;
    }

    [[nodiscard]] std::unique_ptr<Expression> parsePrimary()
    {
        if (match(TokenKind::Number)) {
            const Token& number = previous();
            return std::make_unique<LiteralExpression>(number.lexeme, number.location);
        }

        if (match(TokenKind::Identifier)) {
            const Token& identifier = previous();
            if (match(TokenKind::LeftParen)) {
                return parseCall(identifier);
            }
            return std::make_unique<IdentifierExpression>(identifier.lexeme, identifier.location);
        }

        if (match(TokenKind::LeftParen)) {
            auto expression = parseExpression();
            consume(TokenKind::RightParen, "괄호 표현식 뒤");
            return expression;
        }

        failExpected(TokenKind::Number, "또는 표현식 시작 위치의 식별자나 '('");
    }

    [[nodiscard]] std::unique_ptr<Expression> parseCall(const Token& identifier)
    {
        std::vector<std::unique_ptr<Expression>> arguments;
        if (!check(TokenKind::RightParen)) {
            do {
                arguments.push_back(parseExpression());
            } while (match(TokenKind::Comma));
        }
        consume(TokenKind::RightParen, "함수 인자 뒤");
        return std::make_unique<CallExpression>(
            identifier.lexeme, identifier.location, std::move(arguments));
    }

    const std::vector<Token>& tokens_;
    std::size_t current_ = 0;
    std::size_t expressionDepth_ = 0;
    std::vector<Diagnostic> diagnostics_;
};

} // namespace

ParseResult Parser::parse(const std::vector<Token>& tokens)
{
    if (tokens.empty() || tokens.back().kind != TokenKind::EndOfFile) {
        SourceLocation location;
        if (!tokens.empty()) {
            location = tokens.back().location;
        }
        ParseResult result;
        result.diagnostics.push_back(Diagnostic{
            DiagnosticPhase::Syntax,
            location,
            "Token Stream은 EndOfFile로 끝나야 합니다.",
        });
        return result;
    }
    return RecursiveDescentParser(tokens).run();
}

} // namespace dentalviz::minishader
