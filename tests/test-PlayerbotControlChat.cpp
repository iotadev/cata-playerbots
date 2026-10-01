/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/Cmd/PlayerbotControlChat.h"
#include <catch2/catch.hpp>

TEST_CASE("MultiBot basic chat controls share the existing exact command vocabulary", "[playerbots][chat]")
{
    PlayerbotControlCommand action;
    REQUIRE(ParsePlayerbotControlChat(NormalizePlayerbotControlChat("  FOLLOW\t"), action));
    REQUIRE(action == PlayerbotControlCommand::Follow);
    REQUIRE(ParsePlayerbotControlChat("stay", action));
    REQUIRE(action == PlayerbotControlCommand::Hold);
    REQUIRE(ParsePlayerbotControlChat("hold", action));
    REQUIRE(action == PlayerbotControlCommand::Hold);
    REQUIRE(ParsePlayerbotControlChat("attack", action));
    REQUIRE(action == PlayerbotControlCommand::Attack);
    REQUIRE(ParsePlayerbotControlChat("do attack my target", action));
    REQUIRE(action == PlayerbotControlCommand::Attack);
    REQUIRE(ParsePlayerbotControlChat("stop", action));
    REQUIRE(action == PlayerbotControlCommand::Cease);
    REQUIRE(ParsePlayerbotControlChat("cease", action));
    REQUIRE(action == PlayerbotControlCommand::Cease);
    for (std::string const& command : { "follow me", "attack; stop", "co +focus", "flee", "stay~x",
        "@ranged do attack my target", "do attack my target; stop", "" })
        REQUIRE_FALSE(ParsePlayerbotControlChat(command, action));
    REQUIRE(NormalizePlayerbotControlChat(std::string(257, 'x')).empty());
}

TEST_CASE("MultiBot party chat does not command other raid subgroups", "[playerbots][chat]")
{
    REQUIRE(PlayerbotControlChatReaches(false, 1, 1));
    REQUIRE_FALSE(PlayerbotControlChatReaches(false, 1, 2));
    REQUIRE(PlayerbotControlChatReaches(true, 1, 2));
}
