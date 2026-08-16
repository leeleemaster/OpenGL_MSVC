#include "core/ClickGesture.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("short press and release in viewer produces a click", "[input]")
{
    dentalviz::ClickGesture gesture;

    CHECK_FALSE(gesture.update(true, true, 800.0, 360.0).has_value());
    const std::optional<dentalviz::PointerClick> click =
        gesture.update(false, true, 802.0, 361.0);

    REQUIRE(click.has_value());
    CHECK(click->x == 802.0);
    CHECK(click->y == 361.0);
}

TEST_CASE("movement beyond threshold becomes drag without a click", "[input]")
{
    dentalviz::ClickGesture gesture;

    static_cast<void>(gesture.update(true, true, 800.0, 360.0));
    CHECK_FALSE(gesture.update(true, true, 805.0, 360.0).has_value());
    CHECK(gesture.isDragging());
    CHECK_FALSE(gesture.update(false, true, 805.0, 360.0).has_value());
}

TEST_CASE("interaction starting in properties panel cannot select viewer", "[input]")
{
    dentalviz::ClickGesture gesture;

    static_cast<void>(gesture.update(true, false, 120.0, 200.0));
    CHECK_FALSE(gesture.update(true, true, 500.0, 200.0).has_value());
    CHECK_FALSE(gesture.update(false, true, 500.0, 200.0).has_value());
}

TEST_CASE("release outside viewer cancels click candidate", "[input]")
{
    dentalviz::ClickGesture gesture;

    static_cast<void>(gesture.update(true, true, 800.0, 360.0));
    CHECK_FALSE(gesture.update(false, false, 800.0, 360.0).has_value());
}
