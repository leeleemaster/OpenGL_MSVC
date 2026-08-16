#include "io/MeshLoader.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <glm/geometric.hpp>

#include <filesystem>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace {

std::filesystem::path fixturePath(const char* name)
{
    return std::filesystem::path(DENTALVIZ_TEST_FIXTURE_DIR) / name;
}

class TemporaryFileCleanup final {
public:
    explicit TemporaryFileCleanup(std::filesystem::path path)
        : path_(std::move(path))
    {
    }

    ~TemporaryFileCleanup()
    {
        std::error_code ignoredError;
        std::filesystem::remove(path_, ignoredError);
    }

    TemporaryFileCleanup(const TemporaryFileCleanup&) = delete;
    TemporaryFileCleanup& operator=(const TemporaryFileCleanup&) = delete;

private:
    std::filesystem::path path_;
};

} // namespace

TEST_CASE("Assimp loads an ASCII STL into common mesh data", "[mesh-loader]")
{
    const dentalviz::MeshLoadResult result =
        dentalviz::MeshLoader::load(fixturePath("tetrahedron.stl"));

    CHECK(result.mesh.hasValidIndices());
    CHECK(result.mesh.indices.size() == 12);
    CHECK(result.sourceMeshCount >= 1);
    CHECK(result.loadDuration.count() >= 0);

    const dentalviz::AxisAlignedBounds bounds = result.mesh.bounds();
    CHECK(bounds.minimum.x == Catch::Approx(-1.0F));
    CHECK(bounds.maximum.x == Catch::Approx(1.0F));
    CHECK(bounds.minimum.y == Catch::Approx(-1.0F));
    CHECK(bounds.maximum.y == Catch::Approx(1.0F));

    for (const dentalviz::Vertex& vertex : result.mesh.vertices) {
        CHECK(glm::length(vertex.normal) == Catch::Approx(1.0F).margin(0.0001F));
    }
}

TEST_CASE("mesh loader reports a missing file without terminating", "[mesh-loader]")
{
    CHECK_THROWS_WITH(
        dentalviz::MeshLoader::load(fixturePath("missing.stl")),
        Catch::Matchers::ContainsSubstring("does not exist"));
}

TEST_CASE("Assimp generates normals when an OBJ omits them", "[mesh-loader]")
{
    const dentalviz::MeshLoadResult result =
        dentalviz::MeshLoader::load(fixturePath("triangle-no-normals.obj"));

    REQUIRE(result.mesh.hasValidIndices());
    REQUIRE(result.mesh.indices.size() == 3);
    for (const dentalviz::Vertex& vertex : result.mesh.vertices) {
        CHECK(glm::length(vertex.normal) == Catch::Approx(1.0F).margin(0.0001F));
    }
}

TEST_CASE("mesh loader accepts a Unicode Windows path", "[mesh-loader]")
{
    const std::filesystem::path unicodePath =
        std::filesystem::temp_directory_path() / L"DentalViz-한글-경로.stl";
    TemporaryFileCleanup cleanup(unicodePath);
    std::filesystem::copy_file(
        fixturePath("tetrahedron.stl"),
        unicodePath,
        std::filesystem::copy_options::overwrite_existing);

    const dentalviz::MeshLoadResult result = dentalviz::MeshLoader::load(unicodePath);
    CHECK(result.mesh.hasValidIndices());
    CHECK(result.mesh.indices.size() == 12);
}

TEST_CASE("mesh loader reports invalid mesh contents", "[mesh-loader]")
{
    CHECK_THROWS_AS(
        dentalviz::MeshLoader::load(fixturePath("invalid.stl")),
        std::runtime_error);
}
