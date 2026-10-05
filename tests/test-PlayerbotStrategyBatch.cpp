/* GPL v2 or later. See AUTHORS.md and PORTING.md. */
#include "../src/Bot/Cmd/PlayerbotStrategyBatch.h"
#include <catch2/catch.hpp>
using PlayerbotAddonProtocol::StrategyBatch;
using PlayerbotAddonProtocol::StrategyMutation;
namespace
{
StrategyMutation Request() { return {"PARTY", "", "batch-1", "C", "+focus"}; }
StrategyBatch::Completion Result(uint32_t bot, bool success, StrategyBatch::Stage stage = StrategyBatch::Stage::Executed)
{ return {42, 7, bot, "batch-1", "C", stage, success}; }
}
TEST_CASE("MultiBot strategy batch freezes bounded unique copied identities and rejects unsupported plans", "[playerbots][addon][strategy-batch]")
{
    auto request = Request();
    auto batch = StrategyBatch::Create(42, 7, request, {1, 2}, 100);
    REQUIRE(batch);
    request.Token = "changed";
    REQUIRE(batch->Record(Result(1, true), 101));
    REQUIRE_FALSE(StrategyBatch::Create(0, 7, Request(), {1}, 100));
    REQUIRE_FALSE(StrategyBatch::Create(42, 0, Request(), {1}, 100));
    REQUIRE_FALSE(StrategyBatch::Create(42, 7, Request(), {0}, 100));
    REQUIRE_FALSE(StrategyBatch::Create(42, 7, Request(), {1, 1}, 100));
    REQUIRE_FALSE(StrategyBatch::Create(42, 7, Request(), std::vector<uint32_t>(129, 1), 100));
    request = Request(); request.Changes = "+tank";
    REQUIRE_FALSE(StrategyBatch::Create(42, 7, request, {1}, 100));
    request = Request(); request.Scope = "BOT"; request.Target = "Testone";
    REQUIRE_FALSE(StrategyBatch::Create(42, 7, request, {1}, 100));
    auto empty = StrategyBatch::Create(42, 7, Request(), {}, 100);
    REQUIRE(empty->TakeAck(100) == "STRATEGY_ACK~PARTY~~batch-1~C~0~0~0~NO_MATCH");
}
TEST_CASE("MultiBot strategy batch rejects stale foreign and duplicate completions", "[playerbots][addon][strategy-batch]")
{
    auto batch = StrategyBatch::Create(42, 7, Request(), {1}, 100);
    auto completion = Result(1, true);
    completion.Batch = 43; REQUIRE_FALSE(batch->Record(completion, 101));
    completion = Result(1, true); completion.Requester = 8; REQUIRE_FALSE(batch->Record(completion, 101));
    completion = Result(1, true); completion.Token = "other"; REQUIRE_FALSE(batch->Record(completion, 101));
    completion = Result(1, true); completion.State = "N"; REQUIRE_FALSE(batch->Record(completion, 101));
    REQUIRE_FALSE(batch->Record(Result(2, true), 101));
    REQUIRE(batch->Record(Result(1, true), 101));
    REQUIRE_FALSE(batch->Record(Result(1, false), 102));
    REQUIRE(batch->Snapshot().Succeeded == 1);
    REQUIRE(batch->Snapshot().Failed == 0);
}
TEST_CASE("MultiBot strategy batch never reports queue admission as execution success", "[playerbots][addon][strategy-batch]")
{
    auto batch = StrategyBatch::Create(42, 7, Request(), {1, 2}, 100);
    REQUIRE_FALSE(batch->Record(Result(1, true, StrategyBatch::Stage::Queued), 101));
    REQUIRE_FALSE(batch->Record(Result(1, false, static_cast<StrategyBatch::Stage>(99)), 101));
    REQUIRE_FALSE(batch->Record(Result(1, true, StrategyBatch::Stage::AdmissionRejected), 101));
    REQUIRE_FALSE(batch->TakeAck(101));
    REQUIRE(batch->Record(Result(1, false, StrategyBatch::Stage::AdmissionRejected), 101));
    REQUIRE_FALSE(batch->TakeAck(101));
    REQUIRE(batch->Record(Result(2, true), 102));
    REQUIRE(batch->TakeAck(102) == "STRATEGY_ACK~PARTY~~batch-1~C~2~1~1~PARTIAL");
    REQUIRE_FALSE(batch->TakeAck(103));
    REQUIRE_FALSE(batch->Record(Result(2, true), 103));
}
TEST_CASE("MultiBot strategy batch reports complete success and failure separately", "[playerbots][addon][strategy-batch]")
{
    for (bool success : {false, true})
    {
        auto batch = StrategyBatch::Create(42, 7, Request(), {1}, 100);
        REQUIRE(batch->Record(Result(1, success), 101));
        REQUIRE(batch->TakeAck(101) == (success ? "STRATEGY_ACK~PARTY~~batch-1~C~1~1~0~OK" :
            "STRATEGY_ACK~PARTY~~batch-1~C~1~0~1~FAILED"));
    }
}
TEST_CASE("MultiBot strategy batch timeout preserves unresolved outcomes as unknown", "[playerbots][addon][strategy-batch]")
{
    auto batch = StrategyBatch::Create(42, 7, Request(), {1, 2, 3}, 100);
    REQUIRE(batch->Record(Result(1, true), 101));
    REQUIRE(batch->Record(Result(2, false), 101));
    REQUIRE_FALSE(batch->TakeAck(4099));
    REQUIRE_FALSE(batch->Record(Result(3, true), 4100));
    REQUIRE(batch->TakeAck(4100) == "STRATEGY_ACK~PARTY~~batch-1~C~3~1~1~TIMEOUT");
    REQUIRE(batch->Snapshot().Unknown == 1);
    REQUIRE_FALSE(batch->TakeAck(4101));
}
TEST_CASE("MultiBot strategy batch deadline handles timer wrap without being extended by results", "[playerbots][addon][strategy-batch]")
{
    auto batch = StrategyBatch::Create(42, 7, Request(), {1, 2}, UINT32_MAX - 100);
    REQUIRE(batch->Record(Result(1, true), 100));
    REQUIRE_FALSE(batch->TakeAck(3898));
    REQUIRE(batch->TakeAck(3899) == "STRATEGY_ACK~PARTY~~batch-1~C~2~1~0~TIMEOUT");
    REQUIRE_FALSE(batch->Record(Result(2, true), 3900));
}
