/* Released under GPL v2 or later. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotPotionStrategy.h"
#include "../src/Bot/Engine/Engine.h"
#include <catch2/catch.hpp>
#include <limits>

TEST_CASE("Playerbot combat potion thresholds follow donor defaults", "[playerbot][potions]")
{
    using namespace PlayerbotPotion;
    REQUIRE(Needs(Kind::Healing, 24.9f, 100, false));
    REQUIRE_FALSE(Needs(Kind::Healing, 25, 0, true));
    REQUIRE(Needs(Kind::Mana, 100, 39.9f, true));
    REQUIRE(Needs(Kind::Mana, 100, 0, true));
    REQUIRE_FALSE(Needs(Kind::Mana, 100, 40, true));
    REQUIRE_FALSE(Needs(Kind::Mana, 100, 0, false));
    REQUIRE_FALSE(Needs(Kind::Healing, 0, 0, true));
    REQUIRE_FALSE(Needs(Kind::Mana, 100, -1, true));
    REQUIRE_FALSE(Needs(Kind::Healing, std::numeric_limits<float>::quiet_NaN(), 0, true));
    REQUIRE_FALSE(Needs(Kind::Mana, 100, std::numeric_limits<float>::infinity(), true));
}
TEST_CASE("Playerbot combat potion policy rejects disabled busy and exhausted states", "[playerbot][potions]")
{
    using PlayerbotPotion::CanUse;
    REQUIRE(CanUse(true, true, true, false, false, false, false));
    REQUIRE_FALSE(CanUse(false, true, true, false, false, false, false));
    REQUIRE_FALSE(CanUse(true, false, true, false, false, false, false));
    REQUIRE_FALSE(CanUse(true, true, false, false, false, false, false));
    REQUIRE_FALSE(CanUse(true, true, true, true, false, false, false));
    REQUIRE_FALSE(CanUse(true, true, true, false, true, false, false));
    REQUIRE_FALSE(CanUse(true, true, true, false, false, true, false));
    REQUIRE_FALSE(CanUse(true, true, true, false, false, false, true));
}
TEST_CASE("Playerbot combat potion strategy retains donor emergency priorities", "[playerbot][potions]")
{
    PlayerbotPotion::PotionStrategy strategy(nullptr);
    REQUIRE(strategy.getName() == "potions");
    std::vector<TriggerNode*> nodes;
    strategy.InitTriggers(nodes);
    REQUIRE(nodes.size() == 2);
    REQUIRE(nodes[0]->getName() == "potion critical health");
    REQUIRE(nodes[1]->getName() == "potion medium mana");
    auto healing = nodes[0]->getHandlers();
    auto mana = nodes[1]->getHandlers();
    REQUIRE(healing.size() == 1);
    REQUIRE(mana.size() == 1);
    REQUIRE(healing[0].getName() == "healthstone");
    REQUIRE(healing[0].getRelevance() == ACTION_MEDIUM_HEAL + 1);
    REQUIRE(mana[0].getName() == "mana potion");
    REQUIRE(mana[0].getRelevance() == ACTION_EMERGENCY);
    for (TriggerNode* node : nodes) delete node;
}
TEST_CASE("Playerbot healthstone classification and lockout remain distinct from potions", "[playerbot][potions]")
{
    using namespace PlayerbotPotion;
    REQUIRE(MatchesItem(true, false, 6262, Kind::Healthstone));
    REQUIRE_FALSE(MatchesItem(false, false, 6262, Kind::Healthstone));
    REQUIRE_FALSE(MatchesItem(true, true, 6262, Kind::Healthstone));
    REQUIRE_FALSE(MatchesItem(true, false, 1, Kind::Healthstone));
    REQUIRE_FALSE(MatchesItem(true, false, 6262, Kind::Healing));
    REQUIRE(MatchesItem(true, true, 1, Kind::Healing));
    REQUIRE(MatchesItem(true, true, 1, Kind::Mana));
    REQUIRE(Needs(Kind::Healthstone, 24.9f, 100, false));
    REQUIRE_FALSE(Needs(Kind::Healthstone, 25, 0, true));
    REQUIRE_FALSE(PotionLockoutApplies(Kind::Healthstone, true));
    REQUIRE(PotionLockoutApplies(Kind::Healing, true));
    REQUIRE(PotionLockoutApplies(Kind::Mana, true));
    REQUIRE_FALSE(PotionLockoutApplies(Kind::Healing, false));
    PotionStrategy strategy(nullptr);
    std::unique_ptr<ActionNode> node(strategy.GetAction("healthstone"));
    REQUIRE(node);
    auto alternatives = node->getAlternatives();
    REQUIRE(alternatives.size() == 1);
    REQUIRE(alternatives[0].getName() == "healing potion");
}
namespace
{
class RecoveryTestTrigger final : public Trigger
{
public:
    explicit RecoveryTestTrigger(PlayerbotAI* ai) : Trigger(ai, "potion critical health") { }
    bool IsActive() override { return true; }
};
class RecoveryTestAction final : public Action
{
public:
    RecoveryTestAction(PlayerbotAI* ai, std::string name, int& attempts, bool possible, bool succeeds)
        : Action(ai, std::move(name)), attempts(attempts), possible(possible), succeeds(succeeds) { }
    bool isPossible() override { return possible; }
    bool Execute([[maybe_unused]] Event event) override { ++attempts; return succeeds; }
private:
    int& attempts;
    bool possible, succeeds;
};
}
TEST_CASE("Playerbot engine prefers healthstone and uses donor potion fallback when unavailable or rejected", "[playerbot][potions][engine]")
{
    for (int outcome = 0; outcome < 3; ++outcome)
    {
        CAPTURE(outcome);
        int stones = 0, potions = 0;
        SharedNamedObjectContextList<Strategy> strategies;
        SharedNamedObjectContextList<Action> actions;
        SharedNamedObjectContextList<Trigger> triggers;
        SharedNamedObjectContextList<UntypedValue> values;
        auto* strategyFactory = new NamedObjectContext<Strategy>();
        strategyFactory->creators["potions"] = [](PlayerbotAI* ai) { return new PlayerbotPotion::PotionStrategy(ai); };
        strategies.Add(strategyFactory);
        auto* actionFactory = new NamedObjectContext<Action>();
        actionFactory->creators["healthstone"] = [&](PlayerbotAI* ai)
        { return new RecoveryTestAction(ai, "healthstone", stones, outcome != 1, outcome == 0); };
        actionFactory->creators["healing potion"] = [&](PlayerbotAI* ai)
        { return new RecoveryTestAction(ai, "healing potion", potions, true, true); };
        actions.Add(actionFactory);
        auto* triggerFactory = new NamedObjectContext<Trigger>();
        triggerFactory->creators["potion critical health"] = [](PlayerbotAI* ai) { return new RecoveryTestTrigger(ai); };
        triggers.Add(triggerFactory);
        AiObjectContext context(nullptr, strategies, actions, triggers, values);
        Engine engine(nullptr, context);
        engine.AddStrategy("potions");
        REQUIRE(engine.Tick());
        REQUIRE(stones == (outcome == 1 ? 0 : 1));
        REQUIRE(potions == (outcome == 0 ? 0 : 1));
        REQUIRE(engine.GetLastAction() == (outcome == 0 ? "healthstone" : "healing potion"));
    }
}
