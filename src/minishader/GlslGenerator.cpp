#include "minishader/GlslGenerator.h"

#include "minishader/Type.h"

#include <cstddef>
#include <string>
#include <utility>

namespace dentalviz::minishader {
namespace {

constexpr const char* fragmentHeader = R"glsl(#version 330 core

in vec3 worldPosition;
in vec3 worldNormal;
in vec3 modelPosition;

uniform vec3 uBaseColor;
uniform vec3 uLightPosition;
uniform int uRenderMode;
uniform int uClipEnabled;
uniform vec3 uClipNormal;
uniform float uClipDistance;

out vec4 FragColor;

void main()
{
    if (uClipEnabled != 0 &&
        dot(modelPosition, uClipNormal) + uClipDistance > 0.0) {
        discard;
    }

    vec3 vNormal = normalize(worldNormal);
    if (uRenderMode == 2) {
        FragColor = vec4(vNormal * 0.5 + 0.5, 1.0);
        return;
    }

    vec3 uLightDir = normalize(uLightPosition - worldPosition);
)glsl";

[[nodiscard]] bool isValidatedExpression(const Expression& expression)
{
    if (expression.inferredType == ValueType::Invalid) {
        return false;
    }

    switch (expression.kind) {
    case ExpressionKind::Literal:
    case ExpressionKind::Identifier:
        return true;
    case ExpressionKind::Binary: {
        const auto& binary = static_cast<const BinaryExpression&>(expression);
        return isValidatedExpression(*binary.left) && isValidatedExpression(*binary.right);
    }
    case ExpressionKind::Call: {
        const auto& call = static_cast<const CallExpression&>(expression);
        for (const auto& argument : call.arguments) {
            if (!isValidatedExpression(*argument)) {
                return false;
            }
        }
        return true;
    }
    }

    return false;
}

[[nodiscard]] bool isValidatedMaterial(const MaterialDeclaration& material)
{
    if (material.output == nullptr || material.output->expression == nullptr ||
        material.output->expression->inferredType != ValueType::Vec3 ||
        !isValidatedExpression(*material.output->expression)) {
        return false;
    }

    for (const VariableDeclaration& variable : material.variables) {
        if (variable.inferredType == ValueType::Invalid || variable.initializer == nullptr ||
            !isValidatedExpression(*variable.initializer)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::string generatedIdentifier(const std::string& name)
{
    if (name == "normal") {
        return "vNormal";
    }
    if (name == "lightDir") {
        return "uLightDir";
    }
    if (name == "baseColor") {
        return "uBaseColor";
    }
    return "_ms_" + name;
}

[[nodiscard]] std::string binaryOperator(TokenKind kind)
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
        return "?";
    }
}

[[nodiscard]] std::string generateExpression(const Expression& expression)
{
    switch (expression.kind) {
    case ExpressionKind::Literal: {
        const auto& literal = static_cast<const LiteralExpression&>(expression);
        if (literal.lexeme.find('.') == std::string::npos) {
            return literal.lexeme + ".0";
        }
        return literal.lexeme;
    }
    case ExpressionKind::Identifier: {
        const auto& identifier = static_cast<const IdentifierExpression&>(expression);
        return generatedIdentifier(identifier.name);
    }
    case ExpressionKind::Binary: {
        const auto& binary = static_cast<const BinaryExpression&>(expression);
        return "(" + generateExpression(*binary.left) + " " +
               binaryOperator(binary.operatorKind) + " " + generateExpression(*binary.right) +
               ")";
    }
    case ExpressionKind::Call: {
        const auto& call = static_cast<const CallExpression&>(expression);
        std::string result = call.callee + "(";
        for (std::size_t index = 0; index < call.arguments.size(); ++index) {
            if (index != 0U) {
                result += ", ";
            }
            result += generateExpression(*call.arguments[index]);
        }
        result += ")";
        return result;
    }
    }

    return {};
}

void appendSourceMapping(std::string& source, SourceLocation location)
{
    source += "    // MiniShader line " + std::to_string(location.line) + ", column " +
              std::to_string(location.column) + "\n";
    source += "#line " + std::to_string(location.line) + " 1\n";
}

} // namespace

GlslGenerationResult GlslGenerator::generate(const MaterialDeclaration& material)
{
    if (!isValidatedMaterial(material)) {
        return GlslGenerationResult{
            {},
            "GLSL 생성 전에 MiniShader AST 의미 검증을 통과해야 합니다.",
        };
    }

    std::string source = fragmentHeader;
    source += "\n    // MiniShader material " + material.name + "\n";

    for (const VariableDeclaration& variable : material.variables) {
        appendSourceMapping(source, variable.location);
        source += "    " + std::string(valueTypeName(variable.inferredType)) + " " +
                  generatedIdentifier(variable.name) + " = " +
                  generateExpression(*variable.initializer) + ";\n";
    }

    source += '\n';
    appendSourceMapping(source, material.output->expression->location);
    source += "    FragColor = vec4(" + generateExpression(*material.output->expression) +
              ", 1.0);\n";
    source += "}\n";
    return GlslGenerationResult{std::move(source), {}};
}

} // namespace dentalviz::minishader
