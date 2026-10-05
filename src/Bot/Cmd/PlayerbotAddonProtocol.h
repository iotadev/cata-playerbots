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
inline std::string ManagedCapabilities(bool playerLifecycleEnabled, bool strategyStateEnabled = false, bool strategyMutationEnabled = false)
{
    std::string result = playerLifecycleEnabled ? "ALT_ROSTER_V1,BOT_LIFECYCLE_V1" : "";
    if (strategyStateEnabled) { if (!result.empty()) result += ','; result += "STATE_FRAMING_V1"; }
    if (strategyStateEnabled && strategyMutationEnabled) { if (!result.empty()) result += ','; result += "STRATEGY_MUTATION_V1"; }
    return result;
}
enum class Request { Invalid, Hello, Ping, Roster, AltRoster, Connect, Disconnect, LifecycleState, State, States, Strategy };

struct Parsed
{
    Request Kind = Request::Invalid;
    std::string Payload;
    char const* Error = "UNSUPPORTED_REQUEST";
    uint32_t GuidLow = 0;
    std::string BotName;
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
    if (message.compare(0, 13, "RUN~STRATEGY~") == 0)
        return {Request::Strategy, message.substr(13), nullptr};
    if (message.compare(0, 11, "GET~STATES~") == 0)
    {
        std::string token = message.substr(11);
        return ValidToken(token) ? Parsed{Request::States, token, nullptr} : Parsed{Request::Invalid, {}, "BAD_TOKEN"};
    }
    if (message.compare(0, 10, "GET~STATE~") == 0)
    {
        size_t split = message.find('~', 10);
        if (split == std::string::npos || message.find('~', split + 1) != std::string::npos)
            return {Request::Invalid, {}, "BAD_FIELD_COUNT"};
        std::string name;
        auto hex = [](char c) -> int { return c >= '0' && c <= '9' ? c - '0' :
            c >= 'A' && c <= 'F' ? c - 'A' + 10 : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1; };
        for (size_t i = 10; i < split; ++i)
        {
            unsigned char c = message[i];
            if (c == '%')
            {
                if (i + 2 >= split || hex(message[i + 1]) < 0 || hex(message[i + 2]) < 0)
                    return {Request::Invalid, {}, "BAD_NAME"};
                c = static_cast<unsigned char>(hex(message[i + 1]) * 16 + hex(message[i + 2]));
                i += 2;
            }
            if (c < 128 && !((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')))
                return {Request::Invalid, {}, "BAD_NAME"};
            name += char(c);
            if (name.size() > 64) return {Request::Invalid, {}, "BAD_NAME"};
        }
        if (name.empty()) return {Request::Invalid, {}, "BAD_NAME"};
        std::string token = message.substr(split + 1);
        if (!ValidToken(token)) return {Request::Invalid, {}, "BAD_TOKEN"};
        return {Request::State, token, nullptr, 0, name};
    }
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
