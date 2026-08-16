#pragma once

#include <filesystem>
#include <optional>

namespace dentalviz {

[[nodiscard]] std::optional<std::filesystem::path> chooseMeshFile();

} // namespace dentalviz
