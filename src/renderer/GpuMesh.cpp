#include "renderer/GpuMesh.h"

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace dentalviz {

static_assert(std::is_standard_layout_v<Vertex>);

namespace {

GLsizeiptr checkedBufferSize(
    std::size_t elementCount,
    std::size_t elementSize,
    const char* label)
{
    if (elementCount > std::numeric_limits<std::size_t>::max() / elementSize) {
        throw std::overflow_error(std::string(label) + " byte size overflowed size_t.");
    }
    const std::size_t byteCount = elementCount * elementSize;
    if (byteCount > static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max())) {
        throw std::overflow_error(std::string(label) + " is too large for OpenGL.");
    }
    return static_cast<GLsizeiptr>(byteCount);
}

} // namespace

GpuMesh::GpuMesh(const MeshData& mesh)
{
    if (!mesh.isRenderable()) {
        throw std::invalid_argument("Cannot upload an invalid mesh.");
    }
    if (mesh.indices.size() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())) {
        throw std::overflow_error("Mesh has too many indices for OpenGL draw calls.");
    }
    const GLsizeiptr vertexBufferSize = checkedBufferSize(
        mesh.vertices.size(), sizeof(Vertex), "Vertex buffer");
    const GLsizeiptr indexBufferSize = checkedBufferSize(
        mesh.indices.size(), sizeof(std::uint32_t), "Index buffer");

    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);
    glGenBuffers(1, &indexBuffer_);
    if (vertexArray_ == 0 || vertexBuffer_ == 0 || indexBuffer_ == 0) {
        release();
        throw std::runtime_error("OpenGL could not allocate mesh objects.");
    }

    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertexBufferSize,
        mesh.vertices.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indexBufferSize,
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
    if (vertexArray_ == 0 || indexCount_ == 0) {
        return;
    }
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
