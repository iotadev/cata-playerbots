/*
 * Adapted from AzerothCore mod-playerbots Trigger.cpp at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "Trigger.h"

Trigger::Trigger(PlayerbotAI* botAI, std::string name, int32_t interval)
    : PlayerbotAIAware(botAI), name(std::move(name)),
      checkInterval(interval <= 1 ? 1 : (interval < 100 ? uint32_t(interval * 1000) : uint32_t(interval))) { }

Event Trigger::Check()
{
    return IsActive() ? Event(getName()) : Event();
}

bool Trigger::needCheck(uint32_t now, bool forceRebuffPending, bool inCombat)
{
    if (IsBuffTrigger() && !IsDebuffTrigger() && forceRebuffPending && !inCombat)
        return true;

    if (checkInterval < 2)
        return true;

    if (!lastCheckTime || now - lastCheckTime >= checkInterval)
    {
        lastCheckTime = now;
        return true;
    }

    return false;
}

std::vector<NextAction> TriggerNode::getHandlers()
{
    std::vector<NextAction> result = handlers;
    if (trigger)
    {
        std::vector<NextAction> extra = trigger->getHandlers();
        result.insert(result.end(), extra.begin(), extra.end());
    }
    return result;
}
