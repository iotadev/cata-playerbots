/* Released under GNU GPL v2 or any later version. See AUTHORS.md. */
#include "../src/Ai/Base/PlayerbotLootPolicy.h"
#include <catch2/catch.hpp>

TEST_CASE("Disabled bot loot policy does not overwrite native preference", "[PlayerbotLoot]")
{
    PlayerbotLoot::PassPreference policy;
    REQUIRE_FALSE(policy.Update(false, false).has_value());
    REQUIRE_FALSE(policy.Update(false, true).has_value());
}
TEST_CASE("Enabled bot loot policy writes auto-pass once and restores original on disable", "[PlayerbotLoot]")
{
    PlayerbotLoot::PassPreference policy;
    REQUIRE(policy.Update(true, false) == true);
    REQUIRE_FALSE(policy.Update(true, true).has_value());
    REQUIRE(policy.Update(false, true) == false);
    REQUIRE_FALSE(policy.Update(false, false).has_value());
}
TEST_CASE("Bot loot policy preserves preexisting auto-pass and reasserts while enabled", "[PlayerbotLoot]")
{
    PlayerbotLoot::PassPreference policy;
    REQUIRE_FALSE(policy.Update(true, true).has_value());
    REQUIRE(policy.Update(true, false) == true);
    REQUIRE_FALSE(policy.Update(false, true).has_value());
}
TEST_CASE("Bot loot policy takes a fresh baseline on reenable", "[PlayerbotLoot]")
{
    PlayerbotLoot::PassPreference policy;
    REQUIRE(policy.Update(true, false) == true);
    REQUIRE(policy.Update(false, true) == false);
    REQUIRE_FALSE(policy.Update(true, true).has_value());
    REQUIRE_FALSE(policy.Update(false, true).has_value());
}
