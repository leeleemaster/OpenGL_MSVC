#pragma once

#include <string_view>

namespace dentalviz::minishader {

enum class ValueType {
    Invalid,
    Float,
    Vec2,
    Vec3,
    Vec4,
};

[[nodiscard]] std::string_view valueTypeName(ValueType type) noexcept;
[[nodiscard]] bool isVectorType(ValueType type) noexcept;

} // namespace dentalviz::minishader
