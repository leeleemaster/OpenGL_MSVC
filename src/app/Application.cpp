#include "app/Application.h"
#include "app/CameraController.h"

#include "core/BuildInfo.h"
#include "core/MeshData.h"
#include "core/OrbitCamera.h"
#include "core/PathText.h"
#include "core/PointMeasurement.h"
#include "core/RayPicking.h"
#include "io/MeshLoader.h"
#include "minishader/Compiler.h"
#include "platform/ExecutablePath.h"
#include "renderer/GpuMesh.h"
#include "renderer/ShaderProgram.h"
#include "renderer/SelectionMarker.h"
#include "ui/ViewerUi.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace {

constexpr int initialWindowWidth = 1280;
constexpr int initialWindowHeight = 720;

std::filesystem::path findShaderDirectory()
{
    const std::filesystem::path sourceDirectory =
        std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    std::vector<std::filesystem::path> candidates;
    candidates.reserve(3);
    candidates.push_back(dentalviz::executableDirectory() / "assets" / "shaders");
    std::error_code currentPathError;
    const std::filesystem::path currentDirectory =
        std::filesystem::current_path(currentPathError);
    if (!currentPathError) {
        candidates.push_back(currentDirectory / "assets" / "shaders");
    }
    candidates.push_back(sourceDirectory / "assets" / "shaders");

    constexpr std::array requiredShaderFiles{
        "mesh.vert",
        "mesh.frag",
        "marker.vert",
        "marker.frag",
        "measurement_line.vert",
        "measurement_line.frag",
    };

    for (const std::filesystem::path& candidate : candidates) {
        const bool complete = std::all_of(
            requiredShaderFiles.begin(),
            requiredShaderFiles.end(),
            [&candidate](const char* fileName) {
                std::error_code fileError;
                const bool regular =
                    std::filesystem::is_regular_file(candidate / fileName, fileError);
                return regular && !fileError;
            });
        if (complete) {
            return candidate;
        }
    }

    throw std::runtime_error(
        "Shader assets were not found. Expected mesh, marker, and measurement line shaders.");
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

dentalviz::ViewerModelInfo proceduralModelInfo(const dentalviz::MeshData& mesh)
{
    dentalviz::ViewerModelInfo information;
    information.name = "절차 생성 치아 (테스트 형상)";
    information.vertexCount = mesh.vertices.size();
    information.triangleCount = mesh.indices.size() / 3;
    information.bounds = mesh.bounds();
    return information;
}

dentalviz::ViewerModelInfo loadedModelInfo(
    const dentalviz::MeshData& mesh,
    const dentalviz::MeshLoadResult& result)
{
    dentalviz::ViewerModelInfo information;
    information.name = dentalviz::pathToUtf8(result.sourcePath.filename());
    information.sourcePath = result.sourcePath;
    information.vertexCount = mesh.vertices.size();
    information.triangleCount = mesh.indices.size() / 3;
    information.sourceMeshCount = result.sourceMeshCount;
    information.bounds = mesh.bounds();
    information.loadMilliseconds =
        static_cast<double>(result.loadDuration.count()) / 1000.0;
    information.loadedFromFile = true;
    return information;
}

void printModelInformation(const dentalviz::ViewerModelInfo& information)
{
    const glm::vec3 boundsSize = information.bounds.size();
    std::cout << "Model: " << information.name << '\n'
              << "Mesh: " << information.vertexCount << " vertices, "
              << information.triangleCount << " triangles\n"
              << "Bounds size: " << boundsSize.x << ", "
              << boundsSize.y << ", " << boundsSize.z << '\n';
    if (information.loadedFromFile) {
        std::cout << "Source: " << dentalviz::pathToUtf8(information.sourcePath) << '\n'
                  << "Source meshes: " << information.sourceMeshCount << '\n'
                  << "Load time: " << std::fixed << std::setprecision(3)
                  << information.loadMilliseconds << " ms\n";
    }
    std::cout << "Unit: model units (source scale is not inferred)\n";
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
    glfwSetWindowSizeLimits(window_, 800, 520, GLFW_DONT_CARE, GLFW_DONT_CARE);

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

int Application::run(const ApplicationRunOptions& options)
{
    if (options.maximumRuntimeSeconds.has_value() &&
        (!std::isfinite(options.maximumRuntimeSeconds.value()) ||
         options.maximumRuntimeSeconds.value() <= 0.0)) {
        throw std::invalid_argument("Maximum runtime must be a positive finite number.");
    }
    if (options.modelPath.has_value() && options.modelPath->empty()) {
        throw std::invalid_argument("Model path must not be empty.");
    }

    MeshData modelData = makeProceduralTooth();
    ViewerUiState uiState;
    uiState.model = proceduralModelInfo(modelData);
    if (options.modelPath.has_value()) {
        try {
            MeshLoadResult loadedModel = MeshLoader::load(options.modelPath.value());
            MeshData loadedData = std::move(loadedModel.mesh);
            uiState.model = loadedModelInfo(loadedData, loadedModel);
            modelData = std::move(loadedData);
            uiState.statusMessage = "시작 모델을 불러왔습니다.";
        } catch (const std::exception& error) {
            std::cerr << "Model load failed: " << error.what() << '\n'
                      << "Falling back to procedural test geometry.\n";
            uiState.statusMessage =
                "시작 모델을 불러오지 못해 절차 생성 테스트 형상을 사용합니다.";
            uiState.statusIsError = true;
        }
    }

    AxisAlignedBounds modelBounds = modelData.bounds();
    uiState.clippingPlane.reset(modelBounds);
    GpuMesh gpuMesh(modelData);
    const std::filesystem::path shaderDirectory = findShaderDirectory();
    ShaderProgram toothShader = ShaderProgram::fromFiles(
        shaderDirectory / "mesh.vert",
        shaderDirectory / "mesh.frag");
    const SelectionMarker selectionMarker(
        shaderDirectory / "marker.vert",
        shaderDirectory / "marker.frag",
        shaderDirectory / "measurement_line.vert",
        shaderDirectory / "measurement_line.frag");

    printModelInformation(uiState.model);
    std::cout << "Shaders: " << pathToUtf8(shaderDirectory) << '\n';

    int startupWindowWidth = 0;
    int startupWindowHeight = 0;
    glfwGetWindowSize(window_, &startupWindowWidth, &startupWindowHeight);
    constexpr int initialPropertiesWidth =
        static_cast<int>(ViewerUi::propertiesPanelWidth);
    const int initialViewerWidth = std::max(startupWindowWidth - initialPropertiesWidth, 1);
    const float initialAspectRatio = startupWindowHeight > 0
        ? static_cast<float>(initialViewerWidth) / static_cast<float>(startupWindowHeight)
        : 16.0F / 9.0F;

    OrbitCamera camera;
    camera.fit(modelBounds, initialAspectRatio);
    CameraController cameraController(window_, camera, modelBounds);
    ViewerUi viewerUi(window_);
    std::cout << "Controls: Left click A/B | Left drag orbit | Middle drag pan | Wheel zoom | F fit\n"
              << "Render modes: 1 Solid | 2 Wireframe | 3 Normals | Esc close\n";

    bool solidWasPressed = false;
    bool wireframeWasPressed = false;
    bool normalsWasPressed = false;
    unsigned int renderModeChanges = 0;

    const double startTime = glfwGetTime();
    double titleIntervalStart = startTime;
    unsigned int renderedFrames = 0;

    while (glfwWindowShouldClose(window_) == GLFW_FALSE) {
        const double now = glfwGetTime();
        if (options.maximumRuntimeSeconds.has_value() &&
            now - startTime >= options.maximumRuntimeSeconds.value()) {
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
            continue;
        }

        if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }

        int windowWidth = 0;
        int windowHeight = 0;
        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetWindowSize(window_, &windowWidth, &windowHeight);
        glfwGetFramebufferSize(window_, &framebufferWidth, &framebufferHeight);
        if (windowWidth <= 0 || windowHeight <= 0 ||
            framebufferWidth <= 0 || framebufferHeight <= 0) {
            glfwPollEvents();
            continue;
        }

        viewerUi.beginFrame(windowWidth, windowHeight, framebufferWidth, framebufferHeight);
        const RenderMode modeAtFrameStart = uiState.renderMode;
        const ViewerUiActions uiActions = viewerUi.draw(uiState);

        if (uiActions.compileAndApplyMiniShader) {
            minishader::CompilationResult compilation =
                minishader::Compiler::compile(uiState.miniShader.source);
            if (!compilation.succeeded()) {
                if (!compilation.diagnostics.empty()) {
                    uiState.miniShader.compilerOutput =
                        minishader::formatDiagnostics(compilation.diagnostics);
                } else {
                    uiState.miniShader.compilerOutput = compilation.internalError;
                }
                uiState.miniShader.outputIsError = true;
                uiState.statusMessage =
                    "MiniShader 검증 실패: 마지막 정상 셰이더를 유지합니다.";
                uiState.statusIsError = true;
                std::cerr << uiState.miniShader.compilerOutput << '\n';
            } else {
                uiState.miniShader.generatedGlsl = compilation.fragmentSource;
                try {
                    ShaderProgram replacement =
                        ShaderProgram::fromVertexFileAndFragmentSource(
                            shaderDirectory / "mesh.vert",
                            compilation.fragmentSource,
                            "MiniShader generated fragment");
                    toothShader = std::move(replacement);
                    ++uiState.miniShader.appliedRevision;
                    uiState.miniShader.hasActiveShader = true;
                    uiState.miniShader.compilerOutput =
                        "컴파일 및 적용에 성공했습니다. 생성된 셰이더가 적용됐습니다.";
                    uiState.miniShader.outputIsError = false;
                    uiState.statusMessage =
                        "MiniShader 개정 " +
                        std::to_string(uiState.miniShader.appliedRevision) + " 적용됨.";
                    uiState.statusIsError = false;
                    std::cout << uiState.statusMessage << '\n';
                } catch (const std::exception& error) {
                    uiState.miniShader.compilerOutput =
                        "OpenGL 셰이더 컴파일에 실패했습니다. 자세한 내용은 콘솔을 확인하세요.";
                    uiState.miniShader.outputIsError = true;
                    uiState.statusMessage =
                        "OpenGL 컴파일 실패: 마지막 정상 셰이더를 유지합니다.";
                    uiState.statusIsError = true;
                    std::cerr << error.what() << '\n';
                }
            }
        }

        if (uiActions.modelToLoad.has_value()) {
            try {
                MeshLoadResult loadedModel = MeshLoader::load(uiActions.modelToLoad.value());
                MeshData replacementData = std::move(loadedModel.mesh);
                ViewerModelInfo replacementInformation =
                    loadedModelInfo(replacementData, loadedModel);
                GpuMesh replacementGpuMesh(replacementData);

                modelData = std::move(replacementData);
                gpuMesh = std::move(replacementGpuMesh);
                modelBounds = replacementInformation.bounds;
                uiState.model = std::move(replacementInformation);
                cameraController.setModelBounds(modelBounds);
                camera.fit(modelBounds, viewerUi.viewerRect().aspectRatio());
                uiState.clippingPlane.reset(modelBounds);
                uiState.measurement.reset();
                uiState.statusMessage = "모델을 불러왔습니다.";
                uiState.statusIsError = false;
                printModelInformation(uiState.model);
            } catch (const std::exception& error) {
                uiState.statusMessage =
                    "모델을 불러오지 못했습니다. 자세한 내용은 콘솔을 확인하세요.";
                uiState.statusIsError = true;
                std::cerr << "Model load failed: " << error.what() << '\n';
            }
        }

        if (uiActions.resetCamera) {
            camera.fit(modelBounds, viewerUi.viewerRect().aspectRatio());
        }
        if (uiActions.resetClippingPlane) {
            uiState.clippingPlane.reset(modelBounds);
            uiState.statusMessage = "클리핑 평면을 모델 중심으로 초기화했습니다.";
            uiState.statusIsError = false;
        }
        if (uiActions.resetMeasurement) {
            uiState.measurement.reset();
            uiState.statusMessage = "거리 측정을 초기화했습니다. A점을 선택하세요.";
            uiState.statusIsError = false;
        }

        const bool solidPressed = keyPressedOnce(window_, GLFW_KEY_1, solidWasPressed);
        const bool wireframePressed = keyPressedOnce(window_, GLFW_KEY_2, wireframeWasPressed);
        const bool normalsPressed = keyPressedOnce(window_, GLFW_KEY_3, normalsWasPressed);
        if (!viewerUi.wantsCaptureKeyboard()) {
            if (solidPressed) {
                uiState.renderMode = RenderMode::solid;
            }
            if (wireframePressed) {
                uiState.renderMode = RenderMode::wireframe;
            }
            if (normalsPressed) {
                uiState.renderMode = RenderMode::normals;
            }
        }
        if (uiState.renderMode != modeAtFrameStart) {
            ++renderModeChanges;
            std::cout << "Render mode: " << renderModeName(uiState.renderMode) << '\n';
        }

        const float aspectRatio = viewerUi.viewerRect().aspectRatio();
        cameraController.update(
            aspectRatio,
            viewerUi.isMouseOverViewer() && !viewerUi.wantsCaptureMouse(),
            !viewerUi.wantsCaptureKeyboard());
        const glm::mat4 projection = camera.projectionMatrix(aspectRatio);
        const glm::mat4 view = camera.viewMatrix();
        const glm::mat4 model(1.0F);
        const glm::vec3 cameraPosition = camera.position();

        if (const std::optional<PickRequest> request = cameraController.consumePickRequest();
            request.has_value()) {
            const ViewerRect& pickRect = viewerUi.viewerRect();
            const float normalizedX = static_cast<float>(
                (request->windowX - static_cast<double>(pickRect.windowX)) /
                static_cast<double>(pickRect.windowWidth));
            const float normalizedY = static_cast<float>(
                (request->windowY - static_cast<double>(pickRect.windowY)) /
                static_cast<double>(pickRect.windowHeight));
            const Ray worldRay = makeWorldRayFromViewport(
                std::clamp(normalizedX, 0.0F, 1.0F),
                std::clamp(normalizedY, 0.0F, 1.0F),
                view,
                projection);
            const std::optional<RayHit> hit = pickMesh(worldRay, modelData);
            if (hit.has_value()) {
                const MeasurementUpdate update = uiState.measurement.select(hit.value());
                switch (update) {
                case MeasurementUpdate::pointASet:
                    uiState.statusMessage = "A점을 선택했습니다. B점을 선택하세요.";
                    break;
                case MeasurementUpdate::pointBSet:
                    uiState.statusMessage =
                        "거리 측정을 완료했습니다. 세 번째 클릭부터 새 A점을 선택합니다.";
                    break;
                case MeasurementUpdate::restartedWithPointA:
                    uiState.statusMessage = "새 A점을 선택했습니다. B점을 선택하세요.";
                    break;
                }
                std::cout << "Measurement point: triangle #" << hit->triangleIndex
                          << " at " << hit->position.x << ", "
                          << hit->position.y << ", "
                          << hit->position.z << '\n';
            } else {
                uiState.measurement.reset();
                uiState.statusMessage =
                    "클릭 위치에 표면이 없어 거리 측정을 초기화했습니다.";
            }
            uiState.statusIsError = false;
        }

        glDisable(GL_SCISSOR_TEST);
        glViewport(0, 0, framebufferWidth, framebufferHeight);
        glClearColor(0.055F, 0.075F, 0.090F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const ViewerRect& viewerRect = viewerUi.viewerRect();
        glEnable(GL_SCISSOR_TEST);
        glScissor(
            viewerRect.framebufferX,
            viewerRect.framebufferY,
            viewerRect.framebufferWidth,
            viewerRect.framebufferHeight);
        glViewport(
            viewerRect.framebufferX,
            viewerRect.framebufferY,
            viewerRect.framebufferWidth,
            viewerRect.framebufferHeight);
        glClearColor(0.025F, 0.055F, 0.070F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        toothShader.use();
        toothShader.setMatrix4("uModel", model);
        toothShader.setMatrix4("uView", view);
        toothShader.setMatrix4("uProjection", projection);
        toothShader.setVector3IfPresent("uBaseColor", uiState.baseColor);
        toothShader.setVector3IfPresent("uLightPosition", uiState.lightPosition);
        toothShader.setVector3IfPresent("uCameraPosition", cameraPosition);
        toothShader.setFloatIfPresent("uShininess", uiState.shininess);
        toothShader.setIntegerIfPresent(
            "uRenderMode", static_cast<int>(uiState.renderMode));
        toothShader.setIntegerIfPresent(
            "uClipEnabled", uiState.clippingPlane.enabled() ? 1 : 0);
        toothShader.setVector3IfPresent("uClipNormal", uiState.clippingPlane.normal());
        toothShader.setFloatIfPresent("uClipDistance", uiState.clippingPlane.distance());

        if (uiState.renderMode == RenderMode::wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        gpuMesh.draw();
        if (uiState.renderMode == RenderMode::wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        constexpr glm::vec3 pointAColor(1.0F, 0.30F, 0.055F);
        constexpr glm::vec3 pointBColor(0.10F, 0.82F, 1.0F);
        constexpr glm::vec3 segmentColor(0.98F, 0.80F, 0.24F);
        if (uiState.measurement.pointA().has_value() &&
            uiState.measurement.pointB().has_value()) {
            selectionMarker.drawSegment(
                uiState.measurement.pointA()->position,
                uiState.measurement.pointB()->position,
                segmentColor,
                view,
                projection);
        }
        if (uiState.measurement.pointA().has_value()) {
            selectionMarker.drawMarker(
                uiState.measurement.pointA()->position,
                pointAColor,
                view,
                projection);
        }
        if (uiState.measurement.pointB().has_value()) {
            selectionMarker.drawMarker(
                uiState.measurement.pointB()->position,
                pointBColor,
                view,
                projection);
        }

        if (const std::optional<float> distance = uiState.measurement.distance();
            distance.has_value()) {
            std::ostringstream label;
            label << "3D 직선거리: " << std::fixed << std::setprecision(3)
                  << distance.value() << " 모델 단위";
            const glm::vec3 midpoint =
                (uiState.measurement.pointA()->position +
                 uiState.measurement.pointB()->position) * 0.5F;
            viewerUi.drawMeasurementLabel(midpoint, view, projection, label.str());
        }

        glDisable(GL_SCISSOR_TEST);
        viewerUi.render();

        glfwSwapBuffers(window_);
        glfwPollEvents();
        ++renderedFrames;

        const double titleInterval = now - titleIntervalStart;
        if (titleInterval >= 0.5) {
            const double framesPerSecond = static_cast<double>(renderedFrames) / titleInterval;
            uiState.framesPerSecond = static_cast<float>(framesPerSecond);
            std::ostringstream title;
            title << projectName() << " | " << renderModeName(uiState.renderMode)
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
              << " zoom, " << interactions.fitRequests << " fit, "
              << interactions.selectionRequests << " selection requests\n";
    std::cout << "Render mode changes: " << renderModeChanges << '\n';
    std::cout << "Application loop exited cleanly.\n";
    return 0;
}

} // namespace dentalviz
