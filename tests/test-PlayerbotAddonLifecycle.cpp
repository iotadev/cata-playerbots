/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/Cmd/PlayerbotAddonLifecycle.h"
#include <catch2/catch.hpp>

TEST_CASE("MultiBot lifecycle never reports pending teardown complete", "[playerbots][addon]")
{
    using namespace PlayerbotAddonProtocol;
    ServerOriginPlayerbotLifecycle receipt;
    REQUIRE(std::string(View(&receipt).State) == "CONNECTING");
    receipt.LoginCompleted();
    REQUIRE(std::string(View(&receipt).State) == "ONLINE");
    receipt.StopRequested();
    REQUIRE(std::string(View(&receipt).State) == "CONNECTING");
    REQUIRE(std::string(View(&receipt).Reason) == "STOPPING");
    receipt.SessionClosed(false);
    REQUIRE(std::string(View(&receipt).State) == "OFFLINE");
    REQUIRE(std::string(View(nullptr).State) == "OFFLINE");
    ServerOriginPlayerbotLifecycle failed;
    failed.LoginFailed();
    REQUIRE(std::string(View(&failed).State) == "CONNECTING");
    failed.SessionClosed(false);
    REQUIRE(std::string(View(&failed).Reason) == "LOGIN_FAILED");
}

TEST_CASE("MultiBot mutations retain replay tokens across rate windows", "[playerbots][addon]")
{
    using namespace PlayerbotAddonProtocol;
    MutationGuard guard;
    REQUIRE(guard.Admit("first", 0) == nullptr);
    REQUIRE(std::string(guard.Admit("first", 2000)) == "REPLAY");
    for (unsigned i = 0; i < 63; ++i)
        REQUIRE(guard.Admit(std::to_string(i), 2000) == nullptr);
    REQUIRE(std::string(guard.Admit("next", 2000)) == "RATE_LIMIT");
    REQUIRE(guard.Admit("next", 4000) == nullptr);
    REQUIRE(guard.Admit("first", 120000) == nullptr);
}

TEST_CASE("MultiBot replay storage fails closed until tokens expire", "[playerbots][addon]")
{
    PlayerbotAddonProtocol::MutationGuard guard;
    for (unsigned i = 0; i < 320; ++i)
        REQUIRE(guard.Admit(std::to_string(i), (i / 64) * 2000) == nullptr);
    REQUIRE(std::string(guard.Admit("overflow", 10000)) == "TOKEN_LIMIT");
    REQUIRE(std::string(guard.Admit("0", 12000)) == "REPLAY");
    REQUIRE(guard.Admit("overflow", 120000) == nullptr);
}
