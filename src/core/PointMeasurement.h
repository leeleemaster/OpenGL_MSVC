#pragma once

#include "core/RayPicking.h"

#include <optional>

namespace dentalviz {

enum class MeasurementUpdate {
    pointASet,
    pointBSet,
    restartedWithPointA,
};

class PointMeasurement final {
public:
    [[nodiscard]] MeasurementUpdate select(const RayHit& hit);
    void reset() noexcept;

    [[nodiscard]] const std::optional<RayHit>& pointA() const noexcept;
    [[nodiscard]] const std::optional<RayHit>& pointB() const noexcept;
    [[nodiscard]] std::optional<float> distance() const noexcept;

private:
    std::optional<RayHit> pointA_;
    std::optional<RayHit> pointB_;
};

} // namespace dentalviz
