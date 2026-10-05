/* GPL v2 or later. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotMount.h"
#include <catch2/catch.hpp>
#include <algorithm>
#include <array>
using namespace PlayerbotMount;

TEST_CASE("Ground mount admission requires every control and native gate", "[playerbot][mount]")
{
    REQUIRE(ShouldMount(true, true, true, false, false, true));
    REQUIRE_FALSE(ShouldMount(false, true, true, false, false, true));
    REQUIRE_FALSE(ShouldMount(true, false, true, false, false, true));
    REQUIRE_FALSE(ShouldMount(true, true, false, false, false, true));
    REQUIRE_FALSE(ShouldMount(true, true, true, true, false, true));
    REQUIRE_FALSE(ShouldMount(true, true, true, false, true, true));
    REQUIRE_FALSE(ShouldMount(true, true, true, false, false, false));
}
TEST_CASE("Owned ground mounts yield to commands combat and controller changes", "[playerbot][mount]")
{
    REQUIRE_FALSE(ShouldRelease(true, true, true, true, true, false));
    REQUIRE(ShouldRelease(true, false, true, true, true, false));
    REQUIRE(ShouldRelease(true, true, false, true, true, false));
    REQUIRE(ShouldRelease(true, true, true, false, true, false));
    REQUIRE(ShouldRelease(true, true, true, true, false, false));
    REQUIRE(ShouldRelease(true, true, true, true, true, true));
    REQUIRE_FALSE(ShouldRelease(false, false, false, false, false, true));
}
TEST_CASE("Ground mount capability excludes flight underwater and nonmount spells", "[playerbot][mount]")
{
    REQUIRE(GroundCapability(true, true, false, false));
    REQUIRE_FALSE(GroundCapability(false, true, false, false));
    REQUIRE_FALSE(GroundCapability(true, false, false, false));
    REQUIRE_FALSE(GroundCapability(true, true, true, false));
    REQUIRE_FALSE(GroundCapability(true, true, false, true));
}
TEST_CASE("Mount retries survive clock wrap and ownership cleanup", "[playerbot][mount]")
{
    State state;
    REQUIRE(state.CanAttempt(0));
    state.Attempt(UINT32_MAX - 1000);
    state.Spell = 123; state.Owner = 456; state.Started = UINT32_MAX - 1000;
    REQUIRE_FALSE(state.CanAttempt(3998));
    REQUIRE(state.CanAttempt(3999));
    state.Forget();
    REQUIRE(state.Spell == 0);
    REQUIRE(state.Owner == 0);
    REQUIRE(state.Started == 0);
    REQUIRE_FALSE(state.CanAttempt(3998));
}
TEST_CASE("Mount candidates prefer native speed with stable learned spell ties", "[playerbot][mount]")
{
    std::array<Candidate, 3> candidates{{{30, 300, 60}, {20, 200, 100}, {10, 100, 100}}};
    std::sort(candidates.begin(), candidates.end(), Better);
    REQUIRE(candidates[0].Base == 10);
    REQUIRE(candidates[1].Base == 20);
    REQUIRE(candidates[2].Base == 30);
    REQUIRE_FALSE(Better(candidates[0], candidates[0]));
}
