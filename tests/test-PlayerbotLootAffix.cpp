/* GPL v2 or later. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotLootAffix.h"
#include "../src/Ai/Base/PlayerbotEquipment.h"
#include <catch2/catch.hpp>
using namespace PlayerbotEquipment;
namespace
{
PlayerbotLootRoll RollFacts()
{
    PlayerbotLootRoll roll;
    roll.Group = 1; roll.Roll = 2; roll.Entry = 100; roll.Count = 1;
    return roll;
}
}
TEST_CASE("Playerbot native property metadata maps only to positive donor affix identity", "[playerbot][loot]")
{
    auto roll = RollFacts(); roll.Property = 7;
    auto query = LootQuery(roll, true, false, 0);
    REQUIRE(query); REQUIRE(query.Item == 100); REQUIRE(query.Property == 7);
    roll.PropertyType = 1; REQUIRE_FALSE(LootQuery(roll, true, false, 0));
    roll.PropertyType = 0; roll.SuffixFactor = 10; REQUIRE_FALSE(LootQuery(roll, true, false, 0));
    roll.SuffixFactor = 0; roll.Property = UINT32_MAX; REQUIRE_FALSE(LootQuery(roll, true, false, 0));
}
TEST_CASE("Playerbot suffix metadata requires matching native scaling and negative query identity", "[playerbot][loot]")
{
    auto roll = RollFacts(); roll.PropertyType = 1; roll.Property = 9; roll.SuffixFactor = 500;
    auto query = LootQuery(roll, false, true, 500);
    REQUIRE(query); REQUIRE(query.Property == -9);
    REQUIRE_FALSE(LootQuery(roll, false, true, 501));
    REQUIRE_FALSE(LootQuery(roll, false, true, 0));
    roll.PropertyType = 0; REQUIRE_FALSE(LootQuery(roll, false, true, 500));
    roll.PropertyType = 1; roll.Property = 0; REQUIRE_FALSE(LootQuery(roll, false, true, 500));
}
TEST_CASE("Playerbot affix proof rejects absent native identities and incompatible template modes", "[playerbot][loot]")
{
    auto roll = RollFacts(); REQUIRE(LootQuery(roll, false, false, 0));
    REQUIRE_FALSE(LootQuery(roll, true, false, 0));
    REQUIRE_FALSE(LootQuery(roll, true, true, 0));
    roll.PropertyType = 255; REQUIRE_FALSE(LootQuery(roll, false, false, 0));
    roll = RollFacts(); roll.Property = 1; REQUIRE_FALSE(LootQuery(roll, false, false, 0));
    roll = RollFacts(); roll.Group = 0; REQUIRE_FALSE(LootQuery(roll, false, false, 0));
    roll = RollFacts(); roll.Roll = 0; REQUIRE_FALSE(LootQuery(roll, false, false, 0));
    roll = RollFacts(); roll.Count = 0; REQUIRE_FALSE(LootQuery(roll, false, false, 0));
}
TEST_CASE("Playerbot same item entry permits upgrades between distinct affix variants", "[playerbot][inventory]")
{
    REQUIRE(SameVariant(100, 3, 0, 100, 3, 0));
    REQUIRE_FALSE(SameVariant(100, 3, 0, 100, 4, 0));
    REQUIRE_FALSE(SameVariant(100, -3, 500, 100, -3, 501));
    REQUIRE(Evaluate(120, 100, false, SameVariant(100, 3, 0, 100, 4, 0), false, false, true) == Decision::Upgrade);
    REQUIRE(Evaluate(120, 100, false, SameVariant(100, 3, 0, 100, 3, 0), false, false, true) == Decision::Keep);
}
