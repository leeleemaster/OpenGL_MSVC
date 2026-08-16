#include "core/OrbitCamera.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include <cmath>
#include <limits>
#include <stdexcept>

namespace {

const dentalviz::AxisAlignedBounds testBounds{
    glm::vec3(-1.0F, -2.0F, -0.5F),
    glm::vec3(3.0F, 2.0F, 1.5F),
};

} // namespace

TEST_CASE("camera fit centers and frames bounds", "[camera]")
{
    dentalviz::OrbitCamera camera;
    camera.fit(testBounds, 16.0F / 9.0F);

    CHECK(camera.target().x == Catch::Approx(1.0F));
    CHECK(camera.target().y == Catch::Approx(0.0F));
    CHECK(camera.target().z == Catch::Approx(0.5F));
    CHECK(camera.distance() > 0.5F * glm::length(testBounds.size()));
    CHECK(glm::length(camera.position() - camera.target()) ==
          Catch::Approx(camera.distance()).margin(0.0001F));
}

TEST_CASE("camera orbit preserves distance and clamps pitch", "[camera]")
{
    dentalviz::OrbitCamera camera;
    camera.fit(testBounds, 1.0F);
    const float fittedDistance = camera.distance();

    camera.orbit(100.0F, -100'000.0F);

    CHECK(camera.distance() == Catch::Approx(fittedDistance));
    CHECK(camera.yawRadians() == Catch::Approx(-0.45F));
    CHECK(camera.pitchRadians() < glm::half_pi<float>());
    CHECK(camera.pitchRadians() > 0.0F);
}

TEST_CASE("positive wheel zooms in and remains bounded", "[camera]")
{
    dentalviz::OrbitCamera camera;
    camera.fit(testBounds, 1.0F);
    const float fittedDistance = camera.distance();

    camera.zoom(1.0F);
    CHECK(camera.distance() < fittedDistance);

    camera.zoom(100'000.0F);
    CHECK(camera.distance() > 0.0F);
    const float closestDistance = camera.distance();
    camera.zoom(1.0F);
    CHECK(camera.distance() == Catch::Approx(closestDistance));
}

TEST_CASE("camera pan moves target without changing orbit distance", "[camera]")
{
    dentalviz::OrbitCamera camera;
    camera.fit(testBounds, 16.0F / 9.0F);
    const glm::vec3 originalTarget = camera.target();
    const float originalDistance = camera.distance();

    camera.pan(80.0F, -40.0F, 720.0F);

    CHECK(glm::length(camera.target() - originalTarget) > 0.0F);
    CHECK(camera.distance() == Catch::Approx(originalDistance));
}

TEST_CASE("camera rejects invalid projection inputs", "[camera]")
{
    CHECK_THROWS_AS(dentalviz::OrbitCamera(180.0F), std::invalid_argument);

    dentalviz::OrbitCamera camera;
    CHECK_THROWS_AS(camera.fit(testBounds, 0.0F), std::invalid_argument);
    CHECK_THROWS_AS(
        camera.projectionMatrix(std::numeric_limits<float>::infinity()),
        std::invalid_argument);
}
