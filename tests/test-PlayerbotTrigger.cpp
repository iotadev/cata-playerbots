/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/Engine/Trigger/Trigger.h"
#include <catch2/catch.hpp>

namespace
{
class TestTrigger final : public Trigger
{
public:
    explicit TestTrigger(int32_t interval = 1) : Trigger(nullptr, "test trigger", interval) { }
    bool active = false;
    bool buff = false;
    bool debuff = false;
    bool IsActive() override { return active; }
    bool IsBuffTrigger() override { return buff; }
    bool IsDebuffTrigger() override { return debuff; }
    std::vector<NextAction> getHandlers() override { return { NextAction("from trigger", 2.0f) }; }
};
}

TEST_CASE("Playerbot trigger check emits a named event only while active", "[playerbot][engine][trigger]")
{
    TestTrigger trigger;
    REQUIRE(!trigger.Check());
    trigger.active = true;
    Event event = trigger.Check();
    REQUIRE_FALSE(!event);
    REQUIRE(event.GetSource() == "test trigger");
}

TEST_CASE("Playerbot trigger intervals preserve donor second-to-millisecond rule", "[playerbot][engine][trigger]")
{
    TestTrigger trigger(2);
    REQUIRE(trigger.needCheck(1000));
    REQUIRE_FALSE(trigger.needCheck(2999));
    REQUIRE(trigger.needCheck(3000));
    trigger.Reset();
    REQUIRE(trigger.needCheck(3001));
}

TEST_CASE("Playerbot force-rebuff bypass applies only to out-of-combat buff triggers", "[playerbot][engine][trigger]")
{
    TestTrigger trigger(10000);
    trigger.buff = true;
    REQUIRE(trigger.needCheck(1000));
    REQUIRE_FALSE(trigger.needCheck(1001));
    REQUIRE(trigger.needCheck(1001, true, false));
    REQUIRE_FALSE(trigger.needCheck(1001, true, true));
    trigger.debuff = true;
    REQUIRE_FALSE(trigger.needCheck(1001, true, false));
}

TEST_CASE("Playerbot trigger nodes keep configured handlers before trigger handlers", "[playerbot][engine][trigger]")
{
    TestTrigger trigger;
    TriggerNode node("test", { NextAction("from node", 5.0f) });
    REQUIRE(node.getFirstRelevance() == 5.0f);
    node.setTrigger(&trigger);
    auto handlers = node.getHandlers();
    REQUIRE(handlers.size() == 2);
    REQUIRE(handlers[0].getName() == "from node");
    REQUIRE(handlers[1].getName() == "from trigger");
    REQUIRE(handlers[1].getRelevance() == 2.0f);
}
