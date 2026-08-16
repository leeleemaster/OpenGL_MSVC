#include "renderer/SelectionMarker.h"

#include <array>

namespace dentalviz {

SelectionMarker::SelectionMarker(
    const std::filesystem::path& vertexShaderPath,
    const std::filesystem::path& fragmentShaderPath,
    const std::filesystem::path& lineVertexShaderPath,
    const std::filesystem::path& lineFragmentShaderPath)
    : markerShader_(ShaderProgram::fromFiles(vertexShaderPath, fragmentShaderPath)),
      lineShader_(ShaderProgram::fromFiles(lineVertexShaderPath, lineFragmentShaderPath))
{
    glGenVertexArrays(1, &markerVertexArray_);

    glGenVertexArrays(1, &lineVertexArray_);
    glGenBuffers(1, &lineVertexBuffer_);
    glBindVertexArray(lineVertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, lineVertexBuffer_);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(sizeof(glm::vec3) * 2),
        nullptr,
        GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glBindVertexArray(0);
}

SelectionMarker::~SelectionMarker()
{
    if (lineVertexBuffer_ != 0) {
        glDeleteBuffers(1, &lineVertexBuffer_);
    }
    if (lineVertexArray_ != 0) {
        glDeleteVertexArrays(1, &lineVertexArray_);
    }
    if (markerVertexArray_ != 0) {
        glDeleteVertexArrays(1, &markerVertexArray_);
    }
}

void SelectionMarker::drawMarker(
    const glm::vec3& position,
    const glm::vec3& color,
    const glm::mat4& view,
    const glm::mat4& projection) const
{
    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean programPointSizeWasEnabled = glIsEnabled(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    markerShader_.use();
    markerShader_.setVector3("uPosition", position);
    markerShader_.setVector3("uColor", color);
    markerShader_.setMatrix4("uView", view);
    markerShader_.setMatrix4("uProjection", projection);
    glBindVertexArray(markerVertexArray_);
    glDrawArrays(GL_POINTS, 0, 1);
    glBindVertexArray(0);

    if (programPointSizeWasEnabled == GL_FALSE) {
        glDisable(GL_PROGRAM_POINT_SIZE);
    }
    if (depthTestWasEnabled == GL_TRUE) {
        glEnable(GL_DEPTH_TEST);
    }
}

void SelectionMarker::drawSegment(
    const glm::vec3& pointA,
    const glm::vec3& pointB,
    const glm::vec3& color,
    const glm::mat4& view,
    const glm::mat4& projection) const
{
    const std::array points{pointA, pointB};
    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLfloat previousLineWidth = 1.0F;
    glGetFloatv(GL_LINE_WIDTH, &previousLineWidth);
    glDisable(GL_DEPTH_TEST);

    lineShader_.use();
    lineShader_.setVector3("uColor", color);
    lineShader_.setMatrix4("uView", view);
    lineShader_.setMatrix4("uProjection", projection);
    glBindVertexArray(lineVertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, lineVertexBuffer_);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        static_cast<GLsizeiptr>(sizeof(points)),
        points.data());
    glLineWidth(2.0F);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glLineWidth(previousLineWidth);
    if (depthTestWasEnabled == GL_TRUE) {
        glEnable(GL_DEPTH_TEST);
    }
}

} // namespace dentalviz
