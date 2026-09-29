/*
 * Adapted from AzerothCore mod-playerbots PriestAiObjectContext.cpp and
 * HealPriestStrategy.cpp at 8827dd6fcbb2bb25988787a40f06fc93daf8e02d.
 * See AUTHORS.md and PORTING.md. Released under GNU GPL v2 or any later version.
 */
#include "PriestAiObjectContext.h"
#include "PlayerbotPriestStrategy.h"
#include "../../../Bot/PlayerbotAI.h"
#include "../../Base/PlayerbotCombatDecision.h"
#include "Group.h"
#include "Player.h"

namespace
{
Player* FindPartyMemberToResurrect(Player& bot)
{
    if (!bot.IsAlive() || bot.IsInCombat() || bot.IsNonMeleeSpellCast(false))
        return nullptr;

    Group* group = bot.GetGroup();
    if (!group)
        return nullptr;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (member && member != &bot && member->getDeathState() == CORPSE &&
            !member->IsResurrectRequested() && member->GetMap() == bot.GetMap() &&
            bot.IsWithinDistInMap(member, 30.0f) && bot.IsWithinLOSInMap(member))
            return member;
    }
    return nullptr;
}

class PartyMemberDeadTrigger final : public Trigger
{
public:
    explicit PartyMemberDeadTrigger(PlayerbotAI* ai) : Trigger(ai, "party member dead", 1) { }
    bool IsActive() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        return bot && bot->HasSpell(2006) && FindPartyMemberToResurrect(*bot);
    }
};

class PriestResurrectionAction final : public Action
{
public:
    explicit PriestResurrectionAction(PlayerbotAI* ai) : Action(ai, "resurrection") { }
    bool isUseful() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        return bot && bot->HasSpell(2006) && FindPartyMemberToResurrect(*bot);
    }
    bool Execute([[maybe_unused]] Event event) override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        if (!bot || !bot->HasSpell(2006))
            return false;
        Player* target = FindPartyMemberToResurrect(*bot);
        return target && PlayerbotDecision::TryCast(*bot, *target, 2006, "Resurrection");
    }
};

class PartyHealthTrigger final : public Trigger
{
public:
    PartyHealthTrigger(PlayerbotAI* ai, char const* name, float threshold)
        : Trigger(ai, name, 1), threshold(threshold) { }

    bool IsActive() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        if (!bot || !bot->IsAlive())
            return false;
        for (Player* member : PlayerbotPriest::HealCandidates(*bot, botAI->GetController()))
            if (member->GetHealthPct() > 0.0f && member->GetHealthPct() < threshold)
                return true;
        return false;
    }

private:
    float threshold;
};

class PriestHealAction final : public Action
{
public:
    PriestHealAction(PlayerbotAI* ai, char const* name, uint32 spellId, float threshold)
        : Action(ai, name), spellId(spellId), threshold(threshold) { }

    bool isUseful() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        if (!bot || !bot->IsAlive() || bot->IsNonMeleeSpellCast(false) || !bot->HasSpell(spellId))
            return false;
        for (Player* member : PlayerbotPriest::HealCandidates(*bot, botAI->GetController()))
            if (Eligible(*bot, *member))
                return true;
        return false;
    }

    bool Execute([[maybe_unused]] Event event) override
    {
        if (!isUseful())
            return false;
        Player* bot = botAI->GetBot();
        std::vector<Player*> candidates = PlayerbotPriest::HealCandidates(*bot, botAI->GetController());
        return PlayerbotPriest::TryInHealthOrder(candidates,
            [](Player* member) { return member->GetHealthPct(); },
            [&](Player* member)
            {
                return Eligible(*bot, *member) && PlayerbotDecision::TryCast(*bot, *member, spellId, name.c_str());
            });
    }

private:
    bool Eligible(Player const& bot, Player const& member) const
    {
        float health = member.GetHealthPct();
        if (!(health > 0.0f && health < threshold))
            return false;
        if (spellId == 17)
            return !member.HasAura(17) && !member.HasAura(6788);
        if (spellId == 139)
            return !member.HasAura(139, bot.GetGUID());
        return true;
    }

    uint32 spellId;
    float threshold;
};

class HealPriestStrategy final : public Strategy
{
public:
    explicit HealPriestStrategy(PlayerbotAI* ai) : Strategy(ai) { }
    std::string const getName() override { return "heal"; }
    uint32_t GetType() const override { return STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_HEAL; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("party member critical health",
            { NextAction("power word: shield on party", ACTION_CRITICAL_HEAL + 5),
              NextAction("flash heal on party", ACTION_CRITICAL_HEAL + 2),
              NextAction("heal on party", ACTION_CRITICAL_HEAL + 1),
              NextAction("renew on party", ACTION_CRITICAL_HEAL) }));
        triggers.push_back(new TriggerNode("party member low health",
            { NextAction("heal on party", ACTION_MEDIUM_HEAL + 1),
              NextAction("renew on party", ACTION_MEDIUM_HEAL) }));
        triggers.push_back(new TriggerNode("party member medium health",
            { NextAction("renew on party", ACTION_LIGHT_HEAL + 1) }));
    }
};

class PriestNonCombatStrategy final : public Strategy
{
public:
    explicit PriestNonCombatStrategy(PlayerbotAI* ai) : Strategy(ai) { }
    std::string const getName() override { return "nc"; }
    uint32_t GetType() const override { return STRATEGY_TYPE_NONCOMBAT | STRATEGY_TYPE_HEAL; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("party member dead",
            { NextAction("resurrection", ACTION_CRITICAL_HEAL + 10) }));
    }
};

struct SharedPriestContexts
{
    SharedNamedObjectContextList<Strategy> strategies;
    SharedNamedObjectContextList<Action> actions;
    SharedNamedObjectContextList<Trigger> triggers;
    SharedNamedObjectContextList<UntypedValue> values;

    SharedPriestContexts()
    {
        auto* strategyFactory = new NamedObjectContext<Strategy>();
        strategyFactory->creators["heal"] = [](PlayerbotAI* ai) { return new HealPriestStrategy(ai); };
        strategyFactory->creators["nc"] = [](PlayerbotAI* ai) { return new PriestNonCombatStrategy(ai); };
        strategies.Add(strategyFactory);

        auto* actionFactory = new NamedObjectContext<Action>();
        actionFactory->creators["power word: shield on party"] = [](PlayerbotAI* ai) { return new PriestHealAction(ai, "Power Word: Shield", 17, 35.0f); };
        actionFactory->creators["flash heal on party"] = [](PlayerbotAI* ai) { return new PriestHealAction(ai, "Flash Heal", 2061, 55.0f); };
        actionFactory->creators["heal on party"] = [](PlayerbotAI* ai) { return new PriestHealAction(ai, "Heal", 2050, 80.0f); };
        actionFactory->creators["renew on party"] = [](PlayerbotAI* ai) { return new PriestHealAction(ai, "Renew", 139, 90.0f); };
        actionFactory->creators["resurrection"] = [](PlayerbotAI* ai) { return new PriestResurrectionAction(ai); };
        actions.Add(actionFactory);

        auto* triggerFactory = new NamedObjectContext<Trigger>();
        triggerFactory->creators["party member critical health"] = [](PlayerbotAI* ai) { return new PartyHealthTrigger(ai, "party member critical health", 55.0f); };
        triggerFactory->creators["party member low health"] = [](PlayerbotAI* ai) { return new PartyHealthTrigger(ai, "party member low health", 80.0f); };
        triggerFactory->creators["party member medium health"] = [](PlayerbotAI* ai) { return new PartyHealthTrigger(ai, "party member medium health", 90.0f); };
        triggerFactory->creators["party member dead"] = [](PlayerbotAI* ai) { return new PartyMemberDeadTrigger(ai); };
        triggers.Add(triggerFactory);
    }
};

SharedPriestContexts& Shared()
{
    static SharedPriestContexts contexts;
    return contexts;
}
}

PriestAiObjectContext::PriestAiObjectContext(PlayerbotAI* botAI)
    : AiObjectContext(botAI, Shared().strategies, Shared().actions, Shared().triggers, Shared().values) { }
