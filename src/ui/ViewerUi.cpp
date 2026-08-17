#include "ui/ViewerUi.h"

#include "core/PathText.h"
#include "platform/NativeFileDialog.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_stdlib.h>

#include <glm/vec4.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <stdexcept>

namespace {

constexpr float minimumViewerWidth = 180.0F;

constexpr std::string_view defaultMiniShaderSource =
    "material Dental {\n"
    "    let n = normalize(normal);\n"
    "    let l = normalize(lightDir);\n"
    "    let diffuse = max(dot(n, l), 0.0);\n"
    "    let intensity = 0.2 + diffuse;\n"
    "    output = baseColor * intensity;\n"
    "}";

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
    style.ItemSpacing = ImVec2(8.0F, 6.0F);
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

MiniShaderEditorState::MiniShaderEditorState()
    : source(defaultMiniShaderSource)
{
}

const char* renderModeName(RenderMode mode) noexcept
{
    switch (mode) {
    case RenderMode::solid:
        return "솔리드";
    case RenderMode::wireframe:
        return "와이어프레임";
    case RenderMode::normals:
        return "법선 색상";
    }
    return "알 수 없음";
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
        ImGui::SameLine();
        ImGui::TextDisabled(
            "OpenGL 3.3  |  %.1f FPS",
            static_cast<double>(state.framesPerSecond));
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "뷰어 프레임버퍼: %d x %d px",
                viewerRect_.framebufferWidth,
                viewerRect_.framebufferHeight);
        }

        ImGui::SeparatorText("모델 정보");
        ImGui::TextWrapped("%s", state.model.name.c_str());
        ImGui::Text(
            "정점 %zu  |  삼각형 %zu",
            state.model.vertexCount,
            state.model.triangleCount);
        const glm::vec3 boundsSize = state.model.bounds.size();
        ImGui::Text(
            "경계 크기   %.2f x %.2f x %.2f",
            static_cast<double>(boundsSize.x),
            static_cast<double>(boundsSize.y),
            static_cast<double>(boundsSize.z));
        ImGui::Text("단위        모델 단위 (원본 배율 미확인)");
        if (state.model.loadedFromFile) {
            ImGui::Text("원본 메시   %zu", state.model.sourceMeshCount);
            ImGui::Text("불러오기    %.3f ms", state.model.loadMilliseconds);
            ImGui::TextDisabled("원본 경로");
            const std::string sourcePath = pathToUtf8(state.model.sourcePath);
            ImGui::TextWrapped("%s", sourcePath.c_str());
        } else {
            ImGui::TextDisabled("프로젝트 제작 테스트 형상");
        }

        if (ImGui::Button("모델 불러오기...", ImVec2(-1.0F, 0.0F))) {
            try {
                actions.modelToLoad = chooseMeshFile();
                if (!actions.modelToLoad.has_value()) {
                    state.statusMessage = "모델 선택을 취소했습니다.";
                    state.statusIsError = false;
                }
            } catch (const std::exception&) {
                state.statusMessage =
                    "파일 선택 창을 열지 못했습니다. 자세한 내용은 콘솔을 확인하세요.";
                state.statusIsError = true;
            }
        }

        ImGui::SeparatorText("렌더링");
        if (ImGui::BeginTable("Rendering controls", 2, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("항목", ImGuiTableColumnFlags_WidthFixed, 105.0F);
            ImGui::TableSetupColumn("값", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("모드");
            ImGui::TableSetColumnIndex(1);
            int selectedMode = static_cast<int>(state.renderMode);
            constexpr const char* renderModes[] = {"솔리드", "와이어프레임", "법선 색상"};
            ImGui::SetNextItemWidth(-1.0F);
            if (ImGui::Combo("##Render mode", &selectedMode, renderModes, 3)) {
                state.renderMode = static_cast<RenderMode>(selectedMode);
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("기본 색상");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-1.0F);
            ImGui::ColorEdit3(
                "##Base color",
                &state.baseColor.x,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("광원 위치");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-1.0F);
            ImGui::DragFloat3(
                "##Light position", &state.lightPosition.x, 0.05F, -20.0F, 20.0F, "%.2f");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("광택");
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
        actions.resetCamera = ImGui::Button("카메라 맞춤", ImVec2(-1.0F, 0.0F));

        ImGui::SeparatorText("상태");
        const ImVec4 statusColor = state.statusIsError
            ? ImVec4(1.0F, 0.38F, 0.32F, 1.0F)
            : ImVec4(0.48F, 0.84F, 0.68F, 1.0F);
        ImGui::PushStyleColor(ImGuiCol_Text, statusColor);
        ImGui::TextWrapped("%s", state.statusMessage.c_str());
        ImGui::PopStyleColor();

        ImGui::SeparatorText("도구");
        if (ImGui::BeginTabBar("Viewer tools")) {
            if (ImGui::BeginTabItem("거리 측정")) {
                if (state.measurement.pointA().has_value()) {
                    const RayHit& pointA = state.measurement.pointA().value();
                    ImGui::Text(
                        "A  %.3f, %.3f, %.3f",
                        static_cast<double>(pointA.position.x),
                        static_cast<double>(pointA.position.y),
                        static_cast<double>(pointA.position.z));
                } else {
                    ImGui::TextDisabled("모델 표면에서 A점을 선택하세요.");
                }
                if (state.measurement.pointB().has_value()) {
                    const RayHit& pointB = state.measurement.pointB().value();
                    ImGui::Text(
                        "B  %.3f, %.3f, %.3f",
                        static_cast<double>(pointB.position.x),
                        static_cast<double>(pointB.position.y),
                        static_cast<double>(pointB.position.z));
                } else if (state.measurement.pointA().has_value()) {
                    ImGui::TextDisabled("모델 표면에서 B점을 선택하세요.");
                }
                if (const std::optional<float> distance = state.measurement.distance();
                    distance.has_value()) {
                    ImGui::TextColored(
                        ImVec4(0.98F, 0.80F, 0.24F, 1.0F),
                        "3D 직선거리  %.3f 모델 단위",
                        static_cast<double>(distance.value()));
                }
                ImGui::TextDisabled("원본 배율은 자동으로 추론하지 않습니다.");
                actions.resetMeasurement =
                    ImGui::Button("거리 측정 초기화", ImVec2(-1.0F, 0.0F));
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("클리핑 미리보기")) {
                bool clippingEnabled = state.clippingPlane.enabled();
                if (ImGui::Checkbox("클리핑 사용", &clippingEnabled)) {
                    state.clippingPlane.setEnabled(clippingEnabled);
                }

                if (ImGui::BeginTable(
                        "Clipping controls", 2, ImGuiTableFlags_SizingStretchProp)) {
                    ImGui::TableSetupColumn(
                        "항목", ImGuiTableColumnFlags_WidthFixed, 88.0F);
                    ImGui::TableSetupColumn(
                        "값", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("법선 축");
                    ImGui::TableSetColumnIndex(1);
                    int selectedAxis = static_cast<int>(state.clippingPlane.axis());
                    constexpr const char* clipAxes[] = {"+X", "+Y", "+Z"};
                    ImGui::SetNextItemWidth(-1.0F);
                    if (ImGui::Combo("##Clip normal axis", &selectedAxis, clipAxes, 3)) {
                        state.clippingPlane.setAxis(
                            static_cast<ClipAxis>(selectedAxis), state.model.bounds);
                    }

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("거리 d");
                    ImGui::TableSetColumnIndex(1);
                    const auto [minimumDistance, maximumDistance] =
                        state.clippingPlane.distanceRange(state.model.bounds);
                    float planeDistance = state.clippingPlane.distance();
                    ImGui::SetNextItemWidth(-1.0F);
                    if (ImGui::SliderFloat(
                            "##Clip distance",
                            &planeDistance,
                            minimumDistance,
                            maximumDistance,
                            "%.3f")) {
                        state.clippingPlane.setDistance(
                            planeDistance, state.model.bounds);
                    }
                    ImGui::EndTable();
                }

                ImGui::TextDisabled(
                    "모델 좌표: dot(p, %s) + d <= 0 영역 유지",
                    clipAxisName(state.clippingPlane.axis()));
                ImGui::TextWrapped("미리보기 전용: 절단면 메시나 덮개를 생성하지 않습니다.");
                actions.resetClippingPlane =
                    ImGui::Button("평면 초기화", ImVec2(-1.0F, 0.0F));
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("MiniShader")) {
                ImGui::TextWrapped(
                    "제한된 재질 언어를 편집하고 뷰어 재시작 없이 적용합니다.");
                ImGui::TextDisabled("버튼 기반 실행 중 컴파일 및 적용");
                if (ImGui::Button("컴파일 및 적용", ImVec2(195.0F, 0.0F))) {
                    actions.compileAndApplyMiniShader = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("소스 초기화", ImVec2(-1.0F, 0.0F))) {
                    state.miniShader.source = defaultMiniShaderSource;
                    state.miniShader.compilerOutput =
                        "편집기 소스를 초기화했습니다. 현재 셰이더는 변경되지 않았습니다.";
                    state.miniShader.outputIsError = false;
                }
                ImGui::InputTextMultiline(
                    "##MiniShader source",
                    &state.miniShader.source,
                    ImVec2(-1.0F, 92.0F),
                    ImGuiInputTextFlags_AllowTabInput);

                const ImVec4 compilerColor = state.miniShader.outputIsError
                    ? ImVec4(1.0F, 0.38F, 0.32F, 1.0F)
                    : ImVec4(0.48F, 0.84F, 0.68F, 1.0F);
                ImGui::PushStyleColor(ImGuiCol_Text, compilerColor);
                ImGui::TextWrapped("%s", state.miniShader.compilerOutput.c_str());
                ImGui::PopStyleColor();
                if (state.miniShader.hasActiveShader) {
                    ImGui::TextDisabled(
                        "적용된 MiniShader 개정 %u (마지막 정상 셰이더)",
                        state.miniShader.appliedRevision);
                } else {
                    ImGui::TextDisabled(
                        "적용된 셰이더: 내장 뷰어 셰이더 (마지막 정상 셰이더)");
                }

                if (ImGui::CollapsingHeader("생성된 GLSL 미리보기")) {
                    if (state.miniShader.generatedGlsl.empty()) {
                        ImGui::TextDisabled("아직 생성된 GLSL이 없습니다.");
                    } else {
                        ImGui::InputTextMultiline(
                            "##Generated GLSL",
                            &state.miniShader.generatedGlsl,
                            ImVec2(-1.0F, 170.0F),
                            ImGuiInputTextFlags_ReadOnly);
                    }
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::SeparatorText("조작법");
        ImGui::TextDisabled("왼쪽 클릭: A/B 선택 | 드래그: 회전 | 가운데 드래그: 이동");
        ImGui::TextDisabled("휠: 확대·축소 | F: 화면 맞춤 | 1/2/3: 모드");
    }
    ImGui::End();
    return actions;
}

void ViewerUi::drawMeasurementLabel(
    const glm::vec3& worldPosition,
    const glm::mat4& view,
    const glm::mat4& projection,
    std::string_view label) const
{
    const glm::vec4 clipPosition =
        projection * view * glm::vec4(worldPosition, 1.0F);
    if (!std::isfinite(clipPosition.x) || !std::isfinite(clipPosition.y) ||
        !std::isfinite(clipPosition.z) || !std::isfinite(clipPosition.w) ||
        clipPosition.w <= 0.0F) {
        return;
    }

    const glm::vec3 normalizedDevice = glm::vec3(clipPosition) / clipPosition.w;
    if (normalizedDevice.x < -1.0F || normalizedDevice.x > 1.0F ||
        normalizedDevice.y < -1.0F || normalizedDevice.y > 1.0F ||
        normalizedDevice.z < -1.0F || normalizedDevice.z > 1.0F) {
        return;
    }

    const ImVec2 center(
        viewerRect_.windowX + (normalizedDevice.x * 0.5F + 0.5F) * viewerRect_.windowWidth,
        viewerRect_.windowY + (0.5F - normalizedDevice.y * 0.5F) * viewerRect_.windowHeight);
    const char* labelBegin = label.data();
    const char* labelEnd = label.data() + label.size();
    const ImVec2 textSize = ImGui::CalcTextSize(labelBegin, labelEnd);
    const ImVec2 padding(8.0F, 5.0F);
    ImVec2 topLeft(
        center.x - textSize.x * 0.5F - padding.x,
        center.y - textSize.y * 0.5F - padding.y - 14.0F);
    const float labelWidth = textSize.x + padding.x * 2.0F;
    const float labelHeight = textSize.y + padding.y * 2.0F;
    constexpr float viewerMargin = 5.0F;
    const float minimumLabelX = viewerRect_.windowX + viewerMargin;
    const float maximumLabelX =
        viewerRect_.windowX + viewerRect_.windowWidth - labelWidth - viewerMargin;
    const float minimumLabelY = viewerRect_.windowY + viewerMargin;
    const float maximumLabelY =
        viewerRect_.windowY + viewerRect_.windowHeight - labelHeight - viewerMargin;
    if (maximumLabelX < minimumLabelX || maximumLabelY < minimumLabelY) {
        return;
    }
    topLeft.x = std::clamp(
        topLeft.x,
        minimumLabelX,
        maximumLabelX);
    topLeft.y = std::clamp(
        topLeft.y,
        minimumLabelY,
        maximumLabelY);
    const ImVec2 bottomRight(
        topLeft.x + labelWidth,
        topLeft.y + labelHeight);

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    drawList->AddRectFilled(
        topLeft,
        bottomRight,
        IM_COL32(18, 25, 29, 225),
        5.0F);
    drawList->AddRect(
        topLeft,
        bottomRight,
        IM_COL32(250, 205, 62, 255),
        5.0F);
    drawList->AddText(
        ImVec2(topLeft.x + padding.x, topLeft.y + padding.y),
        IM_COL32(255, 230, 145, 255),
        labelBegin,
        labelEnd);
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
