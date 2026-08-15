#include "core/BuildInfo.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("project metadata is available", "[smoke]")
{
    CHECK(dentalviz::projectName() == "DentalViz");
    CHECK_FALSE(dentalviz::projectVersion().empty());
    CHECK(dentalviz::projectVersion() != "unknown");
}
