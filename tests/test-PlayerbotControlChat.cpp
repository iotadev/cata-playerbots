/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/Cmd/PlayerbotControlChat.h"
#include "../src/Bot/Cmd/PlayerbotStrategyControl.h"
#include <catch2/catch.hpp>
TEST_CASE("Playerbot gear inspection recognizes only fixed read commands", "[playerbots][chat]")
{
    REQUIRE(IsPlayerbotGearInspection(NormalizePlayerbotControlChat(" GEAR? ")));
    REQUIRE(IsPlayerbotGearInspection("gear"));
    REQUIRE(IsPlayerbotGearInspection("gear ?"));
    for (auto command : {"gear equip", "gear 100", "gear; attack", "equipment", "do equip", "gear ??"})
        REQUIRE_FALSE(IsPlayerbotGearInspection(command));
    REQUIRE(IsPlayerbotGearApply("gear apply"));
    REQUIRE_FALSE(IsPlayerbotGearInspection("gear apply"));
    REQUIRE_FALSE(IsPlayerbotGearApply("gear apply all"));
    REQUIRE_FALSE(IsPlayerbotGearApply("gear apply; attack"));
}
TEST_CASE("Playerbot chat range extraction keeps fixed controls and command boundaries separate", "[playerbots][chat]")
{
    std::string param;
    REQUIRE(ExtractPlayerbotRangeChat(NormalizePlayerbotControlChat(" RANGE spell 23 "), param));
    REQUIRE(param == "spell 23");
    REQUIRE(ExtractPlayerbotRangeChat("range\t?", param));
    REQUIRE(param == "?");
    REQUIRE(ExtractPlayerbotRangeChat("range", param));
    REQUIRE(param.empty());
    REQUIRE_FALSE(ExtractPlayerbotRangeChat("ranged spell 20", param));
    REQUIRE_FALSE(ExtractPlayerbotRangeChat("range; stop", param));
    REQUIRE_FALSE(ExtractPlayerbotRangeChat("follow", param));
}

TEST_CASE("MultiBot basic chat controls share the existing exact command vocabulary", "[playerbots][chat]")
{
    PlayerbotControlCommand action;
    REQUIRE(ParsePlayerbotControlChat(NormalizePlayerbotControlChat("  FOLLOW\t"), action));
    REQUIRE(action == PlayerbotControlCommand::Follow);
    REQUIRE(ParsePlayerbotControlChat("stay", action));
    REQUIRE(action == PlayerbotControlCommand::Stay);
    REQUIRE(ParsePlayerbotControlChat("hold", action));
    REQUIRE(action == PlayerbotControlCommand::Hold);
    REQUIRE(ParsePlayerbotControlChat("attack", action));
    REQUIRE(action == PlayerbotControlCommand::Attack);
    REQUIRE(ParsePlayerbotControlChat("do attack my target", action));
    REQUIRE(action == PlayerbotControlCommand::Attack);
    REQUIRE(ParsePlayerbotControlChat("stop", action));
    REQUIRE(action == PlayerbotControlCommand::Cease);
    REQUIRE(ParsePlayerbotControlChat("buff", action));
    REQUIRE(action == PlayerbotControlCommand::Rebuff);
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

TEST_CASE("Playerbot strategy transport recognizes only exact donor state prefixes", "[playerbots][chat][strategy-control]")
{
    using namespace PlayerbotStrategyControl;
    REQUIRE(Parse("co")->State == Scope::Combat);
    REQUIRE(Parse("co")->Param == "?");
    REQUIRE(Parse("de ?")->State == Scope::Dead);
    auto command = Parse(NormalizePlayerbotControlChat(" NC +FOOD, -LOOT, ? "));
    REQUIRE(command);
    REQUIRE(command->Mutation);
    REQUIRE(Parse("co +focus")->Mutation);
    REQUIRE_FALSE(Parse("nc +focus"));
    REQUIRE(command->State == Scope::NonCombat);
    REQUIRE_FALSE(Recognizes("ncombat ?"));
    REQUIRE_FALSE(Recognizes("co; stop"));
    REQUIRE_FALSE(Parse("nc "));
}

TEST_CASE("Playerbot strategy transport protects roles stay and unsupported features", "[playerbots][chat][strategy-control]")
{
    using namespace PlayerbotStrategyControl;
    for (std::string const text : {"co -tank", "co +food", "de +loot", "nc +stay", "nc -heal", "nc +gather",
        "nc -food,+unknown", "nc !", "nc -food,", "nc -food,,?", "nc +loot::x", "nc ?\n"})
        REQUIRE_FALSE(Parse(text));
    REQUIRE(Parse("nc ~food,+loot,?")->Mutation);
    REQUIRE_FALSE(Parse("nc ?, ?")->Mutation);
    REQUIRE_FALSE(Parse("nc " + std::string(251, ' ')));
    std::string queries = "nc ?";
    for (unsigned i = 1; i < 16; ++i) queries += ",?";
    REQUIRE(Parse(queries));
    REQUIRE_FALSE(Parse(queries + ",?"));
}

TEST_CASE("Playerbot strategy mailbox copies expires consumes and cancels requests", "[playerbots][chat][strategy-control]")
{
    PlayerbotStrategyControl::Mailbox mailbox;
    REQUIRE_FALSE(mailbox.Post(0, "nc ?", 0));
    REQUIRE_FALSE(mailbox.Post(1, "nc +stay", 0));
    std::string command = "nc -food,?";
    REQUIRE(mailbox.Post(1, command, 100));
    command = "co ?";
    REQUIRE_FALSE(mailbox.Post(2, command, 101));
    auto request = mailbox.Take(5099);
    REQUIRE(request);
    REQUIRE(request->Requester == 1);
    REQUIRE(request->Value.Param == "-food,?");
    REQUIRE_FALSE(mailbox.Take(5099));
    REQUIRE(mailbox.Post(2, command, 100));
    REQUIRE_FALSE(mailbox.Take(5100));
    REQUIRE(mailbox.Post(2, command, 100));
    mailbox.Cancel();
    REQUIRE_FALSE(mailbox.Take(101));
}

TEST_CASE("Playerbot strategy mailbox expiry survives native timer wrap", "[playerbots][chat][strategy-control]")
{
    PlayerbotStrategyControl::Mailbox mailbox;
    REQUIRE(mailbox.Post(1, "de ?", UINT32_MAX - 100));
    REQUIRE(mailbox.Take(100));
    REQUIRE(mailbox.Post(1, "de ?", UINT32_MAX - 100));
    REQUIRE_FALSE(mailbox.Take(5000));
}

TEST_CASE("Playerbot strategy mailbox copies addon correlation metadata without changing ordinary requests", "[playerbots][chat][strategy-control]")
{
    PlayerbotStrategyControl::Mailbox mailbox;
    std::string token = "request-1", target = "Testone";
    REQUIRE(mailbox.Post(1, "nc -food,?", 100, token, target));
    token = "changed"; target = "changed";
    auto request = mailbox.Take(101);
    REQUIRE(request->Token == "request-1");
    REQUIRE(request->Target == "Testone");
    REQUIRE(mailbox.Post(1, "nc ?", 102));
    REQUIRE(mailbox.Take(103)->Token.empty());
}

TEST_CASE("Playerbot strategy addon mailbox rejects unpaired metadata and query only mutations", "[playerbots][chat][strategy-control]")
{
    PlayerbotStrategyControl::Mailbox mailbox;
    REQUIRE_FALSE(mailbox.Post(1, "nc -food", 100, "t", ""));
    REQUIRE_FALSE(mailbox.Post(1, "nc -food", 100, "", "Testone"));
    REQUIRE_FALSE(mailbox.Post(1, "nc -food", 100, "bad~token", "Testone"));
    REQUIRE_FALSE(mailbox.Post(1, "nc ?", 100, "t", "Testone"));
    REQUIRE_FALSE(mailbox.Post(1, "nc -food", 100, "t", std::string(65, 'x')));
    REQUIRE(mailbox.Post(1, "nc -food", 100, "t", "Testone"));
    mailbox.Cancel();
    REQUIRE_FALSE(mailbox.Take(101));
}
TEST_CASE("Playerbot combat utility strategy controls retain state boundaries and copied correlation", "[playerbots][chat][strategy-control]")
{
    using namespace PlayerbotStrategyControl;
    auto command = Parse("co -threat,~potions,?");
    REQUIRE(command);
    REQUIRE(command->State == Scope::Combat);
    REQUIRE(command->Mutation);
    for (std::string const text : {"co +food", "nc +potions", "de +threat", "co -tank", "co ~frost",
        "co -cure", "co +stay", "co -threat,+heal", "co +threat::x"}) REQUIRE_FALSE(Parse(text));
    REQUIRE(MutableStrategies(Scope::Dead).empty());
    Mailbox mailbox;
    REQUIRE(mailbox.Post(1, "co -potions", 100, "combat-1", "Testone"));
    auto request = mailbox.Take(101);
    REQUIRE(request);
    REQUIRE(request->Value.State == Scope::Combat);
    REQUIRE(request->Token == "combat-1");
    REQUIRE(request->Target == "Testone");
}
TEST_CASE("Playerbot combat utility default restore preserves unrelated role and spec strategies", "[playerbots][chat][strategy-control]")
{
    struct EngineStub
    {
        std::set<std::string> Strategies{"tank", "cure", "threat", "focus"};
        bool HasStrategy(std::string const& name) { return Strategies.count(name) != 0; }
        void AddStrategy(std::string const& name) { Strategies.insert(name); }
        void RemoveStrategy(std::string const& name) { Strategies.erase(name); }
    } engine;
    PlayerbotStrategyControl::RestoreCombatDefaults(engine, false);
    REQUIRE(engine.Strategies == std::set<std::string>{"tank", "cure", "potions"});
    PlayerbotStrategyControl::RestoreCombatDefaults(engine, true);
    REQUIRE(engine.Strategies == std::set<std::string>{"tank", "cure", "potions", "threat"});
    engine.Strategies.erase("potions");
    engine.Strategies.erase("threat");
    PlayerbotStrategyControl::RestoreCombatDefaults(engine, true);
    REQUIRE(engine.Strategies == std::set<std::string>{"tank", "cure", "potions", "threat"});
}
