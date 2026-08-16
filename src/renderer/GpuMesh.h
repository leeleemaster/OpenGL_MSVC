#pragma once

#include "core/MeshData.h"

#include <glad/glad.h>

namespace dentalviz {

class GpuMesh final {
public:
    explicit GpuMesh(const MeshData& mesh);
    ~GpuMesh();

    GpuMesh(const GpuMesh&) = delete;
    GpuMesh& operator=(const GpuMesh&) = delete;
    GpuMesh(GpuMesh&& other) noexcept;
    GpuMesh& operator=(GpuMesh&& other) noexcept;

    void draw() const noexcept;

private:
    void release() noexcept;

    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
    GLuint indexBuffer_ = 0;
    GLsizei indexCount_ = 0;
};

} // namespace dentalviz
