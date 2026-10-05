/* Released under GNU GPL v2 or any later version. See AUTHORS.md. */
#include "../src/Ai/Class/Priest/PlayerbotPriestCureStrategy.h"
#include "../src/Ai/Class/Priest/PlayerbotPriestStrategy.h"
#include <catch2/catch.hpp>
#include <memory>

TEST_CASE("Playerbot Priest cure keeps donor disease trigger priorities and Cata fallback names", "[playerbot][cure]")
{
    PlayerbotPriestCure::CureStrategy strategy(nullptr);
    REQUIRE(strategy.getName() == "cure");
    std::vector<TriggerNode*> nodes;
    strategy.InitTriggers(nodes);
    REQUIRE(nodes.size() == 2);
    std::unique_ptr<TriggerNode> self(nodes[0]);
    std::unique_ptr<TriggerNode> party(nodes[1]);
    REQUIRE(self->getName() == "cure disease");
    REQUIRE(self->getHandlers()[0].getName() == "cure disease");
    REQUIRE(self->getHandlers()[0].getRelevance() == 31.0f);
    REQUIRE(party->getName() == "party member cure disease");
    REQUIRE(party->getHandlers()[0].getName() == "cure disease on party");
    REQUIRE(party->getHandlers()[0].getRelevance() == 30.0f);
}
TEST_CASE("Playerbot Priest cure rejects disabled dead or unlearned spell routes", "[playerbot][cure]")
{
    REQUIRE(PlayerbotPriestCure::CanCure(true, true, true, true));
    REQUIRE_FALSE(PlayerbotPriestCure::CanCure(false, true, true, true));
    REQUIRE_FALSE(PlayerbotPriestCure::CanCure(true, false, true, true));
    REQUIRE_FALSE(PlayerbotPriestCure::CanCure(true, true, false, true));
}
TEST_CASE("Playerbot Priest cure requires native dispellable disease eligibility", "[playerbot][cure]")
{
    REQUIRE_FALSE(PlayerbotPriestCure::CanCure(true, true, true, false));
    REQUIRE(PlayerbotPriestCure::CanCure(true, true, true, true));
}
TEST_CASE("Playerbot party cure retains full health and healing cutoff candidates", "[playerbot][cure]")
{
    std::vector<float> members = {100.0f, 90.0f};
    std::vector<float> attempted;
    REQUIRE(PlayerbotPartySupport::TryCandidates(members,
        [&](float hp) { attempted.push_back(hp); return hp == 90.0f; }));
    REQUIRE(attempted == std::vector<float>{100.0f, 90.0f});
}
TEST_CASE("Playerbot party cure candidate ordering retains stable ties and cast fallback", "[playerbot][cure]")
{
    struct Member { int Id; float Health; };
    std::vector<Member> members = {{1, 100.0f}, {2, 50.0f}, {3, 100.0f}};
    std::vector<int> attempted;
    REQUIRE(PlayerbotPartySupport::TryCandidates(members,
        [&](Member const& member) { attempted.push_back(member.Id); return member.Id == 3; }));
    REQUIRE(attempted == std::vector<int>{1, 2, 3});
}
TEST_CASE("Playerbot empty cure candidate list performs no attempts", "[playerbot][cure]")
{
    std::vector<float> members;
    unsigned attempts = 0;
    REQUIRE_FALSE(PlayerbotPartySupport::TryCandidates(members,
        [&](float) { ++attempts; return true; }));
    REQUIRE(attempts == 0);
}
