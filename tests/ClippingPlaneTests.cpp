#include "core/ClippingPlane.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <stdexcept>

namespace {

const dentalviz::AxisAlignedBounds bounds{
    glm::vec3(-2.0F, -3.0F, -4.0F),
    glm::vec3(6.0F, 7.0F, 8.0F)};

} // namespace

TEST_CASE("clipping plane resets through the model center", "[clipping]")
{
    dentalviz::ClippingPlane plane;
    plane.reset(bounds);

    CHECK(plane.axis() == dentalviz::ClipAxis::x);
    CHECK(plane.normal() == glm::vec3(1.0F, 0.0F, 0.0F));
    CHECK(plane.distance() == Catch::Approx(-2.0F));
    const auto [minimumDistance, maximumDistance] = plane.distanceRange(bounds);
    CHECK(minimumDistance == Catch::Approx(-6.0F));
    CHECK(maximumDistance == Catch::Approx(2.0F));
}

TEST_CASE("axis selection updates the model-space normal and range", "[clipping]")
{
    dentalviz::ClippingPlane plane;
    plane.setAxis(dentalviz::ClipAxis::z, bounds);

    CHECK(plane.normal() == glm::vec3(0.0F, 0.0F, 1.0F));
    CHECK(plane.distance() == Catch::Approx(-2.0F));
    const auto [minimumDistance, maximumDistance] = plane.distanceRange(bounds);
    CHECK(minimumDistance == Catch::Approx(-8.0F));
    CHECK(maximumDistance == Catch::Approx(4.0F));
}

TEST_CASE("distance is clamped to the selected model bounds", "[clipping]")
{
    dentalviz::ClippingPlane plane;
    plane.reset(bounds);

    plane.setDistance(-100.0F, bounds);
    CHECK(plane.distance() == Catch::Approx(-6.0F));
    plane.setDistance(100.0F, bounds);
    CHECK(plane.distance() == Catch::Approx(2.0F));
    CHECK_THROWS_AS(
        plane.setDistance(std::numeric_limits<float>::infinity(), bounds),
        std::invalid_argument);
}

TEST_CASE("only the positive half-space is discarded when enabled", "[clipping]")
{
    dentalviz::ClippingPlane plane;
    plane.reset(bounds);
    plane.setDistance(-1.0F, bounds);

    CHECK_FALSE(plane.discards(glm::vec3(5.0F, 0.0F, 0.0F)));
    plane.setEnabled(true);
    CHECK_FALSE(plane.discards(glm::vec3(0.0F, 100.0F, -100.0F)));
    CHECK_FALSE(plane.discards(glm::vec3(1.0F, 100.0F, -100.0F)));
    CHECK(plane.discards(glm::vec3(1.01F, -100.0F, 100.0F)));
}
