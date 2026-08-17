#include "minishader/Diagnostic.h"

#include <sstream>

namespace dentalviz::minishader {

std::string_view diagnosticPhaseName(DiagnosticPhase phase) noexcept
{
    switch (phase) {
    case DiagnosticPhase::Lexical:
        return "어휘";
    case DiagnosticPhase::Syntax:
        return "구문";
    case DiagnosticPhase::Semantic:
        return "의미";
    }

    return "알 수 없음";
}

std::string formatDiagnostic(const Diagnostic& diagnostic)
{
    std::ostringstream stream;
    stream << "MiniShader " << diagnosticPhaseName(diagnostic.phase) << " 오류\n"
           << diagnostic.location.line << "행, " << diagnostic.location.column << "열"
           << '\n'
           << diagnostic.message;
    return stream.str();
}

} // namespace dentalviz::minishader
