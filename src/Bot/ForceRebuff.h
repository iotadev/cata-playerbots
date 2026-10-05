/* Adapted from donor ForceRebuff.cpp at 037c01418b5d01506917a3db9b44fd56ac5f965c.
 * Released under GNU GPL v2 or any later version. See AUTHORS.md and PORTING.md. */
#ifndef CATA_PLAYERBOT_FORCE_REBUFF_H
#define CATA_PLAYERBOT_FORCE_REBUFF_H
#include <algorithm>
#include <cstdint>
#include <mutex>
#include <optional>

// Map-owned state, no retained Player/SpellInfo pointer or ready-check reply.
class ForceRebuffState
{
public:
    void Begin(std::uint32_t now)
    {
        if (!++serial) ++serial;
        pending = true; completed = false; begin = now; work = true; proposed = false; lastSpell = 0;
    }
    void End() { pending = completed = false; work = proposed = false; lastSpell = 0; }
    void Finish() { End(); completed = true; }
    bool Completed() const { return completed; }
    std::uint32_t Serial() const { return serial; }
    bool IsPending(std::uint32_t now) const { return pending && std::uint32_t(now - begin) < 120000; }
    void BeginCycle() { work = proposed = false; }
    void NoteWork() { work = true; }
    void NoteProposed() { proposed = true; }
    bool HasWork() const { return work || proposed; }
    void NoteCast(std::uint32_t spell) { lastSpell = spell; }
    std::uint32_t LastSpell() const { return lastSpell; }
    bool BelowRefreshTarget(std::int32_t remaining, std::int32_t maximum, std::uint32_t now, bool inCombat) const
    {
        if (!IsPending(now) || inCombat || remaining <= 0 || maximum <= 0) return false;
        // Donor default margin is 60 seconds; elapsed+5s prevents repeated top-offs.
        std::uint32_t margin = std::max(60000u, std::uint32_t(now - begin) + 5000u);
        return std::uint64_t(remaining) + margin < std::uint64_t(maximum);
    }
private:
    bool pending = false, completed = false, work = false, proposed = false;
    std::uint32_t begin = 0, lastSpell = 0, serial = 0;
};
class ForceRebuffMailbox
{
public:
    struct Request { std::uint32_t Requester, Created; };
    bool Post(std::uint32_t requester, std::uint32_t now)
    {
        if (!requester) return false;
        std::lock_guard<std::mutex> lock(mutex);
        if (request) return false;
        request = Request{requester, now};
        return true;
    }
    std::optional<Request> Take(std::uint32_t now)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto result = request;
        request.reset();
        if (result && std::uint32_t(now - result->Created) >= 5000) result.reset();
        return result;
    }
private:
    std::mutex mutex;
    std::optional<Request> request;
};
#endif
