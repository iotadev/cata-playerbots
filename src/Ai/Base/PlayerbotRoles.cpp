/* GPL v2 or later. Donor provenance and Cata adaptations are in PORTING.md. */
#include "PlayerbotRoles.h"
#include "WorldSession.h"
uint32 PlayerbotRoles::Mask(Player const& player, bool bySpec)
{
    uint32 configured = !bySpec && player.GetSession() ? player.GetSession()->GetPlayerbotStrategyRoleMask() : 0;
    return ChooseMask(configured, SpecMask(player.getClass(),
        player.GetPrimaryTalentTree(player.GetActiveSpec()), player.GetShapeshiftForm()));
}
