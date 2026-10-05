/* Released under GNU GPL v2 or any later version. See AUTHORS.md. */
#include "../src/Ai/Base/PlayerbotInterruptStrategy.h"
#include <catch2/catch.hpp>
#include <memory>

TEST_CASE("Playerbot interrupt requires an interruptible noninstant cast", "[playerbot][interrupt]")
{
    REQUIRE(PlayerbotInterrupt::InterruptibleCast(false, true, true, true));
    REQUIRE_FALSE(PlayerbotInterrupt::InterruptibleCast(false, true, false, true));
    REQUIRE_FALSE(PlayerbotInterrupt::InterruptibleCast(false, false, true, true));
    REQUIRE_FALSE(PlayerbotInterrupt::InterruptibleCast(false, true, true, false));
}
TEST_CASE("Playerbot interrupt includes native channeling and excludes protected channels", "[playerbot][interrupt]")
{
    REQUIRE(PlayerbotInterrupt::InterruptibleCast(true, false, false, true));
    REQUIRE_FALSE(PlayerbotInterrupt::InterruptibleCast(true, false, false, false));
}
TEST_CASE("Playerbot interrupt triggers retain donor names and interrupt priority", "[playerbot][interrupt]")
{
    std::vector<TriggerNode*> nodes;
    PlayerbotInterrupt::AddTrigger(nodes, "pummel");
    PlayerbotInterrupt::AddTrigger(nodes, "counterspell");
    REQUIRE(nodes.size() == 4);
    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        std::unique_ptr<TriggerNode> node(nodes[i]);
        std::string expected = i < 2 ? "pummel" : "counterspell";
        if (i % 2) expected += " on enemy healer";
        REQUIRE(node->getName() == expected);
        REQUIRE(node->getHandlers()[0].getName() == node->getName());
        REQUIRE(node->getHandlers()[0].getRelevance() == ACTION_INTERRUPT);
    }
}
TEST_CASE("Playerbot secondary interrupt policy requires an engaged positive interruptible usable cast", "[playerbot][interrupt]")
{
    using PlayerbotInterrupt::EnemyHealerCandidate;
    REQUIRE(EnemyHealerCandidate(false, true, false, true, true));
    REQUIRE_FALSE(EnemyHealerCandidate(true, true, false, true, true));
    REQUIRE_FALSE(EnemyHealerCandidate(false, false, false, true, true));
    REQUIRE_FALSE(EnemyHealerCandidate(false, true, true, true, true));
    REQUIRE_FALSE(EnemyHealerCandidate(false, true, false, false, true));
    REQUIRE_FALSE(EnemyHealerCandidate(false, true, false, true, false));
}
