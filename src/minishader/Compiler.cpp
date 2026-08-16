#include "minishader/Compiler.h"

#include "minishader/GlslGenerator.h"
#include "minishader/Lexer.h"
#include "minishader/Parser.h"
#include "minishader/SemanticAnalyzer.h"

#include <cstddef>
#include <utility>

namespace dentalviz::minishader {

CompilationResult Compiler::compile(std::string_view source)
{
    LexResult lexResult = Lexer::scan(source);
    if (!lexResult.succeeded()) {
        return CompilationResult{{}, std::move(lexResult.diagnostics), {}};
    }

    ParseResult parseResult = Parser::parse(lexResult.tokens);
    if (!parseResult.succeeded()) {
        return CompilationResult{{}, std::move(parseResult.diagnostics), {}};
    }

    SemanticResult semanticResult = SemanticAnalyzer::analyze(*parseResult.material);
    if (!semanticResult.succeeded()) {
        return CompilationResult{{}, std::move(semanticResult.diagnostics), {}};
    }

    GlslGenerationResult generation = GlslGenerator::generate(*parseResult.material);
    if (!generation.succeeded()) {
        return CompilationResult{{}, {}, std::move(generation.error)};
    }

    return CompilationResult{std::move(generation.fragmentSource), {}, {}};
}

std::string formatDiagnostics(const std::vector<Diagnostic>& diagnostics)
{
    std::string result;
    for (std::size_t index = 0; index < diagnostics.size(); ++index) {
        if (index != 0U) {
            result += "\n\n";
        }
        result += formatDiagnostic(diagnostics[index]);
    }
    return result;
}

} // namespace dentalviz::minishader
