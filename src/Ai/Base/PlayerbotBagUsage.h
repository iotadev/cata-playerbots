/* ItemUsageValue bag capacity policy at 79bd428115c8f33b74b13de23b133d49c3f75c60.
 * GPL v2 or later. Positive-only native Cata adaptation; see PORTING.md. */
#ifndef PLAYERBOT_BAG_USAGE_H
#define PLAYERBOT_BAG_USAGE_H
#include "Define.h"
#include <array>
#include <optional>
#include <vector>
class Player;
class WorldSession;
struct ItemTemplate;
namespace PlayerbotBagUsage
{
constexpr uint32 MaxCapacity = 36;
struct Slot { bool Known = false, General = false; uint32 Capacity = 0; };
inline std::optional<uint8> Destination(uint32 candidate, bool usable, std::array<Slot, 4> const& slots)
{
    if (!usable || !candidate || candidate > MaxCapacity) return {};
    std::optional<uint8> best;
    for (uint8 i = 0; i < slots.size(); ++i)
        if (slots[i].Known && slots[i].General && slots[i].Capacity <= MaxCapacity && candidate > slots[i].Capacity &&
            (!best || slots[i].Capacity < slots[*best].Capacity)) best = i;
    return best;
}
inline bool CapacityUpgrade(uint32 candidate, bool usable, std::array<Slot, 4> const& slots)
{ return Destination(candidate, usable, slots).has_value(); } // No negative sale authority.
bool Useful(Player const& player, ItemTemplate const& candidate);
// One native bag fill/upgrade through the existing explicit equip request.
enum class ApplyResult { NoChange, Rejected, Confirmed };
inline bool SafeCarriedBag(bool owned, bool backpack, bool ordinary, bool empty,
    bool busy, uint32 count, bool affixed)
{ return owned && backpack && ordinary && empty && !busy && count == 1 && !affixed; }
struct Content
{
    uint64 Guid = 0;
    uint32 Entry = 0, Count = 0;
    int32 Property = 0;
    uint32 Factor = 0;
    bool operator==(Content const& other) const
    { return Guid == other.Guid && Entry == other.Entry && Count == other.Count && Property == other.Property && Factor == other.Factor; }
};
inline bool SameContents(std::vector<Content> const& before, std::vector<Content> const& after)
{ return before == after; } // Native migration compacts slots in ascending source order.
ApplyResult ApplyOne(WorldSession& session);
}
#endif
