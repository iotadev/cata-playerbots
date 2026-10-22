/* GPL v2 or later. See AUTHORS.md. */
#include "../src/Bot/PlayerbotDevFixture.h"
#include <catch2/catch.hpp>
using namespace PlayerbotDevFixture;
TEST_CASE("Disposable fixture parses only the six explicit role names", "[playerbot][fixture]")
{
    REQUIRE(ParseRole("rogue") == Role::CombatRogue);
    REQUIRE(ParseRole("hunter") == Role::MarksmanHunter);
    REQUIRE(ParseRole("protection") == Role::Protection);
    REQUIRE(ParseRole("arms") == Role::Arms);
    REQUIRE(ParseRole("frost") == Role::Frost);
    REQUIRE(ParseRole("holy") == Role::Holy);
    REQUIRE_FALSE(ParseRole("")); REQUIRE_FALSE(ParseRole("Hunter"));
    REQUIRE_FALSE(ParseRole("hunter extra")); REQUIRE_FALSE(ParseRole("paladin"));
}
TEST_CASE("Disposable fixture routes class and native tab without Warrior fallback", "[playerbot][fixture]")
{
    REQUIRE(ClassFor(Role::CombatRogue) == CLASS_ROGUE); REQUIRE(TabFor(Role::CombatRogue) == 1);
    REQUIRE(ClassFor(Role::MarksmanHunter) == CLASS_HUNTER); REQUIRE(TabFor(Role::MarksmanHunter) == 1);
    REQUIRE(ClassFor(Role::Protection) == CLASS_WARRIOR); REQUIRE(TabFor(Role::Protection) == 2);
    REQUIRE(ClassFor(Role::Arms) == CLASS_WARRIOR); REQUIRE(TabFor(Role::Arms) == 0);
    REQUIRE(ClassFor(Role::Frost) == CLASS_MAGE); REQUIRE(TabFor(Role::Frost) == 2);
    REQUIRE(ClassFor(Role::Holy) == CLASS_PRIEST); REQUIRE(TabFor(Role::Holy) == 1);
    REQUIRE(ClassFor(static_cast<Role>(255)) == 0); REQUIRE(TabFor(static_cast<Role>(255)) == 255);
}
