/*
 * Adapted from AzerothCore mod-playerbots WarriorAiObjectContext.cpp and
 * GenericWarriorNonCombatStrategy.cpp, GenericWarriorStrategy.cpp and
 * TankWarriorStrategy.cpp at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "WarriorAiObjectContext.h"
#include "../../../Bot/PlayerbotAI.h"
#include "../../Base/PlayerbotCombatDecision.h"
#include "Creature.h"
#include "Group.h"
#include "Player.h"
#include <cmath>

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

bool ValidWarriorTarget(Player* bot, Creature* target)
{
    return bot && target && bot->getClass() == CLASS_WARRIOR && bot->IsAlive() &&
        target->IsAlive() && bot->GetVictim() == target && bot->IsValidAttackTarget(target) &&
        target->IsWithinMeleeRange(bot) && bot->IsWithinLOSInMap(target) &&
        bot->HasInArc(2.0f * float(M_PI) / 3.0f, target);
}

bool VictoryRushReady(Player& bot, Creature const&)
{
    return bot.HasAura(32216) || bot.HasAura(82368);
}

bool RendMissing(Player& bot, Creature const& target)
{
    // Cata 772 applies periodic aura 94009.
    return !target.HasAura(94009, bot.GetGUID());
}

bool PartyMemberHasAggro(Player& bot, Creature const& target)
{
    Unit* victim = target.GetVictim();
    if (!victim || victim == &bot || !victim->IsPlayer())
        return false;
    Group* group = bot.GetGroup();
    return group && group->IsMember(victim->GetGUID());
}

bool Always([[maybe_unused]] Player& bot, [[maybe_unused]] Creature const& target)
{
    return true;
}

using WarriorCondition = bool (*)(Player&, Creature const&);

class WarriorCombatTrigger final : public Trigger
{
public:
    WarriorCombatTrigger(PlayerbotAI* ai, char const* name, uint32 spellId, WarriorCondition condition)
        : Trigger(ai, name, 1), spellId(spellId), condition(condition) { }
    bool IsActive() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        Creature* target = botAI ? botAI->GetCurrentTarget() : nullptr;
        return ValidWarriorTarget(bot, target) && bot->HasSpell(spellId) && condition(*bot, *target);
    }
private:
    uint32 spellId;
    WarriorCondition condition;
};

class WarriorCombatAction final : public Action
{
public:
    WarriorCombatAction(PlayerbotAI* ai, char const* name, uint32 spellId, WarriorCondition condition)
        : Action(ai, name), spellId(spellId), condition(condition) { }
    bool isUseful() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        Creature* target = botAI ? botAI->GetCurrentTarget() : nullptr;
        return ValidWarriorTarget(bot, target) && bot->HasSpell(spellId) && condition(*bot, *target);
    }
    bool Execute([[maybe_unused]] Event event) override
    {
        if (!isUseful())
            return false;
        return PlayerbotDecision::TryCast(*botAI->GetBot(), *botAI->GetCurrentTarget(), spellId, name.c_str());
    }
private:
    uint32 spellId;
    WarriorCondition condition;
};

class GenericWarriorStrategy : public Strategy
{
public:
    explicit GenericWarriorStrategy(PlayerbotAI* ai) : Strategy(ai) { }
    std::string const getName() override { return "warrior"; }
    uint32_t GetType() const override { return STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_MELEE | STRATEGY_TYPE_DPS; }
    std::vector<NextAction> getDefaultActions() override
    {
        return { NextAction("strike", ACTION_DEFAULT) };
    }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("victory rush", { NextAction("victory rush", ACTION_HIGH + 5) }));
        triggers.push_back(new TriggerNode("rend", { NextAction("rend", ACTION_HIGH + 2) }));
    }
};

class TankWarriorStrategy final : public GenericWarriorStrategy
{
public:
    explicit TankWarriorStrategy(PlayerbotAI* ai) : GenericWarriorStrategy(ai) { }
    std::string const getName() override { return "tank"; }
    uint32_t GetType() const override { return STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_MELEE | STRATEGY_TYPE_TANK; }
    std::vector<NextAction> getDefaultActions() override
    {
        return { NextAction("shield slam", ACTION_DEFAULT + 0.3f),
                 NextAction("strike", ACTION_DEFAULT) };
    }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        GenericWarriorStrategy::InitTriggers(triggers);
        triggers.push_back(new TriggerNode("taunt", { NextAction("taunt", ACTION_INTERRUPT + 1) }));
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

        auto* combatStrategies = new NamedObjectContext<Strategy>(false, true);
        combatStrategies->creators["warrior"] = [](PlayerbotAI* ai) { return new GenericWarriorStrategy(ai); };
        combatStrategies->creators["tank"] = [](PlayerbotAI* ai) { return new TankWarriorStrategy(ai); };
        strategies.Add(combatStrategies);

        auto* actionFactory = new NamedObjectContext<Action>();
        actionFactory->creators["battle shout"] = [](PlayerbotAI* ai) { return new BattleShoutAction(ai); };
        actionFactory->creators["victory rush"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "victory rush", 34428, VictoryRushReady); };
        actionFactory->creators["rend"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "rend", 772, RendMissing); };
        actionFactory->creators["strike"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "strike", 88161, Always); };
        actionFactory->creators["shield slam"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "shield slam", 23922, Always); };
        actionFactory->creators["taunt"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "taunt", 355, PartyMemberHasAggro); };
        actions.Add(actionFactory);

        auto* triggerFactory = new NamedObjectContext<Trigger>();
        triggerFactory->creators["battle shout"] = [](PlayerbotAI* ai) { return new BattleShoutTrigger(ai); };
        triggerFactory->creators["victory rush"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "victory rush", 34428, VictoryRushReady); };
        triggerFactory->creators["rend"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "rend", 772, RendMissing); };
        triggerFactory->creators["taunt"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "taunt", 355, PartyMemberHasAggro); };
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
