/* Donor ItemUsageValue disenchant retention at 79bd428115c8f33b74b13de23b133d49c3f75c60.
 * GPL v2 or later. See PORTING.md for native Cata and instance qualification. */
#ifndef PLAYERBOT_DISENCHANT_ITEM_H
#define PLAYERBOT_DISENCHANT_ITEM_H
#include "PlayerbotEquipment.h"
#include "ItemTemplate.h"
class Player;
namespace PlayerbotDisenchantItem
{
inline bool Eligible(uint32 itemClass, uint32 quality, uint32 binding, bool soulbound,
    bool enchanting, uint32 skill, uint32 required, uint32 loot)
{
    return (itemClass == ITEM_CLASS_ARMOR || itemClass == ITEM_CLASS_WEAPON) &&
        quality >= ITEM_QUALITY_UNCOMMON && quality <= ITEM_QUALITY_EPIC && enchanting &&
        required != uint32(-1) && skill >= required && loot &&
        (binding == BIND_ON_ACQUIRE || (binding == BIND_ON_EQUIP && soulbound));
}
inline bool NonUpgrade(PlayerbotEquipment::Survey const& survey, ObjectGuid item,
    uint32 entry, int32 property, uint32 factor)
{
    if (!survey.Available || item.IsEmpty()) return false;
    bool found = false;
    for (auto const& row : survey.Items)
    {
        if (row.Input.Item != item) continue;
        found = true;
        if (row.Input.Entry != entry || row.Input.Property != property || row.Input.SuffixFactor != factor ||
            row.Input.Existing.IsEmpty() || row.Input.Existing == item || !row.LayoutKnown ||
            !row.CandidateScore || !row.ExistingScore || !std::isfinite(*row.CandidateScore) ||
            !std::isfinite(*row.ExistingScore) || row.Result != PlayerbotEquipment::Decision::Keep) return false;
    }
    return found;
}
// Read on the inventory owner. True is retention only, never cast/sale authority.
bool Useful(Player const& player, uint32 entry, int32 property, PlayerbotEquipment::Survey const& survey);
}
#endif
