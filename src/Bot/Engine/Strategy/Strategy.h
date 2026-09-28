/*
 * Adapted from AzerothCore mod-playerbots Strategy.h at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOTS_STRATEGY_H
#define PLAYERBOTS_STRATEGY_H

#include "../Action/Action.h"
#include "../NamedObjectContext.h"
#include "../PlayerbotAIAware.h"
#include "../Trigger/Trigger.h"
#include "ObjectGuid.h"
#include <cstdint>
#include <string>
#include <vector>

class Multiplier;

enum class TargetValueExclusionType : uint8_t { None = 0, Tank, Dps, Attacker };

enum StrategyType : uint32_t
{
    STRATEGY_TYPE_GENERIC = 0,
    STRATEGY_TYPE_COMBAT = 1,
    STRATEGY_TYPE_NONCOMBAT = 2,
    STRATEGY_TYPE_TANK = 4,
    STRATEGY_TYPE_DPS = 8,
    STRATEGY_TYPE_HEAL = 16,
    STRATEGY_TYPE_RANGED = 32,
    STRATEGY_TYPE_MELEE = 64
};

static constexpr float ACTION_IDLE = 0.0f;
static constexpr float ACTION_BG = 1.0f;
static constexpr float ACTION_DEFAULT = 5.0f;
static constexpr float ACTION_NORMAL = 10.0f;
static constexpr float ACTION_HIGH = 20.0f;
static constexpr float ACTION_MOVE = 30.0f;
static constexpr float ACTION_INTERRUPT = 40.0f;
static constexpr float ACTION_DISPEL = 50.0f;
static constexpr float ACTION_RAID = 60.0f;
static constexpr float ACTION_LIGHT_HEAL = 10.0f;
static constexpr float ACTION_MEDIUM_HEAL = 20.0f;
static constexpr float ACTION_CRITICAL_HEAL = 30.0f;
static constexpr float ACTION_EMERGENCY = 90.0f;

class Strategy : public PlayerbotAIAware
{
public:
    explicit Strategy(PlayerbotAI* botAI);
    virtual ~Strategy() = default;

    virtual std::vector<NextAction> getDefaultActions() { return {}; }
    virtual void InitTriggers([[maybe_unused]] std::vector<TriggerNode*>& triggers) { }
    virtual void InitMultipliers([[maybe_unused]] std::vector<Multiplier*>& multipliers) { }
    virtual void AppendTargetExclusions([[maybe_unused]] GuidSet& exclusions,
                                        [[maybe_unused]] TargetValueExclusionType type) { }
    virtual bool HasTargetExclusions() const { return false; }
    virtual std::string const getName() = 0;
    virtual uint32_t GetType() const { return STRATEGY_TYPE_GENERIC; }
    virtual ActionNode* GetAction(std::string const name);
    void Update() { }
    void Reset() { }

    NamedObjectFactoryList<ActionNode> actionNodeFactories;
};

#endif
