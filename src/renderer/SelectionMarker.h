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
        const std::filesystem::path& fragmentShaderPath);
    ~SelectionMarker();

    SelectionMarker(const SelectionMarker&) = delete;
    SelectionMarker& operator=(const SelectionMarker&) = delete;
    SelectionMarker(SelectionMarker&&) = delete;
    SelectionMarker& operator=(SelectionMarker&&) = delete;

    void draw(
        const glm::vec3& position,
        const glm::mat4& view,
        const glm::mat4& projection) const;

private:
    ShaderProgram shader_;
    GLuint vertexArray_ = 0;
};

} // namespace dentalviz
