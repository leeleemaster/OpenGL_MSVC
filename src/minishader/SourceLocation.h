#pragma once

#include <cstddef>

namespace dentalviz::minishader {

struct SourceLocation {
    std::size_t offset = 0;
    std::size_t line = 1;
    std::size_t column = 1;

    [[nodiscard]] bool operator==(const SourceLocation&) const noexcept = default;
};

} // namespace dentalviz::minishader
