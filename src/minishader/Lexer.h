#pragma once

#include "minishader/Diagnostic.h"
#include "minishader/Token.h"

#include <string_view>
#include <vector>

namespace dentalviz::minishader {

struct LexResult {
    std::vector<Token> tokens;
    std::vector<Diagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept
    {
        return diagnostics.empty();
    }
};

class Lexer final {
public:
    [[nodiscard]] static LexResult scan(std::string_view source);
};

} // namespace dentalviz::minishader
