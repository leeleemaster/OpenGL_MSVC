#pragma once

#include "minishader/Diagnostic.h"

#include <string>
#include <string_view>
#include <vector>

namespace dentalviz::minishader {

struct CompilationResult {
    std::string fragmentSource;
    std::vector<Diagnostic> diagnostics;
    std::string internalError;

    [[nodiscard]] bool succeeded() const noexcept
    {
        return !fragmentSource.empty() && diagnostics.empty() && internalError.empty();
    }
};

class Compiler final {
public:
    [[nodiscard]] static CompilationResult compile(std::string_view source);
};

[[nodiscard]] std::string formatDiagnostics(const std::vector<Diagnostic>& diagnostics);

} // namespace dentalviz::minishader
