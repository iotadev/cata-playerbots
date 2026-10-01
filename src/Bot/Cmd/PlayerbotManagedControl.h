/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef CATA_PLAYERBOTS_MANAGED_CONTROL_H
#define CATA_PLAYERBOTS_MANAGED_CONTROL_H

#include "PlayerbotRoster.h"
#include "ServerOriginPlayerbotLifecycle.h"
#include <memory>

enum class PlayerbotManagedResult { Accepted, AlreadyOffline, Disabled, NotConfigured, Unauthorized, AdmissionRejected };

struct PlayerbotManagedReply
{
    PlayerbotManagedResult Result;
    std::shared_ptr<ServerOriginPlayerbotLifecycle const> Receipt;
};

struct PlayerbotManagedRosterEntry : PlayerbotRosterEntry
{
    // Offline identities without a previous admission have no receipt.
    std::shared_ptr<ServerOriginPlayerbotLifecycle const> Receipt;
};

// World-thread service for player transports. Rechecks current account links,
// native identity and party control on every request; retains no Player pointers.
class PlayerbotManagedControl
{
public:
    static std::vector<PlayerbotManagedRosterEntry> ListFor(Player& requester);
    static PlayerbotManagedReply Start(Player& requester, ObjectGuid botGuid);
    static PlayerbotManagedReply Stop(Player& requester, ObjectGuid botGuid);
};
#endif
