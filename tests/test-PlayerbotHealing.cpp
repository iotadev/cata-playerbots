/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "catch2/catch.hpp"
#include "../src/Ai/Class/Priest/PlayerbotPriestStrategy.h"
#include <limits>

TEST_CASE("Playerbot Priest healing tiers preserve emergency priority", "[PlayerbotPriest]")
{
    using PlayerbotPriest::HealTier;
    using PlayerbotPriest::TierForHealth;
    REQUIRE(TierForHealth(20.0f) == HealTier::Emergency);
    REQUIRE(TierForHealth(54.9f) == HealTier::Emergency);
    REQUIRE(TierForHealth(55.0f) == HealTier::Heal);
    REQUIRE(TierForHealth(79.9f) == HealTier::Heal);
    REQUIRE(TierForHealth(80.0f) == HealTier::Renew);
    REQUIRE(TierForHealth(89.9f) == HealTier::Renew);
    REQUIRE(TierForHealth(90.0f) == HealTier::None);
}

TEST_CASE("Playerbot healing tries the most injured first and falls back", "[PlayerbotPriest]")
{
    std::vector<float> candidates = { 85.0f, 20.0f, 60.0f };
    std::vector<float> attempted;
    bool healed = PlayerbotPriest::TryInHealthOrder(candidates,
        [](float pct) { return pct; },
        [&](float pct) { attempted.push_back(pct); return pct == 60.0f; });
    REQUIRE(healed);
    REQUIRE(attempted == std::vector<float>{ 20.0f, 60.0f });
}

TEST_CASE("Playerbot healing ignores healthy and invalid health values", "[PlayerbotPriest]")
{
    std::vector<float> candidates = { 100.0f, 90.0f, 0.0f, -1.0f,
        std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), 89.0f };
    std::vector<float> attempted;
    REQUIRE_FALSE(PlayerbotPriest::TryInHealthOrder(candidates,
        [](float pct) { return pct; },
        [&](float pct) { attempted.push_back(pct); return false; }));
    REQUIRE(attempted == std::vector<float>{ 89.0f });
}

TEST_CASE("Playerbot healing preserves equal health tie order", "[PlayerbotPriest]")
{
    struct Candidate { int Id; float Health; };
    std::vector<Candidate> candidates = { { 1, 50.0f }, { 2, 20.0f }, { 3, 50.0f } };
    std::vector<int> attempted;
    REQUIRE_FALSE(PlayerbotPriest::TryInHealthOrder(candidates,
        [](Candidate const& candidate) { return candidate.Health; },
        [&](Candidate const& candidate) { attempted.push_back(candidate.Id); return false; }));
    REQUIRE(attempted == std::vector<int>{ 2, 1, 3 });
}

TEST_CASE("Playerbot healing safely handles an empty party candidate list", "[PlayerbotPriest]")
{
    std::vector<float> candidates;
    unsigned attempts = 0;
    REQUIRE_FALSE(PlayerbotPriest::TryInHealthOrder(candidates,
        [](float pct) { return pct; },
        [&](float) { ++attempts; return true; }));
    REQUIRE(attempts == 0);
}
