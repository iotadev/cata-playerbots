/* Released under GNU GPL v2 or any later version. See AUTHORS.md and PORTING.md. */
#ifndef PLAYERBOTS_ROGUE_AIOBJECTCONTEXT_H
#define PLAYERBOTS_ROGUE_AIOBJECTCONTEXT_H
#include "../../../Bot/Engine/AiObjectContext.h"
class RogueAiObjectContext final : public AiObjectContext
{
public:
    explicit RogueAiObjectContext(PlayerbotAI* ai);
};
#endif
