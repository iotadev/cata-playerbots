/*
 * Cata adaptation of donor interrupt trigger/actions at
 * 7bae1b5c58c76a0aa20381155edc08096d1485b2. See PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "PlayerbotInterruptStrategy.h"
#include "PlayerbotCombatDecision.h"
#include "../../Bot/PlayerbotAI.h"
#include "Creature.h"
#include "Player.h"
#include "Spell.h"
#include "SpellMgr.h"

namespace
{
bool Eligible(PlayerbotAI* ai, uint32 spellId, bool (*enabled)())
{
    Player* bot = ai ? ai->GetBot() : nullptr;
    Creature* target = ai ? ai->GetCurrentTarget() : nullptr;
    if (!enabled() || !bot || !target || !bot->IsAlive() || !target->IsAlive() ||
        !bot->IsInWorld() || !target->IsInWorld() || bot->GetMap() != target->GetMap() ||
        bot->IsBeingTeleported() || bot->IsNonMeleeSpellCast(false) ||
        !bot->HasSpell(spellId) || !bot->IsValidAttackTarget(target))
        return false;
    SpellInfo const* ability = sSpellMgr->GetSpellInfo(spellId);
    if (!ability || !ability->HasEffect(SPELL_EFFECT_INTERRUPT_CAST))
        return false;
    // Match native EffectInterruptCast; do not guess from UNIT_STATE_CASTING.
    for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_AUTOREPEAT_SPELL; ++i)
        if (Spell* spell = target->GetCurrentSpell(CurrentSpellTypes(i)))
            if (PlayerbotInterrupt::InterruptibleCast(spell->getState() == SPELL_STATE_CHANNELING,
                spell->getState() == SPELL_STATE_PREPARING, spell->GetCastTime() > 0,
                spell->m_spellInfo->CanBeInterrupted(target)))
                return true;
    return false;
}
class InterruptTrigger final : public Trigger
{
public:
    InterruptTrigger(PlayerbotAI* ai, char const* name, uint32 spellId, bool (*enabled)())
        : Trigger(ai, name, 1), spellId(spellId), enabled(enabled) { }
    bool IsActive() override { return Eligible(botAI, spellId, enabled); }
private:
    uint32 spellId;
    bool (*enabled)();
};
class InterruptAction final : public Action
{
public:
    InterruptAction(PlayerbotAI* ai, char const* name, uint32 spellId, bool (*enabled)())
        : Action(ai, name), spellId(spellId), enabled(enabled) { }
    bool isUseful() override { return Eligible(botAI, spellId, enabled); }
    bool Execute([[maybe_unused]] Event event) override
    {
        if (!isUseful())
            return false;
        return PlayerbotDecision::TryCast(*botAI->GetBot(), *botAI->GetCurrentTarget(), spellId, name.c_str());
    }
private:
    uint32 spellId;
    bool (*enabled)();
};
}
void PlayerbotInterrupt::AddContexts(SharedNamedObjectContextList<Action>& actions,
    SharedNamedObjectContextList<Trigger>& triggers, char const* name, uint32 spellId, bool (*enabled)())
{
    auto* actionFactory = new NamedObjectContext<Action>();
    actionFactory->creators[name] = [name, spellId, enabled](PlayerbotAI* ai)
    {
        return new InterruptAction(ai, name, spellId, enabled);
    };
    actions.Add(actionFactory);
    auto* triggerFactory = new NamedObjectContext<Trigger>();
    triggerFactory->creators[name] = [name, spellId, enabled](PlayerbotAI* ai)
    {
        return new InterruptTrigger(ai, name, spellId, enabled);
    };
    triggers.Add(triggerFactory);
}
