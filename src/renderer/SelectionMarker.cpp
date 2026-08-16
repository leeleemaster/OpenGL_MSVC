#include "renderer/SelectionMarker.h"

namespace dentalviz {

SelectionMarker::SelectionMarker(
    const std::filesystem::path& vertexShaderPath,
    const std::filesystem::path& fragmentShaderPath)
    : shader_(ShaderProgram::fromFiles(vertexShaderPath, fragmentShaderPath))
{
    glGenVertexArrays(1, &vertexArray_);
}

SelectionMarker::~SelectionMarker()
{
    if (vertexArray_ != 0) {
        glDeleteVertexArrays(1, &vertexArray_);
    }
}

void SelectionMarker::draw(
    const glm::vec3& position,
    const glm::mat4& view,
    const glm::mat4& projection) const
{
    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean programPointSizeWasEnabled = glIsEnabled(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    shader_.use();
    shader_.setVector3("uPosition", position);
    shader_.setMatrix4("uView", view);
    shader_.setMatrix4("uProjection", projection);
    glBindVertexArray(vertexArray_);
    glDrawArrays(GL_POINTS, 0, 1);
    glBindVertexArray(0);

    if (programPointSizeWasEnabled == GL_FALSE) {
        glDisable(GL_PROGRAM_POINT_SIZE);
    }
    if (depthTestWasEnabled == GL_TRUE) {
        glEnable(GL_DEPTH_TEST);
    }
}

} // namespace dentalviz
