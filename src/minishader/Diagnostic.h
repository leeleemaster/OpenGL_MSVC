#pragma once

#include "minishader/SourceLocation.h"

#include <string>
#include <string_view>

namespace dentalviz::minishader {

enum class DiagnosticPhase {
    Lexical,
    Syntax,
    Semantic,
};

struct Diagnostic {
    DiagnosticPhase phase = DiagnosticPhase::Lexical;
    SourceLocation location;
    std::string message;
};

[[nodiscard]] std::string_view diagnosticPhaseName(DiagnosticPhase phase) noexcept;
[[nodiscard]] std::string formatDiagnostic(const Diagnostic& diagnostic);

} // namespace dentalviz::minishader
