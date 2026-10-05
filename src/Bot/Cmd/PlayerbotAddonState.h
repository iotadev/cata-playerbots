/*
 * Adapted from mod-multibot-bridge AppendStateFramesForBot at
 * 1da05982e478cb00e0b6c87314afe7e0e9653ffb. See PORTING.md. GPL v2 or later.
 */
#ifndef PLAYERBOTS_ADDON_STATE_H
#define PLAYERBOTS_ADDON_STATE_H
#include "PlayerbotAddonProtocol.h"
#include "PlayerbotSessionHooks.h"
#include <set>
#include <vector>

namespace PlayerbotAddonProtocol
{
struct StateRow { std::string Name; std::vector<std::string> Combat, NonCombat; };
inline bool SnapshotFresh(PlayerbotStrategySnapshot const& snapshot, uint32_t bot, uint32_t controller, uint32_t now)
{
    // Zero controller is valid for an unattached bot; only native GM read
    // authorization may use that identity. Ordinary callers pass their own GUID.
    return bot && snapshot.Bot == bot && snapshot.Controller == controller &&
        uint32_t(now - snapshot.Created) < 5000;
}
// Preflight the complete response before emitting any BEGIN; failure is never truncation.
inline std::vector<std::string> FrameStrategyStates(std::string const& token, std::vector<StateRow> const& rows, bool global)
{
    auto abort = [&] { return std::vector<std::string>{"STATE_ABORT~" + (ValidToken(token) ? token : "invalid") + "~~STATE_TOO_LARGE"}; };
    if (!ValidToken(token) || rows.size() > 128 || (!global && rows.size() != 1)) return abort();
    std::vector<std::string> frames;
    auto add = [&](std::string value)
    {
        if (value.size() > MaxMessageBytes || frames.size() >= 256) return false;
        frames.push_back(std::move(value));
        return true;
    };
    if (global && !add("STATES_BEGIN~" + token + "~" + std::to_string(rows.size()))) return abort();
    std::set<std::string> names;
    for (auto const& row : rows)
    {
        if (row.Name.empty() || row.Name.size() > 64 || !names.insert(row.Name).second ||
            row.Combat.size() > 256 || row.NonCombat.size() > 256) return abort();
        std::string identity = token + "~" + EncodeField(row.Name);
        std::string counts = "~" + std::to_string(row.Combat.size()) + "~" + std::to_string(row.NonCombat.size());
        if (!add("STATE_BEGIN~" + identity + counts)) return abort();
        for (auto const& scope : {std::pair<char, std::vector<std::string> const*>{'C', &row.Combat}, {'N', &row.NonCombat}})
        {
            std::set<std::string> unique;
            size_t index = 0, bytes = 0;
            for (auto const& name : *scope.second)
            {
                bytes += name.size();
                if (name.empty() || name.size() > 192 || bytes > 16384 || !unique.insert(name).second ||
                    !add("STATE_ITEM~" + identity + "~" + scope.first + "~" + std::to_string(++index) + "~" + EncodeField(name))) return abort();
            }
        }
        if (!add("STATE_END~" + identity + counts)) return abort();
    }
    if (global && !add("STATES_END~" + token + "~" + std::to_string(rows.size()))) return abort();
    return frames;
}
}
#endif
