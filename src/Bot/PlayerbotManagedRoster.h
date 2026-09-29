/*
 * Adapted from AzerothCore mod-playerbots PlayerbotMgr identity and login
 * ownership at 7bae1b5c58c76a0aa20381155edc08096d1485b2.
 * See AUTHORS.md and PORTING.md. Released under GNU GPL v2 or later.
 */
#ifndef CATA_PLAYERBOTS_MANAGED_ROSTER_H
#define CATA_PLAYERBOTS_MANAGED_ROSTER_H

#include "Define.h"
#include <string>
#include <vector>

struct ManagedPlayerbotIdentity
{
    uint32 AccountId;
    uint32 CharacterGuidLow;
};

// Configured identities are independent of a temporary party controller.
// Access only on the world thread; Find returns a borrowed pointer.
class PlayerbotManagedRoster
{
public:
    static bool Configure(std::string const& bindings);
    static std::vector<ManagedPlayerbotIdentity> const& List();
    static ManagedPlayerbotIdentity const* Find(uint32 characterGuidLow);
};

#endif
