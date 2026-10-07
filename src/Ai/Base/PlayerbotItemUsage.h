/* Adapted from donor ItemUsageValue and LootRollAction at
 * 037c01418b5d01506917a3db9b44fd56ac5f965c. GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_ITEM_USAGE_H
#define PLAYERBOT_ITEM_USAGE_H
#include "PlayerbotConsumableUsage.h"
#include "PlayerbotEquipment.h"
namespace PlayerbotItemUsage
{
// Retain donor category identities; Unknown is deliberately not None.
enum class Usage : uint8
{
    None = 0, Equip = 1, Replace = 2, BadEquip = 3, BrokenEquip = 4,
    Quest = 5, Skill = 6, Use = 7, GuildTask = 8, Disenchant = 9,
    Auction = 10, Keep = 11, Vendor = 12, Ammo = 13, Unknown = 255
};
enum class Scope : uint8 { Unavailable, ConsumableStock, CarriedEquipment, TemplateEquipment };
struct Fact
{
    Usage Result = Usage::Unknown;
    Scope Source = Scope::Unavailable;
};
inline Usage Consumable(PlayerbotConsumable::Usage usage)
{
    switch (usage)
    {
        case PlayerbotConsumable::Usage::None: return Usage::None;
        case PlayerbotConsumable::Usage::Use: return Usage::Use;
        case PlayerbotConsumable::Usage::Keep: return Usage::Keep;
        default: return Usage::Unknown;
    }
}
inline Usage Equipment(PlayerbotEquipment::Decision decision)
{
    switch (decision)
    {
        case PlayerbotEquipment::Decision::FillSlot:
        case PlayerbotEquipment::Decision::Upgrade: return Usage::Equip;
        case PlayerbotEquipment::Decision::ReplaceBroken: return Usage::Replace;
        case PlayerbotEquipment::Decision::NeedsRepair: return Usage::BrokenEquip;
        // This is a carried-only observation, not donor vendor/AH classification.
        case PlayerbotEquipment::Decision::Keep: return Usage::Keep;
        default: return Usage::Unknown;
    }
}
inline Usage SurveyUsage(PlayerbotEquipment::Survey const& survey, uint32 entry, int32 property)
{
    if (!survey.Available || !entry) return Usage::Unknown;
    Usage result = Usage::Unknown;
    bool matched = false, incomplete = false;
    for (auto const& row : survey.Items)
    {
        if (row.Input.Entry != entry || row.Input.Property != property) continue;
        matched = true;
        Usage usage = Equipment(row.Result);
        // One safely qualified destination suffices, just as native equip uses
        // one candidate. Unknown alternative slots cannot invalidate that proof.
        if (usage == Usage::Equip || usage == Usage::Replace) return usage;
        if (usage == Usage::Unknown) incomplete = true;
        else if (result == Usage::Unknown || usage == Usage::Keep) result = usage;
    }
    return matched && !incomplete ? result : Usage::Unknown;
}
inline Usage CarriedEquipment(PlayerbotEquipment::Survey const& survey, uint32 entry, int32 property)
{
    return SurveyUsage(survey, entry, property);
}

// Read-only policy, NOT a native roll operation. A future owner-thread adapter
// must supply authoritative loot usage, unique eligibility and allowed choices.
// CarriedEquipment facts alone are not evidence about an unowned loot instance.
enum class Kind : uint8 { Equipment, Recipe, Other };
enum class Vote : uint8 { Pass, Need, Greed, Disenchant };
struct RollPolicy
{
    uint8 NeedLevel = 0; // 0 pass, 1 greed, 2 need; fail closed by default.
    bool Greed = false, Disenchant = false, Recipes = false;
};
inline std::optional<Vote> ChooseRoll(Usage usage, Kind kind, RollPolicy policy,
    bool lootAllowed, bool uniqueBlocked, bool recipeBindsOnPickup)
{
    if (usage == Usage::Unknown || policy.NeedLevel > 2) return {};
    if (!lootAllowed) return Vote::Pass;
    Vote vote = Vote::Pass;
    if (usage == Usage::Disenchant)
        vote = policy.Disenchant ? Vote::Disenchant : Vote::Greed;
    else if (kind == Kind::Equipment)
        vote = usage == Usage::Equip || usage == Usage::Replace || usage == Usage::BadEquip ?
            Vote::Need : (usage != Usage::None ? Vote::Greed : Vote::Pass);
    else if (kind == Kind::Recipe)
    {
        if (policy.Recipes)
            vote = usage == Usage::Skill ? Vote::Need : (!recipeBindsOnPickup ? Vote::Greed : Vote::Pass);
    }
    else
    {
        switch (usage)
        {
            case Usage::Equip: case Usage::Replace: case Usage::BadEquip: case Usage::GuildTask:
                vote = Vote::Need; break;
            case Usage::Skill: case Usage::Use: case Usage::Auction: case Usage::Vendor:
                vote = Vote::Greed; break;
            default: break;
        }
    }
    if (vote == Vote::Need)
    {
        if (!policy.NeedLevel || uniqueBlocked) return Vote::Pass;
        if (policy.NeedLevel == 1) return Vote::Greed; // donor downgrade is independent of Greed gate
    }
    else if (vote == Vote::Greed && !policy.Greed) return Vote::Pass;
    return vote;
}
void AddContexts(SharedNamedObjectContextList<UntypedValue>& values);
}
#endif
