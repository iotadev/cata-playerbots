/*
 * Adapted from AzerothCore mod-playerbots MageAiObjectContext.cpp,
 * GenericMageStrategy.cpp and FrostMageStrategy.cpp at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "MageAiObjectContext.h"
#include "PlayerbotMageArmorStrategy.h"
#include "PlayerbotMageCureStrategy.h"
#include "../../Base/PlayerbotPartySupport.h"
#include "../../../Bot/PlayerbotAI.h"
#include "../../Base/PlayerbotCombatDecision.h"
#include "../../Base/PlayerbotPartyBuffStrategy.h"
#include "../../Base/PlayerbotRestStrategy.h"
#include "../../Base/PlayerbotPotionStrategy.h"
#include "../../Base/PlayerbotCombatValues.h"
#include "../../Base/PlayerbotCombatMovement.h"
#include "../../Base/PlayerbotPosition.h"
#include "../../Base/PlayerbotThreatStrategy.h"
#include "../../Base/PlayerbotInterruptStrategy.h"
#include "../../Base/PlayerbotClassSpellPolicy.h"
#include "../../../Script/PlayerbotConfig.h"
#include "Creature.h"
#include "Player.h"
#include "SpellMgr.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
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

bool CurseCureReady(PlayerbotAI* ai)
{
    Player* bot = ai ? ai->GetBot() : nullptr;
    Player* owner = ai ? ai->GetController() : nullptr;
    return bot && owner && bot->getClass() == CLASS_MAGE &&
        PlayerbotPartySupport::CanCure(PlayerbotModuleEngineMageCombatEnabled(), bot->IsAlive(), bot->HasSpell(475), true) &&
        owner->IsAlive() && owner->GetMap() == bot->GetMap() && bot->IsWithinDistInMap(owner, 35.0f) &&
        !bot->IsBeingTeleported() && !owner->IsBeingTeleported() && !bot->IsMounted() && !bot->IsInFlight() &&
        !bot->IsNonMeleeSpellCast(false) && !ai->GetRestSpellId() &&
        !ai->LootRequests().Pending() && !ai->LootPursuit().Active();
}

template <typename Attempt>
bool VisitCurseCandidates(PlayerbotAI* ai, bool party, Attempt&& attempt)
{
    if (!CurseCureReady(ai))
        return false;
    Player* bot = ai->GetBot();
    auto eligible = [&](Player* member)
    {
        return PlayerbotPartySupport::HasDispellableAura(*bot, *member, 475, DISPEL_CURSE) && attempt(*member);
    };
    if (!party)
        return eligible(bot);
    auto candidates = PlayerbotPartySupport::LivingSupportCandidates(*bot, ai->GetController());
    return PlayerbotPartySupport::TryCandidates(candidates,
        [&](Player* member) { return member != bot && eligible(member); });
}

bool CurseNeeded(PlayerbotAI* ai, bool party)
{
    return party ? CurseCureReady(ai) && !PlayerbotPartySupport::DispelTarget(*ai, DISPEL_CURSE).IsEmpty() :
        VisitCurseCandidates(ai, false, [](Player&) { return true; });
}
class CurseTrigger final : public Trigger
{
public:
    CurseTrigger(PlayerbotAI* ai, char const* name, bool party) : Trigger(ai, name, 1), party(party) { }
    bool IsActive() override { return CurseNeeded(botAI, party); }
private:
    bool party;
};
class CurseAction final : public Action
{
public:
    CurseAction(PlayerbotAI* ai, char const* name, bool party) : Action(ai, name), party(party) { }
    bool isUseful() override { return CurseNeeded(botAI, party); }
    bool Execute([[maybe_unused]] Event event) override
    {
        return VisitCurseCandidates(botAI, party, [&](Player& member)
        {
            return PlayerbotDecision::TryCast(*botAI->GetBot(), member, 475, name.c_str());
        });
    }
    ActionThreatType getThreatType() override { return ActionThreatType::None; }
private:
    bool party;
};

bool Pressed(Player* bot, Creature* target)
{
    return ValidMageTarget(bot, target) && target->GetVictim() == bot &&
        bot->IsWithinDistInMap(target, 8.0f);
}

bool FingersOfFrost(Player const& bot)
{
    return bot.GetAuraEffect(SPELL_AURA_ABILITY_IGNORE_AURASTATE, SPELLFAMILY_MAGE,
        0, 0, 0x0000000A, bot.GetGUID()) != nullptr;
}
bool FrozenForIceLance(Player const& bot, Creature const& target)
{
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(30455);
    return spell && target.HasAuraState(AURA_STATE_FROZEN, spell, &bot);
}
bool IceLanceReady(Player const& bot, Creature const& target)
{
    return PlayerbotClassSpell::IceLanceReady(bot.HasSpell(30455), FrozenForIceLance(bot, target), FingersOfFrost(bot));
}
bool ArcaneMissilesReady(Player const& bot)
{
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(5143);
    return PlayerbotClassSpell::ArcaneMissilesReady(bot.HasSpell(5143),
        spell && spell->CasterAuraSpell && bot.HasAura(spell->CasterAuraSpell));
}

bool BrainFreezeReady(Player const& bot, Creature const&)
{
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(44614);
    Aura const* proc = bot.GetAura(57761);
    AuraEffect const* effect = proc ? proc->GetEffect(EFFECT_1) : nullptr;
    bool affecting = spell && effect && effect->GetAuraType() == SPELL_AURA_ADD_PCT_MODIFIER &&
        effect->GetMiscValue() == int32(SpellModOp::ChangeCastTime) && effect->IsAffectingSpell(spell);
    return PlayerbotClassSpell::BrainFreezeReady(bot.HasSpell(44614), affecting, effect ? effect->GetAmount() : 0);
}

bool DeepFreezeReady(Player const& bot, Creature const& target)
{
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(44572);
    // Native HasAuraState includes only ignore-aura-state effects which affect
    // this spell. Do not infer eligibility from Ice Lance's proc mask.
    return PlayerbotClassSpell::DeepFreezeReady(bot.HasSpell(44572), spell &&
        spell->TargetAuraState == AURA_STATE_FROZEN && target.HasAuraState(AURA_STATE_FROZEN, spell, &bot));
}

class ArcaneMissilesTrigger final : public Trigger
{
public:
    explicit ArcaneMissilesTrigger(PlayerbotAI* ai) : Trigger(ai, "arcane blast 4 stacks and missile barrage", 1) { }
    bool IsActive() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        Creature* target = botAI ? botAI->GetCurrentTarget() : nullptr;
        if (!PlayerbotModuleEngineMageCombatEnabled() || !ValidMageTarget(bot, target) || !ArcaneMissilesReady(*bot))
            return false;
        SpellInfo const* stackSpell = sSpellMgr->GetSpellInfo(36032);
        Aura const* stacks = bot->GetAura(36032);
        return stackSpell && PlayerbotClassSpell::ArcaneBlastAtCap(stacks ? stacks->GetStackAmount() : 0, stackSpell->StackAmount);
    }
};

bool HotStreakReady(Player const& bot, Creature const&)
{
    SpellInfo const* base = sSpellMgr->GetSpellInfo(11366);
    TriggerCastFlags flags = TRIGGERED_NONE;
    SpellInfo const* resolved = base ? bot.GetCastSpellInfo(base, flags) : nullptr;
    return PlayerbotClassSpell::HotStreakReady(bot.HasSpell(11366), bot.HasAura(48108),
        resolved && resolved->Id == 92315);
}

bool CriticalMassScorchReady(Player const& bot, Creature const& target)
{
    SpellInfo const* scorch = sSpellMgr->GetSpellInfo(2948);
    bool affectingTalent = false;
    if (scorch)
        for (AuraEffect const* effect : bot.GetAuraEffectsByType(SPELL_AURA_PROC_TRIGGER_SPELL))
            if (effect->GetSpellInfo()->Effects[effect->GetEffIndex()].TriggerSpell == 22959)
            {
                // Proc matching uses loaded native proc metadata (including
                // database overrides), not just the effect's raw DBC mask.
                SpellProcEntry const* proc = sSpellMgr->GetSpellProcEntry(effect->GetSpellInfo()->Id);
                if (proc && (!proc->SpellFamilyName || proc->SpellFamilyName == scorch->SpellFamilyName) &&
                    (!proc->SpellFamilyMask || (proc->SpellFamilyMask & scorch->SpellFamilyFlags)))
                {
                    affectingTalent = true;
                    break;
                }
            }
    return PlayerbotClassSpell::CriticalMassScorchReady(bot.HasSpell(2948), affectingTalent, target.HasAura(22959));
}

using MageCondition = bool (*)(Player const&, Creature const&);

bool SpellstealReady(Player const& bot, Creature const& target)
{
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(30449);
    if (!spell || !bot.HasSpell(30449))
        return false;
    bool nativeStealEffect = false;
    for (SpellEffectInfo const& effect : spell->Effects)
        if (effect.IsEffect(SPELL_EFFECT_STEAL_BENEFICIAL_BUFF) && effect.MiscValue == DISPEL_MAGIC)
            nativeStealEffect = true;
    if (!nativeStealEffect)
        return false;
    // For the already-validated hostile target, the native list supplies
    // positive/nonpassive magic auras with chance and remaining charges.
    // Spellsteal has one extra filter beyond ordinary offensive dispel.
    DispelChargesList candidates;
    target.GetDispellableAuraList(&bot, SpellInfo::GetDispelMask(DISPEL_MAGIC), candidates);
    for (DispelableAura const& candidate : candidates)
        if (PlayerbotClassSpell::SpellstealReady(true, true,
            candidate.GetAura()->GetSpellInfo()->HasAttribute(SPELL_ATTR4_CANNOT_BE_STOLEN)))
            return true;
    return false; // No native aura pointer is retained.
}

class MageConditionalTrigger final : public Trigger
{
public:
    MageConditionalTrigger(PlayerbotAI* ai, char const* name, MageCondition condition)
        : Trigger(ai, name, 1), condition(condition) { }
    bool IsActive() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        Creature* target = botAI ? botAI->GetCurrentTarget() : nullptr;
        return PlayerbotModuleEngineMageCombatEnabled() && ValidMageTarget(bot, target) && condition(*bot, *target);
    }
private:
    MageCondition condition;
};

class MageSpellAction final : public Action
{
public:
    MageSpellAction(PlayerbotAI* ai, char const* name, uint32 spellId, bool selfTarget = false,
                    bool pressureOnly = false, bool frozenOnly = false, bool missilesOnly = false, MageCondition condition = nullptr)
        : Action(ai, name), spellId(spellId), selfTarget(selfTarget), pressureOnly(pressureOnly), frozenOnly(frozenOnly), missilesOnly(missilesOnly), condition(condition) { }

    bool isUseful() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        Creature* target = botAI ? botAI->GetCurrentTarget() : nullptr;
        return PlayerbotModuleEngineMageCombatEnabled() && ValidMageTarget(bot, target) && bot->HasSpell(spellId) &&
            (!pressureOnly || Pressed(bot, target)) && (!frozenOnly || IceLanceReady(*bot, *target)) &&
            (!missilesOnly || ArcaneMissilesReady(*bot)) && (!condition || condition(*bot, *target));
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
    ActionThreatType getThreatType() override
    {
        SpellInfo const* spell = sSpellMgr->GetSpellInfo(spellId);
        // Self-centered hostile area spells are not friendly self-buffs (e.g. Frost Nova).
        return PlayerbotThreat::ClassifySpellTarget(selfTarget, spell && spell->IsPositive());
    }

private:
    uint32 spellId;
    bool selfTarget;
    bool pressureOnly;
    bool frozenOnly;
    bool missilesOnly;
    MageCondition condition;
};
class IceLanceTrigger final : public Trigger
{
public:
    IceLanceTrigger(PlayerbotAI* ai, char const* name, bool procOnly)
        : Trigger(ai, name, 1), procOnly(procOnly) { }
    bool IsActive() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        Creature* target = botAI ? botAI->GetCurrentTarget() : nullptr;
        return PlayerbotModuleEngineMageCombatEnabled() && ValidMageTarget(bot, target) &&
            bot->HasSpell(30455) && (procOnly ? FingersOfFrost(*bot) : FrozenForIceLance(*bot, *target));
    }
private:
    bool procOnly;
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

bool MageDefenseNeeded(PlayerbotAI* ai, uint32 spellId, float threshold, bool pressure)
{
    Player* bot = ai ? ai->GetBot() : nullptr;
    Creature* target = ai ? ai->GetCurrentTarget() : nullptr;
    // Self protection does not require facing/LOS to the enemy. Existing
    // session routing still owns commands, controller, leash and transfers.
    if (!PlayerbotModuleEngineMageCombatEnabled() || !bot || !target || bot->getClass() != CLASS_MAGE ||
        !bot->IsAlive() || !bot->IsInCombat() || !target->IsAlive() || bot->GetVictim() != target ||
        bot->GetMap() != target->GetMap() || !bot->IsValidAttackTarget(target) ||
        bot->IsNonMeleeSpellCast(false) || bot->HasAura(45438))
        return false;
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(spellId);
    bool blocked = bot->HasAura(spellId) || (spell && spell->ExcludeCasterAuraSpell &&
        bot->HasAura(spell->ExcludeCasterAuraSpell));
    return spell && PlayerbotClassSpell::MageDefenseReady(bot->HasSpell(spellId), blocked,
        bot->GetHealthPct(), threshold, pressure && target->GetVictim() == bot);
}

class MageDefenseTrigger final : public Trigger
{
public:
    MageDefenseTrigger(PlayerbotAI* ai, char const* name, uint32 spell, float threshold, bool pressure = false)
        : Trigger(ai, name, 1), spell(spell), threshold(threshold), pressure(pressure) { }
    bool IsActive() override { return MageDefenseNeeded(botAI, spell, threshold, pressure); }
private:
    uint32 spell;
    float threshold;
    bool pressure;
};

class MageDefenseAction final : public Action
{
public:
    MageDefenseAction(PlayerbotAI* ai, char const* name, uint32 spell, float threshold, bool pressure = false)
        : Action(ai, name), spell(spell), threshold(threshold), pressure(pressure) { }
    bool isUseful() override { return MageDefenseNeeded(botAI, spell, threshold, pressure); }
    bool Execute([[maybe_unused]] Event event) override
    {
        if (!isUseful())
            return false;
        Player* bot = botAI->GetBot();
        return PlayerbotDecision::TryCast(*bot, *bot, spell, name.c_str());
    }
    ActionThreatType getThreatType() override { return ActionThreatType::None; }
private:
    uint32 spell;
    float threshold;
    bool pressure;
};

void AddMageDefenseTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("mage critical health", { NextAction("ice block", 90.0f) }));
    triggers.push_back(new TriggerNode("mage low health", { NextAction("mana shield", 85.0f) }));
}

class GenericMageStrategy : public Strategy
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
        PlayerbotCombatMovement::AddTriggers(triggers, true);
        AddMageDefenseTriggers(triggers);
        PlayerbotInterrupt::AddTrigger(triggers, "counterspell");
        triggers.push_back(new TriggerNode("spellsteal", { NextAction("spellsteal", 40.0f) }));
        triggers.push_back(new TriggerNode("enemy is close", { NextAction("frost nova", 50.0f),
                                                                 NextAction("fire blast", 20.0f) }));
    }
};

class FireMageStrategy final : public GenericMageStrategy
{
public:
    explicit FireMageStrategy(PlayerbotAI* ai) : GenericMageStrategy(ai) { }
    std::string const getName() override { return "fire"; }
    std::vector<NextAction> getDefaultActions() override
    {
        return { NextAction("fireball", 5.3f), NextAction("frostbolt", 5.2f), NextAction("fire blast", 5.1f) };
    }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        GenericMageStrategy::InitTriggers(triggers);
        triggers.push_back(new TriggerNode("hot streak", { NextAction("pyroblast", 25.0f) }));
        triggers.push_back(new TriggerNode("improved scorch", { NextAction("scorch", 19.0f) }));
    }
};

class ArcaneMageStrategy final : public GenericMageStrategy
{
public:
    explicit ArcaneMageStrategy(PlayerbotAI* ai) : GenericMageStrategy(ai) { }
    std::string const getName() override { return "arcane"; }
    std::vector<NextAction> getDefaultActions() override
    {
        return { NextAction("arcane blast", 5.6f), NextAction("arcane missiles", 5.5f),
            NextAction("arcane barrage", 5.4f), NextAction("fire blast", 5.3f), NextAction("frostbolt", 5.2f) };
    }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        GenericMageStrategy::InitTriggers(triggers);
        triggers.push_back(new TriggerNode("arcane blast 4 stacks and missile barrage", { NextAction("arcane missiles", 15.0f) }));
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
        // checkout. Ice Lance requires frozen/proc eligibility; remaining
        // pet and proc actions follow their own data review.
        return { NextAction("frostbolt", 5.4f), NextAction("ice lance", 5.3f), NextAction("fire blast", 5.2f),
                 NextAction("fireball", 5.0f) };
    }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        // Preserve donor Brain Freeze > Deep Freeze ordering, raised above the
        // existing Cata Ice Lance fallback. Native cooldown checks decide casts.
        PlayerbotCombatMovement::AddTriggers(triggers, true);
        triggers.push_back(new TriggerNode("brain freeze", { NextAction("frostfire bolt", 23.0f) }));
        triggers.push_back(new TriggerNode("deep freeze", { NextAction("deep freeze", 22.0f) }));
        AddMageDefenseTriggers(triggers);
        triggers.push_back(new TriggerNode("mage medium health or attacked", { NextAction("ice barrier", 29.0f) }));
        triggers.push_back(new TriggerNode("fingers of frost", { NextAction("ice lance", ACTION_HIGH + 1) }));
        triggers.push_back(new TriggerNode("ice lance", { NextAction("ice lance", ACTION_HIGH + 1) }));
        PlayerbotInterrupt::AddTrigger(triggers, "counterspell");
        triggers.push_back(new TriggerNode("spellsteal", { NextAction("spellsteal", 40.0f) }));
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
        PlayerbotCombatValues::AddContexts(values);
        PlayerbotCombatMovement::AddContexts(actions, triggers, values);
        PlayerbotPosition::AddContexts(strategies, actions, triggers, values);
        PlayerbotThreat::AddContexts(strategies);
        PlayerbotRest::AddContexts(strategies, actions, triggers);
        PlayerbotPotion::AddContexts(strategies, actions, triggers);
        PlayerbotCorpseLoot::AddContexts(strategies, actions, triggers);
        PlayerbotMageArmor::AddContexts(strategies, actions, triggers);
        PlayerbotInterrupt::AddContexts(actions, triggers, values, "counterspell", 2139, PlayerbotModuleEngineMageCombatEnabled);
        auto* strategyFactory = new NamedObjectContext<Strategy>();
        strategyFactory->creators["buff"] = [](PlayerbotAI* ai)
        {
            return new PlayerbotPartyBuff::Strategy(ai, PlayerbotPartyBuff::MageAction, ACTION_HIGH);
        };
        strategies.Add(strategyFactory);
        auto* cureFactory = new NamedObjectContext<Strategy>();
        cureFactory->creators["cure"] = [](PlayerbotAI* ai) { return new PlayerbotMageCure::CureStrategy(ai); };
        strategies.Add(cureFactory);
        // Spec siblings must not remove unrelated buff/rest/loot strategies.
        auto* combatStrategies = new NamedObjectContext<Strategy>(false, true);
        combatStrategies->creators["mage"] = [](PlayerbotAI* ai) { return new GenericMageStrategy(ai); };
        combatStrategies->creators["frost"] = [](PlayerbotAI* ai) { return new FrostMageStrategy(ai); };
        combatStrategies->creators["fire"] = [](PlayerbotAI* ai) { return new FireMageStrategy(ai); };
        combatStrategies->creators["arcane"] = [](PlayerbotAI* ai) { return new ArcaneMageStrategy(ai); };
        strategies.Add(combatStrategies);

        auto* actionFactory = new NamedObjectContext<Action>();
        actionFactory->creators[PlayerbotPartyBuff::MageAction] = [](PlayerbotAI* ai)
        {
            return PlayerbotPartyBuff::CreateAction(ai, PlayerbotPartyBuff::MageAction, PlayerbotPartyBuff::Brilliance);
        };
        actionFactory->creators["frost nova"] = [](PlayerbotAI* ai) { return new MageSpellAction(ai, "frost nova", 122, true, true); };
        actionFactory->creators["fire blast"] = [](PlayerbotAI* ai) { return new MageSpellAction(ai, "fire blast", 2136, false, true); };
        actionFactory->creators["frostbolt"] = [](PlayerbotAI* ai) { return new MageSpellAction(ai, "frostbolt", 116); };
        actionFactory->creators["ice lance"] = [](PlayerbotAI* ai) { return new MageSpellAction(ai, "ice lance", 30455, false, false, true); };
        actionFactory->creators["fireball"] = [](PlayerbotAI* ai) { return new MageSpellAction(ai, "fireball", 133); };
        actionFactory->creators["arcane blast"] = [](PlayerbotAI* ai) { return new MageSpellAction(ai, "arcane blast", 30451); };
        actionFactory->creators["arcane missiles"] = [](PlayerbotAI* ai) { return new MageSpellAction(ai, "arcane missiles", 5143, false, false, false, true); };
        actionFactory->creators["arcane barrage"] = [](PlayerbotAI* ai) { return new MageSpellAction(ai, "arcane barrage", 44425); };
        actionFactory->creators["pyroblast"] = [](PlayerbotAI* ai) { return new MageSpellAction(ai, "pyroblast", 11366, false, false, false, false, HotStreakReady); };
        actionFactory->creators["scorch"] = [](PlayerbotAI* ai) { return new MageSpellAction(ai, "scorch", 2948, false, false, false, false, CriticalMassScorchReady); };
        actionFactory->creators["frostfire bolt"] = [](PlayerbotAI* ai) { return new MageSpellAction(ai, "frostfire bolt", 44614, false, false, false, false, BrainFreezeReady); };
        actionFactory->creators["deep freeze"] = [](PlayerbotAI* ai) { return new MageSpellAction(ai, "deep freeze", 44572, false, false, false, false, DeepFreezeReady); };
        actionFactory->creators["ice block"] = [](PlayerbotAI* ai) { return new MageDefenseAction(ai, "ice block", 45438, 25.0f); };
        actionFactory->creators["mana shield"] = [](PlayerbotAI* ai) { return new MageDefenseAction(ai, "mana shield", 1463, 45.0f); };
        actionFactory->creators["ice barrier"] = [](PlayerbotAI* ai) { return new MageDefenseAction(ai, "ice barrier", 11426, 65.0f, true); };
        actionFactory->creators["remove curse"] = [](PlayerbotAI* ai) { return new CurseAction(ai, "remove curse", false); };
        actionFactory->creators["remove curse on party"] = [](PlayerbotAI* ai) { return new CurseAction(ai, "remove curse on party", true); };
        actionFactory->creators["spellsteal"] = [](PlayerbotAI* ai) { return new MageSpellAction(ai, "spellsteal", 30449, false, false, false, false, SpellstealReady); };
        actions.Add(actionFactory);

        auto* triggerFactory = new NamedObjectContext<Trigger>();
        triggerFactory->creators[PlayerbotPartyBuff::MageAction] = [](PlayerbotAI* ai)
        {
            return PlayerbotPartyBuff::CreateTrigger(ai, PlayerbotPartyBuff::MageAction, PlayerbotPartyBuff::Brilliance);
        };
        triggerFactory->creators["enemy is close"] = [](PlayerbotAI* ai) { return new CloseEnemyTrigger(ai); };
        triggerFactory->creators["fingers of frost"] = [](PlayerbotAI* ai) { return new IceLanceTrigger(ai, "fingers of frost", true); };
        triggerFactory->creators["ice lance"] = [](PlayerbotAI* ai) { return new IceLanceTrigger(ai, "ice lance", false); };
        triggerFactory->creators["arcane blast 4 stacks and missile barrage"] = [](PlayerbotAI* ai) { return new ArcaneMissilesTrigger(ai); };
        triggerFactory->creators["hot streak"] = [](PlayerbotAI* ai) { return new MageConditionalTrigger(ai, "hot streak", HotStreakReady); };
        triggerFactory->creators["improved scorch"] = [](PlayerbotAI* ai) { return new MageConditionalTrigger(ai, "improved scorch", CriticalMassScorchReady); };
        triggerFactory->creators["brain freeze"] = [](PlayerbotAI* ai) { return new MageConditionalTrigger(ai, "brain freeze", BrainFreezeReady); };
        triggerFactory->creators["deep freeze"] = [](PlayerbotAI* ai) { return new MageConditionalTrigger(ai, "deep freeze", DeepFreezeReady); };
        triggerFactory->creators["mage critical health"] = [](PlayerbotAI* ai) { return new MageDefenseTrigger(ai, "mage critical health", 45438, 25.0f); };
        triggerFactory->creators["mage low health"] = [](PlayerbotAI* ai) { return new MageDefenseTrigger(ai, "mage low health", 1463, 45.0f); };
        triggerFactory->creators["mage medium health or attacked"] = [](PlayerbotAI* ai) { return new MageDefenseTrigger(ai, "mage medium health or attacked", 11426, 65.0f, true); };
        triggerFactory->creators["remove curse"] = [](PlayerbotAI* ai) { return new CurseTrigger(ai, "remove curse", false); };
        triggerFactory->creators["remove curse on party"] = [](PlayerbotAI* ai) { return new CurseTrigger(ai, "remove curse on party", true); };
        triggerFactory->creators["spellsteal"] = [](PlayerbotAI* ai) { return new MageConditionalTrigger(ai, "spellsteal", SpellstealReady); };
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
