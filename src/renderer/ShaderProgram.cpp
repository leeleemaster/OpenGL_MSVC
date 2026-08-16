#include "renderer/ShaderProgram.h"

#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

GLuint compileShader(GLenum type, std::string_view source)
{
    const GLuint shader = glCreateShader(type);
    const char* sourceText = source.data();
    const GLint sourceLength = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &sourceText, &sourceLength);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        return shader;
    }

    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<char> log(static_cast<std::size_t>(logLength > 0 ? logLength : 1));
    glGetShaderInfoLog(shader, logLength, nullptr, log.data());
    glDeleteShader(shader);
    throw std::runtime_error("OpenGL shader compilation failed: " + std::string(log.data()));
}

} // namespace

namespace dentalviz {

ShaderProgram::ShaderProgram(std::string_view vertexSource, std::string_view fragmentSource)
{
    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = 0;
    try {
        fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    } catch (...) {
        glDeleteShader(vertexShader);
        throw;
    }

    program_ = glCreateProgram();
    glAttachShader(program_, vertexShader);
    glAttachShader(program_, fragmentShader);
    glLinkProgram(program_);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint linked = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) {
        return;
    }

    GLint logLength = 0;
    glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<char> log(static_cast<std::size_t>(logLength > 0 ? logLength : 1));
    glGetProgramInfoLog(program_, logLength, nullptr, log.data());
    glDeleteProgram(program_);
    program_ = 0;
    throw std::runtime_error("OpenGL shader linking failed: " + std::string(log.data()));
}

ShaderProgram::~ShaderProgram()
{
    if (program_ != 0) {
        glDeleteProgram(program_);
    }
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : program_(std::exchange(other.program_, 0))
{
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
    if (this != &other) {
        if (program_ != 0) {
            glDeleteProgram(program_);
        }
        program_ = std::exchange(other.program_, 0);
    }
    return *this;
}

void ShaderProgram::use() const noexcept
{
    glUseProgram(program_);
}

void ShaderProgram::setMatrix4(const char* name, const glm::mat4& value) const
{
    glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderProgram::setVector3(const char* name, const glm::vec3& value) const
{
    glUniform3fv(uniformLocation(name), 1, glm::value_ptr(value));
}

GLint ShaderProgram::uniformLocation(const char* name) const
{
    const GLint location = glGetUniformLocation(program_, name);
    if (location < 0) {
        throw std::runtime_error("Required shader uniform was not found: " + std::string(name));
    }
    return location;
}

} // namespace dentalviz
