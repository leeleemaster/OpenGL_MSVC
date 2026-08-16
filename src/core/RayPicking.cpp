#include "core/RayPicking.h"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {

constexpr float directionEpsilon = 1.0e-7F;
constexpr float hitEpsilon = 1.0e-6F;

bool isFinite(const glm::vec3& value) noexcept
{
    return std::isfinite(value.x) &&
           std::isfinite(value.y) &&
           std::isfinite(value.z);
}

bool isFinite(const glm::vec4& value) noexcept
{
    return std::isfinite(value.x) &&
           std::isfinite(value.y) &&
           std::isfinite(value.z) &&
           std::isfinite(value.w);
}

bool isFinite(const glm::mat4& value) noexcept
{
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            if (!std::isfinite(value[column][row])) {
                return false;
            }
        }
    }
    return true;
}

dentalviz::Ray normalizedRay(const dentalviz::Ray& ray)
{
    if (!isFinite(ray.origin) || !isFinite(ray.direction)) {
        throw std::invalid_argument("Ray values must be finite.");
    }

    const float squaredLength = glm::dot(ray.direction, ray.direction);
    if (!std::isfinite(squaredLength) || squaredLength <= directionEpsilon) {
        throw std::invalid_argument("Ray direction must be non-zero.");
    }
    return dentalviz::Ray{ray.origin, ray.direction / std::sqrt(squaredLength)};
}

std::optional<dentalviz::RayHit> intersectTriangleNormalized(
    const dentalviz::Ray& ray,
    const dentalviz::Vertex& first,
    const dentalviz::Vertex& second,
    const dentalviz::Vertex& third,
    std::size_t triangleIndex)
{
    const glm::vec3 firstEdge = second.position - first.position;
    const glm::vec3 secondEdge = third.position - first.position;
    const glm::vec3 crossDirection = glm::cross(ray.direction, secondEdge);
    const float determinant = glm::dot(firstEdge, crossDirection);
    if (std::abs(determinant) <= directionEpsilon) {
        return std::nullopt;
    }

    const float inverseDeterminant = 1.0F / determinant;
    const glm::vec3 originOffset = ray.origin - first.position;
    const float secondWeight = glm::dot(originOffset, crossDirection) * inverseDeterminant;
    if (secondWeight < 0.0F || secondWeight > 1.0F) {
        return std::nullopt;
    }

    const glm::vec3 crossOffset = glm::cross(originOffset, firstEdge);
    const float thirdWeight = glm::dot(ray.direction, crossOffset) * inverseDeterminant;
    if (thirdWeight < 0.0F || secondWeight + thirdWeight > 1.0F) {
        return std::nullopt;
    }

    const float distance = glm::dot(secondEdge, crossOffset) * inverseDeterminant;
    if (!std::isfinite(distance) || distance <= hitEpsilon) {
        return std::nullopt;
    }

    const float firstWeight = 1.0F - secondWeight - thirdWeight;
    glm::vec3 normal =
        first.normal * firstWeight +
        second.normal * secondWeight +
        third.normal * thirdWeight;
    float squaredNormalLength = glm::dot(normal, normal);
    if (!std::isfinite(squaredNormalLength) || squaredNormalLength <= directionEpsilon) {
        normal = glm::cross(firstEdge, secondEdge);
        squaredNormalLength = glm::dot(normal, normal);
    }
    if (!std::isfinite(squaredNormalLength) || squaredNormalLength <= directionEpsilon) {
        return std::nullopt;
    }
    normal /= std::sqrt(squaredNormalLength);

    return dentalviz::RayHit{
        distance,
        ray.origin + ray.direction * distance,
        normal,
        glm::vec3(firstWeight, secondWeight, thirdWeight),
        triangleIndex,
    };
}

} // namespace

namespace dentalviz {

Ray makeWorldRayFromViewport(
    float normalizedX,
    float normalizedY,
    const glm::mat4& view,
    const glm::mat4& projection)
{
    if (!std::isfinite(normalizedX) || !std::isfinite(normalizedY) ||
        normalizedX < 0.0F || normalizedX > 1.0F ||
        normalizedY < 0.0F || normalizedY > 1.0F) {
        throw std::invalid_argument("Viewport coordinates must be finite values from 0 to 1.");
    }
    if (!isFinite(view) || !isFinite(projection)) {
        throw std::invalid_argument("View and projection matrices must be finite.");
    }

    const glm::mat4 inverseView = glm::inverse(view);
    const glm::mat4 inverseViewProjection = glm::inverse(projection * view);
    if (!isFinite(inverseView) || !isFinite(inverseViewProjection)) {
        throw std::invalid_argument("View and projection matrices must be invertible.");
    }

    const glm::vec4 cameraHomogeneous = inverseView * glm::vec4(0.0F, 0.0F, 0.0F, 1.0F);
    const glm::vec4 farHomogeneous = inverseViewProjection * glm::vec4(
        normalizedX * 2.0F - 1.0F,
        1.0F - normalizedY * 2.0F,
        1.0F,
        1.0F);
    if (!isFinite(cameraHomogeneous) || !isFinite(farHomogeneous) ||
        std::abs(cameraHomogeneous.w) <= directionEpsilon ||
        std::abs(farHomogeneous.w) <= directionEpsilon) {
        throw std::invalid_argument("Viewport ray could not be unprojected.");
    }

    const glm::vec3 origin = glm::vec3(cameraHomogeneous) / cameraHomogeneous.w;
    const glm::vec3 farPoint = glm::vec3(farHomogeneous) / farHomogeneous.w;
    return normalizedRay(Ray{origin, farPoint - origin});
}

Ray transformRay(const Ray& ray, const glm::mat4& transform)
{
    if (!isFinite(transform)) {
        throw std::invalid_argument("Ray transform must be finite.");
    }

    const Ray source = normalizedRay(ray);
    const glm::vec4 transformedOrigin = transform * glm::vec4(source.origin, 1.0F);
    const glm::vec4 transformedDirection = transform * glm::vec4(source.direction, 0.0F);
    if (!isFinite(transformedOrigin) || !isFinite(transformedDirection) ||
        std::abs(transformedOrigin.w) <= directionEpsilon) {
        throw std::invalid_argument("Ray transform produced invalid values.");
    }
    return normalizedRay(Ray{
        glm::vec3(transformedOrigin) / transformedOrigin.w,
        glm::vec3(transformedDirection),
    });
}

bool intersectsBounds(
    const Ray& ray,
    const AxisAlignedBounds& bounds,
    float& entryDistance,
    float& exitDistance)
{
    const Ray normalized = normalizedRay(ray);
    if (!isFinite(bounds.minimum) || !isFinite(bounds.maximum) ||
        bounds.minimum.x > bounds.maximum.x ||
        bounds.minimum.y > bounds.maximum.y ||
        bounds.minimum.z > bounds.maximum.z) {
        throw std::invalid_argument("Bounds must be finite and ordered.");
    }

    float entry = 0.0F;
    float exit = std::numeric_limits<float>::max();
    for (int axis = 0; axis < 3; ++axis) {
        const float direction = normalized.direction[axis];
        const float origin = normalized.origin[axis];
        if (std::abs(direction) <= directionEpsilon) {
            if (origin < bounds.minimum[axis] || origin > bounds.maximum[axis]) {
                return false;
            }
            continue;
        }

        float firstDistance = (bounds.minimum[axis] - origin) / direction;
        float secondDistance = (bounds.maximum[axis] - origin) / direction;
        if (firstDistance > secondDistance) {
            std::swap(firstDistance, secondDistance);
        }
        entry = std::max(entry, firstDistance);
        exit = std::min(exit, secondDistance);
        if (exit < entry) {
            return false;
        }
    }

    entryDistance = entry;
    exitDistance = exit;
    return exit >= 0.0F;
}

std::optional<RayHit> intersectTriangle(
    const Ray& ray,
    const Vertex& first,
    const Vertex& second,
    const Vertex& third,
    std::size_t triangleIndex)
{
    return intersectTriangleNormalized(
        normalizedRay(ray), first, second, third, triangleIndex);
}

std::optional<RayHit> pickMesh(const Ray& ray, const MeshData& mesh)
{
    if (!mesh.hasValidIndices()) {
        throw std::invalid_argument("Cannot pick an invalid mesh.");
    }

    const Ray normalized = normalizedRay(ray);
    float boundsEntry = 0.0F;
    float boundsExit = 0.0F;
    if (!intersectsBounds(normalized, mesh.bounds(), boundsEntry, boundsExit)) {
        return std::nullopt;
    }

    std::optional<RayHit> closestHit;
    for (std::size_t index = 0; index < mesh.indices.size(); index += 3) {
        const std::optional<RayHit> hit = intersectTriangleNormalized(
            normalized,
            mesh.vertices[mesh.indices[index]],
            mesh.vertices[mesh.indices[index + 1]],
            mesh.vertices[mesh.indices[index + 2]],
            index / 3);
        if (hit.has_value() &&
            (!closestHit.has_value() || hit->distance < closestHit->distance)) {
            closestHit = hit;
        }
    }
    return closestHit;
}

} // namespace dentalviz
