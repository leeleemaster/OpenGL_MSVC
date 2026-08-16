#include "renderer/ShaderProgram.h"

#include "core/PathText.h"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

std::string readTextFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Shader file could not be opened: " + dentalviz::pathToUtf8(path));
    }

    const std::streampos endPosition = file.tellg();
    if (endPosition < 0) {
        throw std::runtime_error(
            "Shader file size could not be read: " + dentalviz::pathToUtf8(path));
    }

    const auto byteCount = static_cast<std::uintmax_t>(endPosition);
    if (byteCount > std::numeric_limits<std::size_t>::max() ||
        byteCount > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
        throw std::overflow_error(
            "Shader file is too large to load: " + dentalviz::pathToUtf8(path));
    }

    std::string source(static_cast<std::size_t>(byteCount), '\0');
    file.seekg(0, std::ios::beg);
    if (!source.empty() && !file.read(source.data(), static_cast<std::streamsize>(source.size()))) {
        throw std::runtime_error("Shader file could not be read: " + dentalviz::pathToUtf8(path));
    }
    return source;
}

GLuint compileShader(GLenum type, std::string_view source, std::string_view label)
{
    if (source.size() > static_cast<std::size_t>(std::numeric_limits<GLint>::max())) {
        throw std::length_error(
            "OpenGL shader source is too large [" + std::string(label) + "].");
    }
    const GLuint shader = glCreateShader(type);
    if (shader == 0) {
        throw std::runtime_error(
            "OpenGL could not create shader object [" + std::string(label) + "].");
    }
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
    throw std::runtime_error(
        "OpenGL shader compilation failed [" + std::string(label) + "]: " +
        std::string(log.data()));
}

} // namespace

namespace dentalviz {

ShaderProgram::ShaderProgram(std::string_view vertexSource, std::string_view fragmentSource)
    : ShaderProgram(vertexSource, fragmentSource, "inline vertex shader", "inline fragment shader")
{
}

ShaderProgram ShaderProgram::fromFiles(
    const std::filesystem::path& vertexPath,
    const std::filesystem::path& fragmentPath)
{
    const std::string vertexSource = readTextFile(vertexPath);
    const std::string fragmentSource = readTextFile(fragmentPath);
    return ShaderProgram(
        vertexSource,
        fragmentSource,
        pathToUtf8(vertexPath),
        pathToUtf8(fragmentPath));
}

ShaderProgram ShaderProgram::fromVertexFileAndFragmentSource(
    const std::filesystem::path& vertexPath,
    std::string_view fragmentSource,
    std::string_view fragmentLabel)
{
    const std::string vertexSource = readTextFile(vertexPath);
    return ShaderProgram(
        vertexSource,
        fragmentSource,
        pathToUtf8(vertexPath),
        fragmentLabel);
}

ShaderProgram::ShaderProgram(
    std::string_view vertexSource,
    std::string_view fragmentSource,
    std::string_view vertexLabel,
    std::string_view fragmentLabel)
{
    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource, vertexLabel);
    GLuint fragmentShader = 0;
    try {
        fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource, fragmentLabel);
    } catch (...) {
        glDeleteShader(vertexShader);
        throw;
    }

    program_ = glCreateProgram();
    if (program_ == 0) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        throw std::runtime_error("OpenGL could not create a shader program object.");
    }
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

void ShaderProgram::setFloat(const char* name, float value) const
{
    glUniform1f(uniformLocation(name), value);
}

void ShaderProgram::setInteger(const char* name, int value) const
{
    glUniform1i(uniformLocation(name), value);
}

void ShaderProgram::setVector3IfPresent(const char* name, const glm::vec3& value) const noexcept
{
    if (name == nullptr || *name == '\0') {
        return;
    }
    const GLint location = glGetUniformLocation(program_, name);
    if (location >= 0) {
        glUniform3fv(location, 1, glm::value_ptr(value));
    }
}

void ShaderProgram::setFloatIfPresent(const char* name, float value) const noexcept
{
    if (name == nullptr || *name == '\0') {
        return;
    }
    const GLint location = glGetUniformLocation(program_, name);
    if (location >= 0) {
        glUniform1f(location, value);
    }
}

void ShaderProgram::setIntegerIfPresent(const char* name, int value) const noexcept
{
    if (name == nullptr || *name == '\0') {
        return;
    }
    const GLint location = glGetUniformLocation(program_, name);
    if (location >= 0) {
        glUniform1i(location, value);
    }
}

GLint ShaderProgram::uniformLocation(const char* name) const
{
    if (name == nullptr || *name == '\0') {
        throw std::invalid_argument("Required shader uniform name must not be empty.");
    }
    const GLint location = glGetUniformLocation(program_, name);
    if (location < 0) {
        throw std::runtime_error("Required shader uniform was not found: " + std::string(name));
    }
    return location;
}

} // namespace dentalviz
