/* GPL v2 or later. See AUTHORS.md and PORTING.md. */
#include "../src/Bot/Cmd/PlayerbotQuestChat.h"
#include <catch2/catch.hpp>
using PlayerbotQuestChat::Parse;
TEST_CASE("Playerbot reward batch is explicit without an item or individual quest operand", "[playerbot][quest][chat]")
{
    auto all = Parse("reward *");
    REQUIRE(all); REQUIRE(all->Action == PlayerbotQuestAccept::Operation::RewardAll);
    REQUIRE(all->Quest == 0); REQUIRE(all->Item == 0);
    REQUIRE(Parse(" REWARD *  "));
    for (auto text : {"reward * 0", "reward * extra", "reward **", "reward 0 0", "share *", "drop *"})
        REQUIRE_FALSE(Parse(text));
}
TEST_CASE("Playerbot accept all is explicit and not a wildcard for other quest operations", "[playerbot][quest][chat]")
{
    auto all = Parse("accept *");
    REQUIRE(all); REQUIRE(all->Action == PlayerbotQuestAccept::Operation::AcceptAll);
    REQUIRE(all->Quest == 0); REQUIRE(all->Item == 0);
    REQUIRE(Parse(" accept *  "));
    REQUIRE_FALSE(Parse("accept * extra"));
    REQUIRE_FALSE(Parse("share *")); REQUIRE_FALSE(Parse("drop *")); REQUIRE_FALSE(Parse("reward * 0"));
}
TEST_CASE("Playerbot quest commands retain numeric and typed Cata hyperlink intent", "[playerbot][quest][chat]")
{
    std::string quest = "|cffffff00|Hquest:9193:20|h[Quest With Spaces]|h|r";
    std::string item = "|cff1eff00|Hitem:9758:0:0:0:0:0:0:0|h[Item With Spaces]|h|r";
    auto accept = Parse("ACCEPT " + quest);
    REQUIRE(accept); REQUIRE(accept->Quest == 9193);
    auto reward = Parse("reward " + quest + " " + item);
    REQUIRE(reward); REQUIRE(reward->Quest == 9193); REQUIRE(reward->Item == 9758);
    REQUIRE(Parse("share " + quest)); REQUIRE(Parse("drop " + quest));
    REQUIRE(Parse("  reward 9193 0  "));
}
TEST_CASE("Playerbot quest link commands reject malformed wrong type and hidden trailing intent", "[playerbot][quest][chat]")
{
    for (auto text : {"accept |cffffff00|Hitem:9193|h[wrong]|h|r",
         "reward 9193 |cffffff00|Hquest:9758|h[wrong]|h|r",
         "accept |cffffff00|Hquest:4294967296:20|h[overflow]|h|r",
         "accept |cffffff00|Hquest:9193junk:20|h[bad]|h|r",
         "accept |cgggggggg|Hquest:9193:20|h[bad]|h|r",
         "accept |cffffff00|Hquest:9193:20|h[unterminated]",
         "accept |cffffff00|Hquest:9193:20|h[quest]|h|rjunk",
         "accept |cffffff00|Hquest:9193:20|h[quest]|h|r extra",
         "reward 9193 9758 extra", "drop *"}) REQUIRE_FALSE(Parse(text));
    REQUIRE_FALSE(Parse(std::string(1025, 'a')));
    REQUIRE_FALSE(Parse(std::string("accept 9193\0junk", 16)));
}
