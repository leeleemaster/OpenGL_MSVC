#pragma once

#include <optional>

namespace dentalviz {

struct PointerClick {
    double x = 0.0;
    double y = 0.0;
};

class ClickGesture final {
public:
    explicit ClickGesture(double dragThresholdPixels = 4.0);

    [[nodiscard]] std::optional<PointerClick> update(
        bool pressed,
        bool inputAllowed,
        double x,
        double y);
    [[nodiscard]] bool isDragging() const noexcept;

private:
    double dragThresholdSquared_ = 16.0;
    double pressX_ = 0.0;
    double pressY_ = 0.0;
    bool wasPressed_ = false;
    bool startedInAllowedRegion_ = false;
    bool clickCandidate_ = false;
};

} // namespace dentalviz
