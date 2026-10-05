/*
 * Cata ready-check transport adaptation. Donor policy: ReadyCheckAction.cpp at
 * 037c01418b5d01506917a3db9b44fd56ac5f965c. See PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOT_READY_CHECK_H
#define PLAYERBOT_READY_CHECK_H
#include <cstdint>
#include <algorithm>
#include <limits>
#include <mutex>
#include <optional>
#include <utility>

namespace PlayerbotReadyCheck
{
inline constexpr uint32_t Lifetime = 30000;
struct Request
{
    uint64_t Group = 0;
    uint64_t Check = 0;
    uint32_t Initiator = 0;
    uint32_t Created = 0;
};
struct Reply { Request Identity; bool Ready; };
struct Supplies
{
    uint32_t Food = 0, Drink = 0, HealingPotion = 0, ManaPotion = 0;
    bool Ready(bool usesMana) const
    { return Food && HealingPotion && (!usesMana || (Drink && ManaPotion)); }
};
inline void AddCount(uint32_t& total, uint32_t count)
{
    total = uint32_t(std::min<uint64_t>(uint64_t(total) + count, std::numeric_limits<uint32_t>::max()));
}
// Native metadata is translated at the map boundary. A stack is counted once
// in each matching category, even when an item has several on-use effects.
inline void CountItem(Supplies& supplies, uint32_t count, bool usable, bool food, bool drink, bool heal, bool mana)
{
    if (!usable) return;
    if (food) AddCount(supplies.Food, count);
    if (drink) AddCount(supplies.Drink, count);
    if (heal) AddCount(supplies.HealingPotion, count);
    if (mana) AddCount(supplies.ManaPotion, count);
}
struct DeferredPass
{
    Request Identity;
    uint64_t Controller;
    uint32_t RebuffSerial;
    bool OwnsPass;
};
enum class DeferredResult { Wait, Evaluate, Reject, Cancel };
inline bool Matches(Request const& a, Request const& b)
{
    return a.Group == b.Group && a.Check == b.Check && a.Initiator == b.Initiator && a.Created == b.Created;
}
inline bool Current(Request const& request, uint32_t now)
{
    return request.Group && request.Check && request.Initiator && uint32_t(now - request.Created) < Lifetime;
}
inline DeferredResult PollDeferred(Request const& request, uint32_t now, bool current,
    bool sameController, bool samePass, bool safe, bool pending, bool completed)
{
    if (!Current(request, now) || !current) return DeferredResult::Cancel;
    if (!sameController || !samePass || !safe) return DeferredResult::Reject;
    if (pending) return DeferredResult::Wait;
    return completed ? DeferredResult::Evaluate : DeferredResult::Reject;
}
// Donor HP/MP/distance thresholds, plus native life/combat/transfer/cast guards.
// This is basic operational readiness, not inventory or encounter readiness.
inline bool BasicReadiness(bool available, bool alive, bool combat, bool transferring,
    bool casting, float health, bool usesMana, float mana, bool nearInitiator)
{
    return available && alive && !combat && !transferring && !casting && nearInitiator &&
        health > 85.0f && (!usesMana || mana > 65.0f);
}
// Session-owned copied identities only. New checks supersede queued work/replies;
// a map result for an older check cannot overwrite the replacement.
class Mailbox
{
public:
    std::optional<Request> Active(uint32_t now)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _active && Current(*_active, now) ? _active : std::nullopt;
    }
    bool IsCurrent(Request const& request, uint32_t now)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _active && Matches(*_active, request) && Current(request, now);
    }
    void Cancel(Request const& request)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_active || !Matches(*_active, request)) return;
        _active.reset(); _request.reset(); _reply.reset();
    }
    void Post(Request request)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _active = request;
        _request = request;
        _reply.reset();
    }
    std::optional<Request> TakeRequest(uint32_t now)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto request = std::exchange(_request, std::nullopt);
        return request && Current(*request, now) ? request : std::nullopt;
    }
    bool Complete(Request const& request, bool ready, uint32_t now)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_active || !Matches(*_active, request) || !Current(request, now) || _reply)
            return false;
        _reply = Reply{request, ready};
        return true;
    }
    std::optional<Reply> TakeReply(uint32_t now)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto reply = std::exchange(_reply, std::nullopt);
        if (!reply) return std::nullopt;
        _active.reset();
        return Current(reply->Identity, now) ? reply : std::nullopt;
    }
private:
    std::mutex _mutex;
    std::optional<Request> _active, _request;
    std::optional<Reply> _reply;
};
}
#endif
