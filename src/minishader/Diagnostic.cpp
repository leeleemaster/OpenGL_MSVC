#include "minishader/Diagnostic.h"

#include <sstream>

namespace dentalviz::minishader {

std::string_view diagnosticPhaseName(DiagnosticPhase phase) noexcept
{
    switch (phase) {
    case DiagnosticPhase::Lexical:
        return "Lexical";
    case DiagnosticPhase::Syntax:
        return "Syntax";
    case DiagnosticPhase::Semantic:
        return "Semantic";
    }

    return "Unknown";
}

std::string formatDiagnostic(const Diagnostic& diagnostic)
{
    std::ostringstream stream;
    stream << "MiniShader " << diagnosticPhaseName(diagnostic.phase) << " Error\n"
           << "line " << diagnostic.location.line << ", column " << diagnostic.location.column
           << '\n'
           << diagnostic.message;
    return stream.str();
}

} // namespace dentalviz::minishader
