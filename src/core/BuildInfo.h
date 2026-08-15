#pragma once

#include <string_view>

namespace dentalviz
{
[[nodiscard]] constexpr std::string_view projectName() noexcept
{
    return "DentalViz";
}

[[nodiscard]] std::string_view projectVersion() noexcept;
} // namespace dentalviz
