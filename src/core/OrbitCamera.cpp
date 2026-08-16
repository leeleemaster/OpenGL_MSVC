#include "core/OrbitCamera.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {

constexpr float orbitRadiansPerPixel = 0.0045F;
constexpr float zoomExponentPerStep = 0.14F;
constexpr float fitMargin = 1.10F;
constexpr float minimumPitch = -glm::half_pi<float>() + 0.01F;
constexpr float maximumPitch = glm::half_pi<float>() - 0.01F;

bool isPositiveFinite(float value) noexcept
{
    return std::isfinite(value) && value > 0.0F;
}

} // namespace

namespace dentalviz {

OrbitCamera::OrbitCamera(float verticalFieldOfViewDegrees)
    : verticalFieldOfViewRadians_(glm::radians(verticalFieldOfViewDegrees))
{
    if (!isPositiveFinite(verticalFieldOfViewDegrees) || verticalFieldOfViewDegrees >= 179.0F) {
        throw std::invalid_argument("Camera field of view must be between 0 and 179 degrees.");
    }
}

void OrbitCamera::fit(const AxisAlignedBounds& bounds, float aspectRatio)
{
    if (!isPositiveFinite(aspectRatio)) {
        throw std::invalid_argument("Camera aspect ratio must be positive and finite.");
    }

    target_ = bounds.center();
    sceneRadius_ = 0.5F * glm::length(bounds.size());
    if (!isPositiveFinite(sceneRadius_)) {
        sceneRadius_ = 0.5F;
    }

    const float halfVerticalFieldOfView = verticalFieldOfViewRadians_ * 0.5F;
    const float halfHorizontalFieldOfView = std::atan(
        std::tan(halfVerticalFieldOfView) * aspectRatio);
    const float limitingHalfFieldOfView = std::min(
        halfVerticalFieldOfView,
        halfHorizontalFieldOfView);
    distance_ = fitMargin * sceneRadius_ / std::sin(limitingHalfFieldOfView);
    distance_ = std::clamp(distance_, minimumDistance(), maximumDistance());
}

void OrbitCamera::orbit(float horizontalPixels, float verticalPixels) noexcept
{
    yawRadians_ -= horizontalPixels * orbitRadiansPerPixel;
    pitchRadians_ -= verticalPixels * orbitRadiansPerPixel;
    pitchRadians_ = std::clamp(pitchRadians_, minimumPitch, maximumPitch);

    if (std::abs(yawRadians_) > glm::two_pi<float>()) {
        yawRadians_ = std::remainder(yawRadians_, glm::two_pi<float>());
    }
}

void OrbitCamera::pan(
    float horizontalPixels,
    float verticalPixels,
    float viewportHeightPixels) noexcept
{
    if (!isPositiveFinite(viewportHeightPixels)) {
        return;
    }

    const glm::vec3 forward = glm::normalize(target_ - position());
    const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0F, 1.0F, 0.0F)));
    const glm::vec3 up = glm::normalize(glm::cross(right, forward));
    const float worldUnitsPerPixel =
        2.0F * distance_ * std::tan(verticalFieldOfViewRadians_ * 0.5F) /
        viewportHeightPixels;

    target_ += (-right * horizontalPixels + up * verticalPixels) * worldUnitsPerPixel;
}

void OrbitCamera::zoom(float wheelOffset) noexcept
{
    if (!std::isfinite(wheelOffset)) {
        return;
    }

    distance_ *= std::exp(-wheelOffset * zoomExponentPerStep);
    distance_ = std::clamp(distance_, minimumDistance(), maximumDistance());
}

glm::mat4 OrbitCamera::viewMatrix() const
{
    return glm::lookAt(position(), target_, glm::vec3(0.0F, 1.0F, 0.0F));
}

glm::mat4 OrbitCamera::projectionMatrix(float aspectRatio) const
{
    if (!isPositiveFinite(aspectRatio)) {
        throw std::invalid_argument("Camera aspect ratio must be positive and finite.");
    }

    const float nearPlane = std::max(0.01F, distance_ - sceneRadius_ * 1.5F);
    const float farPlane = std::max(nearPlane + 0.1F, distance_ + sceneRadius_ * 2.5F);
    return glm::perspective(
        verticalFieldOfViewRadians_,
        aspectRatio,
        nearPlane,
        farPlane);
}

glm::vec3 OrbitCamera::position() const noexcept
{
    const float horizontalDistance = distance_ * std::cos(pitchRadians_);
    return target_ + glm::vec3(
        horizontalDistance * std::sin(yawRadians_),
        distance_ * std::sin(pitchRadians_),
        horizontalDistance * std::cos(yawRadians_));
}

const glm::vec3& OrbitCamera::target() const noexcept
{
    return target_;
}

float OrbitCamera::distance() const noexcept
{
    return distance_;
}

float OrbitCamera::yawRadians() const noexcept
{
    return yawRadians_;
}

float OrbitCamera::pitchRadians() const noexcept
{
    return pitchRadians_;
}

float OrbitCamera::minimumDistance() const noexcept
{
    return std::max(0.01F, sceneRadius_ * 0.15F);
}

float OrbitCamera::maximumDistance() const noexcept
{
    return std::max(10.0F, sceneRadius_ * 50.0F);
}

} // namespace dentalviz
