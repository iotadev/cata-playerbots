/* GPL v2 or later. See AUTHORS.md. Disposable test preparation, not a bot factory. */
#ifndef PLAYERBOT_DEV_FIXTURE_H
#define PLAYERBOT_DEV_FIXTURE_H
#include "ObjectGuid.h"
#include "SharedDefines.h"
#include <optional>
#include <string_view>
class Player;
namespace PlayerbotDevFixture
{
enum class Role { Protection, Arms, Frost, Holy, CombatRogue, MarksmanHunter };
inline std::optional<Role> ParseRole(std::string_view name)
{
    if (name == "protection") return Role::Protection;
    if (name == "arms") return Role::Arms;
    if (name == "frost") return Role::Frost;
    if (name == "holy") return Role::Holy;
    if (name == "rogue") return Role::CombatRogue;
    if (name == "hunter") return Role::MarksmanHunter;
    return {};
}
inline uint8 ClassFor(Role role)
{
    switch (role)
    {
        case Role::Protection: case Role::Arms: return CLASS_WARRIOR;
        case Role::Frost: return CLASS_MAGE;
        case Role::Holy: return CLASS_PRIEST;
        case Role::CombatRogue: return CLASS_ROGUE;
        case Role::MarksmanHunter: return CLASS_HUNTER;
        default: return 0;
    }
}
inline uint8 TabFor(Role role)
{
    switch (role)
    {
        case Role::Arms: return 0;
        case Role::Holy: case Role::CombatRogue: case Role::MarksmanHunter: return 1;
        case Role::Protection: case Role::Frost: return 2;
        default: return 255;
    }
}
void SetEnabled(bool enabled);
void SetRollFixtureEnabled(bool enabled);
bool Request(ObjectGuid guid, Role role);
void Process(Player& bot); // map-thread only; consumes one bounded request
}
#endif
