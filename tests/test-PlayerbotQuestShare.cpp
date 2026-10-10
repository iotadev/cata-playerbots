/* GPL v2 or later. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotQuestShare.h"
#include <catch2/catch.hpp>
TEST_CASE("Playerbot quest confirmation follows native party push rather than manual share flags", "[playerbot][quest]")
{
    using namespace PlayerbotQuestShare;
    REQUIRE(ChooseRoute(100, false, true, true, false) == Route::PartyConfirmation);
    REQUIRE(ChooseRoute(100, false, false, true, true) == Route::Ordinary);
    REQUIRE(ChooseRoute(100, false, false, true, false) == Route::Unavailable);
    REQUIRE(ChooseRoute(100, false, true, false, true) == Route::Unavailable);
}
TEST_CASE("Playerbot quest confirmation refuses turn in zero and signed field overflow", "[playerbot][quest]")
{
    using namespace PlayerbotQuestShare;
    REQUIRE(ChooseRoute(0, false, true, true, true) == Route::Unavailable);
    REQUIRE(ChooseRoute(100, true, true, true, true) == Route::Unavailable);
    REQUIRE(ChooseRoute(0x80000000u, false, true, true, true) == Route::Unavailable);
    REQUIRE(ChooseRoute(0x7fffffffu, false, true, true, true) == Route::PartyConfirmation);
}
TEST_CASE("Playerbot quest share attempts consume one observed native identity", "[playerbot][quest]")
{
    PlayerbotQuestShare::Attempt attempt;
    REQUIRE(attempt.Take(10, 100));
    REQUIRE_FALSE(attempt.Take(10, 100));
    REQUIRE(attempt.Take(11, 100));
    REQUIRE(attempt.Take(11, 101));
}
TEST_CASE("Playerbot cleared quest share permits a later identical offer", "[playerbot][quest]")
{
    PlayerbotQuestShare::Attempt attempt;
    REQUIRE(attempt.Take(10, 100));
    REQUIRE_FALSE(attempt.Take(0, 100));
    REQUIRE(attempt.Take(10, 100));
    attempt.Reset();
    REQUIRE(attempt.Take(10, 100));
    REQUIRE_FALSE(attempt.Take(10, 0));
    REQUIRE(attempt.Take(10, 100));
}
