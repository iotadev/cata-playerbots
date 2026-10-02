/*
 * Native Cata companion loot preference bridge. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOT_LOOT_POLICY_H
#define PLAYERBOT_LOOT_POLICY_H
#include <optional>

namespace PlayerbotLoot
{
// Session/map-owned policy state; no Group/Roll/Player references survive a tick.
class PassPreference final
{
public:
    std::optional<bool> Update(bool enabled, bool current)
    {
        if (enabled)
        {
            if (!_owned)
            {
                _original = current;
                _owned = true;
            }
            return current ? std::nullopt : std::optional<bool>(true);
        }
        if (!_owned)
            return std::nullopt;
        _owned = false;
        return current == _original ? std::nullopt : std::optional<bool>(_original);
    }
private:
    bool _owned = false;
    bool _original = false;
};
}
#endif
