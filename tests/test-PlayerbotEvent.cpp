/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/Engine/WorldPacket/Event.h"
#include "../src/Bot/Engine/Action/NextAction.h"
#include <catch2/catch.hpp>

TEST_CASE("Playerbot events preserve empty and named event semantics", "[playerbot][engine][event]")
{
    Event empty;
    REQUIRE(!empty);
    REQUIRE(empty.getOwnerGuid().IsEmpty());
    REQUIRE(empty.getObject().IsEmpty());
    Event named("combat");
    REQUIRE_FALSE(!named);
    REQUIRE(named.GetSource() == "combat");
    REQUIRE(named.getParam().empty());
}

TEST_CASE("Playerbot command event copies retain owner identity without a Player pointer", "[playerbot][engine][event]")
{
    ObjectGuid owner(HighGuid::Player, uint32(42));
    Event original("chat", "follow", owner);
    Event copy(original);
    Event assigned;
    assigned = original;
    REQUIRE(copy.GetSource() == "chat");
    REQUIRE(copy.getParam() == "follow");
    REQUIRE(copy.getOwnerGuid() == owner);
    REQUIRE(assigned.getOwnerGuid() == owner);
    original = Event("chat", "stay", ObjectGuid(HighGuid::Player, uint32(43)));
    REQUIRE(copy.getParam() == "follow");
    REQUIRE(copy.getOwnerGuid() == owner);
    REQUIRE(assigned.getParam() == "follow");
}

TEST_CASE("Playerbot packet events own independent payloads and read cursors", "[playerbot][engine][event]")
{
    ObjectGuid owner(HighGuid::Player, uint32(42));
    WorldPacket input(123);
    input << uint32(7);
    input << uint32(9);
    input.rpos(sizeof(uint32));
    Event event("packet", input, owner);
    Event copy(event);
    input.clear();
    REQUIRE(event.getPacket().size() == 2 * sizeof(uint32));
    REQUIRE(event.getPacket().GetOpcode() == 123);
    REQUIRE(event.getPacket().rpos() == sizeof(uint32));
    REQUIRE(event.getPacket().read<uint32>() == 9);
    REQUIRE(copy.getPacket().rpos() == sizeof(uint32));
    REQUIRE(copy.getPacket().read<uint32>() == 9);
    event.getPacket().clear();
    REQUIRE(copy.getPacket().size() == 2 * sizeof(uint32));
    REQUIRE(copy.getOwnerGuid() == owner);
}

TEST_CASE("Playerbot GUID events use native Cata uncompressed representation without consuming payload", "[playerbot][engine][event]")
{
    ObjectGuid target(HighGuid::Unit, uint32(123), uint32(456));
    ObjectGuid owner(HighGuid::Player, uint32(42));
    Event event("target", target, owner);
    REQUIRE(event.getPacket().size() == sizeof(uint64));
    REQUIRE(event.getPacket().read<uint64>() == target.GetRawValue());
    auto const cursor = event.getPacket().rpos();
    REQUIRE(event.getObject() == target);
    REQUIRE(event.getObject() == target);
    REQUIRE(event.getPacket().rpos() == cursor);
    REQUIRE(event.getOwnerGuid() == owner);
    Event copy(event);
    event.getPacket().clear();
    REQUIRE(copy.getObject() == target);
    REQUIRE(event.getObject().IsEmpty());
}

TEST_CASE("Playerbot short GUID event payloads are rejected safely", "[playerbot][engine][event]")
{
    WorldPacket shortPacket;
    shortPacket << uint32(17);
    Event event("target", shortPacket, ObjectGuid());
    REQUIRE(event.getObject().IsEmpty());
    REQUIRE(event.getPacket().rpos() == 0);
    REQUIRE(event.getPacket().size() == sizeof(uint32));
}

TEST_CASE("Playerbot NextAction merge preserves ordering duplicates and relevance", "[playerbot][engine]")
{
    std::vector<NextAction> prerequisites{{"reach", 30.0f}, {"cast", 20.0f}};
    std::vector<NextAction> alternatives{{"cast", 10.0f}, {"melee", 5.0f}};
    auto merged = NextAction::merge(prerequisites, alternatives);
    REQUIRE(merged.size() == 4);
    REQUIRE(merged[0].getName() == "reach");
    REQUIRE(merged[1].getName() == "cast");
    REQUIRE(merged[1].getRelevance() == 20.0f);
    REQUIRE(merged[2].getName() == "cast");
    REQUIRE(merged[2].getRelevance() == 10.0f);
    REQUIRE(merged[3].getName() == "melee");
    REQUIRE(prerequisites.size() == 2);
    REQUIRE(alternatives.size() == 2);
    REQUIRE(NextAction::merge({}, {}).empty());
    REQUIRE(NextAction("default").getRelevance() == 0.0f);
}
