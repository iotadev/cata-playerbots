/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Ai/Base/PlayerbotPartyBuffStrategy.h"
#include <catch2/catch.hpp>
#include <memory>
#include "../src/Bot/ForceRebuff.h"

TEST_CASE("Playerbot rebuff window expires and remains wrap safe", "[PlayerbotPartyBuff]")
{
    ForceRebuffState state;
    REQUIRE_FALSE(state.IsPending(0));
    state.Begin(100);
    REQUIRE(state.IsPending(120099));
    REQUIRE_FALSE(state.IsPending(120100));
    state.Begin(0xfffffff0u);
    REQUIRE(state.IsPending(10));
    state.End();
    REQUIRE_FALSE(state.IsPending(10));
}
TEST_CASE("Playerbot rebuff duration policy avoids perpetual and already topped off auras", "[PlayerbotPartyBuff]")
{
    ForceRebuffState state;
    REQUIRE_FALSE(state.BelowRefreshTarget(100, 600000, 0, false));
    state.Begin(0);
    REQUIRE(state.BelowRefreshTarget(539999, 600000, 0, false));
    REQUIRE_FALSE(state.BelowRefreshTarget(540000, 600000, 0, false));
    REQUIRE_FALSE(state.BelowRefreshTarget(590000, 600000, 90000, false));
    REQUIRE(state.BelowRefreshTarget(400000, 600000, 90000, false));
    REQUIRE_FALSE(state.BelowRefreshTarget(-1, -1, 0, false));
    REQUIRE_FALSE(state.BelowRefreshTarget(0, 600000, 0, false));
    REQUIRE_FALSE(state.BelowRefreshTarget(1, 600000, 0, true));
    REQUIRE_FALSE(state.BelowRefreshTarget(1, 600000, 120000, false));
}
TEST_CASE("Playerbot rebuff cycles retain spell identity but clear work and cancellation", "[PlayerbotPartyBuff]")
{
    ForceRebuffState state;
    state.Begin(0);
    REQUIRE(state.HasWork());
    state.BeginCycle();
    REQUIRE_FALSE(state.HasWork());
    state.NoteWork();
    state.NoteProposed();
    state.NoteCast(1459);
    REQUIRE(state.HasWork());
    state.BeginCycle();
    REQUIRE_FALSE(state.HasWork());
    REQUIRE(state.LastSpell() == 1459);
    state.End();
    REQUIRE(state.LastSpell() == 0);
}
TEST_CASE("Playerbot rebuff mailbox copies one request and expires it once", "[PlayerbotPartyBuff]")
{
    ForceRebuffMailbox mailbox;
    REQUIRE_FALSE(mailbox.Post(0, 0));
    REQUIRE(mailbox.Post(7, 100));
    REQUIRE_FALSE(mailbox.Post(8, 100));
    auto request = mailbox.Take(200);
    REQUIRE(request.has_value());
    REQUIRE(request->Requester == 7);
    REQUIRE_FALSE(mailbox.Take(200).has_value());
    REQUIRE(mailbox.Post(7, 100));
    REQUIRE_FALSE(mailbox.Take(5100).has_value());
    REQUIRE(mailbox.Post(7, 0xfffffff0u));
    REQUIRE(mailbox.Take(10).has_value());
}

TEST_CASE("Playerbot missing aura qualifiers resolve donor names to Cata buff variants", "[PlayerbotPartyBuff]")
{
    using namespace PlayerbotPartyBuff;
    REQUIRE(ResolveAuraBuff(CLASS_MAGE, AuraQualifier(Brilliance)) == &Brilliance);
    REQUIRE(ResolveAuraBuff(CLASS_MAGE, "arcane intellect") == &Brilliance);
    REQUIRE(ResolveAuraBuff(CLASS_PRIEST, AuraQualifier(Fortitude)) == &Fortitude);
    REQUIRE(ResolveAuraBuff(CLASS_PRIEST, "power word: fortitude") == &Fortitude);
}
TEST_CASE("Playerbot missing aura qualifiers reject unsupported routes and lists", "[PlayerbotPartyBuff]")
{
    using namespace PlayerbotPartyBuff;
    REQUIRE(ResolveAuraBuff(CLASS_WARRIOR, AuraQualifier(Brilliance)) == nullptr);
    REQUIRE(ResolveAuraBuff(CLASS_PRIEST, AuraQualifier(Brilliance)) == nullptr);
    for (auto qualifier : {"", "arcane brilliance", "arcane intellect,unknown", "arcane intellect,", "Arcane Intellect"})
        REQUIRE(ResolveAuraBuff(CLASS_MAGE, qualifier) == nullptr);
}

TEST_CASE("Playerbot Mage buff strategy retains donor party trigger and priority", "[PlayerbotPartyBuff]")
{
    PlayerbotPartyBuff::Strategy strategy(nullptr, PlayerbotPartyBuff::MageAction, ACTION_HIGH);
    REQUIRE(strategy.getName() == "buff");
    REQUIRE(strategy.GetType() == STRATEGY_TYPE_NONCOMBAT);
    REQUIRE(strategy.getDefaultActions().empty());
    std::vector<TriggerNode*> nodes;
    strategy.InitTriggers(nodes);
    REQUIRE(nodes.size() == 1);
    std::unique_ptr<TriggerNode> node(nodes.front());
    REQUIRE(node->getName() == "arcane intellect on party");
    auto handlers = node->getHandlers();
    REQUIRE(handlers.size() == 1);
    REQUIRE(handlers.front().getName() == node->getName());
    REQUIRE(handlers.front().getRelevance() == ACTION_HIGH);
}

TEST_CASE("Playerbot Priest buff priority stays below healing and resurrection", "[PlayerbotPartyBuff]")
{
    PlayerbotPartyBuff::Strategy strategy(nullptr, PlayerbotPartyBuff::PriestAction, ACTION_NORMAL);
    std::vector<TriggerNode*> nodes;
    strategy.InitTriggers(nodes);
    REQUIRE(nodes.size() == 1);
    std::unique_ptr<TriggerNode> node(nodes.front());
    REQUIRE(node->getName() == "power word: fortitude on party");
    auto handlers = node->getHandlers();
    REQUIRE(handlers.front().getName() == node->getName());
    REQUIRE(handlers.front().getRelevance() < ACTION_LIGHT_HEAL + 1);
    REQUIRE(handlers.front().getRelevance() < ACTION_CRITICAL_HEAL + 10);
}

TEST_CASE("Playerbot party buff definitions use Cata single and party aura variants", "[PlayerbotPartyBuff]")
{
    REQUIRE(PlayerbotPartyBuff::Brilliance.SpellId == 1459);
    REQUIRE(PlayerbotPartyBuff::Brilliance.SingleAuraId == 79057);
    REQUIRE(PlayerbotPartyBuff::Brilliance.PartyAuraId == 79058);
    REQUIRE(PlayerbotPartyBuff::Fortitude.SpellId == 21562);
    REQUIRE(PlayerbotPartyBuff::Fortitude.SingleAuraId == 79104);
    REQUIRE(PlayerbotPartyBuff::Fortitude.PartyAuraId == 79105);
}
