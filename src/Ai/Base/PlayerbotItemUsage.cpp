/* GPL v2 or later. Donor provenance and Cata adaptations are in PORTING.md. */
#include "PlayerbotItemUsage.h"
#include "PlayerbotQuestItem.h"
#include "PlayerbotQuestReward.h"
#include "../../Bot/PlayerbotAI.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

bool PlayerbotQuestItem::Useful(Player const& player, ItemTemplate const& item)
{
    // Preserve the donor's native predicate first: this includes source-item
    // requirements and native quest-state/count rules, not just direct objectives.
    if (player.HasQuestForItem(item.GetId())) return true;
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = player.GetQuestSlotQuestId(slot);
        Quest const* quest = questId ? sObjectMgr->GetQuestTemplate(questId) : nullptr;
        if (!quest) continue;
        auto needed = [&](uint32 candidate)
        {
            if (!candidate) return false;
            for (uint8 objective = 0; objective < QUEST_ITEM_OBJECTIVES_COUNT; ++objective)
                if (quest->RequiredItemId[objective] == candidate &&
                    Outstanding(quest->RequiredItemId[objective], quest->RequiredItemCount[objective],
                    candidate, player.GetItemCount(candidate, false))) return true;
            return false;
        };
        // As upstream, keep an inventory-based fallback (including raid-group
        // usefulness), without bypassing the core's actual loot gating.
        if (needed(item.GetId())) return true;
        for (ItemEffect const& effect : item.Effects)
        {
            SpellInfo const* spell = effect.SpellID ? sSpellMgr->GetSpellInfo(effect.SpellID) : nullptr;
            if (!spell) continue;
            for (auto const& spellEffect : spell->Effects)
                if (spellEffect.Effect == SPELL_EFFECT_CREATE_ITEM && needed(spellEffect.ItemType)) return true;
        }
    }
    return false;
}
namespace
{
class ItemUsageValue final : public CalculatedValue<PlayerbotItemUsage::Fact>, public Qualified
{
public:
    explicit ItemUsageValue(PlayerbotAI* ai) : CalculatedValue(ai, "item usage") { }
protected:
    PlayerbotItemUsage::Fact Calculate() override
    {
        using namespace PlayerbotItemUsage;
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        AiObjectContext* context = botAI ? botAI->GetAiObjectContext() : nullptr;
        auto query = PlayerbotItemStats::ParseQuery(qualifier);
        ItemTemplate const* item = query ? sObjectMgr->GetItemTemplate(query.Item) : nullptr;
        if (!bot || !context || !item || !bot->IsInWorld() || bot->IsBeingTeleported()) return {};
        if (item->GetClass() == ITEM_CLASS_CONSUMABLE)
        {
            if (query.Property) return {}; // Stock reader does not qualify affixed consumables.
            auto* value = context->GetValue<PlayerbotConsumable::Usage>("consumable usage", std::to_string(query.Item));
            Fact stock = value ? Fact{Consumable(value->Get()), Scope::ConsumableStock} : Fact{};
            if (stock.Result == Usage::Use || stock.Result == Usage::Keep) return stock;
            return ConsumableQuestFallback(stock, PlayerbotQuestItem::Useful(*bot, *item));
        }
        if (item->GetClass() == ITEM_CLASS_ARMOR || item->GetClass() == ITEM_CLASS_WEAPON)
        {
            auto* value = context->GetValue<PlayerbotEquipment::Survey>("starter equipment comparisons");
            if (!value) return {};
            auto survey = value->Get();
            if (!survey.Available || survey.Owner != bot->GetGUID()) return {};
            return {CarriedEquipment(survey, query.Item, query.Property), Scope::CarriedEquipment};
        }
        if (!query.Property && PlayerbotQuestItem::Useful(*bot, *item))
            return {Usage::Quest, Scope::QuestLog};
        // Master synchronization, professions, tokens, bags, auction,
        // disenchant and vendor branches are not ported; never invent None.
        return {};
    }
};
class TemplateItemUsageValue final : public CalculatedValue<PlayerbotItemUsage::Fact>, public Qualified
{
public:
    explicit TemplateItemUsageValue(PlayerbotAI* ai) : CalculatedValue(ai, "template item usage") { }
protected:
    PlayerbotItemUsage::Fact Calculate() override
    {
        using namespace PlayerbotItemUsage;
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        AiObjectContext* context = botAI ? botAI->GetAiObjectContext() : nullptr;
        auto query = PlayerbotItemStats::ParseQuery(qualifier);
        if (!bot || !context || !query || !bot->IsInWorld() || bot->IsBeingTeleported()) return {};
        auto* value = context->GetValue<PlayerbotEquipment::Survey>("template equipment comparisons", qualifier);
        if (!value) return {};
        auto survey = value->Get();
        if (!survey.Available || survey.Owner != bot->GetGUID()) return {};
        return {SurveyUsage(survey, query.Item, query.Property), Scope::TemplateEquipment};
    }
};
}
void PlayerbotItemUsage::AddContexts(SharedNamedObjectContextList<UntypedValue>& values)
{
    auto* factory = new NamedObjectContext<UntypedValue>();
    factory->creators["item usage"] = [](PlayerbotAI* ai) { return new ItemUsageValue(ai); };
    factory->creators["template item usage"] = [](PlayerbotAI* ai) { return new TemplateItemUsageValue(ai); };
    values.Add(factory);
    PlayerbotQuestReward::AddContexts(values);
}
