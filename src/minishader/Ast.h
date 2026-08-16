#pragma once

#include "minishader/SourceLocation.h"
#include "minishader/Token.h"
#include "minishader/Type.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace dentalviz::minishader {

enum class ExpressionKind {
    Binary,
    Call,
    Identifier,
    Literal,
};

struct Expression {
    Expression(ExpressionKind expressionKind, SourceLocation sourceLocation)
        : kind(expressionKind)
        , location(sourceLocation)
    {
    }

    virtual ~Expression() = default;

    ExpressionKind kind;
    SourceLocation location;
    ValueType inferredType = ValueType::Invalid;
};

struct LiteralExpression final : Expression {
    LiteralExpression(std::string literalLexeme, SourceLocation sourceLocation)
        : Expression(ExpressionKind::Literal, sourceLocation)
        , lexeme(std::move(literalLexeme))
    {
    }

    std::string lexeme;
};

struct IdentifierExpression final : Expression {
    IdentifierExpression(std::string identifierName, SourceLocation sourceLocation)
        : Expression(ExpressionKind::Identifier, sourceLocation)
        , name(std::move(identifierName))
    {
    }

    std::string name;
};

struct BinaryExpression final : Expression {
    BinaryExpression(
        std::unique_ptr<Expression> leftExpression,
        TokenKind binaryOperator,
        SourceLocation binaryOperatorLocation,
        std::unique_ptr<Expression> rightExpression)
        : Expression(ExpressionKind::Binary, leftExpression->location)
        , left(std::move(leftExpression))
        , operatorKind(binaryOperator)
        , operatorLocation(binaryOperatorLocation)
        , right(std::move(rightExpression))
    {
    }

    std::unique_ptr<Expression> left;
    TokenKind operatorKind = TokenKind::Plus;
    SourceLocation operatorLocation;
    std::unique_ptr<Expression> right;
};

struct CallExpression final : Expression {
    CallExpression(
        std::string functionName,
        SourceLocation sourceLocation,
        std::vector<std::unique_ptr<Expression>> functionArguments)
        : Expression(ExpressionKind::Call, sourceLocation)
        , callee(std::move(functionName))
        , arguments(std::move(functionArguments))
    {
    }

    std::string callee;
    std::vector<std::unique_ptr<Expression>> arguments;
};

struct VariableDeclaration {
    SourceLocation location;
    std::string name;
    std::unique_ptr<Expression> initializer;
    ValueType inferredType = ValueType::Invalid;
};

struct OutputStatement {
    SourceLocation location;
    std::unique_ptr<Expression> expression;
};

struct MaterialDeclaration {
    SourceLocation location;
    SourceLocation nameLocation;
    std::string name;
    std::vector<VariableDeclaration> variables;
    std::unique_ptr<OutputStatement> output;
};

} // namespace dentalviz::minishader
