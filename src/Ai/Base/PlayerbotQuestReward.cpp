/* GPL v2 or later. Read-only donor reward-choice dependency; see PORTING.md. */
#include "PlayerbotQuestReward.h"
#include "../../Bot/PlayerbotAI.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
namespace
{
class QuestRewardValue final : public CalculatedValue<PlayerbotQuestReward::Survey>, public Qualified
{
public:
    explicit QuestRewardValue(PlayerbotAI* ai) : CalculatedValue(ai, "quest reward choices") { }
private:
    PlayerbotQuestReward::Survey Calculate() override
    {
        using namespace PlayerbotQuestReward;
        static_assert(QUEST_REWARD_CHOICES_COUNT == 6);
        Survey result;
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        AiObjectContext* context = botAI ? botAI->GetAiObjectContext() : nullptr;
        uint32 id = PlayerbotConsumable::ParseItem(qualifier);
        Quest const* quest = id ? sObjectMgr->GetQuestTemplate(id) : nullptr;
        if (!bot || !context || !quest || !bot->IsInWorld() || !bot->IsAlive() || bot->IsBeingTeleported()) return result;
        result.Owner = bot->GetGUID(); result.Quest = id;
        for (uint8 slot = 0; slot < QUEST_REWARD_CHOICES_COUNT; ++slot)
        {
            uint32 entry = quest->RewardChoiceItemId[slot], count = quest->RewardChoiceItemCount[slot];
            if (!entry && !count) continue;
            ItemTemplate const* item = entry ? sObjectMgr->GetItemTemplate(entry) : nullptr;
            if (!item || !count) return {}; // Malformed reward rows are not omitted silently.
            Choice choice; choice.Slot = slot; choice.Entry = entry; choice.Count = count;
            std::string query = std::to_string(entry);
            if (item->GetClass() == ITEM_CLASS_ARMOR || item->GetClass() == ITEM_CLASS_WEAPON)
            {
                auto* value = context->GetValue<PlayerbotEquipment::Survey>("template equipment comparisons", query);
                if (value)
                {
                    auto survey = value->Get();
                    if (survey.Available && survey.Owner == result.Owner)
                    {
                        choice.Usage = PlayerbotItemUsage::SurveyUsage(survey, entry, 0);
                        // Item-level score is independent of the destination slot.
                        for (auto const& row : survey.Items)
                            if (row.Input.Entry == entry && !row.Input.Property && row.CandidateScore)
                            { choice.Score = row.CandidateScore; break; }
                    }
                }
            }
            else if (auto* value = context->GetValue<PlayerbotItemUsage::Fact>("item usage", query))
                choice.Usage = value->Get().Result;
            result.Choices.push_back(choice);
        }
        result.Available = true;
        result.Ranked = Select(result.Choices);
        return result;
    }
};
}
void PlayerbotQuestReward::AddContexts(SharedNamedObjectContextList<UntypedValue>& values)
{
    auto* factory = new NamedObjectContext<UntypedValue>();
    factory->creators["quest reward choices"] = [](PlayerbotAI* ai) { return new QuestRewardValue(ai); };
    values.Add(factory);
}
