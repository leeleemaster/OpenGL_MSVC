#include "core/RayPicking.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {

dentalviz::Vertex vertex(float x, float y, float z)
{
    return dentalviz::Vertex{
        glm::vec3(x, y, z),
        glm::vec3(0.0F, 0.0F, 1.0F),
    };
}

} // namespace

TEST_CASE("center viewport creates a forward world ray", "[picking]")
{
    const glm::mat4 view = glm::lookAt(
        glm::vec3(0.0F, 0.0F, 5.0F),
        glm::vec3(0.0F),
        glm::vec3(0.0F, 1.0F, 0.0F));
    const glm::mat4 projection = glm::perspective(
        glm::radians(60.0F), 16.0F / 9.0F, 0.1F, 100.0F);

    const dentalviz::Ray ray = dentalviz::makeWorldRayFromViewport(
        0.5F, 0.5F, view, projection);

    CHECK(ray.origin.x == Catch::Approx(0.0F).margin(0.0001F));
    CHECK(ray.origin.y == Catch::Approx(0.0F).margin(0.0001F));
    CHECK(ray.origin.z == Catch::Approx(5.0F).margin(0.0001F));
    CHECK(ray.direction.x == Catch::Approx(0.0F).margin(0.0001F));
    CHECK(ray.direction.y == Catch::Approx(0.0F).margin(0.0001F));
    CHECK(ray.direction.z == Catch::Approx(-1.0F).margin(0.0001F));
}

TEST_CASE("ray intersects and misses axis aligned bounds", "[picking]")
{
    const dentalviz::AxisAlignedBounds bounds{
        glm::vec3(-1.0F),
        glm::vec3(1.0F),
    };
    float entryDistance = 0.0F;
    float exitDistance = 0.0F;

    CHECK(dentalviz::intersectsBounds(
        {{0.0F, 0.0F, 3.0F}, {0.0F, 0.0F, -1.0F}},
        bounds,
        entryDistance,
        exitDistance));
    CHECK(entryDistance == Catch::Approx(2.0F));
    CHECK(exitDistance == Catch::Approx(4.0F));

    CHECK_FALSE(dentalviz::intersectsBounds(
        {{2.0F, 0.0F, 3.0F}, {0.0F, 0.0F, -1.0F}},
        bounds,
        entryDistance,
        exitDistance));
}

TEST_CASE("ray triangle reports a surface hit and rejects miss or behind hit", "[picking]")
{
    const dentalviz::Vertex first = vertex(-1.0F, -1.0F, 0.0F);
    const dentalviz::Vertex second = vertex(1.0F, -1.0F, 0.0F);
    const dentalviz::Vertex third = vertex(0.0F, 1.0F, 0.0F);

    const std::optional<dentalviz::RayHit> hit = dentalviz::intersectTriangle(
        {{0.0F, 0.0F, 2.0F}, {0.0F, 0.0F, -1.0F}},
        first,
        second,
        third,
        7);
    REQUIRE(hit.has_value());
    CHECK(hit->distance == Catch::Approx(2.0F));
    CHECK(hit->position.z == Catch::Approx(0.0F));
    CHECK(hit->normal.z == Catch::Approx(1.0F));
    CHECK(hit->triangleIndex == 7);
    CHECK(hit->barycentric.x + hit->barycentric.y + hit->barycentric.z ==
          Catch::Approx(1.0F));

    CHECK_FALSE(dentalviz::intersectTriangle(
        {{2.0F, 0.0F, 2.0F}, {0.0F, 0.0F, -1.0F}},
        first,
        second,
        third).has_value());
    CHECK_FALSE(dentalviz::intersectTriangle(
        {{0.0F, 0.0F, -1.0F}, {0.0F, 0.0F, -1.0F}},
        first,
        second,
        third).has_value());
}

TEST_CASE("mesh picking returns the closest positive triangle", "[picking]")
{
    dentalviz::MeshData mesh;
    mesh.vertices = {
        vertex(-1.0F, -1.0F, -2.0F),
        vertex(1.0F, -1.0F, -2.0F),
        vertex(0.0F, 1.0F, -2.0F),
        vertex(-1.0F, -1.0F, 0.0F),
        vertex(1.0F, -1.0F, 0.0F),
        vertex(0.0F, 1.0F, 0.0F),
    };
    mesh.indices = {0, 1, 2, 3, 4, 5};

    const std::optional<dentalviz::RayHit> hit = dentalviz::pickMesh(
        {{0.0F, 0.0F, 2.0F}, {0.0F, 0.0F, -1.0F}},
        mesh);

    REQUIRE(hit.has_value());
    CHECK(hit->triangleIndex == 1);
    CHECK(hit->distance == Catch::Approx(2.0F));
    CHECK(hit->position.z == Catch::Approx(0.0F));
}
