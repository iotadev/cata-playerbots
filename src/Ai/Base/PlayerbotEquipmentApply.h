/* GPL v2 or later. Bounded native inventory execution; see PORTING.md. */
#ifndef PLAYERBOT_EQUIPMENT_APPLY_H
#define PLAYERBOT_EQUIPMENT_APPLY_H
#include "PlayerbotEquipment.h"
#include <mutex>
class WorldSession;
class PlayerbotAI;
namespace PlayerbotEquipmentApply
{
struct Request { uint32 Requester = 0, Created = 0; uint64 Serial = 0; };
inline bool Fresh(Request const& request, uint32 now) { return uint32(now - request.Created) < 5000; }
class Mailbox
{
public:
    bool Post(uint32 requester, uint32 now)
    {
        if (!requester) return false;
        std::lock_guard<std::mutex> lock(mutex);
        if (busy) return false;
        busy = true; pending = Request{requester, now, ++serial}; return true;
    }
    std::optional<Request> Take()
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto request = pending; pending.reset(); return request; // Busy until completion.
    }
    void Finish(uint64 completed)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (completed == serial) busy = false;
    }
private:
    std::mutex mutex;
    bool busy = false;
    uint64 serial = 0;
    std::optional<Request> pending;
};
inline bool Actionable(PlayerbotEquipment::Decision decision)
{
    using PlayerbotEquipment::Decision;
    return decision == Decision::Upgrade || decision == Decision::FillSlot || decision == Decision::ReplaceBroken;
}
enum class Result { Unavailable, NoChange, Rejected, Confirmed };
Result ApplyOne(WorldSession& session, PlayerbotAI& ai);
}
#endif
