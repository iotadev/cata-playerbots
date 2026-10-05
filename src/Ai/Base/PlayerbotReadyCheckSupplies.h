/* Cata adaptation of donor ItemCountValue/ItemVisitors at
 * 037c01418b5d01506917a3db9b44fd56ac5f965c. GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_READY_CHECK_SUPPLIES_H
#define PLAYERBOT_READY_CHECK_SUPPLIES_H
#include "../../Bot/PlayerbotReadyCheck.h"
class Player;
namespace PlayerbotReadyCheck
{
// Map-thread only. Carried backpack/bags, never bank/equipment or cached Item*.
Supplies CarriedSupplies(Player& bot);
}
#endif
