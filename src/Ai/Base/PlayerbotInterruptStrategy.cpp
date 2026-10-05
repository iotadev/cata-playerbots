/*
 * Cata adaptation of donor interrupt trigger/actions at
 * 7bae1b5c58c76a0aa20381155edc08096d1485b2. See PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "PlayerbotInterruptStrategy.h"
#include "PlayerbotCombatDecision.h"
#include "PlayerbotTargetSelection.h"
#include "../../Bot/PlayerbotAI.h"
#include "Creature.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Spell.h"
#include "SpellHistory.h"
#include "SpellMgr.h"

namespace
{
bool Ready(PlayerbotAI* ai, uint32 spellId, bool (*enabled)())
{
    Player* bot = ai ? ai->GetBot() : nullptr;
    Player* owner = ai ? ai->GetController() : nullptr;
    if (!enabled() || !bot || !owner || !bot->IsAlive() || !owner->IsAlive() ||
        !bot->IsInWorld() || !owner->IsInWorld() || bot->GetMap() != owner->GetMap() ||
        owner->IsBeingTeleported() || !bot->IsWithinDistInMap(owner, 35.0f) ||
        bot->IsBeingTeleported() || bot->IsNonMeleeSpellCast(false) ||
        bot->IsCharmed() || bot->IsCharming() || !bot->HasSpell(spellId))
        return false;
    SpellInfo const* ability = sSpellMgr->GetSpellInfo(spellId);
    return ability && ability->HasEffect(SPELL_EFFECT_INTERRUPT_CAST) &&
        !bot->GetSpellHistory()->HasCooldown(ability) && !bot->GetSpellHistory()->HasGlobalCooldown(ability) &&
        bot->CanRequestSpellCast(ability);
}
bool Casting(Creature& target, bool positiveOnly)
{
    // Match native EffectInterruptCast; do not guess from UNIT_STATE_CASTING.
    for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_AUTOREPEAT_SPELL; ++i)
        if (Spell* spell = target.GetCurrentSpell(CurrentSpellTypes(i)))
            if ((!positiveOnly || spell->m_spellInfo->IsPositive()) &&
                PlayerbotInterrupt::InterruptibleCast(spell->getState() == SPELL_STATE_CHANNELING,
                    spell->getState() == SPELL_STATE_PREPARING, spell->GetCastTime() > 0,
                    spell->m_spellInfo->CanBeInterrupted(&target))) return true;
    return false;
}
bool EligibleTarget(PlayerbotAI& ai, Creature& target, uint32 spellId, bool secondary)
{
    Player* bot = ai.GetBot();
    Player* owner = ai.GetController();
    if (!bot || !owner || !target.IsAlive() || !target.IsInWorld() || target.GetMap() != bot->GetMap() ||
        target.IsControlledByPlayer() || target.IsInEvadeMode() ||
        !bot->IsValidAttackTarget(&target) || !bot->CanSeeOrDetect(&target) || !bot->IsWithinLOSInMap(&target))
        return false;
    bool protectedTarget = PlayerbotTargetSelection::IsProtectedTarget(target);
    bool engaged = PlayerbotTargetSelection::IsEngagedWithAttachedParty(*bot, *owner, target) ||
        (!secondary && bot->IsInCombatWith(&target)); // Preserve an ungrouped bot's existing combat target.
    bool interruptible = Casting(target, secondary);
    if (protectedTarget || !engaged || !interruptible) return false;
    SpellInfo const* ability = sSpellMgr->GetSpellInfo(spellId);
    if (!ability) return false;
    Spell check(bot, ability, TRIGGERED_NONE);
    bool nativeUsable = check.CanAutoCast(&target); // Native range/cost/immunity; no cast submission.
    Creature* current = ai.GetCurrentTarget();
    return secondary ? PlayerbotInterrupt::EnemyHealerCandidate(current && current->GetGUID() == target.GetGUID(),
        engaged, protectedTarget, interruptible, nativeUsable) : nativeUsable;
}
class EnemyHealerValue final : public CalculatedValue<ObjectGuid>, public Qualified
{
public:
    EnemyHealerValue(PlayerbotAI* ai, std::string name, uint32 spellId, bool (*enabled)())
        : CalculatedValue(ai, "enemy healer target"), name(std::move(name)), spellId(spellId), enabled(enabled) { }
private:
    ObjectGuid Calculate() override
    {
        if (qualifier != name || !Ready(botAI, spellId, enabled)) return ObjectGuid::Empty;
        AiObjectContext* context = botAI->GetAiObjectContext();
        auto* attackers = context ? context->GetValue<std::vector<ObjectGuid>>("attackers") : nullptr;
        if (!attackers) return ObjectGuid::Empty;
        Creature* current = botAI->GetCurrentTarget();
        for (ObjectGuid guid : attackers->Get())
            if (Creature* target = ObjectAccessor::GetCreature(*botAI->GetBot(), guid))
            {
                bool same = current && current->GetGUID() == guid;
                if (!same && EligibleTarget(*botAI, *target, spellId, true)) return guid;
            }
        return ObjectGuid::Empty; // Copied GUIDs only; never retain native Spell/Creature pointers.
    }
    std::string name;
    uint32 spellId;
    bool (*enabled)();
};
Creature* Resolve(PlayerbotAI* ai, uint32 spellId, bool (*enabled)(), std::string const& name, bool secondary)
{
    if (!Ready(ai, spellId, enabled)) return nullptr;
    Creature* target = ai->GetCurrentTarget();
    if (secondary)
    {
        AiObjectContext* context = ai->GetAiObjectContext();
        auto* value = context ? context->GetValue<ObjectGuid>("enemy healer target", name) : nullptr;
        target = value ? ObjectAccessor::GetCreature(*ai->GetBot(), value->Get()) : nullptr;
        Creature* current = ai->GetCurrentTarget();
        if (target && current && target->GetGUID() == current->GetGUID()) return nullptr;
    }
    return target && EligibleTarget(*ai, *target, spellId, secondary) ? target : nullptr;
}
class InterruptTrigger final : public Trigger
{
public:
    InterruptTrigger(PlayerbotAI* ai, std::string name, std::string abilityName, uint32 spellId, bool (*enabled)(), bool secondary)
        : Trigger(ai, std::move(name), 1), abilityName(std::move(abilityName)), spellId(spellId), enabled(enabled), secondary(secondary) { }
    bool IsActive() override { return Resolve(botAI, spellId, enabled, abilityName, secondary) != nullptr; }
private:
    std::string abilityName;
    uint32 spellId;
    bool (*enabled)();
    bool secondary;
};
class InterruptAction final : public Action
{
public:
    InterruptAction(PlayerbotAI* ai, std::string name, std::string abilityName, uint32 spellId, bool (*enabled)(), bool secondary)
        : Action(ai, std::move(name)), abilityName(std::move(abilityName)), spellId(spellId), enabled(enabled), secondary(secondary) { }
    bool isUseful() override { return Resolve(botAI, spellId, enabled, abilityName, secondary) != nullptr; }
    bool Execute([[maybe_unused]] Event event) override
    {
        Creature* target = Resolve(botAI, spellId, enabled, abilityName, secondary);
        return target && PlayerbotDecision::TryCast(*botAI->GetBot(), *target, spellId, name.c_str());
    }
private:
    std::string abilityName;
    uint32 spellId;
    bool (*enabled)();
    bool secondary;
};
}
void PlayerbotInterrupt::AddContexts(SharedNamedObjectContextList<Action>& actions,
    SharedNamedObjectContextList<Trigger>& triggers, SharedNamedObjectContextList<UntypedValue>& values,
    char const* name, uint32 spellId, bool (*enabled)())
{
    auto* actionFactory = new NamedObjectContext<Action>();
    auto* triggerFactory = new NamedObjectContext<Trigger>();
    for (bool secondary : {false, true})
    {
        std::string key = std::string(name) + (secondary ? " on enemy healer" : "");
        actionFactory->creators[key] = [key, abilityName = std::string(name), spellId, enabled, secondary](PlayerbotAI* ai)
        { return new InterruptAction(ai, key, abilityName, spellId, enabled, secondary); };
        triggerFactory->creators[key] = [key, abilityName = std::string(name), spellId, enabled, secondary](PlayerbotAI* ai)
        { return new InterruptTrigger(ai, key, abilityName, spellId, enabled, secondary); };
    }
    actions.Add(actionFactory);
    triggers.Add(triggerFactory);
    auto* valueFactory = new NamedObjectContext<UntypedValue>();
    valueFactory->creators["enemy healer target"] = [name = std::string(name), spellId, enabled](PlayerbotAI* ai)
    { return new EnemyHealerValue(ai, name, spellId, enabled); };
    values.Add(valueFactory);
}
