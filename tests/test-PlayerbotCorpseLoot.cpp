/* Released under GNU GPL v2 or any later version. See AUTHORS.md. */
#include "../src/Ai/Base/PlayerbotCorpseLoot.h"
#include <catch2/catch.hpp>
#include <memory>

TEST_CASE("Corpse loot strategy registers donor movement and opening stages", "[PlayerbotCorpseLoot]")
{
    PlayerbotCorpseLoot::LootStrategy strategy(nullptr);
    REQUIRE(strategy.getName() == "loot");
    REQUIRE(strategy.GetType() == STRATEGY_TYPE_NONCOMBAT);
    std::vector<TriggerNode*> nodes;
    strategy.InitTriggers(nodes);
    REQUIRE(nodes.size() == 2);
    std::unique_ptr<TriggerNode> movement(nodes[0]);
    REQUIRE(movement->getName() == "far from loot target");
    REQUIRE(movement->getHandlers()[0].getName() == "move to loot");
    REQUIRE(movement->getHandlers()[0].getRelevance() == 7.0f);
    std::unique_ptr<TriggerNode> node(nodes[1]);
    REQUIRE(node->getName() == "can loot");
    REQUIRE(node->getHandlers()[0].getName() == "open loot");
    REQUIRE(node->getHandlers()[0].getRelevance() == 8.0f);
}
TEST_CASE("Corpse candidate collection is bounded and deduplicates identities", "[PlayerbotCorpseLoot]")
{
    PlayerbotCorpseLoot::Candidates candidates;
    ObjectGuid corpse(HighGuid::Unit, 1u, 1u);
    candidates.Add(ObjectGuid {}, 0);
    candidates.Add(corpse, 0);
    candidates.Add(corpse, 100);
    unsigned count = 0;
    candidates.Visit(100, [&](ObjectGuid) { ++count; });
    REQUIRE(count == 1);
    for (uint32 i = 2; i <= 12; ++i)
        candidates.Add(ObjectGuid(HighGuid::Unit, 1u, i), 200);
    count = 0;
    candidates.Visit(200, [&](ObjectGuid) { ++count; });
    REQUIRE(count == 8);
    candidates.Clear();
    count = 0;
    candidates.Visit(200, [&](ObjectGuid) { ++count; });
    REQUIRE(count == 0);
}
TEST_CASE("Corpse candidate lifetime handles wrap without tick renewal", "[PlayerbotCorpseLoot]")
{
    PlayerbotCorpseLoot::Candidates candidates;
    ObjectGuid corpse(HighGuid::Unit, 1u, 1u);
    candidates.Add(corpse, 0xfffffff0u);
    candidates.Add(corpse, 0x10u);
    unsigned count = 0;
    candidates.Visit(0x10u, [&](ObjectGuid) { ++count; });
    REQUIRE(count == 1);
    count = 0;
    candidates.Visit(uint32(0xfffffff0u + PlayerbotCorpseLoot::Candidates::LifetimeMs), [&](ObjectGuid) { ++count; });
    REQUIRE(count == 0);
}
TEST_CASE("Corpse pursuit has one identity and a bounded timeout", "[PlayerbotCorpseLoot]")
{
    PlayerbotCorpseLoot::Pursuit pursuit;
    ObjectGuid corpse(HighGuid::Unit, 1u, 1u);
    REQUIRE_FALSE(pursuit.Begin(ObjectGuid {}, 0));
    REQUIRE(pursuit.Begin(corpse, 0xfffffff0u));
    REQUIRE(pursuit.Active());
    REQUIRE(pursuit.Corpse() == corpse);
    REQUIRE_FALSE(pursuit.Begin(corpse, 0));
    REQUIRE_FALSE(pursuit.Expired(0x10u));
    REQUIRE(pursuit.Expired(uint32(0xfffffff0u + PlayerbotCorpseLoot::Pursuit::TimeoutMs)));
    pursuit.End();
    REQUIRE_FALSE(pursuit.Active());
    REQUIRE(pursuit.Begin(corpse, 100));
}
TEST_CASE("Corpse loot mailbox is bounded and stays pending until completion", "[PlayerbotCorpseLoot]")
{
    PlayerbotCorpseLoot::Mailbox mailbox;
    ObjectGuid corpse(HighGuid::Unit, 1u, 1u);
    ObjectGuid owner(HighGuid::Player, 2u);
    REQUIRE_FALSE(mailbox.Submit(ObjectGuid {}, owner));
    REQUIRE_FALSE(mailbox.Submit(corpse, ObjectGuid {}));
    REQUIRE(mailbox.Submit(corpse, owner));
    REQUIRE_FALSE(mailbox.Submit(corpse, owner));
    auto request = mailbox.Take();
    REQUIRE(request.has_value());
    REQUIRE(request->Corpse == corpse);
    REQUIRE(request->Controller == owner);
    REQUIRE(mailbox.Pending());
    REQUIRE_FALSE(mailbox.TakeResumeNeeded());
    REQUIRE_FALSE(mailbox.Take().has_value());
    mailbox.Finish(request->Generation);
    REQUIRE_FALSE(mailbox.Pending());
    REQUIRE(mailbox.TakeResumeNeeded());
    REQUIRE_FALSE(mailbox.TakeResumeNeeded());
}
TEST_CASE("Corpse loot stale completion cannot clear a newer request", "[PlayerbotCorpseLoot]")
{
    PlayerbotCorpseLoot::Mailbox mailbox;
    ObjectGuid corpse(HighGuid::Unit, 1u, 1u);
    ObjectGuid owner(HighGuid::Player, 2u);
    REQUIRE(mailbox.Submit(corpse, owner));
    auto old = mailbox.Take();
    REQUIRE(old.has_value());
    mailbox.Cancel();
    REQUIRE(mailbox.Submit(corpse, owner));
    mailbox.Finish(old->Generation);
    REQUIRE(mailbox.Pending());
    mailbox.Cancel();
    REQUIRE_FALSE(mailbox.Take().has_value());
    REQUIRE_FALSE(mailbox.Pending());
}
TEST_CASE("Corpse loot backoff is per identity and handles millisecond wrap", "[PlayerbotCorpseLoot]")
{
    PlayerbotCorpseLoot::AttemptHistory history;
    ObjectGuid corpse(HighGuid::Unit, 1u, 1u);
    ObjectGuid other(HighGuid::Unit, 1u, 2u);
    REQUIRE_FALSE(history.CanAttempt(ObjectGuid {}, 0));
    REQUIRE(history.CanAttempt(corpse, 100));
    history.Record(corpse, 100);
    REQUIRE_FALSE(history.CanAttempt(corpse, 101));
    REQUIRE(history.CanAttempt(other, 101));
    REQUIRE(history.CanAttempt(corpse, 30100));
    history.Record(corpse, 0xfffffff0u);
    REQUIRE_FALSE(history.CanAttempt(corpse, 0x10u));
}
