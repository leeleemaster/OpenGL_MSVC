#pragma once

#include "renderer/ShaderProgram.h"

#include <glad/glad.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <filesystem>

namespace dentalviz {

class SelectionMarker final {
public:
    SelectionMarker(
        const std::filesystem::path& vertexShaderPath,
        const std::filesystem::path& fragmentShaderPath,
        const std::filesystem::path& lineVertexShaderPath,
        const std::filesystem::path& lineFragmentShaderPath);
    ~SelectionMarker();

    SelectionMarker(const SelectionMarker&) = delete;
    SelectionMarker& operator=(const SelectionMarker&) = delete;
    SelectionMarker(SelectionMarker&&) = delete;
    SelectionMarker& operator=(SelectionMarker&&) = delete;

    void drawMarker(
        const glm::vec3& position,
        const glm::vec3& color,
        const glm::mat4& view,
        const glm::mat4& projection) const;
    void drawSegment(
        const glm::vec3& pointA,
        const glm::vec3& pointB,
        const glm::vec3& color,
        const glm::mat4& view,
        const glm::mat4& projection) const;

private:
    void release() noexcept;

    ShaderProgram markerShader_;
    ShaderProgram lineShader_;
    GLuint markerVertexArray_ = 0;
    GLuint lineVertexArray_ = 0;
    GLuint lineVertexBuffer_ = 0;
};

} // namespace dentalviz
