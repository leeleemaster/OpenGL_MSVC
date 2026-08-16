#include "minishader/Token.h"

namespace dentalviz::minishader {

std::string_view tokenKindName(TokenKind kind) noexcept
{
    switch (kind) {
    case TokenKind::Identifier:
        return "Identifier";
    case TokenKind::Number:
        return "Number";
    case TokenKind::Material:
        return "Material";
    case TokenKind::Let:
        return "Let";
    case TokenKind::Output:
        return "Output";
    case TokenKind::Equal:
        return "Equal";
    case TokenKind::Plus:
        return "Plus";
    case TokenKind::Minus:
        return "Minus";
    case TokenKind::Star:
        return "Star";
    case TokenKind::Slash:
        return "Slash";
    case TokenKind::LeftParen:
        return "LeftParen";
    case TokenKind::RightParen:
        return "RightParen";
    case TokenKind::LeftBrace:
        return "LeftBrace";
    case TokenKind::RightBrace:
        return "RightBrace";
    case TokenKind::Comma:
        return "Comma";
    case TokenKind::Semicolon:
        return "Semicolon";
    case TokenKind::EndOfFile:
        return "EndOfFile";
    }

    return "Unknown";
}

} // namespace dentalviz::minishader
