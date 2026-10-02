/*
 * Adapted from mod-playerbots InterruptSpellTrigger, CastPummelAction and
 * CastCounterspellAction at 7bae1b5c58c76a0aa20381155edc08096d1485b2.
 * See AUTHORS.md and PORTING.md. Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOT_INTERRUPT_STRATEGY_H
#define PLAYERBOT_INTERRUPT_STRATEGY_H
#include "../../Bot/Engine/Strategy/Strategy.h"

namespace PlayerbotInterrupt
{
inline bool InterruptibleCast(bool channeling, bool preparing, bool hasCastTime, bool nativeAllows)
{
    return nativeAllows && (channeling || (preparing && hasCastTime));
}
inline void AddTrigger(std::vector<TriggerNode*>& triggers, char const* name)
{
    triggers.push_back(new TriggerNode(name, { NextAction(name, ACTION_INTERRUPT) }));
}
void AddContexts(SharedNamedObjectContextList<Action>& actions,
    SharedNamedObjectContextList<Trigger>& triggers, char const* name,
    uint32 spellId, bool (*enabled)());
}
#endif
