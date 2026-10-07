/* Donor LootRollAction at 037c01418b5d01506917a3db9b44fd56ac5f965c.
 * GPL v2 or later. Native world/map adapter; see PORTING.md. */
#ifndef PLAYERBOT_LOOT_ROLL_ADAPTER_H
#define PLAYERBOT_LOOT_ROLL_ADAPTER_H
#include "PlayerbotLootRollIdentity.h"
#include <mutex>
#include <optional>
namespace PlayerbotRoll
{
struct Request { PlayerbotLootRoll Identity; uint32 Controller = 0, Created = 0; uint64 Serial = 0; };
struct Reply { Request Original; uint8 Choice = 0; };
inline bool Fresh(Request const& request, uint32 now) { return uint32(now - request.Created) < 5000; }
class Mailbox
{
public:
    bool Post(PlayerbotLootRoll const& roll, uint32 controller, uint32 now)
    {
        std::lock_guard<std::mutex> lock(mutex);
        Expire(now);
        if (active || !roll.Group || !roll.Roll || !roll.Entry || !controller) return false;
        active = Request{roll, controller, now, ++serial}; pending = active;
        return true;
    }
    std::optional<Request> Take()
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto result = pending; pending.reset(); return result;
    }
    bool Complete(Request const& request, uint8 choice, uint32 now)
    {
        std::lock_guard<std::mutex> lock(mutex);
        Expire(now);
        if (!active || reply || request.Serial != active->Serial || request.Controller != active->Controller ||
            request.Created != active->Created || !request.Identity.Matches(active->Identity) ||
            !PlayerbotLootRoll::Admits(true, active->Identity.Mask, choice)) return false;
        reply = Reply{*active, choice}; return true;
    }
    std::optional<Reply> Consume(uint32 now)
    {
        std::lock_guard<std::mutex> lock(mutex);
        Expire(now);
        auto result = reply;
        if (result) { active.reset(); pending.reset(); reply.reset(); }
        return result;
    }
    void Cancel()
    {
        std::lock_guard<std::mutex> lock(mutex);
        active.reset(); pending.reset(); reply.reset();
    }
private:
    void Expire(uint32 now)
    { if (active && !Fresh(*active, now)) { active.reset(); pending.reset(); reply.reset(); } }
    std::mutex mutex;
    uint64 serial = 0;
    std::optional<Request> active, pending;
    std::optional<Reply> reply;
};
}
#endif
