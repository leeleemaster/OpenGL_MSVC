#include "io/MeshLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec4.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

glm::mat4 toGlmMatrix(const aiMatrix4x4& matrix) noexcept
{
    return glm::mat4(
        matrix.a1, matrix.b1, matrix.c1, matrix.d1,
        matrix.a2, matrix.b2, matrix.c2, matrix.d2,
        matrix.a3, matrix.b3, matrix.c3, matrix.d3,
        matrix.a4, matrix.b4, matrix.c4, matrix.d4);
}

void appendMesh(
    const aiMesh& source,
    const glm::mat4& transform,
    dentalviz::MeshData& destination)
{
    if (!source.HasPositions() || source.mNumVertices == 0) {
        return;
    }
    if (!source.HasNormals()) {
        throw std::runtime_error("Assimp did not provide normals for a loaded mesh.");
    }

    const std::size_t currentVertexCount = destination.vertices.size();
    const std::size_t addedVertexCount = static_cast<std::size_t>(source.mNumVertices);
    const std::size_t maximumIndex = std::numeric_limits<std::uint32_t>::max();
    if (currentVertexCount > maximumIndex || addedVertexCount > maximumIndex - currentVertexCount) {
        throw std::overflow_error("Loaded mesh exceeds the 32-bit index limit.");
    }

    const std::uint32_t baseVertex = static_cast<std::uint32_t>(currentVertexCount);
    const glm::mat3 linearTransform(transform);
    if (std::abs(glm::determinant(linearTransform)) <= std::numeric_limits<float>::epsilon()) {
        throw std::runtime_error("Loaded mesh contains a non-invertible node transform.");
    }
    const glm::mat3 normalMatrix = glm::transpose(glm::inverse(linearTransform));
    destination.vertices.reserve(currentVertexCount + addedVertexCount);

    for (unsigned int index = 0; index < source.mNumVertices; ++index) {
        const aiVector3D& sourcePosition = source.mVertices[index];
        const aiVector3D& sourceNormal = source.mNormals[index];
        const glm::vec4 transformedPosition = transform * glm::vec4(
            sourcePosition.x,
            sourcePosition.y,
            sourcePosition.z,
            1.0F);
        glm::vec3 transformedNormal = normalMatrix * glm::vec3(
            sourceNormal.x,
            sourceNormal.y,
            sourceNormal.z);
        const float squaredNormalLength = glm::dot(transformedNormal, transformedNormal);
        if (squaredNormalLength > std::numeric_limits<float>::epsilon()) {
            transformedNormal = glm::normalize(transformedNormal);
        } else {
            transformedNormal = glm::vec3(0.0F, 1.0F, 0.0F);
        }

        destination.vertices.push_back({
            glm::vec3(transformedPosition),
            transformedNormal,
        });
    }

    for (unsigned int faceIndex = 0; faceIndex < source.mNumFaces; ++faceIndex) {
        const aiFace& face = source.mFaces[faceIndex];
        if (face.mNumIndices != 3) {
            continue;
        }
        if (face.mIndices[0] >= source.mNumVertices ||
            face.mIndices[1] >= source.mNumVertices ||
            face.mIndices[2] >= source.mNumVertices) {
            throw std::runtime_error("Assimp mesh contains an out-of-range vertex index.");
        }
        destination.indices.insert(destination.indices.end(), {
            baseVertex + face.mIndices[0],
            baseVertex + face.mIndices[1],
            baseVertex + face.mIndices[2],
        });
    }
}

void appendNode(
    const aiScene& scene,
    const aiNode& node,
    const glm::mat4& parentTransform,
    dentalviz::MeshData& destination)
{
    const glm::mat4 transform = parentTransform * toGlmMatrix(node.mTransformation);
    for (unsigned int index = 0; index < node.mNumMeshes; ++index) {
        const unsigned int meshIndex = node.mMeshes[index];
        if (meshIndex >= scene.mNumMeshes || scene.mMeshes[meshIndex] == nullptr) {
            throw std::runtime_error("Assimp scene contains an invalid mesh reference.");
        }
        appendMesh(*scene.mMeshes[meshIndex], transform, destination);
    }

    for (unsigned int index = 0; index < node.mNumChildren; ++index) {
        if (node.mChildren[index] == nullptr) {
            throw std::runtime_error("Assimp scene contains an invalid child node.");
        }
        appendNode(scene, *node.mChildren[index], transform, destination);
    }
}

} // namespace

namespace dentalviz {

MeshLoadResult MeshLoader::load(const std::filesystem::path& path)
{
    if (path.empty()) {
        throw std::invalid_argument("Mesh path must not be empty.");
    }
    if (!std::filesystem::is_regular_file(path)) {
        throw std::runtime_error("Mesh file does not exist: " + path.string());
    }

    const auto startTime = std::chrono::steady_clock::now();
    Assimp::Importer importer;
    constexpr unsigned int importFlags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_ImproveCacheLocality |
        aiProcess_SortByPType |
        aiProcess_ValidateDataStructure;

    const aiScene* scene = importer.ReadFile(path.string(), importFlags);
    if (scene == nullptr || scene->mRootNode == nullptr || scene->mNumMeshes == 0) {
        const std::string details = importer.GetErrorString();
        throw std::runtime_error(
            "Assimp could not load mesh file '" + path.string() + "': " +
            (details.empty() ? "unknown import error" : details));
    }

    MeshLoadResult result;
    result.sourcePath = std::filesystem::absolute(path).lexically_normal();
    result.sourceMeshCount = static_cast<std::size_t>(scene->mNumMeshes);
    appendNode(*scene, *scene->mRootNode, glm::mat4(1.0F), result.mesh);
    if (!result.mesh.hasValidIndices()) {
        throw std::runtime_error("Loaded file did not contain a valid triangle mesh: " + path.string());
    }

    result.loadDuration = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - startTime);
    return result;
}

} // namespace dentalviz
