/* GPL v2 or later. See AUTHORS.md and PORTING.md. */
#include "../src/Bot/Cmd/PlayerbotStrategyCompletions.h"
#include <catch2/catch.hpp>
#include <atomic>
#include <thread>
using PlayerbotAddonProtocol::StrategyBatch;
using PlayerbotAddonProtocol::StrategyCompletionInbox;
namespace
{
StrategyBatch::Completion Completion(uint32_t bot = 1)
{ return {42, 7, bot, "group-1", "C", StrategyBatch::Stage::Executed, true}; }
PlayerbotStrategyBinding Binding()
{
    static auto session = std::shared_ptr<void const>(std::make_shared<uint8_t>(0));
    static auto lease = std::shared_ptr<void const>(std::make_shared<uint8_t>(0));
    return {7, 0, "ALL", session, lease};
}
}

TEST_CASE("Group strategy native mailbox copies generation without inventing a BOT target", "[playerbots][strategy-transport]")
{
    PlayerbotStrategyControl::Mailbox mailbox;
    std::string token = "group-1";
    REQUIRE(mailbox.Post(7, "co +focus", 100, token, {}, 42, Binding()));
    token = "changed";
    auto request = mailbox.Take(101);
    REQUIRE(request);
    REQUIRE(request->Batch == 42);
    REQUIRE(request->Requester == 7);
    REQUIRE(request->Token == "group-1");
    REQUIRE(request->Target.empty());
    REQUIRE_FALSE(mailbox.Post(7, "co +focus", 100, "group-1", {}, 42)); // No binding.
    REQUIRE_FALSE(mailbox.Post(7, "co +focus", 100, {}, {}, 42, Binding()));
    REQUIRE_FALSE(mailbox.Post(7, "co +focus", 100, "group-1", "Testone", 42, Binding()));
    REQUIRE_FALSE(mailbox.Post(7, "co +focus", 100, "bad~token", {}, 42, Binding()));
    REQUIRE_FALSE(mailbox.Post(7, "co ?", 100, "group-1", {}, 42, Binding()));
    REQUIRE_FALSE(mailbox.Post(7, "de +focus", 100, "group-1", {}, 42, Binding()));
    REQUIRE_FALSE(mailbox.Post(7, "co +focus", 100, "group-1", {}, 0));
    REQUIRE(mailbox.Post(7, "nc -food", 100, "bot-1", "Testone"));
    REQUIRE(mailbox.Take(101)->Batch == 0);
    REQUIRE(mailbox.Post(7, "co ?", 100));
    REQUIRE(mailbox.Take(101)->Batch == 0);
}

TEST_CASE("Canceled and expired group strategy requests produce no terminal result", "[playerbots][strategy-transport]")
{
    PlayerbotStrategyControl::Mailbox mailbox;
    REQUIRE(mailbox.Post(7, "nc -food", 100, "group-1", {}, 42, Binding()));
    mailbox.Cancel();
    REQUIRE_FALSE(mailbox.Take(101));
    REQUIRE(mailbox.Post(7, "nc -food", UINT32_MAX - 100, "group-1", {}, 43, Binding()));
    REQUIRE_FALSE(mailbox.Take(4899)); // Exactly 5000ms, including wrap.
    REQUIRE(mailbox.Post(7, "nc -food", 100, "group-1", {}, 44, Binding()));
    REQUIRE(mailbox.Take(5099)->Batch == 44);
}

TEST_CASE("Group completion inbox accepts only bounded copied terminal identities", "[playerbots][strategy-transport]")
{
    StrategyCompletionInbox inbox;
    auto result = Completion();
    result.Phase = StrategyBatch::Stage::Queued; REQUIRE_FALSE(inbox.Submit(result));
    result.Phase = StrategyBatch::Stage::AdmissionRejected; REQUIRE_FALSE(inbox.Submit(result));
    result = Completion(); result.Batch = 0; REQUIRE_FALSE(inbox.Submit(result));
    result = Completion(); result.Requester = 0; REQUIRE_FALSE(inbox.Submit(result));
    result = Completion(); result.Bot = 0; REQUIRE_FALSE(inbox.Submit(result));
    result = Completion(); result.Token = "bad~token"; REQUIRE_FALSE(inbox.Submit(result));
    result = Completion(); result.State = "D"; REQUIRE_FALSE(inbox.Submit(result));
    result = Completion(); REQUIRE(inbox.Submit(result));
    result.Token = "changed"; result.Batch = 99;
    auto drained = inbox.Drain();
    REQUIRE(drained.size() == 1);
    REQUIRE(drained.front().Token == "group-1");
    REQUIRE(drained.front().Batch == 42);
    REQUIRE(inbox.Drain().empty());
    for (size_t i = 0; i < StrategyCompletionInbox::Capacity; ++i) REQUIRE(inbox.Submit(Completion()));
    REQUIRE_FALSE(inbox.Submit(Completion()));
    REQUIRE(inbox.Drain().size() == StrategyCompletionInbox::Capacity);
    REQUIRE(inbox.Submit(Completion()));
}

TEST_CASE("Copied inbox completions correlate once and never turn overflow into failure", "[playerbots][strategy-transport]")
{
    StrategyCompletionInbox inbox;
    PlayerbotAddonProtocol::StrategyMutation mutation{"GROUP", "", "group-1", "C", "+focus"};
    auto batch = StrategyBatch::Create(42, 7, mutation, {1, 2}, 100);
    REQUIRE(batch);
    REQUIRE(inbox.Submit(Completion(1)));
    REQUIRE(inbox.Submit(Completion(1))); // Consumer, not transport, owns exactly-once correlation.
    auto stale = Completion(2); stale.Batch = 41;
    REQUIRE(inbox.Submit(stale));
    unsigned accepted = 0;
    for (auto const& result : inbox.Drain()) accepted += batch->Record(result, 101) ? 1 : 0;
    REQUIRE(accepted == 1);
    REQUIRE(batch->Snapshot().Unknown == 1);
    REQUIRE_FALSE(batch->TakeAck(101));
    REQUIRE(batch->TakeAck(4100) == "STRATEGY_ACK~GROUP~~group-1~C~2~1~0~TIMEOUT");
    REQUIRE_FALSE(batch->Record(Completion(2), 4101));
}

TEST_CASE("Group completion inbox serializes concurrent map producers and drain", "[playerbots][strategy-transport]")
{
    StrategyCompletionInbox inbox;
    std::atomic<unsigned> accepted{0}, finished{0};
    auto producer = [&](uint32_t first)
    {
        for (uint32_t i = 0; i < 512; ++i)
            if (inbox.Submit(Completion(first + i))) ++accepted;
        ++finished;
    };
    std::thread first(producer, 1), second(producer, 513);
    std::set<uint32_t> bots;
    while (finished.load() != 2)
    {
        for (auto const& result : inbox.Drain()) bots.insert(result.Bot);
        std::this_thread::yield();
    }
    first.join(); second.join();
    for (auto const& result : inbox.Drain()) bots.insert(result.Bot);
    REQUIRE(accepted.load() == 1024);
    REQUIRE(bots.size() == 1024);
    REQUIRE(inbox.Drain().empty());
}
