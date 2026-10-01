/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef CATA_PLAYERBOT_FACTORY_OWNERSHIP_H
#define CATA_PLAYERBOT_FACTORY_OWNERSHIP_H

#include <cstdint>
#include <cstddef>
#include <string>

struct PlayerbotFactoryIdentity
{
    uint32_t AccountId;
    std::string Name; // Native-normalized name, not an addon-provided alias.
    uint8_t Race;
    uint8_t Class;
    uint8_t Gender;
};

struct PlayerbotFactoryOwnershipEvidence
{
    uint32_t AccountId;
    uint32_t RealmId;
    std::string AccountName;
    uint64_t AccountJoinEpoch;
    uint32_t Version;
};

// A privileged operator explicitly dedicates an account through this record.
// Binding the native identity prevents a stale row from silently adopting a
// renamed or recreated account. It is not proof against a database administrator.
inline bool MatchesFactoryOwnership(PlayerbotFactoryOwnershipEvidence const& evidence,
    uint32_t accountId, uint32_t realmId, std::string const& accountName, uint64_t accountJoinEpoch)
{
    return accountId && realmId && !accountName.empty() && accountJoinEpoch && evidence.Version == 1 &&
        evidence.AccountId == accountId && evidence.RealmId == realmId &&
        evidence.AccountName == accountName && evidence.AccountJoinEpoch == accountJoinEpoch;
}

inline bool MatchesFactoryIdentity(PlayerbotFactoryIdentity const& expected, PlayerbotFactoryIdentity const& actual)
{
    return expected.AccountId && expected.Race && expected.Class && expected.Gender <= 1 &&
        expected.AccountId == actual.AccountId && !expected.Name.empty() &&
        expected.Name == actual.Name && expected.Race == actual.Race && expected.Class == actual.Class &&
        expected.Gender == actual.Gender;
}

enum class PlayerbotFactoryDecision { Reject, Create, Reuse };

inline bool CanRecordFactoryAttempt(bool enabled, bool shuttingDown, bool pending,
    std::size_t trackedAccounts, bool knownAccount)
{
    return enabled && !shuttingDown && !pending && (knownAccount || trackedAccounts < 16);
}

// Inputs must come from trusted ownership evidence and authoritative database
// reads. Defaults/failed reads, name prefixes and party membership are not proof.
// One existing character per dedicated account in this first factory slice.
inline PlayerbotFactoryDecision DecideFactoryProvisioning(bool ownershipVerified, bool profileEligible,
    bool identityReadsSucceeded, uint32_t existingCharacterCount, bool nameConflicts, bool exactExistingIdentity)
{
    if (!ownershipVerified || !profileEligible || !identityReadsSucceeded || nameConflicts)
        return PlayerbotFactoryDecision::Reject;
    if (!existingCharacterCount && !exactExistingIdentity)
        return PlayerbotFactoryDecision::Create;
    if (existingCharacterCount == 1 && exactExistingIdentity)
        return PlayerbotFactoryDecision::Reuse;
    return PlayerbotFactoryDecision::Reject;
}

#endif
