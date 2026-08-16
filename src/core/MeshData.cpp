#include "core/MeshData.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {

struct ToothProfilePoint {
    float height;
    float radius;
};

constexpr std::array<ToothProfilePoint, 9> toothProfile{{
    {-1.25F, 0.14F},
    {-1.05F, 0.29F},
    {-0.72F, 0.40F},
    {-0.38F, 0.48F},
    {-0.17F, 0.61F},
    {0.12F, 0.78F},
    {0.47F, 0.88F},
    {0.78F, 0.73F},
    {1.01F, 0.39F},
}};

constexpr float pi = 3.14159265358979323846F;

bool isFinite(const glm::vec3& value) noexcept
{
    return std::isfinite(value.x) &&
           std::isfinite(value.y) &&
           std::isfinite(value.z);
}

} // namespace

namespace dentalviz {

glm::vec3 AxisAlignedBounds::center() const noexcept
{
    return (minimum + maximum) * 0.5F;
}

glm::vec3 AxisAlignedBounds::size() const noexcept
{
    return maximum - minimum;
}

AxisAlignedBounds MeshData::bounds() const
{
    if (vertices.empty()) {
        throw std::runtime_error("Cannot calculate bounds for an empty mesh.");
    }

    const float highest = std::numeric_limits<float>::max();
    const float lowest = std::numeric_limits<float>::lowest();
    AxisAlignedBounds result{
        glm::vec3(highest),
        glm::vec3(lowest),
    };

    for (const Vertex& vertex : vertices) {
        if (!isFinite(vertex.position)) {
            throw std::invalid_argument("Cannot calculate bounds for non-finite vertex positions.");
        }
        result.minimum = glm::min(result.minimum, vertex.position);
        result.maximum = glm::max(result.maximum, vertex.position);
    }
    return result;
}

bool MeshData::hasValidIndices() const noexcept
{
    if (vertices.empty() || indices.empty() || indices.size() % 3 != 0) {
        return false;
    }

    for (const std::uint32_t index : indices) {
        if (index >= vertices.size()) {
            return false;
        }
    }
    return true;
}

bool MeshData::hasFiniteVertexData() const noexcept
{
    return std::all_of(vertices.begin(), vertices.end(), [](const Vertex& vertex) {
        return isFinite(vertex.position) && isFinite(vertex.normal);
    });
}

bool MeshData::isRenderable() const noexcept
{
    return hasValidIndices() && hasFiniteVertexData();
}

void recalculateSmoothNormals(MeshData& mesh)
{
    if (!mesh.hasValidIndices()) {
        throw std::runtime_error("Cannot calculate normals for invalid mesh indices.");
    }
    if (!std::all_of(mesh.vertices.begin(), mesh.vertices.end(), [](const Vertex& vertex) {
            return isFinite(vertex.position);
        })) {
        throw std::invalid_argument("Cannot calculate normals for non-finite vertex positions.");
    }

    for (Vertex& vertex : mesh.vertices) {
        vertex.normal = glm::vec3(0.0F);
    }

    for (std::size_t index = 0; index < mesh.indices.size(); index += 3) {
        Vertex& first = mesh.vertices[mesh.indices[index]];
        Vertex& second = mesh.vertices[mesh.indices[index + 1]];
        Vertex& third = mesh.vertices[mesh.indices[index + 2]];

        const glm::vec3 faceNormal = glm::cross(
            second.position - first.position,
            third.position - first.position);
        if (glm::dot(faceNormal, faceNormal) <= std::numeric_limits<float>::epsilon()) {
            continue;
        }

        first.normal += faceNormal;
        second.normal += faceNormal;
        third.normal += faceNormal;
    }

    for (Vertex& vertex : mesh.vertices) {
        const float squaredLength = glm::dot(vertex.normal, vertex.normal);
        if (squaredLength <= std::numeric_limits<float>::epsilon()) {
            vertex.normal = glm::vec3(0.0F, 1.0F, 0.0F);
        } else {
            vertex.normal = glm::normalize(vertex.normal);
        }
    }
}

MeshData makeProceduralTooth(std::uint32_t radialSegments)
{
    if (radialSegments < 8) {
        throw std::invalid_argument("A procedural tooth requires at least 8 radial segments.");
    }

    constexpr std::uint32_t maximumRadialSegments =
        (std::numeric_limits<std::uint32_t>::max() - 2U) /
        static_cast<std::uint32_t>(toothProfile.size());
    if (radialSegments > maximumRadialSegments) {
        throw std::overflow_error("Procedural tooth exceeds the 32-bit index limit.");
    }

    MeshData mesh;
    const std::size_t ringCount = toothProfile.size();
    mesh.vertices.reserve(2 + ringCount * radialSegments);
    mesh.indices.reserve(static_cast<std::size_t>(radialSegments) * ringCount * 6);

    mesh.vertices.push_back({glm::vec3(0.0F, -1.48F, 0.0F), {}});

    for (const ToothProfilePoint point : toothProfile) {
        for (std::uint32_t segment = 0; segment < radialSegments; ++segment) {
            const float angle = 2.0F * pi * static_cast<float>(segment) /
                                static_cast<float>(radialSegments);
            const float crownLobes = point.height > 0.0F
                ? 1.0F + 0.055F * std::cos(4.0F * angle)
                : 1.0F;
            const float x = point.radius * 0.84F * crownLobes * std::cos(angle);
            const float z = point.radius * crownLobes * std::sin(angle);
            mesh.vertices.push_back({glm::vec3(x, point.height, z), {}});
        }
    }

    const std::uint32_t topIndex = static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back({glm::vec3(0.0F, 1.19F, 0.0F), {}});

    const auto ringVertex = [radialSegments](std::size_t ring, std::uint32_t segment) {
        return 1U + static_cast<std::uint32_t>(ring) * radialSegments + segment;
    };

    for (std::uint32_t segment = 0; segment < radialSegments; ++segment) {
        const std::uint32_t next = (segment + 1) % radialSegments;
        mesh.indices.insert(mesh.indices.end(), {
            0U,
            ringVertex(0, segment),
            ringVertex(0, next),
        });
    }

    for (std::size_t ring = 0; ring + 1 < ringCount; ++ring) {
        for (std::uint32_t segment = 0; segment < radialSegments; ++segment) {
            const std::uint32_t next = (segment + 1) % radialSegments;
            const std::uint32_t lower = ringVertex(ring, segment);
            const std::uint32_t lowerNext = ringVertex(ring, next);
            const std::uint32_t upper = ringVertex(ring + 1, segment);
            const std::uint32_t upperNext = ringVertex(ring + 1, next);
            mesh.indices.insert(mesh.indices.end(), {
                lower, upper, upperNext,
                lower, upperNext, lowerNext,
            });
        }
    }

    const std::size_t lastRing = ringCount - 1;
    for (std::uint32_t segment = 0; segment < radialSegments; ++segment) {
        const std::uint32_t next = (segment + 1) % radialSegments;
        mesh.indices.insert(mesh.indices.end(), {
            ringVertex(lastRing, segment),
            topIndex,
            ringVertex(lastRing, next),
        });
    }

    recalculateSmoothNormals(mesh);
    return mesh;
}

} // namespace dentalviz
