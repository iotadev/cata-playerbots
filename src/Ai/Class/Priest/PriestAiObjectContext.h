/*
 * Adapted from AzerothCore mod-playerbots PriestAiObjectContext.h at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOTS_PRIEST_AIOBJECTCONTEXT_H
#define PLAYERBOTS_PRIEST_AIOBJECTCONTEXT_H

#include "../../../Bot/Engine/AiObjectContext.h"

class PriestAiObjectContext final : public AiObjectContext
{
public:
    explicit PriestAiObjectContext(PlayerbotAI* botAI);
};

#endif
