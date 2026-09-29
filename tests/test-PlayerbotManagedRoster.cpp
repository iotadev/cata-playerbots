/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/PlayerbotManagedRoster.h"
#include <catch2/catch.hpp>

TEST_CASE("Managed roster accepts unique account and character bindings", "[playerbots][roster]")
{
    REQUIRE(PlayerbotManagedRoster::Configure(" 101:2001, 102:2002 "));
    REQUIRE(PlayerbotManagedRoster::List().size() == 2);
    REQUIRE(PlayerbotManagedRoster::List()[0].AccountId == 101);
    REQUIRE(PlayerbotManagedRoster::Find(2002)->AccountId == 102);
    REQUIRE(PlayerbotManagedRoster::Find(2003) == nullptr);
}

TEST_CASE("Managed roster rejects malformed and conflicting bindings without retaining old entries", "[playerbots][roster]")
{
    REQUIRE(PlayerbotManagedRoster::Configure("101:2001"));
    for (char const* invalid : { "101:2001,101:2002", "101:2001,102:2001", "101:2001,",
        "101:2001x", "0:2001", "101:0", "4294967296:2001" })
    {
        REQUIRE_FALSE(PlayerbotManagedRoster::Configure(invalid));
        REQUIRE(PlayerbotManagedRoster::List().empty());
    }
}
