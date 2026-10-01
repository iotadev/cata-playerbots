/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/Factory/PlayerbotFactoryOwnership.h"
#include <catch2/catch.hpp>

TEST_CASE("Factory requests are gated bounded and reject overlapping attempts", "[playerbots][factory]")
{
    REQUIRE(CanRecordFactoryAttempt(true, false, false, 0, false));
    REQUIRE_FALSE(CanRecordFactoryAttempt(false, false, false, 0, false));
    REQUIRE_FALSE(CanRecordFactoryAttempt(true, true, false, 0, false));
    REQUIRE_FALSE(CanRecordFactoryAttempt(true, false, true, 0, true));
    REQUIRE_FALSE(CanRecordFactoryAttempt(true, false, false, 16, false));
    REQUIRE(CanRecordFactoryAttempt(true, false, false, 16, true));
}

TEST_CASE("Factory ownership binds operator evidence to native account and realm", "[playerbots][factory]")
{
    PlayerbotFactoryOwnershipEvidence evidence{1, 2, "DEDICATED", 12345, 1};
    REQUIRE(MatchesFactoryOwnership(evidence, 1, 2, "DEDICATED", 12345));
    REQUIRE_FALSE(MatchesFactoryOwnership(evidence, 3, 2, "DEDICATED", 12345));
    REQUIRE_FALSE(MatchesFactoryOwnership(evidence, 1, 3, "DEDICATED", 12345));
    REQUIRE_FALSE(MatchesFactoryOwnership(evidence, 1, 2, "RENAMED", 12345));
    REQUIRE_FALSE(MatchesFactoryOwnership(evidence, 1, 2, "DEDICATED", 12346));
    evidence.Version = 0;
    REQUIRE_FALSE(MatchesFactoryOwnership(evidence, 1, 2, "DEDICATED", 12345));
    evidence.Version = 2;
    REQUIRE_FALSE(MatchesFactoryOwnership(evidence, 1, 2, "DEDICATED", 12345));
}

TEST_CASE("Factory ownership does not accept missing native identity fields", "[playerbots][factory]")
{
    PlayerbotFactoryOwnershipEvidence evidence{0, 0, "", 0, 1};
    REQUIRE_FALSE(MatchesFactoryOwnership(evidence, 0, 0, "", 0));
    evidence = {1, 2, "DEDICATED", 0, 1};
    REQUIRE_FALSE(MatchesFactoryOwnership(evidence, 1, 2, "DEDICATED", 0));
}

TEST_CASE("Factory reuse requires exact native identity not a name prefix", "[playerbots][factory]")
{
    PlayerbotFactoryIdentity expected{1, "Botmage", 10, 8, 0};
    REQUIRE(MatchesFactoryIdentity(expected, expected));
    for (auto actual : {PlayerbotFactoryIdentity{2, "Botmage", 10, 8, 0},
        {1, "Botmagealt", 10, 8, 0}, {1, "Botmage", 1, 8, 0},
        {1, "Botmage", 10, 5, 0}, {1, "Botmage", 10, 8, 1}})
        REQUIRE_FALSE(MatchesFactoryIdentity(expected, actual));
    PlayerbotFactoryIdentity missing{0, "", 0, 0, 0};
    REQUIRE_FALSE(MatchesFactoryIdentity(missing, missing));
}

TEST_CASE("Factory does not adopt unverified or ineligible accounts", "[playerbots][factory]")
{
    REQUIRE(DecideFactoryProvisioning(false, true, true, 0, false, false) == PlayerbotFactoryDecision::Reject);
    REQUIRE(DecideFactoryProvisioning(true, false, true, 0, false, false) == PlayerbotFactoryDecision::Reject);
    REQUIRE(DecideFactoryProvisioning(true, true, false, 0, false, false) == PlayerbotFactoryDecision::Reject);
    REQUIRE(DecideFactoryProvisioning(true, true, true, 0, true, false) == PlayerbotFactoryDecision::Reject);
}

TEST_CASE("Factory creates only empty owned accounts and reuses one exact character", "[playerbots][factory]")
{
    REQUIRE(DecideFactoryProvisioning(true, true, true, 0, false, false) == PlayerbotFactoryDecision::Create);
    REQUIRE(DecideFactoryProvisioning(true, true, true, 1, false, true) == PlayerbotFactoryDecision::Reuse);
    REQUIRE(DecideFactoryProvisioning(true, true, true, 1, false, false) == PlayerbotFactoryDecision::Reject);
    REQUIRE(DecideFactoryProvisioning(true, true, true, 2, false, true) == PlayerbotFactoryDecision::Reject);
    REQUIRE(DecideFactoryProvisioning(true, true, true, 0, false, true) == PlayerbotFactoryDecision::Reject);
}
