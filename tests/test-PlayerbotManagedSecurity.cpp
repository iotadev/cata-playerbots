/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Mgr/Security/PlayerbotManagedSecurity.h"
#include "../src/Bot/PlayerbotManagedRoster.h"
#include <catch2/catch.hpp>

TEST_CASE("Managed player access requires enabled service, real identity and human", "[playerbots][security]")
{
    PlayerbotManagedAccess access;
    access.GameMaster = true;
    REQUIRE_FALSE(access.MayStart());
    access.Enabled = true;
    access.ValidIdentity = true;
    REQUIRE_FALSE(access.MayStart());
    access.HumanInWorld = true;
    REQUIRE(access.MayStart());
    access.ValidIdentity = false;
    REQUIRE_FALSE(access.MayStopOrList());
}

TEST_CASE("Managed account links do not grant another party controller's logout rights", "[playerbots][security]")
{
    PlayerbotManagedAccess access;
    access.Enabled = access.ValidIdentity = access.HumanInWorld = true;
    access.SameTeam = true;
    access.FullPartyControl = true;
    REQUIRE_FALSE(access.MayStart()); // Party control alone is not managed account authority.
    access.LinkedAccount = true;
    REQUIRE(access.MayStart());
    access.Grouped = true;
    access.FullPartyControl = false;
    REQUIRE_FALSE(access.MayStopOrList());
    access.FullPartyControl = true;
    REQUIRE(access.MayStopOrList());
    access.SameTeam = false;
    REQUIRE_FALSE(access.MayStart());
    access.GameMaster = true;
    REQUIRE(access.MayStopOrList());
    access.Enabled = false;
    REQUIRE_FALSE(access.MayStopOrList());
}

TEST_CASE("Managed account links are directional and reload revokes previous permissions", "[playerbots][security]")
{
    REQUIRE(PlayerbotManagedRoster::ConfigureAccountLinks(" 1:101,1:102,2:101 "));
    REQUIRE(PlayerbotManagedRoster::IsAccountLinked(1, 101));
    REQUIRE(PlayerbotManagedRoster::IsAccountLinked(1, 102));
    REQUIRE(PlayerbotManagedRoster::IsAccountLinked(2, 101));
    REQUIRE_FALSE(PlayerbotManagedRoster::IsAccountLinked(101, 1));
    REQUIRE_FALSE(PlayerbotManagedRoster::IsAccountLinked(3, 101));
    REQUIRE(PlayerbotManagedRoster::ConfigureAccountLinks("1:102"));
    REQUIRE_FALSE(PlayerbotManagedRoster::IsAccountLinked(1, 101));
    REQUIRE(PlayerbotManagedRoster::ConfigureAccountLinks(""));
    REQUIRE_FALSE(PlayerbotManagedRoster::IsAccountLinked(1, 102));
}

TEST_CASE("Malformed or excessive account links disable player access without changing identities", "[playerbots][security]")
{
    REQUIRE(PlayerbotManagedRoster::Configure("101:2001"));
    for (char const* invalid : { "1:101,1:101", "1:1", "0:101", "1:0", "1:101,",
        "1:101x", "1:101:102", "4294967296:101" })
    {
        REQUIRE(PlayerbotManagedRoster::ConfigureAccountLinks("1:101"));
        PlayerbotManagedRoster::SetPlayerControlEnabled(true);
        REQUIRE_FALSE(PlayerbotManagedRoster::ConfigureAccountLinks(invalid));
        REQUIRE_FALSE(PlayerbotManagedRoster::IsPlayerControlEnabled());
        REQUIRE_FALSE(PlayerbotManagedRoster::IsAccountLinked(1, 101));
        REQUIRE(PlayerbotManagedRoster::Find(2001));
    }
    std::string links;
    for (unsigned int i = 0; i < 257; ++i)
        links += (i ? "," : "") + std::string("1:") + std::to_string(i + 100);
    REQUIRE_FALSE(PlayerbotManagedRoster::ConfigureAccountLinks(links));
    REQUIRE_FALSE(PlayerbotManagedRoster::IsAccountLinked(1, 100));
}
