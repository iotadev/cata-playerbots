/* GPL v2 or later. See AUTHORS.md. */
#include "../src/Ai/Class/Hunter/PlayerbotHunterPolicy.h"
#include <catch2/catch.hpp>
#include <limits>
using namespace PlayerbotHunter;
TEST_CASE("Hunter focus scheduling separates builder and dump without altering power", "[playerbot][hunter]")
{
    for (unsigned focus = 0; focus <= 120; ++focus)
    {
        REQUIRE(FocusBuilder(focus) == (focus < 60));
        REQUIRE(FocusDump(focus) == (focus >= 60));
    }
}
TEST_CASE("Hunter Kill Shot requires a finite native execute threshold", "[playerbot][hunter]")
{
    REQUIRE(KillShot(20)); REQUIRE_FALSE(KillShot(20.1f)); REQUIRE_FALSE(KillShot(-1));
    REQUIRE_FALSE(KillShot(std::numeric_limits<float>::quiet_NaN()));
}
TEST_CASE("Hunter call pet slots map only the five native active slots", "[playerbot][hunter]")
{
    REQUIRE(CallSpell(0) == 883); REQUIRE(CallSpell(1) == 83242); REQUIRE(CallSpell(4) == 83245);
    for (unsigned slot = 5; slot <= 255; ++slot) REQUIRE(CallSpell(slot) == 0);
}
TEST_CASE("Hunter pet attack needs every fresh authority and target fact", "[playerbot][hunter]")
{
    for (unsigned bits = 0; bits < 128; ++bits)
        REQUIRE(PetAttackAllowed(bits & 1, bits & 2, bits & 4, bits & 8, bits & 16, bits & 32, bits & 64) == (bits == 31));
}
TEST_CASE("Hunter mend pet preserves live aura and rejects dead or unknown health", "[playerbot][hunter]")
{
    REQUIRE(MendPet(69.9f, false)); REQUIRE_FALSE(MendPet(70, false)); REQUIRE_FALSE(MendPet(30, true));
    REQUIRE_FALSE(MendPet(0, false)); REQUIRE_FALSE(MendPet(std::numeric_limits<float>::quiet_NaN(), false));
}
