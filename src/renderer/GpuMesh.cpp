#include "renderer/GpuMesh.h"

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace dentalviz {

static_assert(std::is_standard_layout_v<Vertex>);

GpuMesh::GpuMesh(const MeshData& mesh)
{
    if (!mesh.hasValidIndices()) {
        throw std::invalid_argument("Cannot upload an invalid mesh.");
    }
    if (mesh.indices.size() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())) {
        throw std::overflow_error("Mesh has too many indices for OpenGL draw calls.");
    }

    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);
    glGenBuffers(1, &indexBuffer_);

    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex)),
        mesh.vertices.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(std::uint32_t)),
        mesh.indices.data(),
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(Vertex)),
        reinterpret_cast<const void*>(offsetof(Vertex, position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        static_cast<GLsizei>(sizeof(Vertex)),
        reinterpret_cast<const void*>(offsetof(Vertex, normal)));

    glBindVertexArray(0);
    indexCount_ = static_cast<GLsizei>(mesh.indices.size());
}

GpuMesh::~GpuMesh()
{
    release();
}

GpuMesh::GpuMesh(GpuMesh&& other) noexcept
    : vertexArray_(std::exchange(other.vertexArray_, 0)),
      vertexBuffer_(std::exchange(other.vertexBuffer_, 0)),
      indexBuffer_(std::exchange(other.indexBuffer_, 0)),
      indexCount_(std::exchange(other.indexCount_, 0))
{
}

GpuMesh& GpuMesh::operator=(GpuMesh&& other) noexcept
{
    if (this != &other) {
        release();
        vertexArray_ = std::exchange(other.vertexArray_, 0);
        vertexBuffer_ = std::exchange(other.vertexBuffer_, 0);
        indexBuffer_ = std::exchange(other.indexBuffer_, 0);
        indexCount_ = std::exchange(other.indexCount_, 0);
    }
    return *this;
}

void GpuMesh::draw() const noexcept
{
    glBindVertexArray(vertexArray_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void GpuMesh::release() noexcept
{
    if (indexBuffer_ != 0) {
        glDeleteBuffers(1, &indexBuffer_);
    }
    if (vertexBuffer_ != 0) {
        glDeleteBuffers(1, &vertexBuffer_);
    }
    if (vertexArray_ != 0) {
        glDeleteVertexArrays(1, &vertexArray_);
    }
    vertexArray_ = 0;
    vertexBuffer_ = 0;
    indexBuffer_ = 0;
    indexCount_ = 0;
}

} // namespace dentalviz
