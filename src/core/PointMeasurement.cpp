#include "core/PointMeasurement.h"

#include <glm/geometric.hpp>

namespace dentalviz {

MeasurementUpdate PointMeasurement::select(const RayHit& hit)
{
    if (!pointA_.has_value()) {
        pointA_ = hit;
        return MeasurementUpdate::pointASet;
    }
    if (!pointB_.has_value()) {
        pointB_ = hit;
        return MeasurementUpdate::pointBSet;
    }

    pointA_ = hit;
    pointB_.reset();
    return MeasurementUpdate::restartedWithPointA;
}

void PointMeasurement::reset() noexcept
{
    pointA_.reset();
    pointB_.reset();
}

const std::optional<RayHit>& PointMeasurement::pointA() const noexcept
{
    return pointA_;
}

const std::optional<RayHit>& PointMeasurement::pointB() const noexcept
{
    return pointB_;
}

std::optional<float> PointMeasurement::distance() const noexcept
{
    if (!pointA_.has_value() || !pointB_.has_value()) {
        return std::nullopt;
    }
    return glm::length(pointB_->position - pointA_->position);
}

} // namespace dentalviz
