/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef CATA_PLAYERBOTS_ADDON_LIFECYCLE_H
#define CATA_PLAYERBOTS_ADDON_LIFECYCLE_H

#include "ServerOriginPlayerbotLifecycle.h"
#include <deque>
#include <cstdint>
#include <string>
#include <utility>

namespace PlayerbotAddonProtocol
{
struct LifecycleView { char const* State; char const* Reason; };

inline LifecycleView View(ServerOriginPlayerbotLifecycle const* receipt)
{
    if (!receipt)
        return { "OFFLINE", "OK" };
    using State = ServerOriginPlayerbotLifecycle::State;
    switch (receipt->GetState())
    {
        case State::Loading: return { "CONNECTING", "PENDING" };
        case State::Online: return { "ONLINE", "OK" };
        // Donor polling understands only ONLINE/OFFLINE/CONNECTING. Keep a
        // stop pending until native teardown closes the receipt, never OK early.
        case State::Stopping: return { "CONNECTING", "STOPPING" };
        case State::Stopped: return { "OFFLINE", "OK" };
        case State::LoginFailed:
            return receipt->IsClosed() ? LifecycleView{ "OFFLINE", "LOGIN_FAILED" } :
                LifecycleView{ "CONNECTING", "LOGIN_FAILED" };
        case State::Disconnected: return { "OFFLINE", "DISCONNECTED" };
        case State::Shutdown: return { "OFFLINE", "SHUTDOWN" };
    }
    return { "OFFLINE", "UNKNOWN" };
}

// Donor-shaped mutation rate and replay guard, with deterministic time input.
// Tokens are retained for two minutes. Full storage rejects rather than evicts
// unexpired tokens, so pressure cannot bypass replay protection.
class MutationGuard
{
public:
    char const* Admit(std::string const& token, uint64_t nowMs)
    {
        while (!requests.empty() && nowMs - requests.front() >= 2000)
            requests.pop_front();
        while (!tokens.empty() && nowMs - tokens.front().second >= 120000)
            tokens.pop_front();
        if (requests.size() >= 64)
            return "RATE_LIMIT";
        requests.push_back(nowMs);
        for (auto const& entry : tokens)
            if (entry.first == token)
                return "REPLAY";
        if (tokens.size() >= 320)
            return "TOKEN_LIMIT";
        tokens.emplace_back(token, nowMs);
        return nullptr;
    }
private:
    std::deque<uint64_t> requests;
    std::deque<std::pair<std::string, uint64_t>> tokens;
};
}
#endif
