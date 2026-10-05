/*
 * Cata transport for donor ChangeStrategyAction vocabulary at
 * 037c01418b5d01506917a3db9b44fd56ac5f965c. See PORTING.md. GPL v2 or later.
 */
#ifndef PLAYERBOTS_STRATEGY_CONTROL_H
#define PLAYERBOTS_STRATEGY_CONTROL_H
#include <cstdint>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include "PlayerbotAddonProtocol.h"
#include "PlayerbotStrategyBinding.h"

namespace PlayerbotStrategyControl
{
enum class Scope { Combat, NonCombat, Dead };
inline std::set<std::string> const& MutableStrategies(Scope state)
{
    static std::set<std::string> const combat{"focus", "potions", "threat"}, noncombat{"food", "loot"}, dead;
    return state == Scope::Combat ? combat : state == Scope::NonCombat ? noncombat : dead;
}
template <class EngineType>
void RestoreCombatDefaults(EngineType& engine, bool usesThreat)
{
    if (engine.HasStrategy("focus")) engine.RemoveStrategy("focus"); // Donor optional mode; not a class default.
    if (!engine.HasStrategy("potions")) engine.AddStrategy("potions");
    if (usesThreat)
    {
        if (!engine.HasStrategy("threat")) engine.AddStrategy("threat");
    }
    else if (engine.HasStrategy("threat")) engine.RemoveStrategy("threat");
}
struct Command { Scope State; std::string Param; bool Mutation = false; };
inline bool Recognizes(std::string const& text)
{
    return text.size() >= 2 && (text.compare(0, 2, "co") == 0 || text.compare(0, 2, "nc") == 0 || text.compare(0, 2, "de") == 0) &&
        (text.size() == 2 || text[2] == ' ' || text[2] == '\t');
}
inline std::optional<Command> Parse(std::string const& text)
{
    if (!Recognizes(text) || text.size() > 253) return {};
    Command result{text.compare(0, 2, "co") == 0 ? Scope::Combat :
        text.compare(0, 2, "nc") == 0 ? Scope::NonCombat : Scope::Dead, text.size() == 2 ? "?" : text.substr(3)};
    if (result.Param.empty() || result.Param.size() > 250) return {};
    unsigned count = 0;
    for (size_t start = 0; start < result.Param.size();)
    {
        if (++count > 16) return {};
        size_t end = result.Param.find(',', start);
        if (end == std::string::npos) end = result.Param.size();
        std::string token = result.Param.substr(start, end - start);
        size_t first = token.find_first_not_of(" \t"), last = token.find_last_not_of(" \t");
        if (first == std::string::npos) return {};
        token = token.substr(first, last - first + 1);
        if (token != "?")
        {
            if (token.size() < 2 ||
                (token[0] != '+' && token[0] != '-' && token[0] != '~') ||
                !MutableStrategies(result.State).count(token.substr(1))) return {};
            result.Mutation = true;
        }
        if (end == result.Param.size()) break;
        start = end + 1;
        if (start == result.Param.size()) return {};
    }
    return result;
}
class Mailbox
{
public:
    struct Request { uint32_t Requester, Created; Command Value; std::string Token, Target; uint64_t Batch = 0; PlayerbotStrategyBinding Binding; };
    bool Post(uint32_t requester, std::string const& text, uint32_t now, std::string const& token = {},
        std::string const& target = {}, uint64_t batch = 0, PlayerbotStrategyBinding const& binding = {})
    {
        auto command = Parse(text);
        if (!requester || !command) return false;
        if (batch && !binding.Valid()) return false;
        // Group identity is the batch generation, never a fabricated single-bot target.
        if ((batch ? (token.empty() || !target.empty()) : token.empty() != target.empty()) || (!token.empty() &&
            (!PlayerbotAddonProtocol::ValidToken(token) || target.size() > 64 || !command->Mutation))) return false;
        std::lock_guard<std::mutex> lock(mutex);
        if (pending) return false;
        pending = Request{requester, now, std::move(*command), token, target, batch, binding};
        return true;
    }
    std::optional<Request> Take(uint32_t now)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto result = std::move(pending);
        pending.reset();
        if (result && uint32_t(now - result->Created) >= 5000) result.reset();
        return result;
    }
    void Cancel() { std::lock_guard<std::mutex> lock(mutex); pending.reset(); }
private:
    std::mutex mutex;
    std::optional<Request> pending;
};
}
#endif
