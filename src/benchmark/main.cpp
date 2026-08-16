#include "core/MeshData.h"
#include "core/OrbitCamera.h"
#include "core/RayPicking.h"
#include "io/MeshLoader.h"
#include "renderer/GpuMesh.h"
#include "renderer/ShaderProgram.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

constexpr int benchmarkWidth = 1280;
constexpr int benchmarkHeight = 720;
constexpr std::size_t loadWarmupCount = 1;
constexpr std::size_t loadSampleCount = 3;
constexpr std::size_t uploadSampleCount = 10;
constexpr std::size_t frameWarmupCount = 120;
constexpr std::size_t frameSampleCount = 600;
constexpr std::size_t pickingWarmupCount = 1;
constexpr std::size_t pickingSampleCount = 10;

struct BenchmarkCase {
    const char* name;
    std::uint32_t radialSegments;
};

constexpr std::array benchmarkCases{
    BenchmarkCase{"100k", 5'556},
    BenchmarkCase{"500k", 27'778},
};

struct CaseResult {
    std::string name;
    std::size_t triangleCount = 0;
    std::size_t vertexCount = 0;
    std::uintmax_t fileBytes = 0;
    std::vector<double> loadMilliseconds;
    std::vector<double> uploadMilliseconds;
    std::vector<double> frameMilliseconds;
    std::vector<double> pickingMilliseconds;
    std::size_t pickingHitCount = 0;
};

class BenchmarkContext final {
public:
    BenchmarkContext()
    {
        if (glfwInit() != GLFW_TRUE) {
            throw std::runtime_error("Benchmark could not initialize GLFW.");
        }
        initialized_ = true;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        window_ = glfwCreateWindow(
            benchmarkWidth,
            benchmarkHeight,
            "DentalViz benchmark",
            nullptr,
            nullptr);
        if (window_ == nullptr) {
            glfwTerminate();
            initialized_ = false;
            throw std::runtime_error("Benchmark could not create an OpenGL 3.3 context.");
        }

        glfwMakeContextCurrent(window_);
        if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
            glfwDestroyWindow(window_);
            window_ = nullptr;
            glfwTerminate();
            initialized_ = false;
            throw std::runtime_error("Benchmark could not load OpenGL functions.");
        }
        glfwSwapInterval(0);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_MULTISAMPLE);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glViewport(0, 0, benchmarkWidth, benchmarkHeight);
    }

    ~BenchmarkContext()
    {
        if (window_ != nullptr) {
            glfwDestroyWindow(window_);
        }
        if (initialized_) {
            glfwTerminate();
        }
    }

    BenchmarkContext(const BenchmarkContext&) = delete;
    BenchmarkContext& operator=(const BenchmarkContext&) = delete;

    [[nodiscard]] GLFWwindow* window() const noexcept
    {
        return window_;
    }

private:
    GLFWwindow* window_ = nullptr;
    bool initialized_ = false;
};

[[nodiscard]] const char* glString(GLenum name) noexcept
{
    const auto* value = glGetString(name);
    return value != nullptr ? reinterpret_cast<const char*>(value) : "Unavailable";
}

[[nodiscard]] double elapsedMilliseconds(Clock::time_point start, Clock::time_point end)
{
    return std::chrono::duration<double, std::milli>(end - start).count();
}

[[nodiscard]] double mean(const std::vector<double>& samples)
{
    if (samples.empty()) {
        throw std::invalid_argument("Cannot summarize an empty sample set.");
    }
    return std::accumulate(samples.begin(), samples.end(), 0.0) /
           static_cast<double>(samples.size());
}

[[nodiscard]] double median(const std::vector<double>& samples)
{
    if (samples.empty()) {
        throw std::invalid_argument("Cannot summarize an empty sample set.");
    }
    std::vector<double> ordered = samples;
    std::sort(ordered.begin(), ordered.end());
    const std::size_t middle = ordered.size() / 2U;
    if (ordered.size() % 2U == 0U) {
        return (ordered[middle - 1U] + ordered[middle]) * 0.5;
    }
    return ordered[middle];
}

[[nodiscard]] double percentile95(const std::vector<double>& samples)
{
    if (samples.empty()) {
        throw std::invalid_argument("Cannot summarize an empty sample set.");
    }
    std::vector<double> ordered = samples;
    std::sort(ordered.begin(), ordered.end());
    const std::size_t nearestRank = static_cast<std::size_t>(
        std::ceil(0.95 * static_cast<double>(ordered.size())));
    return ordered[std::max<std::size_t>(nearestRank, 1U) - 1U];
}

void writeObj(const dentalviz::MeshData& mesh, const std::filesystem::path& path)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw std::runtime_error("Benchmark OBJ could not be created: " + path.string());
    }
    file.imbue(std::locale::classic());
    file << std::setprecision(9);
    for (const dentalviz::Vertex& vertex : mesh.vertices) {
        file << "v " << vertex.position.x << ' ' << vertex.position.y << ' '
             << vertex.position.z << '\n';
    }
    for (const dentalviz::Vertex& vertex : mesh.vertices) {
        file << "vn " << vertex.normal.x << ' ' << vertex.normal.y << ' '
             << vertex.normal.z << '\n';
    }
    for (std::size_t index = 0; index < mesh.indices.size(); index += 3U) {
        const std::uint32_t first = mesh.indices[index] + 1U;
        const std::uint32_t second = mesh.indices[index + 1U] + 1U;
        const std::uint32_t third = mesh.indices[index + 2U] + 1U;
        file << "f " << first << "//" << first << ' '
             << second << "//" << second << ' '
             << third << "//" << third << '\n';
    }
    if (!file) {
        throw std::runtime_error("Benchmark OBJ could not be written completely: " + path.string());
    }
}

void setViewerUniforms(
    const dentalviz::ShaderProgram& shader,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& cameraPosition)
{
    shader.use();
    shader.setMatrix4("uModel", glm::mat4(1.0F));
    shader.setMatrix4("uView", view);
    shader.setMatrix4("uProjection", projection);
    shader.setVector3("uBaseColor", glm::vec3(0.86F, 0.76F, 0.56F));
    shader.setVector3("uLightPosition", glm::vec3(3.2F, 4.0F, 4.5F));
    shader.setVector3("uCameraPosition", cameraPosition);
    shader.setFloat("uShininess", 72.0F);
    shader.setInteger("uRenderMode", 0);
    shader.setInteger("uClipEnabled", 0);
    shader.setVector3("uClipNormal", glm::vec3(1.0F, 0.0F, 0.0F));
    shader.setFloat("uClipDistance", 0.0F);
}

void drawFrame(
    GLFWwindow* window,
    const dentalviz::ShaderProgram& shader,
    const dentalviz::GpuMesh& gpuMesh,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& cameraPosition)
{
    glClearColor(0.025F, 0.055F, 0.070F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setViewerUniforms(shader, view, projection, cameraPosition);
    gpuMesh.draw();
    glfwSwapBuffers(window);
    glfwPollEvents();
}

[[nodiscard]] std::vector<double> measureUploads(const dentalviz::MeshData& mesh)
{
    std::vector<double> samples;
    samples.reserve(uploadSampleCount);
    for (std::size_t iteration = 0; iteration < uploadSampleCount; ++iteration) {
        glFinish();
        const Clock::time_point start = Clock::now();
        {
            dentalviz::GpuMesh upload(mesh);
            glFinish();
            samples.push_back(elapsedMilliseconds(start, Clock::now()));
        }
        glFinish();
    }
    return samples;
}

[[nodiscard]] std::vector<double> measureFrames(
    GLFWwindow* window,
    const dentalviz::ShaderProgram& shader,
    const dentalviz::GpuMesh& gpuMesh,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& cameraPosition)
{
    for (std::size_t iteration = 0; iteration < frameWarmupCount; ++iteration) {
        drawFrame(window, shader, gpuMesh, view, projection, cameraPosition);
    }
    glFinish();

    std::vector<double> samples;
    samples.reserve(frameSampleCount);
    for (std::size_t iteration = 0; iteration < frameSampleCount; ++iteration) {
        const Clock::time_point start = Clock::now();
        drawFrame(window, shader, gpuMesh, view, projection, cameraPosition);
        samples.push_back(elapsedMilliseconds(start, Clock::now()));
    }
    glFinish();
    return samples;
}

[[nodiscard]] std::pair<std::vector<double>, std::size_t> measurePicking(
    const dentalviz::MeshData& mesh)
{
    const dentalviz::AxisAlignedBounds bounds = mesh.bounds();
    const glm::vec3 center = bounds.center();
    const glm::vec3 origin = center + glm::vec3(0.0F, 0.0F, 5.0F);
    const dentalviz::Ray ray{origin, glm::normalize(center - origin)};

    for (std::size_t iteration = 0; iteration < pickingWarmupCount; ++iteration) {
        static_cast<void>(dentalviz::pickMesh(ray, mesh));
    }

    std::vector<double> samples;
    samples.reserve(pickingSampleCount);
    std::size_t hits = 0;
    for (std::size_t iteration = 0; iteration < pickingSampleCount; ++iteration) {
        const Clock::time_point start = Clock::now();
        const std::optional<dentalviz::RayHit> hit = dentalviz::pickMesh(ray, mesh);
        samples.push_back(elapsedMilliseconds(start, Clock::now()));
        if (hit.has_value()) {
            ++hits;
        }
    }
    return {std::move(samples), hits};
}

[[nodiscard]] CaseResult runCase(
    GLFWwindow* window,
    const dentalviz::ShaderProgram& shader,
    const BenchmarkCase& benchmarkCase,
    const std::filesystem::path& modelDirectory)
{
    std::cout << "Preparing " << benchmarkCase.name << " benchmark model...\n";
    const dentalviz::MeshData generated =
        dentalviz::makeProceduralTooth(benchmarkCase.radialSegments);
    const std::filesystem::path modelPath =
        modelDirectory / (std::string("procedural-tooth-") + benchmarkCase.name + ".obj");
    writeObj(generated, modelPath);

    for (std::size_t iteration = 0; iteration < loadWarmupCount; ++iteration) {
        static_cast<void>(dentalviz::MeshLoader::load(modelPath));
    }

    CaseResult result;
    result.name = benchmarkCase.name;
    result.fileBytes = std::filesystem::file_size(modelPath);
    dentalviz::MeshData loadedMesh;
    result.loadMilliseconds.reserve(loadSampleCount);
    for (std::size_t iteration = 0; iteration < loadSampleCount; ++iteration) {
        dentalviz::MeshLoadResult loaded = dentalviz::MeshLoader::load(modelPath);
        result.loadMilliseconds.push_back(
            static_cast<double>(loaded.loadDuration.count()) / 1000.0);
        loadedMesh = std::move(loaded.mesh);
    }

    result.triangleCount = loadedMesh.indices.size() / 3U;
    result.vertexCount = loadedMesh.vertices.size();
    result.uploadMilliseconds = measureUploads(loadedMesh);

    const dentalviz::AxisAlignedBounds bounds = loadedMesh.bounds();
    dentalviz::OrbitCamera camera;
    constexpr float aspect =
        static_cast<float>(benchmarkWidth) / static_cast<float>(benchmarkHeight);
    camera.fit(bounds, aspect);
    const glm::mat4 view = camera.viewMatrix();
    const glm::mat4 projection = camera.projectionMatrix(aspect);
    const glm::vec3 cameraPosition = camera.position();
    {
        dentalviz::GpuMesh gpuMesh(loadedMesh);
        result.frameMilliseconds =
            measureFrames(window, shader, gpuMesh, view, projection, cameraPosition);
    }

    auto [pickingSamples, hitCount] = measurePicking(loadedMesh);
    result.pickingMilliseconds = std::move(pickingSamples);
    result.pickingHitCount = hitCount;
    if (hitCount != pickingSampleCount) {
        throw std::runtime_error("Benchmark picking ray did not hit on every measured iteration.");
    }
    return result;
}

void appendRawSamples(
    std::ofstream& output,
    const CaseResult& result,
    std::string_view metric,
    const std::vector<double>& samples)
{
    for (std::size_t iteration = 0; iteration < samples.size(); ++iteration) {
        output << result.name << ',' << result.triangleCount << ',' << result.vertexCount << ','
               << result.fileBytes << ',' << metric << ',' << (iteration + 1U) << ','
               << std::setprecision(9) << samples[iteration] << '\n';
    }
}

void writeRawCsv(const std::filesystem::path& path, const std::vector<CaseResult>& results)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Benchmark raw CSV could not be created: " + path.string());
    }
    output.imbue(std::locale::classic());
    output << "case,triangles,vertices,file_bytes,metric,iteration,value_ms\n";
    for (const CaseResult& result : results) {
        appendRawSamples(output, result, "model_load", result.loadMilliseconds);
        appendRawSamples(output, result, "gpu_upload_cpu_observed", result.uploadMilliseconds);
        appendRawSamples(output, result, "cpu_frame_submit_swap", result.frameMilliseconds);
        appendRawSamples(output, result, "cpu_picking", result.pickingMilliseconds);
    }
}

[[nodiscard]] std::string processorDescription()
{
    char* processorValue = nullptr;
    std::size_t processorLength = 0;
    const errno_t result =
        _dupenv_s(&processorValue, &processorLength, "PROCESSOR_IDENTIFIER");
    std::unique_ptr<char, decltype(&std::free)> processor(processorValue, &std::free);
    if (result == 0 && processor != nullptr) {
        return processor.get();
    }
    return "Unavailable";
}

void writeSummary(const std::filesystem::path& path, const std::vector<CaseResult>& results)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Benchmark summary could not be created: " + path.string());
    }
    output.imbue(std::locale::classic());
    output << "# DentalViz Performance Benchmark\n\n"
           << "Generated by the Release `dentalviz_benchmark` target.\n\n"
           << "## Machine and fixed conditions\n\n"
           << "- CPU: " << processorDescription() << "\n"
           << "- Logical threads: " << std::thread::hardware_concurrency() << "\n"
           << "- GPU: " << glString(GL_RENDERER) << "\n"
           << "- OpenGL: " << glString(GL_VERSION) << "\n"
           << "- Window/backbuffer: " << benchmarkWidth << "x" << benchmarkHeight << "\n"
           << "- Build: Release\n"
           << "- VSync: Off (`glfwSwapInterval(0)`)\n"
           << "- Render mode: Solid, fixed camera fitted to model bounds\n"
           << "- Frame warm-up/samples: " << frameWarmupCount << "/" << frameSampleCount
           << "\n"
           << "- Load warm-up/samples: " << loadWarmupCount << "/" << loadSampleCount
           << " (warm filesystem cache)\n"
           << "- GPU upload samples: " << uploadSampleCount
           << " (`glFinish` after upload)\n"
           << "- Picking warm-up/samples: " << pickingWarmupCount << "/"
           << pickingSampleCount << "\n\n"
           << "## Results\n\n"
           << "| Model | Triangles | OBJ MiB | Load median ms | Upload median ms | "
              "CPU frame median ms | CPU frame p95 ms | FPS | Picking median ms | "
              "Picking p95 ms |\n"
           << "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n";

    output << std::fixed << std::setprecision(3);
    for (const CaseResult& result : results) {
        const double framesPerSecond = 1000.0 / mean(result.frameMilliseconds);
        const double fileMebibytes =
            static_cast<double>(result.fileBytes) / (1024.0 * 1024.0);
        output << "| " << result.name << " | " << result.triangleCount << " | "
               << fileMebibytes << " | " << median(result.loadMilliseconds) << " | "
               << median(result.uploadMilliseconds) << " | "
               << median(result.frameMilliseconds) << " | "
               << percentile95(result.frameMilliseconds) << " | " << framesPerSecond << " | "
               << median(result.pickingMilliseconds) << " | "
               << percentile95(result.pickingMilliseconds) << " |\n";
    }

    output << "\n## Interpretation boundary\n\n"
           << "`model_load` includes file read, Assimp import/post-processing, and conversion to "
              "DentalViz CPU mesh data. Upload is CPU-observed wall time through `glFinish`; it "
              "is not a GPU timer query. CPU frame time measures clear, uniform submission, draw "
              "submission, event polling, and VSync-off buffer swap without a per-frame "
              "`glFinish`; it must not be presented as GPU execution time. Picking measures the "
              "current AABB gate plus brute-force nearest ray/triangle search.\n\n"
           << "Every individual measured sample is stored in `benchmark-raw.csv`.\n";
}

[[nodiscard]] std::filesystem::path parseOutputDirectory(int argc, char* argv[])
{
    if (argc == 1) {
        return std::filesystem::path(DENTALVIZ_SOURCE_DIR) / "out" / "benchmark";
    }
    if (argc == 3 && std::string_view(argv[1]) == "--output") {
        return std::filesystem::absolute(std::filesystem::path(argv[2])).lexically_normal();
    }
    throw std::invalid_argument("Usage: dentalviz_benchmark [--output <directory>]");
}

int runBenchmark(const std::filesystem::path& outputDirectory, GLFWwindow* window)
{
    std::filesystem::create_directories(outputDirectory);
    const std::filesystem::path modelDirectory = outputDirectory / "models";
    std::filesystem::create_directories(modelDirectory);

    const std::filesystem::path shaderDirectory(DENTALVIZ_SHADER_DIR);
    const dentalviz::ShaderProgram shader = dentalviz::ShaderProgram::fromFiles(
        shaderDirectory / "mesh.vert",
        shaderDirectory / "mesh.frag");

    std::vector<CaseResult> results;
    results.reserve(benchmarkCases.size());
    for (const BenchmarkCase& benchmarkCase : benchmarkCases) {
        results.push_back(runCase(window, shader, benchmarkCase, modelDirectory));
    }

    const std::filesystem::path rawPath = outputDirectory / "benchmark-raw.csv";
    const std::filesystem::path summaryPath = outputDirectory / "benchmark-summary.md";
    writeRawCsv(rawPath, results);
    writeSummary(summaryPath, results);

    if (const GLenum error = glGetError(); error != GL_NO_ERROR) {
        throw std::runtime_error(
            "OpenGL error after benchmark: " + std::to_string(error));
    }
    std::cout << "Raw results: " << rawPath.string() << '\n'
              << "Summary: " << summaryPath.string() << '\n';
    return 0;
}

} // namespace

int main(int argc, char* argv[])
{
    try {
        const std::filesystem::path outputDirectory = parseOutputDirectory(argc, argv);
        BenchmarkContext context;
        return runBenchmark(outputDirectory, context.window());
    } catch (const std::exception& error) {
        std::cerr << "DentalViz benchmark failed: " << error.what() << '\n';
        return 1;
    }
}
