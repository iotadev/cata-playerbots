/*
 * Cata adaptation of mod-playerbots LootNonCombatStrategy / LootAction at
 * 7bae1b5c58c76a0aa20381155edc08096d1485b2. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOT_CORPSE_LOOT_H
#define PLAYERBOT_CORPSE_LOOT_H
#include "../../Bot/Engine/Strategy/Strategy.h"
#include "ObjectGuid.h"
#include <array>
#include <mutex>
#include <optional>

class WorldSession;
namespace PlayerbotCorpseLoot
{
struct Request
{
    ObjectGuid Corpse;
    ObjectGuid Controller;
    uint64 Generation = 0;
};
// One bounded request; cancellation and stale completion cannot replace a newer request.
class Mailbox final
{
public:
    bool Submit(ObjectGuid corpse, ObjectGuid controller)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_pending || corpse.IsEmpty() || controller.IsEmpty())
            return false;
        _pending = true;
        _resumeNeeded = true;
        _request = Request { corpse, controller, ++_generation };
        return true;
    }
    std::optional<Request> Take()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto result = _request;
        _request.reset();
        return result;
    }
    bool Pending() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _pending;
    }
    void Finish(uint64 generation)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (generation == _generation)
            _pending = false;
    }
    bool TakeResumeNeeded()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_pending)
            return false;
        bool result = _resumeNeeded;
        _resumeNeeded = false;
        return result;
    }
    void Cancel()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        ++_generation;
        _request.reset();
        _pending = false;
    }
private:
    mutable std::mutex _mutex;
    std::optional<Request> _request;
    uint64 _generation = 0;
    bool _pending = false;
    bool _resumeNeeded = false;
};
class AttemptHistory final
{
public:
    bool CanAttempt(ObjectGuid guid, uint32 now) const
    {
        for (auto const& entry : _entries)
            if (entry.Guid == guid && uint32(now - entry.Time) < BackoffMs)
                return false;
        return !guid.IsEmpty();
    }
    void Record(ObjectGuid guid, uint32 now)
    {
        _entries[_next] = { guid, now };
        _next = (_next + 1) % _entries.size();
    }
    static constexpr uint32 BackoffMs = 30000;
private:
    struct Entry { ObjectGuid Guid; uint32 Time = 0; };
    std::array<Entry, 8> _entries {};
    std::size_t _next = 0;
};
class LootStrategy final : public Strategy
{
public:
    explicit LootStrategy(PlayerbotAI* ai) : Strategy(ai) { }
    std::string const getName() override { return "loot"; }
    uint32_t GetType() const override { return STRATEGY_TYPE_NONCOMBAT; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("far from loot target", { NextAction("move to loot", 7.0f) }));
        triggers.push_back(new TriggerNode("can loot", { NextAction("open loot", 8.0f) }));
    }
};
// Map-owned adaptation of LootObjectStack: fixed capacity, GUIDs only, expiry.
// Discovery is restricted to observed defeated targets and controller selection.
class Candidates final
{
public:
    void Add(ObjectGuid guid, uint32 now)
    {
        if (guid.IsEmpty())
            return;
        for (auto& entry : _entries)
            if (entry.Guid == guid)
            {
                if (uint32(now - entry.Time) >= LifetimeMs)
                    entry.Time = now;
                return; // no renewal on every decision tick
            }
        _entries[_next] = { guid, now };
        _next = (_next + 1) % _entries.size();
    }
    template<class Visitor> void Visit(uint32 now, Visitor visitor) const
    {
        for (auto const& entry : _entries)
            if (!entry.Guid.IsEmpty() && uint32(now - entry.Time) < LifetimeMs)
                visitor(entry.Guid);
    }
    void Clear() { _entries = {}; _next = 0; }
    static constexpr uint32 LifetimeMs = 60000;
private:
    struct Entry { ObjectGuid Guid; uint32 Time = 0; };
    std::array<Entry, 8> _entries {};
    std::size_t _next = 0;
};
class Pursuit final
{
public:
    bool Begin(ObjectGuid corpse, uint32 now)
    {
        if (Active() || corpse.IsEmpty())
            return false;
        _corpse = corpse;
        _started = now;
        return true;
    }
    bool Active() const { return !_corpse.IsEmpty(); }
    ObjectGuid Corpse() const { return _corpse; }
    bool Expired(uint32 now) const { return Active() && uint32(now - _started) >= TimeoutMs; }
    void End() { _corpse.Clear(); }
    static constexpr uint32 TimeoutMs = 10000;
private:
    ObjectGuid _corpse;
    uint32 _started = 0;
};
void AddContexts(SharedNamedObjectContextList<Strategy>& strategies,
    SharedNamedObjectContextList<Action>& actions, SharedNamedObjectContextList<Trigger>& triggers);
void ProcessWorld(WorldSession& session, Mailbox& mailbox, ObjectGuid currentController);
// Returns true while moving; ended is a one-update signal to resume formation.
bool UpdateMovement(PlayerbotAI& ai, bool interrupt, bool& ended);
}
#endif
