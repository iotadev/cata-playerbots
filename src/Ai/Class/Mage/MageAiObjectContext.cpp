/*
 * Adapted from AzerothCore mod-playerbots MageAiObjectContext.cpp,
 * GenericMageStrategy.cpp and FrostMageStrategy.cpp at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "MageAiObjectContext.h"
#include "../../../Bot/PlayerbotAI.h"
#include "../../Base/PlayerbotCombatDecision.h"
#include "Creature.h"
#include "Player.h"
#include <cmath>

namespace
{
bool ValidMageTarget(Player* bot, Creature* target)
{
    // A ranged Attack(target, false) selects the victim without entering combat.
    // The opening spell establishes combat, so do not gate that spell on IsInCombat().
    return bot && target && bot->getClass() == CLASS_MAGE && bot->IsAlive() &&
        target->IsAlive() && bot->IsValidAttackTarget(target) &&
        !bot->IsNonMeleeSpellCast(false) && bot->IsWithinDistInMap(target, 30.0f) &&
        bot->IsWithinLOSInMap(target) && bot->HasInArc(2.0f * float(M_PI) / 3.0f, target);
}

bool Pressed(Player* bot, Creature* target)
{
    return ValidMageTarget(bot, target) && target->GetVictim() == bot &&
        bot->IsWithinDistInMap(target, 8.0f);
}

class MageSpellAction final : public Action
{
public:
    MageSpellAction(PlayerbotAI* ai, char const* name, uint32 spellId, bool selfTarget = false,
                    bool pressureOnly = false)
        : Action(ai, name), spellId(spellId), selfTarget(selfTarget), pressureOnly(pressureOnly) { }

    bool isUseful() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        Creature* target = botAI ? botAI->GetCurrentTarget() : nullptr;
        return ValidMageTarget(bot, target) && bot->HasSpell(spellId) &&
            (!pressureOnly || Pressed(bot, target));
    }

    bool Execute([[maybe_unused]] Event event) override
    {
        if (!isUseful())
            return false;
        Player* bot = botAI->GetBot();
        Creature* target = botAI->GetCurrentTarget();
        return PlayerbotDecision::TryCast(*bot, selfTarget ? static_cast<Unit&>(*bot) : static_cast<Unit&>(*target),
                                          spellId, name.c_str());
    }

private:
    uint32 spellId;
    bool selfTarget;
    bool pressureOnly;
};

class CloseEnemyTrigger final : public Trigger
{
public:
    explicit CloseEnemyTrigger(PlayerbotAI* ai) : Trigger(ai, "enemy is close", 1) { }
    bool IsActive() override
    {
        return botAI && Pressed(botAI->GetBot(), botAI->GetCurrentTarget());
    }
};

class GenericMageStrategy final : public Strategy
{
public:
    explicit GenericMageStrategy(PlayerbotAI* ai) : Strategy(ai) { }
    std::string const getName() override { return "mage"; }
    uint32_t GetType() const override { return STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_RANGED | STRATEGY_TYPE_DPS; }
    std::vector<NextAction> getDefaultActions() override
    {
        // An unspecialized or unsupported tree is not silently treated as Frost.
        return { NextAction("fireball", 5.2f), NextAction("frostbolt", 5.1f) };
    }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("enemy is close", { NextAction("frost nova", 50.0f),
                                                                 NextAction("fire blast", 20.0f) }));
    }
};

class FrostMageStrategy final : public Strategy
{
public:
    explicit FrostMageStrategy(PlayerbotAI* ai) : Strategy(ai) { }
    std::string const getName() override { return "frost"; }
    uint32_t GetType() const override { return STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_RANGED | STRATEGY_TYPE_DPS; }
    std::vector<NextAction> getDefaultActions() override
    {
        // Donor Frost default ordering, limited to spells verified in this Cata
        // checkout. Ice Lance/pet/proc actions follow their own data review.
        return { NextAction("frostbolt", 5.4f), NextAction("fire blast", 5.2f),
                 NextAction("fireball", 5.0f) };
    }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("enemy is close", { NextAction("frost nova", 50.0f),
                                                                 NextAction("fire blast", 20.0f) }));
    }
};

struct SharedMageContexts
{
    SharedNamedObjectContextList<Strategy> strategies;
    SharedNamedObjectContextList<Action> actions;
    SharedNamedObjectContextList<Trigger> triggers;
    SharedNamedObjectContextList<UntypedValue> values;

    SharedMageContexts()
    {
        auto* strategyFactory = new NamedObjectContext<Strategy>();
        strategyFactory->creators["mage"] = [](PlayerbotAI* ai) { return new GenericMageStrategy(ai); };
        strategyFactory->creators["frost"] = [](PlayerbotAI* ai) { return new FrostMageStrategy(ai); };
        strategies.Add(strategyFactory);

        auto* actionFactory = new NamedObjectContext<Action>();
        actionFactory->creators["frost nova"] = [](PlayerbotAI* ai) { return new MageSpellAction(ai, "frost nova", 122, true, true); };
        actionFactory->creators["fire blast"] = [](PlayerbotAI* ai) { return new MageSpellAction(ai, "fire blast", 2136, false, true); };
        actionFactory->creators["frostbolt"] = [](PlayerbotAI* ai) { return new MageSpellAction(ai, "frostbolt", 116); };
        actionFactory->creators["fireball"] = [](PlayerbotAI* ai) { return new MageSpellAction(ai, "fireball", 133); };
        actions.Add(actionFactory);

        auto* triggerFactory = new NamedObjectContext<Trigger>();
        triggerFactory->creators["enemy is close"] = [](PlayerbotAI* ai) { return new CloseEnemyTrigger(ai); };
        triggers.Add(triggerFactory);
    }
};

SharedMageContexts& Shared()
{
    static SharedMageContexts contexts;
    return contexts;
}
}

MageAiObjectContext::MageAiObjectContext(PlayerbotAI* botAI)
    : AiObjectContext(botAI, Shared().strategies, Shared().actions, Shared().triggers, Shared().values) { }
