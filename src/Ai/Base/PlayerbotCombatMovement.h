/* Adapted from donor CombatStrategy, MeleeCombatStrategy and MovementActions
 * at 037c01418b5d01506917a3db9b44fd56ac5f965c. See PORTING.md.
 * Released under GNU GPL v2 or any later version. */
#ifndef PLAYERBOT_COMBAT_MOVEMENT_H
#define PLAYERBOT_COMBAT_MOVEMENT_H
#include "../../Bot/Engine/AiObjectContext.h"
#include "../../Bot/Engine/Strategy/Strategy.h"
#include <algorithm>
#include <cmath>
#include <string_view>
#include <charconv>
#include <locale>
#include <sstream>
#include <mutex>
#include <optional>
class Player;
class Creature;
namespace PlayerbotCombatMovement
{
enum class Step { Facing, Reach, Behind, ReachSpell };
// Donor CanMove checks, expressed without native objects for policy coverage.
// Ghost/vehicle movement is outside the current companion adapter.
struct ControlState
{
    bool InWorld = true, Alive = true;
    bool Teleporting = false, Flight = false, Vehicle = false, Restricted = false;
    bool Charmed = false, Frozen = false, Polymorphed = false, ControlledMotion = false;
};
inline bool CanMove(ControlState const& state)
{
    return state.InWorld && state.Alive && !state.Teleporting && !state.Flight &&
        !state.Vehicle && !state.Restricted && !state.Charmed && !state.Frozen &&
        !state.Polymorphed && !state.ControlledMotion;
}
bool CanMove(Player const& bot);
inline constexpr float CasterDistance = 20.0f; // Existing Cata companion chase envelope.
class RangeValue final : public ManualSetValue<float>, public Qualified
{
public:
    explicit RangeValue(PlayerbotAI* ai) : ManualSetValue<float>(ai, 0.0f, "range") { }
};
inline float ResolveRange(std::string_view type, float value)
{
    float fallback = type == "spell" ? CasterDistance : (type == "heal" ? 30.0f : 0.0f);
    if (!fallback) return 0.0f;
    if (!std::isfinite(value) || value < 0.1f) return fallback;
    return std::clamp(value, 2.0f, type == "spell" ? 25.0f : 30.0f);
}
float GetRange(PlayerbotAI& ai, std::string const& type);
enum class RangeOperation { Invalid, QueryAll, Query, Set };
struct RangeCommand
{
    RangeOperation Operation = RangeOperation::Invalid;
    std::string Type;
    float Value = 0.0f;
};
inline RangeCommand ParseRangeCommand(std::string_view param)
{
    if (param.size() > 64) return {};
    auto trim = [](std::string_view text)
    {
        auto first = text.find_first_not_of(" \t\r\n");
        return first == text.npos ? std::string_view{} :
            text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
    };
    param = trim(param);
    if (param == "?") return {RangeOperation::QueryAll, {}, 0.0f};
    auto split = param.find_first_of(" \t");
    if (split == param.npos) return {};
    std::string_view type = param.substr(0, split), value = trim(param.substr(split));
    if (type != "spell" && type != "heal") return {};
    if (value == "?") return {RangeOperation::Query, std::string(type), 0.0f};
    if (value.empty()) return {};
    float parsed = 0.0f;
    auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    float maximum = type == "spell" ? 25.0f : 30.0f;
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
        !std::isfinite(parsed) || parsed < 0.0f || (parsed != 0.0f && (parsed < 2.0f || parsed > maximum)))
        return {};
    return {RangeOperation::Set, std::string(type), parsed};
}
inline std::string FormatRange(std::string const& type, float stored)
{
    if (type != "spell" && type != "heal") return {};
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << type << " range: " << ResolveRange(type, stored);
    if (!std::isfinite(stored) || stored < 0.1f) out << " (default)";
    return out.str();
}
// One copied request per session; never retains Player/context pointers.
class RangeMailbox
{
public:
    struct Request { uint32 Requester; std::string Param; uint32 Created; };
    bool Post(uint32 requester, std::string const& param, uint32 now)
    {
        if (!requester || ParseRangeCommand(param).Operation == RangeOperation::Invalid) return false;
        std::lock_guard<std::mutex> lock(mutex);
        if (pending) return false;
        pending = Request{requester, param, now};
        return true;
    }
    std::optional<Request> Take(uint32 now)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto result = std::move(pending);
        pending.reset();
        if (result && uint32(now - result->Created) >= 5000) result.reset();
        return result;
    }
private:
    std::mutex mutex;
    std::optional<Request> pending;
};
bool RefreshSpellChase(PlayerbotAI& ai);
void AddRangeContexts(SharedNamedObjectContextList<Action>& actions, SharedNamedObjectContextList<UntypedValue>& values);
struct PositionState
{
    bool InMelee, Facing, Behind, Tanking, Moving, TargetMoving, Chasing;
    bool InSpell = false;
};
inline bool Useful(Step step, PositionState const& state)
{
    switch (step)
    {
        case Step::Facing: return (state.InMelee || state.InSpell) && !state.Facing && !state.Moving;
        case Step::Reach: return !state.InMelee && !state.Chasing;
        case Step::ReachSpell: return !state.InSpell && !state.Chasing;
        case Step::Behind: return state.InMelee && !state.Behind && !state.Tanking &&
            !state.Moving && !state.TargetMoving;
    }
    return false;
}
inline void AddTriggers(std::vector<TriggerNode*>& triggers, bool caster = false)
{
    triggers.push_back(new TriggerNode("not facing target", { NextAction("set facing", ACTION_MOVE + 7) }));
    if (caster)
        triggers.push_back(new TriggerNode("enemy out of spell", { NextAction("reach spell", ACTION_HIGH) }));
    else
    {
        triggers.push_back(new TriggerNode("enemy out of melee", { NextAction("reach melee", ACTION_HIGH + 1) }));
        triggers.push_back(new TriggerNode("not behind target", { NextAction("set behind", ACTION_MOVE + 7) }));
    }
}
bool FaceForAttack(Player& bot, Creature& target);
void AddContexts(SharedNamedObjectContextList<Action>& actions, SharedNamedObjectContextList<Trigger>& triggers,
    SharedNamedObjectContextList<UntypedValue>& values);
}
#endif
