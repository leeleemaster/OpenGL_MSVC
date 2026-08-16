#include "app/CameraController.h"

#include <GLFW/glfw3.h>

#include <cmath>
#include <stdexcept>

namespace dentalviz {

CameraController::CameraController(
    GLFWwindow* window,
    OrbitCamera& camera,
    const AxisAlignedBounds& modelBounds)
    : window_(window),
      camera_(camera),
      modelBounds_(modelBounds)
{
    if (window_ == nullptr) {
        throw std::invalid_argument("CameraController requires a valid GLFW window.");
    }
    if (glfwGetWindowUserPointer(window_) != nullptr) {
        throw std::runtime_error("GLFW window user pointer is already in use.");
    }

    glfwSetWindowUserPointer(window_, this);
    glfwSetScrollCallback(window_, scrollCallback);
}

CameraController::~CameraController()
{
    if (window_ != nullptr && glfwGetWindowUserPointer(window_) == this) {
        glfwSetScrollCallback(window_, nullptr);
        glfwSetWindowUserPointer(window_, nullptr);
    }
}

void CameraController::update(
    float aspectRatio,
    bool allowMouseInput,
    bool allowKeyboardInput)
{
    double cursorX = 0.0;
    double cursorY = 0.0;
    glfwGetCursorPos(window_, &cursorX, &cursorY);

    const bool leftPressed = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool middlePressed = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

    if (const std::optional<PointerClick> click = leftClickGesture_.update(
            leftPressed, allowMouseInput, cursorX, cursorY);
        click.has_value()) {
        pendingPickRequest_ = PickRequest{click->x, click->y};
        ++stats_.selectionRequests;
    }

    if (cursorInitialized_) {
        const float horizontalDelta = static_cast<float>(cursorX - lastCursorX_);
        const float verticalDelta = static_cast<float>(cursorY - lastCursorY_);
        const bool cursorMoved = horizontalDelta != 0.0F || verticalDelta != 0.0F;

        if (leftPressed && leftWasPressed_ && leftClickGesture_.isDragging()) {
            if (allowMouseInput && cursorMoved) {
                camera_.orbit(horizontalDelta, verticalDelta);
                ++stats_.orbitUpdates;
            }
        } else if (allowMouseInput && middlePressed && middleWasPressed_ && cursorMoved) {
            int windowWidth = 0;
            int windowHeight = 0;
            glfwGetWindowSize(window_, &windowWidth, &windowHeight);
            static_cast<void>(windowWidth);
            camera_.pan(
                horizontalDelta,
                verticalDelta,
                static_cast<float>(windowHeight));
            ++stats_.panUpdates;
        }
    }

    if (allowMouseInput && pendingScrollOffset_ != 0.0) {
        camera_.zoom(static_cast<float>(pendingScrollOffset_));
        ++stats_.zoomEvents;
    }
    pendingScrollOffset_ = 0.0;

    const bool fitPressed = glfwGetKey(window_, GLFW_KEY_F) == GLFW_PRESS;
    if (allowKeyboardInput && fitPressed && !fitWasPressed_) {
        camera_.fit(modelBounds_, aspectRatio);
        ++stats_.fitRequests;
    }

    lastCursorX_ = cursorX;
    lastCursorY_ = cursorY;
    cursorInitialized_ = true;
    leftWasPressed_ = leftPressed;
    middleWasPressed_ = allowMouseInput && middlePressed;
    fitWasPressed_ = allowKeyboardInput && fitPressed;
}

void CameraController::setModelBounds(const AxisAlignedBounds& modelBounds) noexcept
{
    modelBounds_ = modelBounds;
}

std::optional<PickRequest> CameraController::consumePickRequest() noexcept
{
    std::optional<PickRequest> request = pendingPickRequest_;
    pendingPickRequest_.reset();
    return request;
}

const CameraInteractionStats& CameraController::stats() const noexcept
{
    return stats_;
}

void CameraController::scrollCallback(
    GLFWwindow* window,
    double horizontalOffset,
    double verticalOffset)
{
    static_cast<void>(horizontalOffset);
    auto* controller = static_cast<CameraController*>(glfwGetWindowUserPointer(window));
    if (controller != nullptr && std::isfinite(verticalOffset)) {
        controller->pendingScrollOffset_ += verticalOffset;
    }
}

} // namespace dentalviz
