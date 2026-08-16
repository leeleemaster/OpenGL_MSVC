#include "app/Application.h"
#include "app/CameraController.h"

#include "core/BuildInfo.h"
#include "core/MeshData.h"
#include "core/OrbitCamera.h"
#include "renderer/GpuMesh.h"
#include "renderer/ShaderProgram.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

constexpr int initialWindowWidth = 1280;
constexpr int initialWindowHeight = 720;

enum class RenderMode : int {
    solid = 0,
    wireframe = 1,
    normals = 2,
};

const char* renderModeName(RenderMode mode) noexcept
{
    switch (mode) {
    case RenderMode::solid:
        return "Solid";
    case RenderMode::wireframe:
        return "Wireframe";
    case RenderMode::normals:
        return "Normals";
    }
    return "Unknown";
}

std::filesystem::path findShaderDirectory()
{
    const std::filesystem::path sourceDirectory =
        std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    const std::array candidates{
        std::filesystem::current_path() / "assets" / "shaders",
        sourceDirectory / "assets" / "shaders",
    };

    for (const std::filesystem::path& candidate : candidates) {
        if (std::filesystem::is_regular_file(candidate / "mesh.vert") &&
            std::filesystem::is_regular_file(candidate / "mesh.frag")) {
            return candidate;
        }
    }

    throw std::runtime_error(
        "Shader assets were not found. Expected assets/shaders/mesh.vert and mesh.frag.");
}

bool keyPressedOnce(GLFWwindow* window, int key, bool& wasPressed) noexcept
{
    const bool isPressed = glfwGetKey(window, key) == GLFW_PRESS;
    const bool pressedOnce = isPressed && !wasPressed;
    wasPressed = isPressed;
    return pressedOnce;
}

void reportGlfwError(int errorCode, const char* description)
{
    std::cerr << "GLFW error " << errorCode << ": "
              << (description != nullptr ? description : "Unknown error") << '\n';
}

void resizeFramebuffer(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

const char* glString(GLenum name)
{
    const auto* value = glGetString(name);
    return value != nullptr ? reinterpret_cast<const char*>(value) : "Unavailable";
}

} // namespace

namespace dentalviz {

Application::Application()
{
    glfwSetErrorCallback(reportGlfwError);

    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("Failed to initialize GLFW.");
    }
    glfwInitialized_ = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

    window_ = glfwCreateWindow(
        initialWindowWidth,
        initialWindowHeight,
        "DentalViz",
        nullptr,
        nullptr);
    if (window_ == nullptr) {
        glfwTerminate();
        glfwInitialized_ = false;
        throw std::runtime_error("Failed to create an OpenGL 3.3 Core window.");
    }

    glfwMakeContextCurrent(window_);
    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
        glfwInitialized_ = false;
        throw std::runtime_error("Failed to load OpenGL functions with GLAD.");
    }

    glfwSetFramebufferSizeCallback(window_, resizeFramebuffer);
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window_, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    glfwSwapInterval(1);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    GLint multisampleCount = 0;
    glGetIntegerv(GL_SAMPLES, &multisampleCount);

    std::cout << projectName() << ' ' << projectVersion() << '\n'
              << "OpenGL vendor: " << glString(GL_VENDOR) << '\n'
              << "OpenGL renderer: " << glString(GL_RENDERER) << '\n'
              << "OpenGL version: " << glString(GL_VERSION) << '\n'
              << "GLSL version: " << glString(GL_SHADING_LANGUAGE_VERSION) << '\n'
              << "MSAA samples: " << multisampleCount << '\n';
}

Application::~Application()
{
    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
    }
    if (glfwInitialized_) {
        glfwTerminate();
    }
}

int Application::run(std::optional<double> maximumRuntimeSeconds)
{
    const MeshData toothData = makeProceduralTooth();
    const AxisAlignedBounds toothBounds = toothData.bounds();
    const GpuMesh toothMesh(toothData);
    const std::filesystem::path shaderDirectory = findShaderDirectory();
    const ShaderProgram toothShader = ShaderProgram::fromFiles(
        shaderDirectory / "mesh.vert",
        shaderDirectory / "mesh.frag");

    std::cout << "Test mesh: " << toothData.vertices.size() << " vertices, "
              << toothData.indices.size() / 3 << " triangles\n"
              << "Bounds size: " << toothBounds.size().x << ", "
              << toothBounds.size().y << ", " << toothBounds.size().z << '\n'
              << "Shaders: " << shaderDirectory.string() << '\n';

    int initialFramebufferWidth = 0;
    int initialFramebufferHeight = 0;
    glfwGetFramebufferSize(window_, &initialFramebufferWidth, &initialFramebufferHeight);
    const float initialAspectRatio = initialFramebufferHeight > 0
        ? static_cast<float>(initialFramebufferWidth) /
              static_cast<float>(initialFramebufferHeight)
        : 16.0F / 9.0F;

    OrbitCamera camera;
    camera.fit(toothBounds, initialAspectRatio);
    CameraController cameraController(window_, camera, toothBounds);
    std::cout << "Controls: Left drag orbit | Middle drag pan | Wheel zoom | F fit\n"
              << "Render modes: 1 Solid | 2 Wireframe | 3 Normals | Esc close\n";

    RenderMode renderMode = RenderMode::solid;
    bool solidWasPressed = false;
    bool wireframeWasPressed = false;
    bool normalsWasPressed = false;
    unsigned int renderModeChanges = 0;

    const double startTime = glfwGetTime();
    double titleIntervalStart = startTime;
    unsigned int renderedFrames = 0;

    while (glfwWindowShouldClose(window_) == GLFW_FALSE) {
        const double now = glfwGetTime();
        if (maximumRuntimeSeconds.has_value() &&
            now - startTime >= maximumRuntimeSeconds.value()) {
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
            continue;
        }

        if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }

        RenderMode requestedMode = renderMode;
        if (keyPressedOnce(window_, GLFW_KEY_1, solidWasPressed)) {
            requestedMode = RenderMode::solid;
        }
        if (keyPressedOnce(window_, GLFW_KEY_2, wireframeWasPressed)) {
            requestedMode = RenderMode::wireframe;
        }
        if (keyPressedOnce(window_, GLFW_KEY_3, normalsWasPressed)) {
            requestedMode = RenderMode::normals;
        }
        if (requestedMode != renderMode) {
            renderMode = requestedMode;
            ++renderModeChanges;
            std::cout << "Render mode: " << renderModeName(renderMode) << '\n';
        }

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window_, &framebufferWidth, &framebufferHeight);
        if (framebufferWidth == 0 || framebufferHeight == 0) {
            glfwPollEvents();
            continue;
        }

        const float aspectRatio = static_cast<float>(framebufferWidth) /
                                  static_cast<float>(framebufferHeight);
        cameraController.update(aspectRatio);
        const glm::mat4 projection = camera.projectionMatrix(aspectRatio);
        const glm::mat4 view = camera.viewMatrix();
        const glm::mat4 model(1.0F);
        const glm::vec3 cameraPosition = camera.position();

        glClearColor(0.035F, 0.075F, 0.095F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        toothShader.use();
        toothShader.setMatrix4("uModel", model);
        toothShader.setMatrix4("uView", view);
        toothShader.setMatrix4("uProjection", projection);
        toothShader.setVector3("uBaseColor", glm::vec3(0.86F, 0.76F, 0.56F));
        toothShader.setVector3("uLightPosition", glm::vec3(3.2F, 4.0F, 4.5F));
        toothShader.setVector3("uCameraPosition", cameraPosition);
        toothShader.setFloat("uShininess", 72.0F);
        toothShader.setInteger("uRenderMode", static_cast<int>(renderMode));

        if (renderMode == RenderMode::wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        toothMesh.draw();
        if (renderMode == RenderMode::wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glfwSwapBuffers(window_);
        glfwPollEvents();
        ++renderedFrames;

        const double titleInterval = now - titleIntervalStart;
        if (titleInterval >= 0.5) {
            const double framesPerSecond = static_cast<double>(renderedFrames) / titleInterval;
            std::ostringstream title;
            title << projectName() << " | " << renderModeName(renderMode)
                  << " | OpenGL 3.3 Core | " << std::fixed
                  << std::setprecision(1) << framesPerSecond << " FPS";
            glfwSetWindowTitle(window_, title.str().c_str());
            titleIntervalStart = now;
            renderedFrames = 0;
        }
    }

    const GLenum renderingError = glGetError();
    if (renderingError != GL_NO_ERROR) {
        throw std::runtime_error(
            "OpenGL reported an error during rendering: " + std::to_string(renderingError));
    }

    const CameraInteractionStats& interactions = cameraController.stats();
    std::cout << "Camera input: " << interactions.orbitUpdates << " orbit, "
              << interactions.panUpdates << " pan, " << interactions.zoomEvents
              << " zoom, " << interactions.fitRequests << " fit updates\n";
    std::cout << "Render mode changes: " << renderModeChanges << '\n';
    std::cout << "Application loop exited cleanly.\n";
    return 0;
}

} // namespace dentalviz
