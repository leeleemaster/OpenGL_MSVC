#include "core/ClickGesture.h"

#include <cmath>
#include <stdexcept>

namespace dentalviz {

ClickGesture::ClickGesture(double dragThresholdPixels)
    : dragThresholdSquared_(dragThresholdPixels * dragThresholdPixels)
{
    if (!std::isfinite(dragThresholdPixels) || dragThresholdPixels < 0.0) {
        throw std::invalid_argument("Click drag threshold must be finite and non-negative.");
    }
}

std::optional<PointerClick> ClickGesture::update(
    bool pressed,
    bool inputAllowed,
    double x,
    double y)
{
    if (!std::isfinite(x) || !std::isfinite(y)) {
        startedInAllowedRegion_ = false;
        clickCandidate_ = false;
        wasPressed_ = pressed;
        return std::nullopt;
    }

    if (pressed && !wasPressed_) {
        pressX_ = x;
        pressY_ = y;
        startedInAllowedRegion_ = inputAllowed;
        clickCandidate_ = inputAllowed;
    } else if (pressed && wasPressed_ && startedInAllowedRegion_) {
        const double horizontalDistance = x - pressX_;
        const double verticalDistance = y - pressY_;
        if (horizontalDistance * horizontalDistance + verticalDistance * verticalDistance >
            dragThresholdSquared_) {
            clickCandidate_ = false;
        }
    }

    std::optional<PointerClick> click;
    if (!pressed && wasPressed_) {
        if (startedInAllowedRegion_ && clickCandidate_ && inputAllowed) {
            click = PointerClick{x, y};
        }
        startedInAllowedRegion_ = false;
        clickCandidate_ = false;
    }

    wasPressed_ = pressed;
    return click;
}

bool ClickGesture::isDragging() const noexcept
{
    return wasPressed_ && startedInAllowedRegion_ && !clickCandidate_;
}

} // namespace dentalviz
