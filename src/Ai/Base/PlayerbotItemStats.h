/* Adapted from donor StatsCollector::CollectItemStats/CollectByItemStatType at
 * 037c01418b5d01506917a3db9b44fd56ac5f965c. GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_ITEM_STATS_H
#define PLAYERBOT_ITEM_STATS_H
#include "../../Bot/Engine/AiObjectContext.h"
#include "ItemTemplate.h"
#include "SpellAuraDefines.h"
#include "Unit.h"
#include "Stats.h"
#include <array>
#include <cmath>
#include <charconv>
#include <optional>
#include <string_view>
namespace PlayerbotItemStats
{
// Preserve donor stat order; append Cata mastery rather than alias a Wrath stat.
enum class Stat : uint8
{
    Agility, Strength, Intellect, Spirit, Stamina, Hit, Crit, Haste, Armor,
    Defense, Dodge, Parry, BlockValue, BlockRating, Resilience, HealthRegen,
    SpellPower, SpellPenetration, HealPower, ManaRegen, AttackPower,
    ArmorPenetration, Expertise, MeleeDps, RangedDps, Bonus, Mastery, Count
};
enum Profile : uint8 { MeleeDamage = 1, MeleeTank = 2, Ranged = 4, SpellDamage = 8, SpellHeal = 16 };
inline Stat WeaponChannel(uint32 inventoryType)
{
    return inventoryType == INVTYPE_RANGED || inventoryType == INVTYPE_RANGEDRIGHT || inventoryType == INVTYPE_THROWN ?
        Stat::RangedDps : Stat::MeleeDps;
}
struct ItemQuery { uint32 Item = 0; int32 Property = 0; explicit operator bool() const { return Item != 0; } };
inline ItemQuery ParseQuery(std::string_view text)
{
    if (text.empty() || text.size() > 22) return {};
    size_t comma = text.find(',');
    auto identity = text.substr(0, comma);
    if (identity.empty() || identity.size() > 10) return {};
    ItemQuery result;
    auto item = std::from_chars(identity.data(), identity.data() + identity.size(), result.Item);
    if (item.ec != std::errc{} || item.ptr != identity.data() + identity.size() || !result.Item) return {};
    if (comma != std::string_view::npos)
    {
        auto affix = text.substr(comma + 1);
        if (affix.empty() || affix.size() > 11) return {};
        auto property = std::from_chars(affix.data(), affix.data() + affix.size(), result.Property);
        if (property.ec != std::errc{} || property.ptr != affix.data() + affix.size()) return {};
    }
    return result;
}
inline uint32 PropertyIdentity(int32 value) { return value < 0 ? uint32(-int64(value)) : uint32(value); }
inline std::optional<uint32> SuffixAmount(uint32 allocation, uint32 factor)
{
    uint64 amount = uint64(allocation) * factor / 10000;
    return amount <= UINT32_MAX ? std::optional<uint32>(uint32(amount)) : std::nullopt;
}
inline std::optional<float> SocketMultiplier(uint32 count)
{
    return count <= MAX_ITEM_PROTO_SOCKETS ? std::optional<float>(1.0f + count * 0.03f) : std::nullopt;
}
inline float SetMultiplier(bool hasSet, bool foundEquippedSet, uint32 equippedCount, uint32 maximumThreshold)
{
    if (!hasSet) return 1.0f;
    if (!foundEquippedSet) return 1.05f;
    return equippedCount < maximumThreshold ? 1.0f + equippedCount * 0.1f : 1.0f;
}
inline Profile ChooseProfile(bool healer, bool caster, bool tank, bool melee)
{
    return healer ? SpellHeal : (caster ? SpellDamage : (tank ? MeleeTank : (melee ? MeleeDamage : Ranged)));
}
struct BaseStats
{
    std::array<float, static_cast<size_t>(Stat::Count)> Values{};
    bool Available = false;
    uint8 OwnerClass = 0, OwnerLevel = 0;
    uint32 OwnerSpec = 0;
    Profile OwnerProfile = MeleeDamage;
    bool UnsupportedStats = false;
    // This snapshot is never a complete item score. Later layers own these inputs.
    bool HasItemEffects = false, HasRandomProperties = false, HasSockets = false, HasItemSet = false;
    bool UnsupportedEffects = false, HasProcEffects = false, HasUseEffects = false, HasConditionalEffects = false;
    bool AffixSupplied = false, AffixResolved = false, AffixPoolUnverified = false;
    bool AffixInstanceVerified = false;
    bool AffixLootVerified = false; // Local copied fact, backed by native pending-roll metadata.
    bool SetMetadataKnown = false;
    uint32 SocketCount = 0, SocketBonus = 0, EquippedSetPieces = 0, MaximumSetThreshold = 0;
    float SocketHeuristic = 1.0f, SetHeuristic = 1.0f; // Donor score multipliers, never active bonuses.
    float Get(Stat stat) const { return Values[static_cast<size_t>(stat)]; }
    void Add(Stat stat, float value) { Values[static_cast<size_t>(stat)] += value; }
    bool AddItemStat(int32 type, float value, Profile profile)
    {
        if (type < 0 || !value) return true; // Native empty stat entries.
        if (!std::isfinite(value)) { UnsupportedStats = true; return false; }
        bool melee = profile & (MeleeDamage | MeleeTank);
        bool spell = profile & (SpellDamage | SpellHeal);
        switch (type)
        {
            case ITEM_MOD_MANA: Add(Stat::ManaRegen, value / 10); break;
            case ITEM_MOD_HEALTH: Add(Stat::Stamina, value / 15); break;
            case ITEM_MOD_AGILITY: Add(Stat::Agility, value); break;
            case ITEM_MOD_STRENGTH: Add(Stat::Strength, value); break;
            case ITEM_MOD_INTELLECT: Add(Stat::Intellect, value); break;
            case ITEM_MOD_SPIRIT: Add(Stat::Spirit, value); break;
            case ITEM_MOD_STAMINA: Add(Stat::Stamina, value); break;
            case ITEM_MOD_DEFENSE_SKILL_RATING: Add(Stat::Defense, value); break;
            case ITEM_MOD_DODGE_RATING: Add(Stat::Dodge, value); break;
            case ITEM_MOD_PARRY_RATING: Add(Stat::Parry, value); break;
            case ITEM_MOD_BLOCK_RATING: Add(Stat::BlockRating, value); break;
            case ITEM_MOD_HIT_MELEE_RATING: if (melee) Add(Stat::Hit, value); break;
            case ITEM_MOD_HIT_RANGED_RATING: if (profile == Ranged) Add(Stat::Hit, value); break;
            case ITEM_MOD_HIT_SPELL_RATING: if (spell) Add(Stat::Hit, value); break;
            case ITEM_MOD_CRIT_MELEE_RATING: if (melee) Add(Stat::Crit, value); break;
            case ITEM_MOD_CRIT_RANGED_RATING: if (profile == Ranged) Add(Stat::Crit, value); break;
            case ITEM_MOD_CRIT_SPELL_RATING: if (spell) Add(Stat::Crit, value); break;
            case ITEM_MOD_HASTE_MELEE_RATING: if (melee) Add(Stat::Haste, value); break;
            case ITEM_MOD_HASTE_RANGED_RATING: if (profile == Ranged) Add(Stat::Haste, value); break;
            case ITEM_MOD_HASTE_SPELL_RATING: if (spell) Add(Stat::Haste, value); break;
            case ITEM_MOD_HIT_RATING: Add(Stat::Hit, value); break;
            case ITEM_MOD_CRIT_RATING: Add(Stat::Crit, value); break;
            case ITEM_MOD_HASTE_RATING: Add(Stat::Haste, value); break;
            case ITEM_MOD_RESILIENCE_RATING: Add(Stat::Resilience, value); break;
            case ITEM_MOD_EXPERTISE_RATING: Add(Stat::Expertise, value); break;
            case ITEM_MOD_ATTACK_POWER: Add(Stat::AttackPower, value); break;
            case ITEM_MOD_RANGED_ATTACK_POWER: if (profile == Ranged) Add(Stat::AttackPower, value); break;
            case ITEM_MOD_MANA_REGENERATION: Add(Stat::ManaRegen, value); break;
            case ITEM_MOD_ARMOR_PENETRATION_RATING: Add(Stat::ArmorPenetration, value); break;
            case ITEM_MOD_SPELL_POWER: Add(Stat::SpellPower, value); Add(Stat::HealPower, value); break;
            case ITEM_MOD_HEALTH_REGEN: Add(Stat::HealthRegen, value); break;
            case ITEM_MOD_SPELL_PENETRATION: Add(Stat::SpellPenetration, value); break;
            case ITEM_MOD_BLOCK_VALUE: Add(Stat::BlockValue, value); break;
            case ITEM_MOD_MASTERY_RATING: Add(Stat::Mastery, value); break;
            case ITEM_MOD_EXTRA_ARMOR: Add(Stat::Armor, value); break;
            default: UnsupportedStats = true; return false;
        }
        return true;
    }
    bool AddRating(uint32 mask, float value, Profile profile)
    {
        if (!std::isfinite(value)) { UnsupportedEffects = true; return false; }
        bool supported = true;
        for (uint32 rating = 0; rating < 32; ++rating)
        {
            if (!(mask & (uint32(1) << rating))) continue;
            int32 type = -1;
            switch (rating)
            {
                case CR_DEFENSE_SKILL: type = ITEM_MOD_DEFENSE_SKILL_RATING; break;
                case CR_DODGE: type = ITEM_MOD_DODGE_RATING; break;
                case CR_PARRY: type = ITEM_MOD_PARRY_RATING; break;
                case CR_BLOCK: type = ITEM_MOD_BLOCK_RATING; break;
                case CR_HIT_MELEE: type = ITEM_MOD_HIT_MELEE_RATING; break;
                case CR_HIT_RANGED: type = ITEM_MOD_HIT_RANGED_RATING; break;
                case CR_HIT_SPELL: type = ITEM_MOD_HIT_SPELL_RATING; break;
                case CR_CRIT_MELEE: type = ITEM_MOD_CRIT_MELEE_RATING; break;
                case CR_CRIT_RANGED: type = ITEM_MOD_CRIT_RANGED_RATING; break;
                case CR_CRIT_SPELL: type = ITEM_MOD_CRIT_SPELL_RATING; break;
                case CR_HASTE_MELEE: type = ITEM_MOD_HASTE_MELEE_RATING; break;
                case CR_HASTE_RANGED: type = ITEM_MOD_HASTE_RANGED_RATING; break;
                case CR_HASTE_SPELL: type = ITEM_MOD_HASTE_SPELL_RATING; break;
                case CR_EXPERTISE: type = ITEM_MOD_EXPERTISE_RATING; break;
                case CR_ARMOR_PENETRATION: type = ITEM_MOD_ARMOR_PENETRATION_RATING; break;
                case CR_MASTERY: type = ITEM_MOD_MASTERY_RATING; break;
                default: supported = false; continue;
            }
            supported = AddItemStat(type, value, profile) && supported;
        }
        if (!supported) UnsupportedEffects = true;
        return supported;
    }
    bool AddFlatAura(uint32 aura, int32 misc, float value, Profile profile)
    {
        if (!std::isfinite(value)) { UnsupportedEffects = true; return false; }
        bool melee = profile & (MeleeDamage | MeleeTank);
        switch (aura)
        {
            case SPELL_AURA_MOD_STAT:
                if (misc == int32(StatType::AllPrimaryStats) || misc == int32(StatType::AllPrimaryStats2))
                    for (auto stat : {Stat::Strength, Stat::Agility, Stat::Stamina, Stat::Intellect, Stat::Spirit}) Add(stat, value);
                else
                {
                    switch (misc)
                    {
                        case int32(StatType::Strength): Add(Stat::Strength, value); break;
                        case int32(StatType::Agility): Add(Stat::Agility, value); break;
                        case int32(StatType::Stamina): Add(Stat::Stamina, value); break;
                        case int32(StatType::Intellect): Add(Stat::Intellect, value); break;
                        case int32(StatType::Spirit): Add(Stat::Spirit, value); break;
                        default: UnsupportedEffects = true; return false;
                    }
                }
                break;
            case SPELL_AURA_MOD_RATING: return AddRating(uint32(misc), value, profile);
            case SPELL_AURA_MOD_ATTACK_POWER: if (melee) Add(Stat::AttackPower, value); break;
            case SPELL_AURA_MOD_RANGED_ATTACK_POWER: if (profile == Ranged) Add(Stat::AttackPower, value); break;
            case SPELL_AURA_MOD_HEALING_DONE: Add(Stat::HealPower, value); break;
            case SPELL_AURA_MOD_INCREASE_HEALTH: Add(Stat::Stamina, value / 15); break;
            case SPELL_AURA_MOD_SHIELD_BLOCKVALUE: Add(Stat::BlockValue, value); break;
            case SPELL_AURA_MOD_POWER_REGEN:
                if (misc != POWER_MANA) { UnsupportedEffects = true; return false; }
                Add(Stat::ManaRegen, value); break;
            case SPELL_AURA_MOD_RESISTANCE:
                if (misc != SPELL_SCHOOL_MASK_NORMAL) { UnsupportedEffects = true; return false; }
                Add(Stat::Armor, value); break;
            case SPELL_AURA_MOD_DAMAGE_DONE:
                if (!misc || (uint32(misc) & ~uint32(SPELL_SCHOOL_MASK_ALL)))
                { UnsupportedEffects = true; return false; }
                if (misc & SPELL_SCHOOL_MASK_NORMAL) Add(Stat::AttackPower, value);
                if ((misc & SPELL_SCHOOL_MASK_MAGIC) == SPELL_SCHOOL_MASK_MAGIC) Add(Stat::SpellPower, value);
                if ((misc & SPELL_SCHOOL_MASK_MAGIC) && (misc & SPELL_SCHOOL_MASK_MAGIC) != SPELL_SCHOOL_MASK_MAGIC)
                { UnsupportedEffects = true; return false; }
                break;
            default: UnsupportedEffects = true; return false;
        }
        return true;
    }
};
inline float AveragePoints(float base, int32 dice, bool scalingVariance = false)
{
    // Native variance has zero mean and takes precedence over the die range.
    return base + (scalingVariance || dice == 0 ? 0.0f : (1.0f + float(dice)) / 2.0f);
}
void AddContexts(SharedNamedObjectContextList<UntypedValue>& values);
}
#endif
