/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "CharacterProvisioningPolicy.h"
#include <catch2/catch.hpp>

TEST_CASE("Character provisioning reservation is bounded and excludes active owners", "[playerbots][factory]")
{
    REQUIRE(CanBeginCharacterProvisioning(1, false, false, false, 0));
    REQUIRE_FALSE(CanBeginCharacterProvisioning(0, false, false, false, 0));
    REQUIRE_FALSE(CanBeginCharacterProvisioning(1, true, false, false, 0));
    REQUIRE_FALSE(CanBeginCharacterProvisioning(1, false, true, false, 0));
    REQUIRE_FALSE(CanBeginCharacterProvisioning(1, false, false, true, 0));
    REQUIRE(CanBeginCharacterProvisioning(1, false, false, false, MaxCharacterProvisioningContexts - 1));
    REQUIRE_FALSE(CanBeginCharacterProvisioning(1, false, false, false, MaxCharacterProvisioningContexts));
}

TEST_CASE("Character provisioning rejects online and privileged account profiles", "[playerbots][factory]")
{
    REQUIRE(CharacterProvisioningAccountEligible(false, 0));
    REQUIRE_FALSE(CharacterProvisioningAccountEligible(true, 0));
    REQUIRE_FALSE(CharacterProvisioningAccountEligible(false, 1));
    REQUIRE_FALSE(CharacterProvisioningAccountEligible(false, 3));
}
