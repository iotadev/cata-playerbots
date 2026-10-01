/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/Cmd/PlayerbotAddonProtocol.h"
#include <catch2/catch.hpp>

TEST_CASE("MultiBot advertises only implemented managed capabilities when enabled", "[playerbots][addon]")
{
    REQUIRE(PlayerbotAddonProtocol::ManagedCapabilities(false).empty());
    REQUIRE(PlayerbotAddonProtocol::ManagedCapabilities(true) == "ALT_ROSTER_V1,BOT_LIFECYCLE_V1");
}

TEST_CASE("MultiBot initial request contract is exact and versioned", "[playerbots][addon]")
{
    using namespace PlayerbotAddonProtocol;
    REQUIRE(Parse("HELLO~1").Kind == Request::Hello);
    REQUIRE(Parse("GET~ROSTER").Kind == Request::Roster);
    REQUIRE(Parse("PING~check-1_2").Payload == "check-1_2");
    REQUIRE(Parse("PING~check:1.2").Kind == Request::Ping);
    REQUIRE(EncodeField("a~% b") == "a%7E%25%20b");
    for (std::string const& message : { "HELLO~2", "HELLO~1~extra", "GET~ROSTER~other", "RUN~UNSUPPORTED~1~t", "hello~1" })
        REQUIRE(Parse(message).Kind == Request::Invalid);
}

TEST_CASE("MultiBot lifecycle requests accept only native low GUID and bounded token", "[playerbots][addon]")
{
    using namespace PlayerbotAddonProtocol;
    REQUIRE(Parse("RUN~BOT_CONNECT~1~t").Kind == Request::Connect);
    REQUIRE(Parse("RUN~BOT_DISCONNECT~4294967295~t").GuidLow == 4294967295U);
    REQUIRE(Parse("GET~BOT_LIFECYCLE_STATE~12~poll:1").Kind == Request::LifecycleState);
    for (std::string const& fields : { "0~t", "4294967296~t", "-1~t", "+1~t", "1x~t", "~t", "1~", "1~t~extra", "1" })
        REQUIRE(Parse("RUN~BOT_CONNECT~" + fields).Kind == Request::Invalid);
}

TEST_CASE("MultiBot frames reject oversized controls and invalid tokens", "[playerbots][addon]")
{
    using namespace PlayerbotAddonProtocol;
    REQUIRE(Parse(std::string(251, 'x')).Kind == Request::Invalid);
    REQUIRE(Parse(std::string("PING~x\0y", 8)).Kind == Request::Invalid);
    REQUIRE(Parse("PING~\nx").Kind == Request::Invalid);
    REQUIRE(Parse("PING~").Kind == Request::Invalid);
    REQUIRE(Parse("PING~x~y").Kind == Request::Invalid);
    REQUIRE(Parse("PING~" + std::string(65, 'x')).Kind == Request::Invalid);
    REQUIRE(Parse("PING~" + std::string(64, 'x')).Kind == Request::Ping);
}
