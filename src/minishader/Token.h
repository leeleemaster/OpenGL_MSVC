#pragma once

#include "minishader/SourceLocation.h"

#include <string>
#include <string_view>

namespace dentalviz::minishader {

enum class TokenKind {
    Identifier,
    Number,
    Material,
    Let,
    Output,
    Equal,
    Plus,
    Minus,
    Star,
    Slash,
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    Comma,
    Semicolon,
    EndOfFile,
};

struct Token {
    TokenKind kind = TokenKind::EndOfFile;
    std::string lexeme;
    SourceLocation location;
};

[[nodiscard]] std::string_view tokenKindName(TokenKind kind) noexcept;

} // namespace dentalviz::minishader
