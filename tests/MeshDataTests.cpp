#include "core/MeshData.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <glm/geometric.hpp>

#include <cmath>
#include <limits>
#include <stdexcept>

TEST_CASE("procedural tooth is a valid indexed mesh", "[mesh]")
{
    const dentalviz::MeshData mesh = dentalviz::makeProceduralTooth(32);

    CHECK(mesh.vertices.size() == 290);
    CHECK(mesh.indices.size() == 1'728);
    CHECK(mesh.hasValidIndices());
}

TEST_CASE("procedural tooth has finite unit normals", "[mesh]")
{
    const dentalviz::MeshData mesh = dentalviz::makeProceduralTooth(32);

    for (const dentalviz::Vertex& vertex : mesh.vertices) {
        CHECK(std::isfinite(vertex.normal.x));
        CHECK(std::isfinite(vertex.normal.y));
        CHECK(std::isfinite(vertex.normal.z));
        CHECK(glm::length(vertex.normal) == Catch::Approx(1.0F).margin(0.0001F));
    }
}

TEST_CASE("procedural tooth bounds retain model scale", "[mesh]")
{
    const dentalviz::MeshData mesh = dentalviz::makeProceduralTooth();
    const dentalviz::AxisAlignedBounds bounds = mesh.bounds();

    CHECK(bounds.minimum.y == Catch::Approx(-1.48F));
    CHECK(bounds.maximum.y == Catch::Approx(1.19F));
    CHECK(bounds.size().x > 1.4F);
    CHECK(bounds.size().z > 1.7F);
    CHECK(glm::length(bounds.center()) < 0.2F);
}

TEST_CASE("procedural tooth rejects insufficient radial detail", "[mesh]")
{
    CHECK_THROWS_AS(dentalviz::makeProceduralTooth(7), std::invalid_argument);
}

TEST_CASE("renderable mesh rejects non-finite vertex data", "[mesh][invalid-input]")
{
    dentalviz::MeshData mesh;
    mesh.vertices = {
        {{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}},
        {{1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}},
        {{0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 1.0F}},
    };
    mesh.indices = {0, 1, 2};
    REQUIRE(mesh.isRenderable());

    mesh.vertices[1].position.x = std::numeric_limits<float>::quiet_NaN();
    CHECK_FALSE(mesh.hasFiniteVertexData());
    CHECK_FALSE(mesh.isRenderable());
    CHECK_THROWS_AS(mesh.bounds(), std::invalid_argument);

    mesh.vertices[1].position.x = 1.0F;
    mesh.vertices[2].normal.z = std::numeric_limits<float>::infinity();
    CHECK_FALSE(mesh.hasFiniteVertexData());
    CHECK_FALSE(mesh.isRenderable());
}

TEST_CASE("procedural tooth rejects index-space overflow before allocation", "[mesh][invalid-input]")
{
    CHECK_THROWS_AS(
        dentalviz::makeProceduralTooth(std::numeric_limits<std::uint32_t>::max()),
        std::overflow_error);
}
