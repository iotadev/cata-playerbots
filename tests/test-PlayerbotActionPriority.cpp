/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "catch2/catch.hpp"
#include "../src/Ai/Base/PlayerbotCombatDecision.h"
#include <array>
#include <vector>

TEST_CASE("Playerbot actions are tried in priority order", "[PlayerbotDecision]")
{
    using namespace PlayerbotDecision;
    std::array<Action, 3> actions = {{
        { 1, "first", nullptr, ActionTarget::Enemy },
        { 2, "second", nullptr, ActionTarget::Self },
        { 3, "third", nullptr, ActionTarget::Enemy }
    }};

    std::vector<std::uint32_t> tried;
    bool cast = ExecuteByPriority(actions,
        [](Action const& action) { return action.SpellId != 1; },
        [&tried](Action const& action)
        {
            tried.push_back(action.SpellId);
            return action.SpellId == 3;
        });

    REQUIRE(cast);
    REQUIRE(tried == std::vector<std::uint32_t>{ 2, 3 });
}

TEST_CASE("Playerbot priority stops at first successful action", "[PlayerbotDecision]")
{
    using namespace PlayerbotDecision;
    std::array<Action, 3> actions = {{
        { 1, "first", nullptr, ActionTarget::Enemy },
        { 2, "second", nullptr, ActionTarget::Self },
        { 3, "third", nullptr, ActionTarget::Enemy }
    }};

    std::vector<std::uint32_t> tried;
    bool cast = ExecuteByPriority(actions,
        [](Action const&) { return true; },
        [&tried](Action const& action)
        {
            tried.push_back(action.SpellId);
            return action.SpellId == 2;
        });

    REQUIRE(cast);
    REQUIRE(tried == std::vector<std::uint32_t>{ 1, 2 });
}

TEST_CASE("Playerbot priority reports when no action succeeds", "[PlayerbotDecision]")
{
    using namespace PlayerbotDecision;
    std::array<Action, 1> actions = {{
        { 1, "first", nullptr, ActionTarget::Enemy }
    }};

    bool cast = ExecuteByPriority(actions,
        [](Action const&) { return true; },
        [](Action const&) { return false; });

    REQUIRE_FALSE(cast);
}

TEST_CASE("Playerbot priority never attempts ineligible actions", "[PlayerbotDecision]")
{
    using namespace PlayerbotDecision;
    std::array<Action, 2> actions = {{
        { 1, "unlearned", nullptr, ActionTarget::Enemy },
        { 2, "untriggered", nullptr, ActionTarget::Self }
    }};
    unsigned attempts = 0;
    REQUIRE_FALSE(ExecuteByPriority(actions,
        [](Action const&) { return false; },
        [&attempts](Action const&) { ++attempts; return true; }));
    REQUIRE(attempts == 0);
}

TEST_CASE("Playerbot priority accepts an empty strategy", "[PlayerbotDecision]")
{
    using namespace PlayerbotDecision;
    unsigned attempts = 0;
    REQUIRE_FALSE(ExecuteByPriority(std::span<Action const>{},
        [](Action const&) { return true; },
        [&attempts](Action const&) { ++attempts; return true; }));
    REQUIRE(attempts == 0);
}

TEST_CASE("Playerbot party buff upkeep recognizes both Cata aura variants", "[PlayerbotDecision]")
{
    using PlayerbotDecision::NeedsPartyBuff;
    REQUIRE(NeedsPartyBuff(true, false, false));
    REQUIRE_FALSE(NeedsPartyBuff(false, false, false));
    REQUIRE_FALSE(NeedsPartyBuff(true, true, false));
    REQUIRE_FALSE(NeedsPartyBuff(true, false, true));
    REQUIRE_FALSE(NeedsPartyBuff(true, true, true));
}
