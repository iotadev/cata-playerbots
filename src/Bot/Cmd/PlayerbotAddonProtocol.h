/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef CATA_PLAYERBOTS_ADDON_PROTOCOL_H
#define CATA_PLAYERBOTS_ADDON_PROTOCOL_H

#include <string>
#include <cstdint>
#include <limits>
#include <utility>

namespace PlayerbotAddonProtocol
{
// Donor MBOT budget includes the prefix and separating tab. Cata transports
// the prefix separately, leaving 250 bytes for opcode and payload.
constexpr std::size_t MaxMessageBytes = 250;
inline std::string ManagedCapabilities(bool playerLifecycleEnabled)
{
    return playerLifecycleEnabled ? "ALT_ROSTER_V1,BOT_LIFECYCLE_V1" : "";
}
enum class Request { Invalid, Hello, Ping, Roster, AltRoster, Connect, Disconnect, LifecycleState };

struct Parsed
{
    Request Kind = Request::Invalid;
    std::string Payload;
    char const* Error = "UNSUPPORTED_REQUEST";
    uint32_t GuidLow = 0;
};

inline std::string EncodeField(std::string const& value)
{
    char const* hex = "0123456789ABCDEF";
    std::string encoded;
    for (unsigned char c : value)
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.')
            encoded += char(c);
        else
        {
            encoded += '%';
            encoded += hex[c >> 4];
            encoded += hex[c & 15];
        }
    return encoded;
}

inline bool ValidToken(std::string const& token)
{
    if (token.empty() || token.size() > 64)
        return false;
    for (unsigned char c : token)
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' || c == ':'))
            return false;
    return true;
}

inline Parsed Parse(std::string const& message)
{
    if (message.empty() || message.size() > MaxMessageBytes)
        return { Request::Invalid, {}, "BAD_LENGTH" };
    for (unsigned char c : message)
        if (c < 32 || c == 127)
            return { Request::Invalid, {}, "BAD_CHARACTER" };
    if (message == "HELLO~1")
        return { Request::Hello, {}, nullptr };
    if (message.compare(0, 6, "HELLO~") == 0)
        return { Request::Invalid, {}, "BAD_VERSION" };
    if (message == "GET~ROSTER")
        return { Request::Roster, {}, nullptr };
    if (message == "GET~ALT_ROSTER")
        return { Request::AltRoster, {}, nullptr };
    if (message.compare(0, 5, "PING~") == 0)
    {
        std::string token = message.substr(5);
        if (!ValidToken(token))
            return { Request::Invalid, {}, "BAD_TOKEN" };
        return { Request::Ping, token, nullptr };
    }
    Request kind = Request::Invalid;
    std::size_t start = 0;
    for (auto const& candidate : {
        std::pair<char const*, Request>{ "RUN~BOT_CONNECT~", Request::Connect },
        { "RUN~BOT_DISCONNECT~", Request::Disconnect },
        { "GET~BOT_LIFECYCLE_STATE~", Request::LifecycleState } })
    {
        std::string prefix = candidate.first;
        if (message.compare(0, prefix.size(), prefix) == 0)
        {
            kind = candidate.second;
            start = prefix.size();
            break;
        }
    }
    if (kind != Request::Invalid)
    {
        std::size_t separator = message.find('~', start);
        if (separator == std::string::npos || message.find('~', separator + 1) != std::string::npos)
            return { Request::Invalid, {}, "BAD_FIELD_COUNT" };
        uint32_t guid = 0;
        if (separator == start)
            return { Request::Invalid, {}, "BAD_GUID" };
        for (std::size_t i = start; i < separator; ++i)
        {
            unsigned char c = message[i];
            if (c < '0' || c > '9' || guid > (std::numeric_limits<uint32_t>::max() - (c - '0')) / 10)
                return { Request::Invalid, {}, "BAD_GUID" };
            guid = guid * 10 + c - '0';
        }
        if (!guid)
            return { Request::Invalid, {}, "BAD_GUID" };
        std::string token = message.substr(separator + 1);
        if (!ValidToken(token))
            return { Request::Invalid, {}, "BAD_TOKEN" };
        return { kind, token, nullptr, guid };
    }
    return {};
}
}

void SetPlayerbotAddonBridgeEnabled(bool enabled);

#endif
