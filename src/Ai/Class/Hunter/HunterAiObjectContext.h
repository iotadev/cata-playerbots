/* GPL v2 or later. See AUTHORS.md and PORTING.md. */
#ifndef PLAYERBOTS_HUNTER_AIOBJECTCONTEXT_H
#define PLAYERBOTS_HUNTER_AIOBJECTCONTEXT_H
#include "../../../Bot/Engine/AiObjectContext.h"
class Player;
class HunterAiObjectContext final : public AiObjectContext
{
public:
    explicit HunterAiObjectContext(PlayerbotAI* ai);
};
namespace PlayerbotHunter
{
// Map-owned native PetAI control; never invoke the world-thread pet packet handler here.
void UpdatePet(PlayerbotAI& ai, bool attacking);
}
#endif
