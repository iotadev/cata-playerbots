/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/PlayerbotManagedRoster.h"
#include <catch2/catch.hpp>

TEST_CASE("Managed lifecycle receipt distinguishes login, exit request and closure", "[playerbots][lifecycle]")
{
    ServerOriginPlayerbotLifecycle receipt;
    REQUIRE(receipt.GetState() == ServerOriginPlayerbotLifecycle::State::Loading);
    REQUIRE_FALSE(receipt.HasLoggedIn());
    receipt.LoginCompleted();
    REQUIRE(receipt.GetState() == ServerOriginPlayerbotLifecycle::State::Online);
    receipt.StopRequested();
    REQUIRE(receipt.GetState() == ServerOriginPlayerbotLifecycle::State::Stopping);
    REQUIRE_FALSE(receipt.IsClosed());
    receipt.SessionClosed(false);
    REQUIRE(receipt.GetState() == ServerOriginPlayerbotLifecycle::State::Stopped);
    REQUIRE(receipt.HasLoggedIn());
    REQUIRE(receipt.IsClosed());
}

TEST_CASE("Managed lifecycle reports failed login and cancellation during loading", "[playerbots][lifecycle]")
{
    ServerOriginPlayerbotLifecycle failed;
    failed.LoginFailed();
    REQUIRE(failed.GetState() == ServerOriginPlayerbotLifecycle::State::LoginFailed);
    REQUIRE_FALSE(failed.IsClosed());
    failed.SessionClosed(false);
    REQUIRE(failed.GetState() == ServerOriginPlayerbotLifecycle::State::LoginFailed);

    ServerOriginPlayerbotLifecycle cancelled;
    cancelled.StopRequested();
    cancelled.LoginFailed();
    REQUIRE(cancelled.GetState() == ServerOriginPlayerbotLifecycle::State::Stopping);
    cancelled.SessionClosed(false);
    REQUIRE(cancelled.GetState() == ServerOriginPlayerbotLifecycle::State::Stopped);
    REQUIRE_FALSE(cancelled.HasLoggedIn());
}

TEST_CASE("Managed lifecycle distinguishes unexpected closure and shutdown", "[playerbots][lifecycle]")
{
    ServerOriginPlayerbotLifecycle disconnected;
    disconnected.LoginCompleted();
    disconnected.SessionClosed(false);
    REQUIRE(disconnected.GetState() == ServerOriginPlayerbotLifecycle::State::Disconnected);
    ServerOriginPlayerbotLifecycle shutdown;
    shutdown.StopRequested();
    shutdown.SessionClosed(true);
    REQUIRE(shutdown.GetState() == ServerOriginPlayerbotLifecycle::State::Shutdown);
}

TEST_CASE("Managed receipts survive matching reloads and isolate older attempts", "[playerbots][roster][lifecycle]")
{
    REQUIRE(PlayerbotManagedRoster::Configure("101:2001"));
    auto oldAttempt = std::make_shared<ServerOriginPlayerbotLifecycle>();
    REQUIRE_FALSE(PlayerbotManagedRoster::Track(102, 2001, oldAttempt));
    REQUIRE(PlayerbotManagedRoster::Track(101, 2001, oldAttempt));
    REQUIRE(PlayerbotManagedRoster::Configure("101:2001"));
    REQUIRE(PlayerbotManagedRoster::Find(2001)->Lifecycle == oldAttempt);
    auto newAttempt = std::make_shared<ServerOriginPlayerbotLifecycle>();
    REQUIRE(PlayerbotManagedRoster::Track(101, 2001, newAttempt));
    oldAttempt->LoginFailed();
    oldAttempt->SessionClosed(false);
    REQUIRE(PlayerbotManagedRoster::Find(2001)->Lifecycle->GetState() == ServerOriginPlayerbotLifecycle::State::Loading);
    REQUIRE(PlayerbotManagedRoster::Configure("102:2001"));
    REQUIRE_FALSE(PlayerbotManagedRoster::Find(2001)->Lifecycle);
}

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
