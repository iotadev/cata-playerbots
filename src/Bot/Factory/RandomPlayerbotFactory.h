/*
 * Adapted from mod-playerbots RandomPlayerbotFactory at
 * 7bae1b5c58c76a0aa20381155edc08096d1485b2. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef CATA_RANDOMPLAYERBOTFACTORY_H
#define CATA_RANDOMPLAYERBOTFACTORY_H

#include "PlayerbotAppearance.h"
#include "PlayerbotFactoryOwnership.h"
#include <string>
#include <memory>

class NativeCharacterCreationReceipt;
struct PlayerbotFactoryPlan
{
    PlayerbotFactoryIdentity Identity;
    uint32_t ExistingGuidLow = 0;
};

class RandomPlayerbotFactory
{
public:
    // World-thread, read-only draft. Does not certify account/name eligibility,
    // reserve an identity, create a session or publish a managed character.
    static std::optional<PlayerbotAppearance> PreviewAppearance(
        uint8_t race, uint8_t classId, uint8_t gender, std::string& failure);
    static void SetInspectionEnabled(bool enabled);
    static void SetProvisioningEnabled(bool enabled);
    static bool CheckOwnershipSchema(std::string& detail);
    // Console diagnostic only. Reads operator evidence and native identities;
    // does not enroll accounts, create characters or grant managed admission.
    static PlayerbotFactoryDecision InspectOwnedAccount(uint32_t accountId, std::string& detail,
        PlayerbotFactoryPlan* plan = nullptr);
    static bool EnrollAccount(PlayerbotFactoryIdentity identity, std::string& detail);
    static bool ProvisionAccount(uint32_t accountId, std::string& detail);
    static std::string DescribeAttempt(uint32_t accountId);
    static void UpdateCallbacks();
};

#endif
