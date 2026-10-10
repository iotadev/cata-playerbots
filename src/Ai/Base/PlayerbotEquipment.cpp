/* GPL v2 or later. Native ownership and donor adaptations are in PORTING.md. */
#include "PlayerbotEquipment.h"
#include "PlayerbotEquipmentApply.h"
#include "PlayerbotLootAffix.h"
#include "PlayerbotStarterGearWeights.h"
#include "../../Script/PlayerbotConfig.h"
#include "../../Bot/PlayerbotAI.h"
#include "Bag.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "DBCStores.h"
#include "Player.h"
#include "WorldSession.h"
#include "WorldPacket.h"
#include "Opcodes.h"
namespace
{
bool VerifiedAffix(Item const& instance, PlayerbotItemStats::ItemQuery query)
{
    if (instance.GetEntry() != query.Item || instance.GetItemRandomPropertyId() != query.Property) return false;
    uint32 const* enchants = nullptr;
    if (query.Property > 0)
    {
        auto const* property = sItemRandomPropertiesStore.LookupEntry(PlayerbotItemStats::PropertyIdentity(query.Property));
        if (!property) return false;
        enchants = property->Enchantment;
    }
    else if (query.Property < 0)
    {
        auto const* suffix = sItemRandomSuffixStore.LookupEntry(PlayerbotItemStats::PropertyIdentity(query.Property));
        if (!suffix || instance.GetItemSuffixFactor() != GenerateEnchSuffixFactor(query.Item)) return false;
        enchants = suffix->Enchantment;
    }
    for (uint32 i = 0; i < 5; ++i)
        if (instance.GetEnchantmentId(EnchantmentSlot(PROP_ENCHANTMENT_SLOT_0 + i)) != (enchants ? enchants[i] : 0)) return false;
    for (uint32 i = 0; i < MAX_ENCHANTMENT_SLOT; ++i)
        if ((i < PROP_ENCHANTMENT_SLOT_0 || i > PROP_ENCHANTMENT_SLOT_4) && instance.GetEnchantmentId(EnchantmentSlot(i))) return false;
    return true; // Owned active affix, not hypothetical loot-pool membership.
}
std::optional<float> ReadStarterScore(Player& bot, AiObjectContext& context, std::string const& qualifier,
    Item const* instance = nullptr, bool fresh = false, PlayerbotLootRoll const* loot = nullptr)
{
    if (!PlayerbotModuleStarterGearScoreEnabled()) return {};
    auto query = PlayerbotItemStats::ParseQuery(qualifier);
    ItemTemplate const* item = query ? sObjectMgr->GetItemTemplate(query.Item) : nullptr;
    if (!item || (item->GetClass() != ITEM_CLASS_WEAPON && item->GetClass() != ITEM_CLASS_ARMOR) ||
        bot.CanUseItem(item) != EQUIP_ERR_OK) return {};
    auto model = PlayerbotEquipment::StarterModel(bot.getClass(), bot.GetPrimaryTalentTree(bot.GetActiveSpec()), bot.getLevel());
    if (!model.Qualified || ((bot.getClass() == CLASS_MAGE || bot.getClass() == CLASS_PRIEST) &&
        bot.GetStat(StatType::Intellect) <= 10)) return {};
    auto* value = context.GetValue<PlayerbotItemStats::BaseStats>("item base stats", qualifier);
    if (!value) return {};
    if (fresh) value->Reset();
    auto stats = value->Get();
    if (stats.OwnerLevel != bot.getLevel()) return {};
    if (loot)
    {
        if (instance) return {};
        auto verified = PlayerbotEquipment::LootQuery(*loot, item->GetRandomProperty(), item->GetRandomSuffix(),
            GenerateEnchSuffixFactor(query.Item));
        if (!verified || verified.Item != query.Item || verified.Property != query.Property) return {};
        stats.AffixLootVerified = true; // Never written into the cached template value.
    }
    if (instance)
    {
        if (instance->GetOwnerGUID() != bot.GetGUID() || !VerifiedAffix(*instance, query)) return {};
        stats.AffixInstanceVerified = true;
        if (query.Property == 0) stats.AffixResolved = true; // Instance proves no active affix.
    }
    auto score = PlayerbotEquipment::Score(stats, model);
    if (score && item->GetClass() == ITEM_CLASS_WEAPON)
        *score *= PlayerbotEquipment::WeaponMultiplier(bot.getClass(), model.Spec, item->GetInventoryType(), item->GetSubClass(),
            bot.CanDualWield(), bot.CanTitanGrip());
    return score;
}
std::optional<float> ReadOwnedScore(Player& bot, AiObjectContext& context, Item const& item, bool fresh = false)
{
    std::string query = std::to_string(item.GetEntry());
    if (item.GetItemRandomPropertyId()) query += "," + std::to_string(item.GetItemRandomPropertyId());
    return ReadStarterScore(bot, context, query, &item, fresh);
}
class StarterItemScoreValue final : public CalculatedValue<std::optional<float>>, public Qualified
{
public:
    explicit StarterItemScoreValue(PlayerbotAI* ai) : CalculatedValue(ai, "starter item score") { }
protected:
    std::optional<float> Calculate() override
    {
        if (!PlayerbotModuleStarterGearScoreEnabled()) return {};
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        AiObjectContext* context = botAI ? botAI->GetAiObjectContext() : nullptr;
        if (!bot || !context || !bot->IsInWorld() || !bot->IsAlive() || bot->IsBeingTeleported() ||
            !PlayerbotItemStats::ParseQuery(qualifier)) return {};
        return ReadStarterScore(*bot, *context, qualifier);
    }
};
PlayerbotEquipment::Survey CollectUnowned(PlayerbotAI* ai, std::string const& qualifier, PlayerbotLootRoll const* loot = nullptr)
{
    using namespace PlayerbotEquipment;
    Survey result;
    Player* bot = ai ? ai->GetBot() : nullptr;
    AiObjectContext* context = ai ? ai->GetAiObjectContext() : nullptr;
    auto query = PlayerbotItemStats::ParseQuery(qualifier);
    ItemTemplate const* item = query ? sObjectMgr->GetItemTemplate(query.Item) : nullptr;
    if (!PlayerbotModuleStarterGearScoreEnabled() || !bot || !context || !item ||
        !bot->IsInWorld() || !bot->IsAlive() || bot->IsBeingTeleported()) return result;
    if (!loot && !TemplateComparable(item->GetRandomProperty(), item->GetRandomSuffix(), query.Property)) return result;
    auto score = ReadStarterScore(*bot, *context, qualifier, nullptr, loot != nullptr, loot);
    if (!score) return result;
    result.Owner = bot->GetGUID();
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        if (bot->FindEquipSlot(item, slot, true) != slot) continue;
        uint16 destination = 0;
        // Native dry-run creates/deletes a transient Item. It does not store
        // or save that Item, and supplies native unique/skill/slot checks.
        if (bot->CanEquipNewItem(slot, destination, query.Item, true) != EQUIP_ERR_OK ||
            destination != uint16(uint16(INVENTORY_SLOT_BAG_0) << 8 | slot)) continue;
        Item* existing = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        ItemTemplate const* oldTemplate = existing ? existing->GetTemplate() : nullptr;
        if (existing && !oldTemplate) continue;
        Evaluation row;
        // Item stays empty: this is NOT an owned inventory candidate and
        // cannot be passed to ApplyOne as permission to equip anything.
        row.Input.Entry = query.Item;
        row.Input.Property = query.Property;
        row.Input.SuffixFactor = loot ? loot->SuffixFactor : 0;
        row.Input.Slot = slot;
        row.Input.Existing = existing ? existing->GetGUID() : ObjectGuid::Empty;
        row.Input.ExistingEntry = existing ? existing->GetEntry() : 0;
        row.Input.ExistingBroken = existing && existing->IsBroken();
        bool twoHand = item->GetInventoryType() == INVTYPE_2HWEAPON;
        row.LayoutKnown = LayoutComparable(slot, twoHand,
            oldTemplate ? oldTemplate->GetInventoryType() == INVTYPE_2HWEAPON : twoHand,
            bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND) != nullptr);
        row.CandidateScore = score;
        row.ExistingScore = existing ? ReadOwnedScore(*bot, *context, *existing, loot != nullptr) : std::optional<float>(0);
        row.Result = Evaluate(row.CandidateScore, row.ExistingScore, !existing,
            existing && SameVariant(query.Item, query.Property, row.Input.SuffixFactor, existing->GetEntry(),
                existing->GetItemRandomPropertyId(), existing->GetItemSuffixFactor()), false, row.Input.ExistingBroken, row.LayoutKnown);
        result.Items.push_back(row);
    }
    result.Available = true;
    return result;
}
class TemplateEquipmentSurveyValue final : public CalculatedValue<PlayerbotEquipment::Survey>, public Qualified
{
public:
    explicit TemplateEquipmentSurveyValue(PlayerbotAI* ai) : CalculatedValue(ai, "template equipment comparisons") { }
protected:
    PlayerbotEquipment::Survey Calculate() override { return CollectUnowned(botAI, qualifier); }
};
class EquipmentCandidatesValue final : public CalculatedValue<PlayerbotEquipment::Snapshot>
{
public:
    explicit EquipmentCandidatesValue(PlayerbotAI* ai) : CalculatedValue(ai, "equipment candidates") { }
protected:
    PlayerbotEquipment::Snapshot Calculate() override
    {
        using namespace PlayerbotEquipment;
        Snapshot result;
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        if (!bot || !bot->IsInWorld() || !bot->IsAlive() || bot->IsBeingTeleported()) return result;
        result.Owner = bot->GetGUID();
        result.Class = bot->getClass();
        result.Level = bot->getLevel();
        result.Spec = bot->GetPrimaryTalentTree(bot->GetActiveSpec());
        auto consider = [&](Item* item)
        {
            if (!item || item->GetOwnerGUID() != bot->GetGUID()) return;
            ItemTemplate const* proto = item->GetTemplate();
            if (!proto || (proto->GetClass() != ITEM_CLASS_WEAPON && proto->GetClass() != ITEM_CLASS_ARMOR)) return;
            for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
            {
                uint16 destination = 0;
                // Real owned instance, native swap/unique/skill/combat/cast checks.
                // A successful dry run is not authorization for a later mutation.
                if (bot->CanEquipItem(slot, destination, item, true, true) != EQUIP_ERR_OK ||
                    destination != uint16(uint16(INVENTORY_SLOT_BAG_0) << 8 | slot)) continue;
                Item* existing = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
                Candidate candidate;
                candidate.Item = item->GetGUID();
                candidate.Entry = item->GetEntry();
                candidate.Property = item->GetItemRandomPropertyId();
                candidate.SuffixFactor = item->GetItemSuffixFactor();
                candidate.Slot = slot;
                candidate.Broken = item->IsBroken();
                if (existing)
                {
                    candidate.Existing = existing->GetGUID();
                    candidate.ExistingEntry = existing->GetEntry();
                    candidate.ExistingBroken = existing->IsBroken();
                }
                result.Candidates.push_back(candidate);
            }
        };
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
            consider(bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot));
        for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_BAG_END; ++slot)
            if (Bag* bag = bot->GetBagByPos(slot))
                for (uint32 bagSlot = 0; bagSlot < bag->GetBagSize(); ++bagSlot)
                    consider(bag->GetItemByPos(bagSlot));
        result.Available = true;
        return result;
    }
};
class EquipmentSurveyValue final : public CalculatedValue<PlayerbotEquipment::Survey>
{
public:
    explicit EquipmentSurveyValue(PlayerbotAI* ai) : CalculatedValue(ai, "starter equipment comparisons") { }
    PlayerbotEquipment::Survey Collect(bool fresh)
    {
        using namespace PlayerbotEquipment;
        Survey result;
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        AiObjectContext* context = botAI ? botAI->GetAiObjectContext() : nullptr;
        if (!PlayerbotModuleStarterGearScoreEnabled() || !bot || !context || !bot->IsInWorld() ||
            !bot->IsAlive() || bot->IsBeingTeleported()) return result;
        auto model = StarterModel(bot->getClass(), bot->GetPrimaryTalentTree(bot->GetActiveSpec()), bot->getLevel());
        auto* candidates = context->GetValue<Snapshot>("equipment candidates");
        if (!model.Qualified || !candidates) return result;
        if (fresh) candidates->Reset();
        auto snapshot = candidates->Get();
        if (!snapshot.Available || snapshot.Owner != bot->GetGUID() || snapshot.Class != bot->getClass() ||
            snapshot.Level != bot->getLevel() || snapshot.Spec != model.Spec) return result;
        result.Owner = bot->GetGUID();
        for (auto const& entry : snapshot.Candidates)
        {
            Item* item = bot->GetItemByGuid(entry.Item);
            if (!item || item->GetOwnerGUID() != bot->GetGUID()) continue;
            bool carried = item->GetBagSlot() == INVENTORY_SLOT_BAG_0 ?
                item->GetSlot() >= INVENTORY_SLOT_ITEM_START && item->GetSlot() < INVENTORY_SLOT_ITEM_END :
                item->GetBagSlot() >= INVENTORY_SLOT_BAG_START && item->GetBagSlot() < INVENTORY_SLOT_BAG_END;
            uint16 destination = 0;
            if (!carried || bot->CanEquipItem(entry.Slot, destination, item, true, true) != EQUIP_ERR_OK ||
                destination != uint16(uint16(INVENTORY_SLOT_BAG_0) << 8 | entry.Slot)) continue;
            Item* existing = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, entry.Slot);
            ItemTemplate const* oldTemplate = existing ? existing->GetTemplate() : nullptr;
            if (existing && !oldTemplate) continue;
            Evaluation evaluation;
            evaluation.Input = entry;
            evaluation.Input.Entry = item->GetEntry();
            evaluation.Input.Property = item->GetItemRandomPropertyId();
            evaluation.Input.SuffixFactor = item->GetItemSuffixFactor();
            evaluation.Input.Broken = item->IsBroken();
            evaluation.Input.Existing = existing ? existing->GetGUID() : ObjectGuid::Empty;
            evaluation.Input.ExistingEntry = existing ? existing->GetEntry() : 0;
            evaluation.Input.ExistingBroken = existing && existing->IsBroken();
            bool twoHand = item->GetTemplate()->GetInventoryType() == INVTYPE_2HWEAPON;
            bool oldTwoHand = oldTemplate ? oldTemplate->GetInventoryType() == INVTYPE_2HWEAPON : twoHand;
            evaluation.LayoutKnown = LayoutComparable(entry.Slot, twoHand, oldTwoHand,
                bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND) != nullptr);
            evaluation.CandidateScore = ReadOwnedScore(*bot, *context, *item, fresh);
            evaluation.ExistingScore = existing ? ReadOwnedScore(*bot, *context, *existing, fresh) : std::optional<float>(0);
            evaluation.Result = Evaluate(evaluation.CandidateScore, evaluation.ExistingScore, !existing,
                existing && SameVariant(item->GetEntry(), item->GetItemRandomPropertyId(), item->GetItemSuffixFactor(),
                    existing->GetEntry(), existing->GetItemRandomPropertyId(), existing->GetItemSuffixFactor()),
                item->IsBroken(), existing && existing->IsBroken(), evaluation.LayoutKnown);
            result.Items.push_back(evaluation);
        }
        result.Available = true; // Read-only classification, never a queued equip request.
        return result;
    }
protected:
    PlayerbotEquipment::Survey Calculate() override { return Collect(false); }
};
}
PlayerbotEquipment::Survey PlayerbotEquipment::CompareLoot(PlayerbotAI& ai, PlayerbotLootRoll const& roll)
{
    Player* bot = ai.GetBot();
    ItemTemplate const* item = sObjectMgr->GetItemTemplate(roll.Entry);
    if (!bot || !item || bot->GetMapId() != roll.Map || bot->GetInstanceId() != roll.Instance) return {};
    auto query = LootQuery(roll, item->GetRandomProperty(), item->GetRandomSuffix(), GenerateEnchSuffixFactor(roll.Entry));
    if (!query) return {};
    std::string qualifier = std::to_string(query.Item);
    if (query.Property) qualifier += "," + std::to_string(query.Property);
    return CollectUnowned(&ai, qualifier, &roll);
}
PlayerbotEquipmentApply::Result PlayerbotEquipmentApply::ApplyOne(WorldSession& session, PlayerbotAI& ai)
{
    Player* bot = ai.GetBot();
    if (!PlayerbotModuleStarterEquipEnabled() || !PlayerbotModuleStarterGearScoreEnabled() || !bot ||
        session.GetPlayer() != bot || !bot->IsInWorld() || !bot->IsAlive() || bot->IsBeingTeleported() ||
        bot->IsInCombat() || bot->IsNonMeleeSpellCast(false)) return Result::Unavailable;
    EquipmentSurveyValue reader(&ai);
    auto survey = reader.Collect(true); // Invalidate only calculated inventory/score dependencies.
    if (!survey.Available || survey.Owner != bot->GetGUID()) return Result::Unavailable;
    for (auto const& row : survey.Items)
    {
        if (!Actionable(row.Result) || !row.LayoutKnown) continue;
        Item* item = bot->GetItemByGuid(row.Input.Item);
        Item* old = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, row.Input.Slot);
        if (!item || item->GetOwnerGUID() != bot->GetGUID() || item->IsBroken() ||
            (old ? old->GetGUID() : ObjectGuid::Empty) != row.Input.Existing ||
            item->GetEntry() != row.Input.Entry || item->GetItemRandomPropertyId() != row.Input.Property ||
            item->GetItemSuffixFactor() != row.Input.SuffixFactor) return Result::Rejected;
        bool carried = item->GetBagSlot() == INVENTORY_SLOT_BAG_0 ?
            item->GetSlot() >= INVENTORY_SLOT_ITEM_START && item->GetSlot() < INVENTORY_SLOT_ITEM_END :
            item->GetBagSlot() >= INVENTORY_SLOT_BAG_START && item->GetBagSlot() < INVENTORY_SLOT_BAG_END;
        if (!carried) return Result::Rejected;
        uint16 destination = 0;
        if (bot->CanEquipItem(row.Input.Slot, destination, item, true, true) != EQUIP_ERR_OK ||
            destination != uint16(uint16(INVENTORY_SLOT_BAG_0) << 8 | row.Input.Slot)) return Result::Rejected;
        WorldPacket packet(CMSG_AUTOEQUIP_ITEM_SLOT, 9);
        packet << item->GetGUID() << row.Input.Slot;
        session.HandleAutoEquipItemSlotOpcode(packet); // PROCESS_INPLACE, permitted in native map filtering.
        Item* equipped = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, row.Input.Slot);
        if (!equipped || equipped->GetGUID() != row.Input.Item) return Result::Rejected;
        if (auto* value = ai.GetAiObjectContext()->GetValue<PlayerbotEquipment::Snapshot>("equipment candidates")) value->Reset();
        if (auto* value = ai.GetAiObjectContext()->GetValue<PlayerbotEquipment::Survey>("starter equipment comparisons")) value->Reset();
        return Result::Confirmed; // At most one native change per explicit request.
    }
    return Result::NoChange;
}
void PlayerbotEquipment::AddContexts(SharedNamedObjectContextList<UntypedValue>& values)
{
    auto* factory = new NamedObjectContext<UntypedValue>();
    factory->creators["equipment candidates"] = [](PlayerbotAI* ai) { return new EquipmentCandidatesValue(ai); };
    factory->creators["starter item score"] = [](PlayerbotAI* ai) { return new StarterItemScoreValue(ai); };
    factory->creators["starter equipment comparisons"] = [](PlayerbotAI* ai) { return new EquipmentSurveyValue(ai); };
    factory->creators["template equipment comparisons"] = [](PlayerbotAI* ai) { return new TemplateEquipmentSurveyValue(ai); };
    values.Add(factory);
}
