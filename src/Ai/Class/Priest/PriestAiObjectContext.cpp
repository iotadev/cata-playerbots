/*
 * Adapted from AzerothCore mod-playerbots PriestAiObjectContext.cpp and
 * HealPriestStrategy.cpp at 8827dd6fcbb2bb25988787a40f06fc93daf8e02d.
 * See AUTHORS.md and PORTING.md. Released under GNU GPL v2 or any later version.
 */
#include "PriestAiObjectContext.h"
#include "PlayerbotPriestStrategy.h"
#include "PlayerbotPriestCureStrategy.h"
#include "PlayerbotPriestDpsStrategy.h"
#include "../../../Bot/PlayerbotAI.h"
#include "../../Base/PlayerbotCombatDecision.h"
#include "../../Base/PlayerbotPartyBuffStrategy.h"
#include "../../Base/PlayerbotRestStrategy.h"
#include "../../Base/PlayerbotCombatValues.h"
#include "../../Base/PlayerbotThreatStrategy.h"
#include "../../Base/PlayerbotDpsEstimate.h"
#include "../../../Script/PlayerbotConfig.h"
#include "Group.h"
#include "Creature.h"
#include "Player.h"
#include "SpellMgr.h"

namespace
{
bool HealerDamageReady(PlayerbotAI* ai, uint32 spellId = 0)
{
    Player* bot = ai ? ai->GetBot() : nullptr;
    Player* owner = ai ? ai->GetController() : nullptr;
    Creature* target = ai ? ai->GetCurrentTarget() : nullptr;
    if (!bot || bot->getClass() != CLASS_PRIEST || !bot->IsAlive() ||
        bot->IsBeingTeleported() || bot->IsNonMeleeSpellCast(false) ||
        ai->GetRestSpellId() || ai->LootRequests().Pending() || ai->LootPursuit().Active())
        return false;
    bool controlled = owner && owner->IsAlive() && owner->GetMap() == bot->GetMap() &&
        target && target->GetMap() == bot->GetMap() && target->IsAlive() &&
        !target->IsControlledByPlayer() && owner->IsInCombatWith(target) &&
        bot->IsWithinDistInMap(owner, 35.0f) && bot->IsWithinDistInMap(target, 25.0f) &&
        owner->IsWithinDistInMap(target, 25.0f) && bot->IsValidAttackTarget(target) &&
        bot->IsWithinLOSInMap(target);
    bool healingNeeded = false;
    for (Player* member : PlayerbotPriest::HealCandidates(*bot, owner))
        if (member->GetHealthPct() < 90.0f)
            healingNeeded = true;
    float mana = bot->GetMaxPower(POWER_MANA) ?
        100.0f * bot->GetPower(POWER_MANA) / bot->GetMaxPower(POWER_MANA) : 0.0f;
    AiObjectContext* context = ai->GetAiObjectContext();
    Value<uint8>* balance = context ? context->GetValue<uint8>("balance") : nullptr;
    if (!PlayerbotPriestDps::CanAttack(PlayerbotModuleEnginePriestHealEnabled(),
        !spellId || bot->HasSpell(spellId), controlled, healingNeeded, mana, balance ? balance->Get() : 0))
        return false;
    if (!spellId)
        return true; // common trigger does not require Smite to be learned
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(spellId);
    if (spellId == 589)
    {
        Value<float>* dps = context ? context->GetValue<float>("estimated group dps") : nullptr;
        if (!dps || !PlayerbotDpsEstimate::EnoughLifetime(float(target->GetHealth()), dps->Get(), 8.0f))
            return false;
    }
    return spell && (spell->HasEffect(SPELL_EFFECT_SCHOOL_DAMAGE) || spell->HasAura(SPELL_AURA_PERIODIC_DAMAGE)) &&
        PlayerbotPriestDps::NeedsDamageCast(spell->HasAura(SPELL_AURA_PERIODIC_DAMAGE),
            target->HasAura(spellId, bot->GetGUID()));
}
class HealerAttackTrigger final : public Trigger
{
public:
    explicit HealerAttackTrigger(PlayerbotAI* ai) : Trigger(ai, "healer should attack", 1) { }
    bool IsActive() override { return HealerDamageReady(botAI); }
};
class HealerDamageAction final : public Action
{
public:
    HealerDamageAction(PlayerbotAI* ai, PlayerbotPriestDps::DamageSpell const& spell)
        : Action(ai, spell.Name), spellId(spell.SpellId) { }
    bool isUseful() override { return HealerDamageReady(botAI, spellId); }
    bool Execute([[maybe_unused]] Event event) override
    {
        if (!HealerDamageReady(botAI, spellId))
            return false;
        Player* bot = botAI->GetBot();
        Creature* target = botAI->GetCurrentTarget();
        bot->SetFacingToObject(target);
        return PlayerbotDecision::TryCast(*bot, *target, spellId, name.c_str());
    }
    ActionThreatType getThreatType() override { return ActionThreatType::Single; }
private:
    uint32 spellId;
};
bool CureReady(PlayerbotAI* ai)
{
    Player* bot = ai ? ai->GetBot() : nullptr;
    return bot && PlayerbotPriestCure::CanCure(PlayerbotModuleEnginePriestHealEnabled(),
        bot->IsAlive(), bot->HasSpell(528), true) && !bot->IsBeingTeleported() &&
        !bot->IsNonMeleeSpellCast(false) && !ai->GetRestSpellId() &&
        !ai->LootRequests().Pending() && !ai->LootPursuit().Active();
}
bool NativeDisease(Player& bot, Player& target)
{
    return PlayerbotPartySupport::HasDispellableAura(bot, target, 528, DISPEL_DISEASE);
}
Player* CureTarget(PlayerbotAI* ai, bool party)
{
    if (!CureReady(ai))
        return nullptr;
    Player* bot = ai->GetBot();
    if (!party)
        return NativeDisease(*bot, *bot) ? bot : nullptr;
    auto candidates = PlayerbotPriest::HealCandidates(*bot, ai->GetController());
    Player* selected = nullptr;
    PlayerbotPriest::TryInPriorityOrder(candidates, [](Player* member) { return member->GetHealthPct(); },
        [&](Player* member)
        {
            if (member == bot || !NativeDisease(*bot, *member))
                return false;
            selected = member;
            return true;
        });
    return selected;
}
class DiseaseTrigger final : public Trigger
{
public:
    DiseaseTrigger(PlayerbotAI* ai, char const* name, bool party) : Trigger(ai, name, 1), party(party) { }
    bool IsActive() override { return CureTarget(botAI, party) != nullptr; }
private:
    bool party;
};
class DiseaseAction final : public Action
{
public:
    DiseaseAction(PlayerbotAI* ai, char const* name, bool party) : Action(ai, name), party(party) { }
    bool isUseful() override { return CureTarget(botAI, party) != nullptr; }
    bool Execute([[maybe_unused]] Event event) override
    {
        if (!CureReady(botAI))
            return false;
        Player* bot = botAI->GetBot();
        if (!party)
            return NativeDisease(*bot, *bot) && PlayerbotDecision::TryCast(*bot, *bot, 528, name.c_str());
        auto candidates = PlayerbotPriest::HealCandidates(*bot, botAI->GetController());
        return PlayerbotPriest::TryInPriorityOrder(candidates,
            [](Player* member) { return member->GetHealthPct(); },
            [&](Player* member)
            {
                // A native cast rejection on one member must not starve others.
                return member != bot && NativeDisease(*bot, *member) &&
                    PlayerbotDecision::TryCast(*bot, *member, 528, name.c_str());
            });
    }
private:
    bool party;
};
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
        return (PlayerbotModuleEnginePriestHealEnabled() || PlayerbotModuleEnginePartyBuffEnabled()) &&
            bot && bot->HasSpell(2006) && FindPartyMemberToResurrect(*bot);
    }
    bool Execute([[maybe_unused]] Event event) override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        if (!isUseful())
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
        if (!(PlayerbotModuleEnginePriestHealEnabled() || PlayerbotModuleEnginePartyBuffEnabled()) ||
            !bot || !bot->IsAlive() || bot->IsNonMeleeSpellCast(false) || !bot->HasSpell(spellId))
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
        PlayerbotCombatValues::AddContexts(values);
        PlayerbotThreat::AddContexts(strategies);
        PlayerbotRest::AddContexts(strategies, actions, triggers);
        PlayerbotCorpseLoot::AddContexts(strategies, actions, triggers);
        auto* strategyFactory = new NamedObjectContext<Strategy>();
        strategyFactory->creators["buff"] = [](PlayerbotAI* ai)
        {
            return new PlayerbotPartyBuff::Strategy(ai, PlayerbotPartyBuff::PriestAction, ACTION_NORMAL);
        };
        strategyFactory->creators["heal"] = [](PlayerbotAI* ai) { return new HealPriestStrategy(ai); };
        strategyFactory->creators["healer dps"] = [](PlayerbotAI* ai) { return new PlayerbotPriestDps::HealerDpsStrategy(ai); };
        strategyFactory->creators["cure"] = [](PlayerbotAI* ai) { return new PlayerbotPriestCure::CureStrategy(ai); };
        strategyFactory->creators["nc"] = [](PlayerbotAI* ai) { return new PriestNonCombatStrategy(ai); };
        strategies.Add(strategyFactory);

        auto* actionFactory = new NamedObjectContext<Action>();
        for (auto const& spell : PlayerbotPriestDps::Spells)
            actionFactory->creators[spell.Name] = [spell](PlayerbotAI* ai) { return new HealerDamageAction(ai, spell); };
        actionFactory->creators[PlayerbotPartyBuff::PriestAction] = [](PlayerbotAI* ai)
        {
            return PlayerbotPartyBuff::CreateAction(ai, PlayerbotPartyBuff::PriestAction, PlayerbotPartyBuff::Fortitude);
        };
        actionFactory->creators["power word: shield on party"] = [](PlayerbotAI* ai) { return new PriestHealAction(ai, "Power Word: Shield", 17, 35.0f); };
        actionFactory->creators["flash heal on party"] = [](PlayerbotAI* ai) { return new PriestHealAction(ai, "Flash Heal", 2061, 55.0f); };
        actionFactory->creators["heal on party"] = [](PlayerbotAI* ai) { return new PriestHealAction(ai, "Heal", 2050, 80.0f); };
        actionFactory->creators["renew on party"] = [](PlayerbotAI* ai) { return new PriestHealAction(ai, "Renew", 139, 90.0f); };
        actionFactory->creators["resurrection"] = [](PlayerbotAI* ai) { return new PriestResurrectionAction(ai); };
        actionFactory->creators["cure disease"] = [](PlayerbotAI* ai) { return new DiseaseAction(ai, "cure disease", false); };
        actionFactory->creators["cure disease on party"] = [](PlayerbotAI* ai) { return new DiseaseAction(ai, "cure disease on party", true); };
        actions.Add(actionFactory);

        auto* triggerFactory = new NamedObjectContext<Trigger>();
        triggerFactory->creators["healer should attack"] = [](PlayerbotAI* ai) { return new HealerAttackTrigger(ai); };
        triggerFactory->creators[PlayerbotPartyBuff::PriestAction] = [](PlayerbotAI* ai)
        {
            return PlayerbotPartyBuff::CreateTrigger(ai, PlayerbotPartyBuff::PriestAction, PlayerbotPartyBuff::Fortitude);
        };
        triggerFactory->creators["party member critical health"] = [](PlayerbotAI* ai) { return new PartyHealthTrigger(ai, "party member critical health", 55.0f); };
        triggerFactory->creators["party member low health"] = [](PlayerbotAI* ai) { return new PartyHealthTrigger(ai, "party member low health", 80.0f); };
        triggerFactory->creators["party member medium health"] = [](PlayerbotAI* ai) { return new PartyHealthTrigger(ai, "party member medium health", 90.0f); };
        triggerFactory->creators["party member dead"] = [](PlayerbotAI* ai) { return new PartyMemberDeadTrigger(ai); };
        triggerFactory->creators["cure disease"] = [](PlayerbotAI* ai) { return new DiseaseTrigger(ai, "cure disease", false); };
        triggerFactory->creators["party member cure disease"] = [](PlayerbotAI* ai) { return new DiseaseTrigger(ai, "party member cure disease", true); };
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
