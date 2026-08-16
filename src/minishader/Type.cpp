#include "minishader/Type.h"

namespace dentalviz::minishader {

std::string_view valueTypeName(ValueType type) noexcept
{
    switch (type) {
    case ValueType::Invalid:
        return "invalid";
    case ValueType::Float:
        return "float";
    case ValueType::Vec2:
        return "vec2";
    case ValueType::Vec3:
        return "vec3";
    case ValueType::Vec4:
        return "vec4";
    }

    return "invalid";
}

bool isVectorType(ValueType type) noexcept
{
    return type == ValueType::Vec2 || type == ValueType::Vec3 || type == ValueType::Vec4;
}

} // namespace dentalviz::minishader
