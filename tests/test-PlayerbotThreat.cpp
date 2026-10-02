/* Released under GNU GPL v2 or any later version. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotThreatStrategy.h"
#include <catch2/catch.hpp>
#include <limits>

TEST_CASE("Playerbot threat percent keeps donor tank ratio and integer boundaries", "[playerbot][threat]")
{
    REQUIRE(PlayerbotThreat::Percent(79.99f, 100.0f, true, true, false) == 79);
    REQUIRE(PlayerbotThreat::Percent(80.0f, 100.0f, true, true, false) == 80);
    REQUIRE(PlayerbotThreat::Percent(50.0f, 100.0f, true, true, false) == 50);
    REQUIRE(PlayerbotThreat::Percent(100.0f, 100.0f, false, true, false) == 0);
}
TEST_CASE("Playerbot threat percent handles fleeing startup zero and saturation safely", "[playerbot][threat]")
{
    REQUIRE(PlayerbotThreat::Percent(0.0f, 0.0f, true, true, false) == 100);
    REQUIRE(PlayerbotThreat::Percent(0.0f, 0.0f, true, false, false) == 0);
    REQUIRE(PlayerbotThreat::Percent(1.0f, 0.0f, true, true, false) == 255);
    REQUIRE(PlayerbotThreat::Percent(300.0f, 100.0f, true, true, false) == 255);
    REQUIRE(PlayerbotThreat::Percent(100.0f, 0.0f, true, true, true) == 0);
    REQUIRE(PlayerbotThreat::Percent(std::numeric_limits<float>::quiet_NaN(), 100.0f, true, true, false) == 100);
}
TEST_CASE("Playerbot threat multiplier blocks single target damage at donor cutoff only", "[playerbot][threat]")
{
    using Type = Action::ActionThreatType;
    REQUIRE(PlayerbotThreat::DamageMultiplier(Type::Single, true, 79) == 1.0f);
    REQUIRE(PlayerbotThreat::DamageMultiplier(Type::Single, true, 80) == 0.0f);
    REQUIRE(PlayerbotThreat::DamageMultiplier(Type::Single, true, 255) == 0.0f);
    REQUIRE(PlayerbotThreat::DamageMultiplier(Type::Single, false, 255) == 1.0f);
    REQUIRE(PlayerbotThreat::DamageMultiplier(Type::None, true, 255) == 1.0f);
    REQUIRE(PlayerbotThreat::DamageMultiplier(Type::Aoe, true, 255) == 1.0f); // explicitly unported
    REQUIRE(PlayerbotThreat::DamageMultiplier(Type::Single, true, 255, true) == 1.0f);
}
TEST_CASE("Playerbot neglect threat value preserves donor one shot consumption", "[playerbot][threat]")
{
    PlayerbotThreat::NeglectThreatResetValue value(nullptr);
    REQUIRE_FALSE(value.Get());
    value.Set(true);
    REQUIRE(value.Get());
    REQUIRE_FALSE(value.Get());
    value.Set(true);
    value.Reset();
    REQUIRE_FALSE(value.Get());
}
