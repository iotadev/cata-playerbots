/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef CATA_PLAYERBOTS_ADDON_ROSTER_H
#define CATA_PLAYERBOTS_ADDON_ROSTER_H

#include "PlayerbotAddonProtocol.h"
#include "ServerOriginPlayerbotLifecycle.h"
#include <deque>
#include <vector>

namespace PlayerbotAddonProtocol
{
struct ManagedRosterRow
{
    uint32_t Guid;
    std::string Name;
    uint8_t Class;
    uint8_t Level;
    bool Online;
};

inline bool RosterOnline(ServerOriginPlayerbotLifecycle const* receipt)
{
    // A loaded bot remains present while stopping. Pending admission is not
    // presence, and closure is not inferred from a stop acknowledgement.
    return receipt && receipt->HasLoggedIn() && !receipt->IsClosed();
}

inline std::vector<std::string> FrameManagedRoster(std::vector<ManagedRosterRow> const& rows)
{
    std::vector<std::string> entries;
    bool truncated = false;
    for (auto const& row : rows)
    {
        if (entries.size() >= 128)
        {
            truncated = true;
            break;
        }
        if (!row.Guid || row.Name.empty() || row.Name.size() > 64 || !row.Class || row.Class > 11 || !row.Level)
        {
            truncated = true;
            continue;
        }
        std::string entry = "ALT_ROSTER_ENTRY~" + std::to_string(row.Guid) + "~" + EncodeField(row.Name) +
            "~" + std::to_string(row.Class) + "~" + std::to_string(row.Level) + "~" + (row.Online ? "ONLINE" : "OFFLINE");
        if (entry.size() > MaxMessageBytes)
        {
            truncated = true;
            continue;
        }
        entries.push_back(std::move(entry));
    }
    std::string framing = std::to_string(entries.size()) + "~" + (truncated ? "1" : "0");
    entries.insert(entries.begin(), "ALT_ROSTER_BEGIN~" + framing);
    entries.push_back("ALT_ROSTER_END~" + framing);
    return entries;
}

// Match donor roster amplification limit: four batches per two seconds.
class RosterQueryGuard
{
public:
    bool Admit(uint64_t nowMs)
    {
        while (!requests.empty() && nowMs - requests.front() >= 2000)
            requests.pop_front();
        if (requests.size() >= 4)
            return false;
        requests.push_back(nowMs);
        return true;
    }
private:
    std::deque<uint64_t> requests;
};
}
#endif
