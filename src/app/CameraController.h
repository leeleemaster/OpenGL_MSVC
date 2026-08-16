#pragma once

#include "core/ClickGesture.h"
#include "core/MeshData.h"
#include "core/OrbitCamera.h"

#include <optional>

struct GLFWwindow;

namespace dentalviz {

struct CameraInteractionStats {
    unsigned int orbitUpdates = 0;
    unsigned int panUpdates = 0;
    unsigned int zoomEvents = 0;
    unsigned int fitRequests = 0;
    unsigned int selectionRequests = 0;
};

struct PickRequest {
    double windowX = 0.0;
    double windowY = 0.0;
};

class CameraController final {
public:
    CameraController(
        GLFWwindow* window,
        OrbitCamera& camera,
        const AxisAlignedBounds& modelBounds);
    ~CameraController();

    CameraController(const CameraController&) = delete;
    CameraController& operator=(const CameraController&) = delete;
    CameraController(CameraController&&) = delete;
    CameraController& operator=(CameraController&&) = delete;

    void update(float aspectRatio, bool allowMouseInput = true, bool allowKeyboardInput = true);
    void setModelBounds(const AxisAlignedBounds& modelBounds) noexcept;
    [[nodiscard]] std::optional<PickRequest> consumePickRequest() noexcept;
    [[nodiscard]] const CameraInteractionStats& stats() const noexcept;

private:
    static void scrollCallback(GLFWwindow* window, double horizontalOffset, double verticalOffset);

    GLFWwindow* window_ = nullptr;
    OrbitCamera& camera_;
    AxisAlignedBounds modelBounds_{};
    CameraInteractionStats stats_{};
    double lastCursorX_ = 0.0;
    double lastCursorY_ = 0.0;
    double pendingScrollOffset_ = 0.0;
    std::optional<PickRequest> pendingPickRequest_;
    ClickGesture leftClickGesture_{};
    bool cursorInitialized_ = false;
    bool leftWasPressed_ = false;
    bool middleWasPressed_ = false;
    bool fitWasPressed_ = false;
};

} // namespace dentalviz
