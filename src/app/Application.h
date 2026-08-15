#pragma once

#include <optional>

struct GLFWwindow;

namespace dentalviz {

class Application final {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    int run(std::optional<double> maximumRuntimeSeconds = std::nullopt);

private:
    GLFWwindow* window_ = nullptr;
    bool glfwInitialized_ = false;
};

} // namespace dentalviz
