#pragma once

#include <glm/vec3.hpp>

#include <cstdint>
#include <vector>

namespace dentalviz {

struct Vertex {
    glm::vec3 position{};
    glm::vec3 normal{};
};

struct AxisAlignedBounds {
    glm::vec3 minimum{};
    glm::vec3 maximum{};

    [[nodiscard]] glm::vec3 center() const noexcept;
    [[nodiscard]] glm::vec3 size() const noexcept;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;

    [[nodiscard]] AxisAlignedBounds bounds() const;
    [[nodiscard]] bool hasValidIndices() const noexcept;
};

void recalculateSmoothNormals(MeshData& mesh);
[[nodiscard]] MeshData makeProceduralTooth(std::uint32_t radialSegments = 64);

} // namespace dentalviz
