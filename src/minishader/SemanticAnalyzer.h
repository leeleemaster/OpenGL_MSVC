#pragma once

#include "minishader/Ast.h"
#include "minishader/Diagnostic.h"

#include <vector>

namespace dentalviz::minishader {

struct SemanticResult {
    std::vector<Diagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept
    {
        return diagnostics.empty();
    }
};

class SemanticAnalyzer final {
public:
    [[nodiscard]] static SemanticResult analyze(MaterialDeclaration& material);
};

} // namespace dentalviz::minishader
