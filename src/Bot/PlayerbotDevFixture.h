/* GPL v2 or later. See AUTHORS.md. Disposable test preparation, not a bot factory. */
#ifndef PLAYERBOT_DEV_FIXTURE_H
#define PLAYERBOT_DEV_FIXTURE_H
#include "ObjectGuid.h"
class Player;
namespace PlayerbotDevFixture
{
enum class Role { Protection, Arms, Frost, Holy };
void SetEnabled(bool enabled);
bool Request(ObjectGuid guid, Role role);
void Process(Player& bot); // map-thread only; consumes one bounded request
}
#endif
