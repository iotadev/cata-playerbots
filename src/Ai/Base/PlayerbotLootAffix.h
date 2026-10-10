/* GPL v2 or later. Cata native roll metadata -> donor signed affix query.
 * See PORTING.md. This helper does not establish a live Group/Roll identity. */
#ifndef PLAYERBOT_LOOT_AFFIX_H
#define PLAYERBOT_LOOT_AFFIX_H
#include "PlayerbotItemStats.h"
#include "PlayerbotLootRollIdentity.h"
#include <limits>
namespace PlayerbotEquipment
{
inline PlayerbotItemStats::ItemQuery LootQuery(PlayerbotLootRoll const& roll,
    bool hasProperty, bool hasSuffix, uint32 nativeSuffixFactor)
{
    if (!roll.Group || !roll.Roll || !roll.Entry || !roll.Count || (hasProperty && hasSuffix) ||
        roll.PropertyType > 1 || roll.Property > uint32(std::numeric_limits<int32>::max())) return {};
    if (!hasProperty && !hasSuffix)
        return !roll.PropertyType && !roll.Property && !roll.SuffixFactor ?
            PlayerbotItemStats::ItemQuery{roll.Entry, 0} : PlayerbotItemStats::ItemQuery{};
    if (!roll.Property) return {}; // Random template without a generated affix is incomplete.
    if (hasProperty)
        return !roll.PropertyType && !roll.SuffixFactor ?
            PlayerbotItemStats::ItemQuery{roll.Entry, int32(roll.Property)} : PlayerbotItemStats::ItemQuery{};
    return roll.PropertyType == 1 && nativeSuffixFactor && roll.SuffixFactor == nativeSuffixFactor ?
        PlayerbotItemStats::ItemQuery{roll.Entry, -int32(roll.Property)} : PlayerbotItemStats::ItemQuery{};
}
inline bool SameVariant(uint32 entry, int32 property, uint32 factor,
    uint32 existingEntry, int32 existingProperty, uint32 existingFactor)
{
    return entry == existingEntry && property == existingProperty && factor == existingFactor;
}
}
#endif
