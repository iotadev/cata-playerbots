/* Released under GNU GPL v2 or any later version. See AUTHORS.md. */
#include "../src/Ai/Class/Mage/PlayerbotMageCureStrategy.h"
#include "../src/Ai/Base/PlayerbotPartySupport.h"
#include <catch2/catch.hpp>
#include <memory>

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
    REQUIRE(PlayerbotPartySupport::TryInPriorityOrder(members, [](Member const& member) { return member.Health; },
        [&](Member const& member) { attempted.push_back(member.Id); return member.Id == 3; }));
    REQUIRE(attempted == std::vector<int>{2, 1, 3});
}
