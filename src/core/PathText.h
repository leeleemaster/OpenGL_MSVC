#pragma once

#include <filesystem>
#include <string>

namespace dentalviz {

[[nodiscard]] std::string pathToUtf8(const std::filesystem::path& path);

} // namespace dentalviz
