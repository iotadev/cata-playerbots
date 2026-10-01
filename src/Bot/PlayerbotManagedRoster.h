/*
 * Adapted from AzerothCore mod-playerbots PlayerbotMgr identity and login
 * ownership at 7bae1b5c58c76a0aa20381155edc08096d1485b2.
 * See AUTHORS.md and PORTING.md. Released under GNU GPL v2 or later.
 */
#ifndef CATA_PLAYERBOTS_MANAGED_ROSTER_H
#define CATA_PLAYERBOTS_MANAGED_ROSTER_H

#include "Define.h"
#include "ServerOriginPlayerbotLifecycle.h"
#include <memory>
#include <string>
#include <vector>

struct ManagedPlayerbotIdentity
{
    uint32 AccountId;
    uint32 CharacterGuidLow;
    std::shared_ptr<ServerOriginPlayerbotLifecycle> Lifecycle;
};

// Configured identities are independent of a temporary party controller.
// Access only on the world thread; Find returns a borrowed pointer.
class PlayerbotManagedRoster
{
public:
    static bool Configure(std::string const& bindings);
    static std::vector<ManagedPlayerbotIdentity> const& List();
    static ManagedPlayerbotIdentity const* Find(uint32 characterGuidLow);
    static bool Track(uint32 accountId, uint32 characterGuidLow,
        std::shared_ptr<ServerOriginPlayerbotLifecycle> lifecycle);
    static bool ConfigureAccountLinks(std::string const& links);
    static bool IsAccountLinked(uint32 requesterAccountId, uint32 botAccountId);
    static void SetPlayerControlEnabled(bool enabled);
    static bool IsPlayerControlEnabled();
};

#endif
