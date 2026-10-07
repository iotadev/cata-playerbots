/* GPL v2 or later. Donor provenance and Cata adaptations are in PORTING.md. */
#include "PlayerbotItemStats.h"
#include "PlayerbotConsumableUsage.h"
#include "PlayerbotRoles.h"
#include "../../Bot/PlayerbotAI.h"
#include "DBCStores.h"
#include "ObjectMgr.h"
#include "Item.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
namespace
{
using namespace PlayerbotItemStats;
Profile ProfileFor(Player const& bot)
{
    uint32 roles = PlayerbotRoles::Mask(bot);
    bool caster = (roles & STRATEGY_TYPE_RANGED) && bot.getClass() != CLASS_HUNTER;
    return ChooseProfile(roles & STRATEGY_TYPE_HEAL, caster, roles & STRATEGY_TYPE_TANK, roles & STRATEGY_TYPE_MELEE);
}
void CollectFlatSpell(BaseStats& stats, uint32 id, Player const& bot, Profile profile)
{
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(id);
    if (!spell) { stats.UnsupportedEffects = true; return; }
    if (spell->ProcFlags) { stats.HasProcEffects = true; return; }
    if (spell->SpellFamilyName || spell->Stances || spell->StancesNot || spell->StackAmount > 1 ||
        spell->CasterAuraState || spell->CasterAuraStateNot || spell->CasterAuraSpell || spell->ExcludeCasterAuraSpell ||
        spell->HasAttribute(SPELL_ATTR1_FINISHING_MOVE_DAMAGE) || spell->HasAttribute(SPELL_ATTR8_MASTERY_AFFECTS_POINTS) ||
        spell->HasAttribute(SPELL_ATTR8_USE_TARGETS_LEVEL_FOR_SPELL_SCALING))
    { stats.HasConditionalEffects = true; return; }
    for (auto const& effect : spell->Effects)
    {
        if (!effect.Effect) continue;
        if (effect.Effect != SPELL_EFFECT_APPLY_AURA) { stats.UnsupportedEffects = true; continue; }
        if (effect.TriggerSpell) { stats.HasProcEffects = true; continue; }
        float base = effect.CalcBaseValue(&bot, nullptr);
        if (effect.Scaling.Coefficient == 0.0f && effect.RealPointsPerLevel != 0)
        {
            int32 level = bot.getLevel();
            if (spell->MaxLevel) level = std::min(level, int32(spell->MaxLevel));
            level = std::max(0, level - int32(std::max(spell->BaseLevel, spell->SpellLevel)));
            base += level * effect.RealPointsPerLevel;
        }
        stats.AddFlatAura(effect.ApplyAuraName, effect.MiscValue,
            AveragePoints(base, effect.DieSides, effect.Scaling.Variance != 0.0f), profile);
    }
}
void CollectEnchant(BaseStats& stats, SpellItemEnchantmentEntry const& enchant, Player const& bot, Profile profile,
    std::optional<uint32> suffixAmount = {})
{
    if (enchant.Condition_ID || enchant.RequiredSkillID || bot.getLevel() < enchant.MinLevel)
    { stats.HasConditionalEffects = true; return; }
    for (uint32 i = 0; i < MAX_ITEM_ENCHANTMENT_EFFECTS; ++i)
    {
        switch (enchant.Effect[i])
        {
            case ITEM_ENCHANTMENT_TYPE_NONE: break;
            case ITEM_ENCHANTMENT_TYPE_STAT:
                if (!enchant.EffectPointsMin[i] && !suffixAmount) stats.UnsupportedEffects = true;
                else stats.AddItemStat(enchant.EffectArg[i], enchant.EffectPointsMin[i] ? enchant.EffectPointsMin[i] : *suffixAmount, profile);
                break;
            case ITEM_ENCHANTMENT_TYPE_EQUIP_SPELL:
                stats.HasItemEffects = true;
                CollectFlatSpell(stats, enchant.EffectArg[i], bot, profile); break;
            case ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL: stats.HasProcEffects = true; break;
            case ITEM_ENCHANTMENT_TYPE_USE_SPELL: stats.HasUseEffects = true; break;
            default: stats.UnsupportedEffects = true; break;
        }
    }
}
void CollectAffix(BaseStats& stats, ItemTemplate const& item, int32 property, Player const& bot, Profile profile)
{
    if (!property) return; // A random template without a supplied affix stays unresolved.
    stats.AffixSupplied = true;
    stats.AffixPoolUnverified = true; // Native instance/pool membership must be established by later callers.
    stats.AffixResolved = true;
    auto collect = [&](uint32 enchantId, std::optional<uint32> amount = {})
    {
        if (!enchantId) return;
        auto const* enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId);
        if (!enchant) { stats.AffixResolved = false; stats.UnsupportedEffects = true; return; }
        CollectEnchant(stats, *enchant, bot, profile, amount);
    };
    if (property > 0)
    {
        auto const* affix = item.GetRandomProperty() ? sItemRandomPropertiesStore.LookupEntry(PropertyIdentity(property)) : nullptr;
        if (!affix) { stats.AffixResolved = false; stats.UnsupportedEffects = true; return; }
        for (uint32 enchant : affix->Enchantment) collect(enchant);
    }
    else
    {
        auto const* affix = item.GetRandomSuffix() ? sItemRandomSuffixStore.LookupEntry(PropertyIdentity(property)) : nullptr;
        uint32 factor = GenerateEnchSuffixFactor(item.GetId());
        if (!affix || !factor) { stats.AffixResolved = false; stats.UnsupportedEffects = true; return; }
        // Match each of the five native affix slots to its own allocation.
        for (size_t i = 0; i < std::size(affix->Enchantment); ++i)
        {
            auto amount = SuffixAmount(affix->AllocationPct[i], factor);
            if (!amount) { stats.AffixResolved = false; stats.UnsupportedEffects = true; continue; }
            collect(affix->Enchantment[i], amount);
        }
    }
}
class ItemBaseStatsValue final : public CalculatedValue<PlayerbotItemStats::BaseStats>, public Qualified
{
public:
    explicit ItemBaseStatsValue(PlayerbotAI* ai) : CalculatedValue(ai, "item base stats") { }
protected:
    PlayerbotItemStats::BaseStats Calculate() override
    {
        using namespace PlayerbotItemStats;
        BaseStats result;
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        ItemQuery query = ParseQuery(qualifier);
        ItemTemplate const* item = query ? sObjectMgr->GetItemTemplate(query.Item) : nullptr;
        if (!bot || !bot->IsInWorld() || bot->IsBeingTeleported() || !item) return result;
        Profile profile = ProfileFor(*bot);
        result.OwnerClass = bot->getClass();
        result.OwnerLevel = bot->getLevel();
        result.OwnerSpec = bot->GetPrimaryTalentTree(bot->GetActiveSpec());
        result.OwnerProfile = profile;
        // Same stat-ID fallback and signed values as native Player::_ApplyItemBonuses.
        auto const* scaling = item->GetScalingStatDistribution() ?
            sScalingStatDistributionStore.LookupEntry(item->GetScalingStatDistribution()) : nullptr;
        for (uint32 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
        {
            int32 type = item->GetItemStatType(i);
            if (type < 0 && scaling) type = scaling->StatID[i];
            if (type >= 0)
                result.AddItemStat(type, int32(item->GetStatValue(i, bot)), profile);
        }
        result.Add(Stat::Armor, item->GetEffectiveArmor(bot));
        float minimum = 0, maximum = 0, dps = 0;
        if (item->GetWeaponDamage(bot, minimum, maximum, dps) && std::isfinite(dps) && dps >= 0)
            result.Add(WeaponChannel(item->GetInventoryType()), dps);
        result.HasItemEffects = !item->Effects.empty();
        for (ItemEffect const& effect : item->Effects)
        {
            if (!effect.SpellID) continue;
            if (effect.Trigger == ITEM_SPELLTRIGGER_ON_EQUIP) CollectFlatSpell(result, effect.SpellID, *bot, profile);
            else if (effect.Trigger == ITEM_SPELLTRIGGER_ON_USE) result.HasUseEffects = true;
            else if (effect.Trigger == ITEM_SPELLTRIGGER_CHANCE_ON_HIT) result.HasProcEffects = true;
            else result.UnsupportedEffects = true;
        }
        result.HasRandomProperties = item->GetRandomProperty() || item->GetRandomSuffix();
        CollectAffix(result, *item, query.Property, *bot, profile);
        result.HasSockets = item->GetSocketBonus() != 0;
        result.SocketBonus = item->GetSocketBonus();
        for (uint32 i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i)
        {
            result.HasSockets = result.HasSockets || item->GetSocketColor(i) != SocketColor(0);
            if (item->GetSocketColor(i) != SocketColor(0)) ++result.SocketCount;
        }
        result.SocketHeuristic = *SocketMultiplier(result.SocketCount);
        result.HasItemSet = item->GetItemSet() != 0;
        if (result.HasItemSet)
        {
            auto const* set = sItemSetStore.LookupEntry(item->GetItemSet());
            bool found = false;
            if (set)
            {
                result.SetMetadataKnown = true;
                for (uint32 threshold : set->SetThreshold)
                    result.MaximumSetThreshold = std::max(result.MaximumSetThreshold, threshold);
                for (auto const& equipped : bot->ItemSetEff)
                    if (equipped && equipped->setid == item->GetItemSet())
                    { found = true; result.EquippedSetPieces = equipped->item_count; break; }
                if (set->RequiredSkill) result.HasConditionalEffects = true;
                result.SetHeuristic = SetMultiplier(true, found, result.EquippedSetPieces, result.MaximumSetThreshold);
            }
        }
        result.Available = true; // Base facts only, not a score or native equip permission.
        return result;
    }
};
class EnchantBaseStatsValue final : public CalculatedValue<BaseStats>, public Qualified
{
public:
    explicit EnchantBaseStatsValue(PlayerbotAI* ai) : CalculatedValue(ai, "enchant base stats") { }
protected:
    BaseStats Calculate() override
    {
        BaseStats result;
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        uint32 id = PlayerbotConsumable::ParseItem(qualifier);
        auto const* enchant = id ? sSpellItemEnchantmentStore.LookupEntry(id) : nullptr;
        if (!bot || !bot->IsInWorld() || bot->IsBeingTeleported() || !enchant) return result;
        CollectEnchant(result, *enchant, *bot, ProfileFor(*bot));
        result.OwnerClass = bot->getClass();
        result.OwnerLevel = bot->getLevel();
        result.OwnerSpec = bot->GetPrimaryTalentTree(bot->GetActiveSpec());
        result.OwnerProfile = ProfileFor(*bot);
        result.Available = true;
        return result;
    }
};
}
void PlayerbotItemStats::AddContexts(SharedNamedObjectContextList<UntypedValue>& values)
{
    auto* factory = new NamedObjectContext<UntypedValue>();
    factory->creators["item base stats"] = [](PlayerbotAI* ai) { return new ItemBaseStatsValue(ai); };
    factory->creators["enchant base stats"] = [](PlayerbotAI* ai) { return new EnchantBaseStatsValue(ai); };
    values.Add(factory);
}
