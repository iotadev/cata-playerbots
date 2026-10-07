/* GPL v2 or later. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotRoles.h"
#include <catch2/catch.hpp>
using namespace PlayerbotRoles;

TEST_CASE("Playerbot main tank selection preserves explicit assignment over living fallback", "[playerbot][roles]")
{
    uint64 tank = 10, assigned = 20;
    MainTankSelection<uint64> selection;
    REQUIRE(selection.Get() == 0);
    selection.Observe(0, true, true);
    REQUIRE(selection.Get() == 0);
    selection.Observe(tank, false, true);
    REQUIRE(selection.Get() == tank);
    selection.Observe(assigned, true, false); // offline/dead/non-tank assignee still owns assignment
    REQUIRE(selection.Get() == assigned);
    selection.Observe(tank, true, true); // first explicit slot wins
    REQUIRE(selection.Get() == assigned);
}
TEST_CASE("Playerbot main tank fallback follows first eligible tank and resets per snapshot", "[playerbot][roles]")
{
    uint64 first = 10, second = 20;
    MainTankSelection<uint64> selection;
    selection.Observe(first, false, false);
    REQUIRE(selection.Get() == 0);
    selection.Observe(second, false, true);
    selection.Observe(first, false, true);
    REQUIRE(selection.Get() == second);
    MainTankSelection<uint64> refreshed;
    refreshed.Observe(first, false, true);
    REQUIRE(refreshed.Get() == first);
}

TEST_CASE("Cata tank roles follow native trees and bear form", "[playerbot][roles]")
{
    REQUIRE(SpecMask(CLASS_WARRIOR, TALENT_TREE_WARRIOR_PROTECTION) & STRATEGY_TYPE_TANK);
    REQUIRE(SpecMask(CLASS_PALADIN, TALENT_TREE_PALADIN_PROTECTION) & STRATEGY_TYPE_TANK);
    REQUIRE(SpecMask(CLASS_DEATH_KNIGHT, TALENT_TREE_DEATH_KNIGHT_BLOOD) & STRATEGY_TYPE_TANK);
    REQUIRE(SpecMask(CLASS_DRUID, TALENT_TREE_DRUID_FERAL_COMBAT, FORM_BEAR) & STRATEGY_TYPE_TANK);
    REQUIRE_FALSE(SpecMask(CLASS_DRUID, TALENT_TREE_DRUID_FERAL_COMBAT, FORM_CAT) & STRATEGY_TYPE_TANK);
    REQUIRE_FALSE(SpecMask(CLASS_DEATH_KNIGHT, TALENT_TREE_DEATH_KNIGHT_FROST) & STRATEGY_TYPE_TANK);
}
TEST_CASE("Cata healer and damage roles preserve native specialization distinctions", "[playerbot][roles]")
{
    REQUIRE(SpecMask(CLASS_PRIEST, TALENT_TREE_PRIEST_DISCIPLINE) & STRATEGY_TYPE_HEAL);
    REQUIRE(SpecMask(CLASS_PRIEST, TALENT_TREE_PRIEST_HOLY) & STRATEGY_TYPE_HEAL);
    REQUIRE(SpecMask(CLASS_DRUID, TALENT_TREE_DRUID_RESTORATION) & STRATEGY_TYPE_HEAL);
    REQUIRE(SpecMask(CLASS_SHAMAN, TALENT_TREE_SHAMAN_RESTORATION) & STRATEGY_TYPE_HEAL);
    REQUIRE(SpecMask(CLASS_PALADIN, TALENT_TREE_PALADIN_HOLY) & STRATEGY_TYPE_HEAL);
    REQUIRE_FALSE(SpecMask(CLASS_PRIEST, TALENT_TREE_PRIEST_SHADOW) & STRATEGY_TYPE_HEAL);
    REQUIRE(SpecMask(CLASS_PRIEST, TALENT_TREE_PRIEST_SHADOW) & STRATEGY_TYPE_DPS);
    REQUIRE(SpecMask(CLASS_SHAMAN, TALENT_TREE_SHAMAN_ENHANCEMENT) & STRATEGY_TYPE_MELEE);
    REQUIRE(SpecMask(CLASS_SHAMAN, TALENT_TREE_SHAMAN_ELEMENTAL) & STRATEGY_TYPE_RANGED);
    REQUIRE(SpecMask(CLASS_DRUID, TALENT_TREE_DRUID_BALANCE) & STRATEGY_TYPE_RANGED);
}
TEST_CASE("Uninitialized Cata roles do not fabricate tank or healer specialization", "[playerbot][roles]")
{
    for (uint8 cls : {CLASS_WARRIOR, CLASS_PALADIN, CLASS_DEATH_KNIGHT, CLASS_DRUID, CLASS_SHAMAN, CLASS_PRIEST})
        REQUIRE_FALSE(SpecMask(cls, 0) & (STRATEGY_TYPE_TANK | STRATEGY_TYPE_HEAL));
    REQUIRE(SpecMask(CLASS_MAGE, 0) & STRATEGY_TYPE_RANGED);
    REQUIRE(SpecMask(CLASS_WARLOCK, 0) & STRATEGY_TYPE_DPS);
    REQUIRE(SpecMask(CLASS_HUNTER, 0) & STRATEGY_TYPE_RANGED);
    REQUIRE(SpecMask(CLASS_ROGUE, 0) & STRATEGY_TYPE_MELEE);
    REQUIRE(SpecMask(0, 0) == 0);
}
TEST_CASE("Published bot strategy roles override spec and strip utility bits", "[playerbot][roles]")
{
    auto spec = SpecMask(CLASS_PRIEST, TALENT_TREE_PRIEST_SHADOW);
    auto configured = STRATEGY_TYPE_HEAL | STRATEGY_TYPE_RANGED | STRATEGY_TYPE_COMBAT;
    REQUIRE(ChooseMask(configured, spec) & STRATEGY_TYPE_HEAL);
    REQUIRE_FALSE(ChooseMask(configured, spec) & STRATEGY_TYPE_DPS);
    REQUIRE_FALSE(ChooseMask(configured, spec) & STRATEGY_TYPE_COMBAT);
    REQUIRE(ChooseMask(0, spec) == spec);
    REQUIRE(ChooseMask(STRATEGY_TYPE_DPS | STRATEGY_TYPE_MELEE,
        SpecMask(CLASS_WARRIOR, TALENT_TREE_WARRIOR_PROTECTION)) == (STRATEGY_TYPE_DPS | STRATEGY_TYPE_MELEE));
}
