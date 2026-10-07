/* GPL v2 or later. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotLootRoll.h"
#include <catch2/catch.hpp>
namespace
{
PlayerbotLootRoll TestRoll(uint64 identity = 20)
{
    PlayerbotLootRoll roll;
    roll.Group = 10; roll.Roll = identity; roll.Entry = 9758; roll.Count = 1;
    roll.Mask = 7; roll.Map = 389; roll.Instance = 3;
    return roll;
}
}
TEST_CASE("Playerbot roll mailbox admits only one native pending identity at a time", "[playerbot][loot]")
{
    PlayerbotRoll::Mailbox box;
    REQUIRE(box.Post(TestRoll(), 1, 100));
    REQUIRE_FALSE(box.Post(TestRoll(21), 1, 101));
    auto request = box.Take(); REQUIRE(request);
    REQUIRE_FALSE(box.Take());
    REQUIRE_FALSE(box.Post(TestRoll(21), 1, 102));
    REQUIRE(box.Complete(*request, 1, 110));
    REQUIRE_FALSE(box.Complete(*request, 1, 111));
    auto reply = box.Consume(112); REQUIRE(reply);
    REQUIRE(reply->Choice == 1);
    REQUIRE_FALSE(box.Consume(113));
    REQUIRE(box.Post(TestRoll(21), 1, 114));
    REQUIRE_FALSE(box.Complete(*request, 1, 115));
}
TEST_CASE("Playerbot roll mailbox expires map work and drops late replies including clock wrap", "[playerbot][loot]")
{
    PlayerbotRoll::Mailbox box;
    REQUIRE(box.Post(TestRoll(), 1, UINT32_MAX - 100));
    auto request = box.Take(); REQUIRE(request);
    REQUIRE(PlayerbotRoll::Fresh(*request, 100));
    REQUIRE_FALSE(box.Consume(5000));
    REQUIRE_FALSE(box.Complete(*request, 1, 5001));
    REQUIRE(box.Post(TestRoll(), 1, 5002));
    REQUIRE_FALSE(box.Complete(*request, 1, 5003));
    auto refreshed = box.Take(); REQUIRE(refreshed);
    REQUIRE(box.Complete(*refreshed, 2, 5004));
    auto reply = box.Consume(5005); REQUIRE(reply);
    REQUIRE(reply->Original.Serial != request->Serial);
    REQUIRE(reply->Choice == 2);
}
TEST_CASE("Playerbot roll transport rejects changed loot facts and unavailable vote choices", "[playerbot][loot]")
{
    PlayerbotRoll::Mailbox box;
    REQUIRE(box.Post(TestRoll(), 1, 100));
    auto request = box.Take(); REQUIRE(request);
    auto changed = *request;
    changed.Controller = 2;
    REQUIRE_FALSE(box.Complete(changed, 1, 101));
    changed = *request;
    changed.Identity.Property = 9;
    REQUIRE_FALSE(box.Complete(changed, 1, 101));
    changed = *request; changed.Identity.Instance = 4;
    REQUIRE_FALSE(box.Complete(changed, 1, 102));
    REQUIRE_FALSE(box.Complete(*request, 3, 103));
    REQUIRE_FALSE(box.Complete(*request, 4, 104));
    REQUIRE(box.Complete(*request, 0, 105));
}
TEST_CASE("Playerbot native roll admission rejects an existing vote and refreshed mask restrictions", "[playerbot][loot]")
{
    REQUIRE(PlayerbotLootRoll::Admits(true, 7, 1));
    REQUIRE_FALSE(PlayerbotLootRoll::Admits(false, 7, 1));
    REQUIRE_FALSE(PlayerbotLootRoll::Admits(true, 5, 1));
    REQUIRE(PlayerbotLootRoll::Admits(true, 5, 2));
    REQUIRE_FALSE(PlayerbotLootRoll::Admits(true, 7, 3));
    REQUIRE_FALSE(PlayerbotLootRoll::Admits(true, 255, 255));
    auto old = TestRoll(); auto current = old;
    current.Mask = 5;
    REQUIRE(current.Matches(old)); // identity survives; fresh mask still controls vote
    current.Group = 11; REQUIRE_FALSE(current.Matches(old));
    current = old; current.SuffixFactor = 12; REQUIRE_FALSE(current.Matches(old));
}
TEST_CASE("Playerbot disabling rolls cancels pending evaluation and completion", "[playerbot][loot]")
{
    PlayerbotRoll::Mailbox box;
    REQUIRE_FALSE(box.Post({}, 1, 100));
    REQUIRE_FALSE(box.Post(TestRoll(), 0, 100));
    REQUIRE(box.Post(TestRoll(), 1, 100));
    auto request = box.Take(); REQUIRE(request);
    box.Cancel();
    REQUIRE_FALSE(box.Complete(*request, 1, 101));
    REQUIRE_FALSE(box.Consume(102));
    REQUIRE(box.Post(TestRoll(), 1, 103));
}
