/* Released under GNU GPL v2 or any later version. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotCombatBalance.h"
#include "../src/Ai/Class/Priest/PlayerbotPriestDpsStrategy.h"
#include <catch2/catch.hpp>
#include <limits>

TEST_CASE("Playerbot balance preserves donor creature rank weights", "[playerbot][balance]")
{
    using PlayerbotCombatBalance::EnemyWeight;
    REQUIRE(EnemyWeight(20, CREATURE_ELITE_NORMAL) == 20);
    REQUIRE(EnemyWeight(20, CREATURE_ELITE_RARE) == 40);
    REQUIRE(EnemyWeight(20, CREATURE_ELITE_ELITE) == 60);
    REQUIRE(EnemyWeight(20, CREATURE_ELITE_RAREELITE) == 60);
    REQUIRE(EnemyWeight(20, CREATURE_ELITE_WORLDBOSS) == 400);
}
TEST_CASE("Playerbot balance preserves roster denominator ten member cap and saturation", "[playerbot][balance]")
{
    using PlayerbotCombatBalance::Percent;
    REQUIRE(Percent(100, 5, 200) == 50);
    REQUIRE(Percent(100, 5, 100) == 100);
    REQUIRE(Percent(100, 5, 20) == 200);
    REQUIRE(Percent(500, 25, 200) == 100);
    REQUIRE(Percent(80, 5, 100) == 80); // dead/offline slots retain donor denominator
    REQUIRE(Percent(0, 0, 20) == 0); // no solo party numerator
    REQUIRE(Percent(0, 0, 0) == 100);
    REQUIRE(Percent(79.99f, 5, 100) == 79);
    REQUIRE(Percent(std::numeric_limits<float>::quiet_NaN(), 5, 100) == 0);
}
TEST_CASE("Playerbot healer damage uses donor balance mana boundaries and retains control gates", "[playerbot][balance]")
{
    using PlayerbotPriestDps::CanAttack;
    REQUIRE_FALSE(CanAttack(true, true, true, false, 84.99f, 50));
    REQUIRE(CanAttack(true, true, true, false, 85, 50));
    REQUIRE_FALSE(CanAttack(true, true, true, false, 64.99f, 51));
    REQUIRE(CanAttack(true, true, true, false, 65, 51));
    REQUIRE(CanAttack(true, true, true, false, 65, 100));
    REQUIRE_FALSE(CanAttack(true, true, true, false, 39.99f, 101));
    REQUIRE(CanAttack(true, true, true, false, 40, 101));
    REQUIRE_FALSE(CanAttack(true, true, true, true, 100, 200));
    REQUIRE_FALSE(CanAttack(true, true, false, false, 100, 200));
    REQUIRE_FALSE(CanAttack(false, true, true, false, 100, 200));
    REQUIRE_FALSE(CanAttack(true, false, true, false, 100, 200));
}
