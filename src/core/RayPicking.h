#pragma once

#include "core/MeshData.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <optional>

namespace dentalviz {

struct Ray {
    glm::vec3 origin{};
    glm::vec3 direction{0.0F, 0.0F, -1.0F};
};

struct RayHit {
    float distance = 0.0F;
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec3 barycentric{};
    std::size_t triangleIndex = 0;
};

[[nodiscard]] Ray makeWorldRayFromViewport(
    float normalizedX,
    float normalizedY,
    const glm::mat4& view,
    const glm::mat4& projection);

[[nodiscard]] Ray transformRay(const Ray& ray, const glm::mat4& transform);

[[nodiscard]] bool intersectsBounds(
    const Ray& ray,
    const AxisAlignedBounds& bounds,
    float& entryDistance,
    float& exitDistance);

[[nodiscard]] std::optional<RayHit> intersectTriangle(
    const Ray& ray,
    const Vertex& first,
    const Vertex& second,
    const Vertex& third,
    std::size_t triangleIndex = 0);

[[nodiscard]] std::optional<RayHit> pickMesh(const Ray& ray, const MeshData& mesh);

} // namespace dentalviz
