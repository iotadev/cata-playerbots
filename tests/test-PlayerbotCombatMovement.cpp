/* Released under GNU GPL v2 or any later version. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotCombatMovement.h"
#include <catch2/catch.hpp>
#include <memory>
#include <limits>
using namespace PlayerbotCombatMovement;
TEST_CASE("Playerbot range mailbox copies one bounded request and consumes it once", "[playerbot][movement]")
{
    RangeMailbox mailbox;
    std::string param = "spell 23";
    REQUIRE(mailbox.Post(7, param, 100));
    param = "heal 30";
    REQUIRE_FALSE(mailbox.Post(8, param, 101));
    auto request = mailbox.Take(102);
    REQUIRE(request);
    REQUIRE(request->Requester == 7);
    REQUIRE(request->Param == "spell 23");
    REQUIRE_FALSE(mailbox.Take(103));
    REQUIRE(mailbox.Post(8, param, 104));
    REQUIRE(mailbox.Take(104)->Requester == 8);
}
TEST_CASE("Playerbot range mailbox rejects invalid and expired requests including clock wrap", "[playerbot][movement]")
{
    RangeMailbox mailbox;
    REQUIRE_FALSE(mailbox.Post(0, "?", 0));
    REQUIRE_FALSE(mailbox.Post(7, "unknown 20", 0));
    REQUIRE_FALSE(mailbox.Post(7, std::string(65, ' '), 0));
    REQUIRE(mailbox.Post(7, "?", 10));
    REQUIRE_FALSE(mailbox.Take(5010));
    REQUIRE(mailbox.Post(7, "?", std::numeric_limits<uint32>::max() - 5));
    REQUIRE(mailbox.Take(5));
}
TEST_CASE("Playerbot range action accepts donor query set and reset vocabulary", "[playerbot][movement]")
{
    REQUIRE(ParseRangeCommand("?").Operation == RangeOperation::QueryAll);
    REQUIRE(ParseRangeCommand(" spell ? ").Operation == RangeOperation::Query);
    REQUIRE(ParseRangeCommand("heal\t?").Type == "heal");
    auto set = ParseRangeCommand("spell 23.5");
    REQUIRE(set.Operation == RangeOperation::Set);
    REQUIRE(set.Type == "spell");
    REQUIRE(set.Value == 23.5f);
    REQUIRE(ParseRangeCommand("heal 30").Value == 30.0f);
    REQUIRE(ParseRangeCommand("spell 0").Operation == RangeOperation::Set);
    REQUIRE(ParseRangeCommand("spell 0").Value == 0.0f);
}
TEST_CASE("Playerbot range action rejects malformed values unsupported qualifiers and oversized input", "[playerbot][movement]")
{
    for (std::string const& param : {"", "spell", "heal ", "shoot 20", "flee ?", "spell nonsense",
        "spell 20 trailing", "spell 20;stop", "spell nan", "heal inf", "spell -2", "spell 1",
        "spell 25.1", "heal 30.1", "spell 1e99", "spell 0.05", "spell ? ?"})
        REQUIRE(ParseRangeCommand(param).Operation == RangeOperation::Invalid);
    REQUIRE(ParseRangeCommand(std::string(65, ' ')).Operation == RangeOperation::Invalid);
}
TEST_CASE("Playerbot range responses distinguish defaults from bounded configured distances", "[playerbot][movement]")
{
    REQUIRE(FormatRange("spell", 0) == "spell range: 20 (default)");
    REQUIRE(FormatRange("heal", 0) == "heal range: 30 (default)");
    REQUIRE(FormatRange("spell", 23.5f) == "spell range: 23.5");
    REQUIRE(FormatRange("spell", 100) == "spell range: 25");
    REQUIRE(FormatRange("heal", std::numeric_limits<float>::quiet_NaN()) == "heal range: 30 (default)");
    REQUIRE(FormatRange("unknown", 20).empty());
}
TEST_CASE("Playerbot shared movement permission rejects native control restrictions", "[playerbot][movement]")
{
    ControlState state;
    REQUIRE(CanMove(state));
    for (bool ControlState::* field : { &ControlState::Teleporting, &ControlState::Flight,
        &ControlState::Vehicle, &ControlState::Restricted, &ControlState::Charmed,
        &ControlState::Frozen, &ControlState::Polymorphed, &ControlState::ControlledMotion })
    {
        state.*field = true;
        REQUIRE_FALSE(CanMove(state));
        state.*field = false;
        REQUIRE(CanMove(state));
    }
    state.InWorld = false;
    REQUIRE_FALSE(CanMove(state));
    state.InWorld = true; state.Alive = false;
    REQUIRE_FALSE(CanMove(state));
}
TEST_CASE("Playerbot qualified range values remain independent and reset to donor zero", "[playerbot][movement]")
{
    RangeValue spell(nullptr), heal(nullptr);
    spell.Qualify("spell"); heal.Qualify("heal");
    REQUIRE(spell.Get() == 0.0f);
    spell.Set(24.0f);
    REQUIRE(spell.Get() == 24.0f);
    REQUIRE(heal.Get() == 0.0f);
    spell.Reset();
    REQUIRE(spell.Get() == 0.0f);
}
TEST_CASE("Playerbot range resolution preserves defaults and companion boundaries", "[playerbot][movement]")
{
    REQUIRE(ResolveRange("spell", 0) == 20);
    REQUIRE(ResolveRange("heal", 0) == 30);
    REQUIRE(ResolveRange("spell", 23) == 23);
    REQUIRE(ResolveRange("spell", 40) == 25);
    REQUIRE(ResolveRange("spell", 0.5f) == 2);
    REQUIRE(ResolveRange("spell", -10) == 20);
    REQUIRE(ResolveRange("spell", std::numeric_limits<float>::quiet_NaN()) == 20);
    REQUIRE(ResolveRange("spell", std::numeric_limits<float>::infinity()) == 20);
    REQUIRE(ResolveRange("heal", 100) == 30);
    REQUIRE(ResolveRange("unknown", 25) == 0);
}
TEST_CASE("Playerbot caster movement uses donor spell reach without melee behind triggers", "[playerbot][movement]")
{
    std::vector<TriggerNode*> triggers;
    AddTriggers(triggers, true);
    REQUIRE(triggers.size() == 2);
    std::unique_ptr<TriggerNode> facing(triggers[0]), reach(triggers[1]);
    REQUIRE(facing->getHandlers()[0].getName() == "set facing");
    REQUIRE(reach->getName() == "enemy out of spell");
    REQUIRE(reach->getHandlers()[0].getName() == "reach spell");
    REQUIRE(reach->getHandlers()[0].getRelevance() == ACTION_HIGH);
}
TEST_CASE("Playerbot caster reach and facing preserve active chase and range gates", "[playerbot][movement]")
{
    PositionState state{false, false, false, false, false, false, false, false};
    REQUIRE(Useful(Step::ReachSpell, state));
    REQUIRE_FALSE(Useful(Step::Facing, state));
    state.Chasing = true;
    REQUIRE_FALSE(Useful(Step::ReachSpell, state));
    state.InSpell = true;
    REQUIRE_FALSE(Useful(Step::ReachSpell, state));
    REQUIRE(Useful(Step::Facing, state));
    state.Moving = true;
    REQUIRE_FALSE(Useful(Step::Facing, state));
    state.Moving = false; state.Facing = true;
    REQUIRE_FALSE(Useful(Step::Facing, state));
}
TEST_CASE("Playerbot movement strategy retains donor trigger action names and priorities", "[playerbot][movement]")
{
    std::vector<TriggerNode*> triggers;
    AddTriggers(triggers);
    REQUIRE(triggers.size() == 3);
    std::unique_ptr<TriggerNode> facing(triggers[0]), reach(triggers[1]), behind(triggers[2]);
    REQUIRE(facing->getName() == "not facing target");
    REQUIRE(facing->getHandlers()[0].getName() == "set facing");
    REQUIRE(facing->getHandlers()[0].getRelevance() == ACTION_MOVE + 7);
    REQUIRE(reach->getName() == "enemy out of melee");
    REQUIRE(reach->getHandlers()[0].getName() == "reach melee");
    REQUIRE(reach->getHandlers()[0].getRelevance() == ACTION_HIGH + 1);
    REQUIRE(behind->getName() == "not behind target");
    REQUIRE(behind->getHandlers()[0].getName() == "set behind");
}
TEST_CASE("Playerbot facing waits for stationary melee without redundant turns", "[playerbot][movement]")
{
    PositionState state{true, false, false, false, false, false, true};
    REQUIRE(Useful(Step::Facing, state));
    state.Moving = true;
    REQUIRE_FALSE(Useful(Step::Facing, state));
    state.Moving = false; state.Facing = true;
    REQUIRE_FALSE(Useful(Step::Facing, state));
    state.Facing = false; state.InMelee = false;
    REQUIRE_FALSE(Useful(Step::Facing, state));
}
TEST_CASE("Playerbot reach preserves active native chase and behind positioning excludes tanks", "[playerbot][movement]")
{
    PositionState state{false, true, false, false, false, false, false};
    REQUIRE(Useful(Step::Reach, state));
    state.Chasing = true;
    REQUIRE_FALSE(Useful(Step::Reach, state));
    state.InMelee = true;
    REQUIRE_FALSE(Useful(Step::Reach, state));
    REQUIRE(Useful(Step::Behind, state));
    state.Tanking = true;
    REQUIRE_FALSE(Useful(Step::Behind, state));
    state.Tanking = false; state.TargetMoving = true;
    REQUIRE_FALSE(Useful(Step::Behind, state));
    state.TargetMoving = false; state.Moving = true;
    REQUIRE_FALSE(Useful(Step::Behind, state));
    state.Moving = false; state.Behind = true;
    REQUIRE_FALSE(Useful(Step::Behind, state));
}
