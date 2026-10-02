/* Released under GNU GPL v2 or any later version. See AUTHORS.md. */
#include "../src/Ai/Class/Priest/PlayerbotPriestDpsStrategy.h"
#include <catch2/catch.hpp>
#include <limits>
#include <memory>

TEST_CASE("Playerbot healer DPS keeps donor single target action order", "[playerbot][priest-dps]")
{
    PlayerbotPriestDps::HealerDpsStrategy strategy(nullptr);
    REQUIRE(strategy.getName() == "healer dps");
    std::vector<TriggerNode*> nodes;
    strategy.InitTriggers(nodes);
    REQUIRE(nodes.size() == 1);
    std::unique_ptr<TriggerNode> trigger(nodes[0]);
    REQUIRE(trigger->getName() == "healer should attack");
    auto handlers = trigger->getHandlers();
    REQUIRE(handlers.size() == 4);
    REQUIRE(handlers[0].getName() == "shadow word: pain");
    REQUIRE(handlers[0].getRelevance() == Approx(5.5f));
    REQUIRE(handlers[1].getName() == "holy fire");
    REQUIRE(handlers[1].getRelevance() == Approx(5.4f));
    REQUIRE(handlers[2].getName() == "smite");
    REQUIRE(handlers[2].getRelevance() == Approx(5.3f));
    REQUIRE(handlers[3].getName() == "mind blast");
    REQUIRE(handlers[3].getRelevance() == Approx(5.2f));
}
TEST_CASE("Playerbot healer damage binds donor names to Cata spell identities", "[playerbot][priest-dps]")
{
    REQUIRE(PlayerbotPriestDps::Spells[0].SpellId == 589);
    REQUIRE(PlayerbotPriestDps::Spells[1].SpellId == 14914);
    REQUIRE(PlayerbotPriestDps::Spells[2].SpellId == 585);
    REQUIRE(PlayerbotPriestDps::Spells[3].SpellId == 8092);
}
TEST_CASE("Playerbot healer damage avoids reapplying its own periodic aura", "[playerbot][priest-dps]")
{
    using PlayerbotPriestDps::NeedsDamageCast;
    REQUIRE(NeedsDamageCast(true, false)); // no own aura, including another caster's aura
    REQUIRE_FALSE(NeedsDamageCast(true, true));
    REQUIRE(NeedsDamageCast(false, false));
    REQUIRE(NeedsDamageCast(false, true)); // direct spells do not inherit DoT gating
}
TEST_CASE("Playerbot healer DPS requires known spell controlled target and spare mana", "[playerbot][priest-dps]")
{
    using PlayerbotPriestDps::CanAttack;
    REQUIRE(CanAttack(true, true, true, false, 85.0f));
    REQUIRE(CanAttack(true, true, true, false, 100.0f));
    REQUIRE_FALSE(CanAttack(false, true, true, false, 100.0f));
    REQUIRE_FALSE(CanAttack(true, false, true, false, 100.0f));
    REQUIRE_FALSE(CanAttack(true, true, false, false, 100.0f));
    REQUIRE_FALSE(CanAttack(true, true, true, false, 84.99f));
    REQUIRE_FALSE(CanAttack(true, true, true, false, std::numeric_limits<float>::quiet_NaN()));
}
TEST_CASE("Playerbot healer damage eligibility yields to healing and target loss", "[playerbot][priest-dps]")
{
    REQUIRE(PlayerbotPriestDps::CanAttack(true, true, true, false, 100.0f));
    REQUIRE_FALSE(PlayerbotPriestDps::CanAttack(true, true, true, true, 100.0f));
    REQUIRE_FALSE(PlayerbotPriestDps::CanAttack(true, true, false, false, 100.0f));
}
