/* GPL v2 or later. RepairAllAction donor adaptation; see PORTING.md. */
#ifndef PLAYERBOT_REPAIR_H
#define PLAYERBOT_REPAIR_H
#include "PlayerbotNpcRequest.h"
class WorldSession;
namespace PlayerbotRepair
{
using PlayerbotNpcRequest::Request;
using PlayerbotNpcRequest::Mailbox;
using PlayerbotNpcRequest::Fresh;
void Update(WorldSession& session, Mailbox& mailbox);
}
#endif
