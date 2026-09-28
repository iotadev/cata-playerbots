/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/Engine/AiObjectContext.h"
#include <catch2/catch.hpp>
#include <string>

namespace
{
class PersistedValue final : public ManualSetValue<int>
{
public:
    explicit PersistedValue(PlayerbotAI* ai) : ManualSetValue<int>(ai, 7, "counter") { }
    std::string const Format() override { return std::to_string(Get()); }
    std::string const Save() override { return Format(); }
    bool Load(std::string const value) override
    {
        try
        {
            Set(std::stoi(value));
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
};

class TestAction final : public Action
{
public:
    explicit TestAction(PlayerbotAI* ai) : Action(ai, "test action") { }
};

class TestTrigger final : public Trigger
{
public:
    explicit TestTrigger(PlayerbotAI* ai) : Trigger(ai, "test trigger") { }
};

class TestStrategy final : public Strategy
{
public:
    explicit TestStrategy(PlayerbotAI* ai) : Strategy(ai) { }
    std::string const getName() override { return "test strategy"; }
};
}

TEST_CASE("Playerbot AI context keeps typed objects per bot and persists values", "[playerbot][engine][context]")
{
    SharedNamedObjectContextList<Strategy> strategies;
    SharedNamedObjectContextList<Action> actions;
    SharedNamedObjectContextList<Trigger> triggers;
    SharedNamedObjectContextList<UntypedValue> values;

    auto* strategyFactory = new NamedObjectContext<Strategy>();
    strategyFactory->creators["test strategy"] = [](PlayerbotAI* ai) { return new TestStrategy(ai); };
    strategies.Add(strategyFactory);
    auto* actionFactory = new NamedObjectContext<Action>();
    actionFactory->creators["test action"] = [](PlayerbotAI* ai) { return new TestAction(ai); };
    actions.Add(actionFactory);
    auto* triggerFactory = new NamedObjectContext<Trigger>();
    triggerFactory->creators["test trigger"] = [](PlayerbotAI* ai) { return new TestTrigger(ai); };
    triggers.Add(triggerFactory);
    auto* valueFactory = new NamedObjectContext<UntypedValue>();
    valueFactory->creators["counter"] = [](PlayerbotAI* ai) { return new PersistedValue(ai); };
    values.Add(valueFactory);

    AiObjectContext first(nullptr, strategies, actions, triggers, values);
    AiObjectContext second(nullptr, strategies, actions, triggers, values);
    REQUIRE(first.GetStrategy("test strategy")->getName() == "test strategy");
    REQUIRE(first.GetAction("test action")->getName() == "test action");
    REQUIRE(first.GetTrigger("test trigger")->getName() == "test trigger");
    REQUIRE(first.GetAction("test action") == first.GetAction("test action"));
    REQUIRE(first.GetAction("test action") != second.GetAction("test action"));
    REQUIRE(first.GetSupportedStrategies().count("test strategy") == 1);
    REQUIRE(first.GetSupportedActions().count("test action") == 1);

    Value<int>* firstValue = first.GetValue<int>("counter");
    Value<int>* secondValue = second.GetValue<int>("counter");
    REQUIRE(firstValue != nullptr);
    REQUIRE(secondValue != nullptr);
    REQUIRE(first.GetValue<std::string>("counter") == nullptr);
    firstValue->Set(19);
    REQUIRE(secondValue->Get() == 7);
    REQUIRE(first.FormatValues() == "{counter=19}");
    REQUIRE(first.Save() == std::vector<std::string>{"counter>19"});
    second.Load(first.Save());
    REQUIRE(secondValue->Get() == 19);
    second.Load({"invalid", "counter>not-a-number"});
    REQUIRE(secondValue->Get() == 19);
}
