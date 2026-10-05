/* Released under GNU GPL v2 or any later version. See AUTHORS.md. */
#include "../src/Bot/PlayerbotReadyCheck.h"
#include "../src/Bot/ForceRebuff.h"
#include <catch2/catch.hpp>
using namespace PlayerbotReadyCheck;

TEST_CASE("Ready check supply counts preserve usable stack identity", "[PlayerbotReadyCheck]")
{
    Supplies supplies;
    CountItem(supplies, 20, false, true, true, true, true);
    REQUIRE(supplies.Food == 0);
    REQUIRE(supplies.Drink == 0);
    REQUIRE(supplies.HealingPotion == 0);
    REQUIRE(supplies.ManaPotion == 0);
    CountItem(supplies, 20, true, true, false, false, false);
    CountItem(supplies, 5, true, false, true, false, false);
    CountItem(supplies, 2, true, false, false, true, true);
    REQUIRE(supplies.Food == 20);
    REQUIRE(supplies.Drink == 5);
    REQUIRE(supplies.HealingPotion == 2);
    REQUIRE(supplies.ManaPotion == 2);
    CountItem(supplies, 0xffffffffu, true, true, false, false, false);
    REQUIRE(supplies.Food == 0xffffffffu);
}
TEST_CASE("Ready check supply policy distinguishes mana users and missing categories", "[PlayerbotReadyCheck]")
{
    REQUIRE_FALSE(Supplies{}.Ready(false));
    REQUIRE((Supplies{1, 0, 1, 0}.Ready(false)));
    REQUIRE_FALSE((Supplies{1, 0, 1, 0}.Ready(true)));
    REQUIRE((Supplies{1, 1, 1, 1}.Ready(true)));
    REQUIRE_FALSE((Supplies{0, 1, 1, 1}.Ready(true)));
    REQUIRE_FALSE((Supplies{1, 0, 1, 1}.Ready(true)));
    REQUIRE_FALSE((Supplies{1, 1, 0, 1}.Ready(true)));
    REQUIRE_FALSE((Supplies{1, 1, 1, 0}.Ready(true)));
}
TEST_CASE("Deferred ready replies distinguish completion from cancel replacement and expiry", "[PlayerbotReadyCheck]")
{
    Request request{1, 2, 3, 100};
    auto poll = [&](bool current = true, bool controller = true, bool samePass = true,
        bool safe = true, bool pending = true, bool completed = false, uint32_t now = 101)
    { return PollDeferred(request, now, current, controller, samePass, safe, pending, completed); };
    REQUIRE(poll() == DeferredResult::Wait);
    REQUIRE(poll(true, true, true, true, false, true) == DeferredResult::Evaluate);
    REQUIRE(poll(true, true, true, true, false, false) == DeferredResult::Reject);
    REQUIRE(poll(true, false) == DeferredResult::Reject);
    REQUIRE(poll(true, true, false) == DeferredResult::Reject);
    REQUIRE(poll(true, true, true, false) == DeferredResult::Reject);
    REQUIRE(poll(false) == DeferredResult::Cancel);
    REQUIRE(poll(true, true, true, true, true, false, 30100) == DeferredResult::Cancel);
}
TEST_CASE("Ready check cancellation cannot clear a replacement mailbox", "[PlayerbotReadyCheck]")
{
    Mailbox box;
    Request old{1, 2, 3, 100}, replacement{1, 4, 3, 110};
    box.Post(old);
    REQUIRE(box.IsCurrent(old, 100));
    box.Post(replacement);
    box.Cancel(old);
    REQUIRE(box.Active(111));
    REQUIRE(box.IsCurrent(replacement, 111));
    REQUIRE_FALSE(box.IsCurrent(old, 111));
    box.Cancel(replacement);
    REQUIRE_FALSE(box.Active(112));
    REQUIRE_FALSE(box.TakeRequest(112));
    REQUIRE_FALSE(box.Complete(replacement, true, 112));
}
TEST_CASE("Rebuff success is explicit and scoped to the current pass", "[PlayerbotReadyCheck]")
{
    ForceRebuffState state;
    state.Begin(100);
    auto serial = state.Serial();
    REQUIRE_FALSE(state.Completed());
    state.Finish();
    REQUIRE(state.Completed());
    REQUIRE_FALSE(state.IsPending(101));
    REQUIRE(state.Serial() == serial);
    state.Begin(200);
    REQUIRE(state.Serial() != serial);
    REQUIRE_FALSE(state.Completed());
    state.End();
    REQUIRE_FALSE(state.Completed());
    state.Begin(300);
    REQUIRE_FALSE(state.IsPending(120300));
    REQUIRE_FALSE(state.Completed());
}

TEST_CASE("Ready check bridge consumes each direction once", "[PlayerbotReadyCheck]")
{
    Mailbox box;
    Request request{1, 2, 3, 100};
    box.Post(request);
    REQUIRE(box.TakeRequest(100));
    REQUIRE_FALSE(box.TakeRequest(100));
    REQUIRE(box.Complete(request, true, 101));
    REQUIRE_FALSE(box.Complete(request, false, 101));
    auto reply = box.TakeReply(102);
    REQUIRE(reply);
    REQUIRE(reply->Ready);
    REQUIRE_FALSE(box.TakeReply(103));
    REQUIRE_FALSE(box.Complete(request, true, 103));
}
TEST_CASE("Ready check replacement rejects stale and mismatched map results", "[PlayerbotReadyCheck]")
{
    Mailbox box;
    Request old{1, 2, 3, 100}, current{1, 4, 3, 110};
    box.Post(old);
    REQUIRE(box.TakeRequest(100));
    box.Post(current);
    REQUIRE_FALSE(box.Complete(old, true, 111));
    Request changed = current;
    changed.Group = 9;
    REQUIRE_FALSE(box.Complete(changed, true, 111));
    changed = current; changed.Initiator = 9;
    REQUIRE_FALSE(box.Complete(changed, true, 111));
    changed = current; changed.Created = 9;
    REQUIRE_FALSE(box.Complete(changed, true, 111));
    REQUIRE(box.Complete(current, false, 111));
    box.Post({1, 5, 3, 112});
    REQUIRE_FALSE(box.TakeReply(113));
}
TEST_CASE("Ready check lifetime rejects expiry and handles clock wrap", "[PlayerbotReadyCheck]")
{
    Mailbox box;
    Request request{1, 2, 3, 100};
    REQUIRE(Current(request, 30099));
    REQUIRE_FALSE(Current(request, 30100));
    REQUIRE_FALSE(Current({0, 2, 3, 100}, 100));
    REQUIRE_FALSE(Current({1, 0, 3, 100}, 100));
    REQUIRE_FALSE(Current({1, 2, 0, 100}, 100));
    box.Post(request);
    REQUIRE_FALSE(box.TakeRequest(30100));
    REQUIRE_FALSE(box.Complete(request, true, 30100));
    request.Created = 0xfffffff0u;
    box.Post(request);
    REQUIRE(box.TakeRequest(10));
    REQUIRE(box.Complete(request, true, 10));
    REQUIRE_FALSE(box.TakeReply(uint32_t(request.Created + Lifetime)));
}
TEST_CASE("Basic readiness uses donor thresholds without inventing supply readiness", "[PlayerbotReadyCheck]")
{
    auto ready = [](float hp = 100, float mp = 100, bool mana = true, bool nearby = true,
        bool alive = true, bool combat = false, bool transfer = false, bool cast = false, bool available = true)
    { return BasicReadiness(available, alive, combat, transfer, cast, hp, mana, mp, nearby); };
    REQUIRE(ready());
    REQUIRE_FALSE(ready(85));
    REQUIRE_FALSE(ready(100, 65));
    REQUIRE(ready(100, 0, false));
    REQUIRE_FALSE(ready(100, 100, true, false));
    REQUIRE_FALSE(ready(100, 100, true, true, false));
    REQUIRE_FALSE(ready(100, 100, true, true, true, true));
    REQUIRE_FALSE(ready(100, 100, true, true, true, false, true));
    REQUIRE_FALSE(ready(100, 100, true, true, true, false, false, true));
    REQUIRE_FALSE(ready(100, 100, true, true, true, false, false, false, false));
}
