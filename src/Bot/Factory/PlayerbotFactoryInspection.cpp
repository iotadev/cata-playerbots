/*
 * Native ownership/persistence adaptation for RandomPlayerbotFactory.
 * See AUTHORS.md and PORTING.md. Released under GNU GPL v2 or later.
 */
#include "RandomPlayerbotFactory.h"
#include "AccountMgr.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "ObjectMgr.h"
#include "World.h"
#include "WorldSession.h"
#include "Realm.h"

namespace
{
    bool InspectionEnabled = false; // World-thread configuration and access only.
}

void RandomPlayerbotFactory::SetInspectionEnabled(bool enabled)
{
    InspectionEnabled = enabled;
}

bool RandomPlayerbotFactory::CheckOwnershipSchema(std::string& detail)
{
    // Missing optional tables/columns are fatal SQL errors in TrinityCore.
    // Inspect metadata first, without querying a possibly absent module table.
    // No caching: an operator may apply the optional schema before a later call.
    QueryResult schema = LoginDatabase.Query(
        "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() "
        "AND table_name='playerbots_factory_ownership' AND ("
        "(column_name IN ('account_id','realm_id','evidence_version') AND data_type='int' AND column_type LIKE '%unsigned') OR "
        "(column_name='account_join_epoch' AND data_type='bigint' AND column_type LIKE '%unsigned') OR "
        "(column_name IN ('race','class','gender') AND data_type='tinyint' AND column_type LIKE '%unsigned') OR "
        "(column_name='account_name' AND data_type='varchar' AND character_maximum_length>=32 AND collation_name='utf8mb4_bin') OR "
        "(column_name='character_name' AND data_type='varchar' AND character_maximum_length>=12 AND collation_name='utf8mb4_bin'))");
    if (!schema || (*schema)[0].GetUInt64() != 9)
    {
        detail = "optional factory ownership schema is missing or incompatible; apply the documented auth schema before enabling mutations";
        return false;
    }
    return true;
}

PlayerbotFactoryDecision RandomPlayerbotFactory::InspectOwnedAccount(uint32_t accountId, std::string& detail,
    PlayerbotFactoryPlan* plan)
{
    auto reject = [&detail](char const* reason)
    {
        detail = reason;
        return PlayerbotFactoryDecision::Reject;
    };
    if (!InspectionEnabled)
        return reject("factory inspection is disabled");
    if (!CheckOwnershipSchema(detail))
        return PlayerbotFactoryDecision::Reject;
    if (!accountId || sWorld->IsStopped() || sWorld->IsShuttingDown() ||
        sWorld->FindSession(accountId) || sWorld->IsCharacterProvisioningAccount(accountId))
        return reject("account is invalid, active, reserved or shutting down");

    // Only numeric arguments enter this query. LEFT JOIN preserves a native
    // account row with absent evidence; a missing result is never inferred empty.
    QueryResult account = LoginDatabase.PQuery(
        "SELECT a.username, CAST(UNIX_TIMESTAMP(a.joindate) AS UNSIGNED), a.online, a.expansion, "
        "CAST((SELECT COALESCE(MAX(SecurityLevel),0) FROM account_access WHERE AccountID=a.id) AS UNSIGNED), "
        "CAST((SELECT COALESCE(SUM(numchars),0) FROM realmcharacters WHERE acctid=a.id AND realmid<>%u) AS UNSIGNED), "
        "o.account_id, o.realm_id, o.account_name, o.account_join_epoch, o.evidence_version, "
        "o.character_name, o.race, o.class, o.gender "
        "FROM account a LEFT JOIN playerbots_factory_ownership o ON o.account_id=a.id AND o.realm_id=%u "
        "WHERE a.id=%u", realm.Id.Realm, realm.Id.Realm, accountId);
    if (!account || account->GetRowCount() != 1 || account->GetFieldCount() != 15)
        return reject("account/evidence query failed or account is missing; check the optional auth schema");
    Field* fields = account->Fetch();
    if (fields[6].IsNull())
        return reject("no explicit dedicated-account ownership evidence exists");
    PlayerbotFactoryOwnershipEvidence evidence{fields[6].GetUInt32(), fields[7].GetUInt32(),
        fields[8].GetString(), fields[9].GetUInt64(), fields[10].GetUInt32()};
    std::string accountName = fields[0].GetString();
    if (!MatchesFactoryOwnership(evidence, accountId, realm.Id.Realm, accountName, fields[1].GetUInt64()))
        return reject("ownership evidence version or native account identity does not match");
    if (fields[2].GetUInt8() || fields[4].GetUInt64() || fields[5].GetUInt64() ||
        sAccountMgr->IsBannedAccount(accountName))
        return reject("account is online, privileged, banned or has recorded characters in another realm");

    PlayerbotFactoryIdentity expected{accountId, fields[11].GetString(), fields[12].GetUInt8(),
        fields[13].GetUInt8(), fields[14].GetUInt8()};
    std::string normalizedName = expected.Name;
    if (!normalizePlayerName(normalizedName) || normalizedName != expected.Name ||
        ObjectMgr::CheckPlayerName(expected.Name, sWorld->GetDefaultDbcLocale(), true) != CHAR_NAME_SUCCESS)
        return reject("stored character name is not native-normalized or valid");
    std::string appearanceFailure;
    if (!PreviewAppearance(expected.Race, expected.Class, expected.Gender, appearanceFailure))
    {
        detail = "stored profile rejected: " + appearanceFailure;
        return PlayerbotFactoryDecision::Reject;
    }
    if (sChrRacesStore.LookupEntry(expected.Race)->Race_related > fields[3].GetUInt8() ||
        sChrClassesStore.LookupEntry(expected.Class)->Required_expansion > fields[3].GetUInt8())
        return reject("stored race/class requires an expansion unavailable to this account");
    // The native creation path must additionally enforce character limits and class unlocks.
    // Inspection is only a decision draft, never permission to bypass that path.
    std::string escapedName = expected.Name;
    CharacterDatabase.EscapeString(escapedName);
    QueryResult characters = CharacterDatabase.PQuery(
        "SELECT COUNT(*), CAST(COALESCE(MIN(guid),0) AS UNSIGNED), COALESCE(MIN(name),''), "
        "CAST(COALESCE(MIN(race),0) AS UNSIGNED), CAST(COALESCE(MIN(class),0) AS UNSIGNED), CAST(COALESCE(MIN(gender),0) AS UNSIGNED) "
        "FROM characters WHERE account=%u", accountId);
    QueryResult names = CharacterDatabase.PQuery("SELECT COUNT(*) FROM characters WHERE name='%s'", escapedName);
    if (!characters || characters->GetRowCount() != 1 || characters->GetFieldCount() != 6 ||
        !names || names->GetRowCount() != 1 || names->GetFieldCount() != 1)
        return reject("native character identity query failed");
    Field* character = characters->Fetch();
    uint64 count = character[0].GetUInt64();
    if (count > 1)
        return reject("dedicated account contains additional characters");
    PlayerbotFactoryIdentity actual{accountId, character[2].GetString(), uint8(character[3].GetUInt64()),
        uint8(character[4].GetUInt64()), uint8(character[5].GetUInt64())};
    bool exact = count == 1 && character[1].GetUInt64() && MatchesFactoryIdentity(expected, actual);
    bool conflict = (*names)[0].GetUInt64() != (exact ? 1u : 0u);
    auto decision = DecideFactoryProvisioning(true, true, true, uint32(count), conflict, exact);
    if (plan && decision != PlayerbotFactoryDecision::Reject)
        *plan = {expected, exact ? uint32(character[1].GetUInt64()) : 0u};
    detail = decision == PlayerbotFactoryDecision::Create ? "owned empty account; native creation would be required" :
        decision == PlayerbotFactoryDecision::Reuse ? "one exact owned character; accounting recovery still required before readiness" :
        "character identity or name conflicts with the stored intent";
    return decision;
}
