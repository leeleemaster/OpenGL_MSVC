#include "minishader/SemanticAnalyzer.h"

#include "minishader/Type.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace dentalviz::minishader {
namespace {

[[nodiscard]] bool isKnownFunction(std::string_view name) noexcept
{
    return name == "normalize" || name == "dot" || name == "max" || name == "min" ||
           name == "clamp" || name == "vec2" || name == "vec3" || name == "vec4";
}

[[nodiscard]] std::size_t expectedArgumentCount(std::string_view name) noexcept
{
    if (name == "normalize") {
        return 1;
    }
    if (name == "dot" || name == "max" || name == "min" || name == "vec2") {
        return 2;
    }
    if (name == "clamp" || name == "vec3") {
        return 3;
    }
    if (name == "vec4") {
        return 4;
    }
    return 0;
}

[[nodiscard]] std::string operatorText(TokenKind kind)
{
    switch (kind) {
    case TokenKind::Plus:
        return "+";
    case TokenKind::Minus:
        return "-";
    case TokenKind::Star:
        return "*";
    case TokenKind::Slash:
        return "/";
    default:
        return std::string(tokenKindName(kind));
    }
}

[[nodiscard]] std::string argumentTypeList(const std::vector<ValueType>& types)
{
    std::string result;
    for (std::size_t index = 0; index < types.size(); ++index) {
        if (index != 0U) {
            result += ", ";
        }
        result += valueTypeName(types[index]);
    }
    return result;
}

class Analyzer final {
public:
    explicit Analyzer(MaterialDeclaration& material)
        : material_(material)
    {
        symbols_.emplace("normal", ValueType::Vec3);
        symbols_.emplace("lightDir", ValueType::Vec3);
        symbols_.emplace("baseColor", ValueType::Vec3);

        for (const VariableDeclaration& variable : material_.variables) {
            ++remainingDeclarations_[variable.name];
        }
    }

    [[nodiscard]] SemanticResult run()
    {
        for (VariableDeclaration& variable : material_.variables) {
            analyzeVariable(variable);
        }

        const ValueType outputType = analyzeExpression(*material_.output->expression);
        if (outputType != ValueType::Invalid && outputType != ValueType::Vec3) {
            addDiagnostic(
                material_.output->expression->location,
                "Output type mismatch: expected vec3, got " +
                    std::string(valueTypeName(outputType)) + ".");
        }

        std::stable_sort(
            diagnostics_.begin(),
            diagnostics_.end(),
            [](const Diagnostic& left, const Diagnostic& right) {
                return left.location.offset < right.location.offset;
            });
        return SemanticResult{std::move(diagnostics_)};
    }

private:
    void addDiagnostic(SourceLocation location, std::string message)
    {
        diagnostics_.push_back(
            Diagnostic{DiagnosticPhase::Semantic, location, std::move(message)});
    }

    void analyzeVariable(VariableDeclaration& variable)
    {
        const auto existing = symbols_.find(variable.name);
        const bool duplicate = existing != symbols_.end();
        if (duplicate) {
            if (variable.name == "normal" || variable.name == "lightDir" ||
                variable.name == "baseColor") {
                addDiagnostic(
                    variable.location,
                    "Cannot redeclare built-in symbol: " + variable.name + ".");
            } else {
                addDiagnostic(variable.location, "Duplicate variable: " + variable.name + ".");
            }
        }

        variable.inferredType = analyzeExpression(*variable.initializer);

        auto remaining = remainingDeclarations_.find(variable.name);
        if (remaining != remainingDeclarations_.end()) {
            if (remaining->second > 1U) {
                --remaining->second;
            } else {
                remainingDeclarations_.erase(remaining);
            }
        }

        if (!duplicate) {
            symbols_.emplace(variable.name, variable.inferredType);
        }
    }

    [[nodiscard]] ValueType analyzeExpression(Expression& expression)
    {
        ValueType type = ValueType::Invalid;
        switch (expression.kind) {
        case ExpressionKind::Literal:
            type = ValueType::Float;
            break;
        case ExpressionKind::Identifier:
            type = analyzeIdentifier(static_cast<IdentifierExpression&>(expression));
            break;
        case ExpressionKind::Binary:
            type = analyzeBinary(static_cast<BinaryExpression&>(expression));
            break;
        case ExpressionKind::Call:
            type = analyzeCall(static_cast<CallExpression&>(expression));
            break;
        }
        expression.inferredType = type;
        return type;
    }

    [[nodiscard]] ValueType analyzeIdentifier(const IdentifierExpression& identifier)
    {
        const auto symbol = symbols_.find(identifier.name);
        if (symbol != symbols_.end()) {
            return symbol->second;
        }

        if (remainingDeclarations_.contains(identifier.name)) {
            addDiagnostic(
                identifier.location, "Use before declaration: " + identifier.name + ".");
        } else {
            addDiagnostic(identifier.location, "Unknown identifier: " + identifier.name + ".");
        }
        return ValueType::Invalid;
    }

    [[nodiscard]] ValueType analyzeBinary(BinaryExpression& binary)
    {
        const ValueType left = analyzeExpression(*binary.left);
        const ValueType right = analyzeExpression(*binary.right);
        if (left == ValueType::Invalid || right == ValueType::Invalid) {
            return ValueType::Invalid;
        }

        if (left == ValueType::Float && right == ValueType::Float) {
            return ValueType::Float;
        }

        if ((binary.operatorKind == TokenKind::Plus || binary.operatorKind == TokenKind::Minus) &&
            left == right && isVectorType(left)) {
            return left;
        }

        if (binary.operatorKind == TokenKind::Star) {
            if (isVectorType(left) && right == ValueType::Float) {
                return left;
            }
            if (left == ValueType::Float && isVectorType(right)) {
                return right;
            }
        }

        if (binary.operatorKind == TokenKind::Slash && isVectorType(left) &&
            right == ValueType::Float) {
            return left;
        }

        addDiagnostic(
            binary.operatorLocation,
            "Unsupported operand types for '" + operatorText(binary.operatorKind) + "': " +
                std::string(valueTypeName(left)) + " and " +
                std::string(valueTypeName(right)) + ".");
        return ValueType::Invalid;
    }

    [[nodiscard]] ValueType analyzeCall(CallExpression& call)
    {
        const bool knownFunction = isKnownFunction(call.callee);
        if (!knownFunction) {
            if (symbols_.contains(call.callee)) {
                addDiagnostic(call.location, "Identifier is not callable: " + call.callee + ".");
            } else {
                addDiagnostic(call.location, "Unknown function: " + call.callee + ".");
            }
        } else if (call.arguments.size() != expectedArgumentCount(call.callee)) {
            addDiagnostic(
                call.location,
                "Wrong argument count for " + call.callee + ": expected " +
                    std::to_string(expectedArgumentCount(call.callee)) + ", got " +
                    std::to_string(call.arguments.size()) + ".");
        }

        std::vector<ValueType> arguments;
        arguments.reserve(call.arguments.size());
        bool validArguments = true;
        for (const auto& argument : call.arguments) {
            const ValueType type = analyzeExpression(*argument);
            arguments.push_back(type);
            validArguments = validArguments && type != ValueType::Invalid;
        }

        if (!knownFunction || call.arguments.size() != expectedArgumentCount(call.callee) ||
            !validArguments) {
            return ValueType::Invalid;
        }

        ValueType result = ValueType::Invalid;
        if (call.callee == "normalize" && isVectorType(arguments[0])) {
            result = arguments[0];
        } else if (call.callee == "dot" && isVectorType(arguments[0]) &&
                   arguments[0] == arguments[1]) {
            result = ValueType::Float;
        } else if ((call.callee == "max" || call.callee == "min") &&
                   arguments[0] == ValueType::Float && arguments[1] == ValueType::Float) {
            result = ValueType::Float;
        } else if (call.callee == "clamp" && arguments[0] == ValueType::Float &&
                   arguments[1] == ValueType::Float && arguments[2] == ValueType::Float) {
            result = ValueType::Float;
        } else if (call.callee == "vec2" && arguments[0] == ValueType::Float &&
                   arguments[1] == ValueType::Float) {
            result = ValueType::Vec2;
        } else if (call.callee == "vec3" && arguments[0] == ValueType::Float &&
                   arguments[1] == ValueType::Float && arguments[2] == ValueType::Float) {
            result = ValueType::Vec3;
        } else if (call.callee == "vec4" && arguments[0] == ValueType::Float &&
                   arguments[1] == ValueType::Float && arguments[2] == ValueType::Float &&
                   arguments[3] == ValueType::Float) {
            result = ValueType::Vec4;
        }

        if (result == ValueType::Invalid) {
            addDiagnostic(
                call.location,
                "Unsupported argument types for " + call.callee + "(" +
                    argumentTypeList(arguments) + ").");
        }
        return result;
    }

    MaterialDeclaration& material_;
    std::unordered_map<std::string, ValueType> symbols_;
    std::unordered_map<std::string, std::size_t> remainingDeclarations_;
    std::vector<Diagnostic> diagnostics_;
};

} // namespace

SemanticResult SemanticAnalyzer::analyze(MaterialDeclaration& material)
{
    return Analyzer(material).run();
}

} // namespace dentalviz::minishader
