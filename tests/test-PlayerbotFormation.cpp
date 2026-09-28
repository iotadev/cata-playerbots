/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "catch2/catch.hpp"
#include "../src/Ai/Base/PlayerbotGroupStrategy.h"
#include <cmath>

TEST_CASE("Playerbot full-party formation has distinct symmetric slots", "[PlayerbotGroup]")
{
    auto first = PlayerbotGroup::PositionForSlot(0, 4);
    auto second = PlayerbotGroup::PositionForSlot(1, 4);
    auto third = PlayerbotGroup::PositionForSlot(2, 4);
    auto fourth = PlayerbotGroup::PositionForSlot(3, 4);

    REQUIRE(first.Angle < second.Angle);
    REQUIRE(second.Angle < third.Angle);
    REQUIRE(third.Angle < fourth.Angle);
    REQUIRE(std::abs((first.Angle + fourth.Angle) / 2.0f - 3.14159265f) < 0.001f);
    REQUIRE(first.Signature != second.Signature);
    REQUIRE(second.Signature != third.Signature);
    REQUIRE(third.Signature != fourth.Signature);
}

TEST_CASE("Solo Playerbot stays directly behind owner", "[PlayerbotGroup]")
{
    auto solo = PlayerbotGroup::PositionForSlot(0, 1);
    REQUIRE(solo.Distance == 2.0f);
    REQUIRE(std::abs(solo.Angle - 3.14159265f) < 0.001f);
}

TEST_CASE("Playerbot far-follow path mode has hysteresis", "[PlayerbotGroup]")
{
    REQUIRE_FALSE(PlayerbotGroup::ShouldPathCatchUp(false, 18.0f));
    REQUIRE(PlayerbotGroup::ShouldPathCatchUp(false, 18.1f));
    REQUIRE(PlayerbotGroup::ShouldPathCatchUp(true, 12.1f));
    REQUIRE_FALSE(PlayerbotGroup::ShouldPathCatchUp(true, 12.0f));
}

TEST_CASE("Playerbot melee stance tracks target aggro", "[PlayerbotGroup]")
{
    REQUIRE(PlayerbotGroup::MeleeChaseAngle(false) > 3.0f);
    REQUIRE(PlayerbotGroup::MeleeChaseAngle(true) == 0.0f);
}
