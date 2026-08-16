cmake_minimum_required(VERSION 3.25)

project(
    DentalViz
    VERSION 0.1.0
    DESCRIPTION "C++ dental mesh visualization and geometry interaction"
    LANGUAGES CXX
)

include(CTest)
include(cmake/Warnings.cmake)

option(DENTALVIZ_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

find_package(assimp CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_library(dentalviz_core STATIC
    src/core/BuildInfo.cpp
    src/core/ClickGesture.cpp
    src/core/ClippingPlane.cpp
    src/core/MeshData.cpp
    src/core/OrbitCamera.cpp
    src/core/PathText.cpp
    src/core/PointMeasurement.cpp
    src/core/RayPicking.cpp
    src/minishader/Diagnostic.cpp
    src/minishader/Compiler.cpp
    src/minishader/GlslGenerator.cpp
    src/minishader/Lexer.cpp
    src/minishader/Parser.cpp
    src/minishader/SemanticAnalyzer.cpp
    src/minishader/Token.cpp
    src/minishader/Type.cpp
)

target_include_directories(dentalviz_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(dentalviz_core PUBLIC cxx_std_20)
target_link_libraries(dentalviz_core PUBLIC glm::glm)
target_compile_definitions(dentalviz_core
    PRIVATE
        DENTALVIZ_VERSION="${PROJECT_VERSION}"
)
dentalviz_set_warnings(dentalviz_core)

add_library(dentalviz_io STATIC
    src/io/MeshLoader.cpp
)
target_include_directories(dentalviz_io
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_features(dentalviz_io PUBLIC cxx_std_20)
target_link_libraries(dentalviz_io
    PUBLIC
        dentalviz_core
        assimp::assimp
)
dentalviz_set_warnings(dentalviz_io)

add_executable(DentalViz
    src/app/Application.cpp
    src/app/CameraController.cpp
    src/platform/ExecutablePath.cpp
    src/platform/NativeFileDialog.cpp
    src/renderer/GpuMesh.cpp
    src/renderer/ShaderProgram.cpp
    src/renderer/SelectionMarker.cpp
    src/ui/ViewerUi.cpp
    src/main.cpp
)
target_link_libraries(DentalViz
    PRIVATE
        dentalviz_core
        dentalviz_io
        glad::glad
        glfw
        imgui::imgui
        Comdlg32
)
target_compile_features(DentalViz PRIVATE cxx_std_20)
target_compile_definitions(DentalViz PRIVATE GLFW_INCLUDE_NONE)
dentalviz_set_warnings(DentalViz)

add_executable(dentalviz_benchmark
    src/benchmark/main.cpp
    src/renderer/GpuMesh.cpp
    src/renderer/ShaderProgram.cpp
)
target_link_libraries(dentalviz_benchmark
    PRIVATE
        dentalviz_core
        dentalviz_io
        glad::glad
        glfw
)
target_compile_features(dentalviz_benchmark PRIVATE cxx_std_20)
target_compile_definitions(dentalviz_benchmark
    PRIVATE
        GLFW_INCLUDE_NONE
        DENTALVIZ_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders"
        DENTALVIZ_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
dentalviz_set_warnings(dentalviz_benchmark)

add_custom_command(TARGET DentalViz POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders
        $<TARGET_FILE_DIR:DentalViz>/assets/shaders
    COMMENT "Copying DentalViz shader assets"
)

if(BUILD_TESTING)
    find_package(Catch2 3 CONFIG REQUIRED)

    add_executable(dentalviz_tests
        tests/ClippingPlaneTests.cpp
        tests/MeshDataTests.cpp
        tests/ClickGestureTests.cpp
        tests/MeshLoaderTests.cpp
        tests/MiniShaderCompilerTests.cpp
        tests/MiniShaderGlslGeneratorTests.cpp
        tests/MiniShaderLexerTests.cpp
        tests/MiniShaderParserTests.cpp
        tests/MiniShaderSemanticTests.cpp
        tests/OrbitCameraTests.cpp
        tests/PointMeasurementTests.cpp
        tests/RayPickingTests.cpp
        tests/SmokeTests.cpp
        tests/ViewerRectTests.cpp
    )
    target_link_libraries(dentalviz_tests
        PRIVATE
            dentalviz_core
            dentalviz_io
            Catch2::Catch2WithMain
    )
    target_compile_features(dentalviz_tests PRIVATE cxx_std_20)
    target_compile_definitions(dentalviz_tests
        PRIVATE
            DENTALVIZ_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
    )
    dentalviz_set_warnings(dentalviz_tests)

    include(Catch)
    catch_discover_tests(dentalviz_tests)
endif()
