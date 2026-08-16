#pragma once

#include <filesystem>
#include <optional>

struct GLFWwindow;

namespace dentalviz {

struct ApplicationRunOptions {
    std::optional<double> maximumRuntimeSeconds;
    std::optional<std::filesystem::path> modelPath;
};

class Application final {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    int run(const ApplicationRunOptions& options = {});

private:
    GLFWwindow* window_ = nullptr;
    bool glfwInitialized_ = false;
};

} // namespace dentalviz
