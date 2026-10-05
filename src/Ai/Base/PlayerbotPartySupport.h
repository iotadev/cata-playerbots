/* Released under GNU GPL v2 or any later version. See AUTHORS.md and PORTING.md. */
#ifndef PLAYERBOT_PARTY_SUPPORT_H
#define PLAYERBOT_PARTY_SUPPORT_H
#include <algorithm>
#include <cstdint>
#include <vector>
#include <charconv>
#include <string_view>
#include "SharedDefines.h"
#include "ObjectGuid.h"

class Player;
class PlayerbotAI;
namespace PlayerbotPartySupport
{
enum class Role { Controller, Healer, Tank, Other };
struct DispelRequest { std::uint32_t Spell = 0, Type = 0; };
inline DispelRequest ResolveDispelRequest(std::uint8_t playerClass, std::string_view qualifier)
{
    if (qualifier.empty() || qualifier.size() > 10) return {};
    std::uint32_t type = 0;
    auto parsed = std::from_chars(qualifier.data(), qualifier.data() + qualifier.size(), type);
    if (parsed.ec != std::errc{} || parsed.ptr != qualifier.data() + qualifier.size()) return {};
    if (playerClass == CLASS_MAGE && type == DISPEL_CURSE) return {475, type};
    if (playerClass == CLASS_PRIEST && type == DISPEL_DISEASE) return {528, type};
    return {}; // Magic/poison and unported classes have no implemented cure route.
}
ObjectGuid DispelTarget(PlayerbotAI& ai, std::uint32_t dispelType);
template <typename Candidate, typename RoleOf, typename SameSubgroup>
void OrderCandidates(std::vector<Candidate>& candidates, RoleOf roleOf, SameSubgroup sameSubgroup)
{
    std::stable_sort(candidates.begin(), candidates.end(), [&](Candidate const& left, Candidate const& right)
    {
        Role leftRole = roleOf(left), rightRole = roleOf(right);
        return leftRole != rightRole ? leftRole < rightRole : sameSubgroup(left) && !sameSubgroup(right);
    });
}
std::vector<Player*> OrderedPartyMembers(Player& bot, Player* owner);
// Map-thread-only intent. The session retains native movement ownership.
enum class ReachKind { None, Heal, Resurrect };
class ReachRequest
{
public:
    void Enable(bool value) { enabled = value; if (!value) pending = ReachKind::None; }
    bool Enabled() const { return enabled; }
    bool Submit(ReachKind kind = ReachKind::Heal)
    {
        if (!enabled || kind == ReachKind::None) return false;
        pending = kind;
        return true;
    }
    ReachKind Take() { ReachKind result = enabled ? pending : ReachKind::None; pending = ReachKind::None; return result; }
private:
    bool enabled = false;
    ReachKind pending = ReachKind::None;
};
inline bool CanCure(bool enabled, bool alive, bool learned, bool nativeEligible)
{
    return enabled && alive && learned && nativeEligible;
}
// Eligibility is supplied by the caller. Unlike healing, cures must not drop
// full-health members. Preserve the caller's donor role order and cast fallback.
template <typename Candidate, typename Attempt>
bool TryCandidates(std::vector<Candidate> const& candidates, Attempt&& attempt)
{
    for (Candidate const& candidate : candidates)
        if (attempt(candidate))
            return true;
    return false;
}
std::vector<Player*> Candidates(Player& bot, Player* owner);
std::vector<Player*> LivingSupportCandidates(Player& bot, Player* owner);
bool HasDispellableAura(Player& bot, Player& target, std::uint32_t spellId, std::uint32_t dispelType);
}
#endif
