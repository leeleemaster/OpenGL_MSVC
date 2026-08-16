#include "core/ClippingPlane.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {

float axisComponent(const glm::vec3& value, dentalviz::ClipAxis axis)
{
    switch (axis) {
    case dentalviz::ClipAxis::x:
        return value.x;
    case dentalviz::ClipAxis::y:
        return value.y;
    case dentalviz::ClipAxis::z:
        return value.z;
    }
    throw std::invalid_argument("Clipping plane axis is invalid.");
}

void validateBoundsComponent(float minimum, float maximum)
{
    if (!std::isfinite(minimum) || !std::isfinite(maximum)) {
        throw std::invalid_argument("Clipping plane bounds must be finite.");
    }
}

} // namespace

namespace dentalviz {

const char* clipAxisName(ClipAxis axis) noexcept
{
    switch (axis) {
    case ClipAxis::x:
        return "+X";
    case ClipAxis::y:
        return "+Y";
    case ClipAxis::z:
        return "+Z";
    }
    return "Unknown";
}

bool ClippingPlane::enabled() const noexcept
{
    return enabled_;
}

void ClippingPlane::setEnabled(bool enabled) noexcept
{
    enabled_ = enabled;
}

ClipAxis ClippingPlane::axis() const noexcept
{
    return axis_;
}

void ClippingPlane::setAxis(ClipAxis axis, const AxisAlignedBounds& bounds)
{
    static_cast<void>(axisComponent(bounds.minimum, axis));
    axis_ = axis;
    const auto [minimumDistance, maximumDistance] = distanceRange(bounds);
    distance_ = (minimumDistance + maximumDistance) * 0.5F;
}

float ClippingPlane::distance() const noexcept
{
    return distance_;
}

void ClippingPlane::setDistance(float distance, const AxisAlignedBounds& bounds)
{
    if (!std::isfinite(distance)) {
        throw std::invalid_argument("Clipping plane distance must be finite.");
    }
    const auto [minimumDistance, maximumDistance] = distanceRange(bounds);
    distance_ = std::clamp(distance, minimumDistance, maximumDistance);
}

void ClippingPlane::reset(const AxisAlignedBounds& bounds)
{
    setAxis(ClipAxis::x, bounds);
}

glm::vec3 ClippingPlane::normal() const noexcept
{
    switch (axis_) {
    case ClipAxis::x:
        return glm::vec3(1.0F, 0.0F, 0.0F);
    case ClipAxis::y:
        return glm::vec3(0.0F, 1.0F, 0.0F);
    case ClipAxis::z:
        return glm::vec3(0.0F, 0.0F, 1.0F);
    }
    return glm::vec3(1.0F, 0.0F, 0.0F);
}

std::pair<float, float> ClippingPlane::distanceRange(
    const AxisAlignedBounds& bounds) const
{
    const float first = axisComponent(bounds.minimum, axis_);
    const float second = axisComponent(bounds.maximum, axis_);
    validateBoundsComponent(first, second);
    const float minimumPosition = std::min(first, second);
    const float maximumPosition = std::max(first, second);
    return {-maximumPosition, -minimumPosition};
}

float ClippingPlane::signedDistance(const glm::vec3& modelPosition) const noexcept
{
    return glm::dot(modelPosition, normal()) + distance_;
}

bool ClippingPlane::discards(const glm::vec3& modelPosition) const noexcept
{
    return enabled_ && signedDistance(modelPosition) > 0.0F;
}

} // namespace dentalviz
