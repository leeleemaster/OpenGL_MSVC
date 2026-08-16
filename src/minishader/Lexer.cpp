#include "minishader/Lexer.h"

#include <iomanip>
#include <sstream>
#include <utility>

namespace dentalviz::minishader {
namespace {

[[nodiscard]] bool isDigit(char character) noexcept
{
    return character >= '0' && character <= '9';
}

[[nodiscard]] bool isIdentifierStart(char character) noexcept
{
    return (character >= 'A' && character <= 'Z') ||
           (character >= 'a' && character <= 'z') || character == '_';
}

[[nodiscard]] bool isIdentifierContinue(char character) noexcept
{
    return isIdentifierStart(character) || isDigit(character);
}

[[nodiscard]] TokenKind identifierKind(std::string_view lexeme) noexcept
{
    if (lexeme == "material") {
        return TokenKind::Material;
    }
    if (lexeme == "let") {
        return TokenKind::Let;
    }
    if (lexeme == "output") {
        return TokenKind::Output;
    }
    return TokenKind::Identifier;
}

[[nodiscard]] std::string describeCharacter(char character)
{
    const auto value = static_cast<unsigned char>(character);
    if (value >= 0x20U && value <= 0x7eU) {
        return std::string("'") + character + "'";
    }

    std::ostringstream stream;
    stream << "byte 0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<unsigned int>(value);
    return stream.str();
}

class Scanner final {
public:
    explicit Scanner(std::string_view source)
        : source_(source)
    {
        result_.tokens.reserve(source.size() / 2U + 1U);
    }

    [[nodiscard]] LexResult run()
    {
        while (!atEnd()) {
            skipTrivia();
            if (atEnd()) {
                break;
            }
            scanToken();
        }

        result_.tokens.push_back(Token{TokenKind::EndOfFile, {}, currentLocation()});
        return std::move(result_);
    }

private:
    [[nodiscard]] bool atEnd() const noexcept
    {
        return current_ >= source_.size();
    }

    [[nodiscard]] char peek() const noexcept
    {
        return atEnd() ? '\0' : source_[current_];
    }

    [[nodiscard]] char peekNext() const noexcept
    {
        return current_ + 1U >= source_.size() ? '\0' : source_[current_ + 1U];
    }

    [[nodiscard]] SourceLocation currentLocation() const noexcept
    {
        return SourceLocation{current_, line_, column_};
    }

    char advance() noexcept
    {
        const char character = source_[current_++];
        if (character == '\n') {
            ++line_;
            column_ = 1;
        } else {
            ++column_;
        }
        return character;
    }

    void skipTrivia() noexcept
    {
        for (;;) {
            switch (peek()) {
            case ' ':
            case '\t':
            case '\n':
                static_cast<void>(advance());
                break;
            case '\r':
                if (peekNext() != '\n') {
                    return;
                }
                static_cast<void>(advance());
                static_cast<void>(advance());
                break;
            case '/':
                if (peekNext() != '/') {
                    return;
                }
                while (!atEnd() && peek() != '\n' && peek() != '\r') {
                    static_cast<void>(advance());
                }
                break;
            default:
                return;
            }
        }
    }

    void addToken(TokenKind kind, SourceLocation start, std::size_t startOffset)
    {
        result_.tokens.push_back(Token{
            kind,
            std::string(source_.substr(startOffset, current_ - startOffset)),
            start,
        });
    }

    void scanIdentifier(SourceLocation start, std::size_t startOffset)
    {
        while (isIdentifierContinue(peek())) {
            static_cast<void>(advance());
        }
        const std::string_view lexeme = source_.substr(startOffset, current_ - startOffset);
        addToken(identifierKind(lexeme), start, startOffset);
    }

    void scanNumber(SourceLocation start, std::size_t startOffset)
    {
        while (isDigit(peek())) {
            static_cast<void>(advance());
        }
        if (peek() == '.' && isDigit(peekNext())) {
            static_cast<void>(advance());
            while (isDigit(peek())) {
                static_cast<void>(advance());
            }
        }
        addToken(TokenKind::Number, start, startOffset);
    }

    void scanToken()
    {
        const SourceLocation start = currentLocation();
        const std::size_t startOffset = current_;
        const char character = advance();

        if (isIdentifierStart(character)) {
            scanIdentifier(start, startOffset);
            return;
        }
        if (isDigit(character)) {
            scanNumber(start, startOffset);
            return;
        }

        switch (character) {
        case '=':
            addToken(TokenKind::Equal, start, startOffset);
            break;
        case '+':
            addToken(TokenKind::Plus, start, startOffset);
            break;
        case '-':
            addToken(TokenKind::Minus, start, startOffset);
            break;
        case '*':
            addToken(TokenKind::Star, start, startOffset);
            break;
        case '/':
            addToken(TokenKind::Slash, start, startOffset);
            break;
        case '(':
            addToken(TokenKind::LeftParen, start, startOffset);
            break;
        case ')':
            addToken(TokenKind::RightParen, start, startOffset);
            break;
        case '{':
            addToken(TokenKind::LeftBrace, start, startOffset);
            break;
        case '}':
            addToken(TokenKind::RightBrace, start, startOffset);
            break;
        case ',':
            addToken(TokenKind::Comma, start, startOffset);
            break;
        case ';':
            addToken(TokenKind::Semicolon, start, startOffset);
            break;
        default:
            result_.diagnostics.push_back(Diagnostic{
                DiagnosticPhase::Lexical,
                start,
                "Unknown character: " + describeCharacter(character),
            });
            break;
        }
    }

    std::string_view source_;
    std::size_t current_ = 0;
    std::size_t line_ = 1;
    std::size_t column_ = 1;
    LexResult result_;
};

} // namespace

LexResult Lexer::scan(std::string_view source)
{
    return Scanner(source).run();
}

} // namespace dentalviz::minishader
