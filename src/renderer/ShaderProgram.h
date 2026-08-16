#pragma once

#include <glad/glad.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <string_view>

namespace dentalviz {

class ShaderProgram final {
public:
    ShaderProgram(std::string_view vertexSource, std::string_view fragmentSource);
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    void use() const noexcept;
    void setMatrix4(const char* name, const glm::mat4& value) const;
    void setVector3(const char* name, const glm::vec3& value) const;

private:
    [[nodiscard]] GLint uniformLocation(const char* name) const;

    GLuint program_ = 0;
};

} // namespace dentalviz
