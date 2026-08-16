#pragma once

#include "core/MeshData.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace dentalviz {

class OrbitCamera final {
public:
    explicit OrbitCamera(float verticalFieldOfViewDegrees = 42.0F);

    void fit(const AxisAlignedBounds& bounds, float aspectRatio);
    void orbit(float horizontalPixels, float verticalPixels) noexcept;
    void pan(float horizontalPixels, float verticalPixels, float viewportHeightPixels) noexcept;
    void zoom(float wheelOffset) noexcept;

    [[nodiscard]] glm::mat4 viewMatrix() const;
    [[nodiscard]] glm::mat4 projectionMatrix(float aspectRatio) const;
    [[nodiscard]] glm::vec3 position() const noexcept;
    [[nodiscard]] const glm::vec3& target() const noexcept;
    [[nodiscard]] float distance() const noexcept;
    [[nodiscard]] float yawRadians() const noexcept;
    [[nodiscard]] float pitchRadians() const noexcept;

private:
    [[nodiscard]] float minimumDistance() const noexcept;
    [[nodiscard]] float maximumDistance() const noexcept;

    glm::vec3 target_{0.0F};
    float distance_ = 5.0F;
    float yawRadians_ = 0.0F;
    float pitchRadians_ = 0.12F;
    float verticalFieldOfViewRadians_ = 0.0F;
    float sceneRadius_ = 1.0F;
};

} // namespace dentalviz
