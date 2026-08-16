#pragma once

#include "core/ClippingPlane.h"
#include "core/MeshData.h"
#include "core/PointMeasurement.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

struct GLFWwindow;

namespace dentalviz {

enum class RenderMode : int {
    solid = 0,
    wireframe = 1,
    normals = 2,
};

[[nodiscard]] const char* renderModeName(RenderMode mode) noexcept;

struct ViewerModelInfo {
    std::string name;
    std::filesystem::path sourcePath;
    std::size_t vertexCount = 0;
    std::size_t triangleCount = 0;
    std::size_t sourceMeshCount = 0;
    AxisAlignedBounds bounds{};
    double loadMilliseconds = 0.0;
    bool loadedFromFile = false;
};

struct ViewerUiState {
    ViewerModelInfo model;
    RenderMode renderMode = RenderMode::solid;
    glm::vec3 baseColor{0.86F, 0.76F, 0.56F};
    glm::vec3 lightPosition{3.2F, 4.0F, 4.5F};
    float shininess = 72.0F;
    float framesPerSecond = 0.0F;
    ClippingPlane clippingPlane;
    PointMeasurement measurement;
    std::string statusMessage = "Viewer ready.";
    bool statusIsError = false;
};

struct ViewerRect {
    int framebufferX = 0;
    int framebufferY = 0;
    int framebufferWidth = 1;
    int framebufferHeight = 1;
    float windowX = 0.0F;
    float windowY = 0.0F;
    float windowWidth = 1.0F;
    float windowHeight = 1.0F;

    [[nodiscard]] float aspectRatio() const noexcept;
    [[nodiscard]] bool containsWindowPoint(float x, float y) const noexcept
    {
        return x >= windowX && y >= windowY &&
               x < windowX + windowWidth && y < windowY + windowHeight;
    }
};

struct ViewerUiActions {
    std::optional<std::filesystem::path> modelToLoad;
    bool resetCamera = false;
    bool resetClippingPlane = false;
    bool resetMeasurement = false;
};

class ViewerUi final {
public:
    static constexpr float propertiesPanelWidth = 380.0F;

    explicit ViewerUi(GLFWwindow* window);
    ~ViewerUi();

    ViewerUi(const ViewerUi&) = delete;
    ViewerUi& operator=(const ViewerUi&) = delete;
    ViewerUi(ViewerUi&&) = delete;
    ViewerUi& operator=(ViewerUi&&) = delete;

    void beginFrame(
        int windowWidth,
        int windowHeight,
        int framebufferWidth,
        int framebufferHeight);
    [[nodiscard]] ViewerUiActions draw(ViewerUiState& state);
    void drawMeasurementLabel(
        const glm::vec3& worldPosition,
        const glm::mat4& view,
        const glm::mat4& projection,
        std::string_view label) const;
    void render();

    [[nodiscard]] bool wantsCaptureMouse() const noexcept;
    [[nodiscard]] bool wantsCaptureKeyboard() const noexcept;
    [[nodiscard]] bool isMouseOverViewer() const noexcept;
    [[nodiscard]] const ViewerRect& viewerRect() const noexcept;

private:
    GLFWwindow* window_ = nullptr;
    ViewerRect viewerRect_{};
    bool glfwBackendInitialized_ = false;
    bool openGlBackendInitialized_ = false;
};

} // namespace dentalviz
