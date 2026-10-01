/*
 * Bounded Cata adaptation of the RandomPlayerbotFactory account/creation flow.
 * Donor: 7bae1b5c58c76a0aa20381155edc08096d1485b2. See PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "RandomPlayerbotFactory.h"
#include "NativeCharacterCreationReceipt.h"
#include "AsyncCallbackProcessor.h"
#include "DatabaseEnv.h"
#include "AccountMgr.h"
#include "DBCStores.h"
#include "ObjectMgr.h"
#include "Realm.h"
#include "World.h"
#include "WorldSession.h"
#include <unordered_map>

namespace
{
    struct FactoryAttempt
    {
        bool EnrollmentPending = false;
        std::string EnrollmentStatus;
        std::shared_ptr<NativeCharacterCreationReceipt const> Receipt;
    };
    // World-thread-only. At most 16 account histories, one current attempt each.
    bool ProvisioningEnabled = false;
    std::unordered_map<uint32, std::shared_ptr<FactoryAttempt>> Attempts;
    AsyncCallbackProcessor<TransactionCallback> EnrollmentCallbacks;

    bool CanSubmit(uint32 accountId, std::string& detail)
    {
        auto found = Attempts.find(accountId);
        bool pending = found != Attempts.end() && (found->second->EnrollmentPending ||
            (found->second->Receipt && (found->second->Receipt->GetState() == NativeCharacterCreationReceipt::State::Pending ||
                found->second->Receipt->GetState() == NativeCharacterCreationReceipt::State::Reconciling)));
        if (!accountId || !CanRecordFactoryAttempt(ProvisioningEnabled,
            sWorld->IsStopped() || sWorld->IsShuttingDown(), pending, Attempts.size(), found != Attempts.end()))
        {
            detail = "factory disabled, shutting down, invalid account, pending attempt or 16-account history limit";
            return false;
        }
        return true;
    }
}

void RandomPlayerbotFactory::SetProvisioningEnabled(bool enabled)
{
    ProvisioningEnabled = enabled;
}

void RandomPlayerbotFactory::UpdateCallbacks()
{
    // Continue already-submitted receipts even if configuration is disabled.
    EnrollmentCallbacks.ProcessReadyCallbacks();
}

bool RandomPlayerbotFactory::EnrollAccount(PlayerbotFactoryIdentity identity, std::string& detail)
{
    if (!CanSubmit(identity.AccountId, detail))
        return false;
    if (!CheckOwnershipSchema(detail))
        return false;
    auto reject = [&detail](char const* reason) { detail = reason; return false; };
    if (sWorld->FindSession(identity.AccountId) || sWorld->IsCharacterProvisioningAccount(identity.AccountId))
        return reject("account is active or reserved");
    if (!normalizePlayerName(identity.Name) ||
        ObjectMgr::CheckPlayerName(identity.Name, sWorld->GetDefaultDbcLocale(), true) != CHAR_NAME_SUCCESS)
        return reject("invalid character intent name");
    if (!PreviewAppearance(identity.Race, identity.Class, identity.Gender, detail))
        return false;
    QueryResult account = LoginDatabase.PQuery(
        "SELECT a.username, CAST(UNIX_TIMESTAMP(a.joindate) AS UNSIGNED), a.online, a.expansion, "
        "CAST((SELECT COALESCE(MAX(SecurityLevel),0) FROM account_access WHERE AccountID=a.id) AS UNSIGNED), "
        "CAST((SELECT COALESCE(SUM(numchars),0) FROM realmcharacters WHERE acctid=a.id) AS UNSIGNED), "
        "(SELECT COUNT(*) FROM playerbots_factory_ownership WHERE account_id=a.id) "
        "FROM account a WHERE a.id=%u", identity.AccountId);
    if (!account || account->GetRowCount() != 1 || account->GetFieldCount() != 7)
        return reject("account/schema read failed or account is missing");
    Field* fields = account->Fetch();
    std::string accountName = fields[0].GetString();
    uint64 joinEpoch = fields[1].GetUInt64();
    if (accountName.empty() || !joinEpoch || !realm.Id.Realm || fields[2].GetUInt8() ||
        fields[4].GetUInt64() || fields[5].GetUInt64() || fields[6].GetUInt64() ||
        sAccountMgr->IsBannedAccount(accountName))
        return reject("account is ineligible, already enrolled or has recorded characters");
    if (sChrRacesStore.LookupEntry(identity.Race)->Race_related > fields[3].GetUInt8() ||
        sChrClassesStore.LookupEntry(identity.Class)->Required_expansion > fields[3].GetUInt8())
        return reject("account expansion does not support the intended profile");
    QueryResult characters = CharacterDatabase.PQuery("SELECT COUNT(*) FROM characters WHERE account=%u", identity.AccountId);
    std::string escapedName = identity.Name;
    CharacterDatabase.EscapeString(escapedName);
    QueryResult names = CharacterDatabase.PQuery("SELECT COUNT(*) FROM characters WHERE name='%s'", escapedName);
    if (!characters || !names || (*characters)[0].GetUInt64() || (*names)[0].GetUInt64())
        return reject("character reads failed, account is not empty or intended name is taken");

    // Explicit console dedication, not a prefix scan. No native account writes,
    // password changes, automatic adoption or cross-database atomicity claims.
    std::string authName = identity.Name;
    std::string escapedAccount = accountName;
    LoginDatabase.EscapeString(authName);
    LoginDatabase.EscapeString(escapedAccount);
    auto attempt = std::make_shared<FactoryAttempt>();
    attempt->EnrollmentPending = true;
    attempt->EnrollmentStatus = "enrollment pending";
    Attempts[identity.AccountId] = attempt;
    LoginDatabaseTransaction transaction = LoginDatabase.BeginTransaction();
    // Plain INSERT: an existing/conflicting record is never overwritten.
    // Recheck mutable auth eligibility on the same transaction as insertion.
    transaction->PAppend(
        "INSERT INTO playerbots_factory_ownership "
        "(account_id,realm_id,account_name,account_join_epoch,evidence_version,character_name,race,class,gender) "
        "SELECT id,%u,username,UNIX_TIMESTAMP(joindate),1,'%s',%u,%u,%u FROM account "
        "WHERE id=%u AND BINARY username=BINARY '%s' AND UNIX_TIMESTAMP(joindate)=%llu AND online=0 "
        "AND NOT EXISTS (SELECT 1 FROM account_access WHERE AccountID=%u AND SecurityLevel<>0) "
        "AND NOT EXISTS (SELECT 1 FROM realmcharacters WHERE acctid=%u AND numchars<>0) "
        "AND NOT EXISTS (SELECT 1 FROM account_banned WHERE id=%u AND active=1 AND (unbandate>UNIX_TIMESTAMP() OR unbandate=bandate))",
        realm.Id.Realm, authName, uint32(identity.Race), uint32(identity.Class), uint32(identity.Gender),
        identity.AccountId, escapedAccount, static_cast<unsigned long long>(joinEpoch),
        identity.AccountId, identity.AccountId, identity.AccountId);
    EnrollmentCallbacks.AddCallback(LoginDatabase.AsyncCommitTransaction(transaction)).AfterComplete(
        [attempt, identity, accountName, joinEpoch](bool success)
        {
            attempt->EnrollmentPending = false;
            // A committed INSERT...SELECT can affect zero rows. Confirm the
            // actual record with the same native account binding and intent.
            QueryResult record = success ? LoginDatabase.PQuery(
                "SELECT account_id,realm_id,account_name,account_join_epoch,evidence_version,character_name,race,class,gender "
                "FROM playerbots_factory_ownership WHERE account_id=%u AND realm_id=%u",
                identity.AccountId, realm.Id.Realm) : QueryResult(nullptr);
            bool confirmed = false;
            if (record && record->GetRowCount() == 1)
            {
                Field* row = record->Fetch();
                PlayerbotFactoryOwnershipEvidence evidence{row[0].GetUInt32(), row[1].GetUInt32(), row[2].GetString(),
                    row[3].GetUInt64(), row[4].GetUInt32()};
                PlayerbotFactoryIdentity stored{row[0].GetUInt32(), row[5].GetString(), row[6].GetUInt8(), row[7].GetUInt8(), row[8].GetUInt8()};
                confirmed = MatchesFactoryOwnership(evidence, identity.AccountId, realm.Id.Realm, accountName, joinEpoch) &&
                    MatchesFactoryIdentity(identity, stored);
            }
            attempt->EnrollmentStatus = confirmed ? "enrolled; no character created; eligibility is rechecked on provision" :
                "enrollment failed or unconfirmed; inspect persisted evidence before retry";
        });
    detail = "enrollment submitted; use factory-status to confirm completion";
    return true;
}

bool RandomPlayerbotFactory::ProvisionAccount(uint32_t accountId, std::string& detail)
{
    if (!CanSubmit(accountId, detail))
        return false;
    PlayerbotFactoryPlan plan;
    auto decision = InspectOwnedAccount(accountId, detail, &plan);
    if (decision == PlayerbotFactoryDecision::Reject)
        return false;
    std::shared_ptr<NativeCharacterCreationReceipt const> receipt;
    if (decision == PlayerbotFactoryDecision::Reuse)
        receipt = sWorld->BeginCharacterReconciliation(accountId, ObjectGuid::Create<HighGuid::Player>(plan.ExistingGuidLow));
    else
    {
        auto appearance = PreviewAppearance(plan.Identity.Race, plan.Identity.Class, plan.Identity.Gender, detail);
        if (!appearance)
            return false;
        CharacterCreateInfo request(plan.Identity.Name, plan.Identity.Race, plan.Identity.Class, plan.Identity.Gender,
            appearance->Skin, appearance->Face, appearance->HairStyle, appearance->HairColor, appearance->FacialHair);
        receipt = sWorld->BeginCharacterProvisioning(accountId, request);
    }
    if (!receipt)
    {
        detail = "native provisioning/reconciliation rejected; no ready identity published";
        return false;
    }
    auto attempt = std::make_shared<FactoryAttempt>();
    attempt->Receipt = std::move(receipt);
    Attempts[accountId] = std::move(attempt);
    detail = decision == PlayerbotFactoryDecision::Reuse ? "existing identity accounting recovery submitted" : "native character creation submitted";
    return true;
}

std::string RandomPlayerbotFactory::DescribeAttempt(uint32_t accountId)
{
    auto found = Attempts.find(accountId);
    if (found == Attempts.end())
        return "no attempt recorded in this process";
    auto const& attempt = *found->second;
    if (!attempt.Receipt)
        return attempt.EnrollmentStatus;
    auto const& receipt = *attempt.Receipt;
    using State = NativeCharacterCreationReceipt::State;
    switch (receipt.GetState())
    {
        case State::Pending: return "native creation pending";
        case State::Reconciling: return "realm accounting reconciliation pending";
        case State::Succeeded: return "provisioned GUID " + std::to_string(receipt.GetCharacterGuidLow()) + "; not automatically admitted or added to managed config";
        case State::Rejected: return "native creation rejected, result " + std::to_string(receipt.GetNativeResult());
        case State::AccountingFailed: return "accounting failed; committed GUID " + std::to_string(receipt.GetCommittedCharacterGuidLow()) + "; rerun provision for recovery";
        case State::Abandoned: return "abandoned; submitted writes may still have committed; inspect before rerun";
    }
    return "unknown receipt state";
}
