/* Released under GNU GPL v2 or any later version. See AUTHORS.md. */
#include "../src/Ai/Class/Mage/PlayerbotMageCureStrategy.h"
#include "../src/Ai/Base/PlayerbotPartySupport.h"
#include <catch2/catch.hpp>
#include <memory>

TEST_CASE("Playerbot qualified dispel requests expose only implemented native cure routes", "[playerbot][cure]")
{
    using PlayerbotPartySupport::ResolveDispelRequest;
    auto curse = ResolveDispelRequest(CLASS_MAGE, std::to_string(DISPEL_CURSE));
    REQUIRE(curse.Spell == 475);
    REQUIRE(curse.Type == DISPEL_CURSE);
    auto disease = ResolveDispelRequest(CLASS_PRIEST, std::to_string(DISPEL_DISEASE));
    REQUIRE(disease.Spell == 528);
    REQUIRE(disease.Type == DISPEL_DISEASE);
    REQUIRE(ResolveDispelRequest(CLASS_MAGE, std::to_string(DISPEL_DISEASE)).Spell == 0);
    REQUIRE(ResolveDispelRequest(CLASS_PRIEST, std::to_string(DISPEL_MAGIC)).Spell == 0);
    REQUIRE(ResolveDispelRequest(CLASS_DRUID, std::to_string(DISPEL_CURSE)).Spell == 0);
}

TEST_CASE("Playerbot qualified dispel requests reject malformed and unsupported types", "[playerbot][cure]")
{
    for (auto qualifier : {"", "curse", "2x", "-2", "+2", " 2", "2 ", "4294967296", "22222222222", "0", "4"})
        REQUIRE(PlayerbotPartySupport::ResolveDispelRequest(CLASS_MAGE, qualifier).Spell == 0);
}

TEST_CASE("Playerbot Mage cure preserves donor names priorities and both engine states", "[playerbot][cure]")
{
    PlayerbotMageCure::CureStrategy strategy(nullptr);
    REQUIRE(strategy.getName() == "cure");
    REQUIRE(strategy.GetType() == (STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_NONCOMBAT));
    std::vector<TriggerNode*> nodes;
    strategy.InitTriggers(nodes);
    REQUIRE(nodes.size() == 2);
    std::unique_ptr<TriggerNode> self(nodes[0]), party(nodes[1]);
    REQUIRE(self->getName() == "remove curse");
    REQUIRE(self->getHandlers()[0].getName() == "remove curse");
    REQUIRE(self->getHandlers()[0].getRelevance() == 41.0f);
    REQUIRE(party->getName() == "remove curse on party");
    REQUIRE(party->getHandlers()[0].getName() == "remove curse on party");
    REQUIRE(party->getHandlers()[0].getRelevance() == 40.0f);
}
TEST_CASE("Playerbot shared cure gate rejects disabled dead unlearned or native ineligible", "[playerbot][cure]")
{
    REQUIRE(PlayerbotPartySupport::CanCure(true, true, true, true));
    REQUIRE_FALSE(PlayerbotPartySupport::CanCure(false, true, true, true));
    REQUIRE_FALSE(PlayerbotPartySupport::CanCure(true, false, true, true));
    REQUIRE_FALSE(PlayerbotPartySupport::CanCure(true, true, false, true));
    REQUIRE_FALSE(PlayerbotPartySupport::CanCure(true, true, true, false));
}
TEST_CASE("Playerbot shared support retains healthy candidates and falls back on native rejection", "[playerbot][cure]")
{
    struct Member { int Id; float Health; };
    std::vector<Member> members = {{1, 100}, {2, 90}, {3, 100}};
    std::vector<int> attempted;
    REQUIRE(PlayerbotPartySupport::TryCandidates(members,
        [&](Member const& member) { attempted.push_back(member.Id); return member.Id == 3; }));
    REQUIRE(attempted == std::vector<int>{1, 2, 3});
}

TEST_CASE("Playerbot support skips ineligible roles and continues after cast rejection", "[playerbot][cure]")
{
    using PlayerbotPartySupport::Role;
    struct Member { int Id; Role Type; bool Eligible; };
    std::vector<Member> members = {{1, Role::Other, true}, {2, Role::Controller, false},
        {3, Role::Tank, true}, {4, Role::Healer, true}};
    PlayerbotPartySupport::OrderCandidates(members, [](Member const& member) { return member.Type; },
        [](Member const&) { return true; });
    std::vector<int> attempted;
    REQUIRE(PlayerbotPartySupport::TryCandidates(members, [&](Member const& member)
    {
        if (!member.Eligible) return false;
        attempted.push_back(member.Id);
        return member.Id == 3;
    }));
    REQUIRE(attempted == std::vector<int>{4, 3});
}
