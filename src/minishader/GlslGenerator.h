#pragma once

#include "minishader/Ast.h"

#include <string>

namespace dentalviz::minishader {

struct GlslGenerationResult {
    std::string fragmentSource;
    std::string error;

    [[nodiscard]] bool succeeded() const noexcept
    {
        return error.empty() && !fragmentSource.empty();
    }
};

class GlslGenerator final {
public:
    [[nodiscard]] static GlslGenerationResult generate(const MaterialDeclaration& material);
};

} // namespace dentalviz::minishader
