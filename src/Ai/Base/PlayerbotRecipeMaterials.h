/* RandomItemMgr recipe fallback at 79bd428115c8f33b74b13de23b133d49c3f75c60.
 * GPL v2 or later. Copied positive skill/item facts; see PORTING.md. */
#ifndef PLAYERBOT_RECIPE_MATERIALS_H
#define PLAYERBOT_RECIPE_MATERIALS_H
#include "Define.h"
#include <unordered_set>
namespace PlayerbotRecipeMaterials
{
constexpr uint32 MaxTemplates = 250000;
constexpr uint32 MaxFacts = 100000;
inline uint64 Key(uint32 skill, uint32 item) { return (uint64(skill) << 32) | item; }
class Index
{
public:
    explicit Index(uint32 limit = MaxFacts) : _limit(limit) { }
    bool Add(uint32 skill, uint32 item)
    {
        if (!skill || !item) return false;
        uint64 key = Key(skill, item);
        if (_facts.count(key)) return true;
        if (_facts.size() >= _limit) { Truncated = true; return false; }
        _facts.insert(key); return true;
    }
    bool Contains(uint32 skill, uint32 item) const
    { return skill && item && _facts.count(Key(skill, item)); }
    size_t Size() const { return _facts.size(); }
    bool Truncated = false;
private:
    uint32 _limit;
    std::unordered_set<uint64> _facts;
};
void Rebuild(); // Startup/config-reload owner only; publish copied immutable data.
void Clear(); // Shutdown owner only.
bool Contains(uint32 skill, uint32 item); // Positive-only; absence is unresolved.
}
#endif
