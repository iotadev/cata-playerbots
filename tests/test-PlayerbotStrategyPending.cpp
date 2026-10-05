/* GPL v2 or later. See PORTING.md. Pure ownership/correlation tests, not gameplay. */
#include "../src/Bot/Cmd/PlayerbotStrategyPending.h"
#include <catch2/catch.hpp>
using PlayerbotAddonProtocol::StrategyPending;
using PlayerbotAddonProtocol::StrategyBatch;
namespace
{
struct Owner
{
    std::shared_ptr<void const> Session = std::make_shared<uint8_t>(0);
    PlayerbotStrategyBinding Binding(uint32_t account = 10) { return {account, 99, "PARTY", Session, {}}; }
};
PlayerbotAddonProtocol::StrategyMutation Mutation() { return {"PARTY", "", "group-1", "C", "+focus"}; }
StrategyBatch::Completion Result(uint64_t batch, uint32_t bot, bool success = true)
{ return {batch, 7, bot, "group-1", "C", StrategyBatch::Stage::Executed, success}; }
}
TEST_CASE("Group strategy binding preserves donor scope and rejects replacement login or membership", "[playerbots][strategy-pending]")
{
    Owner owner;
    auto binding = owner.Binding();
    auto lease = std::shared_ptr<void const>(std::make_shared<uint8_t>(0));
    binding.Lease = lease;
    REQUIRE(binding.Valid());
    REQUIRE(binding.SessionMatches(10, owner.Session));
    REQUIRE_FALSE(binding.SessionMatches(11, owner.Session));
    auto oldSession = owner.Session;
    owner.Session = std::make_shared<uint8_t>(0); // Same account/character, different login identity.
    REQUIRE_FALSE(binding.SessionMatches(10, owner.Session));
    REQUIRE(binding.GroupMatches(99, 99, false));
    REQUIRE(binding.GroupMatches(99, 99, true)); // PARTY includes raids.
    REQUIRE_FALSE(binding.GroupMatches(100, 99, true));
    REQUIRE_FALSE(binding.GroupMatches(99, 100, true));
    binding.Scope = "RAID";
    REQUIRE_FALSE(binding.GroupMatches(99, 99, false));
    REQUIRE(binding.GroupMatches(99, 99, true));
    binding.Scope = "GROUP"; REQUIRE(binding.GroupMatches(99, 99, true));
    binding.Scope = "ALL"; binding.Group = 0; REQUIRE(binding.Valid());
    REQUIRE(binding.GroupMatches(0, 101, false));
    binding.Scope = "BOT"; REQUIRE_FALSE(binding.Valid());
    binding.Scope = "ALL"; lease.reset(); REQUIRE_FALSE(binding.Valid());
}
TEST_CASE("Group strategy pending admission is bounded one per account and rejects invalid plans atomically", "[playerbots][strategy-pending]")
{
    Owner owner;
    StrategyPending pending;
    REQUIRE_FALSE(pending.Begin(7, owner.Binding(0), Mutation(), {1}, 100));
    REQUIRE_FALSE(pending.Begin(7, owner.Binding(), Mutation(), {1, 1}, 100));
    REQUIRE_FALSE(pending.Begin(7, owner.Binding(), Mutation(), std::vector<uint32_t>(129, 1), 100));
    REQUIRE(pending.Entries().empty());
    for (uint32_t i = 1; i <= StrategyPending::Capacity; ++i)
        REQUIRE(pending.Begin(7, owner.Binding(i), Mutation(), {1}, 100));
    REQUIRE(pending.Busy(1));
    REQUIRE_FALSE(pending.Begin(7, owner.Binding(1), Mutation(), {2}, 100));
    REQUIRE_FALSE(pending.Begin(7, owner.Binding(33), Mutation(), {2}, 100));
    REQUIRE(pending.Entries().size() == StrategyPending::Capacity);
}
TEST_CASE("Group strategy pending produces one ACK after out of order terminal and admission results", "[playerbots][strategy-pending]")
{
    Owner owner;
    StrategyPending pending;
    auto id = pending.Begin(7, owner.Binding(), Mutation(), {1, 2, 3}, 100);
    REQUIRE(id);
    REQUIRE(pending.Record(Result(*id, 3), 101));
    REQUIRE_FALSE(pending.Record(Result(*id, 3), 101));
    auto rejected = Result(*id, 2, false); rejected.Phase = StrategyBatch::Stage::AdmissionRejected;
    REQUIRE(pending.Record(rejected, 101));
    REQUIRE(pending.Poll(101).empty());
    REQUIRE(pending.Record(Result(*id, 1), 102));
    auto replies = pending.Poll(102);
    REQUIRE(replies.size() == 1);
    REQUIRE(replies.front().Requester == 7);
    REQUIRE(replies.front().Binding.SessionMatches(10, owner.Session));
    REQUIRE(replies.front().Message == "STRATEGY_ACK~PARTY~~group-1~C~3~2~1~PARTIAL");
    REQUIRE(pending.Poll(103).empty());
    REQUIRE_FALSE(pending.Busy(10));
    REQUIRE_FALSE(pending.Record(Result(*id, 1), 103));
}
TEST_CASE("Group strategy abandon revokes queued lease and old generations cannot complete new batches", "[playerbots][strategy-pending]")
{
    Owner owner;
    StrategyPending pending;
    auto first = pending.Begin(7, owner.Binding(), Mutation(), {1}, 100);
    auto binding = pending.Entries().at(*first).Binding;
    REQUIRE(binding.Valid());
    pending.Abandon(*first);
    REQUIRE_FALSE(binding.Valid());
    REQUIRE(pending.Poll(4100).empty());
    auto second = pending.Begin(7, owner.Binding(), Mutation(), {1}, 101);
    REQUIRE(*second > *first);
    REQUIRE_FALSE(pending.Record(Result(*first, 1), 102));
    REQUIRE(pending.Record(Result(*second, 1), 102));
    REQUIRE(pending.Poll(102).size() == 1);
}
TEST_CASE("Group strategy cancellation revokes unexecuted work without inventing failure or rollback", "[playerbots][strategy-pending]")
{
    Owner owner;
    StrategyPending pending;
    auto id = pending.Begin(7, owner.Binding(), Mutation(), {1, 2}, 100);
    auto binding = pending.Entries().at(*id).Binding;
    REQUIRE(pending.Record(Result(*id, 1), 101));
    pending.Cancel(*id);
    REQUIRE_FALSE(binding.Valid());
    REQUIRE(pending.Poll(102).empty());
    REQUIRE(pending.Poll(4100).front().Message == "STRATEGY_ACK~PARTY~~group-1~C~2~1~0~TIMEOUT");
    REQUIRE_FALSE(pending.Record(Result(*id, 2), 4101));
}
TEST_CASE("Group strategy pending supports no match and timer wrap without extending deadline", "[playerbots][strategy-pending]")
{
    Owner owner;
    StrategyPending pending;
    REQUIRE(pending.Begin(7, owner.Binding(), Mutation(), {}, 100));
    REQUIRE(pending.Poll(100).front().Message == "STRATEGY_ACK~PARTY~~group-1~C~0~0~0~NO_MATCH");
    auto id = pending.Begin(7, owner.Binding(), Mutation(), {1, 2}, UINT32_MAX - 100);
    REQUIRE(pending.Record(Result(*id, 1), 100));
    REQUIRE(pending.Poll(3898).empty());
    REQUIRE(pending.Poll(3899).front().Message == "STRATEGY_ACK~PARTY~~group-1~C~2~1~0~TIMEOUT");
}
