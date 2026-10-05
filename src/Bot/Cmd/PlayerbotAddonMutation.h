/*
 * Adapted from mod-multibot-bridge RunStrategyMutationCommand/SendStrategyMutationAck
 * at 1da05982e478cb00e0b6c87314afe7e0e9653ffb. See PORTING.md. GPL v2 or later.
 */
#ifndef PLAYERBOTS_ADDON_MUTATION_H
#define PLAYERBOTS_ADDON_MUTATION_H
#include "PlayerbotAddonProtocol.h"
#include <optional>
#include <vector>

namespace PlayerbotAddonProtocol
{
struct StrategyMutation { std::string Scope, Target, Token, State, Changes; };
inline std::optional<std::string> DecodeMutationField(std::string const& value, size_t limit)
{
    auto hex = [](char c) -> int { return c >= '0' && c <= '9' ? c - '0' :
        c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1; };
    std::string result;
    for (size_t i = 0; i < value.size(); ++i)
    {
        unsigned char c = value[i];
        if (c == '%')
        {
            if (i + 2 >= value.size() || hex(value[i + 1]) < 0 || hex(value[i + 2]) < 0) return {};
            c = static_cast<unsigned char>(hex(value[i + 1]) * 16 + hex(value[i + 2]));
            i += 2;
        }
        if (c < 32 || c == 127) return {};
        result += char(c);
        if (result.size() > limit) return {};
    }
    return result;
}
inline std::optional<StrategyMutation> ParseStrategyMutation(std::string const& payload)
{
    if (payload.size() > MaxMessageBytes - 13) return {};
    std::vector<std::string> fields;
    size_t start = 0;
    for (;;)
    {
        size_t end = payload.find('~', start);
        fields.push_back(payload.substr(start, end == std::string::npos ? end : end - start));
        if (fields.size() > 5) return {};
        if (end == std::string::npos) break;
        start = end + 1;
    }
    if (fields.size() != 5 || !ValidToken(fields[2]) || (fields[3] != "C" && fields[3] != "N")) return {};
    if (fields[0] != "BOT" && fields[0] != "ALL" && fields[0] != "GROUP" && fields[0] != "PARTY" && fields[0] != "RAID") return {};
    auto target = DecodeMutationField(fields[1], 64);
    auto changes = DecodeMutationField(fields[4], 160);
    if (!target || !changes || changes->empty() || (fields[0] == "BOT") != !target->empty()) return {};
    return StrategyMutation{fields[0], *target, fields[2], fields[3], *changes};
}
inline std::string StrategyAck(StrategyMutation const& request, unsigned matched, unsigned succeeded, unsigned failed, std::string const& reason)
{
    if (!ValidToken(request.Token) || matched > 128 || succeeded > matched || failed > matched - succeeded ||
        reason.empty() || reason.size() > 64 || (request.State != "C" && request.State != "N") ||
        (request.Scope != "BOT" && request.Scope != "ALL" && request.Scope != "GROUP" && request.Scope != "PARTY" && request.Scope != "RAID") ||
        (request.Scope == "BOT") != !request.Target.empty()) return {};
    std::string result = "STRATEGY_ACK~" + request.Scope + "~" + EncodeField(request.Target) + "~" + request.Token + "~" + request.State +
        "~" + std::to_string(matched) + "~" + std::to_string(succeeded) + "~" + std::to_string(failed) + "~" + EncodeField(reason);
    return result.size() <= MaxMessageBytes ? result : std::string{};
}
}
#endif
