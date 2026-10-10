#include "../src/Bot/Engine/ActionTrace.h"
#include <catch2/catch.hpp>

TEST_CASE("Playerbot passive action trace is bounded coalesced and explicit about loss", "[playerbot][trace]")
{
    ActionTraceBuffer trace;
    trace.Enable(true);
    trace.Record("combat", {100, "attack", "rejected", "not_possible"});
    trace.Record("combat", {200, "attack", "rejected", "not_possible"});
    auto first = trace.Read(300);
    REQUIRE(first.Events.size() == 1);
    REQUIRE(first.Events[0].Repeats == 2);
    REQUIRE(first.Events[0].FirstSequence == 1);
    REQUIRE(first.Events[0].Sequence == 2);
    for (uint32 i = 0; i < 80; ++i)
        trace.Record("combat", {1000 + i * 1000, "action " + std::to_string(i), "execution", "action_returned_true", true, true, true});
    auto result = trace.Read(80000);
    REQUIRE(result.Events.size() <= 64);
    REQUIRE(result.Expired > 0);
    REQUIRE(result.LastSequence == 82);
    ActionTraceBuffer capacity;
    capacity.Enable(true);
    for (uint32 i = 0; i < 70; ++i)
        capacity.Record("combat", {1000 + i * 500, "action " + std::to_string(i), "execution", "action_returned_true", true, true, true});
    REQUIRE(capacity.Read(36000).Events.size() == 64);
    REQUIRE(capacity.Read(36000).Overwritten == 6);
    trace.Enable(false);
    REQUIRE_FALSE(trace.Read(80000).Available);
    REQUIRE(trace.Read(80000).Events.empty());
    trace.Enable(true);
    REQUIRE(trace.Read(80000).LastSequence == 0);
    REQUIRE(trace.Read(80000).Epoch > first.Epoch);
}

TEST_CASE("Playerbot trace admission flood does not consume execution budget", "[playerbot][trace]")
{
    ActionTraceBuffer trace;
    trace.Enable(true);
    for (uint32 i = 0; i < 40; ++i)
        trace.Record("combat", {100, "candidate " + std::to_string(i), "rejected", "not_useful"});
    trace.Record("combat", {100, "cast", "execution", "action_returned_false", true, false, false});
    auto result = trace.Read(100);
    REQUIRE(result.Suppressed == 24);
    REQUIRE(result.Events.back().Action == "cast");
    REQUIRE(result.Events.back().ActionReturn == false);
    REQUIRE(result.Events.back().EngineResult == false);
}

TEST_CASE("Playerbot trace expiry is safe across the native millisecond wrap", "[playerbot][trace]")
{
    ActionTraceBuffer trace;
    trace.Enable(true);
    trace.Record("noncombat", {0xfffffff0u, "buff", "execution", "action_returned_true", true, true, true});
    REQUIRE(trace.Read(0x10u).Events[0].FirstAgeMs == 32);
    REQUIRE(trace.Read(60032u).Events.empty());
}
