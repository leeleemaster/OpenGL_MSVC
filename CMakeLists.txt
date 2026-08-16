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

find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_library(dentalviz_core STATIC
    src/core/BuildInfo.cpp
    src/core/MeshData.cpp
    src/core/OrbitCamera.cpp
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

add_executable(DentalViz
    src/app/Application.cpp
    src/app/CameraController.cpp
    src/renderer/GpuMesh.cpp
    src/renderer/ShaderProgram.cpp
    src/main.cpp
)
target_link_libraries(DentalViz
    PRIVATE
        dentalviz_core
        glad::glad
        glfw
)
target_compile_features(DentalViz PRIVATE cxx_std_20)
target_compile_definitions(DentalViz PRIVATE GLFW_INCLUDE_NONE)
dentalviz_set_warnings(DentalViz)

if(BUILD_TESTING)
    find_package(Catch2 3 CONFIG REQUIRED)

    add_executable(dentalviz_tests
        tests/MeshDataTests.cpp
        tests/OrbitCameraTests.cpp
        tests/SmokeTests.cpp
    )
    target_link_libraries(dentalviz_tests
        PRIVATE
            dentalviz_core
            Catch2::Catch2WithMain
    )
    target_compile_features(dentalviz_tests PRIVATE cxx_std_20)
    dentalviz_set_warnings(dentalviz_tests)

    include(Catch)
    catch_discover_tests(dentalviz_tests)
endif()
