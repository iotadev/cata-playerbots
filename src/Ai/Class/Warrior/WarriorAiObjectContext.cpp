/*
 * Adapted from AzerothCore mod-playerbots WarriorAiObjectContext.cpp and
 * GenericWarriorNonCombatStrategy.cpp, GenericWarriorStrategy.cpp and
 * TankWarriorStrategy.cpp at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "WarriorAiObjectContext.h"
#include "../../Base/PlayerbotRestStrategy.h"
#include "../../Base/PlayerbotPotionStrategy.h"
#include "../../Base/PlayerbotCombatValues.h"
#include "../../Base/PlayerbotCombatMovement.h"
#include "../../Base/PlayerbotPosition.h"
#include "../../Base/PlayerbotThreatStrategy.h"
#include "../../Base/PlayerbotInterruptStrategy.h"
#include "../../Base/PlayerbotClassSpellPolicy.h"
#include "../../../Script/PlayerbotConfig.h"
#include "../../../Bot/PlayerbotAI.h"
#include "../../Base/PlayerbotCombatDecision.h"
#include "Creature.h"
#include "Group.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"
#include "SpellDefines.h"
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
    bool isUseful() override { return PlayerbotModuleEngineWarriorBuffEnabled() && NeedsBattleShout(botAI ? botAI->GetBot() : nullptr); }
    bool Execute([[maybe_unused]] Event event) override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        return isUseful() && PlayerbotDecision::TryCast(*bot, *bot, 6673, "Battle Shout");
    }
};

bool ValidWarriorTarget(Player* bot, Creature* target)
{
    return bot && target && bot->getClass() == CLASS_WARRIOR && bot->IsAlive() &&
        target->IsAlive() && bot->GetVictim() == target && bot->IsValidAttackTarget(target) &&
        target->IsWithinMeleeRange(bot) && bot->IsWithinLOSInMap(target) &&
        bot->HasInArc(2.0f * float(M_PI) / 3.0f, target);
}

// Self support does not require facing or melee reach, but stays inside the
// existing controlled fight. It never acquires or changes a hostile target.
bool ValidWarriorSupportTarget(Player* bot, Creature* target, uint32 primaryTree)
{
    return bot && target && bot->getClass() == CLASS_WARRIOR && bot->IsAlive() &&
        bot->IsInCombat() && target->IsAlive() && bot->GetVictim() == target &&
        bot->GetMap() == target->GetMap() && bot->IsValidAttackTarget(target) &&
        bot->GetPrimaryTalentTree(bot->GetActiveSpec()) == primaryTree;
}

bool DefensiveStanceNeeded(Player& bot, Creature const&)
{
    return bot.GetShapeshiftForm() != FORM_DEFENSIVESTANCE;
}

bool BattleStanceNeeded(Player& bot, Creature const&)
{
    return bot.GetShapeshiftForm() != FORM_BATTLESTANCE;
}

bool BerserkerStanceNeeded(Player& bot, Creature const&)
{
    return bot.GetShapeshiftForm() != FORM_BERSERKERSTANCE;
}

bool ExecuteReady(Player&, Creature const& target)
{
    return PlayerbotClassSpell::ExecuteReady(target.GetHealthPct());
}

bool ColossusSmashReady(Player& bot, Creature const& target)
{
    return PlayerbotClassSpell::ColossusSmashReady(bot.HasSpell(86346), target.HasAura(86346, bot.GetGUID()));
}

bool RagingBlowReady(Player& bot, Creature const&)
{
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(85288);
    bool enraged = spell && spell->CasterAuraState &&
        bot.HasAuraState(AuraStateType(spell->CasterAuraState), spell, &bot);
    return PlayerbotClassSpell::RagingBlowReady(bot.HasSpell(85288), enraged);
}

bool TasteForBloodReady(Player& bot, Creature const&)
{
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(7384);
    Aura const* proc = bot.GetAura(60503);
    AuraEffect const* effect = proc ? proc->GetEffect(EFFECT_0) : nullptr;
    return spell && effect && effect->GetAuraType() == SPELL_AURA_ABILITY_IGNORE_AURASTATE &&
        effect->IsAffectingSpell(spell);
}

bool OverpowerReady(Player& bot, Creature const& target)
{
    // Native Warrior dodge reactions use target-bound combo points; their
    // reactive timer clears them. Taste for Blood bypasses that requirement.
    bool reactive = bot.GetComboPoints() > 0 && bot.GetComboTarget() == target.GetGUID();
    return PlayerbotClassSpell::OverpowerReady(bot.HasSpell(7384), reactive, TasteForBloodReady(bot, target));
}

bool InstantSlamReady(Player& bot, Creature const&)
{
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(1464);
    Aura const* proc = bot.GetAura(46916);
    AuraEffect const* effect = proc ? proc->GetEffect(EFFECT_0) : nullptr;
    bool affecting = spell && effect && effect->GetAuraType() == SPELL_AURA_ADD_PCT_MODIFIER &&
        effect->GetMiscValue() == int32(SpellModOp::ChangeCastTime) && effect->IsAffectingSpell(spell);
    return PlayerbotClassSpell::InstantSlamReady(bot.HasSpell(1464), affecting, effect ? effect->GetAmount() : 0);
}

bool ShieldBlockNeeded(Player& bot, Creature const&)
{
    return !bot.HasAura(2565);
}

bool ShieldWallNeeded(Player& bot, Creature const&)
{
    return !bot.HasAura(871) && PlayerbotClassSpell::DefensiveHealthReady(bot.GetHealthPct(), false);
}

bool LastStandNeeded(Player& bot, Creature const&)
{
    // Cata spell_warr_last_stand applies triggered health aura 12976.
    return !bot.HasAura(12976) && PlayerbotClassSpell::DefensiveHealthReady(bot.GetHealthPct(), true);
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
bool HeroicStrikeReady(Player& bot, [[maybe_unused]] Creature const& target)
{
    return PlayerbotClassSpell::HeroicStrikeReady(bot.HasSpell(78), bot.GetPower(POWER_RAGE),
        bot.GetPrimaryTalentTree(bot.GetActiveSpec()) == TALENT_TREE_WARRIOR_PROTECTION);
}

bool Always([[maybe_unused]] Player& bot, [[maybe_unused]] Creature const& target)
{
    return true;
}

bool SunderArmorNeeded(Player&, Creature const& target)
{
    // Both Cata Sunder and Devastate apply the separate 58567 debuff.
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(58567);
    Aura const* aura = target.GetAura(58567);
    return spell && PlayerbotClassSpell::SunderArmorNeeded(aura != nullptr,
        aura ? aura->GetStackAmount() : 0, spell->StackAmount,
        aura ? aura->GetDuration() : 0);
}

bool SunderArmorFallback(Player& bot, Creature const& target)
{
    return !bot.HasSpell(20243) && SunderArmorNeeded(bot, target);
}

bool RevengeReady(Player& bot, Creature const&)
{
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(6572);
    return spell && spell->CasterAuraState &&
        bot.HasAuraState(AuraStateType(spell->CasterAuraState), spell, &bot);
}

bool SwordAndBoardReady(Player& bot, Creature const&)
{
    return bot.HasAura(50227);
}

using WarriorCondition = bool (*)(Player&, Creature const&);

class WarriorCombatTrigger final : public Trigger
{
public:
    WarriorCombatTrigger(PlayerbotAI* ai, char const* name, uint32 spellId, WarriorCondition condition, bool self = false,
        uint32 primaryTree = TALENT_TREE_WARRIOR_PROTECTION)
        : Trigger(ai, name, 1), spellId(spellId), condition(condition), self(self), primaryTree(primaryTree) { }
    bool IsActive() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        Creature* target = botAI ? botAI->GetCurrentTarget() : nullptr;
        return PlayerbotModuleEngineWarriorCombatEnabled() &&
            (self ? ValidWarriorSupportTarget(bot, target, primaryTree) : ValidWarriorTarget(bot, target)) &&
            bot->HasSpell(spellId) && condition(*bot, *target);
    }
private:
    uint32 spellId;
    WarriorCondition condition;
    bool self;
    uint32 primaryTree;
};

class WarriorCombatAction final : public Action
{
public:
    WarriorCombatAction(PlayerbotAI* ai, char const* name, uint32 spellId, WarriorCondition condition, bool self = false,
        uint32 primaryTree = TALENT_TREE_WARRIOR_PROTECTION)
        : Action(ai, name), spellId(spellId), condition(condition), self(self), primaryTree(primaryTree) { }
    bool isUseful() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        Creature* target = botAI ? botAI->GetCurrentTarget() : nullptr;
        return PlayerbotModuleEngineWarriorCombatEnabled() &&
            (self ? ValidWarriorSupportTarget(bot, target, primaryTree) : ValidWarriorTarget(bot, target)) &&
            bot->HasSpell(spellId) && condition(*bot, *target);
    }
    bool Execute([[maybe_unused]] Event event) override
    {
        if (!isUseful())
            return false;
        Player* bot = botAI->GetBot();
        Unit& target = self ? static_cast<Unit&>(*bot) : static_cast<Unit&>(*botAI->GetCurrentTarget());
        return PlayerbotDecision::TryCast(*bot, target, spellId, name.c_str());
    }
private:
    uint32 spellId;
    WarriorCondition condition;
    bool self;
    uint32 primaryTree;
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
        PlayerbotCombatMovement::AddTriggers(triggers);
        PlayerbotInterrupt::AddTrigger(triggers, "pummel");
        bool tank = (GetType() & STRATEGY_TYPE_TANK) != 0;
        triggers.push_back(new TriggerNode(tank ? "high rage available" : "medium rage available",
            { NextAction("heroic strike", tank ? ACTION_HIGH : ACTION_DEFAULT + 0.1f) }));
        triggers.push_back(new TriggerNode("victory rush", { NextAction("victory rush", ACTION_HIGH + 5) }));
        triggers.push_back(new TriggerNode("rend", { NextAction("rend", ACTION_HIGH + 2) }));
    }
};

class ArmsWarriorStrategy final : public GenericWarriorStrategy
{
public:
    explicit ArmsWarriorStrategy(PlayerbotAI* ai) : GenericWarriorStrategy(ai) { }
    std::string const getName() override { return "arms"; }
    std::vector<NextAction> getDefaultActions() override
    {
        return { NextAction("mortal strike", ACTION_DEFAULT + 0.1f), NextAction("strike", ACTION_DEFAULT) };
    }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        GenericWarriorStrategy::InitTriggers(triggers);
        triggers.push_back(new TriggerNode("battle stance", { NextAction("battle stance", ACTION_HIGH + 10) }));
        triggers.push_back(new TriggerNode("colossus smash", { NextAction("colossus smash", ACTION_HIGH + 8) }));
        triggers.push_back(new TriggerNode("mortal strike", { NextAction("mortal strike", ACTION_HIGH + 3) }));
        triggers.push_back(new TriggerNode("target critical health", { NextAction("execute", ACTION_HIGH + 5) }));
        triggers.push_back(new TriggerNode("overpower", { NextAction("overpower", ACTION_HIGH + 4) }));
        triggers.push_back(new TriggerNode("taste for blood", { NextAction("overpower", ACTION_HIGH + 4) }));
    }
};

class FuryWarriorStrategy final : public GenericWarriorStrategy
{
public:
    explicit FuryWarriorStrategy(PlayerbotAI* ai) : GenericWarriorStrategy(ai) { }
    std::string const getName() override { return "fury"; }
    std::vector<NextAction> getDefaultActions() override
    {
        return { NextAction("bloodthirst", ACTION_DEFAULT + 0.5f),
            NextAction("execute", ACTION_DEFAULT + 0.2f), NextAction("strike", ACTION_DEFAULT) };
    }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        GenericWarriorStrategy::InitTriggers(triggers);
        triggers.push_back(new TriggerNode("berserker stance", { NextAction("berserker stance", ACTION_HIGH + 9) }));
        triggers.push_back(new TriggerNode("colossus smash", { NextAction("colossus smash", ACTION_HIGH + 8) }));
        triggers.push_back(new TriggerNode("bloodthirst", { NextAction("bloodthirst", ACTION_HIGH + 7) }));
        triggers.push_back(new TriggerNode("raging blow", { NextAction("raging blow", ACTION_HIGH + 6) }));
        triggers.push_back(new TriggerNode("instant slam", { NextAction("slam", ACTION_HIGH + 5) }));
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
        return { NextAction("shield slam", ACTION_DEFAULT + 0.4f),
                 NextAction("devastate", ACTION_DEFAULT + 0.3f),
                 NextAction("revenge", ACTION_DEFAULT + 0.2f),
                 NextAction("sunder armor", ACTION_DEFAULT + 0.1f),
                 NextAction("strike", ACTION_DEFAULT) };
    }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        GenericWarriorStrategy::InitTriggers(triggers);
        triggers.push_back(new TriggerNode("taunt", { NextAction("taunt", ACTION_INTERRUPT + 1) }));
        triggers.push_back(new TriggerNode("sunder armor",
            { NextAction("devastate", ACTION_HIGH + 2), NextAction("sunder armor", ACTION_HIGH + 2) }));
        triggers.push_back(new TriggerNode("revenge", { NextAction("revenge", ACTION_HIGH + 2) }));
        triggers.push_back(new TriggerNode("sword and board", { NextAction("shield slam", ACTION_INTERRUPT) }));
        triggers.push_back(new TriggerNode("defensive stance", { NextAction("defensive stance", ACTION_HIGH + 9) }));
        triggers.push_back(new TriggerNode("shield block", { NextAction("shield block", ACTION_INTERRUPT + 1) }));
        triggers.push_back(new TriggerNode("warrior low health", { NextAction("shield wall", ACTION_MEDIUM_HEAL) }));
        triggers.push_back(new TriggerNode("warrior critical health", { NextAction("last stand", ACTION_EMERGENCY + 3) }));
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
        PlayerbotCombatValues::AddContexts(values);
        PlayerbotCombatMovement::AddContexts(actions, triggers, values);
        PlayerbotPosition::AddContexts(strategies, actions, triggers, values);
        PlayerbotThreat::AddContexts(strategies);
        PlayerbotRest::AddContexts(strategies, actions, triggers);
        PlayerbotPotion::AddContexts(strategies, actions, triggers);
        PlayerbotCorpseLoot::AddContexts(strategies, actions, triggers);
        PlayerbotInterrupt::AddContexts(actions, triggers, values, "pummel", 6552, PlayerbotModuleEngineWarriorCombatEnabled);
        auto* strategyFactory = new NamedObjectContext<Strategy>();
        strategyFactory->creators["nc"] = [](PlayerbotAI* ai) { return new WarriorNonCombatStrategy(ai); };
        strategies.Add(strategyFactory);

        auto* combatStrategies = new NamedObjectContext<Strategy>(false, true);
        combatStrategies->creators["warrior"] = [](PlayerbotAI* ai) { return new GenericWarriorStrategy(ai); };
        combatStrategies->creators["tank"] = [](PlayerbotAI* ai) { return new TankWarriorStrategy(ai); };
        combatStrategies->creators["arms"] = [](PlayerbotAI* ai) { return new ArmsWarriorStrategy(ai); };
        combatStrategies->creators["fury"] = [](PlayerbotAI* ai) { return new FuryWarriorStrategy(ai); };
        strategies.Add(combatStrategies);

        auto* actionFactory = new NamedObjectContext<Action>();
        actionFactory->creators["battle shout"] = [](PlayerbotAI* ai) { return new BattleShoutAction(ai); };
        actionFactory->creators["victory rush"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "victory rush", 34428, VictoryRushReady); };
        actionFactory->creators["heroic strike"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "heroic strike", 78, HeroicStrikeReady); };
        actionFactory->creators["rend"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "rend", 772, RendMissing); };
        actionFactory->creators["strike"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "strike", 88161, Always); };
        actionFactory->creators["mortal strike"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "mortal strike", 12294, Always); };
        actionFactory->creators["bloodthirst"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "bloodthirst", 23881, Always); };
        actionFactory->creators["execute"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "execute", 5308, ExecuteReady); };
        actionFactory->creators["colossus smash"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "colossus smash", 86346, ColossusSmashReady); };
        actionFactory->creators["raging blow"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "raging blow", 85288, RagingBlowReady); };
        actionFactory->creators["overpower"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "overpower", 7384, OverpowerReady); };
        actionFactory->creators["slam"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "slam", 1464, InstantSlamReady); };
        actionFactory->creators["battle stance"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "battle stance", 2457, BattleStanceNeeded, true, TALENT_TREE_WARRIOR_ARMS); };
        actionFactory->creators["berserker stance"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "berserker stance", 2458, BerserkerStanceNeeded, true, TALENT_TREE_WARRIOR_FURY); };
        actionFactory->creators["shield slam"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "shield slam", 23922, Always); };
        actionFactory->creators["devastate"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "devastate", 20243, Always); };
        actionFactory->creators["sunder armor"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "sunder armor", 7386, SunderArmorFallback); };
        actionFactory->creators["revenge"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "revenge", 6572, RevengeReady); };
        actionFactory->creators["taunt"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "taunt", 355, PartyMemberHasAggro); };
        actionFactory->creators["defensive stance"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "defensive stance", 71, DefensiveStanceNeeded, true); };
        actionFactory->creators["shield block"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "shield block", 2565, ShieldBlockNeeded, true); };
        actionFactory->creators["shield wall"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "shield wall", 871, ShieldWallNeeded, true); };
        actionFactory->creators["last stand"] = [](PlayerbotAI* ai) { return new WarriorCombatAction(ai, "last stand", 12975, LastStandNeeded, true); };
        actions.Add(actionFactory);

        auto* triggerFactory = new NamedObjectContext<Trigger>();
        triggerFactory->creators["battle shout"] = [](PlayerbotAI* ai) { return new BattleShoutTrigger(ai); };
        triggerFactory->creators["mortal strike"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "mortal strike", 12294, Always); };
        triggerFactory->creators["bloodthirst"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "bloodthirst", 23881, Always); };
        triggerFactory->creators["target critical health"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "target critical health", 5308, ExecuteReady); };
        triggerFactory->creators["colossus smash"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "colossus smash", 86346, ColossusSmashReady); };
        triggerFactory->creators["raging blow"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "raging blow", 85288, RagingBlowReady); };
        triggerFactory->creators["overpower"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "overpower", 7384, OverpowerReady); };
        triggerFactory->creators["taste for blood"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "taste for blood", 7384, TasteForBloodReady); };
        triggerFactory->creators["instant slam"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "instant slam", 1464, InstantSlamReady); };
        triggerFactory->creators["battle stance"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "battle stance", 2457, BattleStanceNeeded, true, TALENT_TREE_WARRIOR_ARMS); };
        triggerFactory->creators["berserker stance"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "berserker stance", 2458, BerserkerStanceNeeded, true, TALENT_TREE_WARRIOR_FURY); };
        triggerFactory->creators["victory rush"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "victory rush", 34428, VictoryRushReady); };
        triggerFactory->creators["rend"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "rend", 772, RendMissing); };
        triggerFactory->creators["medium rage available"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "medium rage available", 78, HeroicStrikeReady); };
        triggerFactory->creators["high rage available"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "high rage available", 78, HeroicStrikeReady); };
        triggerFactory->creators["taunt"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "taunt", 355, PartyMemberHasAggro); };
        triggerFactory->creators["sunder armor"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "sunder armor", 7386, SunderArmorNeeded); };
        triggerFactory->creators["revenge"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "revenge", 6572, RevengeReady); };
        triggerFactory->creators["sword and board"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "sword and board", 23922, SwordAndBoardReady); };
        triggerFactory->creators["defensive stance"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "defensive stance", 71, DefensiveStanceNeeded, true); };
        triggerFactory->creators["shield block"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "shield block", 2565, ShieldBlockNeeded, true); };
        triggerFactory->creators["warrior low health"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "warrior low health", 871, ShieldWallNeeded, true); };
        triggerFactory->creators["warrior critical health"] = [](PlayerbotAI* ai) { return new WarriorCombatTrigger(ai, "warrior critical health", 12975, LastStandNeeded, true); };
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
