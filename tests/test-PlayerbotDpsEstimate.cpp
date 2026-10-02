/* Released under GNU GPL v2 or any later version. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotDpsEstimate.h"
#include <catch2/catch.hpp>
#include <limits>

using namespace PlayerbotDpsEstimate;
TEST_CASE("Playerbot DPS estimate preserves donor level curve without Cata extrapolation", "[playerbot][combat-values]")
{
    REQUIRE(BasicDps(1) == 6.0f);
    REQUIRE(BasicDps(15) == 20.0f);
    REQUIRE(BasicDps(16) == 22.0f);
    REQUIRE(BasicDps(25) == 40.0f);
    REQUIRE(BasicDps(26) == 43.0f);
    REQUIRE(BasicDps(45) == 100.0f);
    REQUIRE(BasicDps(46) == 120.0f);
    REQUIRE(BasicDps(55) == 300.0f);
    REQUIRE(BasicDps(56) == 350.0f);
    REQUIRE(BasicDps(60) == 550.0f);
    REQUIRE(BasicDps(61) == 615.0f);
    REQUIRE(BasicDps(70) == 1200.0f);
    REQUIRE(BasicDps(71) == 1400.0f);
    REQUIRE(BasicDps(80) == 3200.0f);
    REQUIRE(BasicDps(0) == 0.0f);
    REQUIRE(BasicDps(81) == 0.0f);
    REQUIRE(BasicDps(85) == 0.0f);
}
TEST_CASE("Playerbot gear baseline preserves donor quality and integer truncation", "[playerbot][combat-values]")
{
    REQUIRE(QualityMultiplier(ITEM_QUALITY_EPIC) == Approx(1.4641f));
    REQUIRE(QualityMultiplier(99) == 1.0f);
    REQUIRE(BasicGearScore(8) == 14.0f);
    REQUIRE(BasicGearScore(9) == 16.0f);
    REQUIRE(BasicGearScore(15) == 24.0f);
    REQUIRE(BasicGearScore(16) == 27.0f);
    REQUIRE(BasicGearScore(60) == 86.0f);
    REQUIRE(BasicGearScore(61) == 117.0f);
    REQUIRE(BasicGearScore(70) == 153.0f);
    REQUIRE(BasicGearScore(71) == 211.0f);
    REQUIRE(BasicGearScore(80) == 259.0f);
    REQUIRE(BasicGearScore(81) == 0.0f);
}
TEST_CASE("Playerbot group DPS uses donor role weights and gear bounds", "[playerbot][combat-values]")
{
    REQUIRE(Contribution(20, 33, false, false) == Approx(30.0f));
    REQUIRE(Contribution(20, 33, true, false) == Approx(9.0f));
    REQUIRE(Contribution(20, 33, false, true) == Approx(3.0f));
    REQUIRE(Contribution(20, 0, false, false) == Approx(22.5f));
    REQUIRE(Contribution(20, 1000, false, false) == Approx(120.0f));
    REQUIRE(Contribution(81, 400, false, false) == 0.0f);
}
TEST_CASE("Playerbot group DPS retains donor party and raid bonuses", "[playerbot][combat-values]")
{
    REQUIRE(GroupBonus(4) == 1.0f);
    REQUIRE(GroupBonus(5) == Approx(1.05f));
    REQUIRE(GroupBonus(9) == Approx(1.05f));
    REQUIRE(GroupBonus(10) == Approx(1.1f));
    REQUIRE(GroupBonus(24) == Approx(1.1f));
    REQUIRE(GroupBonus(25) == Approx(1.2f));
}
TEST_CASE("Playerbot mixed gear score keeps best slot pairs and native robe offhand mappings", "[playerbot][combat-values]")
{
    GearScores rings;
    rings.Add(INVTYPE_FINGER, 100, ITEM_QUALITY_POOR);
    rings.Add(INVTYPE_FINGER, 80, ITEM_QUALITY_POOR);
    rings.Add(INVTYPE_FINGER, 120, ITEM_QUALITY_POOR);
    REQUIRE(rings.TopTwelve() == 18); // (120+100)/12, not all carried rings
    GearScores armor;
    armor.Add(INVTYPE_CHEST, 60, ITEM_QUALITY_POOR);
    armor.Add(INVTYPE_ROBE, 120, ITEM_QUALITY_POOR);
    armor.Add(INVTYPE_HOLDABLE, 120, ITEM_QUALITY_POOR);
    REQUIRE(armor.TopTwelve() == 20); // robe replaces chest; holdable scores offhand
}
TEST_CASE("Playerbot mixed gear score compares two handed against both weapon slots", "[playerbot][combat-values]")
{
    GearScores gear;
    gear.Add(INVTYPE_2HWEAPON, 120, ITEM_QUALITY_POOR);
    gear.Add(INVTYPE_WEAPONMAINHAND, 80, ITEM_QUALITY_POOR);
    gear.Add(INVTYPE_WEAPONOFFHAND, 60, ITEM_QUALITY_POOR);
    REQUIRE(gear.TopTwelve() == 20);
    gear.Add(INVTYPE_WEAPONMAINHAND, 200, ITEM_QUALITY_POOR);
    gear.Add(INVTYPE_WEAPONOFFHAND, 150, ITEM_QUALITY_POOR);
    REQUIRE(gear.TopTwelve() == 29);
}
TEST_CASE("Playerbot DoT lifetime gate honors eight seconds and rejects unknown estimates", "[playerbot][combat-values]")
{
    REQUIRE(EnoughLifetime(80.0f, 10.0f, 8.0f));
    REQUIRE_FALSE(EnoughLifetime(79.9f, 10.0f, 8.0f));
    REQUIRE_FALSE(EnoughLifetime(100.0f, 0.0f, 8.0f));
    REQUIRE_FALSE(EnoughLifetime(0.0f, 10.0f, 8.0f));
    REQUIRE_FALSE(EnoughLifetime(100.0f, std::numeric_limits<float>::quiet_NaN(), 8.0f));
    REQUIRE_FALSE(EnoughLifetime(std::numeric_limits<float>::infinity(), 10.0f, 0.0f));
    REQUIRE_FALSE(EnoughLifetime(100.0f, 10.0f, -1.0f));
}
