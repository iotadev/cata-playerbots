/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/Engine/Engine.h"
#include "../src/Bot/Engine/StateEngines.h"
#include "../src/Ai/Base/PlayerbotSpecStrategy.h"
#include "../src/Ai/Base/PlayerbotRoles.h"
#include "../src/Ai/Class/Priest/PlayerbotPriestDpsStrategy.h"
#include "../src/Ai/Base/PlayerbotThreatStrategy.h"
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
class NamedStrategy final : public Strategy
{
public:
    NamedStrategy(PlayerbotAI* ai, std::string name) : Strategy(ai), name(std::move(name)) { }
    std::string const getName() override { return name; }
private:
    std::string name;
};
class RoleStrategy final : public Strategy
{
public:
    RoleStrategy(PlayerbotAI* ai, std::string name, uint32 mask) : Strategy(ai), name(std::move(name)), mask(mask) { }
    std::string const getName() override { return name; }
    uint32_t GetType() const override { return mask; }
private:
    std::string name;
    uint32 mask;
};

class ExclusionStrategy final : public Strategy
{
public:
    ExclusionStrategy(PlayerbotAI* ai, GuidSet& targets) : Strategy(ai), targets(targets) { }
    std::string const getName() override { return "exclusion"; }
    bool HasTargetExclusions() const override { return true; }
    void AppendTargetExclusions(GuidSet& result, TargetValueExclusionType type) override
    {
        if (type == TargetValueExclusionType::Dps) result.insert(targets.begin(), targets.end());
    }
private:
    GuidSet& targets;
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

class HealerPolicyAction final : public Action
{
public:
    HealerPolicyAction(PlayerbotAI* ai, int& count, bool& target, bool& healing, float& mana)
        : Action(ai, "fallback"), count(count), target(target), healing(healing), mana(mana) { }
    bool isUseful() override { return PlayerbotPriestDps::CanAttack(true, true, target, healing, mana); }
    bool Execute([[maybe_unused]] Event event) override { ++count; return true; }
private:
    int& count;
    bool& target;
    bool& healing;
    float& mana;
};
class ThreatDamageAction final : public Action
{
public:
    ThreatDamageAction(PlayerbotAI* ai, int& count) : Action(ai, "primary"), count(count) { }
    ActionThreatType getThreatType() override { return ActionThreatType::Single; }
    bool Execute([[maybe_unused]] Event event) override { ++count; return true; }
private:
    int& count;
};
class PolicyThreatMultiplier final : public Multiplier
{
public:
    PolicyThreatMultiplier(PlayerbotAI* ai, uint8_t& percent) : Multiplier(ai, "threat"), percent(percent) { }
    float GetValue(Action* action) override
    {
        return PlayerbotThreat::DamageMultiplier(action->getThreatType(), true, percent);
    }
private:
    uint8_t& percent;
};
class PolicyThreatStrategy final : public Strategy
{
public:
    PolicyThreatStrategy(PlayerbotAI* ai, uint8_t& percent) : Strategy(ai), percent(percent) { }
    std::string const getName() override { return "policy threat"; }
    std::vector<NextAction> getDefaultActions() override
    {
        return {NextAction("primary", 20.0f), NextAction("fallback", 10.0f)};
    }
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override
    {
        multipliers.push_back(new PolicyThreatMultiplier(botAI, percent));
    }
private:
    uint8_t& percent;
};
}

TEST_CASE("Playerbot engine threat policy yields damage to support then resumes at lower threat", "[playerbot][engine][threat]")
{
    Fixture fixture;
    uint8_t percent = 80;
    fixture.strategies.creators["policy threat"] = [&](PlayerbotAI* ai) { return new PolicyThreatStrategy(ai, percent); };
    fixture.actions.creators["primary"] = [&](PlayerbotAI* ai) { return new ThreatDamageAction(ai, fixture.counts.primary); };
    Engine engine(nullptr, fixture.context);
    engine.AddStrategy("policy threat");
    REQUIRE(engine.Tick());
    REQUIRE(fixture.counts.primary == 0);
    REQUIRE(fixture.counts.fallback == 1); // support metadata remains None
    percent = 79;
    REQUIRE(engine.Tick());
    REQUIRE(fixture.counts.primary == 1);
    REQUIRE(fixture.counts.fallback == 1);
}
namespace
{
class FocusTestAction final : public Action
{
public:
    FocusTestAction(PlayerbotAI* ai, int& count, int& mode) : Action(ai, "primary"), count(count), mode(mode) { }
    ActionThreatType getThreatType() override { return mode < 2 ? ActionThreatType::Aoe : ActionThreatType::Single; }
    bool isHealingAction() override { return mode == 1; }
    bool isDebuffOnAttacker() override { return mode == 3; }
    bool Execute([[maybe_unused]] Event event) override { ++count; return true; }
private:
    int& count;
    int& mode;
};
}
TEST_CASE("Playerbot engine focus blocks area and attacker debuffs while retaining healing and single target actions", "[playerbot][engine][focus]")
{
    Fixture fixture;
    int mode = 0;
    fixture.strategies.creators["focus"] = [](PlayerbotAI* ai) { return new PlayerbotThreat::FocusStrategy(ai); };
    fixture.actions.creators["primary"] = [&](PlayerbotAI* ai) { return new FocusTestAction(ai, fixture.counts.primary, mode); };
    Engine engine(nullptr, fixture.context);
    engine.AddStrategy("test");
    engine.AddStrategy("focus");
    REQUIRE_FALSE(engine.Tick());
    REQUIRE(fixture.counts.primary == 0);
    mode = 1; // Area healing keeps the donor exemption.
    REQUIRE(engine.Tick());
    mode = 2;
    REQUIRE(engine.Tick());
    REQUIRE(fixture.counts.primary == 2);
    mode = 3;
    REQUIRE_FALSE(engine.Tick());
    REQUIRE(fixture.counts.primary == 2);
    REQUIRE(engine.RemoveStrategy("focus"));
    mode = 0;
    REQUIRE(engine.Tick());
    REQUIRE(fixture.counts.primary == 3);
}

TEST_CASE("Playerbot active engine gathers dynamic typed exclusions and clears removed strategies", "[playerbot][engine][target]")
{
    Fixture fixture;
    GuidSet targets{ObjectGuid(uint64(1))};
    fixture.strategies.creators["exclusion"] = [&targets](PlayerbotAI* ai) { return new ExclusionStrategy(ai, targets); };
    Engine engine(nullptr, fixture.context);
    REQUIRE(engine.GatherTargetExclusions(TargetValueExclusionType::Dps).empty());
    engine.AddStrategy("exclusion");
    REQUIRE(engine.HasTargetExclusions());
    REQUIRE(engine.GatherTargetExclusions(TargetValueExclusionType::Dps) == targets);
    REQUIRE(engine.GatherTargetExclusions(TargetValueExclusionType::Tank).empty());
    REQUIRE(engine.GatherTargetExclusions(TargetValueExclusionType::None).empty());
    targets.insert(ObjectGuid(uint64(2)));
    REQUIRE(engine.GatherTargetExclusions(TargetValueExclusionType::Dps) == targets);
    REQUIRE(engine.RemoveStrategy("exclusion"));
    REQUIRE_FALSE(engine.HasTargetExclusions());
    REQUIRE(engine.GatherTargetExclusions(TargetValueExclusionType::Dps).empty());
}

TEST_CASE("Playerbot engine rechecks queued healer damage after support eligibility changes", "[playerbot][engine][priest-dps]")
{
    Fixture fixture;
    bool target = true, healing = false;
    float mana = 100.0f;
    fixture.actions.creators["primary"] = [&fixture](PlayerbotAI* ai)
    {
        return new ContinuingAction(ai, fixture.counts.primary);
    };
    fixture.actions.creators["fallback"] = [&](PlayerbotAI* ai)
    {
        return new HealerPolicyAction(ai, fixture.counts.fallback, target, healing, mana);
    };
    Engine engine(nullptr, fixture.context);
    REQUIRE(engine.ExecuteAction("primary") == ACTION_RESULT_OK);
    REQUIRE(engine.QueuedCount() == 1);
    SECTION("healing demand") { healing = true; }
    SECTION("target cleared by control") { target = false; }
    SECTION("mana spent") { mana = 84.0f; }
    REQUIRE_FALSE(engine.Tick());
    REQUIRE(fixture.counts.fallback == 0);
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

TEST_CASE("Playerbot state transition discards prerequisites and separates defaults", "[playerbot][engine][state]")
{
    Fixture fixture;
    auto* strategy = fixture.context.GetStrategy("test");
    strategy->actionNodeFactories.creators["primary"] = [](PlayerbotAI*)
    {
        return new ActionNode("primary", {NextAction("prepare", 10.0f)});
    };
    StateEngines engines(nullptr, fixture.context);
    engines.Get(StateEngines::State::Combat).AddStrategy("test");
    REQUIRE_FALSE(engines.Active().Tick()); // combat defaults cannot leak into idle
    REQUIRE(engines.Select(StateEngines::State::Combat));
    REQUIRE(engines.Active().Tick());
    REQUIRE(fixture.counts.prepare == 1);
    REQUIRE(engines.Active().QueuedCount() > 0);
    SECTION("target lost") { engines.Select(StateEngines::State::NonCombat); }
    SECTION("death") { engines.Select(StateEngines::State::Dead); }
    REQUIRE_FALSE(engines.Active().Tick());
    REQUIRE(engines.Get(StateEngines::State::Combat).QueuedCount() == 0);
    REQUIRE(fixture.counts.primary == 0);
    engines.Select(StateEngines::State::Combat);
    REQUIRE(engines.Active().Tick()); // fresh engagement must prepare again
    REQUIRE(fixture.counts.prepare == 2);
    REQUIRE(fixture.counts.primary == 0);
    REQUIRE(engines.Active().HasStrategy("test"));
}

TEST_CASE("Combat strategy roles remain available in idle and dead states and refresh on replacement", "[playerbot][roles]")
{
    Fixture fixture;
    auto* factory = new NamedObjectContext<Strategy>();
    factory->creators["tank role"] = [](PlayerbotAI* ai)
    { return new RoleStrategy(ai, "tank role", STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_TANK | STRATEGY_TYPE_MELEE); };
    factory->creators["damage role"] = [](PlayerbotAI* ai)
    { return new RoleStrategy(ai, "damage role", STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_DPS | STRATEGY_TYPE_MELEE); };
    fixture.strategies.Add(factory);
    StateEngines engines(nullptr, fixture.context);
    Engine& combat = engines.Get(StateEngines::State::Combat);
    combat.AddStrategy("tank role");
    auto published = [&] { return combat.GetStrategyTypeMask() & PlayerbotRoles::RoleFlags; };
    REQUIRE(published() & STRATEGY_TYPE_TANK);
    REQUIRE_FALSE(engines.Active().ContainsStrategy(STRATEGY_TYPE_TANK));
    engines.Select(StateEngines::State::Dead);
    REQUIRE(published() & STRATEGY_TYPE_TANK);
    REQUIRE_FALSE(engines.Active().ContainsStrategy(STRATEGY_TYPE_TANK));
    combat.RemoveStrategy("tank role");
    combat.AddStrategy("damage role");
    REQUIRE_FALSE(published() & STRATEGY_TYPE_TANK);
    REQUIRE(published() & STRATEGY_TYPE_DPS);
}

TEST_CASE("Playerbot stop clears continuers but steady state preserves them", "[playerbot][engine][state]")
{
    Fixture fixture;
    fixture.actions.creators["primary"] = [&fixture](PlayerbotAI* ai)
    {
        return new ContinuingAction(ai, fixture.counts.primary);
    };
    StateEngines engines(nullptr, fixture.context);
    engines.Select(StateEngines::State::Combat);
    REQUIRE(engines.Active().ExecuteAction("primary") == ACTION_RESULT_OK);
    REQUIRE_FALSE(engines.Select(StateEngines::State::Combat));
    REQUIRE(engines.Active().QueuedCount() == 1);
    SECTION("uninterrupted")
    {
        REQUIRE(engines.Active().Tick());
        REQUIRE(fixture.counts.fallback == 1);
    }
    SECTION("stop or transfer without a native combat flag change")
    {
        engines.CancelPendingActions();
        REQUIRE_FALSE(engines.Active().Tick());
        REQUIRE(fixture.counts.fallback == 0);
    }
}

TEST_CASE("Playerbot recovery restores noncombat support without dead-state actions", "[playerbot][engine][state]")
{
    Fixture fixture;
    StateEngines engines(nullptr, fixture.context);
    engines.Get(StateEngines::State::NonCombat).AddStrategy("test");
    REQUIRE(engines.Active().Tick());
    REQUIRE(fixture.counts.primary == 1);
    engines.Select(StateEngines::State::Dead);
    REQUIRE_FALSE(engines.Active().Tick());
    REQUIRE(fixture.counts.primary == 1);
    engines.Select(StateEngines::State::NonCombat);
    REQUIRE(engines.Active().Tick());
    REQUIRE(fixture.counts.primary == 2);
}

TEST_CASE("Playerbot spec policy selects only implemented native Cata routes", "[playerbot][spec]")
{
    REQUIRE(std::string(PlayerbotSpec::CombatStrategy(CLASS_WARRIOR, TALENT_TREE_WARRIOR_PROTECTION)) == "tank");
    REQUIRE(std::string(PlayerbotSpec::CombatStrategy(CLASS_WARRIOR, 0)) == "warrior");
    REQUIRE(std::string(PlayerbotSpec::CombatStrategy(CLASS_WARRIOR, TALENT_TREE_WARRIOR_ARMS)) == "arms");
    REQUIRE(std::string(PlayerbotSpec::CombatStrategy(CLASS_WARRIOR, TALENT_TREE_WARRIOR_FURY)) == "fury");
    REQUIRE(std::string(PlayerbotSpec::CombatStrategy(CLASS_MAGE, TALENT_TREE_MAGE_FROST)) == "frost");
    REQUIRE(std::string(PlayerbotSpec::CombatStrategy(CLASS_MAGE, TALENT_TREE_MAGE_ARCANE)) == "arcane");
    REQUIRE(std::string(PlayerbotSpec::CombatStrategy(CLASS_MAGE, TALENT_TREE_MAGE_FIRE)) == "fire");
    REQUIRE(std::string(PlayerbotSpec::CombatStrategy(CLASS_MAGE, 0)) == "mage");
    REQUIRE(std::string(PlayerbotSpec::CombatStrategy(CLASS_PRIEST, TALENT_TREE_PRIEST_SHADOW)) == "heal");
    REQUIRE(PlayerbotSpec::CombatStrategy(CLASS_ROGUE, 0) == nullptr);
}

TEST_CASE("Playerbot spec refresh replaces combat sibling and preserves shared strategies", "[playerbot][spec]")
{
    Fixture fixture;
    auto* combat = new NamedObjectContext<Strategy>(false, true);
    for (std::string const name : {"mage", "frost", "fire", "arcane"})
        combat->creators[name] = [name](PlayerbotAI* ai) { return new NamedStrategy(ai, name); };
    fixture.strategies.Add(combat);
    Engine engine(nullptr, fixture.context);
    engine.AddStrategy("test"); // unrelated shared strategy
    REQUIRE(PlayerbotSpec::Refresh(engine, CLASS_MAGE, 0));
    REQUIRE(engine.HasStrategy("mage"));
    REQUIRE(PlayerbotSpec::Refresh(engine, CLASS_MAGE, TALENT_TREE_MAGE_FROST));
    REQUIRE(engine.HasStrategy("frost"));
    REQUIRE_FALSE(engine.HasStrategy("mage"));
    REQUIRE(engine.HasStrategy("test"));
    REQUIRE(PlayerbotSpec::Refresh(engine, CLASS_MAGE, TALENT_TREE_MAGE_FIRE));
    REQUIRE(engine.HasStrategy("fire"));
    REQUIRE_FALSE(engine.HasStrategy("frost"));
    REQUIRE(PlayerbotSpec::Refresh(engine, CLASS_MAGE, TALENT_TREE_MAGE_ARCANE));
    REQUIRE(engine.HasStrategy("arcane"));
    REQUIRE_FALSE(engine.HasStrategy("fire"));
    REQUIRE(engine.HasStrategy("test"));
    REQUIRE(PlayerbotSpec::Refresh(engine, CLASS_MAGE, 0));
    REQUIRE(engine.HasStrategy("mage"));
    REQUIRE_FALSE(engine.HasStrategy("arcane"));
}

TEST_CASE("Playerbot unchanged spec leaves queued work while a changed route resets it", "[playerbot][spec]")
{
    Fixture fixture;
    auto* combat = new NamedObjectContext<Strategy>(false, true);
    for (std::string const name : {"warrior", "arms", "fury", "tank"})
        combat->creators[name] = [name](PlayerbotAI* ai) { return new NamedStrategy(ai, name); };
    fixture.strategies.Add(combat);
    fixture.actions.creators["primary"] = [&fixture](PlayerbotAI* ai)
    {
        return new ContinuingAction(ai, fixture.counts.primary);
    };
    Engine engine(nullptr, fixture.context);
    REQUIRE(PlayerbotSpec::Refresh(engine, CLASS_WARRIOR, TALENT_TREE_WARRIOR_ARMS));
    REQUIRE(engine.ExecuteAction("primary") == ACTION_RESULT_OK);
    REQUIRE(engine.QueuedCount() == 1);
    REQUIRE_FALSE(PlayerbotSpec::Refresh(engine, CLASS_WARRIOR, TALENT_TREE_WARRIOR_ARMS));
    REQUIRE(engine.QueuedCount() == 1);
    REQUIRE(PlayerbotSpec::Refresh(engine, CLASS_WARRIOR, TALENT_TREE_WARRIOR_PROTECTION));
    REQUIRE(engine.QueuedCount() == 0);
    REQUIRE(engine.HasStrategy("tank"));
    REQUIRE_FALSE(engine.HasStrategy("arms"));
    REQUIRE(PlayerbotSpec::Refresh(engine, CLASS_WARRIOR, TALENT_TREE_WARRIOR_FURY));
    REQUIRE(engine.HasStrategy("fury"));
    REQUIRE_FALSE(engine.HasStrategy("tank"));
    REQUIRE_FALSE(engine.HasStrategy("warrior"));
    REQUIRE(PlayerbotSpec::Refresh(engine, CLASS_WARRIOR, 0));
    REQUIRE_FALSE(engine.HasStrategy("fury"));
    REQUIRE_FALSE(engine.HasStrategy("tank"));
    REQUIRE_FALSE(PlayerbotSpec::Refresh(engine, CLASS_ROGUE, 0));
}

TEST_CASE("Playerbot strategy operators apply donor add remove toggle and query", "[playerbot][engine][strategy-control]")
{
    Fixture fixture;
    Engine engine(nullptr, fixture.context);
    auto result = engine.ChangeStrategies(" +test , ? ", {"test"});
    REQUIRE(result.Status == Engine::StrategyChangeStatus::Changed);
    REQUIRE(result.Query);
    REQUIRE(engine.ContainsStrategy(STRATEGY_TYPE_COMBAT));
    REQUIRE(engine.ChangeStrategies("~test", {"test"}).Status == Engine::StrategyChangeStatus::Changed);
    REQUIRE_FALSE(engine.HasStrategy("test"));
    REQUIRE_FALSE(engine.ContainsStrategy(STRATEGY_TYPE_COMBAT));
    REQUIRE(engine.ChangeStrategies("-test", {"test"}).Status == Engine::StrategyChangeStatus::Unchanged);
    REQUIRE(engine.ChangeStrategies("~test", {"test"}).Status == Engine::StrategyChangeStatus::Changed);
    REQUIRE(engine.HasStrategy("test"));
}

TEST_CASE("Playerbot invalid strategy batch leaves registrations and continuers intact", "[playerbot][engine][strategy-control]")
{
    Fixture fixture;
    fixture.actions.creators["primary"] = [&fixture](PlayerbotAI* ai)
    { return new ContinuingAction(ai, fixture.counts.primary); };
    Engine engine(nullptr, fixture.context);
    engine.AddStrategy("test");
    REQUIRE(engine.ExecuteAction("primary") == ACTION_RESULT_OK);
    for (std::string const command : {"", "-test,+unknown", "-test,,?", "-test,", "!", "+", "?test", "test", "-test,\n?"})
    {
        auto result = engine.ChangeStrategies(command, {"test", "unknown"});
        REQUIRE(result.Status == Engine::StrategyChangeStatus::Rejected);
        REQUIRE_FALSE(result.Query);
        REQUIRE(engine.HasStrategy("test"));
        REQUIRE(engine.QueuedCount() == 1);
        REQUIRE(engine.GetLastAction() == "primary");
    }
}

TEST_CASE("Playerbot strategy query and net unchanged batch preserve queued work", "[playerbot][engine][strategy-control]")
{
    Fixture fixture;
    fixture.actions.creators["primary"] = [&fixture](PlayerbotAI* ai)
    { return new ContinuingAction(ai, fixture.counts.primary); };
    Engine engine(nullptr, fixture.context);
    engine.AddStrategy("test");
    REQUIRE(engine.ExecuteAction("primary") == ACTION_RESULT_OK);
    REQUIRE(engine.ChangeStrategies("?", {}).Query);
    REQUIRE(engine.ChangeStrategies("-test,+test,?", {"test"}).Status == Engine::StrategyChangeStatus::Unchanged);
    REQUIRE(engine.QueuedCount() == 1);
    REQUIRE(engine.ChangeStrategies("-test,?", {"test"}).Status == Engine::StrategyChangeStatus::Changed);
    REQUIRE(engine.QueuedCount() == 0);
    REQUIRE(engine.GetLastAction().empty());
}

TEST_CASE("Playerbot strategy sibling replacement cannot bypass caller allowlist", "[playerbot][engine][strategy-control]")
{
    Fixture fixture;
    auto* siblings = new NamedObjectContext<Strategy>(false, true);
    siblings->creators["tank"] = [](PlayerbotAI* ai)
    { return new RoleStrategy(ai, "tank", STRATEGY_TYPE_TANK); };
    siblings->creators["damage"] = [](PlayerbotAI* ai)
    { return new RoleStrategy(ai, "damage", STRATEGY_TYPE_DPS); };
    siblings->creators["alias"] = [](PlayerbotAI* ai)
    { return new NamedStrategy(ai, "damage"); };
    fixture.strategies.Add(siblings);
    Engine engine(nullptr, fixture.context);
    engine.AddStrategy("tank");
    REQUIRE(engine.ChangeStrategies("-tank", {}).Status == Engine::StrategyChangeStatus::Rejected);
    REQUIRE(engine.ChangeStrategies("+damage", {"damage"}).Status == Engine::StrategyChangeStatus::Rejected);
    REQUIRE(engine.ChangeStrategies("+alias", {"tank", "damage", "alias"}).Status == Engine::StrategyChangeStatus::Rejected);
    REQUIRE(engine.HasStrategy("tank"));
    REQUIRE(engine.ChangeStrategies("+damage", {"tank", "damage"}).Status == Engine::StrategyChangeStatus::Changed);
    REQUIRE_FALSE(engine.HasStrategy("tank"));
    REQUIRE(engine.ContainsStrategy(STRATEGY_TYPE_DPS));
    REQUIRE_FALSE(engine.ContainsStrategy(STRATEGY_TYPE_TANK));
}

TEST_CASE("Playerbot strategy changes are isolated to the selected state engine", "[playerbot][engine][strategy-control]")
{
    Fixture fixture;
    StateEngines engines(nullptr, fixture.context);
    for (auto state : {StateEngines::State::NonCombat, StateEngines::State::Combat, StateEngines::State::Dead})
        engines.Get(state).AddStrategy("test");
    REQUIRE(engines.Get(StateEngines::State::Combat).ChangeStrategies("-test", {"test"}).Status == Engine::StrategyChangeStatus::Changed);
    REQUIRE(engines.Get(StateEngines::State::NonCombat).HasStrategy("test"));
    REQUIRE(engines.Get(StateEngines::State::Dead).HasStrategy("test"));
    REQUIRE(engines.Current() == StateEngines::State::NonCombat);
}

TEST_CASE("Playerbot strategy commands bound bytes and operation count", "[playerbot][engine][strategy-control]")
{
    Fixture fixture;
    Engine engine(nullptr, fixture.context);
    std::string queries = "?";
    for (unsigned i = 1; i < 16; ++i) queries += ",?";
    REQUIRE(engine.ChangeStrategies(queries, {}).Status == Engine::StrategyChangeStatus::Unchanged);
    REQUIRE(engine.ChangeStrategies(queries + ",?", {}).Status == Engine::StrategyChangeStatus::Rejected);
    REQUIRE(engine.ChangeStrategies(std::string(249, ' ') + "?", {}).Query);
    REQUIRE(engine.ChangeStrategies(std::string(250, ' ') + "?", {}).Status == Engine::StrategyChangeStatus::Rejected);
    REQUIRE(engine.ChangeStrategies("+test::qualifier", {"test::qualifier"}).Status == Engine::StrategyChangeStatus::Rejected);
}
