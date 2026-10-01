/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "PlayerbotManagedControl.h"
#include "PlayerbotManagedRoster.h"
#include "PlayerbotManagedSecurity.h"
#include "PlayerbotSecurity.h"
#include "CharacterCache.h"
#include "DBCStores.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"

namespace
{
PlayerbotManagedAccess AccessFor(Player& requester, ManagedPlayerbotIdentity const& identity)
{
    PlayerbotManagedAccess access;
    access.Enabled = PlayerbotManagedRoster::IsPlayerControlEnabled();
    WorldSession const* requesterSession = requester.GetSession();
    access.HumanInWorld = requester.IsInWorld() && requesterSession && !requesterSession->IsServerOrigin();
    if (!access.HumanInWorld)
        return access;

    ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(identity.CharacterGuidLow);
    CharacterCacheEntry const* character = sCharacterCache->GetCharacterCacheByGuid(guid);
    access.ValidIdentity = character && character->AccountId == identity.AccountId &&
        WorldSession::IsSupportedServerOriginClass(character->Class) && sChrRacesStore.LookupEntry(character->Race);
    if (!access.ValidIdentity)
        return access;

    access.GameMaster = requester.CanBeGameMaster();
    access.LinkedAccount = PlayerbotManagedRoster::IsAccountLinked(requesterSession->GetAccountId(), identity.AccountId);
    access.SameTeam = requester.GetTeam() == Player::TeamForRace(character->Race);
    if (WorldSession* session = sWorld->FindServerOriginPlayerbot(guid))
    {
        access.ValidIdentity = session->GetAccountId() == identity.AccountId;
        if (Player* bot = session->GetPlayer())
        {
            access.Grouped = bot->GetGroup() != nullptr;
            access.FullPartyControl = PlayerbotSecurity(*bot).CheckLevelFor(PLAYERBOT_SECURITY_ALLOW_ALL, requester);
        }
    }
    return access;
}

ManagedPlayerbotIdentity const* FindIdentity(ObjectGuid guid)
{
    return !guid.IsEmpty() && guid.IsPlayer() ? PlayerbotManagedRoster::Find(guid.GetCounter()) : nullptr;
}
}

std::vector<PlayerbotManagedRosterEntry> PlayerbotManagedControl::ListFor(Player& requester)
{
    std::vector<PlayerbotManagedRosterEntry> result;
    if (!PlayerbotManagedRoster::IsPlayerControlEnabled())
        return result;
    for (ManagedPlayerbotIdentity const& identity : PlayerbotManagedRoster::List())
    {
        if (!AccessFor(requester, identity).MayStopOrList())
            continue;
        ObjectGuid guid = ObjectGuid::Create<HighGuid::Player>(identity.CharacterGuidLow);
        CharacterCacheEntry const* character = sCharacterCache->GetCharacterCacheByGuid(guid);
        WorldSession* session = sWorld->FindServerOriginPlayerbot(guid);
        PlayerbotManagedRosterEntry entry;
        entry.Guid = guid;
        entry.Name = character->Name;
        entry.Class = character->Class;
        entry.Level = character->Level;
        entry.Receipt = session ? session->GetServerOriginLifecycle() : identity.Lifecycle;
        result.push_back(std::move(entry));
    }
    return result;
}

PlayerbotManagedReply PlayerbotManagedControl::Start(Player& requester, ObjectGuid botGuid)
{
    if (!PlayerbotManagedRoster::IsPlayerControlEnabled() || !sWorld->getBoolConfig(CONFIG_PLAYERBOTS_MANAGED_ENABLED))
        return { PlayerbotManagedResult::Disabled, nullptr };
    ManagedPlayerbotIdentity const* identity = FindIdentity(botGuid);
    if (!identity)
        return { PlayerbotManagedResult::NotConfigured, nullptr };
    if (!AccessFor(requester, *identity).MayStart())
        return { PlayerbotManagedResult::Unauthorized, nullptr };
    if (!sWorld->TryStartServerOriginPlayerbot(identity->AccountId, botGuid))
        return { PlayerbotManagedResult::AdmissionRejected, nullptr };
    WorldSession* session = sWorld->FindServerOriginPlayerbot(botGuid);
    ASSERT(session && session->GetAccountId() == identity->AccountId);
    auto receipt = session->GetServerOriginLifecycle();
    PlayerbotManagedRoster::Track(identity->AccountId, identity->CharacterGuidLow, receipt);
    return { PlayerbotManagedResult::Accepted, std::move(receipt) };
}

PlayerbotManagedReply PlayerbotManagedControl::Stop(Player& requester, ObjectGuid botGuid)
{
    if (!PlayerbotManagedRoster::IsPlayerControlEnabled())
        return { PlayerbotManagedResult::Disabled, nullptr };
    ManagedPlayerbotIdentity const* identity = FindIdentity(botGuid);
    if (!identity)
        return { PlayerbotManagedResult::NotConfigured, nullptr };
    if (!AccessFor(requester, *identity).MayStopOrList())
        return { PlayerbotManagedResult::Unauthorized, nullptr };
    WorldSession* session = sWorld->FindServerOriginPlayerbot(botGuid);
    if (!session)
        return { PlayerbotManagedResult::AlreadyOffline, identity->Lifecycle };
    auto receipt = session->GetServerOriginLifecycle();
    PlayerbotManagedRoster::Track(identity->AccountId, identity->CharacterGuidLow, receipt);
    sWorld->RequestStopServerOriginPlayerbot(botGuid);
    return { PlayerbotManagedResult::Accepted, std::move(receipt) };
}
