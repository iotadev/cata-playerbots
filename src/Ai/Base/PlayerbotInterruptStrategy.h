/*
 * Adapted from mod-playerbots InterruptSpellTrigger, CastPummelAction and
 * CastCounterspellAction at 7bae1b5c58c76a0aa20381155edc08096d1485b2.
 * See AUTHORS.md and PORTING.md. Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOT_INTERRUPT_STRATEGY_H
#define PLAYERBOT_INTERRUPT_STRATEGY_H
#include "../../Bot/Engine/Strategy/Strategy.h"
#include "../../Bot/Engine/AiObjectContext.h"

namespace PlayerbotInterrupt
{
inline bool InterruptibleCast(bool channeling, bool preparing, bool hasCastTime, bool nativeAllows)
{
    return nativeAllows && (channeling || (preparing && hasCastTime));
}
inline bool EnemyHealerCandidate(bool currentTarget, bool engaged, bool protectedTarget,
    bool positiveInterruptibleCast, bool nativeUsable)
{
    // Donor uses positive casts (including buffs), not only spells with a HEAL effect.
    return !currentTarget && engaged && !protectedTarget && positiveInterruptibleCast && nativeUsable;
}
inline void AddTrigger(std::vector<TriggerNode*>& triggers, char const* name)
{
    triggers.push_back(new TriggerNode(name, { NextAction(name, ACTION_INTERRUPT) }));
    std::string secondary = std::string(name) + " on enemy healer";
    triggers.push_back(new TriggerNode(secondary, { NextAction(secondary, ACTION_INTERRUPT) }));
}
void AddContexts(SharedNamedObjectContextList<Action>& actions,
    SharedNamedObjectContextList<Trigger>& triggers, SharedNamedObjectContextList<UntypedValue>& values, char const* name,
    uint32 spellId, bool (*enabled)());
}
#endif
