#pragma once

#include "minishader/Ast.h"
#include "minishader/Diagnostic.h"
#include "minishader/Token.h"

#include <memory>
#include <vector>

namespace dentalviz::minishader {

struct ParseResult {
    std::unique_ptr<MaterialDeclaration> material;
    std::vector<Diagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept
    {
        return material != nullptr && diagnostics.empty();
    }
};

class Parser final {
public:
    [[nodiscard]] static ParseResult parse(const std::vector<Token>& tokens);
};

} // namespace dentalviz::minishader
