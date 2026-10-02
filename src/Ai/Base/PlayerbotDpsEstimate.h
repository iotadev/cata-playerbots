/* Adapted from donor EstimatedLifetimeValue.cpp, PlayerbotAI gear scoring and
 * PlayerbotFactory::CalcMixedGearScore at 7bae1b5c58c76a0aa20381155edc08096d1485b2.
 * See AUTHORS.md and PORTING.md. Released under GNU GPL v2 or any later version. */
#ifndef PLAYERBOT_DPS_ESTIMATE_H
#define PLAYERBOT_DPS_ESTIMATE_H
#include "Player.h"
#include "ItemTemplate.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>

namespace PlayerbotDpsEstimate
{
inline float QualityMultiplier(uint32 quality)
{
    constexpr std::array<float, 6> multipliers = {1.0f, 1.1f, 1.21f, 1.331f, 1.4641f, 1.61051f};
    return quality < multipliers.size() ? multipliers[quality] : 1.0f;
}
inline float BasicDps(uint32 level)
{
    // Do not extrapolate the Wrath model into the unqualified Cata 81-85 range.
    if (!level || level > 80) return 0.0f;
    if (level <= 15) return 5.0f + level;
    if (level <= 25) return 20.0f + (level - 15) * 2;
    if (level <= 45) return 40.0f + (level - 25) * 3;
    if (level <= 55) return 100.0f + (level - 45) * 20;
    if (level <= 60) return 300.0f + (level - 55) * 50;
    if (level <= 70) return 550.0f + (level - 60) * 65;
    return 1200.0f + (level - 70) * 200;
}
inline float BasicGearScore(uint32 level)
{
    if (!level || level > 80) return 0.0f;
    uint32 itemLevel = level <= 60 ? level + 5 :
        (level <= 70 ? 85 + (level - 60) * 3 : 155 + (level - 70) * 4);
    uint32 quality = level <= 8 ? ITEM_QUALITY_NORMAL :
        (level <= 15 ? ITEM_QUALITY_UNCOMMON : ITEM_QUALITY_RARE);
    return uint32(itemLevel * QualityMultiplier(quality)); // donor truncates the mixed score
}
inline float Contribution(uint32 level, uint32 gearScore, bool tank, bool healer)
{
    float baseline = BasicGearScore(level);
    if (baseline <= 0.0f) return 0.0f;
    float modifier = gearScore / baseline;
    if (gearScore >= 300) modifier *= 1.0f + (gearScore - 300) * 0.01f;
    modifier = std::clamp(modifier, 0.75f, 4.0f);
    return BasicDps(level) * (tank ? 0.3f : (healer ? 0.1f : 1.0f)) * modifier;
}
inline float GroupBonus(std::size_t members)
{
    return members >= 25 ? 1.2f : (members >= 10 ? 1.1f : (members >= 5 ? 1.05f : 1.0f));
}
inline float Lifetime(float health, float dps)
{
    // Unknown estimates are unavailable, not infinity or permission to spend mana.
    if (!(health > 0.0f && dps > 0.0f && std::isfinite(health) && std::isfinite(dps))) return 0.0f;
    float lifetime = health / dps;
    return std::isfinite(lifetime) ? lifetime : 0.0f;
}
inline bool EnoughLifetime(float health, float dps, float minimum)
{
    float lifetime = Lifetime(health, dps);
    return std::isfinite(minimum) && minimum >= 0.0f && lifetime > 0.0f && lifetime >= minimum;
}

// Best usable carried gear per native slot; no equipment mutations or Item pointers.
class GearScores
{
public:
    void Add(uint32 inventoryType, uint32 itemLevel, uint32 quality)
    {
        uint32 score = uint32(itemLevel * QualityMultiplier(quality));
        switch (inventoryType)
        {
            case INVTYPE_2HWEAPON: twoHand = std::max(twoHand, score); return;
            case INVTYPE_WEAPON: case INVTYPE_WEAPONMAINHAND: Best(EQUIPMENT_SLOT_MAINHAND, score); return;
            case INVTYPE_SHIELD: case INVTYPE_WEAPONOFFHAND: case INVTYPE_HOLDABLE: Best(EQUIPMENT_SLOT_OFFHAND, score); return;
            case INVTYPE_THROWN: case INVTYPE_RANGEDRIGHT: case INVTYPE_RANGED:
            case INVTYPE_QUIVER: case INVTYPE_RELIC: Best(EQUIPMENT_SLOT_RANGED, score); return;
            case INVTYPE_HEAD: Best(EQUIPMENT_SLOT_HEAD, score); return;
            case INVTYPE_NECK: Best(EQUIPMENT_SLOT_NECK, score); return;
            case INVTYPE_SHOULDERS: Best(EQUIPMENT_SLOT_SHOULDERS, score); return;
            case INVTYPE_BODY: Best(EQUIPMENT_SLOT_BODY, score); return;
            case INVTYPE_CHEST: case INVTYPE_ROBE: Best(EQUIPMENT_SLOT_CHEST, score); return;
            case INVTYPE_WAIST: Best(EQUIPMENT_SLOT_WAIST, score); return;
            case INVTYPE_LEGS: Best(EQUIPMENT_SLOT_LEGS, score); return;
            case INVTYPE_FEET: Best(EQUIPMENT_SLOT_FEET, score); return;
            case INVTYPE_WRISTS: Best(EQUIPMENT_SLOT_WRISTS, score); return;
            case INVTYPE_HANDS: Best(EQUIPMENT_SLOT_HANDS, score); return;
            case INVTYPE_FINGER: Pair(EQUIPMENT_SLOT_FINGER1, EQUIPMENT_SLOT_FINGER2, score); return;
            case INVTYPE_TRINKET: Pair(EQUIPMENT_SLOT_TRINKET1, EQUIPMENT_SLOT_TRINKET2, score); return;
            case INVTYPE_CLOAK: Best(EQUIPMENT_SLOT_BACK, score); return;
            default: return;
        }
    }
    uint32 TopTwelve() const
    {
        auto sorted = scores;
        if (sorted[EQUIPMENT_SLOT_MAINHAND] + sorted[EQUIPMENT_SLOT_OFFHAND] < twoHand * 2)
            sorted[EQUIPMENT_SLOT_MAINHAND] = sorted[EQUIPMENT_SLOT_OFFHAND] = twoHand;
        std::sort(sorted.begin(), sorted.end(), std::greater<uint32>());
        uint32 sum = 0;
        for (std::size_t i = 0; i < 12; ++i) sum += sorted[i];
        return sum / 12;
    }
private:
    void Best(std::size_t slot, uint32 score) { scores[slot] = std::max(scores[slot], score); }
    void Pair(std::size_t first, std::size_t second, uint32 score)
    {
        if (score > scores[first]) { scores[second] = scores[first]; scores[first] = score; }
        else Best(second, score);
    }
    std::array<uint32, EQUIPMENT_SLOT_END> scores{};
    uint32 twoHand = 0;
};
}
#endif
