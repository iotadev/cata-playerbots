/*
 * Adapted from AzerothCore mod-playerbots WarriorAiObjectContext.cpp and
 * GenericWarriorNonCombatStrategy.cpp at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "WarriorAiObjectContext.h"
#include "../../../Bot/PlayerbotAI.h"
#include "../../Base/PlayerbotCombatDecision.h"
#include "Player.h"

namespace
{
class WarriorNonCombatStrategy final : public Strategy
{
public:
    explicit WarriorNonCombatStrategy(PlayerbotAI* ai) : Strategy(ai) { }
    std::string const getName() override { return "nc"; }
    uint32_t GetType() const override { return STRATEGY_TYPE_NONCOMBAT; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("battle shout", {NextAction("battle shout", ACTION_NORMAL)}));
    }
};

bool NeedsBattleShout(Player const* bot)
{
    return bot && bot->getClass() == CLASS_WARRIOR && bot->IsAlive() && !bot->IsInCombat() &&
        bot->HasSpell(6673) && !bot->HasAura(6673);
}

class BattleShoutTrigger final : public Trigger
{
public:
    explicit BattleShoutTrigger(PlayerbotAI* ai) : Trigger(ai, "battle shout", 2) { }
    bool IsActive() override { return NeedsBattleShout(botAI ? botAI->GetBot() : nullptr); }
    bool IsBuffTrigger() override { return true; }
};

class BattleShoutAction final : public Action
{
public:
    explicit BattleShoutAction(PlayerbotAI* ai) : Action(ai, "battle shout") { }
    bool isUseful() override { return NeedsBattleShout(botAI ? botAI->GetBot() : nullptr); }
    bool Execute([[maybe_unused]] Event event) override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        return NeedsBattleShout(bot) && PlayerbotDecision::TryCast(*bot, *bot, 6673, "Battle Shout");
    }
};

struct SharedWarriorContexts
{
    SharedNamedObjectContextList<Strategy> strategies;
    SharedNamedObjectContextList<Action> actions;
    SharedNamedObjectContextList<Trigger> triggers;
    SharedNamedObjectContextList<UntypedValue> values;

    SharedWarriorContexts()
    {
        auto* strategyFactory = new NamedObjectContext<Strategy>();
        strategyFactory->creators["nc"] = [](PlayerbotAI* ai) { return new WarriorNonCombatStrategy(ai); };
        strategies.Add(strategyFactory);

        auto* actionFactory = new NamedObjectContext<Action>();
        actionFactory->creators["battle shout"] = [](PlayerbotAI* ai) { return new BattleShoutAction(ai); };
        actions.Add(actionFactory);

        auto* triggerFactory = new NamedObjectContext<Trigger>();
        triggerFactory->creators["battle shout"] = [](PlayerbotAI* ai) { return new BattleShoutTrigger(ai); };
        triggers.Add(triggerFactory);
    }
};

SharedWarriorContexts& Shared()
{
    static SharedWarriorContexts contexts;
    return contexts;
}
}

WarriorAiObjectContext::WarriorAiObjectContext(PlayerbotAI* botAI)
    : AiObjectContext(botAI, Shared().strategies, Shared().actions, Shared().triggers, Shared().values) { }
