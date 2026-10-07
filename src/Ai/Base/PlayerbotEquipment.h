/* Donor StatsWeightCalculator::CalculateItem / ItemUsageValue::QueryItemUsageForEquip
 * at 037c01418b5d01506917a3db9b44fd56ac5f965c. GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_EQUIPMENT_H
#define PLAYERBOT_EQUIPMENT_H
#include "PlayerbotItemStats.h"
#include <limits>
#include <vector>
namespace PlayerbotEquipment
{
struct WeightModel
{
    std::array<float, static_cast<size_t>(PlayerbotItemStats::Stat::Count)> Values{};
    std::array<bool, static_cast<size_t>(PlayerbotItemStats::Stat::Count)> Mapped{};
    uint8 Class = 0, MinimumLevel = 1, MaximumLevel = 85;
    uint32 Spec = 0;
    PlayerbotItemStats::Profile Profile = PlayerbotItemStats::MeleeDamage;
    bool Qualified = false; // No Wrath table is silently qualified as Cata.
    void Set(PlayerbotItemStats::Stat stat, float weight)
    {
        Values[static_cast<size_t>(stat)] = weight;
        Mapped[static_cast<size_t>(stat)] = true;
    }
};
inline std::optional<float> Score(PlayerbotItemStats::BaseStats const& stats, WeightModel const& weights)
{
    if (!weights.Qualified || !stats.Available || !weights.Class || !weights.Spec ||
        stats.OwnerClass != weights.Class || stats.OwnerSpec != weights.Spec || stats.OwnerProfile != weights.Profile ||
        stats.OwnerLevel < weights.MinimumLevel || stats.OwnerLevel > weights.MaximumLevel ||
        stats.UnsupportedStats || stats.UnsupportedEffects || stats.HasProcEffects || stats.HasUseEffects ||
        stats.HasConditionalEffects || stats.HasSockets || stats.HasItemSet ||
        (stats.HasRandomProperties && (!stats.AffixResolved || (stats.AffixPoolUnverified && !stats.AffixInstanceVerified))))
        return {};
    double total = 0;
    for (size_t i = 0; i < stats.Values.size(); ++i)
    {
        if (!std::isfinite(stats.Values[i]) || (weights.Mapped[i] && !std::isfinite(weights.Values[i]))) return {};
        if (!stats.Values[i]) continue;
        if (!weights.Mapped[i]) return {}; // Unknown is not a zero-weight stat.
        total += double(stats.Values[i]) * weights.Values[i];
    }
    if (!std::isfinite(total) || total > std::numeric_limits<float>::max() || total < std::numeric_limits<float>::lowest()) return {};
    return float(total);
}
enum class Comparison { Unknown, NotUpgrade, Upgrade };
inline Comparison Compare(std::optional<float> candidate, std::optional<float> existing, float threshold)
{
    if (!candidate || !existing || !std::isfinite(*candidate) || !std::isfinite(*existing) ||
        !std::isfinite(threshold) || threshold <= 0) return Comparison::Unknown;
    // Donor requires both a better score and the configured improvement factor.
    return *candidate > 0 && *candidate > *existing && double(*candidate) > double(*existing) * threshold ?
        Comparison::Upgrade : Comparison::NotUpgrade;
}
struct Candidate
{
    ObjectGuid Item, Existing;
    uint32 Entry = 0, ExistingEntry = 0, SuffixFactor = 0;
    int32 Property = 0;
    uint8 Slot = 0;
    bool Broken = false, ExistingBroken = false;
};
struct Snapshot
{
    bool Available = false;
    ObjectGuid Owner;
    uint8 Class = 0, Level = 0;
    uint32 Spec = 0;
    std::vector<Candidate> Candidates;
};
enum class Decision { Unknown, Keep, Upgrade, FillSlot, ReplaceBroken, NeedsRepair };
inline bool TemplateComparable(bool randomProperty, bool randomSuffix, int32 suppliedProperty)
{
    // Template facts cannot prove which affix an actual loot instance received.
    return !randomProperty && !randomSuffix && suppliedProperty == 0;
}
inline Decision Evaluate(std::optional<float> candidate, std::optional<float> existing,
    bool empty, bool sameEntry, bool brokenCandidate, bool brokenExisting, bool layoutKnown, float threshold = 1.1f)
{
    if (!layoutKnown || !candidate || !std::isfinite(*candidate) || !std::isfinite(threshold) || threshold <= 0)
        return Decision::Unknown;
    if (*candidate <= 0) return Decision::Keep;
    if (brokenCandidate) return Decision::NeedsRepair;
    if (empty) return Decision::FillSlot;
    if (!existing || !std::isfinite(*existing)) return Decision::Unknown;
    if (brokenExisting) return Decision::ReplaceBroken;
    if (sameEntry) return Decision::Keep;
    return Compare(candidate, existing, threshold) == Comparison::Upgrade ? Decision::Upgrade : Decision::Keep;
}
struct Evaluation
{
    Candidate Input;
    std::optional<float> CandidateScore, ExistingScore;
    bool LayoutKnown = false;
    Decision Result = Decision::Unknown;
};
struct Survey
{
    bool Available = false;
    ObjectGuid Owner;
    std::vector<Evaluation> Items;
};
void AddContexts(SharedNamedObjectContextList<UntypedValue>& values);
}
#endif
