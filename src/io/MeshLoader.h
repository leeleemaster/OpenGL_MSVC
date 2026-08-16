#pragma once

#include "core/MeshData.h"

#include <chrono>
#include <filesystem>

namespace dentalviz {

struct MeshLoadResult {
    MeshData mesh;
    std::filesystem::path sourcePath;
    std::chrono::microseconds loadDuration{};
    std::size_t sourceMeshCount = 0;
};

class MeshLoader final {
public:
    [[nodiscard]] static MeshLoadResult load(const std::filesystem::path& path);
};

} // namespace dentalviz
