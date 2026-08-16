#include "ui/ViewerUi.h"

#include "platform/NativeFileDialog.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <stdexcept>

namespace {

constexpr float minimumViewerWidth = 180.0F;

std::string pathToUtf8(const std::filesystem::path& path)
{
    const std::u8string value = path.u8string();
    return std::string(reinterpret_cast<const char*>(value.data()), value.size());
}

void configureStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark(&style);
    style.WindowRounding = 0.0F;
    style.ChildRounding = 7.0F;
    style.FrameRounding = 5.0F;
    style.GrabRounding = 5.0F;
    style.WindowPadding = ImVec2(18.0F, 15.0F);
    style.FramePadding = ImVec2(9.0F, 5.0F);
    style.ItemSpacing = ImVec2(8.0F, 7.0F);
    style.WindowBorderSize = 0.0F;

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.055F, 0.075F, 0.090F, 1.0F);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.075F, 0.100F, 0.115F, 1.0F);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.105F, 0.135F, 0.150F, 1.0F);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.135F, 0.190F, 0.205F, 1.0F);
    style.Colors[ImGuiCol_Button] = ImVec4(0.070F, 0.400F, 0.460F, 1.0F);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.090F, 0.510F, 0.570F, 1.0F);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.060F, 0.340F, 0.400F, 1.0F);
    style.Colors[ImGuiCol_Header] = ImVec4(0.070F, 0.400F, 0.460F, 1.0F);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.090F, 0.510F, 0.570F, 1.0F);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.300F, 0.820F, 0.850F, 1.0F);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.300F, 0.820F, 0.850F, 1.0F);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.160F, 0.230F, 0.250F, 1.0F);
}

} // namespace

namespace dentalviz {

const char* renderModeName(RenderMode mode) noexcept
{
    switch (mode) {
    case RenderMode::solid:
        return "Solid";
    case RenderMode::wireframe:
        return "Wireframe";
    case RenderMode::normals:
        return "Normal Color";
    }
    return "Unknown";
}

float ViewerRect::aspectRatio() const noexcept
{
    return framebufferHeight > 0
        ? static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight)
        : 1.0F;
}

ViewerUi::ViewerUi(GLFWwindow* window)
    : window_(window)
{
    if (window == nullptr) {
        throw std::invalid_argument("ViewerUi requires a valid GLFW window.");
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImFontConfig fontConfiguration;
    fontConfiguration.SizePixels = 16.0F;
    ImFont* userInterfaceFont = nullptr;
    char* windowsDirectoryValue = nullptr;
    std::size_t windowsDirectoryLength = 0;
    const errno_t environmentResult =
        _dupenv_s(&windowsDirectoryValue, &windowsDirectoryLength, "WINDIR");
    std::unique_ptr<char, decltype(&std::free)> windowsDirectory(
        windowsDirectoryValue,
        &std::free);
    if (environmentResult == 0 && windowsDirectory != nullptr) {
        const std::filesystem::path fontPath =
            std::filesystem::path(windowsDirectory.get()) / "Fonts" / "malgun.ttf";
        if (std::filesystem::is_regular_file(fontPath)) {
            userInterfaceFont = io.Fonts->AddFontFromFileTTF(
                fontPath.string().c_str(),
                fontConfiguration.SizePixels,
                &fontConfiguration,
                io.Fonts->GetGlyphRangesKorean());
        }
    }
    if (userInterfaceFont == nullptr) {
        io.Fonts->AddFontDefault(&fontConfiguration);
    }
    configureStyle();

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        ImGui::DestroyContext();
        throw std::runtime_error("Dear ImGui GLFW backend initialization failed.");
    }
    glfwBackendInitialized_ = true;

    if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
        ImGui_ImplGlfw_Shutdown();
        glfwBackendInitialized_ = false;
        ImGui::DestroyContext();
        throw std::runtime_error("Dear ImGui OpenGL3 backend initialization failed.");
    }
    openGlBackendInitialized_ = true;
}

ViewerUi::~ViewerUi()
{
    if (openGlBackendInitialized_) {
        ImGui_ImplOpenGL3_Shutdown();
    }
    if (glfwBackendInitialized_) {
        ImGui_ImplGlfw_Shutdown();
    }
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGui::DestroyContext();
    }
}

void ViewerUi::beginFrame(
    int windowWidth,
    int windowHeight,
    int framebufferWidth,
    int framebufferHeight)
{
    if (windowWidth <= 0 || windowHeight <= 0 ||
        framebufferWidth <= 0 || framebufferHeight <= 0) {
        throw std::invalid_argument("ViewerUi frame dimensions must be positive.");
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const float sidebarWidth = std::min(
        propertiesPanelWidth,
        std::max(0.0F, static_cast<float>(windowWidth) - minimumViewerWidth));
    const float framebufferScaleX =
        static_cast<float>(framebufferWidth) / static_cast<float>(windowWidth);
    const int sidebarFramebufferWidth = std::clamp(
        static_cast<int>(std::lround(sidebarWidth * framebufferScaleX)),
        0,
        framebufferWidth - 1);

    viewerRect_.windowX = sidebarWidth;
    viewerRect_.windowY = 0.0F;
    viewerRect_.windowWidth = static_cast<float>(windowWidth) - sidebarWidth;
    viewerRect_.windowHeight = static_cast<float>(windowHeight);
    viewerRect_.framebufferX = sidebarFramebufferWidth;
    viewerRect_.framebufferY = 0;
    viewerRect_.framebufferWidth = framebufferWidth - sidebarFramebufferWidth;
    viewerRect_.framebufferHeight = framebufferHeight;
}

ViewerUiActions ViewerUi::draw(ViewerUiState& state)
{
    ViewerUiActions actions;
    ImGui::SetNextWindowPos(ImVec2(0.0F, 0.0F));
    ImGui::SetNextWindowSize(ImVec2(viewerRect_.windowX, viewerRect_.windowHeight));
    constexpr ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("DentalViz Properties", nullptr, windowFlags)) {
        ImGui::TextColored(ImVec4(0.30F, 0.82F, 0.85F, 1.0F), "DENTALVIZ");
        ImGui::TextDisabled("OpenGL 3.3 Dental Mesh Viewer");

        ImGui::SeparatorText("Model Information");
        ImGui::TextWrapped("%s", state.model.name.c_str());
        ImGui::Text("Vertices    %zu", state.model.vertexCount);
        ImGui::Text("Triangles   %zu", state.model.triangleCount);
        const glm::vec3 boundsSize = state.model.bounds.size();
        ImGui::Text(
            "Bounds      %.2f x %.2f x %.2f",
            static_cast<double>(boundsSize.x),
            static_cast<double>(boundsSize.y),
            static_cast<double>(boundsSize.z));
        ImGui::Text("Unit        1 model unit = 1 mm");
        if (state.model.loadedFromFile) {
            ImGui::Text("Source meshes  %zu", state.model.sourceMeshCount);
            ImGui::Text("Load time      %.3f ms", state.model.loadMilliseconds);
            ImGui::TextDisabled("Source");
            const std::string sourcePath = pathToUtf8(state.model.sourcePath);
            ImGui::TextWrapped("%s", sourcePath.c_str());
        } else {
            ImGui::TextDisabled("Project-authored test geometry");
        }

        if (ImGui::Button("Load Model...", ImVec2(-1.0F, 0.0F))) {
            try {
                actions.modelToLoad = chooseMeshFile();
                if (!actions.modelToLoad.has_value()) {
                    state.statusMessage = "Model selection cancelled.";
                    state.statusIsError = false;
                }
            } catch (const std::exception& error) {
                state.statusMessage = error.what();
                state.statusIsError = true;
            }
        }

        ImGui::SeparatorText("Rendering");
        if (ImGui::BeginTable("Rendering controls", 2, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 105.0F);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Mode");
            ImGui::TableSetColumnIndex(1);
            int selectedMode = static_cast<int>(state.renderMode);
            constexpr const char* renderModes[] = {"Solid", "Wireframe", "Normal Color"};
            ImGui::SetNextItemWidth(-1.0F);
            if (ImGui::Combo("##Render mode", &selectedMode, renderModes, 3)) {
                state.renderMode = static_cast<RenderMode>(selectedMode);
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Base color");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-1.0F);
            ImGui::ColorEdit3(
                "##Base color",
                &state.baseColor.x,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Light position");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-1.0F);
            ImGui::DragFloat3(
                "##Light position", &state.lightPosition.x, 0.05F, -20.0F, 20.0F, "%.2f");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Shininess");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-1.0F);
            ImGui::SliderFloat(
                "##Shininess",
                &state.shininess,
                8.0F,
                256.0F,
                "%.0f",
                ImGuiSliderFlags_Logarithmic);
            ImGui::EndTable();
        }
        actions.resetCamera = ImGui::Button("Reset Camera", ImVec2(-1.0F, 0.0F));

        ImGui::SeparatorText("Status");
        ImGui::Text("%.1f FPS", static_cast<double>(state.framesPerSecond));
        ImGui::TextDisabled(
            "Viewport %d x %d px",
            viewerRect_.framebufferWidth,
            viewerRect_.framebufferHeight);
        const ImVec4 statusColor = state.statusIsError
            ? ImVec4(1.0F, 0.38F, 0.32F, 1.0F)
            : ImVec4(0.48F, 0.84F, 0.68F, 1.0F);
        ImGui::PushStyleColor(ImGuiCol_Text, statusColor);
        ImGui::TextWrapped("%s", state.statusMessage.c_str());
        ImGui::PopStyleColor();

        ImGui::SeparatorText("Controls");
        ImGui::TextDisabled("LMB Orbit  |  MMB Pan  |  Wheel Zoom");
        ImGui::TextDisabled("F Fit  |  1/2/3 Render Mode");
    }
    ImGui::End();
    return actions;
}

void ViewerUi::render()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool ViewerUi::wantsCaptureMouse() const noexcept
{
    return ImGui::GetIO().WantCaptureMouse;
}

bool ViewerUi::wantsCaptureKeyboard() const noexcept
{
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool ViewerUi::isMouseOverViewer() const noexcept
{
    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window_, &mouseX, &mouseY);
    return viewerRect_.containsWindowPoint(
        static_cast<float>(mouseX),
        static_cast<float>(mouseY));
}

const ViewerRect& ViewerUi::viewerRect() const noexcept
{
    return viewerRect_;
}

} // namespace dentalviz
