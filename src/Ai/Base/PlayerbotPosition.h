/* Adapted from donor PositionValue, PositionAction and StayStrategy at
 * 037c01418b5d01506917a3db9b44fd56ac5f965c. GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_POSITION_H
#define PLAYERBOT_POSITION_H
#include "../../Bot/Engine/AiObjectContext.h"
#include "../../Bot/Engine/Strategy/Strategy.h"
#include <cmath>
#include <map>
#include <string>
#include <mutex>
#include <optional>
namespace PlayerbotPosition
{
struct PhaseStamp
{
    uint32 Flags = 0;
    uint64 Personal = 0;
    std::vector<std::pair<uint32, uint32>> Phases;
    std::vector<uint32> Terrain, UiMaps;
    bool operator==(PhaseStamp const&) const = default;
};
inline constexpr uint32 ReturnMovementId = 0x50425354; // PBST, not a generic point move.
inline bool OwnsReturn(uint32 movementId) { return movementId == ReturnMovementId; }
inline bool KeepStay(bool enabled, bool suspended, bool valid) { return enabled && !suspended && valid; }
class Mailbox
{
public:
    struct Request { uint32 Requester, Created; };
    bool Post(uint32 requester, uint32 now)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!requester || pending) return false;
        pending = Request{requester, now}; return true;
    }
    std::optional<Request> Take(uint32 now)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto result = pending; pending.reset();
        return result && uint32(now - result->Created) < 5000 ? result : std::nullopt;
    }
    void Cancel() { std::lock_guard<std::mutex> lock(mutex); pending.reset(); }
private:
    std::mutex mutex;
    std::optional<Request> pending;
};
struct PositionInfo
{
    float X = 0, Y = 0, Z = 0;
    uint32 Map = 0, Instance = 0;
    uint64 Controller = 0;
    bool Set = false;
    PhaseStamp BotPhase, OwnerPhase;
    bool Attempted = false;
    uint32 LastAttempt = 0;
    bool CanAttempt(uint32 now) const { return !Attempted || uint32(now - LastAttempt) >= 5000; }
    bool Capture(float x, float y, float z, uint32 map, uint32 instance, uint64 controller)
    {
        if (!controller || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return false;
        X = x; Y = y; Z = z; Map = map; Instance = instance; Controller = controller; Set = true;
        return true;
    }
    void Reset() { *this = {}; }
    bool Matches(uint32 map, uint32 instance, uint64 controller) const
    { return Set && Controller && Controller == controller && Map == map && Instance == instance; }
};
using PositionMap = std::map<std::string, PositionInfo>;
// Own the map directly: no reference to a member before that member is constructed.
// Deliberately no donor permissive atof/atoi persistence until lifecycle invalidation exists.
class PositionValue final : public ManualSetValue<PositionMap>
{
public:
    explicit PositionValue(PlayerbotAI* ai) : ManualSetValue<PositionMap>(ai, {}, "position") { }
};
enum class ReturnDecision { Wait, Move, Reanchor };
inline ReturnDecision DecideReturn(bool valid, bool safe, float distance)
{
    if (!valid || !safe || !std::isfinite(distance) || distance <= 3.0f) return ReturnDecision::Wait;
    return distance > 35.0f ? ReturnDecision::Reanchor : ReturnDecision::Move;
}
class StayStrategy final : public Strategy
{
public:
    explicit StayStrategy(PlayerbotAI* ai) : Strategy(ai) { }
    std::string const getName() override { return "stay"; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    { triggers.push_back(new TriggerNode("return to stay position", {NextAction("return to stay position", ACTION_MOVE)})); }
    std::vector<NextAction> getDefaultActions() override { return {NextAction("stay", 1.0f)}; }
};
// Map-thread lifecycle/ownership helpers; no native pointer is retained.
bool CaptureStay(PlayerbotAI& ai);
bool ValidStay(PlayerbotAI& ai);
void SuspendReturn(PlayerbotAI& ai);
void Release(PlayerbotAI& ai);
void UpdateReturn(PlayerbotAI& ai);
void AddContexts(SharedNamedObjectContextList<Strategy>& strategies,
    SharedNamedObjectContextList<Action>& actions, SharedNamedObjectContextList<Trigger>& triggers,
    SharedNamedObjectContextList<UntypedValue>& values);
}
#endif
