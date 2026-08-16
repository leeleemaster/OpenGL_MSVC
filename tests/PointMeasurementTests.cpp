#include "core/PointMeasurement.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace {

dentalviz::RayHit hitAt(float x, float y, float z, std::size_t triangleIndex)
{
    dentalviz::RayHit hit;
    hit.position = glm::vec3(x, y, z);
    hit.normal = glm::vec3(0.0F, 0.0F, 1.0F);
    hit.triangleIndex = triangleIndex;
    return hit;
}

} // namespace

TEST_CASE("two selected points produce a 3D straight-line distance", "[measurement]")
{
    dentalviz::PointMeasurement measurement;

    CHECK(measurement.select(hitAt(1.0F, 2.0F, 3.0F, 10)) ==
          dentalviz::MeasurementUpdate::pointASet);
    CHECK_FALSE(measurement.distance().has_value());
    CHECK(measurement.select(hitAt(4.0F, 6.0F, 3.0F, 20)) ==
          dentalviz::MeasurementUpdate::pointBSet);

    REQUIRE(measurement.distance().has_value());
    CHECK(measurement.distance().value() == Catch::Approx(5.0F));
    REQUIRE(measurement.pointA().has_value());
    REQUIRE(measurement.pointB().has_value());
    CHECK(measurement.pointA()->triangleIndex == 10);
    CHECK(measurement.pointB()->triangleIndex == 20);
}

TEST_CASE("third selected point consistently restarts with point A", "[measurement]")
{
    dentalviz::PointMeasurement measurement;
    static_cast<void>(measurement.select(hitAt(0.0F, 0.0F, 0.0F, 1)));
    static_cast<void>(measurement.select(hitAt(1.0F, 0.0F, 0.0F, 2)));

    CHECK(measurement.select(hitAt(2.0F, 0.0F, 0.0F, 3)) ==
          dentalviz::MeasurementUpdate::restartedWithPointA);

    REQUIRE(measurement.pointA().has_value());
    CHECK(measurement.pointA()->triangleIndex == 3);
    CHECK_FALSE(measurement.pointB().has_value());
    CHECK_FALSE(measurement.distance().has_value());
}

TEST_CASE("measurement reset clears both points and distance", "[measurement]")
{
    dentalviz::PointMeasurement measurement;
    static_cast<void>(measurement.select(hitAt(0.0F, 0.0F, 0.0F, 1)));
    static_cast<void>(measurement.select(hitAt(0.0F, 0.0F, 0.0F, 2)));
    REQUIRE(measurement.distance().has_value());
    CHECK(measurement.distance().value() == Catch::Approx(0.0F));

    measurement.reset();

    CHECK_FALSE(measurement.pointA().has_value());
    CHECK_FALSE(measurement.pointB().has_value());
    CHECK_FALSE(measurement.distance().has_value());
}
