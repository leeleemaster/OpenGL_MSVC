#pragma once

#include "core/MeshData.h"

#include <glm/vec3.hpp>

#include <utility>

namespace dentalviz {

enum class ClipAxis : int {
    x = 0,
    y = 1,
    z = 2,
};

[[nodiscard]] const char* clipAxisName(ClipAxis axis) noexcept;

class ClippingPlane final {
public:
    [[nodiscard]] bool enabled() const noexcept;
    void setEnabled(bool enabled) noexcept;

    [[nodiscard]] ClipAxis axis() const noexcept;
    void setAxis(ClipAxis axis, const AxisAlignedBounds& bounds);

    [[nodiscard]] float distance() const noexcept;
    void setDistance(float distance, const AxisAlignedBounds& bounds);

    void reset(const AxisAlignedBounds& bounds);

    [[nodiscard]] glm::vec3 normal() const noexcept;
    [[nodiscard]] std::pair<float, float> distanceRange(
        const AxisAlignedBounds& bounds) const;
    [[nodiscard]] float signedDistance(const glm::vec3& modelPosition) const noexcept;
    [[nodiscard]] bool discards(const glm::vec3& modelPosition) const noexcept;

private:
    bool enabled_ = false;
    ClipAxis axis_ = ClipAxis::x;
    float distance_ = 0.0F;
};

} // namespace dentalviz
