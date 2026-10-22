/* Released under GNU GPL v2 or any later version. See AUTHORS.md. */
#include "../src/Ai/Class/Rogue/PlayerbotRoguePolicy.h"
#include <catch2/catch.hpp>
#include <limits>
using namespace PlayerbotRogue;
TEST_CASE("Rogue combo ownership never transfers across hostile targets", "[playerbot][rogue]")
{
    for (unsigned points = 0; points <= 255; ++points)
    {
        REQUIRE(Combo(false, points) == 0);
        REQUIRE(Combo(true, points) == (points <= 5 ? points : 0));
    }
}
TEST_CASE("Rogue builder conserves energy and stops at combo cap", "[playerbot][rogue]")
{
    REQUIRE_FALSE(BuilderReady(59, 0));
    REQUIRE(BuilderReady(60, 0));
    REQUIRE(BuilderReady(100, 4));
    REQUIRE_FALSE(BuilderReady(100, 5));
    REQUIRE_FALSE(BuilderReady(100, 6));
}
TEST_CASE("Rogue finishers require current target points and bounded lifetime", "[playerbot][rogue]")
{
    REQUIRE_FALSE(FinisherReady(0, 1));
    REQUIRE_FALSE(FinisherReady(1, 1));
    REQUIRE_FALSE(FinisherReady(2, 1));
    REQUIRE(FinisherReady(3, 3));
    REQUIRE_FALSE(FinisherReady(4, 3.1f));
    REQUIRE_FALSE(FinisherReady(4, 0));
    REQUIRE_FALSE(FinisherReady(4, -1));
    REQUIRE_FALSE(FinisherReady(4, std::numeric_limits<float>::quiet_NaN()));
    REQUIRE_FALSE(FinisherReady(4, std::numeric_limits<float>::infinity()));
    REQUIRE(FinisherReady(5, 0));
    REQUIRE_FALSE(FinisherReady(6, 1));
}
TEST_CASE("Rogue Slice and Dice preserves existing aura and requires points", "[playerbot][rogue]")
{
    REQUIRE_FALSE(SliceReady(0, false));
    REQUIRE(SliceReady(1, false));
    REQUIRE_FALSE(SliceReady(5, true));
    REQUIRE_FALSE(SliceReady(6, false));
}
TEST_CASE("Rogue rupture preserves its bleed and yields to near-death damage", "[playerbot][rogue]")
{
    REQUIRE_FALSE(RuptureReady(4, 20, false));
    REQUIRE_FALSE(RuptureReady(5, 3, false));
    REQUIRE(RuptureReady(5, 3.1f, false));
    REQUIRE_FALSE(RuptureReady(5, 20, true));
    REQUIRE(RuptureReady(5, 0, false));
}
