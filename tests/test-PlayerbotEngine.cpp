/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/Engine/Engine.h"
#include <catch2/catch.hpp>

namespace
{
struct Counts
{
    int prepare = 0;
    int primary = 0;
    int fallback = 0;
};

class CountAction final : public Action
{
public:
    CountAction(PlayerbotAI* ai, std::string name, int& count, bool succeeds = true)
        : Action(ai, std::move(name)), count(count), succeeds(succeeds) { }
    bool Execute([[maybe_unused]] Event event) override
    {
        ++count;
        return succeeds;
    }
private:
    int& count;
    bool succeeds;
};

class ContinuingAction final : public Action
{
public:
    ContinuingAction(PlayerbotAI* ai, int& count) : Action(ai, "primary"), count(count) { }
    bool Execute([[maybe_unused]] Event event) override { ++count; return true; }
    std::vector<NextAction> getContinuers() override { return {NextAction("fallback", 10.0f)}; }
private:
    int& count;
};

class TestStrategy final : public Strategy
{
public:
    TestStrategy(PlayerbotAI* ai, bool triggerOnly)
        : Strategy(ai), triggerOnly(triggerOnly) { }
    std::string const getName() override { return "test"; }
    uint32_t GetType() const override { return STRATEGY_TYPE_COMBAT; }
    std::vector<NextAction> getDefaultActions() override
    {
        return triggerOnly ? std::vector<NextAction>{} : std::vector<NextAction>{NextAction("primary", 10.0f)};
    }
    void InitTriggers(std::vector<TriggerNode*>& nodes) override
    {
        if (triggerOnly)
            nodes.push_back(new TriggerNode("ready", {NextAction("primary", 100.0f)}));
    }
private:
    bool triggerOnly;
};

class ReadyTrigger final : public Trigger
{
public:
    explicit ReadyTrigger(PlayerbotAI* ai) : Trigger(ai, "ready") { }
    bool IsActive() override { return true; }
};

class VetoListener final : public ActionExecutionListener
{
public:
    explicit VetoListener(int& afterCount) : afterCount(afterCount) { }
    bool Before([[maybe_unused]] Action* action, [[maybe_unused]] Event event) override { return true; }
    bool AllowExecution([[maybe_unused]] Action* action, [[maybe_unused]] Event event) override { return false; }
    void After([[maybe_unused]] Action* action, bool executed, [[maybe_unused]] Event event) override
    {
        ++afterCount;
        REQUIRE(executed);
    }
    bool OverrideResult([[maybe_unused]] Action* action, bool executed, [[maybe_unused]] Event event) override
    {
        return executed;
    }
private:
    int& afterCount;
};

struct Fixture
{
    SharedNamedObjectContextList<Strategy> strategies;
    SharedNamedObjectContextList<Action> actions;
    SharedNamedObjectContextList<Trigger> triggers;
    SharedNamedObjectContextList<UntypedValue> values;
    Counts counts;
    AiObjectContext context;

    Fixture(bool triggerOnly = false, bool primarySucceeds = true)
        : context(nullptr, strategies, actions, triggers, values)
    {
        auto* strategyFactory = new NamedObjectContext<Strategy>();
        strategyFactory->creators["test"] = [triggerOnly](PlayerbotAI* ai) { return new TestStrategy(ai, triggerOnly); };
        strategies.Add(strategyFactory);

        auto* actionFactory = new NamedObjectContext<Action>();
        actionFactory->creators["prepare"] = [this](PlayerbotAI* ai) { return new CountAction(ai, "prepare", counts.prepare); };
        actionFactory->creators["primary"] = [this, primarySucceeds](PlayerbotAI* ai)
        {
            return new CountAction(ai, "primary", counts.primary, primarySucceeds);
        };
        actionFactory->creators["fallback"] = [this](PlayerbotAI* ai) { return new CountAction(ai, "fallback", counts.fallback); };
        actions.Add(actionFactory);

        auto* triggerFactory = new NamedObjectContext<Trigger>();
        triggerFactory->creators["ready"] = [](PlayerbotAI* ai) { return new ReadyTrigger(ai); };
        triggers.Add(triggerFactory);
    }
};
}

TEST_CASE("Playerbot engine schedules prerequisites before the primary action", "[playerbot][engine]")
{
    Fixture fixture;
    auto* strategy = dynamic_cast<TestStrategy*>(fixture.context.GetStrategy("test"));
    strategy->actionNodeFactories.creators["primary"] = [](PlayerbotAI*)
    {
        return new ActionNode("primary", {NextAction("prepare", 10.0f)});
    };
    Engine engine(nullptr, fixture.context);
    engine.AddStrategy("test");
    REQUIRE(engine.ContainsStrategy(STRATEGY_TYPE_COMBAT));
    REQUIRE(engine.Tick());
    REQUIRE(fixture.counts.prepare == 1);
    REQUIRE(fixture.counts.primary == 0);
    REQUIRE(engine.Tick());
    REQUIRE(fixture.counts.primary == 1);
    REQUIRE(engine.GetLastAction() == "primary");
}

TEST_CASE("Playerbot engine schedules an alternative after failed execution", "[playerbot][engine]")
{
    Fixture fixture(false, false);
    auto* strategy = dynamic_cast<TestStrategy*>(fixture.context.GetStrategy("test"));
    strategy->actionNodeFactories.creators["primary"] = [](PlayerbotAI*)
    {
        return new ActionNode("primary", {}, {NextAction("fallback", 10.0f)});
    };
    Engine engine(nullptr, fixture.context);
    engine.AddStrategy("test");
    REQUIRE(engine.Tick());
    REQUIRE(fixture.counts.primary == 1);
    REQUIRE(fixture.counts.fallback == 1);
    REQUIRE(engine.GetLastAction() == "fallback");
}

TEST_CASE("Playerbot engine fires registered triggers and honors a minimal tick", "[playerbot][engine]")
{
    Fixture fixture(true);
    Engine engine(nullptr, fixture.context);
    engine.AddStrategy("test");
    REQUIRE(engine.Tick(true));
    REQUIRE(fixture.counts.primary == 1);
    REQUIRE(engine.ExecuteAction("missing") == ACTION_RESULT_UNKNOWN);
}

TEST_CASE("Playerbot engine listeners can veto action execution without losing callbacks", "[playerbot][engine]")
{
    Fixture fixture;
    Engine engine(nullptr, fixture.context);
    int afterCount = 0;
    engine.AddActionExecutionListener(std::make_unique<VetoListener>(afterCount));
    REQUIRE(engine.ExecuteAction("primary") == ACTION_RESULT_OK);
    REQUIRE(fixture.counts.primary == 0);
    REQUIRE(afterCount == 1);
}

TEST_CASE("Playerbot minimal tick leaves low-priority queued work for a normal tick", "[playerbot][engine]")
{
    Fixture fixture;
    fixture.actions.creators["primary"] = [&fixture](PlayerbotAI* ai)
    {
        return new ContinuingAction(ai, fixture.counts.primary);
    };
    Engine engine(nullptr, fixture.context);
    REQUIRE(engine.ExecuteAction("primary") == ACTION_RESULT_OK);
    REQUIRE(engine.QueuedCount() == 1);
    REQUIRE_FALSE(engine.Tick(true));
    REQUIRE(engine.QueuedCount() == 1);
    REQUIRE(engine.Tick());
    REQUIRE(fixture.counts.fallback == 1);
}
