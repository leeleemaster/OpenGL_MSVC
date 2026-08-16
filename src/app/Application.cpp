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

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

constexpr int initialWindowWidth = 1280;
constexpr int initialWindowHeight = 720;

constexpr std::string_view toothVertexShader = R"glsl(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 worldPosition;
out vec3 worldNormal;

void main()
{
    vec4 position = uModel * vec4(aPosition, 1.0);
    worldPosition = position.xyz;
    worldNormal = mat3(transpose(inverse(uModel))) * aNormal;
    gl_Position = uProjection * uView * position;
}
)glsl";

constexpr std::string_view toothFragmentShader = R"glsl(
#version 330 core

in vec3 worldPosition;
in vec3 worldNormal;

uniform vec3 uBaseColor;
uniform vec3 uLightPosition;
uniform vec3 uCameraPosition;

out vec4 fragmentColor;

void main()
{
    vec3 normal = normalize(worldNormal);
    vec3 lightDirection = normalize(uLightPosition - worldPosition);
    vec3 viewDirection = normalize(uCameraPosition - worldPosition);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);

    float diffuse = max(dot(normal, lightDirection), 0.0);
    float specular = pow(max(dot(normal, halfwayDirection), 0.0), 72.0);
    float rim = pow(1.0 - max(dot(normal, viewDirection), 0.0), 2.0);

    vec3 color = uBaseColor * (0.20 + 0.80 * diffuse);
    color += vec3(0.42) * specular;
    color += vec3(0.08, 0.16, 0.18) * rim;
    fragmentColor = vec4(color, 1.0);
}
)glsl";

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
    const ShaderProgram toothShader(toothVertexShader, toothFragmentShader);

    std::cout << "Test mesh: " << toothData.vertices.size() << " vertices, "
              << toothData.indices.size() / 3 << " triangles\n"
              << "Bounds size: " << toothBounds.size().x << ", "
              << toothBounds.size().y << ", " << toothBounds.size().z << '\n';

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
    std::cout << "Controls: Left drag orbit | Middle drag pan | Wheel zoom | F fit | Esc close\n";

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
        toothMesh.draw();

        glfwSwapBuffers(window_);
        glfwPollEvents();
        ++renderedFrames;

        const double titleInterval = now - titleIntervalStart;
        if (titleInterval >= 0.5) {
            const double framesPerSecond = static_cast<double>(renderedFrames) / titleInterval;
            std::ostringstream title;
            title << projectName() << " | OpenGL 3.3 Core | " << std::fixed
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
    std::cout << "Application loop exited cleanly.\n";
    return 0;
}

} // namespace dentalviz
