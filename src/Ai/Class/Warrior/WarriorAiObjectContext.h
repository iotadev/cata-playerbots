/*
 * Adapted from AzerothCore mod-playerbots WarriorAiObjectContext.h at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOTS_WARRIOR_AIOBJECTCONTEXT_H
#define PLAYERBOTS_WARRIOR_AIOBJECTCONTEXT_H

#include "../../../Bot/Engine/AiObjectContext.h"

// First Cata class registry slice. Additional donor Warrior contexts will
// extend the creator tables without changing per-bot object ownership.
class WarriorAiObjectContext final : public AiObjectContext
{
public:
    explicit WarriorAiObjectContext(PlayerbotAI* botAI);
};

#endif
