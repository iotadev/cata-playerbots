/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_GROUP_STRATEGY_H
#define PLAYERBOT_GROUP_STRATEGY_H

#include <cstddef>
#include <cstdint>

class Player;

namespace PlayerbotGroup
{
struct FollowPosition
{
    float Distance;
    float Angle;
    std::uint32_t Signature;
};

// A compact Cata adaptation of Playerbots' roster-aware formation value.
// Angles are relative to the owner's facing; pi is directly behind.
inline FollowPosition PositionForSlot(std::size_t index, std::size_t count)
{
    constexpr float Behind = 3.14159265f;
    if (count < 2)
        return { 2.0f, Behind, 1 };

    // Symmetric rear arc: fixed spacing, no shared destination. This is a
    // position value; movement/path validation stays with the Cata core.
    float center = float(count - 1) / 2.0f;
    float angle = Behind + (float(index) - center) * 0.50f;
    return { 2.8f, angle, std::uint32_t((count << 16) | index) };
}
// The far-follow path mode has hysteresis so short distance changes do not
// repeatedly replace a movement generator.
inline bool ShouldPathCatchUp(bool alreadyPathing, float distance)
{
    return distance > (alreadyPathing ? 12.0f : 18.0f);
}

inline float MeleeChaseAngle(bool tanking)
{
    return tanking ? 0.0f : 3.14159265f;
}
FollowPosition PositionFor(Player const& bot, Player const& owner);
bool CanResumeAfterDeath(Player const& bot, Player const& owner);
void GreetOnJoin(Player& bot, Player& owner);
}

#endif
