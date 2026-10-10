/* Adapted from TalkToQuestGiverAction::BestRewards/RewardMultipleItem at
 * 037c01418b5d01506917a3db9b44fd56ac5f965c. GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_QUEST_REWARD_H
#define PLAYERBOT_QUEST_REWARD_H
#include "PlayerbotItemUsage.h"
namespace PlayerbotQuestReward
{
inline std::optional<uint8> ResolveItem(std::array<uint32, 6> const& entries,
    std::array<uint32, 6> const& counts, uint32 selected)
{
    std::optional<uint8> found;
    bool any = false;
    for (uint8 slot = 0; slot < entries.size(); ++slot)
    {
        if (bool(entries[slot]) != bool(counts[slot])) return {};
        any |= entries[slot] != 0;
        if (selected && entries[slot] == selected)
        {
            if (found) return {}; // Duplicate entries can have different quantities.
            found = slot;
        }
    }
    if (!selected) return !any ? std::optional<uint8>(0) : std::nullopt;
    return found;
}
// Donor zero/single-choice reward branches; never rank several rewards here.
inline std::optional<uint8> UnambiguousSlot(std::array<uint32, 6> const& entries,
    std::array<uint32, 6> const& counts)
{
    uint32 entry = 0;
    for (uint8 slot = 0; slot < entries.size(); ++slot)
    {
        if (bool(entries[slot]) != bool(counts[slot])) return {};
        if (!entries[slot]) continue;
        if (entry) return {};
        entry = entries[slot];
    }
    return ResolveItem(entries, counts, entry);
}
struct Choice
{
    uint8 Slot = 0;
    uint32 Entry = 0, Count = 0;
    PlayerbotItemUsage::Usage Usage = PlayerbotItemUsage::Usage::Unknown;
    std::optional<float> Score;
};
struct Selection
{
    bool Ready = false, NoChoiceRequired = false, ScoreIncomplete = false;
    std::vector<uint8> Candidates;
    std::optional<uint8> Preferred;
};
inline Selection Select(std::vector<Choice> const& choices)
{
    using PlayerbotItemUsage::Usage;
    Selection result;
    // Cata has six native reward-choice slots. Do not compact sparse slot IDs
    // or return an item entry where a future native call requires a slot index.
    std::array<Choice const*, 6> slots{};
    if (choices.size() > slots.size()) return result;
    for (auto const& choice : choices)
    {
        if (choice.Slot >= slots.size() || !choice.Entry || !choice.Count || slots[choice.Slot]) return result;
        slots[choice.Slot] = &choice;
    }
    if (choices.empty()) { result.Ready = true; result.NoChoiceRequired = true; return result; }
    if (choices.size() == 1)
    {
        result.Ready = true; result.Candidates = {choices.front().Slot}; result.Preferred = choices.front().Slot;
        return result; // Donor: a single mandatory reward needs no ranking.
    }
    Usage best = Usage::None;
    for (auto const* choice : slots)
    {
        if (!choice) continue;
        if (choice->Usage == Usage::Unknown) return result; // Unknown is not donor None.
        if (choice->Usage == Usage::Equip || choice->Usage == Usage::Replace) best = Usage::Equip;
        else if (choice->Usage == Usage::BadEquip && best != Usage::Equip) best = Usage::BadEquip;
        else if (choice->Usage != Usage::None && best == Usage::None) best = choice->Usage;
    }
    for (auto const* choice : slots)
        if (choice && (choice->Usage == best || choice->Usage == Usage::Replace))
            result.Candidates.push_back(choice->Slot);
    result.Ready = true;
    if (result.Candidates.size() == 1) { result.Preferred = result.Candidates.front(); return result; }
    float highest = std::numeric_limits<float>::lowest();
    for (uint8 slot : result.Candidates)
    {
        auto score = slots[slot]->Score;
        if (!score || !std::isfinite(*score)) { result.ScoreIncomplete = true; result.Preferred.reset(); return result; }
        if (!result.Preferred || *score > highest) { result.Preferred = slot; highest = *score; }
    }
    return result;
}
struct Survey
{
    bool Available = false;
    ObjectGuid Owner;
    uint32 Quest = 0;
    std::vector<Choice> Choices;
    Selection Ranked;
};
void AddContexts(SharedNamedObjectContextList<UntypedValue>& values);
}
#endif
