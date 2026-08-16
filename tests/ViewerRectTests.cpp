#include "ui/ViewerUi.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("viewer rectangle separates panel and viewer coordinates", "[viewer-ui]")
{
    dentalviz::ViewerRect rectangle;
    rectangle.windowX = 380.0F;
    rectangle.windowY = 0.0F;
    rectangle.windowWidth = 900.0F;
    rectangle.windowHeight = 720.0F;

    CHECK_FALSE(rectangle.containsWindowPoint(120.0F, 300.0F));
    CHECK(rectangle.containsWindowPoint(380.0F, 0.0F));
    CHECK(rectangle.containsWindowPoint(1279.0F, 719.0F));
    CHECK_FALSE(rectangle.containsWindowPoint(1280.0F, 719.0F));
    CHECK_FALSE(rectangle.containsWindowPoint(500.0F, 720.0F));
}
