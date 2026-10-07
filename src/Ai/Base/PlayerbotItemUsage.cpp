/* GPL v2 or later. Donor provenance and Cata adaptations are in PORTING.md. */
#include "PlayerbotItemUsage.h"
#include "../../Bot/PlayerbotAI.h"
#include "ObjectMgr.h"
#include "Player.h"
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
            return value ? Fact{Consumable(value->Get()), Scope::ConsumableStock} : Fact{};
        }
        if (item->GetClass() == ITEM_CLASS_ARMOR || item->GetClass() == ITEM_CLASS_WEAPON)
        {
            auto* value = context->GetValue<PlayerbotEquipment::Survey>("starter equipment comparisons");
            if (!value) return {};
            auto survey = value->Get();
            if (!survey.Available || survey.Owner != bot->GetGUID()) return {};
            return {CarriedEquipment(survey, query.Item, query.Property), Scope::CarriedEquipment};
        }
        // Quest/master synchronization, professions, tokens, bags, auction,
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
}
