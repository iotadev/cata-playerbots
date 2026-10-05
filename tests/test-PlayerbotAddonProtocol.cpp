/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/Cmd/PlayerbotAddonProtocol.h"
#include "../src/Bot/Cmd/PlayerbotAddonState.h"
#include "../src/Bot/Cmd/PlayerbotAddonMutation.h"
#include "../src/Bot/Cmd/PlayerbotAddonLifecycle.h"
#include <catch2/catch.hpp>

TEST_CASE("MultiBot advertises only implemented managed capabilities when enabled", "[playerbots][addon]")
{
    REQUIRE(PlayerbotAddonProtocol::ManagedCapabilities(false).empty());
    REQUIRE(PlayerbotAddonProtocol::ManagedCapabilities(true) == "ALT_ROSTER_V1,BOT_LIFECYCLE_V1");
    REQUIRE(PlayerbotAddonProtocol::ManagedCapabilities(false, true) == "STATE_FRAMING_V1");
    REQUIRE(PlayerbotAddonProtocol::ManagedCapabilities(true, true) == "ALT_ROSTER_V1,BOT_LIFECYCLE_V1,STATE_FRAMING_V1");
    REQUIRE(PlayerbotAddonProtocol::ManagedCapabilities(false, false, true).empty());
    REQUIRE(PlayerbotAddonProtocol::ManagedCapabilities(false, true, true) == "STATE_FRAMING_V1,STRATEGY_MUTATION_V1");
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

TEST_CASE("MultiBot strategy state requests require strict encoded names and transaction tokens", "[playerbots][addon][state]")
{
    using namespace PlayerbotAddonProtocol;
    REQUIRE(Parse("GET~STATE~Testone~t").Kind == Request::State);
    REQUIRE(Parse("GET~STATE~T%65stone~t").BotName == "Testone");
    REQUIRE(Parse("GET~STATES~state:1").Kind == Request::States);
    for (std::string const text : {"GET~STATE~Testone", "GET~STATE~~t", "GET~STATE~Testone~", "GET~STATE~Testone~t~x",
        "GET~STATE~Test%~t", "GET~STATE~Test%00~t", "GET~STATE~Test%7E~t", "GET~STATE~Test+one~t", "GET~STATES", "GET~STATES~t~x"})
        REQUIRE(Parse(text).Kind == Request::Invalid);
}

TEST_CASE("MultiBot single strategy snapshot uses donor counts scopes indices and escaping", "[playerbots][addon][state]")
{
    using namespace PlayerbotAddonProtocol;
    auto frames = FrameStrategyStates("t", {{"Testone", {"tank", "healer dps"}, {"food"}}}, false);
    REQUIRE(frames == std::vector<std::string>{"STATE_BEGIN~t~Testone~2~1", "STATE_ITEM~t~Testone~C~1~tank",
        "STATE_ITEM~t~Testone~C~2~healer%20dps", "STATE_ITEM~t~Testone~N~1~food", "STATE_END~t~Testone~2~1"});
}

TEST_CASE("MultiBot global strategy frames bracket all bots including an empty roster", "[playerbots][addon][state]")
{
    using namespace PlayerbotAddonProtocol;
    REQUIRE(FrameStrategyStates("t", {}, true) == std::vector<std::string>{"STATES_BEGIN~t~0", "STATES_END~t~0"});
    auto frames = FrameStrategyStates("t", {{"Testone", {}, {}}, {"Testtwo", {}, {"loot"}}}, true);
    REQUIRE(frames.front() == "STATES_BEGIN~t~2");
    REQUIRE(frames.back() == "STATES_END~t~2");
    REQUIRE(frames.size() == 7);
}

TEST_CASE("MultiBot strategy response preflight aborts rather than emitting incomplete begins", "[playerbots][addon][state]")
{
    using namespace PlayerbotAddonProtocol;
    auto rejected = [](std::vector<std::string> const& frames)
    { REQUIRE(frames.size() == 1); REQUIRE(frames[0].find("STATE_ABORT~") == 0); };
    rejected(FrameStrategyStates("t", {}, false));
    rejected(FrameStrategyStates("t", {{"Testone", {std::string(193, 'x')}, {}}}, false));
    rejected(FrameStrategyStates(std::string(64, 't'), {{"Testone", {std::string(191, ' ')}, {}}}, false));
    rejected(FrameStrategyStates("t", {{"Testone", {"tank", "tank"}, {}}}, false));
    rejected(FrameStrategyStates("t", {{"Testone", {}, {}}, {"Testone", {}, {}}}, true));
    std::vector<StateRow> rows;
    for (unsigned i = 0; i < 129; ++i) rows.push_back({"Bot" + std::to_string(i), {}, {}});
    rejected(FrameStrategyStates("t", rows, true));
}

TEST_CASE("Playerbot strategy snapshots reject wrong identity stale data and survive timer wrap", "[playerbots][addon][state]")
{
    using namespace PlayerbotAddonProtocol;
    PlayerbotStrategySnapshot snapshot;
    snapshot.Bot = 1; snapshot.Controller = 2; snapshot.Created = 100;
    REQUIRE(SnapshotFresh(snapshot, 1, 2, 5099));
    REQUIRE_FALSE(SnapshotFresh(snapshot, 1, 2, 5100));
    REQUIRE_FALSE(SnapshotFresh(snapshot, 3, 2, 101));
    REQUIRE_FALSE(SnapshotFresh(snapshot, 1, 3, 101));
    snapshot.Controller = 0;
    REQUIRE(SnapshotFresh(snapshot, 1, 0, 101)); // GM's separately authorized unattached read
    REQUIRE_FALSE(SnapshotFresh(snapshot, 1, 2, 101));
    snapshot.Controller = 2;
    snapshot.Created = UINT32_MAX - 100;
    REQUIRE(SnapshotFresh(snapshot, 1, 2, 100));
    REQUIRE_FALSE(SnapshotFresh(snapshot, 1, 2, 5000));
}

TEST_CASE("MultiBot strategy mutation envelope decodes copied target token state and operators", "[playerbots][addon][mutation]")
{
    using namespace PlayerbotAddonProtocol;
    auto envelope = Parse("RUN~STRATEGY~BOT~Testone~t~N~%2Bfood%2C-loot%2C%3F");
    REQUIRE(envelope.Kind == Request::Strategy);
    auto request = ParseStrategyMutation(envelope.Payload);
    REQUIRE(request);
    REQUIRE(request->Target == "Testone");
    REQUIRE(request->Token == "t");
    REQUIRE(request->State == "N");
    REQUIRE(request->Changes == "+food,-loot,?");
    REQUIRE(ParseStrategyMutation("PARTY~~t~C~%2Btank")); // syntactically valid, execution policy rejects
}

TEST_CASE("MultiBot strategy mutation fields reject malformed escaping boundaries and state", "[playerbots][addon][mutation]")
{
    using namespace PlayerbotAddonProtocol;
    for (std::string const fields : {"BOT~Testone~t~N", "BOT~~t~N~%2Bfood", "ALL~Testone~t~N~%2Bfood",
        "BOT~Testone~~N~%2Bfood", "BOT~Testone~t~D~%2Bfood", "BOT~Testone~t~N~", "BOT~Testone~t~N~%",
        "BOT~Testone~t~N~%00", "BOT~Testone~t~N~%0A", "BOT~Testone~t~N~%2Bfood~x", "UNKNOWN~~t~N~%2Bfood"})
        REQUIRE_FALSE(ParseStrategyMutation(fields));
    REQUIRE_FALSE(ParseStrategyMutation("BOT~" + std::string(65, 'x') + "~t~N~%2Bfood"));
    REQUIRE_FALSE(ParseStrategyMutation("BOT~Testone~t~N~" + std::string(161, 'x')));
}

TEST_CASE("MultiBot strategy acknowledgement preserves request identity and truthful result counts", "[playerbots][addon][mutation]")
{
    using namespace PlayerbotAddonProtocol;
    StrategyMutation request{"BOT", "Testone", "t", "N", "+food"};
    REQUIRE(StrategyAck(request, 1, 1, 0, "OK") == "STRATEGY_ACK~BOT~Testone~t~N~1~1~0~OK");
    REQUIRE(StrategyAck(request, 1, 0, 1, "BUSY") == "STRATEGY_ACK~BOT~Testone~t~N~1~0~1~BUSY");
    REQUIRE(StrategyAck(request, 0, 0, 0, "NO_BOT") == "STRATEGY_ACK~BOT~Testone~t~N~0~0~0~NO_BOT");
    request.State = "C";
    request.Changes = "-potions";
    REQUIRE(StrategyAck(request, 1, 1, 0, "OK") == "STRATEGY_ACK~BOT~Testone~t~C~1~1~0~OK");
    REQUIRE(StrategyAck(request, 1, 0, 1, "BUSY") == "STRATEGY_ACK~BOT~Testone~t~C~1~0~1~BUSY");
    request.State = "N";
    REQUIRE(StrategyAck(request, 1, 1, 1, "OK").empty());
    REQUIRE(StrategyAck(request, 1, UINT32_MAX, 2, "OK").empty());
    request.Target = std::string(64, ' ');
    request.Token = std::string(64, 't');
    REQUIRE(StrategyAck(request, 1, 0, 1, "UNSUPPORTED_STRATEGY").empty());
}

TEST_CASE("MultiBot strategy mutation replay guard prevents duplicate toggles per account", "[playerbots][addon][mutation]")
{
    // The same guard implementation is shared with lifecycle admission.
    PlayerbotAddonProtocol::MutationGuard guard;
    REQUIRE(guard.Admit("toggle-1", 100) == nullptr);
    REQUIRE(std::string(guard.Admit("toggle-1", 101)) == "REPLAY");
    REQUIRE(guard.Admit("toggle-2", 102) == nullptr);
    REQUIRE(guard.Admit("toggle-1", 120100) == nullptr);
}
