/*
 * Adapted from AzerothCore mod-playerbots PlayerbotMgr identity and login
 * ownership at 7bae1b5c58c76a0aa20381155edc08096d1485b2.
 * See AUTHORS.md and PORTING.md. Released under GNU GPL v2 or later.
 */
#include "PlayerbotManagedRoster.h"
#include <algorithm>
#include <charconv>
#include <string_view>
#include <utility>

namespace
{
std::vector<ManagedPlayerbotIdentity> managedIdentities;

std::string_view Trim(std::string_view value)
{
    std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos)
        return {};
    return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

bool ParsePositive(std::string_view text, uint32& value)
{
    text = Trim(text);
    if (text.empty())
        return false;
    auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    return error == std::errc{} && end == text.data() + text.size() && value != 0;
}
}

bool PlayerbotManagedRoster::Configure(std::string const& bindings)
{
    std::vector<ManagedPlayerbotIdentity> parsed;
    std::string_view remaining = Trim(bindings);
    while (!remaining.empty())
    {
        std::size_t separator = remaining.find(',');
        std::string_view item = Trim(remaining.substr(0, separator));
        std::size_t colon = item.find(':');
        uint32 accountId = 0;
        uint32 guidLow = 0;
        if (colon == std::string_view::npos || !ParsePositive(item.substr(0, colon), accountId) ||
            !ParsePositive(item.substr(colon + 1), guidLow) || parsed.size() >= 256 ||
            std::any_of(parsed.begin(), parsed.end(), [accountId, guidLow](ManagedPlayerbotIdentity const& entry)
            {
                return entry.AccountId == accountId || entry.CharacterGuidLow == guidLow;
            }))
        {
            managedIdentities.clear();
            return false;
        }

        parsed.push_back({ accountId, guidLow });
        if (separator == std::string_view::npos)
            break;
        remaining = Trim(remaining.substr(separator + 1));
        if (remaining.empty())
        {
            managedIdentities.clear();
            return false;
        }
    }

    managedIdentities = std::move(parsed);
    return true;
}

std::vector<ManagedPlayerbotIdentity> const& PlayerbotManagedRoster::List()
{
    return managedIdentities;
}

ManagedPlayerbotIdentity const* PlayerbotManagedRoster::Find(uint32 characterGuidLow)
{
    auto found = std::find_if(managedIdentities.begin(), managedIdentities.end(),
        [characterGuidLow](ManagedPlayerbotIdentity const& entry) { return entry.CharacterGuidLow == characterGuidLow; });
    return found == managedIdentities.end() ? nullptr : &*found;
}
