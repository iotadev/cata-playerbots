/* GPL v2 or later. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotPosition.h"
#include <catch2/catch.hpp>
#include <limits>
#include <memory>
using namespace PlayerbotPosition;

TEST_CASE("Stay request mailbox is bounded single-use and cancelable", "[playerbot][position]")
{
    Mailbox mailbox;
    REQUIRE_FALSE(mailbox.Post(0, 100));
    REQUIRE(mailbox.Post(1, 100));
    REQUIRE_FALSE(mailbox.Post(2, 101));
    auto request = mailbox.Take(102);
    REQUIRE(request);
    REQUIRE(request->Requester == 1);
    REQUIRE_FALSE(mailbox.Take(103));
    REQUIRE(mailbox.Post(2, 104));
    mailbox.Cancel();
    REQUIRE_FALSE(mailbox.Take(105));
}
TEST_CASE("Stay request expiry handles timer wrap", "[playerbot][position]")
{
    Mailbox mailbox;
    REQUIRE(mailbox.Post(1, UINT32_MAX - 1000));
    REQUIRE(mailbox.Take(3998));
    REQUIRE(mailbox.Post(1, UINT32_MAX - 1000));
    REQUIRE_FALSE(mailbox.Take(3999));
}
TEST_CASE("Stay phase snapshots compare copied phase and terrain identities", "[playerbot][position]")
{
    PhaseStamp original{1, 2, {{3, 4}}, {5}, {6}};
    auto changed = original;
    REQUIRE(changed == original);
    changed.Flags = 0; REQUIRE_FALSE(changed == original);
    changed = original; changed.Personal = 9; REQUIRE_FALSE(changed == original);
    changed = original; changed.Phases[0].second = 0; REQUIRE_FALSE(changed == original);
    changed = original; changed.Terrain.push_back(7); REQUIRE_FALSE(changed == original);
    changed = original; changed.UiMaps.clear(); REQUIRE_FALSE(changed == original);
}
TEST_CASE("Stay lifecycle and movement cleanup require exact ownership", "[playerbot][position]")
{
    REQUIRE(KeepStay(true, false, true));
    REQUIRE_FALSE(KeepStay(false, false, true));
    REQUIRE_FALSE(KeepStay(true, true, true));
    REQUIRE_FALSE(KeepStay(true, false, false));
    REQUIRE(OwnsReturn(ReturnMovementId));
    REQUIRE_FALSE(OwnsReturn(0));
    REQUIRE_FALSE(OwnsReturn(ReturnMovementId + 1));
}
TEST_CASE("Stay movement retry timing is bounded across clock wrap", "[playerbot][position]")
{
    PositionInfo position;
    REQUIRE(position.CanAttempt(0));
    position.Attempted = true; position.LastAttempt = UINT32_MAX - 1000;
    REQUIRE_FALSE(position.CanAttempt(3998));
    REQUIRE(position.CanAttempt(3999));
    position.Reset();
    REQUIRE(position.CanAttempt(0));
}

TEST_CASE("Saved positions bind coordinates to controller map and instance", "[playerbot][position]")
{
    PositionInfo position;
    REQUIRE_FALSE(position.Matches(0, 0, 1));
    REQUIRE(position.Capture(0, 0, 0, 0, 0, 1));
    REQUIRE(position.Matches(0, 0, 1)); // Origin and map zero are valid, not unset sentinels.
    REQUIRE_FALSE(position.Matches(1, 0, 1));
    REQUIRE_FALSE(position.Matches(0, 1, 1));
    REQUIRE_FALSE(position.Matches(0, 0, 2));
    REQUIRE_FALSE(position.Matches(0, 0, 0));
    position.Reset();
    REQUIRE_FALSE(position.Set);
    REQUIRE(position.Controller == 0);
}
TEST_CASE("Saved position capture rejects nonfinite input without partial mutation", "[playerbot][position]")
{
    PositionInfo position;
    REQUIRE(position.Capture(1, 2, 3, 4, 5, 6));
    REQUIRE_FALSE(position.Capture(std::numeric_limits<float>::infinity(), 0, 0, 0, 0, 1));
    REQUIRE_FALSE(position.Capture(0, std::numeric_limits<float>::quiet_NaN(), 0, 0, 0, 1));
    REQUIRE_FALSE(position.Capture(0, 0, std::numeric_limits<float>::infinity(), 0, 0, 1));
    REQUIRE_FALSE(position.Capture(0, 0, 0, 0, 0, 0));
    REQUIRE(position.X == 1);
    REQUIRE(position.Matches(4, 5, 6));
}
TEST_CASE("Stay return policy waits nearby moves within leash and reanchors far away", "[playerbot][position]")
{
    REQUIRE(DecideReturn(true, true, 3) == ReturnDecision::Wait);
    REQUIRE(DecideReturn(true, true, 3.1f) == ReturnDecision::Move);
    REQUIRE(DecideReturn(true, true, 35) == ReturnDecision::Move);
    REQUIRE(DecideReturn(true, true, 35.1f) == ReturnDecision::Reanchor);
    REQUIRE(DecideReturn(false, true, 10) == ReturnDecision::Wait);
    REQUIRE(DecideReturn(true, false, 10) == ReturnDecision::Wait);
    REQUIRE(DecideReturn(true, true, std::numeric_limits<float>::quiet_NaN()) == ReturnDecision::Wait);
    REQUIRE(DecideReturn(true, true, std::numeric_limits<float>::infinity()) == ReturnDecision::Wait);
}
TEST_CASE("Position value owns independent maps and does not accept donor permissive persistence", "[playerbot][position]")
{
    PositionValue first(nullptr), second(nullptr);
    REQUIRE(first.RefGet()["stay"].Capture(1, 2, 3, 4, 5, 6));
    REQUIRE(second.RefGet().empty());
    REQUIRE(first.Get().at("stay").Matches(4, 5, 6));
    REQUIRE_FALSE(first.Load("stay=invalid,0,0,0"));
    REQUIRE(first.RefGet().at("stay").X == 1);
    first.Reset();
    REQUIRE(first.RefGet().empty());
}
TEST_CASE("Stay strategy retains donor return priority and stay default action", "[playerbot][position]")
{
    StayStrategy strategy(nullptr);
    std::vector<TriggerNode*> triggers;
    strategy.InitTriggers(triggers);
    REQUIRE(triggers.size() == 1);
    std::unique_ptr<TriggerNode> trigger(triggers.front());
    REQUIRE(trigger->getName() == "return to stay position");
    REQUIRE(trigger->getFirstRelevance() == ACTION_MOVE);
    auto handlers = trigger->getHandlers();
    REQUIRE(handlers.at(0).getName() == "return to stay position");
    auto defaults = strategy.getDefaultActions();
    REQUIRE(defaults.at(0).getName() == "stay");
    REQUIRE(defaults.at(0).getRelevance() == 1.0f);
}
