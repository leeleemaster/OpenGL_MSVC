#pragma once

#include <glad/glad.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <filesystem>
#include <string_view>

namespace dentalviz {

class ShaderProgram final {
public:
    ShaderProgram(std::string_view vertexSource, std::string_view fragmentSource);
    [[nodiscard]] static ShaderProgram fromFiles(
        const std::filesystem::path& vertexPath,
        const std::filesystem::path& fragmentPath);
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    void use() const noexcept;
    void setMatrix4(const char* name, const glm::mat4& value) const;
    void setVector3(const char* name, const glm::vec3& value) const;
    void setFloat(const char* name, float value) const;
    void setInteger(const char* name, int value) const;

private:
    ShaderProgram(
        std::string_view vertexSource,
        std::string_view fragmentSource,
        std::string_view vertexLabel,
        std::string_view fragmentLabel);

    [[nodiscard]] GLint uniformLocation(const char* name) const;

    GLuint program_ = 0;
};

} // namespace dentalviz
