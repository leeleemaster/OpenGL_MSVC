#include "app/Application.h"

#include "core/BuildInfo.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

constexpr int initialWindowWidth = 1280;
constexpr int initialWindowHeight = 720;

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

    std::cout << projectName() << ' ' << projectVersion() << '\n'
              << "OpenGL vendor: " << glString(GL_VENDOR) << '\n'
              << "OpenGL renderer: " << glString(GL_RENDERER) << '\n'
              << "OpenGL version: " << glString(GL_VERSION) << '\n'
              << "GLSL version: " << glString(GL_SHADING_LANGUAGE_VERSION) << '\n';
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

        glClearColor(0.035F, 0.075F, 0.095F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

    std::cout << "Application loop exited cleanly.\n";
    return 0;
}

} // namespace dentalviz
